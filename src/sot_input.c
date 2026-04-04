#include "sot_input.h"
#include "sot_display.h"
#include <string.h>

// ---- Internal helpers ----

static SOT_InputAction* FindAction(SOT_Input *input, const char *name)
{
    for (int i = 0; i < input->actionCount; i++) {
        if (SDL_strcmp(input->actions[i].name, name) == 0)
            return &input->actions[i];
    }
    return NULL;
}

static const SOT_InputAction* FindActionConst(const SOT_Input *input, const char *name)
{
    for (int i = 0; i < input->actionCount; i++) {
        if (SDL_strcmp(input->actions[i].name, name) == 0)
            return &input->actions[i];
    }
    return NULL;
}

static bool IsBindingPressed(const SOT_Input *input, const SOT_InputBinding *bind)
{
    switch (bind->type) {
        case SOT_BIND_KEY:
            return input->keysCurrent[bind->scancode];

        case SOT_BIND_GAMEPAD_BUTTON:
            if (input->gamepad == NULL) return false;
            return SDL_GetGamepadButton(input->gamepad, bind->gamepadButton);

        case SOT_BIND_GAMEPAD_AXIS: {
            if (input->gamepad == NULL) return false;
            Sint16 raw = SDL_GetGamepadAxis(input->gamepad, bind->gamepadAxis.axis);
            float normalized = raw / 32767.0f;
            if (bind->gamepadAxis.direction > 0)
                return normalized > input->gamepadDeadzone;
            else
                return normalized < -input->gamepadDeadzone;
        }

        case SOT_BIND_MOUSE_BUTTON:
            return input->mouse.buttons[bind->mouseButton];

        default:
            return false;
    }
}

static float GetBindingAxis(const SOT_Input *input, const SOT_InputBinding *bind)
{
    switch (bind->type) {
        case SOT_BIND_KEY:
            return input->keysCurrent[bind->scancode] ? 1.0f : 0.0f;

        case SOT_BIND_GAMEPAD_BUTTON:
            if (input->gamepad == NULL) return 0.0f;
            return SDL_GetGamepadButton(input->gamepad, bind->gamepadButton) ? 1.0f : 0.0f;

        case SOT_BIND_GAMEPAD_AXIS: {
            if (input->gamepad == NULL) return 0.0f;
            Sint16 raw = SDL_GetGamepadAxis(input->gamepad, bind->gamepadAxis.axis);
            float normalized = raw / 32767.0f;
            // Apply deadzone
            if (bind->gamepadAxis.direction > 0) {
                if (normalized < input->gamepadDeadzone) return 0.0f;
                return (normalized - input->gamepadDeadzone) / (1.0f - input->gamepadDeadzone);
            } else {
                if (normalized > -input->gamepadDeadzone) return 0.0f;
                return -(normalized + input->gamepadDeadzone) / (1.0f - input->gamepadDeadzone);
            }
        }

        default:
            return 0.0f;
    }
}

// ---- Lifecycle ----

void SOT_Input_Init(SOT_Input *input)
{
    SDL_memset(input, 0, sizeof(SOT_Input));
    input->gamepadDeadzone = 0.25f;
    input->mouse.visible = true;

    // Get pointer to SDL's keyboard state array (valid for the program's lifetime)
    int numKeys = 0;
    input->keysCurrent = SDL_GetKeyboardState(&numKeys);

    // Open the first connected gamepad if any
    int count = 0;
    SDL_JoystickID *gamepads = SDL_GetGamepads(&count);
    if (gamepads != NULL && count > 0) {
        input->gamepad = SDL_OpenGamepad(gamepads[0]);
    }
    SDL_free(gamepads);
}

void SOT_Input_Destroy(SOT_Input *input)
{
    if (input->gamepad != NULL) {
        SDL_CloseGamepad(input->gamepad);
        input->gamepad = NULL;
    }
}

// ---- Per-frame update ----

void SOT_Input_BeginFrame(SOT_Input *input)
{
    // Snapshot previous keyboard state
    SDL_memcpy(input->keysPrevious, input->keysCurrent, SDL_SCANCODE_COUNT * sizeof(bool));

    // Snapshot previous mouse button state
    SDL_memcpy(input->mouse.buttonsLast, input->mouse.buttons, sizeof(input->mouse.buttons));
}

void SOT_Input_EndFrame(SOT_Input *input)
{
    // Update mouse position and buttons
    float mx, my;
    Uint32 mouseButtons = SDL_GetMouseState(&mx, &my);
    input->mouse.windowX = mx;
    input->mouse.windowY = my;
    input->mouse.buttons[SDL_BUTTON_LEFT]   = (mouseButtons & SDL_BUTTON_LMASK) != 0;
    input->mouse.buttons[SDL_BUTTON_RIGHT]  = (mouseButtons & SDL_BUTTON_RMASK) != 0;
    input->mouse.buttons[SDL_BUTTON_MIDDLE] = (mouseButtons & SDL_BUTTON_MMASK) != 0;

    // Update all action states
    for (int i = 0; i < input->actionCount; i++) {
        SOT_InputAction *action = &input->actions[i];
        bool wasPressed = action->pressed;
        bool isPressed = false;
        float maxAxis = 0.0f;

        for (int b = 0; b < action->bindingCount; b++) {
            if (IsBindingPressed(input, &action->bindings[b]))
                isPressed = true;

            float axis = GetBindingAxis(input, &action->bindings[b]);
            if (axis > maxAxis)
                maxAxis = axis;
        }

        action->pressed = isPressed;
        action->justPressed = isPressed && !wasPressed;
        action->justReleased = !isPressed && wasPressed;
        action->axisValue = maxAxis;
    }
}

