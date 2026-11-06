__kernel void detect_collisions(
    __global const float4* boxes,
    __global uint* overlaps,
    const uint pair_count)
{
    const uint i = get_global_id(0);
    if (i >= pair_count) return;

    const float4 a = boxes[i * 2];
    const float4 b = boxes[i * 2 + 1];
    overlaps[i] = (a.x < b.z && a.z > b.x &&
                   a.y < b.w && a.w > b.y) ? 1u : 0u;
}
