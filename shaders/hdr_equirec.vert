#version 460 core
layout (location = 0) in vec3 aPos;

out vec3 v_local_pos;

uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    v_local_pos = aPos;
    
    gl_Position = u_projection * u_view *  vec4(v_local_pos, 1.0);
}
