#include "sot_camera.h"

/* Allocates memory for a sot_transforms structure and
 * returns a valid memory pointer */
SOT_Camera* CreateCamera() {
    SOT_Camera *t = (SOT_Camera*)malloc(sizeof(SOT_Camera));
    glm_mat4_identity(t->view);
    glm_mat4_identity(t->projection);
    return t;
}

SOT_Camera* CreateCameraWitInfo(SOT_CameraInfo cameraInfo, SOT_ProjectionInfo projectionInfo) {
    SOT_Camera *t = (SOT_Camera*)malloc(sizeof(SOT_Camera));
    SetCamera(t, cameraInfo);
    SetProjection(t, projectionInfo);
    
    return t;
}

/* Free heap memory allocated to the transform and 
 * Nullifies the original pointer.  */
void FreeTransforms(SOT_Camera ** t) {
    free(*t);
    *t = NULL;
}

/* Update the View matrix stored in the transofrm entity. */
void SetCamera(SOT_Camera *t, SOT_CameraInfo cameraInfo) {
    glm_lookat(cameraInfo.eye, cameraInfo.center, cameraInfo.up, t->view);
}

/* Update the Projection matrix stored in the transofrm entity. */
void SetProjection(SOT_Camera *t, SOT_ProjectionInfo projectionInfo) {

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

    return;
}

