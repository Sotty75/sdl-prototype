#include "sot_camera.h"
#include "sot_common.h"
#include <math.h>

sot_camera CreateCameraWithInfo(SOT_CameraInfo cameraInfo, SOT_ProjectionInfo projectionInfo) {
    sot_camera t = {0};
    glm_mat4_identity(t.view);
    glm_mat4_identity(t.projection);
    SetCameraView(&t, cameraInfo);
    SetCameraProjection(&t, projectionInfo);
    
    // update the projection-view matrix for the renderer.
    glm_mat4_mul(t.projection, t.view, t.pvMatrix);

    return t;
}

/* Free heap memory allocated to the transform and 
 * Nullifies the original pointer.  */
void UpdateCamera(sot_camera *t, vec3 direction, float deltaTime, float velocity) 
{
    glm_vec3_normalize(direction);
    glm_vec3_scale(direction, velocity * deltaTime, direction);
    glm_vec3_add(t->cameraInfo.eye, direction, t->cameraInfo.eye);
    glm_lookat(t->cameraInfo.eye, t->cameraInfo.center, t->cameraInfo.up, t->view);

    // update the projection-view matrix for the renderer.
    glm_mat4_mul(t->projection, t->view, t->pvMatrix);
}

void UpdateCameraPan(sot_camera *t, vec3 direction, float deltaTime, float velocity) 
{
    glm_vec3_normalize(direction);
    glm_vec3_scale(direction, velocity * deltaTime, direction);
    glm_vec3_add(t->cameraInfo.eye, direction, t->cameraInfo.eye);
    glm_vec3_add(t->cameraInfo.center, direction, t->cameraInfo.center);
    glm_lookat(t->cameraInfo.eye, t->cameraInfo.center, t->cameraInfo.up, t->view);

    // update the projection-view matrix for the renderer.
    glm_mat4_mul(t->projection, t->view, t->pvMatrix);
}

/* Update the View matrix stored in the transofrm entity. */
void SetCameraView(sot_camera *t, SOT_CameraInfo cameraInfo) {
    t->cameraInfo = cameraInfo;
    glm_lookat(t->cameraInfo.eye, t->cameraInfo.center, t->cameraInfo.up, t->view);

    // update the projection-view matrix for the renderer.
    glm_mat4_mul(t->projection, t->view, t->pvMatrix);
}

/* Update the Projection matrix stored in the transofrm entity. */
void SetCameraProjection(sot_camera *t, SOT_ProjectionInfo projectionInfo) {

    switch (projectionInfo.mode) 
    {
        case SOT_PERSPECTIVE_DEFAULT: {
            glm_perspective_default(projectionInfo.aspect, t->projection);
        } break;
        case SOT_PERSPECTIVE: {
            glm_perspective(projectionInfo.fov,
                projectionInfo.aspect, 
                projectionInfo.near,
                projectionInfo.far,
                t->projection);
        } break;
        case SOT_ORTHO: {
            glm_ortho(projectionInfo.left, 
                projectionInfo.right, 
                projectionInfo.bottom, 
                projectionInfo.top,
                projectionInfo.near, 
                projectionInfo.far, 
                t->projection);
        } break;
        case SOT_ORTHO_DEFAULT: {
            glm_ortho_default(projectionInfo.aspect, t->projection);
        } break;
        default:
            break;
    }

    // update the projection-view matrix for the renderer.
    glm_mat4_mul(t->projection, t->view, t->pvMatrix);

    return;
}

// ---- Camera follow modes ----

static float Lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

static float Clamp(float val, float min, float max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

void SOT_Camera_FollowTarget(sot_camera *cam, SOT_CameraFollow *follow, float targetX, float targetY, float deltaTime)
{
    float camX = cam->cameraInfo.eye[0];
    float camY = cam->cameraInfo.eye[1];
    float newX = camX;
    float newY = camY;

    switch (follow->mode) {
        case SOT_CAM_FOLLOW_DEADZONE: {
            // Only move camera when target exits the dead zone
            float diffX = targetX - camX;
            float diffY = targetY - camY;

            if (diffX > follow->deadZoneX)       newX = targetX - follow->deadZoneX;
            else if (diffX < -follow->deadZoneX)  newX = targetX + follow->deadZoneX;

            if (diffY > follow->deadZoneY)       newY = targetY - follow->deadZoneY;
            else if (diffY < -follow->deadZoneY)  newY = targetY + follow->deadZoneY;

            // Smooth easing
            float t = 1.0f - expf(-follow->smoothSpeed * deltaTime);
            newX = Lerp(camX, newX, t);
            newY = Lerp(camY, newY, t);
            break;
        }

        case SOT_CAM_AUTO_SCROLL: {
            newX = camX + follow->scrollSpeedX * deltaTime;
            newY = camY + follow->scrollSpeedY * deltaTime;
            break;
        }

        case SOT_CAM_ROOM_SNAP: {
            // Snap to room grid based on target position
            if (follow->roomWidth > 0)
                newX = floorf(targetX / follow->roomWidth) * follow->roomWidth + follow->roomWidth * 0.5f;
            if (follow->roomHeight > 0)
                newY = floorf(targetY / follow->roomHeight) * follow->roomHeight + follow->roomHeight * 0.5f;
            break;
        }

        case SOT_CAM_FREE:
        default:
            return; // No auto-follow
    }

    // Clamp to bounds
    if (follow->hasBounds) {
        newX = Clamp(newX, follow->boundsMinX, follow->boundsMaxX);
        newY = Clamp(newY, follow->boundsMinY, follow->boundsMaxY);
    }

    // Apply new position (move both eye and center together for pan)
    float dx = newX - cam->cameraInfo.eye[0];
    float dy = newY - cam->cameraInfo.eye[1];
    cam->cameraInfo.eye[0] = newX;
    cam->cameraInfo.eye[1] = newY;
    cam->cameraInfo.center[0] += dx;
    cam->cameraInfo.center[1] += dy;

    glm_lookat(cam->cameraInfo.eye, cam->cameraInfo.center, cam->cameraInfo.up, cam->view);
    glm_mat4_mul(cam->projection, cam->view, cam->pvMatrix);
}

