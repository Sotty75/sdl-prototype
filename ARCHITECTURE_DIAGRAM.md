# Architecture Diagram

Visual architecture reference for the SDL Prototype engine.

---

## 1. High-Level System Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│                              main.c                                     │
│                     (SDL3 Callback Entry Point)                         │
│                                                                         │
│   SDL_AppInit()  ──►  SDL_AppEvent()  ──►  SDL_AppIterate()             │
│   (one-time)          (per event)          (per frame)                  │
│       │                    │                    │                       │
│       ▼                    ▼                    ▼                       │
│  ┌─────────┐       ┌────────────┐       ┌────────────┐                  │
│  │ Init    │       │ Read Input │       │ Update +   │                  │
│  │ GPU     │       │ Gamepad /  │       │ Render     │                  │
│  │ Scene   │       │ Keyboard   │       │ Loop       │                  │
│  └─────────┘       └────────────┘       └────────────┘                  │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Core Data Model

```
┌──────────────────────────────────────────────────────────────────┐
│                          AppState                                │
│  (Global context passed to all subsystems)                       │
│                                                                  │
│  ┌──────────────┐  ┌─────────────────┐  ┌─────────────────────┐  │
│  │ SOT_GPU_State│  │ sot_texture_t * │  │ sot_collider_node_t │  │
│  │ *gpu         │  │ pTexturesPool   │  │ *pStaticColliders   │  │
│  │              │  │ (linked list)   │  │ *pDynamicColliders  │  │
│  └──────┬───────┘  └─────────────────┘  │ (linked lists)      │  │
│         │                               └─────────────────────┘  │
│         │          ┌─────────────────┐                           │
│         │          │ sot_debug_info_t│                           │
│         │          │ debugInfo       │                           │
│         │          └─────────────────┘                           │
└─────────┼────────────────────────────────────────────────────────┘
          │
          ▼
┌──────────────────────────────────────────────────────┐
│                    SOT_GPU_State                     │
│                                                      │
│  SDL_Window *window                                  │
│  SDL_GPUDevice *device                               │
│  SDL_GPUSampler *nearestSampler                      │
│  SOT_GPU_TransferBuffers transferBuffers             │
│  uint32_t pipelineFlags  ──── bitmask toggle ──────┐ │
│  SDL_GPUGraphicsPipeline *pipeline[16]  ◄──────────┘ │
│  SOT_GPU_Buffers buffers[16]                         │
│  SOT_GPU_DebugInfo *debugInfo                        │
└──────────────────────────────────────────────────────┘
```

---

## 3. Scene Composition

