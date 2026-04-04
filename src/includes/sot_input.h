#ifndef SOT_INPUT_H_
#define SOT_INPUT_H_

#include <SDL3/SDL.h>
#include <stdbool.h>

// ---- Configuration limits ----
#define SOT_INPUT_MAX_ACTIONS     32
#define SOT_INPUT_MAX_BINDINGS     4   // max physical inputs per action
#define SOT_INPUT_ACTION_NAME_LEN 32

// ---- Binding types ----
typedef enum SOT_InputBindingType {
    SOT_BIND_NONE = 0,
    SOT_BIND_KEY,            // Keyboard scancode
    SOT_BIND_GAMEPAD_BUTTON, // Gamepad button
    SOT_BIND_GAMEPAD_AXIS,   // Gamepad axis (positive or negative half)
    SOT_BIND_MOUSE_BUTTON,   // Mouse button
} SOT_InputBindingType;

typedef struct SOT_InputBinding {
    SOT_InputBindingType type;
    union {
        SDL_Scancode scancode;
        SDL_GamepadButton gamepadButton;
        struct {
            SDL_GamepadAxis axis;
            float direction;     // +1.0 or -1.0 (which half of the axis)
        } gamepadAxis;
        Uint8 mouseButton;       // SDL_BUTTON_LEFT, etc.
    };
} SOT_InputBinding;

// ---- Action definition ----
typedef struct SOT_InputAction {
    char name[SOT_INPUT_ACTION_NAME_LEN];
    SOT_InputBinding bindings[SOT_INPUT_MAX_BINDINGS];
    int bindingCount;

    // Runtime state (updated each frame)
    bool pressed;       // Currently held
    bool justPressed;   // Became pressed this frame
    bool justReleased;  // Was released this frame
    float axisValue;    // Analog value [-1.0, 1.0] (for axis bindings)
} SOT_InputAction;

// ---- Mouse state ----
typedef struct SOT_MouseState {
    float gameX, gameY;     // Position in game (virtual framebuffer) coordinates
    float windowX, windowY; // Position in window coordinates
    bool buttons[5];        // Current button state
    bool buttonsLast[5];    // Previous frame button state
    bool visible;
} SOT_MouseState;

// ---- Input system state ----
typedef struct SOT_Input {
    SOT_InputAction actions[SOT_INPUT_MAX_ACTIONS];
    int actionCount;

    // Raw keyboard state snapshots
    const bool *keysCurrent;    // SDL's keyboard state array (pointer, not owned)
    bool keysPrevious[SDL_SCANCODE_COUNT];

    // Gamepad
    SDL_Gamepad *gamepad;
    float gamepadDeadzone;      // Default 0.25 (normalized)

    // Mouse
    SOT_MouseState mouse;

} SOT_Input;

// ---- Public API ----

// Lifecycle
void SOT_Input_Init(SOT_Input *input);
void SOT_Input_Destroy(SOT_Input *input);

// Call once per frame BEFORE processing events to snapshot previous state
void SOT_Input_BeginFrame(SOT_Input *input);

// Call once per frame AFTER all events to compute just-pressed/just-released
void SOT_Input_EndFrame(SOT_Input *input);

// Route an SDL event through the input system (gamepad connect/disconnect, etc.)
void SOT_Input_ProcessEvent(SOT_Input *input, const SDL_Event *event);

// Action registration
int SOT_Input_AddAction(SOT_Input *input, const char *name);
void SOT_Input_BindKey(SOT_Input *input, const char *action, SDL_Scancode scancode);
void SOT_Input_BindGamepadButton(SOT_Input *input, const char *action, SDL_GamepadButton button);
void SOT_Input_BindGamepadAxis(SOT_Input *input, const char *action, SDL_GamepadAxis axis, float direction);
void SOT_Input_BindMouseButton(SOT_Input *input, const char *action, Uint8 button);

// Action queries
bool SOT_Input_IsPressed(const SOT_Input *input, const char *action);
bool SOT_Input_IsJustPressed(const SOT_Input *input, const char *action);
bool SOT_Input_IsJustReleased(const SOT_Input *input, const char *action);
float SOT_Input_GetAxis(const SOT_Input *input, const char *action);

// Mouse queries (position in game/virtual framebuffer coordinates)
void SOT_Input_GetMousePosition(const SOT_Input *input, float *x, float *y);
bool SOT_Input_IsMouseButtonPressed(const SOT_Input *input, Uint8 button);
bool SOT_Input_IsMouseButtonJustPressed(const SOT_Input *input, Uint8 button);

#endif
