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

typedef struct sot_camera {
    SOT_CameraInfo cameraInfo;
    mat4 view;
    mat4 projection;
    mat4 pvMatrix;
} sot_camera;

// ---- Camera follow modes ----
typedef enum SOT_CameraFollowMode {
    SOT_CAM_FREE = 0,         // No auto-follow (manual pan)
    SOT_CAM_FOLLOW_DEADZONE,  // Follow target with dead zone (platformer)
    SOT_CAM_AUTO_SCROLL,      // Auto-scroll at fixed speed (shmup)
    SOT_CAM_ROOM_SNAP,        // Snap to room boundaries (adventure)
} SOT_CameraFollowMode;

typedef struct SOT_CameraFollow {
    SOT_CameraFollowMode mode;

    // Dead-zone follow (PLAT)
    float deadZoneX, deadZoneY;     // Half-size of dead zone in pixels
    float smoothSpeed;              // Easing speed

    // Auto-scroll (SHMUP)
    float scrollSpeedX, scrollSpeedY;

    // Room-snap (ADV)
    float roomWidth, roomHeight;    // Room dimensions for snapping

    // Bounds clamping
    float boundsMinX, boundsMinY;
    float boundsMaxX, boundsMaxY;
    bool hasBounds;
} SOT_CameraFollow;

sot_camera CreateCameraWithInfo(SOT_CameraInfo cameraInfo, SOT_ProjectionInfo projectionInfo);
void SetCameraView(sot_camera *t, SOT_CameraInfo cameraInfo);
void SetCameraProjection(sot_camera *t, SOT_ProjectionInfo projectionInfo);
void UpdateCamera(sot_camera *t, vec3 direction, float deltaTime, float velocity);
void UpdateCameraPan(sot_camera *t, vec3 direction, float deltaTime, float velocity);

// Follow a target position using the configured follow mode
void SOT_Camera_FollowTarget(sot_camera *cam, SOT_CameraFollow *follow, float targetX, float targetY, float deltaTime);

void FreeCamera(sot_camera **);

#endif