```
┌──────────────────────────────────────────────────────────────────────┐
│                           SOT_Scene                                  │
│                                                                      │
│  ┌────────────────┐   ┌──────────────────┐   ┌────────────────────┐  │
│  │ sot_camera     │   │ sot_camera       │   │ sot_tilemap *      │  │
│  │ worldCamera    │   │ uiCamera         │   │ tilemap            │  │
│  │                │   │                  │   │                    │  │
│  │  view matrix   │   │  view matrix     │   │  tiles[]           │  │
│  │  proj matrix   │   │  proj matrix     │   │  colliders (list)  │  │
│  │  pvMatrix      │   │  pvMatrix        │   │  tilesetFilename   │  │
│  └────────────────┘   └──────────────────┘   │  gpuTilemapInfo    │  │
│                                              └────────────────────┘  │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │ SOT_Actor actors[2000]                    actorsCount: int     |  │
│  │                                                                │  │
│  │  ┌─────────────────────────────────────────────────────────┐   │  │
│  │  │ SOT_Actor                                               │   │  │
│  │  │                                                         │   │  │
│  │  │  actorID ── index into GPU sprite storage buffer        │   │  │
│  │  │  actorName                                              │   │  │
│  │  │                                                         │   │  │
│  │  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐   │   │  │
│  │  │  │ SOT_Transform│  │ SOT_Physics  │  │sot_collider_t│   │   │  │
│  │  │  │  position    │  │  body type   │  │  type (enum) │   │   │  │
│  │  │  │  scale       │  │  v_magnitude │  │  shape union │   │   │  │
│  │  │  │  rotation    │  │  v_direction │  │  (circle,    │   │   │  │
│  │  │  └──────────────┘  └──────────────┘  │   capsule,   │   │   │  │
│  │  │                                      │   AABB, poly)│   │   │  │
│  │  │  ┌──────────────────────────────┐    └──────────────┘   │   │  │
│  │  │  │ SOT_Animation animations[256]│                       │   │  │
│  │  │  │  currentAnimation (index)    │    ┌──────────────┐   │   │  │
│  │  │  │  animationsCount             │    │GPU_Sprite    │   │   │  │
│  │  │  │                              │    │Instance      │   │   │  │
│  │  │  │  Each animation has:         │    │ position     │   │   │  │
│  │  │  │   .atlasName                 │    │ frameCoords  │   │   │  │
│  │  │  │   .step_ms                   │    │ frameSize    │   │   │  │
│  │  │  │   .sequence (frames[])       │    │ atlasSize    │   │   │  │
│  │  │  └──────────────────────────────┘    └──────────────┘   │   │  │
│  │  └─────────────────────────────────────────────────────────┘   │  │
│  └────────────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 4. Rendering Pipeline Architecture

```
                          SOT_GPU_Render()
                               │
               Acquire Command Buffer + Swapchain
                               │
                     Begin Render Pass
                               │
            ┌──────────────────┼──────────────────┐
            │                  │                   │
            ▼                  ▼                   ▼
   ┌────────────────┐ ┌───────────────┐ ┌─────────────────┐
   │ SOT_RP_TILEMAP │ │ SOT_RP_SPRITE │ │  SOT_RP_OVERLAY │
   │                │ │               │ │                 │
   │ Single draw    │ │ Instanced     │ │  UI layer       │
   │ call for whole │ │ sprites via   │ │  (not yet       │
   │ tilemap using  │ │ SSBO          │ │   implemented)  │
   │ vertex shader  │ │               │ │                 │
   │ tile calc      │ │ (WIP)         │ │                 │
   └───────┬────────┘ └───────┬───────┘ └─────────────────┘
           │                  │
           │    ┌─────────────┘
           ▼    ▼
   ┌────────────────┐
   │ SOT_RP_DEBUG   │
   │                │
   │ Wireframe      │
   │ collider       │
   │ shapes         │
   │ (toggle: key 1)│
   └────────────────┘
            │
      End Render Pass
            │
      Submit Command Buffer
```

---

## 5. GPU Data Upload Flow

```
    CPU Side                              GPU Side
  ─────────────                         ─────────────

  SOT_GPU_Data                         SOT_GPU_Buffers
  (staging struct)                     (per pipeline)
       │                                     │
       ├── vertexData ──► Map ──► Upload ──► vertexBuffer
       ├── indexData  ──► Map ──► Upload ──► indexBuffer
       ├── surfaces[] ──► Map ──► Upload ──► textures[]
       ├── tilemapData ─► Map ──► Upload ──► storageBuffer[0]  (tilemap pipeline)
       └── sprites[]  ──► Map ──► Upload ──► storageBuffer[0]  (sprite pipeline)
                              │
                     SOT_UploadBufferData()
                     (flag-based selector)
                              │
                    ┌─────────┴──────────┐
                    │  SOT_BUFFER_VERTEX │
                    │  SOT_BUFFER_INDEX  │
                    │  SOT_BUFFER_TEXTURE│
                    │  SOT_TILEMAP_SSB   │
                    │  SOT_SPRITES_SSB   │
                    └────────────────────┘
```

---

## 6. Per-Frame Update + Render Cycle

```
SDL_AppIterate()
       │
       ├── Calculate deltaTime
       │
       ├── UpdateScene()
       │       │
       │       ├── UpdateActor()          ◄── movement, animation, collision
       │       └── UpdateCameraPan()      ◄── camera follow
       │
       └── SOT_GPU_Render()
               │
               ├── Acquire command buffer
               ├── Acquire swapchain texture
               ├── Begin render pass (clear color)
               │
               ├── [TEST pipeline]        ◄── if SOT_RPF_TEST flag set
               │
               ├── SOT_GPU_RenderScene()
               │       │
               │       ├── SOT_GPU_RenderTilemap()
               │       │       ├── Bind tilemap pipeline
               │       │       ├── Push uniforms (pvMatrix, tilemapInfo)
               │       │       ├── Bind vertex/index/storage buffers
               │       │       ├── DrawIndexedPrimitives (6 indices x N tiles)
               │       │       │
               │       │       └── [DEBUG overlay]
               │       │               ├── Build collider wireframes
               │       │               ├── Upload debug geometry
               │       │               └── Render debug lines
               │       │
               │       ├── SOT_GPU_RenderActors()   ◄── WIP / empty
               │       └── [OVERLAY section]        ◄── WIP / empty
               │
               ├── End render pass
               └── Submit command buffer