// ---- Event processing ----

void SOT_Input_ProcessEvent(SOT_Input *input, const SDL_Event *event)
{
    switch (event->type) {
        case SDL_EVENT_GAMEPAD_ADDED: {
            if (input->gamepad == NULL)
                input->gamepad = SDL_OpenGamepad(event->gdevice.which);
            break;
        }
        case SDL_EVENT_GAMEPAD_REMOVED: {
            if (input->gamepad != NULL) {
                SDL_CloseGamepad(input->gamepad);
                input->gamepad = NULL;
            }
            break;
        }
        default:
            break;
    }
}

// ---- Action registration ----

int SOT_Input_AddAction(SOT_Input *input, const char *name)
{
    if (input->actionCount >= SOT_INPUT_MAX_ACTIONS) {
        SDL_Log("SOT_Input: max actions reached (%d)", SOT_INPUT_MAX_ACTIONS);
        return -1;
    }

    SOT_InputAction *action = &input->actions[input->actionCount];
    SDL_memset(action, 0, sizeof(SOT_InputAction));
    SDL_strlcpy(action->name, name, SOT_INPUT_ACTION_NAME_LEN);
    return input->actionCount++;
}

void SOT_Input_BindKey(SOT_Input *input, const char *action, SDL_Scancode scancode)
{
    SOT_InputAction *a = FindAction(input, action);
    if (a == NULL || a->bindingCount >= SOT_INPUT_MAX_BINDINGS) return;

    a->bindings[a->bindingCount] = (SOT_InputBinding) {
        .type = SOT_BIND_KEY,
        .scancode = scancode,
    };
    a->bindingCount++;
}

void SOT_Input_BindGamepadButton(SOT_Input *input, const char *action, SDL_GamepadButton button)
{
    SOT_InputAction *a = FindAction(input, action);
    if (a == NULL || a->bindingCount >= SOT_INPUT_MAX_BINDINGS) return;

    a->bindings[a->bindingCount] = (SOT_InputBinding) {
        .type = SOT_BIND_GAMEPAD_BUTTON,
        .gamepadButton = button,
    };
    a->bindingCount++;
}

void SOT_Input_BindGamepadAxis(SOT_Input *input, const char *action, SDL_GamepadAxis axis, float direction)
{
    SOT_InputAction *a = FindAction(input, action);
    if (a == NULL || a->bindingCount >= SOT_INPUT_MAX_BINDINGS) return;

    a->bindings[a->bindingCount] = (SOT_InputBinding) {
        .type = SOT_BIND_GAMEPAD_AXIS,
        .gamepadAxis = { .axis = axis, .direction = direction },
    };
    a->bindingCount++;
}

void SOT_Input_BindMouseButton(SOT_Input *input, const char *action, Uint8 button)
{
    SOT_InputAction *a = FindAction(input, action);
    if (a == NULL || a->bindingCount >= SOT_INPUT_MAX_BINDINGS) return;

    a->bindings[a->bindingCount] = (SOT_InputBinding) {
        .type = SOT_BIND_MOUSE_BUTTON,
        .mouseButton = button,
    };
    a->bindingCount++;
}

// ---- Action queries ----

bool SOT_Input_IsPressed(const SOT_Input *input, const char *action)
{
    const SOT_InputAction *a = FindActionConst(input, action);
    return a != NULL && a->pressed;
}

bool SOT_Input_IsJustPressed(const SOT_Input *input, const char *action)
{
    const SOT_InputAction *a = FindActionConst(input, action);
    return a != NULL && a->justPressed;
}

bool SOT_Input_IsJustReleased(const SOT_Input *input, const char *action)
{
    const SOT_InputAction *a = FindActionConst(input, action);
    return a != NULL && a->justReleased;
}

float SOT_Input_GetAxis(const SOT_Input *input, const char *action)
{
    const SOT_InputAction *a = FindActionConst(input, action);
    return a != NULL ? a->axisValue : 0.0f;
}

// ---- Mouse queries ----

void SOT_Input_GetMousePosition(const SOT_Input *input, float *x, float *y)
{
    *x = input->mouse.gameX;
    *y = input->mouse.gameY;
}

bool SOT_Input_IsMouseButtonPressed(const SOT_Input *input, Uint8 button)
{
    if (button >= 5) return false;
    return input->mouse.buttons[button];
}

bool SOT_Input_IsMouseButtonJustPressed(const SOT_Input *input, Uint8 button)
{
    if (button >= 5) return false;
    return input->mouse.buttons[button] && !input->mouse.buttonsLast[button];
}
