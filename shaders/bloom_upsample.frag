#version 460 core
out vec3 upSample;
in vec2 v_uv;

uniform sampler2D u_src;
uniform float u_filter_radius;

void main()
{
    // 3x3 tent filter
    float x = u_filter_radius;
    float y = u_filter_radius;

    vec3 a = texture(u_src, vec2(v_uv.x - x, v_uv.y + y)).rgb;
    vec3 b = texture(u_src, vec2(v_uv.x, v_uv.y + y)).rgb;
    vec3 c = texture(u_src, vec2(v_uv.x + x, v_uv.y + y)).rgb;
    vec3 d = texture(u_src, vec2(v_uv.x - x, v_uv.y)).rgb;
    vec3 e = texture(u_src, vec2(v_uv.x, v_uv.y)).rgb;
    vec3 f = texture(u_src, vec2(v_uv.x + x, v_uv.y)).rgb;
    vec3 g = texture(u_src, vec2(v_uv.x - x, v_uv.y - y)).rgb;
    vec3 h = texture(u_src, vec2(v_uv.x, v_uv.y - y)).rgb;
    vec3 i = texture(u_src, vec2(v_uv.x + x, v_uv.y - y)).rgb;

    // tent weights (center=4 edges=2 corners=1 sum=16)
    vec3 result = e * 4.0;
    result += (b + d + f + h) * 2.0;
    result += (a + c + g + i);
    result /= 16.0;

    upSample = result;
}
