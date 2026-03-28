#version 460 core
#define PI 3.14159265359
#define MAX_POINTLIGHT_COUNT 64

out vec4 fragColor;

in vec2 v_uv;
in vec2 v_uv2;
in mat3 v_TBN;
in vec3 v_fragment_pos;

uniform sampler2D u_albedo;
uniform sampler2D u_metallic_roughness;
uniform sampler2D u_normal;
uniform sampler2D u_emissive;
uniform sampler2D u_ao;
uniform sampler2D u_volume_thickness;

uniform bool u_has_albedo;
uniform bool u_has_metallic_roughness;
uniform bool u_has_normal;
uniform bool u_has_emissive;
uniform bool u_has_emissive_texcoord;
uniform bool u_has_ao;
uniform bool u_has_occlusion_texcoord;
uniform bool u_has_volume;
uniform bool u_unlit;

uniform vec4 u_albedo_factor;
uniform float u_metallic_factor;
uniform float u_roughness_factor;
uniform float u_occlusion_strength;
uniform vec2 u_occlusion_scale;
uniform vec2 u_normal_scale;
uniform float u_emissive_strength;
uniform vec3 u_emissive_factor;
uniform float u_volume_thickness_factor;
uniform float u_volume_attenuation_distance;
uniform vec3 u_volume_attenuation_color;

struct point_light
{
    vec3 position;
    float linear;
    float quadratic;
    vec3 diffuse;
    float intensity;
};

uniform vec3 u_camera_position;
uniform int u_pointLight_count;
uniform point_light u_pointlights[MAX_POINTLIGHT_COUNT];

float D_GGX(float NdotH, float roughness);
float G_SchlickGGX(float NdotX, float roughness);
float G_Smith(float NdotV, float NdotL, float roughness);
vec3 F_Schlick(float cosTheta, vec3 F0);
vec3 PBR(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic, float roughness, vec3 radiance, float NdotV);
vec3 get_point_light(point_light light, vec3 N, vec3 fragPos, vec3 V, vec3 albedo, float metallic, float roughness, float NdotV);

void main()
{
    vec4 color = u_has_albedo ? texture(u_albedo, v_uv) * u_albedo_factor : u_albedo_factor;
    if(color.a < 0.1 && !u_has_volume)
        discard;

    vec3 albedo = pow(color.rgb, vec3(2.2));

    vec2 emissive_uvs = u_has_emissive_texcoord ? v_uv2 : v_uv;
    vec3 emissive = u_has_emissive ? pow(texture(u_emissive, emissive_uvs).rgb, vec3(2.2)) * u_emissive_factor * u_emissive_strength : u_emissive_factor * u_emissive_strength;

    vec3 normal = u_has_normal ? texture(u_normal, v_uv * u_normal_scale).rgb * 2.0 - 1.0 : vec3(0.0, 0.0, 1.0);
    normal = normalize(v_TBN * normal);

    vec2 ao_uvs = u_has_occlusion_texcoord ? v_uv2 : v_uv;
    float ao = u_has_ao ? texture(u_ao, ao_uvs * u_occlusion_scale).r * u_occlusion_strength : 1.0;

    vec3 metal_rough = u_has_metallic_roughness ? texture(u_metallic_roughness, v_uv).rgb : vec3(0.0);
    float roughness = u_has_metallic_roughness ? metal_rough.g * u_roughness_factor : u_roughness_factor;
    float metallic  = u_has_metallic_roughness ? metal_rough.b * u_metallic_factor  : u_metallic_factor;

    vec3 view = normalize(u_camera_position - v_fragment_pos);
    float NdotV = max(dot(normal, view), 0.0);

    vec3 total_light = vec3(0.1) * albedo * ao; // ambient
    for(int i = 0; i < u_pointLight_count; i++)
    {
        point_light pl = u_pointlights[i];
        pl.diffuse *= pl.intensity;
        total_light += get_point_light(pl, normal, v_fragment_pos, view, albedo, metallic, roughness, NdotV);
    }

    float out_alpha = color.a;
    if(u_has_volume && u_volume_attenuation_distance > 0.0)
    {
        float thickness = texture(u_volume_thickness, v_uv).g * u_volume_thickness_factor;
        vec3 attenuation = exp(-thickness / u_volume_attenuation_distance * (1.0 - u_volume_attenuation_color));
        total_light *= attenuation;
        out_alpha = color.a > 0.0 ? color.a : 0.5;
    }

    fragColor = vec4(total_light + emissive, out_alpha);
}

float D_GGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d + 1e-7);
}

float G_SchlickGGX(float NdotX, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k + 1e-7);
}

float G_Smith(float NdotV, float NdotL, float roughness)
{
    return G_SchlickGGX(NdotV, roughness) * G_SchlickGGX(NdotL, roughness);
}

vec3 F_Schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

// shared Cook-Torrance evaluator
vec3 PBR(vec3 N, vec3 V, vec3 L,
         vec3 albedo, float metallic, float roughness,
         vec3 radiance, float NdotV)
{
    vec3 H = normalize(L + V);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    // F0: 0.04 for dielectrics, albedo for metals
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    float D = D_GGX(NdotH, roughness);
    float G = G_Smith(NdotV, NdotL, roughness);
    vec3 F = F_Schlick(HdotV, F0);

    // specularity
    vec3 spec = (D * G * F) / (4.0 * NdotV * NdotL + 1e-4);

    // diffuse
    vec3 kD = (1.0 - F) * (1.0 - metallic);
    vec3 diff = kD * albedo / PI;

    return (diff + spec) * radiance * NdotL;
}

vec3 get_point_light(point_light light, vec3 N, vec3 fragPos, vec3 V, vec3 albedo, float metallic, float roughness, float NdotV)
{
    vec3 toLight = light.position - fragPos;
    float dist = length(toLight);
    vec3 L = toLight / dist;

    float atten = 1.0 / (1.0 + light.linear * dist + light.quadratic * dist * dist);
    vec3 radiance = light.diffuse * atten;

    return PBR(N, V, L, albedo, metallic, roughness, radiance, NdotV);
}
