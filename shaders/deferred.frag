#version 460 core

#define PI 3.14159265359
#define MAX_POINTLIGHT_COUNT 64

const int forward = 0;
const int deferred = 1;
uniform int render_type;

out vec4 fragColor;

layout (location = 0) out vec4 g_position;
layout (location = 1) out vec4 g_normal;
layout (location = 2) out vec4 g_albedo;
layout (location = 3) out vec4 g_orm;
layout (location = 4) out vec4 g_emissive;

in vec2 v_uv;
in vec2 v_uv2;
in mat3 v_TBN;
in vec3 v_fragment_pos;

// REMBEMBER!! GLTF STORES ROUGHNESS ON CHANNEL G
// AND METALNESS ON CHANNEL B!!!
uniform sampler2D u_albedo;
uniform sampler2D u_metallic_roughness;
uniform sampler2D u_normal;
uniform sampler2D u_emissive;
uniform sampler2D u_ao;
uniform sampler2D u_iridescence;
uniform sampler2D u_iridescence_thickness;
uniform sampler2D u_volume_thickness;

uniform bool u_has_albedo;
uniform bool u_has_metallic_roughness;
uniform bool u_has_normal;
uniform bool u_has_emissive;
uniform bool u_has_emissive_texcoord;
uniform bool u_has_ao;
uniform bool u_has_occlusion_texcoord;
uniform bool u_has_iridescence;
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

uniform float u_iridescence_factor;
uniform float u_iridescence_ior;
uniform float u_iridescence_thickness_min;
uniform float u_iridescence_thickness_max;

void main()
{
    // send position
    g_position = vec4(v_fragment_pos,1.0);

    // extract albedo
    vec4 color = u_has_albedo ? texture(u_albedo, v_uv) * u_albedo_factor : u_albedo_factor;

    if(color.a < 0.1)
        discard;
    // linear to srgb & send albedo
    g_albedo = vec4(pow(color.rgb, vec3(2.2)),1.0);

    // extract emissive maps, convert to srgb & send them
    vec2 emissive_uvs = u_has_emissive_texcoord ? v_uv2 : v_uv;
    vec3 emissive_map = u_has_emissive ? pow(texture(u_emissive, emissive_uvs).rgb, vec3(2.2)) * u_emissive_factor * u_emissive_strength : u_emissive_factor * u_emissive_strength;
    g_emissive = vec4(emissive_map, 1.0);

    // grab the normal texture and convert from NDC
    vec3 normal = u_has_normal ? texture(u_normal, v_uv * u_normal_scale).rgb * 2.0 - 1.0 : vec3(0.0);
    // send normal
    g_normal = vec4(normalize(v_TBN * normal),1.0);

    // get ao
    vec2 ao_uvs = u_has_occlusion_texcoord ? v_uv2 : v_uv;
    float ao_sample = u_has_ao ? texture(u_ao, ao_uvs * u_occlusion_scale).r : 1.0;
    float ao = ao_sample * u_occlusion_strength;

    // grab material's metalness and roughness from metallic roughness texture
    vec3 metal_rough = u_has_metallic_roughness ? texture(u_metallic_roughness, v_uv).rgb : vec3(0.0);
    float roughness = u_has_metallic_roughness ? metal_rough.g * u_roughness_factor : u_roughness_factor;
    float metallic = u_has_metallic_roughness ? metal_rough.b * u_metallic_factor : u_metallic_factor;

    // send orm
    g_orm = vec4(ao, roughness, metallic, 1.0);
} 
