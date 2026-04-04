#ifndef SOT_EDITOR_H_
#define SOT_EDITOR_H_

#include <stdbool.h>
#include <SDL3/SDL.h>
#include "sot_camera.h"

// Forward declarations
struct AppState;
struct SOT_Scene;
struct SOT_GPU_State;
struct SOT_AnimationInfo;

// ---- Log severity ----
typedef enum SOT_LogSeverity {
    SOT_LOG_INFO = 0,
    SOT_LOG_WARNING,
    SOT_LOG_ERROR,
    SOT_LOG_LUA,
} SOT_LogSeverity;

// ---- Undo/redo system ----
typedef enum SOT_UndoType {
    SOT_UNDO_POSITION,
    SOT_UNDO_SCALE,
    SOT_UNDO_ENABLED,
    SOT_UNDO_PROP_NUMBER,
    SOT_UNDO_PROP_BOOL,
} SOT_UndoType;

typedef struct SOT_UndoEntry {
    SOT_UndoType type;
    int actorIndex;
    int propertyIndex;
    union {
        float position[2];
        float scale[2];
        bool enabled;
        float number;
        bool boolean;
    } old;
} SOT_UndoEntry;

// ---- Play/stop actor snapshot ----
typedef struct SOT_ActorSnapshot {
    float position[2];
    float scale[2];
    bool enabled;
    int propertyCount;
    // Properties stored as opaque blob; size SOT_ACTOR_MAX_PROPERTIES * sizeof(SOT_Property)
    // Allocated and copied via memcpy in sot_editor.c which has the full type
    void *propertyData;
} SOT_ActorSnapshot;

// ---- Editor theme system ----
typedef enum SOT_ThemeID {
    SOT_THEME_DARK = 0,     // ImGui stock dark
    SOT_THEME_LIGHT,        // ImGui stock light
    SOT_THEME_MODERN,       // Professional IDE-style (blue accent)
    SOT_THEME_PICO8,        // PICO-8 palette
    SOT_THEME_C64,          // Commodore 64 palette
    SOT_THEME_ZXSPECTRUM,   // ZX Spectrum palette
    SOT_THEME_NES,          // NES/Famicom palette
    SOT_THEME_AMIGA,        // Amiga Workbench 2.0 palette
    SOT_THEME_COUNT
} SOT_ThemeID;

// ---- Editor preferences ----
typedef struct SOT_EditorPrefs {
    SOT_ThemeID theme;
    float fontSize;
    float gridColor[4];
    int gridSize;
    bool fontDirty;
} SOT_EditorPrefs;

// ---- Animator Editor state ----
typedef struct SOT_AnimatorEditor {
    bool active;
    bool dirty;                         // unsaved changes flag

    // Working copy of loaded animation data
    struct SOT_AnimationInfo *animInfo;
    char sourceFilename[128];           // e.g. "monkey.json"

    // Atlas texture (standalone GPU texture for ImGui display)
    SDL_GPUTexture *atlasTexture;
    int atlasWidth, atlasHeight;

    // Grid config
    int spriteWidth, spriteHeight;      // cell size (default from first frame or 16)
    int gridCols, gridRows;             // computed

    // Selection
    int selectedSequence;               // -1 = none
    int selectedFrame;                  // -1 = none
    bool cellSelection[256];            // toggled grid cells for frame picking

    // Viewport interaction
    float zoom;
    float panX, panY;

    // Preview playback
    bool previewPlaying;
    int previewFrame;
    uint32_t previewElapsedMs;
    uint32_t previewLastTick;

    // UI buffers
    char newSequenceName[64];
    bool showNewSequencePopup;
} SOT_AnimatorEditor;

// ---- Editor camera ----
typedef struct SOT_EditorCamera {
    float posX, posY;       // Center of editor view in world coords
    float zoom;             // 1.0 = match game camera, <1 = zoom out, >1 = zoom in
    bool initialized;
} SOT_EditorCamera;

