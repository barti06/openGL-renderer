#version 460 core
layout (location = 0) in vec3 aPos;

uniform mat4 u_projection;
uniform mat4 u_view;

out vec3 v_worldpos;

void main()
{
    v_worldpos = aPos;

	mat4 rotView = mat4(mat3(u_view));
	vec4 clipPos =   u_projection * rotView * vec4(v_worldpos, 1.0);

	gl_Position = clipPos.xyww;
}
