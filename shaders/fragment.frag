#version 460 core

#define PI 3.14159265359

out vec4 FragColor;

in vec2 v_uv;
in mat3 v_TBN;
in vec3 v_fragment_pos;
in vec3 v_view_direction;

struct point_light
{
    // light position
    vec3 position;
    
    // for calculating point light's attenuation
    float constant;
    float linear;
    float quadratic;  

    // light color
    vec3 diffuse;
};

// REMBEMBER!! GLTF STORES ROUGHNESS ON CHANNEL G
// AND METALNESS ON CHANNEL B!!!
uniform sampler2D u_albedo;
uniform sampler2D u_metallic_roughness;
uniform sampler2D u_normal;
uniform sampler2D u_emissive;
uniform sampler2D u_ao;

uniform vec4 u_albedo_factor;
uniform float u_metallic_factor;
uniform float u_roughness_factor;

uniform float u_emissive_strength;
uniform vec3 u_emissive_factor;

// proper tone mapping
vec3 ACESFilm(vec3 x);

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
    vec2 uv = v_uv;
    vec4 color = texture(u_albedo, uv) * u_albedo_factor;
    if(color.a < 0.1)
        discard;

    // linear to srgb
    vec3 albedo = pow(color.rgb, vec3(2.2));

    // grab material's metalness and roughness from metallic roughness texture
    vec3 metal_rough = texture(u_metallic_roughness, uv).rgb;
    float roughness = metal_rough.g * u_roughness_factor;
    float metallic = metal_rough.b * u_metallic_factor;

    // grab the normal texture and convert from NDC
    vec3 normal = texture(u_normal, uv).rgb;
    normal = normal * 2.0 - 1.0;
    normal = normalize(v_TBN * normal);

    // extract emissive maps and convert to srgb
    vec3 emissive_map = pow(texture(u_emissive, uv).rgb, vec3(2.2)) * u_emissive_strength;
    emissive_map *= u_emissive_factor;

    // get ao maps
    float ambient_occlusion = texture(u_ao, uv).r;

    float norm_dot_view = max(dot(normal, v_view_direction), 0.0);

    point_light my_light;
    my_light.position = vec3(0.0, 10.0, 5.0);
    my_light.diffuse = vec3(10.0, 10.0, 10.0);
    my_light.constant = 1.0;
    my_light.quadratic = 0.0075;
    my_light.linear = 0.045;

    vec3 light_result = vec3(0.0);
    light_result += get_point_light(my_light, normal, v_fragment_pos, v_view_direction, albedo, metallic, roughness, norm_dot_view);
    float direct_ao = mix(1.0, ambient_occlusion, 0.5); // 0.5 is the 'influence'
    light_result *= direct_ao;

    vec3 ambient = vec3(0.03) * albedo * ambient_occlusion;

    vec3 final_color = vec3(0.0);
    final_color += ambient;
    final_color += light_result;
    final_color += emissive_map;

    final_color = ACESFilm(final_color);
    final_color = pow(final_color, vec3(1.0 / 2.2));

    FragColor = vec4(final_color, 1.0);
} 

vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

float D_GGX(float NdotH, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
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
    vec3  H     = normalize(L + V);
    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    // F0: 0.04 for dielectrics, albedo for metals
    vec3  F0  = mix(vec3(0.04), albedo, metallic);
    float D   = D_GGX(NdotH, roughness);
    float G   = G_Smith(NdotV, NdotL, roughness);
    vec3  F   = F_Schlick(HdotV, F0);

    // specularity
    vec3 spec = (D * G * F) / (4.0 * NdotV * NdotL + 1e-4);

    // diffuse
    vec3 kD   = (1.0 - F) * (1.0 - metallic);
    vec3 diff = kD * albedo / PI;

    return (diff + spec) * radiance * NdotL;
}

vec3 get_point_light(point_light light, vec3 N, vec3 fragPos, vec3 V, vec3 albedo, float metallic, float roughness, float NdotV)
{
    vec3  toLight = light.position - fragPos;
    float dist    = length(toLight);
    vec3  L       = toLight / dist;

    float atten   = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);
    vec3  radiance = light.diffuse * atten;

    return PBR(N, V, L, albedo, metallic, roughness, radiance, NdotV);
}