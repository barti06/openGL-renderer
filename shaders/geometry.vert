#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUv;
layout(location = 3) in vec4 aTangent;
layout (location = 4) in vec2 aUv2;

out vec2 v_uv;
out vec2 v_uv2;
out mat3 v_TBN;
out vec3 v_fragment_pos;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

void main()
{
    // fragment in world position
    vec4 world_pos = u_model * vec4(aPos, 1.0);
    v_fragment_pos = world_pos.xyz;
    
    // normal matrix for tbn calculation
    mat3 normal_mat = transpose(inverse(mat3(u_model)));

    // tbn matrix calculation
    vec3 T = normalize(normal_mat * aTangent.xyz); // tangent
    vec3 N = normalize(normal_mat * aNormal); // normal
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * aTangent.w; // bitangent

    v_TBN = mat3(T, B, N);
    v_uv = aUv;
    v_uv2 = aUv2;

    gl_Position = u_projection * u_view * world_pos;
}
