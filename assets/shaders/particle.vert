#version 430 core
layout (location = 0) in vec3 aPos;
// Instance attributes
layout (location = 1) in vec3 aInstancePos;
layout (location = 2) in float aInstanceScale;
layout (location = 3) in vec4 aInstanceColor;

uniform mat4 u_ViewProjection;
out vec4 vColor;

void main() {
    vColor = aInstanceColor;
    vec3 scaledPos = (aPos * aInstanceScale) + aInstancePos;
    gl_Position = u_ViewProjection * vec4(scaledPos, 1.0);
}
