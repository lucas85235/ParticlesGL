#version 430 core
#extension GL_ARB_shader_draw_parameters : require

// Geometry base (mesh vertex, same for all instances)
layout(location = 0) in vec3 aPos;

// ── Per-instance data read directly from SSBOs (Global Pool) ───────────────
layout(std430, binding = 0) buffer Positions { vec4 positions[]; };
layout(std430, binding = 4) buffer Colors    { vec4 colors[];    };
layout(std430, binding = 7) buffer Scales    { float scales[];   };

uniform mat4 u_ViewProjection;

out vec4 vColor;

void main() {
    // MultiDrawElementsIndirect sets gl_BaseInstanceARB to the emitter's pool_offset!
    uint global_idx = gl_BaseInstanceARB + uint(gl_InstanceID);

    vec3 pos    = positions[global_idx].xyz;
    float scale = scales[global_idx];

    vColor      = colors[global_idx];
    
    // Simple billboard-like or mesh scaling
    vec3 world  = (aPos * scale) + pos;
    gl_Position = u_ViewProjection * vec4(world, 1.0);
}
