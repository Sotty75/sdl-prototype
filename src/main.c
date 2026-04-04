/*
 * SOT Engine — Main application entry point.
 *
 * The editor is the primary interface. It renders ImGui to the main window.
 * The scene is rendered to the virtual framebuffer and shown in an ImGui
 * viewport panel. When Play is pressed, a second window opens for the game.
 *
 * Uses SDL3's callback model (no traditional main loop).
 */

#define SDL_MAIN_USE_CALLBACKS 1

#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <box2d/box2d.h>
#include "sot_engine.h"
#include "sot_common.h"
#include "cglm.h"

//---- Globals Definition -----//
SOT_Paths Paths = {0};

//---- Locals Definition -----//
static AppState *as = NULL;
static SOT_Scene *currentScene = NULL;

// Forward declare the default action registration
static void RegisterDefaultActions(SOT_Input *input);

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    as = (AppState *)SDL_calloc(1, sizeof(AppState));
    if (!as) { return SDL_APP_FAILURE; }

    InitializeAssetsLoader();

    // ----------------------------- Initialize the graphics system -----------------------------------//
    SOT_GPU_InitRenderer(as, SOT_RP_TILEMAP_FLAG | SOT_RP_DEBUG_FLAG);

    // ----------------------------- Initialize the input system --------------------------------------//
    SOT_Input_Init(&as->input);
    RegisterDefaultActions(&as->input);

    // ----------------------------- Initialize the audio system ----------------------------------------//
    if (!SOT_Audio_Init(&as->audio)) {
        SDL_Log("Warning: Audio system failed to initialize, continuing without audio");
    }

    // ----------------------------- Initialize the UI system -------------------------------------------//
    SOT_UI_Init(&as->ui);

    // ----------------------------- Initialize the editor (ImGui) -----------------------------------//
    if (!SOT_Editor_Init(&as->editor, as->gpu)) {
        SDL_Log("FATAL: Editor failed to initialize");
        return SDL_APP_FAILURE;
    }

    // ----------------------------- Initialize the Lua scripting system -----------------------------//
    if (!SOT_Lua_Init(&as->lua)) {
        SDL_Log("Warning: Lua scripting failed to initialize, continuing without scripts");
    }

    // Initialize our main scene
    char *scene = "scene_00_.json";
    currentScene = SOT_InitializeScene(as, scene);
    if (currentScene == NULL)
        return SDL_APP_FAILURE;

    // Register Lua APIs now that scene is available, then fire deferred on_create callbacks
    if (as->lua.initialized) {
        SOT_Lua_RegisterAllAPIs(&as->lua, as, currentScene);
        SOT_Lua_RedirectPrint(&as->lua, &as->editor);
        for (int i = 0; i < currentScene->actorsCount; i++) {
            SOT_Actor *actor = &currentScene->actors[i];
            if (actor->enabled)
                SOT_Lua_CallOnCreate(&as->lua, actor);
        }
    }

    // World Initialization
    if (as->gpu->pipelineFlags & SOT_RP_TEST_FLAG)
        SOT_GPU_InitializeTestData(as->gpu);

    as->pTexturesPool = NULL;
    as->full_screen_enabled = false;
    as->last_step = SDL_GetTicks();
    as->debugInfo.displayColliders = false;
    *appstate = as;

    SOT_Editor_Log(&as->editor, "Engine initialized");
    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState *as = (AppState *)appstate;

    if (event->type == SDL_EVENT_QUIT)
        return SDL_APP_SUCCESS;

    // Editor gets all events (ImGui input)
    SOT_Editor_ProcessEvent(&as->editor, event);

    // If game window is closed, stop the game
    if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && as->editor.gameWindow) {
        SDL_WindowID gameWinId = SDL_GetWindowID(as->editor.gameWindow);
        if (event->window.windowID == gameWinId) {
            SOT_Editor_StopGame(&as->editor, as->gpu, currentScene);
            return SDL_APP_CONTINUE;
        }
    }

    // When game is playing, forward input events to the game input system
    if (as->editor.playing) {
        SOT_Input_ProcessEvent(&as->input, event);
    }

    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *as = (AppState *)appstate;
    const Uint64 now = SDL_GetTicks();
    const float deltaTime = (now - as->last_step) / 1000.0f;
    as->last_step = now;

    // ======================== GAME LOGIC (only when playing) ========================
    if (as->editor.playing) {
        SOT_Input_BeginFrame(&as->input);
        SOT_Input_EndFrame(&as->input);

        SOT_Display *d = &as->gpu->display;
        as->input.mouse.gameX = (as->input.mouse.windowX - d->viewportX) / (float)d->scaleFactor;
        as->input.mouse.gameY = (as->input.mouse.windowY - d->viewportY) / (float)d->scaleFactor;

        SOT_UI_HandleInput(&as->ui,
            SOT_Input_IsJustPressed(&as->input, "move_up"),
            SOT_Input_IsJustPressed(&as->input, "move_down"),
            SOT_Input_IsJustPressed(&as->input, "interact"),
            SOT_Input_IsJustPressed(&as->input, "quit"));

        SOT_UI_BeginFrame(&as->ui);
        UpdateScene(as, currentScene, deltaTime);
        SOT_UI_Update(&as->ui, deltaTime);
        SOT_Audio_Update(&as->audio);
    }

    // ======================== PENDING SCENE LOAD (from editor) ========================
    if (as->editor.pendingSceneLoad[0] != '\0') {
        SOT_Scene *newScene = SOT_InitializeScene(as, as->editor.pendingSceneLoad);
        if (newScene) {
            if (currentScene) DestroyScene(currentScene);
            currentScene = newScene;
            SOT_Editor_Log(&as->editor, "Loaded scene: %s", as->editor.pendingSceneLoad);

            // Re-register Lua APIs with new scene
            SOT_Lua_RegisterAllAPIs(&as->lua, as, currentScene);
            for (int i = 0; i < currentScene->actorsCount; i++) {
                SOT_Lua_CallOnCreate(&as->lua, &currentScene->actors[i]);
            }
        } else {
            SOT_Editor_LogError(&as->editor, "Failed to load scene: %s", as->editor.pendingSceneLoad);
        }
        as->editor.pendingSceneLoad[0] = '\0';
    }

    // ======================== RENDER SCENE TO FRAMEBUFFER (always, for preview) ========================
    // In editor mode (not playing), use the editor camera to show a wider view
    if (!as->editor.playing && currentScene) {
        // Initialize editor camera from scene on first frame
        if (!as->editor.editorCam.initialized && currentScene->tilemap) {
            SOT_GPU_TilemapInfo *ti = &currentScene->tilemap->gpuTilemapInfo;
            as->editor.editorCam.posX = (float)(ti->COLUMNS * ti->TILE_WIDTH) * 0.5f;
            as->editor.editorCam.posY = (float)(ti->ROWS * ti->TILE_HEIGHT) * -0.5f;
            as->editor.editorCam.zoom = 0.5f;
            as->editor.editorCam.initialized = true;
        }

        // Build editor camera PV matrix and swap it into the scene temporarily
        mat4 savedPV;
        glm_mat4_copy(currentScene->worldCamera.pvMatrix, savedPV);

        SOT_EditorCam_BuildPVMatrix(&as->editor.editorCam, &currentScene->worldCamera,
                                     currentScene->worldCamera.pvMatrix);

        // Also save a copy for the editor overlays
        glm_mat4_copy(currentScene->worldCamera.pvMatrix, as->editor.editorCamState.pvMatrix);

        SOT_GPU_RenderSceneToFramebuffer(as->gpu, currentScene);

        // Restore game camera
        glm_mat4_copy(savedPV, currentScene->worldCamera.pvMatrix);
    } else {
        SOT_GPU_RenderSceneToFramebuffer(as->gpu, currentScene);
    }

    // ======================== EDITOR (always) ========================
    SOT_Editor_BeginFrame(&as->editor, as->gpu);
    SOT_Editor_Render(&as->editor, as, currentScene);
    SOT_Editor_EndFrame(&as->editor);
    SOT_Editor_RenderToSwapchain(&as->editor, as->gpu);

    // ======================== GAME WINDOW (only when playing) ========================
    if (as->editor.playing) {
        SOT_Editor_RenderGameWindow(&as->editor, as->gpu);
    }

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    if (as == NULL) return;

    if (currentScene != NULL)
        DestroyScene(currentScene);

    // Destroy editor (ImGui + game window)
    SOT_Editor_Shutdown(&as->editor, as->gpu);

    // Destroy UI system
    SOT_UI_Shutdown(&as->ui);

    // Destroy audio system
    SOT_Audio_Shutdown(&as->audio);

    // Destroy scripting system
    SOT_Lua_Shutdown(&as->lua);

    // Destroy input system (closes gamepad)
    SOT_Input_Destroy(&as->input);

    if (as->gpu != NULL) {
        SOT_GPU_State *gpu = as->gpu;

        // Free CPU-side debug info
        if (gpu->debugInfo != NULL) {
            if (gpu->debugInfo->vertexList != NULL)
                SDL_free(gpu->debugInfo->vertexList);
            SDL_free(gpu->debugInfo);
            gpu->debugInfo = NULL;
        }

        // Release GPU resources before destroying the device
        if (gpu->device != NULL) {
            SDL_WaitForGPUIdle(gpu->device);

            for (int i = 0; i < 16; i++) {
                if (gpu->pipeline[i] != NULL)
                    SDL_ReleaseGPUGraphicsPipeline(gpu->device, gpu->pipeline[i]);
            }

            for (int i = 0; i < 16; i++) {
                if (gpu->buffers[i].vertexBuffer != NULL)
                    SDL_ReleaseGPUBuffer(gpu->device, gpu->buffers[i].vertexBuffer);
                if (gpu->buffers[i].indexBuffer != NULL)
                    SDL_ReleaseGPUBuffer(gpu->device, gpu->buffers[i].indexBuffer);
                for (int j = 0; j < 8; j++) {
                    if (gpu->buffers[i].storageBuffer[j] != NULL)
                        SDL_ReleaseGPUBuffer(gpu->device, gpu->buffers[i].storageBuffer[j]);
                }
                for (int j = 0; j < gpu->buffers[i].texturesCount; j++) {
                    if (gpu->buffers[i].textures[j] != NULL)
                        SDL_ReleaseGPUTexture(gpu->device, gpu->buffers[i].textures[j]);
                }
            }

            SOT_Display_Destroy(gpu);

            if (gpu->nearestSampler != NULL)
                SDL_ReleaseGPUSampler(gpu->device, gpu->nearestSampler);

            SDL_ReleaseWindowFromGPUDevice(gpu->device, gpu->window);
            SDL_DestroyGPUDevice(gpu->device);
        }

        if (gpu->window != NULL)
            SDL_DestroyWindow(gpu->window);

        SDL_free(gpu);
    }

    SDL_free(Paths.Shaders);
    SDL_free(Paths.Textures);
    SDL_free(Paths.TiledMaps);
    SDL_free(Paths.Animations);
    SDL_free(Paths.Scenes);

    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
    SDL_free(as);
}

