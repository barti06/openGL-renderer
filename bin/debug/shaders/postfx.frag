#version 460 core
out vec4 FragColor;

in vec2 v_uv;

uniform sampler2D u_color_tex;
uniform sampler2D u_depth_tex;

void main()
{
    vec3 col = texture(u_color_tex, v_uv).rgb;
    float dep = texture(u_depth_tex, v_uv).r;
    FragColor = vec4(col, 1.0);
} 