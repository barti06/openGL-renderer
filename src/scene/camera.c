#include "camera.h"

// Default camera values (could do as #define(s))
const float D_YAW = -90.0f;
const float D_PITCH = 0.0f;
const float D_SPEED = 5.0f;
const float D_SENSITIVITY = 0.333f;
const float D_FOV = 75.0f;

// update camera vectors
static inline void camera_update_vectors(Camera* camera);

void camera_update_vectors(Camera* camera)
{
    // calculate new front vector
    camera->front[0] = cosf(glm_rad(camera->yaw)) * cosf(glm_rad(camera->pitch));
    camera->front[1] = sinf(glm_rad(camera->pitch));
    camera->front[2] = sinf(glm_rad(camera->yaw)) * cosf(glm_rad(camera->pitch));
    
    // normalize it, as it is a direction vector
    glm_normalize(camera->front);

    // normalize the vectors, because their length gets closer to 0 
    // the more you look up or down, which results in slower movement
    glm_cross(camera->front, camera->worldUp, camera->right); // get the right vector
    glm_normalize(camera->right);
    
    glm_cross(camera->right, camera->front, camera->up); // get the up vector
    glm_normalize(camera->up);
}

Camera camera_init(void)
{
    Camera camera;

    // setup start position
    camera.position[0] = 0.0f;
    camera.position[1] = 1.0f;
    camera.position[2] = 5.0f;

    // setup worldUp (Y)
    camera.worldUp[0] = 0.0f;
    camera.worldUp[1] = 1.0f;
    camera.worldUp[2] = 0.0f;

    // setup camera front vector (set to z = -1, because z = +1 is the observator)
    camera.front[0] = 0.0f;
    camera.front[1] = 0.0f;
    camera.front[2] = -1.0f;

    camera.yaw = D_YAW;
    camera.pitch = D_PITCH;

    // setup camera values
    camera.movementSpeed = D_SPEED;
    camera.mouseSensitivity = D_SENSITIVITY;
    camera.fieldOfView = D_FOV;

    camera_update_vectors(&camera);
    return camera;
}

void camera_process_movement(Camera* camera, float delta_time, InputType input_type, ActionType action_type)
{
    float speed = camera->movementSpeed * delta_time;
    if(action_type == SPRINT)
        speed *= 2.0f;
    if(action_type == CROUCH)
        speed *= 0.5f;

    switch(input_type)
    {
    case FORWARD:
    {
        vec3 temp;
        glm_vec3_scale(camera->front, speed, temp);
        glm_vec3_add(camera->position, temp, camera->position);
        break;
    }
    case BACKWARD:
    {
        vec3 temp;
        glm_vec3_scale(camera->front, speed, temp);
        glm_vec3_sub(camera->position, temp, camera->position);
        break;
    }
    case RIGHT:
    {
        vec3 temp;
        glm_vec3_scale(camera->right, speed, temp);
        glm_vec3_add(camera->position, temp, camera->position);
        break;
    }
    case LEFT:
    {
        vec3 temp;
        glm_vec3_scale(camera->right, speed, temp);
        glm_vec3_sub(camera->position, temp, camera->position);
        break;
    }
    case UP:
    {
        camera->position[1] += speed;
        break;
    }
    case DOWN:
    {
        camera->position[1] -= speed;
        break;
    }
    default:
        break;
    }
}

void camera_process_rotation(Camera* camera, float xoffset, float yoffset)
{
    xoffset *= camera->mouseSensitivity;
    yoffset *= camera->mouseSensitivity;

    camera->yaw += xoffset;
    camera->pitch += yoffset;

    // make sure that when pitch is out of bounds screen doesn't get flipped

    if (camera->pitch > 89.0f)
         camera->pitch = 89.0f;
    if (camera->pitch < -89.0f)
        camera->pitch = -89.0f;

    // update front, right and up Vectors using the updated Euler angles
    camera_update_vectors(camera);
}

void camera_update_matrices(Camera* camera, vec2 viewportSize, float nearZ, float farZ)
{
    // get view matrix
    vec3 pos_add_front;
    glm_vec3_add(camera->position, camera->front, pos_add_front);
    glm_lookat(camera->position, pos_add_front, camera->up, camera->view);
        
    // get projection matrix
    glm_perspective(glm_rad(camera->fieldOfView), 
        (viewportSize[0] / viewportSize[1]), nearZ, 
        farZ, camera->projection);
    
    // update frustum
    glm_mat4_mul(camera->projection, camera->view, camera->view_proj);
    glm_frustum_planes(camera->view_proj, camera->frustum);
}