// ---- Default action bindings ----

static void RegisterDefaultActions(SOT_Input *input)
{
    // Movement
    SOT_Input_AddAction(input, "move_left");
    SOT_Input_BindKey(input, "move_left", SDL_SCANCODE_LEFT);
    SOT_Input_BindKey(input, "move_left", SDL_SCANCODE_A);
    SOT_Input_BindGamepadAxis(input, "move_left", SDL_GAMEPAD_AXIS_LEFTX, -1.0f);

    SOT_Input_AddAction(input, "move_right");
    SOT_Input_BindKey(input, "move_right", SDL_SCANCODE_RIGHT);
    SOT_Input_BindKey(input, "move_right", SDL_SCANCODE_D);
    SOT_Input_BindGamepadAxis(input, "move_right", SDL_GAMEPAD_AXIS_LEFTX, 1.0f);

    SOT_Input_AddAction(input, "move_up");
    SOT_Input_BindKey(input, "move_up", SDL_SCANCODE_UP);
    SOT_Input_BindKey(input, "move_up", SDL_SCANCODE_W);
    SOT_Input_BindGamepadAxis(input, "move_up", SDL_GAMEPAD_AXIS_LEFTY, -1.0f);

    SOT_Input_AddAction(input, "move_down");
    SOT_Input_BindKey(input, "move_down", SDL_SCANCODE_DOWN);
    SOT_Input_BindKey(input, "move_down", SDL_SCANCODE_S);
    SOT_Input_BindGamepadAxis(input, "move_down", SDL_GAMEPAD_AXIS_LEFTY, 1.0f);

    // Actions
    SOT_Input_AddAction(input, "jump");
    SOT_Input_BindKey(input, "jump", SDL_SCANCODE_SPACE);
    SOT_Input_BindGamepadButton(input, "jump", SDL_GAMEPAD_BUTTON_SOUTH);

    SOT_Input_AddAction(input, "shoot");
    SOT_Input_BindKey(input, "shoot", SDL_SCANCODE_Z);
    SOT_Input_BindGamepadButton(input, "shoot", SDL_GAMEPAD_BUTTON_WEST);

    SOT_Input_AddAction(input, "interact");
    SOT_Input_BindKey(input, "interact", SDL_SCANCODE_E);
    SOT_Input_BindGamepadButton(input, "interact", SDL_GAMEPAD_BUTTON_NORTH);

    // System
    SOT_Input_AddAction(input, "quit");
    SOT_Input_BindKey(input, "quit", SDL_SCANCODE_ESCAPE);

    SOT_Input_AddAction(input, "fullscreen_toggle");
    SOT_Input_BindKey(input, "fullscreen_toggle", SDL_SCANCODE_F11);

    SOT_Input_AddAction(input, "debug_colliders");
    SOT_Input_BindKey(input, "debug_colliders", SDL_SCANCODE_F1);
}
