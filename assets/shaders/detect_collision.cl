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
