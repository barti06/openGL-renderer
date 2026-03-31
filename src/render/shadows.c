#include "shadows.h"
#include "shader.h"

#define SHADOWMAP_RES 16384
#define SHADOWMAP_DIR_SIZE 50.0f
#define SHADOWMAP_NEAR 1.0f
#define SHADOWMAP_FAR 200.0f

void shadowMap_init(shadowMap_t *shadowmap)
{
    // gen and bind fb
    glGenFramebuffers(1, &shadowmap->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowmap->fbo);

    glGenTextures(1, &shadowmap->tex);
    glBindTexture(GL_TEXTURE_2D, shadowmap->tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWMAP_RES, SHADOWMAP_RES, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowmap->tex, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // init shadowmap vars
    shadowmap->shadow_far = SHADOWMAP_FAR;
    shadowmap->shadow_near = SHADOWMAP_NEAR;
    glm_ortho(-SHADOWMAP_DIR_SIZE, SHADOWMAP_DIR_SIZE, -SHADOWMAP_DIR_SIZE, SHADOWMAP_DIR_SIZE, shadowmap->shadow_near, shadowmap->shadow_far, shadowmap->lightProj);
}

void shadowMap_pass(Shader *shadowMap_shader, shadowMap_t *shadowmap, vec3 lightDir)
{
    // put light far away
    vec3 lightPos;
    glm_vec3_dup(lightDir, lightPos);
    glm_vec3_scale(lightPos, -SHADOWMAP_DIR_SIZE, lightPos);
    vec3 up = {0.0f, 1.0f, 0.0f};

    glm_lookat(lightPos, GLM_VEC3_ZERO, up, shadowmap->lightView);
    glm_mat4_mul(shadowmap->lightProj, shadowmap->lightView, shadowmap->lightSpaceMat);

    glViewport(0, 0, SHADOWMAP_RES, SHADOWMAP_RES);
    glClear(GL_DEPTH_BUFFER_BIT);
    shader_use(shadowMap_shader);
    shader_set_mat4(shadowMap_shader, "u_lightSpaceMat", shadowmap->lightSpaceMat);
}

void shadowMap_send(Shader *shader, int unit, shadowMap_t *shadows)
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, shadows->tex);
    shader_set_int(shader, "u_shadowMap", 0 + unit);
    shader_set_mat4(shader, "u_lightSpaceMat", shadows->lightSpaceMat);
}
