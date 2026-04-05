#version 460 core
out vec3 downSample;
in vec2 v_uv;

uniform sampler2D u_src;
uniform vec2 u_src_texel_size; // 1.0 / src resolution
uniform float u_threshold;
uniform bool u_use_threshold;

void main()
{
    vec2 ts = u_src_texel_size;

    // 13-tap filter (Jimenez 2014)
    // take samples in a cross pattern + 4 box samples
    vec3 a = texture(u_src, v_uv + vec2(-2, 2) * ts).rgb;
    vec3 b = texture(u_src, v_uv + vec2( 0, 2) * ts).rgb;
    vec3 c = texture(u_src, v_uv + vec2( 2, 2) * ts).rgb;
    vec3 d = texture(u_src, v_uv + vec2(-2, 0) * ts).rgb;
    vec3 e = texture(u_src, v_uv).rgb;
    vec3 f = texture(u_src, v_uv + vec2( 2, 0) * ts).rgb;
    vec3 g = texture(u_src, v_uv + vec2(-2,-2) * ts).rgb;
    vec3 h = texture(u_src, v_uv + vec2( 0,-2) * ts).rgb;
    vec3 i = texture(u_src, v_uv + vec2( 2,-2) * ts).rgb;
    vec3 j = texture(u_src, v_uv + vec2(-1, 1) * ts).rgb;
    vec3 k = texture(u_src, v_uv + vec2( 1, 1) * ts).rgb;
    vec3 l = texture(u_src, v_uv + vec2(-1,-1) * ts).rgb;
    vec3 m = texture(u_src, v_uv + vec2( 1,-1) * ts).rgb;

    // weighted average (inner 2x2 boxes get 0.5 and outer corners get 0.125)
    vec3 result = e * 0.125;
    result += (a + c + g + i) * 0.03125;
    result += (b + d + f + h) * 0.0625;
    result += (j + k + l + m) * 0.125;

    // only threshold at the first downsample (from full res scene)
    if(u_use_threshold)
    {
        float brightness = max(result.r, max(result.g, result.b));
        float soft = clamp(brightness - u_threshold, 0.0, 1.0);
        soft = (soft * soft) / (2.0 * u_threshold + 0.0001);
        float contribution = max(soft, brightness - u_threshold) / max(brightness, 0.0001);
        result *= contribution;
    }

    downSample = result;
}
