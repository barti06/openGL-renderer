#version 460 core
out float fragColor;
in vec2 v_uv;

uniform sampler2D u_ssao;

void main()
{
    vec2 texel_size = 1.0 / vec2(textureSize(u_ssao, 0));
    float result = 0.0;
    // simple 4x4 box blur
    for(int x = -2; x < 2; x++)
    {
        for(int y = -2; y < 2; y++)
        {
            vec2 offset = vec2(float(x), float(y)) * texel_size;
            result += texture(u_ssao, v_uv + offset).r;
        }
    }
    fragColor = result / 16.0;
}