```

---

## 7. Animation Data Flow

```
  JSON File                    In-Memory Model                      GPU
  (assets/animations/)
                          SOT_LoadAnimations()
  ┌──────────────┐              │
  │ monkey.json  │──────►  SOT_AnimationInfo
  │              │         │  atlasName
  │  sequences[] │         │  step_ms
  │  frames[]    │         │  count
  │  collider    │         │  sequences[128]
  └──────────────┘         │    ├── name ("Idle", "Run", ...)
                           │    ├── count (frame count)
                           │    └── frames[] (vec4: x, y, w, h)
                           │
                    SOT_ActorBindAnimations()
                           │
                           ▼
                    SOT_Actor.animations[256]
                    │  Each SOT_Animation:
                    │    .id
                    │    .atlasName
                    │    .step_ms
                    │    .sequence ◄── copied from SOT_AnimationSequence
                    │    .gpuInfo  ◄── ptr to SOT_GPU_SpriteInstance
                    │
                    │  Per frame tick:
                    │    Advance sequence.current
                    │    Update gpuSprite with new frameCoords
                    │
                    └──────────► GPU Storage Buffer (SSBO)
                                 sprite instance data
```

---

## 8. Collision System

```
                   Tilemap Loading
                        │
                        ▼
              SOT_GetCollider() ──► Parses Tiled objects
                        │             into cute_c2 shapes
                        ▼
            sot_collider_node_t       ┌──────────────────┐
            (linked list)             │ cute_c2 shapes   │
                   │                  │  c2Circle        │
                   ├──► static  ──►  │  c2Capsule       │
                   │   colliders     │  c2AABB          │
                   │                  │  c2Poly          │
                   │                  └──────────────────┘
            Actor Creation                   │
                   │                         │
                   ├──► dynamic              ▼
                       colliders       Collision Test
                                      (cute_c2 functions)
                                             │
                                             ▼
                                       c2Manifold
                                       (contact info)
```

---

## 9. Module Dependency Graph

```
                        sot_engine.h
                     (umbrella header)
                            │
          ┌─────────┬───────┼────────┬──────────┐
          ▼         ▼       ▼        ▼          ▼
     sot_scene  sot_actor  sot_   sot_gpu_   sot_camera
                           animation pipeline
          │         │       │        │
          │         ▼       │        │
          │    sot_physics  │        │
          │         │       │        │
          ▼         ▼       ▼        ▼
      sot_tilemap  sot_collider  sot_common ◄── SOT_Paths, vertex,
          │              │                       LoadShader, LoadImage
          │              │
          ▼              ▼
     sot_texture    sot_gpu_debug_info
          │
          ▼
       appstate ◄── forward-declares to break cycles


External Libraries:
  ├── SDL3          (windowing, GPU API, input)
  ├── SDL3_image    (PNG/BMP loading)
  ├── cglm          (vec/mat math)
  ├── cJSON         (JSON parsing)
  ├── cute_tiled    (Tiled map loading)
  ├── cute_c2       (2D collision detection)
  └── Box2D         (included, not integrated)
```

---

## 10. File Organization

```
src/
├── main.c                  ── SDL3 callbacks (init, event, iterate, quit)
├── sot_scene.c             ── Scene lifecycle + GPU init/render for tilemap & actors
├── sot_actor.c             ── Actor creation, update, animation tick, collision response
├── sot_animation.c         ── JSON animation loader
├── sot_gpu_pipeline.c      ── GPU state init, buffer management, render dispatch
├── sot_tilemap.c           ── Tiled map loading, tile/collider extraction
├── sot_camera.c            ── Camera view/projection, panning
├── sot_collider.c          ── Collider list management, debug drawing
├── sot_texture.c           ── Texture pool (linked list cache)
├── sot_common.c            ── Asset path init, shader/image loading helpers
├── sot_quad.c              ── Unit quad geometry (4 verts, 6 indices)
├── sot_gpu_debug_info.c    ── Debug line buffer management
└── includes/               ── All corresponding .h headers
```
