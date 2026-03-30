#ifndef CAMERA_H
#define CAMERA_H

#include <cglm/cglm.h>

typedef enum InputType {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    MAX_MOVEMENT
} InputType;

typedef enum ActionType
{
    WALK,
    SPRINT,
    CROUCH,
    MAX_ACTION
} ActionType;

typedef struct Camera
{
    vec3 position;
    vec3 front;
    vec3 up;
    vec3 right;
    vec3 worldUp;

    float yaw;
    float pitch;

    float movementSpeed;
    float mouseSensitivity;
    float fieldOfView;
    
    mat4 view;
    mat4 projection;
    mat4 view_proj;
    vec4 frustum[6];
    
    int fov;
} Camera;

Camera camera_init(void);

void camera_process_movement(Camera* camera, float delta_time, InputType input_type, ActionType action_type);

void camera_process_rotation(Camera* camera, float xoffset, float yoffset);

void camera_update_matrices(Camera* camera, vec2 viewportSize, float nearZ, float farZ);

//bool camera_is_in_frustum(Camera *camera, vec3 center);
#endif
