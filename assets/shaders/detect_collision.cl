inline float2 read_point(__global const float* vertices, const uint index)
{
    return (float2)(vertices[(size_t)index * 2], vertices[(size_t)index * 2 + 1]);
}

inline float cross2(const float2 a, const float2 b)
{
    return a.x * b.y - a.y * b.x;
}

inline int point_in_triangle(
    const float2 point,
    const float2 a,
    const float2 b,
    const float2 c,
    const float orientation)
{
    const float epsilon = 1.0e-6f;
    return orientation * cross2(b - a, point - a) >= -epsilon &&
           orientation * cross2(c - b, point - b) >= -epsilon &&
           orientation * cross2(a - c, point - c) >= -epsilon;
}

__kernel void triangulate_polygons(
    __global const float* vertices,
    __global const uint* polygon_offsets,
    __global const uint* polygon_counts,
    __global const uint* triangle_offsets,
    __global uint* output_triangles,
    __global uint* output_triangle_counts,
    __global uint* scratch,
    __global uint* statuses,
    const uint polygon_count)
{
    const uint polygon_index = get_global_id(0);
    if (polygon_index >= polygon_count) return;

    const uint vertex_offset = polygon_offsets[polygon_index];
    const uint input_count = polygon_counts[polygon_index];
    const uint output_offset = triangle_offsets[polygon_index] * 3;
    output_triangle_counts[polygon_index] = 0;
    statuses[polygon_index] = 0;
    if (input_count < 3) {
        statuses[polygon_index] = 1;
        return;
    }

    uint active_count = 0;
    const float epsilon_squared = 1.0e-12f;
    for (uint i = 0; i < input_count; ++i) {
        const uint source_index = vertex_offset + i;
        if (active_count == 0) {
            scratch[vertex_offset + active_count++] = source_index;
            continue;
        }
        const float2 delta = read_point(vertices, source_index) -
                             read_point(vertices, scratch[vertex_offset + active_count - 1]);
        if (dot(delta, delta) > epsilon_squared) {
            scratch[vertex_offset + active_count++] = source_index;
        }
    }
    if (active_count > 1) {
        const float2 closing_delta =
            read_point(vertices, scratch[vertex_offset]) -
            read_point(vertices, scratch[vertex_offset + active_count - 1]);
        if (dot(closing_delta, closing_delta) <= epsilon_squared) {
            --active_count;
        }
    }

    int removed_collinear = 1;
    while (removed_collinear && active_count > 3) {
        removed_collinear = 0;
        for (uint i = 0; i < active_count; ++i) {
            const uint previous_index =
                scratch[vertex_offset + (i + active_count - 1) % active_count];
            const uint current_index = scratch[vertex_offset + i];
            const uint next_index = scratch[vertex_offset + (i + 1) % active_count];
            const float2 incoming = read_point(vertices, current_index) -
                                    read_point(vertices, previous_index);
            const float2 outgoing = read_point(vertices, next_index) -
                                    read_point(vertices, current_index);
            if (fabs(cross2(incoming, outgoing)) <= 1.0e-6f &&
                dot(incoming, outgoing) >= 0.0f) {
                for (uint j = i; j + 1 < active_count; ++j) {
                    scratch[vertex_offset + j] = scratch[vertex_offset + j + 1];
                }
                --active_count;
                removed_collinear = 1;
                break;
            }
        }
    }
    if (active_count < 3) {
        statuses[polygon_index] = 1;
        return;
    }

    float signed_area = 0.0f;
    for (uint i = 0; i < active_count; ++i) {
        const float2 a = read_point(
            vertices, scratch[vertex_offset + i]);
        const float2 b = read_point(
            vertices, scratch[vertex_offset + (i + 1) % active_count]);
        signed_area += cross2(a, b);
    }
    if (fabs(signed_area) <= 2.0e-6f) {
        statuses[polygon_index] = 1;
        return;
    }
    const float orientation = signed_area > 0.0f ? 1.0f : -1.0f;

    uint triangle_count = 0;
    while (active_count > 3) {
        int clipped_ear = 0;
        for (uint i = 0; i < active_count; ++i) {
            const uint previous_position = (i + active_count - 1) % active_count;
            const uint next_position = (i + 1) % active_count;
            const uint previous_index = scratch[vertex_offset + previous_position];
            const uint current_index = scratch[vertex_offset + i];
            const uint next_index = scratch[vertex_offset + next_position];
            const float2 previous = read_point(vertices, previous_index);
            const float2 current = read_point(vertices, current_index);
            const float2 next = read_point(vertices, next_index);
            if (orientation * cross2(current - previous, next - current) <= 1.0e-6f) {
                continue;
            }

            int contains_vertex = 0;
            for (uint candidate = 0; candidate < active_count; ++candidate) {
                if (candidate == previous_position || candidate == i ||
                    candidate == next_position) {
                    continue;
                }
                const float2 point = read_point(
                    vertices, scratch[vertex_offset + candidate]);
                if (point_in_triangle(point, previous, current, next, orientation)) {
                    contains_vertex = 1;
                    break;
                }
            }
            if (contains_vertex) {
                continue;
            }

            const uint output_base = output_offset + triangle_count * 3;
            output_triangles[output_base] = previous_index;
            output_triangles[output_base + 1] = current_index;
            output_triangles[output_base + 2] = next_index;
            ++triangle_count;
            for (uint j = i; j + 1 < active_count; ++j) {
                scratch[vertex_offset + j] = scratch[vertex_offset + j + 1];
            }
            --active_count;
            clipped_ear = 1;
            break;
        }
        if (!clipped_ear) {
            output_triangle_counts[polygon_index] = 0;
            return;
        }
    }

    const float2 last_a = read_point(vertices, scratch[vertex_offset]);
    const float2 last_b = read_point(vertices, scratch[vertex_offset + 1]);
    const float2 last_c = read_point(vertices, scratch[vertex_offset + 2]);
    if (orientation * cross2(last_b - last_a, last_c - last_b) <= 1.0e-6f) {
        output_triangle_counts[polygon_index] = 0;
        return;
    }
    const uint final_base = output_offset + triangle_count * 3;
    output_triangles[final_base] = scratch[vertex_offset];
    output_triangles[final_base + 1] = scratch[vertex_offset + 1];
    output_triangles[final_base + 2] = scratch[vertex_offset + 2];
    output_triangle_counts[polygon_index] = triangle_count + 1;
    statuses[polygon_index] = 1;
}

