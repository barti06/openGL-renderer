#version 460 core
out float fragColor;

in vec2 v_uv;

uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_depth;
uniform sampler2D u_noise;

uniform vec3 u_samples[64];
uniform mat4 u_projection;
uniform mat4 u_view;
uniform vec2 u_noise_scale;  // usually one fourth of screen size
uniform float u_radius = 0.5;
uniform float u_bias = 0.01;
uniform float u_strength = 1.3;

const int KERNEL_SIZE = 64;

void main()
{
    float depth = texture(g_depth, v_uv).r;
    // skip background pixels
    if(depth == 1.0)
    {
        fragColor = 1.0;
        return;
    }

    // get fragment pos and normal
    vec3 frag_pos = texture(g_position, v_uv).xyz; 
    vec3 normal = texture(g_normal, v_uv).xyz;

    // tile the noise texture across the screen
    vec3 random = normalize(texture(u_noise, v_uv * u_noise_scale).rgb);

    // build TBN matrix to orient hemisphere samples along the surface normal
    vec3 tangent = normalize(random - normal * dot(random, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for(int i = 0; i < KERNEL_SIZE; i++)
    {
        // transform sample from tangent space to world space
        vec3 sample_pos = TBN * u_samples[i];
        sample_pos = frag_pos + sample_pos * u_radius;

        // project sample to get its screen space UV
        vec4 offset = vec4(sample_pos, 1.0);
        offset = u_projection * offset;
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        vec3 sample_world = texture(g_position, offset.xy).rgb;
        // get the actual depth at the sample's screen position
        float sample_depth = texture(g_position, offset.xy).z;

        // only count occlusion from nearby geometry
        float range_check = smoothstep(0.0, 1.0, u_radius / abs(frag_pos.z - sample_depth));

        // if sample_depth is closer to camera than the sample there is occlusion
        occlusion += (sample_depth >= sample_pos.z + u_bias ? 1.0 : 0.0) * range_check;
    }

    // 1.0 is no occlusion and 0.0 is fully occluded
    occlusion = 1.0 - (occlusion / float(KERNEL_SIZE)) * u_strength;
    fragColor = occlusion;
}
