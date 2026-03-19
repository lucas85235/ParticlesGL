#version 430 core
#extension GL_ARB_shader_draw_parameters : require

// Geometry base (mesh vertex, same for all instances)
layout(location = 0) in vec3 aPos;

// ── Per-instance data read directly from SSBOs (Global Pool) ───────────────
layout(std430, binding = 0)  buffer Positions     { vec4  positions[];    };
layout(std430, binding = 4)  buffer Colors        { vec4  colors[];       };
layout(std430, binding = 7)  buffer Scales        { float scales[];       };
// Phase 6: sorted index buffer (binding 11). When the sort is not active,
// this is an identity mapping so behaviour is identical to the unsorted path.
layout(std430, binding = 11) buffer SortedIndices { uint  sortedIndices[];};

uniform mat4 u_ViewProjection;

out vec4 vColor;

void main() {
    // gl_BaseInstanceARB holds the emitter's pool_offset (set by indirect draw).
    uint sorted_slot = gl_BaseInstanceARB + uint(gl_InstanceID);

    // Indirect index lookup: for Additive emitters the sortedIndices buffer is
    // the identity range, so this has zero cost. For Alpha emitters it yields
    // the back-to-front ordering produced by the GPU Radix Sort.
    uint global_idx = sortedIndices[sorted_slot];

    vec3 pos    = positions[global_idx].xyz;
    float scale = scales[global_idx];

    vColor      = colors[global_idx];

    // Simple billboard-like or mesh scaling
    vec3 world  = (aPos * scale) + pos;
    gl_Position = u_ViewProjection * vec4(world, 1.0);
}
