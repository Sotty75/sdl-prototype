/* 
 * Copyright (c), Luca Paolo Sotgiu
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
*/

#ifndef SOT_CAMERA_H_
#define SOT_CAMERA_H_

#include "cglm.h"

typedef enum SOT_ProjectionMode {
    SOT_PERSPECTIVE,
    SOT_PERSPECTIVE_DEFAULT,
    SOT_ORTHO,
    SOT_ORTHO_DEFAULT,
} SOT_ProjectionMode;

typedef struct SOT_CameraInfo {
    vec3 eye;
    vec3 center;
    vec3 up;
} SOT_CameraInfo;

typedef struct SOT_ProjectionInfo {
    SOT_ProjectionMode mode;
    float fov;
    float aspect;
    float near, far;
    float left, right, bottom, top;
} SOT_ProjectionInfo;

typedef struct SOT_Camera {
    SOT_CameraInfo cameraInfo;
    mat4 view;
    mat4 projection;
} SOT_Camera;

SOT_Camera* CreateCamera();
SOT_Camera* CreateCameraWitInfo(SOT_CameraInfo cameraInfo, SOT_ProjectionInfo projectionInfo);
void SetCamera(SOT_Camera *t, SOT_CameraInfo cameraInfo);
void SetProjection(SOT_Camera *t, SOT_ProjectionInfo projectionInfo);
void MoveCamera(SOT_Camera *t, vec3 direction, float deltaTime, float velocity);
void FreeCamera(SOT_Camera **);

#endif
