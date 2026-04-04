#version 460 core

out vec4 fragColor;

in vec3 v_local_pos;

uniform samplerCube u_environmentMap;
uniform float u_roughness;

#define PI 3.14159265359

vec3 ggx_imp_sample(vec2 Xi, vec3 N, float roughness);

vec2 hammersley(uint i, uint N);

float vdc_radicalInv(uint bits);

void main()
{
    vec3 N = normalize(v_local_pos);
    vec3 R = N;
    vec3 V = R;

    const uint sample_count = 1024u;
    float total_weight = 0.0;
    vec3 prefiler_color = vec3(0.0);
    for(uint i = 0u; i < sample_count; i++)
    {
        vec2 Xi = hammersley(i, sample_count);
        vec3 H = ggx_imp_sample(Xi, N, u_roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            prefiler_color += texture(u_environmentMap, L).rgb * NdotL;
            total_weight += NdotL;
        }
    }
    prefiler_color = prefiler_color / total_weight;

    fragColor = vec4(prefiler_color, 1.0);
}

// fast van der corput radical inverse from http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html
float vdc_radicalInv(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), vdc_radicalInv(i));
}

vec3 ggx_imp_sample(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness*roughness;
	
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
	
    // spherical to cartesian
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
	
    // from tangent space vector to world space sample vector
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
	
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}
