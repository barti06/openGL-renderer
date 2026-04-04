#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 v_uv;

void main()
{
    v_uv = aTexCoords;
	gl_Position = vec4(aPos, 1.0);
}
