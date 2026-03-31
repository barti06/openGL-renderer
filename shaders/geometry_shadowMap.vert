#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aUv;

uniform mat4 u_lightSpaceMat;
uniform mat4 u_model;

out vec2 v_uv;

void main()
{
    v_uv = aUv;
    gl_Position = u_lightSpaceMat * u_model * vec4(aPos, 1.0);
}
