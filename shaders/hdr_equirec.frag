#version 460 core
out vec4 FragColor;

in vec3 v_local_pos;

uniform sampler2D u_equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 sample_spherical_map(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{		
    vec2 uv = sample_spherical_map(normalize(v_local_pos)); // make sure to normalize localPos
    vec3 color = texture(u_equirectangularMap, uv).rgb;
    
    color = color / (color + vec3(1.0)); // reinhard

    FragColor = vec4(color, 1.0);
}
