#version 460 core
#define PI 3.14159265359
#define MAX_POINTLIGHT_COUNT 64

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 brightColor;


in vec2 v_uv;

struct point_light
{
    // light position
    vec3 position;
    
    // for calculating point light's attenuation
    float linear;
    float quadratic;  

    // light color
    vec3 diffuse;

    float intensity;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;

    // this values MUST be passed as cosines
    float cutOff;
    float outerCutOff;

    // for calculating point light's attenuation
    float linear;
    float quadratic;  
    
    vec3 diffuse;
};

uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_albedo;
uniform sampler2D g_orm;
uniform sampler2D g_emissive;
uniform sampler2D g_depth;

uniform vec3 u_camera_position;
uniform int u_gbuffer_view;

uniform int u_pointLight_count;
uniform point_light u_pointlights[MAX_POINTLIGHT_COUNT];

uniform float u_bloom_threshold;

// pbr utilities
float D_GGX(float NdotH, float roughness);

float G_SchlickGGX(float NdotX, float roughness);

float G_Smith(float NdotV, float NdotL, float roughness);

vec3 F_Schlick(float cosTheta, vec3 F0);

// shared Cook-Torrance evaluator
vec3 PBR(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic, float roughness, vec3 radiance, float NdotV);

vec3 get_point_light(point_light light, vec3 N, vec3 fragPos, vec3 V, vec3 albedo, float metallic, float roughness, float NdotV);


void main()
{
    vec3 position = texture(g_position, v_uv).rgb;
    
    vec3 normal = texture(g_normal, v_uv).rgb;

    vec3 albedo = texture(g_albedo, v_uv).rgb;

    vec4 ormt = texture(g_orm, v_uv);
    float occlusion = ormt.r;
    float roughness = ormt.g;
    float metalness = ormt.b;

    vec3 emissive = texture(g_emissive, v_uv).rgb;


    float depth = texture(g_depth, v_uv).r;

    vec3 view_direction = normalize(u_camera_position - position);
    float norm_dot_view = max(dot(normal, view_direction), 0.0);

    vec3 color = vec3(0.0);
    color += emissive;

    // lighting calculations
    vec3 total_light = vec3(0.0);
    for(int i = 0; i < u_pointLight_count; i++)
    {
        point_light current_pointlight = u_pointlights[i];
        current_pointlight.diffuse *= current_pointlight.intensity;
        total_light += get_point_light(current_pointlight, normal, position, view_direction, albedo, metalness, roughness, norm_dot_view);
    }
    vec3 ambient = vec3(0.1) * albedo;
    if(occlusion > 0.0)
        ambient *= occlusion;
    total_light += ambient;

    color += total_light;


    if(u_gbuffer_view == 1)
        color = position;
    else if(u_gbuffer_view == 2) 
        color = normal * 0.5 + 0.5; // remap
    else if (u_gbuffer_view == 3)
        color = albedo;
    else if (u_gbuffer_view == 4)
        color = vec3(occlusion);
    else if (u_gbuffer_view == 5)
        color = vec3(roughness);
    else if (u_gbuffer_view == 6)
        color = vec3(metalness);
    else if (u_gbuffer_view == 7)
        color = emissive;
    else if (u_gbuffer_view == 8)
        color = vec3(depth);

    FragColor = vec4(color, 1.0);

    // bloom
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    brightColor = brightness > u_bloom_threshold ? vec4(color, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);
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
