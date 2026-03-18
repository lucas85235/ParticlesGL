#version 430 core

// Geometry base (mesh vertex, same for all instances)
layout(location = 0) in vec3 aPos;

// ── Per-instance data read directly from SSBOs — no VBOs needed ───────────
// std430 layout matches GpuParticleBuffer SSBO layout exactly.
layout(std430, binding = 0) buffer Positions { vec4 positions[]; };
layout(std430, binding = 4) buffer Colors    { vec4 colors[];    };
layout(std430, binding = 7) buffer Scales    { float scales[];   };

uniform mat4 u_ViewProjection;

out vec4 vColor;

void main() {
    uint i      = uint(gl_InstanceID);
    vec3 pos    = positions[i].xyz;
    float scale = scales[i];

    vColor      = colors[i];
    vec3 world  = (aPos * scale) + pos;
    gl_Position = u_ViewProjection * vec4(world, 1.0);
}