__kernel void detect_sat_triangle_pairs(
    __global const float* vertices,
    __global float4* contacts,
    const uint pair_count)
{
    const uint pair_index = get_global_id(0);
    if (pair_index >= pair_count) return;

    const size_t base = (size_t)pair_index * 12;
    float2 first[3];
    float2 second[3];
    for (uint i = 0; i < 3; ++i) {
        first[i] = (float2)(vertices[base + i * 2], vertices[base + i * 2 + 1]);
        second[i] = (float2)(vertices[base + 6 + i * 2], vertices[base + 6 + i * 2 + 1]);
    }

    float minimum_overlap = INFINITY;
    float2 minimum_axis = (float2)(0.0f, 0.0f);

    for (uint edge_index = 0; edge_index < 6; ++edge_index) {
        const uint polygon_index = edge_index < 3 ? 0 : 1;
        const uint edge = edge_index % 3;
        const float2 a0 = polygon_index == 0 ? first[edge] : second[edge];
        const float2 a1 = polygon_index == 0 ? first[(edge + 1) % 3] : second[(edge + 1) % 3];
        const float2 direction = a1 - a0;
        const float edge_length = length(direction);
        if (edge_length <= 1.0e-6f) continue;

        const float2 axis = (float2)(direction.y, -direction.x) / edge_length;
        float first_min = dot(first[0], axis);
        float first_max = first_min;
        float second_min = dot(second[0], axis);
        float second_max = second_min;
        for (uint i = 1; i < 3; ++i) {
            const float first_projection = dot(first[i], axis);
            const float second_projection = dot(second[i], axis);
            first_min = fmin(first_min, first_projection);
            first_max = fmax(first_max, first_projection);
            second_min = fmin(second_min, second_projection);
            second_max = fmax(second_max, second_projection);
        }

        const float positive_overlap = first_max - second_min;
        const float negative_overlap = second_max - first_min;
        if (positive_overlap <= 1.0e-6f || negative_overlap <= 1.0e-6f) {
            contacts[pair_index] = (float4)(0.0f);
            return;
        }
        const float overlap = fmin(positive_overlap, negative_overlap);
        if (overlap < minimum_overlap) {
            minimum_overlap = overlap;
            minimum_axis = positive_overlap < negative_overlap ? axis : -axis;
        }
    }

    if (!isfinite(minimum_overlap)) {
        contacts[pair_index] = (float4)(0.0f);
        return;
    }

    contacts[pair_index] = (float4)(minimum_axis.x, minimum_axis.y, minimum_overlap, 1.0f);
}
