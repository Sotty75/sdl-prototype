#ifndef SOT_UI_H_
#define SOT_UI_H_

#include <stdbool.h>
#include <SDL3/SDL.h>
#include "cglm.h"

// ---- Bitmap font ----

#define SOT_FONT_MAX_GLYPHS 256

typedef struct SOT_FontGlyph {
    int x, y, w, h;     // Source rect in atlas
    int xOffset, yOffset; // Rendering offset
    int xAdvance;         // Advance to next character
} SOT_FontGlyph;

typedef struct SOT_BitmapFont {
    char name[64];
    SOT_FontGlyph glyphs[SOT_FONT_MAX_GLYPHS];
    int lineHeight;
    int base;
    char atlasFile[128];
    bool loaded;
} SOT_BitmapFont;

// ---- Text alignment ----
typedef enum SOT_TextAlign {
    SOT_ALIGN_LEFT = 0,
    SOT_ALIGN_CENTER,
    SOT_ALIGN_RIGHT,
} SOT_TextAlign;

// ---- UI draw command types ----
typedef enum SOT_UICommandType {
    SOT_UI_CMD_TEXT,
    SOT_UI_CMD_RECT,       // Solid color rect
    SOT_UI_CMD_PANEL,      // 9-slice panel
    SOT_UI_CMD_IMAGE,      // Textured rect
} SOT_UICommandType;

// ---- UI draw command ----
#define SOT_UI_MAX_TEXT 256

typedef struct SOT_UICommand {
    SOT_UICommandType type;
    float x, y, w, h;
    vec4 color;            // RGBA
    char text[SOT_UI_MAX_TEXT];
    SOT_TextAlign align;
    int fontSize;          // Scale factor (1 = native)
} SOT_UICommand;

// ---- Dialog box ----
typedef struct SOT_DialogBox {
    bool active;
    char text[1024];
    char speaker[64];
    int revealIndex;       // Characters revealed so far (typewriter effect)
    float revealTimer;
    float revealSpeed;     // Characters per second
    bool fullyRevealed;

    // Choices
    char choices[4][128];
    int choiceCount;
    int selectedChoice;
    bool waitingForChoice;
} SOT_DialogBox;

// ---- Menu ----
#define SOT_MENU_MAX_ITEMS 16

typedef struct SOT_MenuItem {
    char label[64];
    bool enabled;
} SOT_MenuItem;

typedef struct SOT_Menu {
    bool active;
    char title[64];
    SOT_MenuItem items[SOT_MENU_MAX_ITEMS];
    int itemCount;
    int selectedIndex;
} SOT_Menu;

// ---- UI state ----
#define SOT_UI_MAX_COMMANDS 512

typedef struct SOT_UI {
    bool initialized;

    // Font
    SOT_BitmapFont font;

    // Draw command buffer (rebuilt each frame)
    SOT_UICommand commands[SOT_UI_MAX_COMMANDS];
    int commandCount;

    // Dialog
    SOT_DialogBox dialog;

    // Menu
    SOT_Menu menu;

    // Vertex buffer for UI quads (rebuilt each frame)
    float *vertices;
    int vertexCount;
    int vertexCapacity;
} SOT_UI;

// ---- Lifecycle ----
bool SOT_UI_Init(SOT_UI *ui);
void SOT_UI_Shutdown(SOT_UI *ui);

// ---- Font loading ----
bool SOT_UI_LoadFont(SOT_UI *ui, const char *fontJsonFile);

// ---- Per-frame ----
void SOT_UI_BeginFrame(SOT_UI *ui);
void SOT_UI_Update(SOT_UI *ui, float deltaTime);

// ---- Immediate-mode draw commands ----
void SOT_UI_DrawText(SOT_UI *ui, const char *text, float x, float y, vec4 color, SOT_TextAlign align);
void SOT_UI_DrawRect(SOT_UI *ui, float x, float y, float w, float h, vec4 color);

// ---- Dialog box ----
void SOT_UI_ShowDialog(SOT_UI *ui, const char *text, const char *speaker);
void SOT_UI_AddDialogChoice(SOT_UI *ui, const char *choiceText);
void SOT_UI_HideDialog(SOT_UI *ui);
bool SOT_UI_IsDialogActive(const SOT_UI *ui);
int  SOT_UI_GetDialogChoice(const SOT_UI *ui);

// ---- Menu ----
void SOT_UI_ShowMenu(SOT_UI *ui, const char *title);
void SOT_UI_AddMenuItem(SOT_UI *ui, const char *label, bool enabled);
void SOT_UI_HideMenu(SOT_UI *ui);
bool SOT_UI_IsMenuActive(const SOT_UI *ui);
int  SOT_UI_GetMenuSelection(const SOT_UI *ui);

// ---- Input handling (call from input system) ----
void SOT_UI_HandleInput(SOT_UI *ui, bool up, bool down, bool confirm, bool cancel);

// ---- Text measurement ----
int SOT_UI_MeasureTextWidth(const SOT_UI *ui, const char *text);

#endif
