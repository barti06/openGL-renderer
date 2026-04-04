#version 460 core

out vec4 FragColor;

in vec3 v_worldpos;

uniform samplerCube u_environmentMap;

void main()
{		
    vec3 env_color = texture(u_environmentMap, v_worldpos).rgb;
    
    // gamma correct
    env_color = pow(env_color, vec3(1.0/2.2)); 
    
    FragColor = vec4(env_color, 1.0);
}
