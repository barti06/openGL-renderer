#version 460 core

in vec2 v_uv;

uniform sampler2D u_albedo;

void main() 
{
    vec4 color = texture(u_albedo, v_uv);
    if(color.a < 0.1)
        discard;
}
