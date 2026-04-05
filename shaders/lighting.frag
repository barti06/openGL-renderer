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

struct directional_light
{
    vec3 direction;
    vec3 diffuse;
    float intensity;
};

uniform sampler2D g_normal;
uniform sampler2D g_albedo;
uniform sampler2D g_orm;
uniform sampler2D g_emissive;
uniform sampler2D g_depth;
uniform mat4 u_inv_viewproj; // used for world pos reconstruction
vec3 get_world_pos(vec2 uv);

uniform sampler2D ssao;

uniform sampler2DShadow u_shadowMap;
uniform mat4 u_lightSpaceMat;

uniform vec3 u_camera_position;
uniform int u_gbuffer_view;

uniform int u_pointLight_count;
uniform point_light u_pointlights[MAX_POINTLIGHT_COUNT];

uniform directional_light u_dirlight;
uniform bool u_dirlight_enabled;

uniform float u_bloom_threshold;

uniform bool u_ssao_enabled;

uniform bool u_shadows_enabled;
uniform float u_shadow_bias;
uniform float u_shadow_spread;

uniform float u_ambient_str;

// pbr utilities
float D_GGX(float NdotH, float roughness);

float G_SchlickGGX(float NdotX, float roughness);

float G_Smith(float NdotV, float NdotL, float roughness);

vec3 F_Schlick(float cosTheta, vec3 F0);

// shared Cook-Torrance evaluator
vec3 PBR(vec3 N, vec3 V, vec3 L, vec3 albedo, float metallic, float roughness, vec3 radiance, float NdotV);

// specific Fresnel schlick for IBLs as default f_S can overcount specular
vec3 F_Schlick_Roughness(float cosTheta, vec3 F0, float roughness);

vec3 get_point_light(point_light light, vec3 N, vec3 fragPos, vec3 V, vec3 albedo, float metallic, float roughness, float NdotV);

vec3 get_directional_light(directional_light light, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, float NdotV);

float dirLight_shadow(vec4 lightSpaceVec, vec3 N, vec3 lightDir);

// ibl uniforms
uniform samplerCube u_irradiance_map;
uniform samplerCube u_prefilter_map;
uniform sampler2D u_brdfLUT;

