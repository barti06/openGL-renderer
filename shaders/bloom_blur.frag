#version 460 core

out vec4 FragColor;

in vec2 v_uv;

uniform sampler2D u_image;
uniform bool u_horizontal;

// 9-tap Gaussian weights
const float weights[5] = float[](0.227027, 0.194595, 0.121622, 0.054054, 0.016216);

void main()
{
    vec2 tex_offset = 1.0 / textureSize(u_image, 0);
    vec3 result = texture(u_image, v_uv).rgb * weights[0];

    if(u_horizontal)
    {
        for(int i = 1; i < 5; i++)
        {
            result += texture(u_image, v_uv + vec2(tex_offset.x * i, 0.0)).rgb * weights[i];
            result += texture(u_image, v_uv - vec2(tex_offset.x * i, 0.0)).rgb * weights[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; i++)
        {
            result += texture(u_image, v_uv + vec2(0.0, tex_offset.y * i)).rgb * weights[i];
            result += texture(u_image, v_uv - vec2(0.0, tex_offset.y * i)).rgb * weights[i];
        }
    }
    FragColor = vec4(result, 1.0);
}
