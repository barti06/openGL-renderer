#version 460 core
out float fragColor;

#define PI 3.14159265359

in vec2 v_uv;

uniform sampler2D g_position;
uniform sampler2D g_normal;
uniform sampler2D g_depth;
uniform sampler2D u_noise;

uniform mat4 u_projection;
uniform mat4 u_view;

uniform vec2 u_noise_scale;
uniform float u_radius;
uniform float u_bias;
uniform float u_strength;
uniform int u_directions;  // number of ray directions
uniform int u_steps; // samples per direction
uniform vec2 u_viewport_size;

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
    vec3 frag_pos = vec3(u_view * texture(g_position, v_uv));
    vec3 normal = normalize(mat3(u_view) * texture(g_normal, v_uv).xyz);

    // use noise to rotate the ray directions per pixel
    float noise_angle = texture(u_noise, v_uv * u_noise_scale).r * 2.0 * PI;

    // step size in view space
    float step_size = u_radius / float(u_steps);
    float angle_step = (2.0 * PI) / float(u_directions);

    float occlusion = 0.0;
    for(int d = 0; d < u_directions; d++)
    {
        // rotate each dir by the noise angle
        float angle = float(d) * angle_step + noise_angle;
        vec2 dir = vec2(cos(angle), sin(angle));

        // convert direction from view space XY to UV space
        vec2 uv_dir = dir * (u_radius / float(u_steps)) / vec2(u_viewport_size) * vec2(u_projection[0][0], u_projection[1][1]);

        float max_horizon_sin = sin(u_bias);

        for(int s = 1; s <= u_steps; s++)
        {
            vec2 sample_uv = v_uv + uv_dir * float(s);

            // avoid off screen sampling
            if(sample_uv.x < 0.0 || sample_uv.x > 1.0 || sample_uv.y < 0.0 || sample_uv.y > 1.0)
                break;

            vec3 sample_pos = vec3(u_view * texture(g_position, sample_uv));
            vec3 horizon = sample_pos - frag_pos;
            float squared_dist = dot(horizon, horizon);

            if(squared_dist < 0.001)
                continue;

            float dist = sqrt(squared_dist);

            if(dist > u_radius)
                continue;
            
            float attenuation = 1.0 - (dist / u_radius);

            float horizon_sin = dot(normalize(horizon), normal) * attenuation;

            // keep track of the highest horizon angle seen
            max_horizon_sin = max(max_horizon_sin, horizon_sin);
        }
        // add up occlusion contribution per direction
        occlusion += max_horizon_sin;
    }
    occlusion = occlusion / float(u_directions);
    fragColor  = clamp(1.0 - occlusion * u_strength, 0.0, 1.0);
}