// ---- Editor state ----
typedef struct SOT_Editor {
    bool initialized;
    bool playing;           // true = game running in game window
    int selectedActor;      // Index of selected actor in scene (-1 = none)

    // Editor camera (independent from game camera)
    SOT_EditorCamera editorCam;
    sot_camera editorCamState;      // built each frame from editorCam

    // Game window (created on Play, destroyed on Stop)
    SDL_Window *gameWindow;

    // Console log ring buffer
    char logLines[256][256];
    SOT_LogSeverity logSeverity[256];
    int logCount;
    int logScroll;

    // FPS counter
    float fpsTimer;
    int frameCount;
    float currentFPS;

    // Undo/redo stack
    SOT_UndoEntry undoStack[50];
    int undoCount;          // total valid entries
    int undoTop;            // current position (next free slot)

    // Play/stop snapshot (heap-allocated on Play)
    SOT_ActorSnapshot *snapshot;
    int snapshotCount;

    // Visibility toggles
    bool showTilemap;
    bool showDebugOverlay;
    bool showGrid;
    bool showGizmos;

    // Panel visibility
    bool showAssetBrowser;
    bool showAnimPreview;
    bool showPreferences;
    bool showAnimatorEditor;

    // Asset browser state
    char abSelectedFolder[512];     // Currently selected folder (relative to assets/)
    float abSplitRatio;             // Left/right splitter ratio (0.0-1.0)
    float abIconSize;               // Icon size in pixels (32-128)
    char abFilter[64];              // File filter text
    bool abNeedsRefresh;            // Trigger filesystem re-scan

    // Asset browser thumbnail cache
    struct {
        char filename[128];
        SDL_GPUTexture *texture;
        int width, height;
    } abThumbnails[128];
    int abThumbnailCount;

    // Animator editor
    SOT_AnimatorEditor animator;

    // Viewport drag-to-move
    int dragActor;
    float dragOffsetX;
    float dragOffsetY;
    bool isDragging;

    // Move gizmo state
    int gizmoDragAxis;          // 0=none, 1=X, 2=Y, 3=both (center box)
    bool gizmoDragging;
    float gizmoDragStartX;      // actor pos at drag start (for undo)
    float gizmoDragStartY;

    // Snap to grid
    bool snapToGrid;

    // Hierarchy panel
    char hierarchyFilter[64];       // Search/filter text
    bool isRenaming;                // Inline rename active
    int renamingActor;              // Actor index being renamed
    char renameBuf[64];             // Rename text buffer

    // Lua console input
    char luaInputBuf[256];

    // Deferred scene load
    char pendingSceneLoad[128];

    // Actor template editing
    struct SOT_ActorTemplate *editingTemplate;  // Working copy of selected actor's template
    bool templateDirty;                          // Unsaved template changes
    int lastInspectedActor;                      // Track selection changes

    // Preferences
    SOT_EditorPrefs prefs;
} SOT_Editor;

// ---- Editor camera ----
void SOT_EditorCam_BuildPVMatrix(SOT_EditorCamera *ecam, sot_camera *gameCam, mat4 outPV);

// ---- Lifecycle ----
bool SOT_Editor_Init(SOT_Editor *editor, struct SOT_GPU_State *gpu);
void SOT_Editor_Shutdown(SOT_Editor *editor, struct SOT_GPU_State *gpu);

// ---- Per-frame ----
void SOT_Editor_ProcessEvent(SOT_Editor *editor, SDL_Event *event);
void SOT_Editor_BeginFrame(SOT_Editor *editor, struct SOT_GPU_State *gpu);
void SOT_Editor_Render(SOT_Editor *editor, struct AppState *as, struct SOT_Scene *scene);
void SOT_Editor_EndFrame(SOT_Editor *editor);

// ---- GPU rendering (editor window swapchain) ----
void SOT_Editor_RenderToSwapchain(SOT_Editor *editor, struct SOT_GPU_State *gpu);

// ---- Game window management ----
void SOT_Editor_StartGame(SOT_Editor *editor, struct SOT_GPU_State *gpu, struct SOT_Scene *scene);
void SOT_Editor_StopGame(SOT_Editor *editor, struct SOT_GPU_State *gpu, struct SOT_Scene *scene);

// ---- Render game to its window ----
void SOT_Editor_RenderGameWindow(SOT_Editor *editor, struct SOT_GPU_State *gpu);

// ---- Logging ----
void SOT_Editor_Log(SOT_Editor *editor, const char *fmt, ...);
void SOT_Editor_LogWarn(SOT_Editor *editor, const char *fmt, ...);
void SOT_Editor_LogError(SOT_Editor *editor, const char *fmt, ...);
void SOT_Editor_LogLua(SOT_Editor *editor, const char *fmt, ...);

#endif
