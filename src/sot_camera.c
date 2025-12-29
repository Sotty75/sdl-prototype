#include "sot_camera.h"

sot_camera CreateCameraWitInfo(SOT_CameraInfo cameraInfo, SOT_ProjectionInfo projectionInfo) {
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

