#version 460 core

out vec4 FragColor;

in vec2 v_uv;

uniform sampler2D fx_scene;
uniform sampler2D fx_bloom;
uniform sampler2D fx_depth;

// tone map settings
uniform int u_tonemap = 0; // 1 is HDR, 2 is filmic
uniform float u_gamma = 2.2;
uniform float u_brightness = 1.0;
uniform float u_exposure = 1.0;

// to disable post processing when viewing specific gbuffer textures
uniform bool u_gbuffer_viewing;

// general settings
uniform bool u_vignette_enabled = false;
uniform float u_vignette_strength = 2.0;

uniform bool u_chromatic_aberration_enabled = false;
uniform float u_chromatic_aberration_strength = 2.0;

uniform bool u_bloom_enabled = true;
uniform float u_bloom_strength = 1.0;

// tone mapping functions
vec3 ACES(vec3 x);

vec3 Filmic(vec3 x);

vec3 Reinhard(vec3 x);

// post processing functions
float vignette(vec2 x, float strength);

vec3 chromatic_aberration(sampler2D tex, vec2 uv, float intensity);

void main()
{
    if(u_gbuffer_viewing)
    {
        FragColor = vec4(texture(fx_scene, v_uv).rgb, 1.0);
        return;
    }
    
    vec3 color;
    if(u_chromatic_aberration_enabled)
        color = chromatic_aberration(fx_scene, v_uv, u_chromatic_aberration_strength);
    else
        color = texture(fx_scene, v_uv).rgb;

    color *= u_exposure;

    if(u_bloom_enabled)
        color += texture(fx_bloom, v_uv).rgb * u_bloom_strength;

    // tone mapping, always default to ACES
    if(u_tonemap == 1)
        color = Reinhard(color);
    else if(u_tonemap == 2)
        color = Filmic(color);
    else
        color = ACES(color);
    
    color *= u_brightness;

    // gamma correction
    color = pow(color, vec3(1.0 / u_gamma));

    if(u_vignette_enabled)
        color *= vignette(v_uv, u_vignette_strength);

    FragColor = vec4(color, 1.0);
}

vec3 ACES(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

vec3 Filmic(vec3 x) 
{
  vec3 a = max(vec3(0.0), x - 0.004);
  vec3 b = (a * (6.2 * a + 0.5)) / (a * (6.2 * a + 1.7) + 0.06);
  return pow(b, vec3(2.2));
}

vec3 Reinhard(vec3 x)
{
    return(vec3(1.0) - exp(-x));
}

float vignette(vec2 x, float strength) 
{
    vec2 uv = x - 0.5;
    return 1.0 - dot(uv, uv) * strength;
}

vec3 chromatic_aberration(sampler2D tex, vec2 uv, float intensity)
{
    vec2 dist = uv - 0.5;

    float redOffset   =  0.009 * intensity;
    float greenOffset =  0.006 * intensity;
    float blueOffset  = -0.006 * intensity;

    vec3 CAvec;
    CAvec.r = texture(tex, uv - (dist * vec2(redOffset))).r;
    CAvec.g = texture(tex, uv - (dist * vec2(greenOffset))).g;
    CAvec.b = texture(tex, uv).b;
    return CAvec;
}
