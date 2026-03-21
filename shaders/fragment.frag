#version 460 core
out vec4 FragColor;

in vec2 uv;

uniform sampler2D u_albedo;

void main()
{
    //FragColor = vec4(1.0, 0.5, 0.3, 1.0);
    FragColor = texture(u_albedo, uv);
} 