void main()
{
    float depth = texture(g_depth, v_uv).r;
    if(depth == 1.0)
    {
        FragColor = vec4(0.2, 0.3, 0.3, 1.0);
        brightColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 position = get_world_pos(v_uv);
    
    vec3 normal = texture(g_normal, v_uv).rgb;

    vec3 albedo = texture(g_albedo, v_uv).rgb;

    vec4 orm = texture(g_orm, v_uv);
    
    float occlusion = 1.0;
    if(u_ssao_enabled)
    {
        if(orm.a == 0.0)
            occlusion = texture(ssao, v_uv).r;
        else
            occlusion = orm.r;
    }
    float roughness = orm.g;
    float metalness = orm.b;

    vec3 emissive = texture(g_emissive, v_uv).rgb;

    vec3 view_direction = normalize(u_camera_position - position);
    float norm_dot_view = max(dot(normal, view_direction), 0.0);

    vec3 color = vec3(0.0);
    color = emissive;

    // lighting calculations
    vec3 total_light = vec3(0.0);
    for(int i = 0; i < u_pointLight_count; i++)
    {
        point_light current_pointlight = u_pointlights[i];
        current_pointlight.diffuse *= current_pointlight.intensity;
        total_light += get_point_light(current_pointlight, normal, position, view_direction, albedo, metalness, roughness, norm_dot_view);
    }

    float dir_shadow = 1.0;
    if(u_dirlight_enabled)
    {
        if(u_shadows_enabled)
        {
            vec4 lightSpaceVec = u_lightSpaceMat * vec4(position, 1.0);
            dir_shadow = dirLight_shadow(lightSpaceVec, normal, -u_dirlight.direction);
        }
        total_light += dir_shadow * get_directional_light(u_dirlight, normal, view_direction, albedo, metalness, roughness, norm_dot_view);
    }


    vec3 F0 = mix(vec3(0.04), albedo, metalness);
    vec3 F = F_Schlick_Roughness(max(norm_dot_view, 0.0), F0, roughness);
    
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metalness;

    vec3 irradiance = texture(u_irradiance_map, normal).rgb;
    vec3 diffuse = irradiance * albedo;

    vec3 R = reflect(-view_direction, normal);
    const float MAX_REFLECTION_LOD = 4.0; // number of mipmaps minus one
    vec3 prefiltered_color = textureLod(u_prefilter_map, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 env_BRDF  = texture(u_brdfLUT, vec2(max(dot(normal, view_direction), 0.0), roughness)).rg;
    vec3 specular = prefiltered_color * (F * env_BRDF.x + env_BRDF.y);

    vec3 ambient = vec3(u_ambient_str) * (kD * diffuse + specular) * occlusion;
    //vec3 ambient = vec3(0.05) * albedo * occlusion;

    total_light += ambient;

    color += total_light;

    // bloom
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    brightColor = brightness > u_bloom_threshold ? vec4(color, 1.0) : vec4(0.0, 0.0, 0.0, 1.0);

    if(u_gbuffer_view != 0)
    {
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
        else if (u_gbuffer_view == 9)
            color = brightColor.xyz;
        else if (u_gbuffer_view == 10)
            color = vec3(dir_shadow);
    }

    FragColor = vec4(color, 1.0);
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

vec3 F_Schlick_Roughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 get_point_light(point_light light, vec3 N, vec3 fragPos, vec3 V, 
                    vec3 albedo, float metallic, float roughness, float NdotV)
{
    vec3 toLight = light.position - fragPos;
    float dist = length(toLight);
    vec3 L = toLight / dist;

    float atten = 1.0 / (1.0 + light.linear * dist + light.quadratic * dist * dist);
    vec3 radiance = light.diffuse * atten;

    return PBR(N, V, L, albedo, metallic, roughness, radiance, NdotV);
}

vec3 get_directional_light(directional_light light, vec3 N, vec3 V, 
                           vec3 albedo, float metallic, float roughness, float NdotV)
{
    vec3 L = normalize(-light.direction);
    vec3 radiance = light.diffuse * light.intensity;
    return PBR(N, V, L, albedo, metallic, roughness, radiance, NdotV);
}

const vec2 poisson_disk[16] = vec2[](
    vec2(-0.94201624,  -0.39906216),
    vec2( 0.94558609,  -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2( 0.34495938,   0.29387760),
    vec2(-0.91588581,   0.45771432),
    vec2(-0.81544232,  -0.87912464),
    vec2(-0.38277543,   0.27676845),
    vec2( 0.97484398,   0.75648379),
    vec2( 0.44323325,  -0.97511554),
    vec2( 0.53742981,  -0.47373420),
    vec2(-0.26496911,  -0.41893023),
    vec2( 0.79197514,   0.19090188),
    vec2(-0.24188840,   0.99706507),
    vec2(-0.81409955,   0.91437590),
    vec2( 0.19984126,   0.78641367),
    vec2( 0.14383161,  -0.14100790)
);

float dirLight_shadow(vec4 lightSpaceVec, vec3 N, vec3 lightDir)
{
    // transform lightspace fragpos from clip space to ndc
    vec3 projCoords = lightSpaceVec.xyz / lightSpaceVec.w;

    // convert ndc [-1,1] to 0,1
    projCoords = projCoords * 0.5 + 0.5;
    
    if( 
       projCoords.z > 1.0)
    {
        return 0.0;
    }
    
    // sample shadow map
    float currentDepth = projCoords.z;
    
    float bias = u_shadow_bias; // fixed bias i choose

    float shadow = 0.0;

    // PCF
    vec2 texelSize = 1.0 / textureSize(u_shadowMap, 0);
    float spread = u_shadow_spread; // should move this as a uniform

    for(int i = 0; i < 16; i++)
    {
        vec2 offset = poisson_disk[i] * spread * texelSize;
        //float pcfDepth = texture(u_shadowMap, projCoords.xy + offset).r;
        //shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        float pcfDepth = textureProj(u_shadowMap, vec4(projCoords.xy + offset, projCoords.z - bias, 1.0));
        shadow += pcfDepth;
    }
    return shadow / 16.0;
}

vec3 get_world_pos(vec2 uv) 
{
    // grab depth
    float depth = texture(g_depth, uv).r;

    // convert to ndc
    vec4 ndc = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    
    // unproject
    vec4 world_pos = u_inv_viewproj * ndc;
    
    // undo perspective divide
    return world_pos.xyz / world_pos.w;
}
