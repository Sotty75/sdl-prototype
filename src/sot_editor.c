/*
 * SOT Editor — Dear ImGui-based standalone editor.
 *
 * The editor is the primary application window. It renders ImGui at native
 * resolution directly to the swapchain. The scene is rendered to the virtual
 * framebuffer and displayed in an ImGui Image panel (scene viewport).
 *
 * When "Play" is pressed, a second SDL window is created for the game.
 * The game loop runs in the game window; the editor continues to display
 * the scene preview and panels.
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "cimgui.h"
#include "cimgui_impl.h"
#include "sot_imgui_sdlgpu3.h"

#include "sot_editor.h"
#include "sot_scene.h"
#include "sot_actor.h"
#include "sot_animation.h"
#include "sot_gpu_pipeline.h"
#include "sot_display.h"
#include "sot_physics.h"
#include "sot_common.h"
#include "sot_lua.h"
#include "sot_texture.h"
#include "cJSON.h"

// Alias for cimgui naming
#define igGetIO igGetIO_Nil
#define igGetPlatformIO igGetPlatformIO_Nil

// ---- Sampler callback: switch to nearest for pixel-art framebuffer ----

static void SetSamplerNearest(const ImDrawList *parent_list, const ImDrawCmd *cmd)
{
    (void)parent_list; (void)cmd;
    ImGuiPlatformIO *pio = igGetPlatformIO();
    ImGui_ImplSDLGPU3_RenderState *rs = (ImGui_ImplSDLGPU3_RenderState *)pio->Renderer_RenderState;
    if (rs) rs->SamplerCurrent = rs->SamplerNearest;
}

static void SetSamplerLinear(const ImDrawList *parent_list, const ImDrawCmd *cmd)
{
    (void)parent_list; (void)cmd;
    ImGuiPlatformIO *pio = igGetPlatformIO();
    ImGui_ImplSDLGPU3_RenderState *rs = (ImGui_ImplSDLGPU3_RenderState *)pio->Renderer_RenderState;
    if (rs) rs->SamplerCurrent = rs->SamplerLinear;
}

// Forward declarations for prefs
static void LoadEditorPrefs(SOT_Editor *editor);
static void SaveEditorPrefs(SOT_Editor *editor);

// ---- Theme system ----

static const char *SOT_ThemeNames[SOT_THEME_COUNT] = {
    "Dark",
    "Light",
    "Modern",
    "PICO-8",
    "C64",
    "ZX Spectrum",
    "NES",
    "Amiga",
};

// PICO-8 palette (16 colors)
#define PICO8_BLACK       (ImVec4_c){0.000f, 0.000f, 0.000f, 1.0f}
#define PICO8_DARK_BLUE   (ImVec4_c){0.114f, 0.169f, 0.325f, 1.0f}
#define PICO8_DARK_PURPLE (ImVec4_c){0.494f, 0.145f, 0.325f, 1.0f}
#define PICO8_DARK_GREEN  (ImVec4_c){0.000f, 0.529f, 0.318f, 1.0f}
#define PICO8_BROWN       (ImVec4_c){0.671f, 0.322f, 0.212f, 1.0f}
#define PICO8_DARK_GREY   (ImVec4_c){0.373f, 0.341f, 0.310f, 1.0f}
#define PICO8_LIGHT_GREY  (ImVec4_c){0.761f, 0.765f, 0.780f, 1.0f}
#define PICO8_WHITE       (ImVec4_c){1.000f, 0.945f, 0.910f, 1.0f}
#define PICO8_RED         (ImVec4_c){1.000f, 0.000f, 0.302f, 1.0f}
#define PICO8_ORANGE      (ImVec4_c){1.000f, 0.639f, 0.000f, 1.0f}
#define PICO8_YELLOW      (ImVec4_c){1.000f, 0.925f, 0.153f, 1.0f}
#define PICO8_GREEN       (ImVec4_c){0.000f, 0.894f, 0.212f, 1.0f}
#define PICO8_BLUE        (ImVec4_c){0.161f, 0.678f, 1.000f, 1.0f}
#define PICO8_LAVENDER    (ImVec4_c){0.514f, 0.463f, 0.612f, 1.0f}
#define PICO8_PINK        (ImVec4_c){1.000f, 0.467f, 0.659f, 1.0f}
#define PICO8_PEACH       (ImVec4_c){1.000f, 0.800f, 0.667f, 1.0f}

static inline ImVec4_c pico8_alpha(ImVec4_c c, float a) {
    c.w = a;
    return c;
}

static void SOT_ApplyRetroStyleVars(void)
{
    ImGuiStyle *style = igGetStyle();

    // Subtle rounding — soft but not bubbly
    style->WindowRounding    = 3.0f;
    style->ChildRounding     = 2.0f;
    style->FrameRounding     = 2.0f;
    style->PopupRounding     = 3.0f;
    style->ScrollbarRounding = 2.0f;
    style->GrabRounding      = 2.0f;
    style->TabRounding       = 2.0f;

    // Borders: keep window/tab, drop frame borders for cleaner widgets
    style->WindowBorderSize  = 1.0f;
    style->FrameBorderSize   = 0.0f;
    style->TabBorderSize     = 1.0f;

    // Comfortable spacing — breathing room without waste
    style->WindowPadding     = (ImVec2_c){8, 8};
    style->FramePadding      = (ImVec2_c){6, 4};
    style->ItemSpacing       = (ImVec2_c){8, 4};
    style->ItemInnerSpacing  = (ImVec2_c){6, 4};
    style->IndentSpacing     = 16.0f;
    style->ScrollbarSize     = 14.0f;
    style->GrabMinSize       = 10.0f;
}

static void SOT_ApplyThemePICO8(void)
{
    igStyleColorsDark(NULL);
    SOT_ApplyRetroStyleVars();

    ImGuiStyle *style = igGetStyle();
    ImVec4_c *c = style->Colors;

    // Text
    c[ImGuiCol_Text]                  = PICO8_WHITE;
    c[ImGuiCol_TextDisabled]          = PICO8_LAVENDER;
    c[ImGuiCol_TextLink]              = PICO8_BLUE;

    // Backgrounds
    c[ImGuiCol_WindowBg]              = pico8_alpha(PICO8_DARK_BLUE, 0.95f);
    c[ImGuiCol_ChildBg]               = pico8_alpha(PICO8_DARK_BLUE, 0.00f);
    c[ImGuiCol_PopupBg]               = pico8_alpha(PICO8_BLACK, 0.95f);
    c[ImGuiCol_MenuBarBg]             = PICO8_BLACK;

    // Borders
    c[ImGuiCol_Border]                = pico8_alpha(PICO8_LAVENDER, 0.50f);
    c[ImGuiCol_BorderShadow]          = pico8_alpha(PICO8_BLACK, 0.00f);

    // Frame (input fields, checkboxes)
    c[ImGuiCol_FrameBg]               = pico8_alpha(PICO8_BLACK, 0.70f);
    c[ImGuiCol_FrameBgHovered]        = pico8_alpha(PICO8_DARK_GREY, 0.70f);
    c[ImGuiCol_FrameBgActive]         = pico8_alpha(PICO8_LAVENDER, 0.50f);

    // Title bar
    c[ImGuiCol_TitleBg]               = PICO8_BLACK;
    c[ImGuiCol_TitleBgActive]         = pico8_alpha(PICO8_DARK_PURPLE, 0.80f);
    c[ImGuiCol_TitleBgCollapsed]      = pico8_alpha(PICO8_BLACK, 0.50f);

    // Scrollbar
    c[ImGuiCol_ScrollbarBg]           = pico8_alpha(PICO8_BLACK, 0.50f);
    c[ImGuiCol_ScrollbarGrab]         = PICO8_DARK_GREY;
    c[ImGuiCol_ScrollbarGrabHovered]  = PICO8_LAVENDER;
    c[ImGuiCol_ScrollbarGrabActive]   = PICO8_LIGHT_GREY;

    // Interactive
    c[ImGuiCol_CheckMark]             = PICO8_GREEN;
    c[ImGuiCol_SliderGrab]            = PICO8_BLUE;
    c[ImGuiCol_SliderGrabActive]      = PICO8_PEACH;

    // Buttons
    c[ImGuiCol_Button]                = pico8_alpha(PICO8_DARK_GREY, 0.65f);
    c[ImGuiCol_ButtonHovered]         = PICO8_LAVENDER;
    c[ImGuiCol_ButtonActive]          = PICO8_BLUE;

    // Headers (collapsing headers, selectable, menu items)
    c[ImGuiCol_Header]                = pico8_alpha(PICO8_DARK_PURPLE, 0.50f);
    c[ImGuiCol_HeaderHovered]         = pico8_alpha(PICO8_DARK_PURPLE, 0.80f);
    c[ImGuiCol_HeaderActive]          = PICO8_DARK_PURPLE;

    // Separators
    c[ImGuiCol_Separator]             = pico8_alpha(PICO8_LAVENDER, 0.40f);
    c[ImGuiCol_SeparatorHovered]      = PICO8_BLUE;
    c[ImGuiCol_SeparatorActive]       = PICO8_PEACH;

    // Resize grip
    c[ImGuiCol_ResizeGrip]            = pico8_alpha(PICO8_LAVENDER, 0.20f);
    c[ImGuiCol_ResizeGripHovered]     = pico8_alpha(PICO8_BLUE, 0.70f);
    c[ImGuiCol_ResizeGripActive]      = PICO8_BLUE;

    // Tabs
    c[ImGuiCol_Tab]                   = pico8_alpha(PICO8_DARK_BLUE, 0.80f);
    c[ImGuiCol_TabHovered]            = pico8_alpha(PICO8_DARK_PURPLE, 0.80f);
    c[ImGuiCol_TabSelected]           = pico8_alpha(PICO8_DARK_PURPLE, 1.00f);
    c[ImGuiCol_TabSelectedOverline]   = PICO8_BLUE;
    c[ImGuiCol_TabDimmed]             = pico8_alpha(PICO8_BLACK, 0.80f);
    c[ImGuiCol_TabDimmedSelected]     = pico8_alpha(PICO8_DARK_BLUE, 0.80f);

    // Docking
    c[ImGuiCol_DockingPreview]        = pico8_alpha(PICO8_BLUE, 0.70f);
    c[ImGuiCol_DockingEmptyBg]        = PICO8_BLACK;

    // Tables
    c[ImGuiCol_TableHeaderBg]         = pico8_alpha(PICO8_BLACK, 0.80f);
    c[ImGuiCol_TableBorderStrong]     = pico8_alpha(PICO8_LAVENDER, 0.50f);
    c[ImGuiCol_TableBorderLight]      = pico8_alpha(PICO8_LAVENDER, 0.25f);
    c[ImGuiCol_TableRowBg]            = pico8_alpha(PICO8_BLACK, 0.00f);
    c[ImGuiCol_TableRowBgAlt]         = pico8_alpha(PICO8_WHITE, 0.04f);

    // Drag/drop
    c[ImGuiCol_DragDropTarget]        = PICO8_YELLOW;
}

// ---- Commodore 64 theme ----

#define C64_BLACK       (ImVec4_c){0.000f, 0.000f, 0.000f, 1.0f}
#define C64_WHITE       (ImVec4_c){1.000f, 1.000f, 1.000f, 1.0f}
#define C64_RED         (ImVec4_c){0.533f, 0.188f, 0.180f, 1.0f}
#define C64_CYAN        (ImVec4_c){0.416f, 0.784f, 0.788f, 1.0f}
#define C64_PURPLE      (ImVec4_c){0.545f, 0.227f, 0.631f, 1.0f}
#define C64_GREEN       (ImVec4_c){0.286f, 0.624f, 0.243f, 1.0f}
#define C64_BLUE        (ImVec4_c){0.165f, 0.118f, 0.502f, 1.0f}
#define C64_YELLOW      (ImVec4_c){0.808f, 0.835f, 0.373f, 1.0f}
#define C64_ORANGE      (ImVec4_c){0.545f, 0.341f, 0.094f, 1.0f}
#define C64_BROWN       (ImVec4_c){0.353f, 0.235f, 0.000f, 1.0f}
#define C64_LIGHT_RED   (ImVec4_c){0.745f, 0.439f, 0.431f, 1.0f}
#define C64_DARK_GREY   (ImVec4_c){0.247f, 0.247f, 0.247f, 1.0f}
#define C64_GREY        (ImVec4_c){0.412f, 0.412f, 0.412f, 1.0f}
#define C64_LIGHT_GREEN (ImVec4_c){0.537f, 0.875f, 0.494f, 1.0f}
#define C64_LIGHT_BLUE  (ImVec4_c){0.416f, 0.369f, 0.753f, 1.0f}
#define C64_LIGHT_GREY  (ImVec4_c){0.600f, 0.600f, 0.600f, 1.0f}

static void SOT_ApplyThemeC64(void)
{
    igStyleColorsDark(NULL);
    SOT_ApplyRetroStyleVars();

    ImGuiStyle *style = igGetStyle();
    ImVec4_c *c = style->Colors;

    c[ImGuiCol_Text]                  = C64_LIGHT_BLUE;
    c[ImGuiCol_TextDisabled]          = C64_GREY;
    c[ImGuiCol_TextLink]              = C64_CYAN;
    c[ImGuiCol_WindowBg]              = pico8_alpha(C64_BLUE, 0.95f);
    c[ImGuiCol_ChildBg]               = pico8_alpha(C64_BLUE, 0.00f);
    c[ImGuiCol_PopupBg]               = pico8_alpha(C64_BLACK, 0.95f);
    c[ImGuiCol_MenuBarBg]             = C64_BLUE;
    c[ImGuiCol_Border]                = pico8_alpha(C64_LIGHT_BLUE, 0.50f);
    c[ImGuiCol_BorderShadow]          = pico8_alpha(C64_BLACK, 0.00f);
    c[ImGuiCol_FrameBg]               = pico8_alpha(C64_BLACK, 0.70f);
    c[ImGuiCol_FrameBgHovered]        = pico8_alpha(C64_DARK_GREY, 0.70f);
    c[ImGuiCol_FrameBgActive]         = pico8_alpha(C64_PURPLE, 0.50f);
    c[ImGuiCol_TitleBg]               = C64_BLACK;
    c[ImGuiCol_TitleBgActive]         = C64_BLUE;
    c[ImGuiCol_TitleBgCollapsed]      = pico8_alpha(C64_BLACK, 0.50f);
    c[ImGuiCol_ScrollbarBg]           = pico8_alpha(C64_BLACK, 0.50f);
    c[ImGuiCol_ScrollbarGrab]         = C64_GREY;
    c[ImGuiCol_ScrollbarGrabHovered]  = C64_LIGHT_BLUE;
    c[ImGuiCol_ScrollbarGrabActive]   = C64_CYAN;
    c[ImGuiCol_CheckMark]             = C64_LIGHT_GREEN;
    c[ImGuiCol_SliderGrab]            = C64_LIGHT_BLUE;
    c[ImGuiCol_SliderGrabActive]      = C64_CYAN;
    c[ImGuiCol_Button]                = pico8_alpha(C64_DARK_GREY, 0.65f);
    c[ImGuiCol_ButtonHovered]         = C64_LIGHT_BLUE;
    c[ImGuiCol_ButtonActive]          = C64_CYAN;
    c[ImGuiCol_Header]                = pico8_alpha(C64_BLUE, 0.70f);
    c[ImGuiCol_HeaderHovered]         = pico8_alpha(C64_LIGHT_BLUE, 0.50f);
    c[ImGuiCol_HeaderActive]          = C64_LIGHT_BLUE;
    c[ImGuiCol_Separator]             = pico8_alpha(C64_LIGHT_BLUE, 0.40f);
    c[ImGuiCol_SeparatorHovered]      = C64_CYAN;
    c[ImGuiCol_SeparatorActive]       = C64_LIGHT_GREEN;
    c[ImGuiCol_ResizeGrip]            = pico8_alpha(C64_LIGHT_BLUE, 0.20f);
    c[ImGuiCol_ResizeGripHovered]     = pico8_alpha(C64_CYAN, 0.70f);
    c[ImGuiCol_ResizeGripActive]      = C64_CYAN;
    c[ImGuiCol_Tab]                   = pico8_alpha(C64_BLUE, 0.80f);
    c[ImGuiCol_TabHovered]            = pico8_alpha(C64_LIGHT_BLUE, 0.60f);
    c[ImGuiCol_TabSelected]           = C64_BLUE;
    c[ImGuiCol_TabSelectedOverline]   = C64_CYAN;
    c[ImGuiCol_TabDimmed]             = pico8_alpha(C64_BLACK, 0.80f);
    c[ImGuiCol_TabDimmedSelected]     = pico8_alpha(C64_BLUE, 0.60f);
    c[ImGuiCol_DockingPreview]        = pico8_alpha(C64_CYAN, 0.70f);
    c[ImGuiCol_DockingEmptyBg]        = C64_BLACK;
    c[ImGuiCol_TableHeaderBg]         = pico8_alpha(C64_BLACK, 0.80f);
    c[ImGuiCol_TableBorderStrong]     = pico8_alpha(C64_LIGHT_BLUE, 0.50f);
    c[ImGuiCol_TableBorderLight]      = pico8_alpha(C64_LIGHT_BLUE, 0.25f);
    c[ImGuiCol_TableRowBg]            = pico8_alpha(C64_BLACK, 0.00f);
    c[ImGuiCol_TableRowBgAlt]         = pico8_alpha(C64_WHITE, 0.03f);
    c[ImGuiCol_DragDropTarget]        = C64_YELLOW;
}

// ---- ZX Spectrum theme ----

#define ZX_BLACK        (ImVec4_c){0.000f, 0.000f, 0.000f, 1.0f}
#define ZX_BLUE         (ImVec4_c){0.000f, 0.000f, 0.831f, 1.0f}
#define ZX_RED          (ImVec4_c){0.831f, 0.000f, 0.000f, 1.0f}
#define ZX_MAGENTA      (ImVec4_c){0.831f, 0.000f, 0.831f, 1.0f}
#define ZX_GREEN        (ImVec4_c){0.000f, 0.831f, 0.000f, 1.0f}
#define ZX_CYAN         (ImVec4_c){0.000f, 0.831f, 0.831f, 1.0f}
#define ZX_YELLOW       (ImVec4_c){0.831f, 0.831f, 0.000f, 1.0f}
#define ZX_WHITE        (ImVec4_c){0.831f, 0.831f, 0.831f, 1.0f}
#define ZX_BR_BLUE      (ImVec4_c){0.000f, 0.000f, 1.000f, 1.0f}
#define ZX_BR_RED       (ImVec4_c){1.000f, 0.000f, 0.000f, 1.0f}
#define ZX_BR_MAGENTA   (ImVec4_c){1.000f, 0.000f, 1.000f, 1.0f}
#define ZX_BR_GREEN     (ImVec4_c){0.000f, 1.000f, 0.000f, 1.0f}
#define ZX_BR_CYAN      (ImVec4_c){0.000f, 1.000f, 1.000f, 1.0f}
#define ZX_BR_YELLOW    (ImVec4_c){1.000f, 1.000f, 0.000f, 1.0f}
#define ZX_BR_WHITE     (ImVec4_c){1.000f, 1.000f, 1.000f, 1.0f}

static void SOT_ApplyThemeZXSpectrum(void)
{
    igStyleColorsDark(NULL);
    SOT_ApplyRetroStyleVars();

    ImGuiStyle *style = igGetStyle();
    ImVec4_c *c = style->Colors;

    c[ImGuiCol_Text]                  = ZX_BR_WHITE;
    c[ImGuiCol_TextDisabled]          = ZX_WHITE;
    c[ImGuiCol_TextLink]              = ZX_BR_CYAN;
    c[ImGuiCol_WindowBg]              = pico8_alpha(ZX_BLUE, 0.95f);
    c[ImGuiCol_ChildBg]               = pico8_alpha(ZX_BLUE, 0.00f);
    c[ImGuiCol_PopupBg]               = pico8_alpha(ZX_BLACK, 0.95f);
    c[ImGuiCol_MenuBarBg]             = ZX_BLACK;
    c[ImGuiCol_Border]                = pico8_alpha(ZX_WHITE, 0.50f);
    c[ImGuiCol_BorderShadow]          = pico8_alpha(ZX_BLACK, 0.00f);
    c[ImGuiCol_FrameBg]               = pico8_alpha(ZX_BLACK, 0.70f);
    c[ImGuiCol_FrameBgHovered]        = pico8_alpha(ZX_BLUE, 0.90f);
    c[ImGuiCol_FrameBgActive]         = pico8_alpha(ZX_BR_BLUE, 0.50f);
    c[ImGuiCol_TitleBg]               = ZX_BLACK;
    c[ImGuiCol_TitleBgActive]         = pico8_alpha(ZX_RED, 0.80f);
    c[ImGuiCol_TitleBgCollapsed]      = pico8_alpha(ZX_BLACK, 0.50f);
    c[ImGuiCol_ScrollbarBg]           = pico8_alpha(ZX_BLACK, 0.50f);
    c[ImGuiCol_ScrollbarGrab]         = ZX_WHITE;
    c[ImGuiCol_ScrollbarGrabHovered]  = ZX_BR_WHITE;
    c[ImGuiCol_ScrollbarGrabActive]   = ZX_BR_CYAN;
    c[ImGuiCol_CheckMark]             = ZX_BR_GREEN;
    c[ImGuiCol_SliderGrab]            = ZX_BR_CYAN;
    c[ImGuiCol_SliderGrabActive]      = ZX_BR_YELLOW;
    c[ImGuiCol_Button]                = pico8_alpha(ZX_RED, 0.65f);
    c[ImGuiCol_ButtonHovered]         = ZX_BR_RED;
    c[ImGuiCol_ButtonActive]          = ZX_BR_YELLOW;
    c[ImGuiCol_Header]                = pico8_alpha(ZX_RED, 0.50f);
    c[ImGuiCol_HeaderHovered]         = pico8_alpha(ZX_RED, 0.80f);
    c[ImGuiCol_HeaderActive]          = ZX_BR_RED;
    c[ImGuiCol_Separator]             = pico8_alpha(ZX_WHITE, 0.40f);
    c[ImGuiCol_SeparatorHovered]      = ZX_BR_CYAN;
    c[ImGuiCol_SeparatorActive]       = ZX_BR_YELLOW;
    c[ImGuiCol_ResizeGrip]            = pico8_alpha(ZX_WHITE, 0.20f);
    c[ImGuiCol_ResizeGripHovered]     = pico8_alpha(ZX_BR_CYAN, 0.70f);
    c[ImGuiCol_ResizeGripActive]      = ZX_BR_CYAN;
    c[ImGuiCol_Tab]                   = pico8_alpha(ZX_BLUE, 0.80f);
    c[ImGuiCol_TabHovered]            = pico8_alpha(ZX_BR_RED, 0.60f);
    c[ImGuiCol_TabSelected]           = pico8_alpha(ZX_RED, 1.00f);
    c[ImGuiCol_TabSelectedOverline]   = ZX_BR_YELLOW;
    c[ImGuiCol_TabDimmed]             = pico8_alpha(ZX_BLACK, 0.80f);
    c[ImGuiCol_TabDimmedSelected]     = pico8_alpha(ZX_BLUE, 0.60f);
    c[ImGuiCol_DockingPreview]        = pico8_alpha(ZX_BR_CYAN, 0.70f);
    c[ImGuiCol_DockingEmptyBg]        = ZX_BLACK;
    c[ImGuiCol_TableHeaderBg]         = pico8_alpha(ZX_BLACK, 0.80f);
    c[ImGuiCol_TableBorderStrong]     = pico8_alpha(ZX_WHITE, 0.50f);
    c[ImGuiCol_TableBorderLight]      = pico8_alpha(ZX_WHITE, 0.25f);
    c[ImGuiCol_TableRowBg]            = pico8_alpha(ZX_BLACK, 0.00f);
    c[ImGuiCol_TableRowBgAlt]         = pico8_alpha(ZX_BR_WHITE, 0.03f);
    c[ImGuiCol_DragDropTarget]        = ZX_BR_YELLOW;
}

// ---- NES theme ----

#define NES_BLACK       (ImVec4_c){0.000f, 0.000f, 0.000f, 1.0f}
#define NES_DARK_BLUE   (ImVec4_c){0.000f, 0.071f, 0.373f, 1.0f}
#define NES_BLUE        (ImVec4_c){0.118f, 0.118f, 0.733f, 1.0f}
#define NES_PURPLE      (ImVec4_c){0.267f, 0.000f, 0.600f, 1.0f}
#define NES_MAGENTA     (ImVec4_c){0.600f, 0.000f, 0.400f, 1.0f}
#define NES_RED         (ImVec4_c){0.733f, 0.067f, 0.067f, 1.0f}
#define NES_ORANGE      (ImVec4_c){0.667f, 0.200f, 0.000f, 1.0f}
#define NES_BROWN       (ImVec4_c){0.467f, 0.267f, 0.000f, 1.0f}
#define NES_GREEN       (ImVec4_c){0.000f, 0.533f, 0.000f, 1.0f}
#define NES_TEAL        (ImVec4_c){0.000f, 0.467f, 0.200f, 1.0f}
#define NES_CYAN        (ImVec4_c){0.000f, 0.400f, 0.533f, 1.0f}
#define NES_GREY        (ImVec4_c){0.467f, 0.467f, 0.467f, 1.0f}
#define NES_LIGHT_BLUE  (ImVec4_c){0.333f, 0.467f, 1.000f, 1.0f}
#define NES_LIGHT_GREEN (ImVec4_c){0.333f, 0.800f, 0.333f, 1.0f}
#define NES_PEACH       (ImVec4_c){1.000f, 0.733f, 0.533f, 1.0f}
#define NES_WHITE       (ImVec4_c){0.933f, 0.933f, 0.933f, 1.0f}

static void SOT_ApplyThemeNES(void)
{
    igStyleColorsDark(NULL);
    SOT_ApplyRetroStyleVars();

    ImGuiStyle *style = igGetStyle();
    ImVec4_c *c = style->Colors;

    c[ImGuiCol_Text]                  = NES_WHITE;
    c[ImGuiCol_TextDisabled]          = NES_GREY;
    c[ImGuiCol_TextLink]              = NES_LIGHT_BLUE;
    c[ImGuiCol_WindowBg]              = pico8_alpha(NES_BLACK, 0.95f);
    c[ImGuiCol_ChildBg]               = pico8_alpha(NES_BLACK, 0.00f);
    c[ImGuiCol_PopupBg]               = pico8_alpha(NES_DARK_BLUE, 0.95f);
    c[ImGuiCol_MenuBarBg]             = NES_DARK_BLUE;
    c[ImGuiCol_Border]                = pico8_alpha(NES_GREY, 0.50f);
    c[ImGuiCol_BorderShadow]          = pico8_alpha(NES_BLACK, 0.00f);
    c[ImGuiCol_FrameBg]               = pico8_alpha(NES_DARK_BLUE, 0.70f);
    c[ImGuiCol_FrameBgHovered]        = pico8_alpha(NES_BLUE, 0.50f);
    c[ImGuiCol_FrameBgActive]         = pico8_alpha(NES_BLUE, 0.70f);
    c[ImGuiCol_TitleBg]               = NES_DARK_BLUE;
    c[ImGuiCol_TitleBgActive]         = pico8_alpha(NES_RED, 0.80f);
    c[ImGuiCol_TitleBgCollapsed]      = pico8_alpha(NES_BLACK, 0.50f);
    c[ImGuiCol_ScrollbarBg]           = pico8_alpha(NES_BLACK, 0.50f);
    c[ImGuiCol_ScrollbarGrab]         = NES_GREY;
    c[ImGuiCol_ScrollbarGrabHovered]  = NES_PEACH;
    c[ImGuiCol_ScrollbarGrabActive]   = NES_WHITE;
    c[ImGuiCol_CheckMark]             = NES_LIGHT_GREEN;
    c[ImGuiCol_SliderGrab]            = NES_LIGHT_BLUE;
    c[ImGuiCol_SliderGrabActive]      = NES_PEACH;
    c[ImGuiCol_Button]                = pico8_alpha(NES_RED, 0.65f);
    c[ImGuiCol_ButtonHovered]         = pico8_alpha(NES_RED, 0.90f);
    c[ImGuiCol_ButtonActive]          = NES_ORANGE;
    c[ImGuiCol_Header]                = pico8_alpha(NES_BLUE, 0.50f);
    c[ImGuiCol_HeaderHovered]         = pico8_alpha(NES_BLUE, 0.80f);
    c[ImGuiCol_HeaderActive]          = NES_BLUE;
    c[ImGuiCol_Separator]             = pico8_alpha(NES_GREY, 0.40f);
    c[ImGuiCol_SeparatorHovered]      = NES_LIGHT_BLUE;
    c[ImGuiCol_SeparatorActive]       = NES_PEACH;
    c[ImGuiCol_ResizeGrip]            = pico8_alpha(NES_GREY, 0.20f);
    c[ImGuiCol_ResizeGripHovered]     = pico8_alpha(NES_LIGHT_BLUE, 0.70f);
    c[ImGuiCol_ResizeGripActive]      = NES_LIGHT_BLUE;
    c[ImGuiCol_Tab]                   = pico8_alpha(NES_DARK_BLUE, 0.80f);
    c[ImGuiCol_TabHovered]            = pico8_alpha(NES_RED, 0.60f);
    c[ImGuiCol_TabSelected]           = pico8_alpha(NES_RED, 0.80f);
    c[ImGuiCol_TabSelectedOverline]   = NES_PEACH;
    c[ImGuiCol_TabDimmed]             = pico8_alpha(NES_BLACK, 0.80f);
    c[ImGuiCol_TabDimmedSelected]     = pico8_alpha(NES_DARK_BLUE, 0.60f);
    c[ImGuiCol_DockingPreview]        = pico8_alpha(NES_LIGHT_BLUE, 0.70f);
    c[ImGuiCol_DockingEmptyBg]        = NES_BLACK;
    c[ImGuiCol_TableHeaderBg]         = pico8_alpha(NES_DARK_BLUE, 0.80f);
    c[ImGuiCol_TableBorderStrong]     = pico8_alpha(NES_GREY, 0.50f);
    c[ImGuiCol_TableBorderLight]      = pico8_alpha(NES_GREY, 0.25f);
    c[ImGuiCol_TableRowBg]            = pico8_alpha(NES_BLACK, 0.00f);
    c[ImGuiCol_TableRowBgAlt]         = pico8_alpha(NES_WHITE, 0.03f);
    c[ImGuiCol_DragDropTarget]        = NES_PEACH;
}

// ---- Amiga Workbench 2.0 theme ----

#define AMIGA_BLACK     (ImVec4_c){0.000f, 0.000f, 0.000f, 1.0f}
#define AMIGA_WHITE     (ImVec4_c){1.000f, 1.000f, 1.000f, 1.0f}
#define AMIGA_GREY      (ImVec4_c){0.667f, 0.667f, 0.667f, 1.0f}
#define AMIGA_BLUE      (ImVec4_c){0.337f, 0.514f, 0.765f, 1.0f}
#define AMIGA_DARK_GREY (ImVec4_c){0.400f, 0.400f, 0.400f, 1.0f}
#define AMIGA_LIGHT_BLUE (ImVec4_c){0.600f, 0.725f, 0.890f, 1.0f}
#define AMIGA_ORANGE    (ImVec4_c){1.000f, 0.565f, 0.000f, 1.0f}
#define AMIGA_DARK_BLUE (ImVec4_c){0.180f, 0.310f, 0.530f, 1.0f}

static void SOT_ApplyThemeAmiga(void)
{
    igStyleColorsDark(NULL);
    SOT_ApplyRetroStyleVars();

    ImGuiStyle *style = igGetStyle();
    ImVec4_c *c = style->Colors;

    c[ImGuiCol_Text]                  = AMIGA_BLACK;
    c[ImGuiCol_TextDisabled]          = AMIGA_DARK_GREY;
    c[ImGuiCol_TextLink]              = AMIGA_DARK_BLUE;
    c[ImGuiCol_WindowBg]              = pico8_alpha(AMIGA_GREY, 0.95f);
    c[ImGuiCol_ChildBg]               = pico8_alpha(AMIGA_GREY, 0.00f);
    c[ImGuiCol_PopupBg]               = pico8_alpha(AMIGA_GREY, 0.98f);
    c[ImGuiCol_MenuBarBg]             = AMIGA_GREY;
    c[ImGuiCol_Border]                = pico8_alpha(AMIGA_BLACK, 0.50f);
    c[ImGuiCol_BorderShadow]          = pico8_alpha(AMIGA_WHITE, 0.30f);
    c[ImGuiCol_FrameBg]               = pico8_alpha(AMIGA_WHITE, 0.80f);
    c[ImGuiCol_FrameBgHovered]        = pico8_alpha(AMIGA_LIGHT_BLUE, 0.70f);
    c[ImGuiCol_FrameBgActive]         = pico8_alpha(AMIGA_BLUE, 0.50f);
    c[ImGuiCol_TitleBg]               = AMIGA_DARK_GREY;
    c[ImGuiCol_TitleBgActive]         = AMIGA_BLUE;
    c[ImGuiCol_TitleBgCollapsed]      = pico8_alpha(AMIGA_GREY, 0.50f);
    c[ImGuiCol_ScrollbarBg]           = pico8_alpha(AMIGA_GREY, 0.80f);
    c[ImGuiCol_ScrollbarGrab]         = AMIGA_BLUE;
    c[ImGuiCol_ScrollbarGrabHovered]  = AMIGA_LIGHT_BLUE;
    c[ImGuiCol_ScrollbarGrabActive]   = AMIGA_DARK_BLUE;
    c[ImGuiCol_CheckMark]             = AMIGA_BLUE;
    c[ImGuiCol_SliderGrab]            = AMIGA_BLUE;
    c[ImGuiCol_SliderGrabActive]      = AMIGA_DARK_BLUE;
    c[ImGuiCol_Button]                = pico8_alpha(AMIGA_BLUE, 0.65f);
    c[ImGuiCol_ButtonHovered]         = AMIGA_LIGHT_BLUE;
    c[ImGuiCol_ButtonActive]          = AMIGA_DARK_BLUE;
    c[ImGuiCol_Header]                = pico8_alpha(AMIGA_BLUE, 0.50f);
    c[ImGuiCol_HeaderHovered]         = pico8_alpha(AMIGA_BLUE, 0.80f);
    c[ImGuiCol_HeaderActive]          = AMIGA_BLUE;
    c[ImGuiCol_Separator]             = pico8_alpha(AMIGA_BLACK, 0.30f);
    c[ImGuiCol_SeparatorHovered]      = AMIGA_BLUE;
    c[ImGuiCol_SeparatorActive]       = AMIGA_DARK_BLUE;
    c[ImGuiCol_ResizeGrip]            = pico8_alpha(AMIGA_BLUE, 0.20f);
    c[ImGuiCol_ResizeGripHovered]     = pico8_alpha(AMIGA_BLUE, 0.70f);
    c[ImGuiCol_ResizeGripActive]      = AMIGA_BLUE;
    c[ImGuiCol_Tab]                   = pico8_alpha(AMIGA_GREY, 0.90f);
    c[ImGuiCol_TabHovered]            = pico8_alpha(AMIGA_BLUE, 0.60f);
    c[ImGuiCol_TabSelected]           = AMIGA_WHITE;
    c[ImGuiCol_TabSelectedOverline]   = AMIGA_BLUE;
    c[ImGuiCol_TabDimmed]             = pico8_alpha(AMIGA_DARK_GREY, 0.80f);
    c[ImGuiCol_TabDimmedSelected]     = pico8_alpha(AMIGA_GREY, 0.80f);
    c[ImGuiCol_DockingPreview]        = pico8_alpha(AMIGA_BLUE, 0.70f);
    c[ImGuiCol_DockingEmptyBg]        = AMIGA_GREY;
    c[ImGuiCol_TableHeaderBg]         = pico8_alpha(AMIGA_BLUE, 0.30f);
    c[ImGuiCol_TableBorderStrong]     = pico8_alpha(AMIGA_BLACK, 0.40f);
    c[ImGuiCol_TableBorderLight]      = pico8_alpha(AMIGA_BLACK, 0.20f);
    c[ImGuiCol_TableRowBg]            = pico8_alpha(AMIGA_BLACK, 0.00f);
    c[ImGuiCol_TableRowBgAlt]         = pico8_alpha(AMIGA_BLACK, 0.04f);
    c[ImGuiCol_DragDropTarget]        = AMIGA_ORANGE;
}

static void SOT_ApplyModernStyleVars(void)
{
    ImGuiStyle *style = igGetStyle();

    style->WindowRounding    = 5.0f;
    style->ChildRounding     = 4.0f;
    style->FrameRounding     = 4.0f;
    style->PopupRounding     = 5.0f;
    style->ScrollbarRounding = 4.0f;
    style->GrabRounding      = 3.0f;
    style->TabRounding       = 4.0f;

    style->WindowBorderSize  = 1.0f;
    style->FrameBorderSize   = 0.0f;
    style->TabBorderSize     = 0.0f;

    style->WindowPadding     = (ImVec2_c){10, 8};
    style->FramePadding      = (ImVec2_c){8, 5};
    style->ItemSpacing       = (ImVec2_c){8, 6};
    style->ItemInnerSpacing  = (ImVec2_c){6, 4};
    style->IndentSpacing     = 18.0f;
    style->ScrollbarSize     = 14.0f;
    style->GrabMinSize       = 10.0f;

    style->SeparatorTextBorderSize = 2.0f;
}

static void SOT_ApplyThemeModern(void)
{
    igStyleColorsDark(NULL);
    SOT_ApplyModernStyleVars();

    ImGuiStyle *style = igGetStyle();
    ImVec4_c *c = style->Colors;

    // Base palette
    #define MOD_BG_DARK      (ImVec4_c){0.110f, 0.118f, 0.133f, 1.0f}   // #1C1E22
    #define MOD_BG           (ImVec4_c){0.145f, 0.155f, 0.173f, 1.0f}   // #252730
    #define MOD_BG_LIGHT     (ImVec4_c){0.180f, 0.192f, 0.212f, 1.0f}   // #2E3136
    #define MOD_SURFACE      (ImVec4_c){0.220f, 0.235f, 0.259f, 1.0f}   // #383C42
    #define MOD_BORDER       (ImVec4_c){0.255f, 0.275f, 0.302f, 1.0f}   // #41464D
    #define MOD_TEXT         (ImVec4_c){0.882f, 0.894f, 0.914f, 1.0f}   // #E1E4E9
    #define MOD_TEXT_DIM     (ImVec4_c){0.545f, 0.573f, 0.620f, 1.0f}   // #8B929E
    #define MOD_ACCENT       (ImVec4_c){0.161f, 0.475f, 1.000f, 1.0f}   // #2979FF
    #define MOD_ACCENT_HOV   (ImVec4_c){0.267f, 0.541f, 1.000f, 1.0f}   // #448AFF
    #define MOD_ACCENT_DIM   (ImVec4_c){0.161f, 0.475f, 1.000f, 0.45f}
    #define MOD_ERR          (ImVec4_c){0.902f, 0.298f, 0.235f, 1.0f}   // #E64C3C
    #define MOD_WARN         (ImVec4_c){0.945f, 0.769f, 0.059f, 1.0f}   // #F1C40F

    // Text
    c[ImGuiCol_Text]                  = MOD_TEXT;
    c[ImGuiCol_TextDisabled]          = MOD_TEXT_DIM;
    c[ImGuiCol_TextLink]              = MOD_ACCENT_HOV;

    // Backgrounds
    c[ImGuiCol_WindowBg]              = MOD_BG;
    c[ImGuiCol_ChildBg]               = (ImVec4_c){0, 0, 0, 0};
    c[ImGuiCol_PopupBg]               = (ImVec4_c){MOD_BG_DARK.x, MOD_BG_DARK.y, MOD_BG_DARK.z, 0.96f};

    // Borders
    c[ImGuiCol_Border]                = MOD_BORDER;
    c[ImGuiCol_BorderShadow]          = (ImVec4_c){0, 0, 0, 0};

    // Frame (inputs, checkboxes, sliders)
    c[ImGuiCol_FrameBg]               = MOD_BG_DARK;
    c[ImGuiCol_FrameBgHovered]        = MOD_BG_LIGHT;
    c[ImGuiCol_FrameBgActive]         = MOD_SURFACE;

    // Title bar
    c[ImGuiCol_TitleBg]               = MOD_BG_DARK;
    c[ImGuiCol_TitleBgActive]         = MOD_BG_DARK;
    c[ImGuiCol_TitleBgCollapsed]      = MOD_BG_DARK;

    // Menu bar
    c[ImGuiCol_MenuBarBg]             = MOD_BG_DARK;

    // Scrollbar
    c[ImGuiCol_ScrollbarBg]           = MOD_BG_DARK;
    c[ImGuiCol_ScrollbarGrab]         = MOD_SURFACE;
    c[ImGuiCol_ScrollbarGrabHovered]  = MOD_BORDER;
    c[ImGuiCol_ScrollbarGrabActive]   = MOD_TEXT_DIM;

    // Buttons
    c[ImGuiCol_Button]                = MOD_SURFACE;
    c[ImGuiCol_ButtonHovered]         = MOD_ACCENT;
    c[ImGuiCol_ButtonActive]          = MOD_ACCENT_HOV;

    // Headers (collapsing headers, selectable, menu items)
    c[ImGuiCol_Header]                = MOD_BG_LIGHT;
    c[ImGuiCol_HeaderHovered]         = MOD_ACCENT_DIM;
    c[ImGuiCol_HeaderActive]          = MOD_ACCENT;

    // Checkmark, slider grab
    c[ImGuiCol_CheckMark]             = MOD_ACCENT_HOV;
    c[ImGuiCol_SliderGrab]            = MOD_ACCENT;
    c[ImGuiCol_SliderGrabActive]      = MOD_ACCENT_HOV;

    // Separator
    c[ImGuiCol_Separator]             = MOD_BORDER;
    c[ImGuiCol_SeparatorHovered]      = MOD_ACCENT;
    c[ImGuiCol_SeparatorActive]       = MOD_ACCENT_HOV;

    // Resize grip
    c[ImGuiCol_ResizeGrip]            = MOD_ACCENT_DIM;
    c[ImGuiCol_ResizeGripHovered]     = MOD_ACCENT;
    c[ImGuiCol_ResizeGripActive]      = MOD_ACCENT_HOV;

    // Tabs
    c[ImGuiCol_Tab]                   = MOD_BG_DARK;
    c[ImGuiCol_TabHovered]            = MOD_ACCENT_DIM;
    c[ImGuiCol_TabSelected]           = MOD_ACCENT;
    c[ImGuiCol_TabSelectedOverline]   = MOD_ACCENT;
    c[ImGuiCol_TabDimmed]             = MOD_BG_DARK;
    c[ImGuiCol_TabDimmedSelected]     = MOD_BG_LIGHT;

    // Docking
    c[ImGuiCol_DockingPreview]        = MOD_ACCENT_DIM;
    c[ImGuiCol_DockingEmptyBg]        = MOD_BG_DARK;

    // Table
    c[ImGuiCol_TableHeaderBg]         = MOD_BG_DARK;
    c[ImGuiCol_TableBorderStrong]     = MOD_BORDER;
    c[ImGuiCol_TableBorderLight]      = (ImVec4_c){MOD_BORDER.x, MOD_BORDER.y, MOD_BORDER.z, 0.5f};
    c[ImGuiCol_TableRowBg]            = (ImVec4_c){0, 0, 0, 0};
    c[ImGuiCol_TableRowBgAlt]         = (ImVec4_c){1, 1, 1, 0.02f};

    // Misc
    c[ImGuiCol_DragDropTarget]        = MOD_ACCENT_HOV;
    c[ImGuiCol_NavCursor]             = MOD_ACCENT;
    c[ImGuiCol_NavWindowingHighlight] = (ImVec4_c){MOD_ACCENT.x, MOD_ACCENT.y, MOD_ACCENT.z, 0.70f};
    c[ImGuiCol_NavWindowingDimBg]     = (ImVec4_c){0.2f, 0.2f, 0.2f, 0.20f};
    c[ImGuiCol_ModalWindowDimBg]      = (ImVec4_c){0.1f, 0.1f, 0.1f, 0.60f};

    #undef MOD_BG_DARK
    #undef MOD_BG
    #undef MOD_BG_LIGHT
    #undef MOD_SURFACE
    #undef MOD_BORDER
    #undef MOD_TEXT
    #undef MOD_TEXT_DIM
    #undef MOD_ACCENT
    #undef MOD_ACCENT_HOV
    #undef MOD_ACCENT_DIM
    #undef MOD_ERR
    #undef MOD_WARN
}

static void SOT_ApplyTheme(SOT_ThemeID theme)
{
    switch (theme) {
    case SOT_THEME_LIGHT:
        igStyleColorsLight(NULL);
        break;
    case SOT_THEME_MODERN:
        SOT_ApplyThemeModern();
        break;
    case SOT_THEME_PICO8:
        SOT_ApplyThemePICO8();
        break;
    case SOT_THEME_C64:
        SOT_ApplyThemeC64();
        break;
    case SOT_THEME_ZXSPECTRUM:
        SOT_ApplyThemeZXSpectrum();
        break;
    case SOT_THEME_NES:
        SOT_ApplyThemeNES();
        break;
    case SOT_THEME_AMIGA:
        SOT_ApplyThemeAmiga();
        break;
    case SOT_THEME_DARK:
    default:
        igStyleColorsDark(NULL);
        break;
    }
}

// Forward declaration for animator editor
static void SOT_AnimatorEditor_Close(SOT_AnimatorEditor *animator, SDL_GPUDevice *device);

// ---- Lifecycle ----

bool SOT_Editor_Init(SOT_Editor *editor, SOT_GPU_State *gpu)
{
    SDL_memset(editor, 0, sizeof(SOT_Editor));
    editor->selectedActor = -1;
    editor->playing = false;
    editor->gameWindow = NULL;

    // Create ImGui context
    igCreateContext(NULL);

    ImGuiIO *io = igGetIO();
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Theme applied after prefs load below

    // Platform backend: SDL3
    ImGui_ImplSDL3_InitForSDLGPU(gpu->window);

    // Renderer backend: SDL GPU3
    ImGui_ImplSDLGPU3_InitInfo init_info;
    SDL_memset(&init_info, 0, sizeof(init_info));
    init_info.Device = gpu->device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu->device, gpu->window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;

    if (!ImGui_ImplSDLGPU3_Init(&init_info)) {
        SDL_Log("Editor: ImGui_ImplSDLGPU3_Init failed");
        return false;
    }

    // Default visibility and preferences
    editor->showTilemap = true;
    editor->showDebugOverlay = false;
    editor->showGrid = false;
    editor->showGizmos = false;
    editor->showAssetBrowser = false;
    editor->showAnimPreview = false;
    editor->showPreferences = false;
    editor->showAnimatorEditor = false;

    // Actor template editing
    editor->editingTemplate = NULL;
    editor->templateDirty = false;
    editor->lastInspectedActor = -1;

    // Asset browser defaults
    SDL_strlcpy(editor->abSelectedFolder, "", sizeof(editor->abSelectedFolder));
    editor->abSplitRatio = 0.30f;
    editor->abIconSize = 64.0f;
    editor->abFilter[0] = '\0';
    editor->abNeedsRefresh = true;
    editor->abThumbnailCount = 0;
    SDL_memset(&editor->animator, 0, sizeof(SOT_AnimatorEditor));
    editor->animator.selectedSequence = -1;
    editor->animator.selectedFrame = -1;
    editor->animator.zoom = 1.0f;
    editor->editorCam.posX = 0;
    editor->editorCam.posY = 0;
    editor->editorCam.zoom = 0.5f;  // zoomed out to show more of the scene
    editor->editorCam.initialized = false;
    editor->dragActor = -1;
    editor->isDragging = false;
    editor->gizmoDragAxis = 0;
    editor->gizmoDragging = false;
    editor->snapToGrid = false;
    editor->pendingSceneLoad[0] = '\0';
    editor->luaInputBuf[0] = '\0';

    // Default preferences
    editor->prefs.theme = SOT_THEME_MODERN;
    editor->prefs.fontSize = 18.0f;
    editor->prefs.gridColor[0] = 0.5f;
    editor->prefs.gridColor[1] = 0.7f;
    editor->prefs.gridColor[2] = 1.0f;
    editor->prefs.gridColor[3] = 0.40f;
    editor->prefs.gridSize = 16;
    editor->prefs.fontDirty = false;

    editor->initialized = true;

    // Load saved preferences
    LoadEditorPrefs(editor);

    SDL_Log("Editor: Initialized successfully");
    return true;
}

void SOT_Editor_Shutdown(SOT_Editor *editor, SOT_GPU_State *gpu)
{
    if (!editor->initialized) return;

    // Close game window if open
    if (editor->gameWindow) {
        SOT_Editor_StopGame(editor, gpu, NULL);
    }

    // Free editing template
    if (editor->editingTemplate) {
        SOT_FreeActorTemplate(editor->editingTemplate);
        editor->editingTemplate = NULL;
    }

    // Close animator editor if open
    if (editor->animator.active) {
        SOT_AnimatorEditor_Close(&editor->animator, gpu->device);
    }

    // Release asset browser thumbnail textures
    for (int i = 0; i < editor->abThumbnailCount; i++) {
        if (editor->abThumbnails[i].texture) {
            SDL_ReleaseGPUTexture(gpu->device, editor->abThumbnails[i].texture);
        }
    }
    editor->abThumbnailCount = 0;

    // Free snapshot memory if leaked
    if (editor->snapshot) {
        SDL_free(editor->snapshot);
        editor->snapshot = NULL;
    }

    // Save preferences before shutdown
    SaveEditorPrefs(editor);

    ImGui_ImplSDLGPU3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    igDestroyContext(NULL);

    editor->initialized = false;
}

// ---- Event handling ----

void SOT_Editor_ProcessEvent(SOT_Editor *editor, SDL_Event *event)
{
    if (!editor->initialized) return;

    // Always forward events to ImGui
    ImGui_ImplSDL3_ProcessEvent(event);
}

// ---- Per-frame ----

void SOT_Editor_BeginFrame(SOT_Editor *editor, SOT_GPU_State *gpu)
{
    if (!editor->initialized) return;

    // Handle font file rebuild (theme switch changes the font file)
    if (editor->prefs.fontDirty) {
        // Wait for all GPU work to complete
        SDL_WaitForGPUIdle(gpu->device);

        SDL_Log("Editor: Rebuilding font atlas (theme font change)");

        ImGuiIO *io = igGetIO();
        ImFontAtlas_Clear(io->Fonts);
        ImFontConfig *cfg = ImFontConfig_ImFontConfig();
        cfg->SizePixels = editor->prefs.fontSize;

        // Pick font file based on active theme
        char fontPath[512];
        if (editor->prefs.theme == SOT_THEME_MODERN) {
            cfg->OversampleH = 2;
            cfg->OversampleV = 1;
            cfg->PixelSnapH = false;
            SDL_snprintf(fontPath, sizeof(fontPath), "%sassets/fonts/Inter-Regular.ttf", Paths.Base);
        } else {
            cfg->OversampleH = 1;
            cfg->OversampleV = 1;
            cfg->PixelSnapH = true;
            SDL_snprintf(fontPath, sizeof(fontPath), "%sassets/fonts/PixelOperatorHB8.ttf", Paths.Base);
        }
        FILE *fp = fopen(fontPath, "r");
        if (fp) {
            fclose(fp);
            ImFontAtlas_AddFontFromFileTTF(io->Fonts, fontPath, editor->prefs.fontSize, cfg, NULL);
        } else {
            ImFontAtlas_AddFontDefault(io->Fonts, cfg);
        }

        editor->prefs.fontDirty = false;
    }

    // Always sync font size via style (ImGui 1.92+ dynamic font baking)
    {
        ImGuiStyle *style = igGetStyle();
        style->FontSizeBase = editor->prefs.fontSize;
    }

    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    igNewFrame();
}

// ---- Draw editor panels ----

static void DrawMenuBar(SOT_Editor *editor, SOT_GPU_State *gpu, SOT_Scene *scene)
{
    if (igBeginMainMenuBar()) {
        if (igBeginMenu("File", true)) {
            if (igMenuItem_Bool("Save Scene", "Ctrl+S", false, true)) {
                if (scene && scene->name[0]) {
                    if (SOT_SaveScene(scene, scene->name))
                        SOT_Editor_Log(editor, "Scene saved: %s", scene->name);
                    else
                        SOT_Editor_LogError(editor, "Failed to save scene");
                }
            }
            if (igBeginMenu("Open Scene", true)) {
                static char sceneFiles[32][128];
                static int sceneFileCount = -1;
                if (sceneFileCount < 0) {
                    sceneFileCount = 0;
                    char *scenesPath = Paths.Scenes;
                    if (scenesPath) {
                        int count = 0;
                        char **list = SDL_GlobDirectory(scenesPath, "*.json", 0, &count);
                        if (list) {
                            for (int i = 0; i < count && sceneFileCount < 32; i++) {
                                SDL_strlcpy(sceneFiles[sceneFileCount], list[i], 128);
                                sceneFileCount++;
                            }
                            SDL_free(list);
                        }
                    }
                }
                for (int i = 0; i < sceneFileCount; i++) {
                    if (igMenuItem_Bool(sceneFiles[i], NULL, false, true)) {
                        SDL_strlcpy(editor->pendingSceneLoad, sceneFiles[i], 128);
                        sceneFileCount = -1;  // rescan next time
                    }
                }
                if (sceneFileCount == 0) igText("(no scenes found)");
                igEndMenu();
            }
            igSeparator();
            if (igMenuItem_Bool("Quit", NULL, false, true)) {
                SDL_Event quit = { .type = SDL_EVENT_QUIT };
                SDL_PushEvent(&quit);
            }
            igEndMenu();
        }
        if (igBeginMenu("View", true)) {
            igMenuItem_BoolPtr("Grid", NULL, &editor->showGrid, true);
            igMenuItem_BoolPtr("Gizmos", NULL, &editor->showGizmos, true);
            igMenuItem_BoolPtr("Snap to Grid", NULL, &editor->snapToGrid, true);
            igSeparator();
            igMenuItem_BoolPtr("Asset Browser", NULL, &editor->showAssetBrowser, true);
            igMenuItem_BoolPtr("Animation Preview", NULL, &editor->showAnimPreview, true);
            if (editor->animator.active) {
                if (igMenuItem_Bool("Close Animator Editor", NULL, false, true)) {
                    if (!editor->animator.dirty) {
                        SOT_AnimatorEditor_Close(&editor->animator, gpu->device);
                        editor->showAnimatorEditor = false;
                    }
                }
            }
            igSeparator();
            if (igMenuItem_Bool("Preferences...", NULL, false, true))
                editor->showPreferences = true;
            igEndMenu();
        }

        igSeparator();

        // Play/Stop buttons
        if (!editor->playing) {
            if (igSmallButton("  >  Play  ")) {
                SOT_Editor_StartGame(editor, gpu, scene);
            }
        } else {
            if (igSmallButton("  []  Stop  ")) {
                SOT_Editor_StopGame(editor, gpu, scene);
            }
        }

        // FPS display
        ImGuiIO *io = igGetIO();
        igSameLine(0, 20);
        igText("FPS: %.1f", io->Framerate);

        if (editor->playing) {
            igSameLine(0, 20);
            igText("PLAYING");
        }

        igEndMainMenuBar();
    }
}

// ---- Coordinate mapping helpers ----

// Map world position to screen position within the viewport image.
// Uses the camera's projection-view matrix and the framebuffer internal resolution.
static void WorldToViewport(float worldX, float worldY, const sot_camera *cam,
                            int internalW, int internalH,
                            ImVec2_c imgTopLeft, float imgW, float imgH,
                            float *outX, float *outY)
{
    vec3 worldPos = { worldX, worldY, 0.0f };
    vec3 projected;
    vec4 viewport = { 0, 0, (float)internalW, (float)internalH };
    glm_project(worldPos, (vec4*)cam->pvMatrix, viewport, projected);

    *outX = imgTopLeft.x + (projected[0] / (float)internalW) * imgW;
    // Flip Y: framebuffer Y=0 is top, projected Y=0 is bottom
    *outY = imgTopLeft.y + (1.0f - projected[1] / (float)internalH) * imgH;
}

// Inverse: map screen position within viewport image to world position.
// Shoots a ray from the camera through the screen point and intersects with the z=0 plane.
static void ViewportToWorld(float screenX, float screenY, const sot_camera *cam,
                            int internalW, int internalH,
                            ImVec2_c imgTopLeft, float imgW, float imgH,
                            float *outWorldX, float *outWorldY)
{
    // Convert screen position to framebuffer pixel coords
    float fbX = ((screenX - imgTopLeft.x) / imgW) * (float)internalW;
    float fbY = (1.0f - (screenY - imgTopLeft.y) / imgH) * (float)internalH;

    vec4 viewport = { 0, 0, (float)internalW, (float)internalH };

    // Unproject at near (z=0) and far (z=1) to get a ray
    vec3 nearPt, farPt;
    vec3 winNear = { fbX, fbY, 0.0f };
    vec3 winFar  = { fbX, fbY, 1.0f };
    glm_unproject(winNear, (vec4*)cam->pvMatrix, viewport, nearPt);
    glm_unproject(winFar,  (vec4*)cam->pvMatrix, viewport, farPt);

    // Intersect ray with z=0 plane
    float dz = farPt[2] - nearPt[2];
    if (fabsf(dz) < 1e-6f) {
        // Ray parallel to z=0, fallback to near point
        *outWorldX = nearPt[0];
        *outWorldY = nearPt[1];
    } else {
        float t = (0.0f - nearPt[2]) / dz;
        *outWorldX = nearPt[0] + t * (farPt[0] - nearPt[0]);
        *outWorldY = nearPt[1] + t * (farPt[1] - nearPt[1]);
    }
}

// ---- Grid overlay ----

static void DrawGridOverlay(SOT_Editor *editor, SOT_Scene *scene, SOT_GPU_State *gpu,
                            ImVec2_c imgTopLeft, float imgW, float imgH)
{
    if (!editor->showGrid) return;

    ImDrawList *dl = igGetWindowDrawList();
    int gridSize = editor->prefs.gridSize;
    if (gridSize < 1) gridSize = 16;
    int intW = gpu->display.internalWidth;
    int intH = gpu->display.internalHeight;

    // Use editor camera when not playing, game camera when playing
    const sot_camera *cam = editor->editorCam.initialized && !editor->playing
                            ? &editor->editorCamState
                            : &scene->worldCamera;

    // Clip grid lines to the image rect so they're visible over the full viewport
    ImDrawList_PushClipRect(dl, imgTopLeft,
        (ImVec2_c){imgTopLeft.x + imgW, imgTopLeft.y + imgH}, true);

    // Get visible world bounds by unprojecting the four corners
    float worldMinX, worldMinY, worldMaxX, worldMaxY;
    ViewportToWorld(imgTopLeft.x, imgTopLeft.y + imgH, cam,
                    intW, intH, imgTopLeft, imgW, imgH, &worldMinX, &worldMinY);
    ViewportToWorld(imgTopLeft.x + imgW, imgTopLeft.y, cam,
                    intW, intH, imgTopLeft, imgW, imgH, &worldMaxX, &worldMaxY);

    // Use tilemap tile size if available, otherwise use pref grid size
    if (scene->tilemap && scene->tilemap->gpuTilemapInfo.TILE_WIDTH > 0)
        gridSize = scene->tilemap->gpuTilemapInfo.TILE_WIDTH;

    // Snap to grid (floor division for negative coords)
    int startX = (int)floorf(worldMinX / (float)gridSize) * gridSize;
    int startY = (int)floorf(worldMinY / (float)gridSize) * gridSize;
    int endX   = (int)ceilf(worldMaxX / (float)gridSize) * gridSize;
    int endY   = (int)ceilf(worldMaxY / (float)gridSize) * gridSize;

    ImU32 gridCol = igColorConvertFloat4ToU32((ImVec4_c){
        editor->prefs.gridColor[0], editor->prefs.gridColor[1],
        editor->prefs.gridColor[2], editor->prefs.gridColor[3]
    });

    // Vertical lines
    for (int x = startX; x <= endX; x += gridSize) {
        float sx1, sy1, sx2, sy2;
        WorldToViewport((float)x, worldMinY, cam, intW, intH, imgTopLeft, imgW, imgH, &sx1, &sy1);
        WorldToViewport((float)x, worldMaxY, cam, intW, intH, imgTopLeft, imgW, imgH, &sx2, &sy2);
        ImDrawList_AddLine(dl, (ImVec2_c){sx1, sy1}, (ImVec2_c){sx2, sy2}, gridCol, 1.0f);
    }

    // Horizontal lines
    for (int y = startY; y <= endY; y += gridSize) {
        float sx1, sy1, sx2, sy2;
        WorldToViewport(worldMinX, (float)y, cam, intW, intH, imgTopLeft, imgW, imgH, &sx1, &sy1);
        WorldToViewport(worldMaxX, (float)y, cam, intW, intH, imgTopLeft, imgW, imgH, &sx2, &sy2);
        ImDrawList_AddLine(dl, (ImVec2_c){sx1, sy1}, (ImVec2_c){sx2, sy2}, gridCol, 1.0f);
    }

    ImDrawList_PopClipRect(dl);
}

// ---- Gizmos: collider outlines ----

static void DrawGizmos(SOT_Editor *editor, SOT_Scene *scene, SOT_GPU_State *gpu,
                       ImVec2_c imgTopLeft, float imgW, float imgH)
{
    if (!editor->showGizmos) return;

    ImDrawList *dl = igGetWindowDrawList();
    int intW = gpu->display.internalWidth;
    int intH = gpu->display.internalHeight;

    // Use editor camera when not playing
    const sot_camera *cam = editor->editorCam.initialized && !editor->playing
                            ? &editor->editorCamState
                            : &scene->worldCamera;

    for (int i = 0; i < scene->actorsCount; i++) {
        SOT_Actor *actor = &scene->actors[i];
        if (!actor->enabled) continue;
        if (!b2Body_IsValid(actor->bodyId)) continue;

        b2Vec2 bpos = b2Body_GetPosition(actor->bodyId);
        float px = bpos.x / SOT_METERS_PER_PIXEL;
        float py = -bpos.y / SOT_METERS_PER_PIXEL;  // Box2D Y-down → game Y-up

        // Determine color
        bool isSelected = (i == editor->selectedActor);
        b2BodyType btype = b2Body_GetType(actor->bodyId);
        ImU32 col;
        if (isSelected) {
            col = igColorConvertFloat4ToU32((ImVec4_c){1.0f, 1.0f, 1.0f, 1.0f});
        } else if (btype == b2_staticBody) {
            col = igColorConvertFloat4ToU32((ImVec4_c){0.3f, 0.5f, 1.0f, 0.8f});
        } else if (btype == b2_dynamicBody) {
            col = igColorConvertFloat4ToU32((ImVec4_c){0.3f, 1.0f, 0.3f, 0.8f});
        } else {
            col = igColorConvertFloat4ToU32((ImVec4_c){1.0f, 1.0f, 0.3f, 0.8f});
        }

        // Use collider from cute_c2 (legacy) for shape info
        sot_collider_t *coll = &actor->collider;
        if (coll->type == C2_TYPE_AABB) {
            float x0 = coll->shape.AABB.min.x;
            float y0 = coll->shape.AABB.min.y;
            float x1 = coll->shape.AABB.max.x;
            float y1 = coll->shape.AABB.max.y;

            float sx0, sy0, sx1, sy1;
            WorldToViewport(x0, y0, cam, intW, intH, imgTopLeft, imgW, imgH, &sx0, &sy0);
            WorldToViewport(x1, y1, cam, intW, intH, imgTopLeft, imgW, imgH, &sx1, &sy1);
            ImDrawList_AddRect(dl, (ImVec2_c){sx0, sy0}, (ImVec2_c){sx1, sy1}, col, 0, 0, isSelected ? 2.0f : 1.0f);
        } else if (coll->type == C2_TYPE_CIRCLE) {
            float cx, cy;
            WorldToViewport(coll->shape.circle.p.x, coll->shape.circle.p.y,
                          cam, intW, intH, imgTopLeft, imgW, imgH, &cx, &cy);
            float r = coll->shape.circle.r * (imgW / (float)intW);
            ImDrawList_AddCircle(dl, (ImVec2_c){cx, cy}, r, col, 0, isSelected ? 2.0f : 1.0f);
        } else {
            // Fallback: draw a small cross at actor position
            float sx, sy;
            WorldToViewport(px, py, cam, intW, intH, imgTopLeft, imgW, imgH, &sx, &sy);
            ImDrawList_AddLine(dl, (ImVec2_c){sx - 4, sy}, (ImVec2_c){sx + 4, sy}, col, 1.0f);
            ImDrawList_AddLine(dl, (ImVec2_c){sx, sy - 4}, (ImVec2_c){sx, sy + 4}, col, 1.0f);
        }
    }
}

// ---- Viewport drag-to-move ----

static int HitTestActors(SOT_Scene *scene, float worldX, float worldY)
{
    for (int i = scene->actorsCount - 1; i >= 0; i--) {
        SOT_Actor *actor = &scene->actors[i];
        if (!actor->enabled) continue;
        if (actor->animationsCount <= 0 || actor->currentAnimation >= actor->animationsCount) continue;

        SOT_Animation *anim = &actor->animations[actor->currentAnimation];
        if (!anim->info || anim->sequenceIndex >= anim->info->count) continue;
        SOT_AnimationSequence *seq = &anim->info->sequences[anim->sequenceIndex];
        if (anim->currentFrame >= seq->count) continue;

        vec4 *frame = &seq->frames[anim->currentFrame];
        float fw = (*frame)[2];
        float fh = (*frame)[3];
        float ax = actor->transform.position[0];
        float ay = actor->transform.position[1];

        if (worldX >= ax && worldX <= ax + fw &&
            worldY >= ay && worldY <= ay + fh) {
            return i;
        }
    }
    return -1;
}

static void HandleViewportInteraction(SOT_Editor *editor, SOT_Scene *scene, SOT_GPU_State *gpu,
                                       ImVec2_c imgTopLeft, float imgW, float imgH)
{
    if (editor->playing) return;  // No editing during play
    if (editor->gizmoDragging) return;  // Gizmo has priority

    int intW = gpu->display.internalWidth;
    int intH = gpu->display.internalHeight;

    const sot_camera *cam = editor->editorCam.initialized
                            ? &editor->editorCamState
                            : &scene->worldCamera;

    if (igIsItemHovered(0)) {
        ImVec2_c mousePos = igGetMousePos();

        if (igIsMouseClicked_Bool(ImGuiMouseButton_Left, false)) {
            float wx, wy;
            ViewportToWorld(mousePos.x, mousePos.y, cam,
                          intW, intH, imgTopLeft, imgW, imgH, &wx, &wy);

            int hit = HitTestActors(scene, wx, wy);
            if (hit >= 0) {
                editor->selectedActor = hit;
                editor->isDragging = true;
                editor->dragActor = hit;
                SOT_Actor *a = &scene->actors[hit];
                editor->dragOffsetX = wx - a->transform.position[0];
                editor->dragOffsetY = wy - a->transform.position[1];

                // Push undo for drag start
                if (editor->undoTop < 50) {
                    SOT_UndoEntry *undo = &editor->undoStack[editor->undoTop];
                    undo->type = SOT_UNDO_POSITION;
                    undo->actorIndex = hit;
                    undo->old.position[0] = a->transform.position[0];
                    undo->old.position[1] = a->transform.position[1];
                    editor->undoTop++;
                    editor->undoCount = editor->undoTop;
                }
            } else {
                editor->selectedActor = -1;
            }
        }
    }

    if (editor->isDragging && igIsMouseDown_Nil(ImGuiMouseButton_Left)) {
        ImVec2_c mousePos = igGetMousePos();
        float wx, wy;
        ViewportToWorld(mousePos.x, mousePos.y, cam,
                      intW, intH, imgTopLeft, imgW, imgH, &wx, &wy);

        SOT_Actor *a = &scene->actors[editor->dragActor];
        vec2 newPos = { wx - editor->dragOffsetX, wy - editor->dragOffsetY };
        SetPosition(a, newPos);
    }

    if (editor->isDragging && igIsMouseReleased_Nil(ImGuiMouseButton_Left)) {
        editor->isDragging = false;
        editor->dragActor = -1;
    }
}

// ---- Editor camera helpers ----

// Initialize editor camera to center on the tilemap
static void SOT_EditorCam_InitFromScene(SOT_EditorCamera *ecam, SOT_Scene *scene)
{
    if (!scene || !scene->tilemap) return;

    SOT_GPU_TilemapInfo *ti = &scene->tilemap->gpuTilemapInfo;
    ecam->posX = (float)(ti->COLUMNS * ti->TILE_WIDTH) * 0.5f;
    ecam->posY = (float)(ti->ROWS * ti->TILE_HEIGHT) * -0.5f;
    ecam->zoom = 0.5f;
    ecam->initialized = true;
}

// Build a perspective PV matrix matching the game camera style but with editor zoom/pan
void SOT_EditorCam_BuildPVMatrix(SOT_EditorCamera *ecam, sot_camera *gameCam,
                                  mat4 outPV)
{
    // Reuse the game camera's projection but adjust the view for editor pan/zoom
    float fov = 45.0f * (GLM_PIf / 180.0f);

    // Compute Z distance based on zoom — smaller zoom = further away = see more
    // Base Z from the game camera
    float baseZ = gameCam->cameraInfo.eye[2];
    float editorZ = baseZ / ecam->zoom;

    vec3 eye    = { ecam->posX, ecam->posY, editorZ };
    vec3 center = { ecam->posX, ecam->posY, 0.0f };
    vec3 up     = { 0.0f, 1.0f, 0.0f };

    mat4 view, proj;
    glm_lookat(eye, center, up, view);
    glm_perspective(fov, SCREEN_WIDTH / SCREEN_HEIGHT, 5.0f, 100000.0f, proj);
    glm_mat4_mul(proj, view, outPV);
}

// Draw darkened overlay outside game camera view, and bright border around it
static void DrawCameraBoundsOverlay(SOT_Editor *editor, SOT_Scene *scene, SOT_GPU_State *gpu,
                                     ImVec2_c imgTopLeft, float imgW, float imgH)
{
    ImDrawList *dl = igGetWindowDrawList();
    int intW = gpu->display.internalWidth;
    int intH = gpu->display.internalHeight;

    // Get the four corners of the game camera's view in world space,
    // then project them using the editor camera (which is what the framebuffer shows)
    float gcMinX, gcMinY, gcMaxX, gcMaxY;

    // Game camera sees from (0,0) to (internalW, internalH) in its own framebuffer.
    // Unproject those corners using the GAME camera to get world coords.
    {
        vec4 viewport = { 0, 0, (float)intW, (float)intH };
        vec3 nearPt, farPt;

        // Bottom-left of game view
        vec3 winBL_n = { 0, 0, 0 };
        vec3 winBL_f = { 0, 0, 1 };
        glm_unproject(winBL_n, (vec4*)scene->worldCamera.pvMatrix, viewport, nearPt);
        glm_unproject(winBL_f, (vec4*)scene->worldCamera.pvMatrix, viewport, farPt);
        float dz = farPt[2] - nearPt[2];
        float t = (fabsf(dz) > 1e-6f) ? -nearPt[2] / dz : 0;
        gcMinX = nearPt[0] + t * (farPt[0] - nearPt[0]);
        gcMinY = nearPt[1] + t * (farPt[1] - nearPt[1]);

        // Top-right of game view
        vec3 winTR_n = { (float)intW, (float)intH, 0 };
        vec3 winTR_f = { (float)intW, (float)intH, 1 };
        glm_unproject(winTR_n, (vec4*)scene->worldCamera.pvMatrix, viewport, nearPt);
        glm_unproject(winTR_f, (vec4*)scene->worldCamera.pvMatrix, viewport, farPt);
        dz = farPt[2] - nearPt[2];
        t = (fabsf(dz) > 1e-6f) ? -nearPt[2] / dz : 0;
        gcMaxX = nearPt[0] + t * (farPt[0] - nearPt[0]);
        gcMaxY = nearPt[1] + t * (farPt[1] - nearPt[1]);
    }

    // Now project those world corners using the EDITOR camera (what the framebuffer currently shows)
    float sMinX, sMinY, sMaxX, sMaxY;
    WorldToViewport(gcMinX, gcMinY, &editor->editorCamState, intW, intH, imgTopLeft, imgW, imgH, &sMinX, &sMaxY);
    WorldToViewport(gcMaxX, gcMaxY, &editor->editorCamState, intW, intH, imgTopLeft, imgW, imgH, &sMaxX, &sMinY);

    // Clamp to image bounds
    float iLeft   = imgTopLeft.x;
    float iTop    = imgTopLeft.y;
    float iRight  = imgTopLeft.x + imgW;
    float iBottom = imgTopLeft.y + imgH;

    ImU32 shadowCol = igColorConvertFloat4ToU32((ImVec4_c){0, 0, 0, 0.55f});

    ImDrawList_PushClipRect(dl, imgTopLeft, (ImVec2_c){iRight, iBottom}, true);

    // Top shadow (above camera)
    if (sMinY > iTop)
        ImDrawList_AddRectFilled(dl, (ImVec2_c){iLeft, iTop}, (ImVec2_c){iRight, sMinY}, shadowCol, 0, 0);
    // Bottom shadow (below camera)
    if (sMaxY < iBottom)
        ImDrawList_AddRectFilled(dl, (ImVec2_c){iLeft, sMaxY}, (ImVec2_c){iRight, iBottom}, shadowCol, 0, 0);
    // Left shadow (between top and bottom shadows)
    if (sMinX > iLeft)
        ImDrawList_AddRectFilled(dl, (ImVec2_c){iLeft, sMinY}, (ImVec2_c){sMinX, sMaxY}, shadowCol, 0, 0);
    // Right shadow
    if (sMaxX < iRight)
        ImDrawList_AddRectFilled(dl, (ImVec2_c){sMaxX, sMinY}, (ImVec2_c){iRight, sMaxY}, shadowCol, 0, 0);

    // Bright border around game camera bounds
    ImU32 borderCol = igColorConvertFloat4ToU32((ImVec4_c){1.0f, 0.9f, 0.0f, 0.8f});
    ImDrawList_AddRect(dl, (ImVec2_c){sMinX, sMinY}, (ImVec2_c){sMaxX, sMaxY}, borderCol, 0, 0, 2.0f);

    ImDrawList_PopClipRect(dl);
}

// ---- Move Gizmo ----

#define SOT_SELECTED_CAMERA -2

#define GIZMO_ARROW_LEN   40.0f
#define GIZMO_ARROW_THICK  3.0f
#define GIZMO_ARROW_HEAD   10.0f
#define GIZMO_CENTER_SIZE  8.0f
#define GIZMO_HIT_PADDING  6.0f

static float SnapToHalfGrid(float val, int gridSize)
{
    float halfGrid = (float)gridSize * 0.5f;
    return floorf(val / halfGrid + 0.5f) * halfGrid;
}

static void DrawAndHandleMoveGizmo(SOT_Editor *editor, SOT_Scene *scene, SOT_GPU_State *gpu,
                                    ImVec2_c imgTopLeft, float imgW, float imgH)
{
    if (editor->playing) return;

    // Determine what we're moving: actor or camera
    bool isCamera = (editor->selectedActor == SOT_SELECTED_CAMERA);
    SOT_Actor *actor = NULL;
    float ax, ay;

    if (isCamera) {
        ax = scene->worldCamera.cameraInfo.eye[0];
        ay = scene->worldCamera.cameraInfo.eye[1];
    } else {
        if (editor->selectedActor < 0 || editor->selectedActor >= scene->actorsCount) return;
        actor = &scene->actors[editor->selectedActor];
        if (!actor->enabled) return;
        ax = actor->transform.position[0];
        ay = actor->transform.position[1];
    }

    int intW = gpu->display.internalWidth;
    int intH = gpu->display.internalHeight;

    const sot_camera *cam = editor->editorCam.initialized
                            ? &editor->editorCamState
                            : &scene->worldCamera;
    float sx, sy;
    WorldToViewport(ax, ay, cam, intW, intH, imgTopLeft, imgW, imgH, &sx, &sy);

    // Arrow endpoints (screen space, fixed size)
    ImVec2_c center = { sx, sy };
    ImVec2_c xEnd   = { sx + GIZMO_ARROW_LEN, sy };
    ImVec2_c yEnd   = { sx, sy - GIZMO_ARROW_LEN };  // up in screen = negative Y

    // Colors
    bool xActive = (editor->gizmoDragAxis == 1);
    bool yActive = (editor->gizmoDragAxis == 2);
    bool cActive = (editor->gizmoDragAxis == 3);

    ImU32 xCol = xActive
        ? igColorConvertFloat4ToU32((ImVec4_c){1.0f, 1.0f, 0.0f, 1.0f})
        : igColorConvertFloat4ToU32((ImVec4_c){1.0f, 0.2f, 0.2f, 1.0f});
    ImU32 yCol = yActive
        ? igColorConvertFloat4ToU32((ImVec4_c){1.0f, 1.0f, 0.0f, 1.0f})
        : igColorConvertFloat4ToU32((ImVec4_c){0.2f, 1.0f, 0.2f, 1.0f});
    ImU32 cCol = cActive
        ? igColorConvertFloat4ToU32((ImVec4_c){1.0f, 1.0f, 0.0f, 1.0f})
        : igColorConvertFloat4ToU32((ImVec4_c){1.0f, 1.0f, 1.0f, 0.9f});

    float xThick = xActive ? GIZMO_ARROW_THICK * 2.0f : GIZMO_ARROW_THICK;
    float yThick = yActive ? GIZMO_ARROW_THICK * 2.0f : GIZMO_ARROW_THICK;

    ImDrawList *dl = igGetWindowDrawList();

    // Draw X arrow (right)
    ImDrawList_AddLine(dl, center, xEnd, xCol, xThick);
    // Arrowhead
    ImDrawList_AddTriangleFilled(dl,
        (ImVec2_c){ xEnd.x + GIZMO_ARROW_HEAD, xEnd.y },
        (ImVec2_c){ xEnd.x - 2, xEnd.y - GIZMO_ARROW_HEAD * 0.5f },
        (ImVec2_c){ xEnd.x - 2, xEnd.y + GIZMO_ARROW_HEAD * 0.5f },
        xCol);

    // Draw Y arrow (up)
    ImDrawList_AddLine(dl, center, yEnd, yCol, yThick);
    // Arrowhead
    ImDrawList_AddTriangleFilled(dl,
        (ImVec2_c){ yEnd.x, yEnd.y - GIZMO_ARROW_HEAD },
        (ImVec2_c){ yEnd.x - GIZMO_ARROW_HEAD * 0.5f, yEnd.y + 2 },
        (ImVec2_c){ yEnd.x + GIZMO_ARROW_HEAD * 0.5f, yEnd.y + 2 },
        yCol);

    // Draw center box
    float hs = GIZMO_CENTER_SIZE * 0.5f;
    ImDrawList_AddRectFilled(dl,
        (ImVec2_c){ center.x - hs, center.y - hs },
        (ImVec2_c){ center.x + hs, center.y + hs },
        cCol, 0, 0);

    // ---- Hit testing and interaction ----
    ImVec2_c mousePos = igGetMousePos();
    float mx = mousePos.x;
    float my = mousePos.y;

    // Check if mouse is over each gizmo part
    bool overX = false, overY = false, overCenter = false;

    // Center box hit
    if (mx >= center.x - hs - GIZMO_HIT_PADDING && mx <= center.x + hs + GIZMO_HIT_PADDING &&
        my >= center.y - hs - GIZMO_HIT_PADDING && my <= center.y + hs + GIZMO_HIT_PADDING) {
        overCenter = true;
    }

    // X arrow hit (line from center to xEnd, with padding)
    if (!overCenter &&
        mx >= center.x + hs && mx <= xEnd.x + GIZMO_ARROW_HEAD + GIZMO_HIT_PADDING &&
        my >= center.y - GIZMO_HIT_PADDING * 2 && my <= center.y + GIZMO_HIT_PADDING * 2) {
        overX = true;
    }

    // Y arrow hit (line from center to yEnd, with padding)
    if (!overCenter && !overX &&
        mx >= center.x - GIZMO_HIT_PADDING * 2 && mx <= center.x + GIZMO_HIT_PADDING * 2 &&
        my >= yEnd.y - GIZMO_ARROW_HEAD - GIZMO_HIT_PADDING && my <= center.y - hs) {
        overY = true;
    }

    // Start dragging
    if (igIsMouseClicked_Bool(ImGuiMouseButton_Left, false) && !editor->gizmoDragging) {
        if (overCenter || overX || overY) {
            editor->gizmoDragging = true;
            editor->gizmoDragAxis = overX ? 1 : overY ? 2 : 3;
            editor->gizmoDragStartX = ax;
            editor->gizmoDragStartY = ay;

            // Push undo (only for actors, not camera)
            if (!isCamera && editor->undoTop < 50) {
                editor->undoStack[editor->undoTop] = (SOT_UndoEntry){
                    .type = SOT_UNDO_POSITION,
                    .actorIndex = editor->selectedActor,
                    .old.position = { ax, ay }
                };
                editor->undoTop++;
                editor->undoCount = editor->undoTop;
            }
        }
    }

    // During drag
    if (editor->gizmoDragging && igIsMouseDown_Nil(ImGuiMouseButton_Left)) {
        ImVec2_c delta = igGetIO()->MouseDelta;
        if (delta.x != 0 || delta.y != 0) {
            // Convert screen delta to world delta
            float wMinX, wMinY, wMaxX, wMaxY;
            ViewportToWorld(imgTopLeft.x, imgTopLeft.y + imgH, cam,
                           intW, intH, imgTopLeft, imgW, imgH, &wMinX, &wMinY);
            ViewportToWorld(imgTopLeft.x + imgW, imgTopLeft.y, cam,
                           intW, intH, imgTopLeft, imgW, imgH, &wMaxX, &wMaxY);
            float worldW = wMaxX - wMinX;
            float worldH = wMaxY - wMinY;
            float pixPerWorldX = imgW / worldW;
            float pixPerWorldY = imgH / worldH;

            float dx = delta.x / pixPerWorldX;
            float dy = -delta.y / pixPerWorldY;  // screen Y is flipped

            float curX = isCamera ? scene->worldCamera.cameraInfo.eye[0]
                                  : actor->transform.position[0];
            float curY = isCamera ? scene->worldCamera.cameraInfo.eye[1]
                                  : actor->transform.position[1];
            float newX = curX;
            float newY = curY;

            if (editor->gizmoDragAxis == 1 || editor->gizmoDragAxis == 3)
                newX += dx;
            if (editor->gizmoDragAxis == 2 || editor->gizmoDragAxis == 3)
                newY += dy;

            // Snap logic: Ctrl inverts the default snap setting
            bool doSnap = editor->snapToGrid;
            if (igGetIO()->KeyCtrl) doSnap = !doSnap;

            if (doSnap) {
                int gridSize = editor->prefs.gridSize;
                if (gridSize < 1) gridSize = 16;
                if (scene->tilemap && scene->tilemap->gpuTilemapInfo.TILE_WIDTH > 0)
                    gridSize = scene->tilemap->gpuTilemapInfo.TILE_WIDTH;

                if (editor->gizmoDragAxis == 1 || editor->gizmoDragAxis == 3)
                    newX = SnapToHalfGrid(newX, gridSize);
                if (editor->gizmoDragAxis == 2 || editor->gizmoDragAxis == 3)
                    newY = SnapToHalfGrid(newY, gridSize);
            }

            if (isCamera) {
                scene->worldCamera.cameraInfo.eye[0] = newX;
                scene->worldCamera.cameraInfo.eye[1] = newY;
                scene->worldCamera.cameraInfo.center[0] = newX;
                scene->worldCamera.cameraInfo.center[1] = newY;
                // Rebuild view + PV matrices so the change is visible
                glm_lookat(scene->worldCamera.cameraInfo.eye,
                           scene->worldCamera.cameraInfo.center,
                           scene->worldCamera.cameraInfo.up,
                           scene->worldCamera.view);
                glm_mat4_mul(scene->worldCamera.projection,
                             scene->worldCamera.view,
                             scene->worldCamera.pvMatrix);
            } else {
                vec2 newPos = { newX, newY };
                SetPosition(actor, newPos);
            }
        }
    }

    // End drag
    if (editor->gizmoDragging && igIsMouseReleased_Nil(ImGuiMouseButton_Left)) {
        editor->gizmoDragging = false;
        editor->gizmoDragAxis = 0;
    }
}

// ---- Scene Viewport panel ----

static void DrawSceneViewport(SOT_Editor *editor, SOT_Scene *scene, SOT_GPU_State *gpu)
{
    igBegin("Scene Viewport", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Get available size for the viewport
    ImVec2_c avail = igGetContentRegionAvail();

    if (gpu->display.framebuffer != NULL && avail.x > 0 && avail.y > 0) {
        // Compute size maintaining aspect ratio of the internal resolution
        float internalAspect = (float)gpu->display.internalWidth / (float)gpu->display.internalHeight;
        float panelAspect = avail.x / avail.y;

        float imgW, imgH;
        if (panelAspect > internalAspect) {
            imgH = avail.y;
            imgW = imgH * internalAspect;
        } else {
            imgW = avail.x;
            imgH = imgW / internalAspect;
        }

        // Center the image
        float padX = (avail.x - imgW) * 0.5f;
        float padY = (avail.y - imgH) * 0.5f;
        if (padX > 0) { igSetCursorPosX(igGetCursorPosX() + padX); }
        if (padY > 0) { igSetCursorPosY(igGetCursorPosY() + padY); }

        // Fill the entire viewport panel with black
        ImVec2_c panelTopLeft = igGetCursorScreenPos();
        panelTopLeft.x -= padX > 0 ? padX : 0;
        panelTopLeft.y -= padY > 0 ? padY : 0;
        ImDrawList *drawList = igGetWindowDrawList();
        ImDrawList_AddRectFilled(drawList, panelTopLeft,
            (ImVec2_c){panelTopLeft.x + avail.x, panelTopLeft.y + avail.y},
            igColorConvertFloat4ToU32((ImVec4_c){0, 0, 0, 1}), 0, 0);

        // Capture image top-left position for overlays
        ImVec2_c imgTopLeft = igGetCursorScreenPos();

        // Switch to nearest-neighbor sampling for pixel-art framebuffer
        ImDrawList_AddCallback(drawList, SetSamplerNearest, NULL, 0);

        // Display the framebuffer texture
        ImTextureRef_c texRef;
        texRef._TexData = NULL;
        texRef._TexID = (ImTextureID)(uintptr_t)gpu->display.framebuffer;

        ImVec2_c imgSize = { imgW, imgH };
        ImVec2_c uv0 = { 0, 0 };
        ImVec2_c uv1 = { 1, 1 };
        igImage(texRef, imgSize, uv0, uv1);

        // Restore linear sampling for other ImGui content
        ImDrawList_AddCallback(drawList, SetSamplerLinear, NULL, 0);

        // ---- Editor camera pan/zoom ----
        if (igIsItemHovered(0) && !editor->playing) {
            int intW = gpu->display.internalWidth;
            int intH = gpu->display.internalHeight;
            const sot_camera *cam = editor->editorCam.initialized
                                    ? &editor->editorCamState
                                    : &scene->worldCamera;

            // Scroll wheel: zoom toward mouse cursor
            float wheel = igGetIO()->MouseWheel;
            if (wheel != 0.0f) {
                float oldZoom = editor->editorCam.zoom;

                // Zoom factor per scroll notch
                float zoomFactor = 1.15f;
                if (wheel > 0) editor->editorCam.zoom *= zoomFactor;
                else           editor->editorCam.zoom /= zoomFactor;

                // Clamp zoom
                if (editor->editorCam.zoom < 0.1f) editor->editorCam.zoom = 0.1f;
                if (editor->editorCam.zoom > 4.0f) editor->editorCam.zoom = 4.0f;

                // Zoom toward mouse cursor: adjust camera position so the
                // world point under the cursor stays fixed
                ImVec2_c mousePos = igGetMousePos();
                float worldX, worldY;
                ViewportToWorld(mousePos.x, mousePos.y, cam,
                               intW, intH, imgTopLeft, imgW, imgH, &worldX, &worldY);

                float zoomRatio = oldZoom / editor->editorCam.zoom;
                editor->editorCam.posX = worldX + (editor->editorCam.posX - worldX) * zoomRatio;
                editor->editorCam.posY = worldY + (editor->editorCam.posY - worldY) * zoomRatio;
            }

            // Middle mouse drag: pan
            if (igIsMouseDragging(ImGuiMouseButton_Middle, 0)) {
                ImVec2_c delta = igGetIO()->MouseDelta;
                // Convert screen delta to world delta
                // At current zoom, how many world units per screen pixel?
                float worldW, worldH;
                float wMinX, wMinY, wMaxX, wMaxY;
                ViewportToWorld(imgTopLeft.x, imgTopLeft.y + imgH, cam,
                               intW, intH, imgTopLeft, imgW, imgH, &wMinX, &wMinY);
                ViewportToWorld(imgTopLeft.x + imgW, imgTopLeft.y, cam,
                               intW, intH, imgTopLeft, imgW, imgH, &wMaxX, &wMaxY);
                worldW = wMaxX - wMinX;
                worldH = wMaxY - wMinY;

                float pixelsPerWorldX = imgW / worldW;
                float pixelsPerWorldY = imgH / worldH;

                editor->editorCam.posX -= delta.x / pixelsPerWorldX;
                editor->editorCam.posY += delta.y / pixelsPerWorldY;  // Y is flipped
            }
        }

        // Home key: reset editor camera
        if (igIsKeyPressed_Bool(ImGuiKey_Home, false) && !editor->playing) {
            editor->editorCam.initialized = false;  // Will re-init from scene next frame
        }

        // Move gizmo runs first so it can consume clicks before viewport selection
        DrawAndHandleMoveGizmo(editor, scene, gpu, imgTopLeft, imgW, imgH);

        // Viewport interaction (click to select, drag to move)
        HandleViewportInteraction(editor, scene, gpu, imgTopLeft, imgW, imgH);

        // Draw overlays on top of the image
        DrawGridOverlay(editor, scene, gpu, imgTopLeft, imgW, imgH);
        if (!editor->playing)
            DrawCameraBoundsOverlay(editor, scene, gpu, imgTopLeft, imgW, imgH);
        DrawGizmos(editor, scene, gpu, imgTopLeft, imgW, imgH);
    } else {
        igText("No framebuffer available");
    }

    igEnd();
}

// Case-insensitive substring search
static bool StrContainsCI(const char *haystack, const char *needle)
{
    if (!needle[0]) return true;
    for (const char *h = haystack; *h; h++) {
        const char *a = h, *b = needle;
        while (*a && *b && SDL_tolower((unsigned char)*a) == SDL_tolower((unsigned char)*b)) { a++; b++; }
        if (!*b) return true;
    }
    return false;
}

// Delete actor at index, shift remaining down, fixup references
static void HierarchyDeleteActor(SOT_Editor *editor, SOT_Scene *scene, int idx)
{
    if (idx < 0 || idx >= scene->actorsCount) return;

    // Shift actors and GPU sprite info down
    for (int i = idx; i < scene->actorsCount - 1; i++) {
        scene->actors[i] = scene->actors[i + 1];
        scene->gpuSpritesInfo[i] = scene->gpuSpritesInfo[i + 1];
    }
    scene->actorsCount--;

    // Fixup camera target
    if (scene->cameraTargetActor == idx)
        scene->cameraTargetActor = -1;
    else if (scene->cameraTargetActor > idx)
        scene->cameraTargetActor--;

    // Fixup selection
    if (editor->selectedActor == idx)
        editor->selectedActor = -1;
    else if (editor->selectedActor > idx)
        editor->selectedActor--;

    // Fixup undo stack (invalidate entries referencing deleted actor, adjust indices)
    for (int i = 0; i < editor->undoTop; i++) {
        if (editor->undoStack[i].actorIndex == idx)
            editor->undoStack[i].actorIndex = -1;  // invalidate
        else if (editor->undoStack[i].actorIndex > idx)
            editor->undoStack[i].actorIndex--;
    }

    // Cancel any active drag/rename
    editor->isDragging = false;
    editor->dragActor = -1;
    editor->isRenaming = false;
}

// Duplicate actor at index, insert copy right after it
static void HierarchyDuplicateActor(SOT_Editor *editor, SOT_Scene *scene, int idx)
{
    if (idx < 0 || idx >= scene->actorsCount) return;
    if (scene->actorsCount >= 2000) return;  // array full

    // Shift everything after idx+1 up by one
    int insertAt = idx + 1;
    for (int i = scene->actorsCount; i > insertAt; i--) {
        scene->actors[i] = scene->actors[i - 1];
        scene->gpuSpritesInfo[i] = scene->gpuSpritesInfo[i - 1];
    }

    // Deep copy
    scene->actors[insertAt] = scene->actors[idx];
    scene->gpuSpritesInfo[insertAt] = scene->gpuSpritesInfo[idx];
    scene->actorsCount++;

    // Offset position by one grid cell
    int gridSize = 16;
    if (scene->tilemap && scene->tilemap->gpuTilemapInfo.TILE_WIDTH > 0)
        gridSize = scene->tilemap->gpuTilemapInfo.TILE_WIDTH;
    scene->actors[insertAt].transform.position[0] += (float)gridSize;

    // Append " (copy)" to name
    SOT_Actor *dup = &scene->actors[insertAt];
    char tmp[64];
    SDL_strlcpy(tmp, dup->actorName, sizeof(tmp));
    SDL_snprintf(dup->actorName, sizeof(dup->actorName), "%s (copy)", tmp);

    // Assign new actor ID
    dup->actorID = scene->actorsCount - 1;

    // Physics body is NOT duplicated (would need separate Box2D create)
    dup->bodyId = b2_nullBodyId;

    // Fixup camera target and undo indices above insertAt
    if (scene->cameraTargetActor >= insertAt)
        scene->cameraTargetActor++;
    for (int i = 0; i < editor->undoTop; i++) {
        if (editor->undoStack[i].actorIndex >= insertAt)
            editor->undoStack[i].actorIndex++;
    }

    // Select the new actor
    editor->selectedActor = insertAt;
}

// Swap two actors in the array, fixup references
static void HierarchySwapActors(SOT_Editor *editor, SOT_Scene *scene, int a, int b)
{
    if (a < 0 || a >= scene->actorsCount || b < 0 || b >= scene->actorsCount) return;

    SOT_Actor tmpActor = scene->actors[a];
    scene->actors[a] = scene->actors[b];
    scene->actors[b] = tmpActor;

    SOT_GPU_SpriteInstance tmpSprite = scene->gpuSpritesInfo[a];
    scene->gpuSpritesInfo[a] = scene->gpuSpritesInfo[b];
    scene->gpuSpritesInfo[b] = tmpSprite;

    // Fixup camera target
    if (scene->cameraTargetActor == a) scene->cameraTargetActor = b;
    else if (scene->cameraTargetActor == b) scene->cameraTargetActor = a;

    // Fixup undo stack
    for (int i = 0; i < editor->undoTop; i++) {
        if (editor->undoStack[i].actorIndex == a) editor->undoStack[i].actorIndex = b;
        else if (editor->undoStack[i].actorIndex == b) editor->undoStack[i].actorIndex = a;
    }

    // Fixup selection
    if (editor->selectedActor == a) editor->selectedActor = b;
    else if (editor->selectedActor == b) editor->selectedActor = a;
}

static void DrawSceneHierarchy(SOT_Editor *editor, SOT_Scene *scene, AppState *as)
{
    igBegin("Hierarchy", NULL, 0);

    igText("Scene: %s", scene->name[0] ? scene->name : "(unnamed)");
    igSameLine(0, 8);
    igText("(%d)", scene->actorsCount);
    igSameLine(0, 8);

    // "+" button to add actor from template
    if (igSmallButton("+##AddActor")) {
        igOpenPopup_Str("##AddActorPopup", 0);
    }
    if (igIsItemHovered(0)) igSetTooltip("Add Actor from Template");

    if (igBeginPopup("##AddActorPopup", 0)) {
        // Cache template file list
        static char cachedTemplates[64][128];
        static int cachedTemplateCount = -1;
        static bool needsRefresh = true;

        if (needsRefresh) {
            cachedTemplateCount = 0;
            char dir[512];
            SDL_snprintf(dir, sizeof(dir), "%sassets/actors", Paths.Base);
            int count = 0;
            char **list = SDL_GlobDirectory(dir, "*.json", 0, &count);
            if (list) {
                for (int i = 0; i < count && cachedTemplateCount < 64; i++) {
                    SDL_strlcpy(cachedTemplates[cachedTemplateCount], list[i], 128);
                    cachedTemplateCount++;
                }
                SDL_free(list);
            }
            needsRefresh = false;
        }

        igText("Select Template:");
        igSeparator();

        for (int i = 0; i < cachedTemplateCount; i++) {
            // Strip .json extension for display and loading
            char displayName[128];
            SDL_strlcpy(displayName, cachedTemplates[i], sizeof(displayName));
            char *dot = SDL_strrchr(displayName, '.');
            if (dot) *dot = '\0';

            if (igSelectable_Bool(displayName, false, 0, (ImVec2_c){0, 0})) {
                if (scene->actorsCount >= 2000) {
                    SOT_Editor_LogWarn(editor, "Cannot add actor: scene limit (2000) reached");
                } else {
                    SOT_ActorTemplate *tmpl = SOT_LoadActorTemplate(displayName);
                    if (tmpl) {
                        vec2 spawnPos = {editor->editorCam.posX, editor->editorCam.posY};
                        int idx = scene->actorsCount;
                        scene->actors[idx] = SOT_CreateActorFromTemplate(as, tmpl, spawnPos, displayName);
                        scene->actors[idx].actorID = idx;
                        SOT_Actor_CreatePhysicsBody(&scene->actors[idx], &scene->physics, tmpl);
                        scene->actorsCount++;

                        // Rebuild GPU sprite data
                        if (as->gpu->pipelineFlags & SOT_RP_SPRITES_FLAG)
                            SOT_GPU_InitializeActors(scene, as->gpu);

                        editor->selectedActor = idx;
                        SOT_Editor_Log(editor, "Added actor '%s' from template '%s'", displayName, displayName);
                        SOT_FreeActorTemplate(tmpl);
                    }
                }
                needsRefresh = true;
                igCloseCurrentPopup();
            }
        }

        igSeparator();
        // New Template option
        static char newTmplName[64] = "";
        static bool showNewTmplInput = false;
        if (!showNewTmplInput) {
            if (igSelectable_Bool("+ New Template...", false, 0, (ImVec2_c){0, 0})) {
                showNewTmplInput = true;
                newTmplName[0] = '\0';
            }
        } else {
            igInputText("Name", newTmplName, sizeof(newTmplName), 0, NULL, NULL);
            if (igSmallButton("Create") && newTmplName[0]) {
                SOT_ActorTemplate defaultTmpl = {0};
                SDL_strlcpy(defaultTmpl.name, newTmplName, sizeof(defaultTmpl.name));
                defaultTmpl.bodyType = SOT_BODY_NONE;
                defaultTmpl.gravityScale = 1.0f;
                SOT_SaveActorTemplate(&defaultTmpl, newTmplName);
                SOT_Editor_Log(editor, "Created new template: %s", newTmplName);
                needsRefresh = true;
                showNewTmplInput = false;
                igCloseCurrentPopup();
            }
            igSameLine(0, 4);
            if (igSmallButton("Cancel")) {
                showNewTmplInput = false;
            }
        }

        igEndPopup();
    }

    igSeparator();

    // Visibility toggles
    igCheckbox("Tilemap", &scene->editorShowTilemap);
    igSameLine(0, 10);
    igCheckbox("Debug", &scene->editorShowDebug);
    igSeparator();

    // Search/filter bar
    igSetNextItemWidth(-1);
    igInputTextWithHint("##filter", "Filter...", editor->hierarchyFilter,
                        sizeof(editor->hierarchyFilter), 0, NULL, NULL);
    igSeparator();

    // Camera pseudo-actor entry
    {
        bool camSelected = (editor->selectedActor == SOT_SELECTED_CAMERA);
        if (igSelectable_Bool("[Camera]", camSelected, 0, (ImVec2_c){0, 0})) {
            editor->selectedActor = SOT_SELECTED_CAMERA;
            editor->isRenaming = false;
        }
    }

    igSeparator();

    // Keyboard shortcuts (when hierarchy window is focused)
    bool hierarchyFocused = igIsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    // Track deferred actions (don't modify array while iterating)
    int deleteIdx = -1;
    int duplicateIdx = -1;
    int moveUpIdx = -1;
    int moveDownIdx = -1;
    int focusIdx = -1;

    for (int i = 0; i < scene->actorsCount; i++) {
        SOT_Actor *actor = &scene->actors[i];

        // Filter check
        if (editor->hierarchyFilter[0]) {
            bool match = StrContainsCI(actor->actorName, editor->hierarchyFilter);
            if (!match && actor->templateName[0])
                match = StrContainsCI(actor->templateName, editor->hierarchyFilter);
            if (!match) continue;
        }

        igPushID_Int(i);
        bool selected = (editor->selectedActor == i);

        // Eye icon (visibility toggle) — small button before the selectable
        {
            ImVec4_c eyeCol = actor->enabled
                ? (ImVec4_c){1.0f, 1.0f, 1.0f, 1.0f}
                : (ImVec4_c){0.4f, 0.4f, 0.4f, 1.0f};
            igPushStyleColor_Vec4(ImGuiCol_Text, eyeCol);
            if (igSmallButton(actor->enabled ? "o" : "-")) {
                SOT_Actor_SetEnabled(actor, !actor->enabled);
            }
            igPopStyleColor(1);
        }
        igSameLine(0, 4);

        // Dim disabled actors
        if (!actor->enabled) {
            igPushStyleColor_Vec4(ImGuiCol_Text, (ImVec4_c){0.5f, 0.5f, 0.5f, 1.0f});
        }

        // Inline rename mode
        if (editor->isRenaming && editor->renamingActor == i) {
            igSetKeyboardFocusHere(0);
            igSetNextItemWidth(-1);
            bool enterPressed = igInputText("##rename", editor->renameBuf,
                sizeof(editor->renameBuf),
                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll,
                NULL, NULL);

            if (enterPressed || igIsItemDeactivatedAfterEdit()) {
                if (editor->renameBuf[0])
                    SDL_strlcpy(actor->actorName, editor->renameBuf, sizeof(actor->actorName));
                editor->isRenaming = false;
            }
            if (igIsKeyPressed_Bool(ImGuiKey_Escape, false)) {
                editor->isRenaming = false;
            }
        } else {
            // Normal selectable row: name + [index]
            char label[192];
            if (actor->templateName[0]) {
                SDL_snprintf(label, sizeof(label), "%s [%d]",
                    actor->actorName[0] ? actor->actorName : "(unnamed)", i);
            } else {
                SDL_snprintf(label, sizeof(label), "%s [%d]",
                    actor->actorName[0] ? actor->actorName : "(unnamed)", i);
            }

            if (igSelectable_Bool(label, selected, ImGuiSelectableFlags_AllowDoubleClick, (ImVec2_c){0, 0})) {
                editor->selectedActor = i;

                // Double-click to start rename
                if (igIsMouseDoubleClicked_Nil(ImGuiMouseButton_Left)) {
                    editor->isRenaming = true;
                    editor->renamingActor = i;
                    SDL_strlcpy(editor->renameBuf, actor->actorName, sizeof(editor->renameBuf));
                }
            }

            // Template name in dimmed text on the same line
            if (actor->templateName[0]) {
                igSameLine(0, 6);
                igTextDisabled("(%s)", actor->templateName);
            }
        }

        if (!actor->enabled) {
            igPopStyleColor(1);
        }

        // Context menu (right-click)
        if (igBeginPopupContextItem("##actorctx", ImGuiPopupFlags_MouseButtonRight)) {
            editor->selectedActor = i;

            if (igMenuItem_Bool(actor->enabled ? "Disable" : "Enable", NULL, false, true)) {
                SOT_Actor_SetEnabled(actor, !actor->enabled);
            }
            if (igMenuItem_Bool("Rename", "F2", false, true)) {
                editor->isRenaming = true;
                editor->renamingActor = i;
                SDL_strlcpy(editor->renameBuf, actor->actorName, sizeof(editor->renameBuf));
            }
            if (igMenuItem_Bool("Duplicate", "Ctrl+D", false, true)) {
                duplicateIdx = i;
            }
            if (igMenuItem_Bool("Delete", "Del", false, true)) {
                deleteIdx = i;
            }
            igSeparator();
            if (igMenuItem_Bool("Focus Camera", NULL, false, true)) {
                focusIdx = i;
            }
            igSeparator();
            if (igMenuItem_Bool("Move Up", NULL, false, i > 0)) {
                moveUpIdx = i;
            }
            if (igMenuItem_Bool("Move Down", NULL, false, i < scene->actorsCount - 1)) {
                moveDownIdx = i;
            }
            igEndPopup();
        }

        igPopID();
    }

    // Keyboard shortcuts (only when hierarchy is focused and not renaming)
    if (hierarchyFocused && !editor->isRenaming && editor->selectedActor >= 0
        && editor->selectedActor != SOT_SELECTED_CAMERA) {
        int sel = editor->selectedActor;

        if (igIsKeyPressed_Bool(ImGuiKey_Delete, false)) {
            deleteIdx = sel;
        }
        if (igGetIO()->KeyCtrl && igIsKeyPressed_Bool(ImGuiKey_D, false)) {
            duplicateIdx = sel;
        }
        if (igIsKeyPressed_Bool(ImGuiKey_F2, false)) {
            editor->isRenaming = true;
            editor->renamingActor = sel;
            SDL_strlcpy(editor->renameBuf, scene->actors[sel].actorName, sizeof(editor->renameBuf));
        }
        if (igIsKeyPressed_Bool(ImGuiKey_UpArrow, false) && sel > 0) {
            editor->selectedActor = sel - 1;
        }
        if (igIsKeyPressed_Bool(ImGuiKey_DownArrow, false) && sel < scene->actorsCount - 1) {
            editor->selectedActor = sel + 1;
        }
    }

    // Execute deferred actions (order matters: delete last since it shifts indices)
    if (focusIdx >= 0 && focusIdx < scene->actorsCount) {
        editor->editorCam.posX = scene->actors[focusIdx].transform.position[0];
        editor->editorCam.posY = scene->actors[focusIdx].transform.position[1];
    }
    if (moveUpIdx >= 0) HierarchySwapActors(editor, scene, moveUpIdx, moveUpIdx - 1);
    if (moveDownIdx >= 0) HierarchySwapActors(editor, scene, moveDownIdx, moveDownIdx + 1);
    if (duplicateIdx >= 0) HierarchyDuplicateActor(editor, scene, duplicateIdx);
    if (deleteIdx >= 0) HierarchyDeleteActor(editor, scene, deleteIdx);

    igEnd();
}

// ---- Undo/Redo system ----

static void PushUndo(SOT_Editor *editor, SOT_UndoEntry entry)
{
    if (editor->undoTop >= 50) {
        // Shift stack left, discard oldest
        SDL_memmove(&editor->undoStack[0], &editor->undoStack[1], 49 * sizeof(SOT_UndoEntry));
        editor->undoTop = 49;
    }
    editor->undoStack[editor->undoTop] = entry;
    editor->undoTop++;
    editor->undoCount = editor->undoTop;
}

static void DoUndo(SOT_Editor *editor, SOT_Scene *scene)
{
    if (editor->undoTop <= 0) return;
    editor->undoTop--;
    SOT_UndoEntry *e = &editor->undoStack[editor->undoTop];

    if (e->actorIndex < 0 || e->actorIndex >= scene->actorsCount) return;
    SOT_Actor *a = &scene->actors[e->actorIndex];

    switch (e->type) {
        case SOT_UNDO_POSITION: {
            // Save current for redo, then restore
            float curPos[2] = { a->transform.position[0], a->transform.position[1] };
            vec2 newPos = { e->old.position[0], e->old.position[1] };
            SetPosition(a, newPos);
            e->old.position[0] = curPos[0];
            e->old.position[1] = curPos[1];
            break;
        }
        case SOT_UNDO_SCALE: {
            float cur[2] = { a->transform.scale[0], a->transform.scale[1] };
            a->transform.scale[0] = e->old.scale[0];
            a->transform.scale[1] = e->old.scale[1];
            e->old.scale[0] = cur[0];
            e->old.scale[1] = cur[1];
            break;
        }
        case SOT_UNDO_ENABLED: {
            bool cur = a->enabled;
            SOT_Actor_SetEnabled(a, e->old.enabled);
            e->old.enabled = cur;
            break;
        }
        case SOT_UNDO_PROP_NUMBER: {
            if (e->propertyIndex >= 0 && e->propertyIndex < a->propertyCount) {
                float cur = a->properties[e->propertyIndex].value.number;
                a->properties[e->propertyIndex].value.number = e->old.number;
                e->old.number = cur;
            }
            break;
        }
        case SOT_UNDO_PROP_BOOL: {
            if (e->propertyIndex >= 0 && e->propertyIndex < a->propertyCount) {
                bool cur = a->properties[e->propertyIndex].value.boolean;
                a->properties[e->propertyIndex].value.boolean = e->old.boolean;
                e->old.boolean = cur;
            }
            break;
        }
    }
}

static void DoRedo(SOT_Editor *editor, SOT_Scene *scene)
{
    if (editor->undoTop >= editor->undoCount) return;
    // Redo is just undo again (we swapped old/current in DoUndo)
    SOT_UndoEntry *e = &editor->undoStack[editor->undoTop];
    editor->undoTop++;

    if (e->actorIndex < 0 || e->actorIndex >= scene->actorsCount) return;
    SOT_Actor *a = &scene->actors[e->actorIndex];

    switch (e->type) {
        case SOT_UNDO_POSITION: {
            float cur[2] = { a->transform.position[0], a->transform.position[1] };
            vec2 newPos = { e->old.position[0], e->old.position[1] };
            SetPosition(a, newPos);
            e->old.position[0] = cur[0];
            e->old.position[1] = cur[1];
            break;
        }
        case SOT_UNDO_SCALE: {
            float cur[2] = { a->transform.scale[0], a->transform.scale[1] };
            a->transform.scale[0] = e->old.scale[0];
            a->transform.scale[1] = e->old.scale[1];
            e->old.scale[0] = cur[0];
            e->old.scale[1] = cur[1];
            break;
        }
        case SOT_UNDO_ENABLED: {
            bool cur = a->enabled;
            SOT_Actor_SetEnabled(a, e->old.enabled);
            e->old.enabled = cur;
            break;
        }
        case SOT_UNDO_PROP_NUMBER: {
            if (e->propertyIndex >= 0 && e->propertyIndex < a->propertyCount) {
                float cur = a->properties[e->propertyIndex].value.number;
                a->properties[e->propertyIndex].value.number = e->old.number;
                e->old.number = cur;
            }
            break;
        }
        case SOT_UNDO_PROP_BOOL: {
            if (e->propertyIndex >= 0 && e->propertyIndex < a->propertyCount) {
                bool cur = a->properties[e->propertyIndex].value.boolean;
                a->properties[e->propertyIndex].value.boolean = e->old.boolean;
                e->old.boolean = cur;
            }
            break;
        }
    }
}

// ---- Animator Inspector ----

static void DrawAnimatorInspector(SOT_Editor *editor)
{
    SOT_AnimatorEditor *anim = &editor->animator;
    SOT_AnimationInfo *info = anim->animInfo;
    if (!info) return;

    // ---- File Properties ----
    if (igCollapsingHeader_TreeNodeFlags("File Properties", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Atlas name (read-only display for now, editable in Phase 4)
        igText("Atlas: %s", info->atlasName ? info->atlasName : "(none)");
        igText("Image: %s", info->atlasPath ? info->atlasPath : "(none)");
        igText("Collider: %s", info->collider ? info->collider : "(none)");

        int stepMs = (int)info->step_ms;
        if (igDragInt("Default Step (ms)", &stepMs, 1, 1, 1000, "%d", 0)) {
            info->step_ms = (uint16_t)stepMs;
            anim->dirty = true;
        }

        igSeparator();

        // Sprite size config
        bool gridChanged = false;
        if (igDragInt("Sprite Width", &anim->spriteWidth, 1, 1, 512, "%d", 0)) gridChanged = true;
        if (igDragInt("Sprite Height", &anim->spriteHeight, 1, 1, 512, "%d", 0)) gridChanged = true;
        if (gridChanged) {
            anim->gridCols = (anim->atlasWidth > 0 && anim->spriteWidth > 0) ? anim->atlasWidth / anim->spriteWidth : 1;
            anim->gridRows = (anim->atlasHeight > 0 && anim->spriteHeight > 0) ? anim->atlasHeight / anim->spriteHeight : 1;
            // Clear cell selection since grid changed
            SDL_memset(anim->cellSelection, 0, sizeof(anim->cellSelection));
        }

        igText("Grid: %d x %d cells", anim->gridCols, anim->gridRows);
    }

    igSeparator();

    // ---- Sequences List ----
    if (igCollapsingHeader_TreeNodeFlags("Sequences", ImGuiTreeNodeFlags_DefaultOpen)) {
        igText("%d sequences", info->count);

        // Add Sequence button
        if (igSmallButton("+ Add Sequence")) {
            anim->showNewSequencePopup = true;
            anim->newSequenceName[0] = '\0';
        }

        // New sequence popup
        if (anim->showNewSequencePopup) {
            igOpenPopup_Str("NewSequence", 0);
            anim->showNewSequencePopup = false;
        }
        if (igBeginPopup("NewSequence", 0)) {
            igText("New Sequence");
            igInputText("Name", anim->newSequenceName, sizeof(anim->newSequenceName), 0, NULL, NULL);
            if (igSmallButton("Create") && anim->newSequenceName[0] != '\0') {
                // Reallocate sequences array
                int newCount = info->count + 1;
                SOT_AnimationSequence *newSeqs = (SOT_AnimationSequence *)SDL_realloc(
                    info->sequences, newCount * sizeof(SOT_AnimationSequence));
                if (newSeqs) {
                    info->sequences = newSeqs;
                    SOT_AnimationSequence *seq = &info->sequences[info->count];
                    SDL_memset(seq, 0, sizeof(SOT_AnimationSequence));
                    int sl = (int)SDL_strlen(anim->newSequenceName) + 1;
                    seq->name = (char *)SDL_malloc(sl);
                    SDL_strlcpy(seq->name, anim->newSequenceName, sl);
                    seq->playMode = SOT_PLAY_LOOP;
                    seq->count = 0;
                    seq->frames = NULL;
                    seq->frameDefs = NULL;
                    anim->selectedSequence = info->count;
                    info->count = newCount;
                    anim->dirty = true;
                }
                igCloseCurrentPopup();
            }
            igSameLine(0, 4);
            if (igSmallButton("Cancel")) {
                igCloseCurrentPopup();
            }
            igEndPopup();
        }

        igSeparator();

        // Sequence list
        for (int i = 0; i < info->count; i++) {
            SOT_AnimationSequence *seq = &info->sequences[i];
            char label[128];
            SDL_snprintf(label, sizeof(label), "%s (%d frames)###seq%d",
                seq->name ? seq->name : "(unnamed)", seq->count, i);
            bool selected = (anim->selectedSequence == i);
            if (igSelectable_Bool(label, selected, 0, (ImVec2_c){0, 0})) {
                anim->selectedSequence = i;
                anim->selectedFrame = -1;
            }
        }
    }

    igSeparator();

    // ---- Selected Sequence Details ----
    if (anim->selectedSequence >= 0 && anim->selectedSequence < info->count) {
        SOT_AnimationSequence *seq = &info->sequences[anim->selectedSequence];

        if (igCollapsingHeader_TreeNodeFlags("Sequence Details", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Name editing
            static char nameEditBuf[64];
            if (seq->name) SDL_strlcpy(nameEditBuf, seq->name, sizeof(nameEditBuf));
            else nameEditBuf[0] = '\0';
            if (igInputText("Name##seqname", nameEditBuf, sizeof(nameEditBuf), ImGuiInputTextFlags_EnterReturnsTrue, NULL, NULL)) {
                if (seq->name) SDL_free(seq->name);
                int sl = (int)SDL_strlen(nameEditBuf) + 1;
                seq->name = (char *)SDL_malloc(sl);
                SDL_strlcpy(seq->name, nameEditBuf, sl);
                anim->dirty = true;
            }

            // Play mode
            const char *playModes[] = { "Loop", "Once", "Ping Pong", "Once Destroy" };
            int pm = (int)seq->playMode;
            if (igCombo_Str_arr("Play Mode", &pm, playModes, 4, 0)) {
                seq->playMode = (SOT_PlayMode)pm;
                anim->dirty = true;
            }

            igText("Frames: %d", seq->count);

            // Duplicate sequence
            igSameLine(0, 10);
            if (igSmallButton("Duplicate")) {
                int newCount = info->count + 1;
                SOT_AnimationSequence *newSeqs = (SOT_AnimationSequence *)SDL_realloc(
                    info->sequences, newCount * sizeof(SOT_AnimationSequence));
                if (newSeqs) {
                    info->sequences = newSeqs;
                    // Re-grab seq pointer after realloc
                    seq = &info->sequences[anim->selectedSequence];
                    SOT_AnimationSequence *dup = &info->sequences[info->count];
                    SDL_memset(dup, 0, sizeof(SOT_AnimationSequence));
                    // Copy name with " (copy)" suffix
                    char dupName[128];
                    SDL_snprintf(dupName, sizeof(dupName), "%s (copy)", seq->name ? seq->name : "unnamed");
                    int sl = (int)SDL_strlen(dupName) + 1;
                    dup->name = (char *)SDL_malloc(sl);
                    SDL_strlcpy(dup->name, dupName, sl);
                    dup->playMode = seq->playMode;
                    dup->count = seq->count;
                    if (seq->count > 0) {
                        dup->frames = (vec4 *)SDL_malloc(seq->count * sizeof(vec4));
                        dup->frameDefs = (SOT_FrameDef *)SDL_malloc(seq->count * sizeof(SOT_FrameDef));
                        SDL_memcpy(dup->frames, seq->frames, seq->count * sizeof(vec4));
                        SDL_memcpy(dup->frameDefs, seq->frameDefs, seq->count * sizeof(SOT_FrameDef));
                    }
                    info->count = newCount;
                    anim->dirty = true;
                }
            }

            // Delete sequence
            igSameLine(0, 4);
            if (igSmallButton("Delete##seq")) {
                // Free the sequence data
                if (seq->name) SDL_free(seq->name);
                if (seq->frames) SDL_free(seq->frames);
                if (seq->frameDefs) SDL_free(seq->frameDefs);
                // Shift remaining sequences
                for (int j = anim->selectedSequence; j < info->count - 1; j++) {
                    info->sequences[j] = info->sequences[j + 1];
                }
                info->count--;
                if (anim->selectedSequence >= info->count) {
                    anim->selectedSequence = info->count - 1;
                }
                anim->selectedFrame = -1;
                anim->dirty = true;
                // Don't access seq after deletion
                goto end_seq_details;
            }

            // Add frames from cell selection
            {
                int selCount = 0;
                for (int idx = 0; idx < anim->gridCols * anim->gridRows && idx < 256; idx++) {
                    if (anim->cellSelection[idx]) selCount++;
                }
                if (selCount > 0) {
                    char addLabel[64];
                    SDL_snprintf(addLabel, sizeof(addLabel), "Add %d Frames from Selection", selCount);
                    if (igButton(addLabel, (ImVec2_c){0, 0})) {
                        // Expand frames arrays
                        int newTotal = seq->count + selCount;
                        vec4 *newFrames = (vec4 *)SDL_realloc(seq->frames, newTotal * sizeof(vec4));
                        SOT_FrameDef *newDefs = (SOT_FrameDef *)SDL_realloc(seq->frameDefs, newTotal * sizeof(SOT_FrameDef));
                        if (newFrames && newDefs) {
                            seq->frames = newFrames;
                            seq->frameDefs = newDefs;
                            int addIdx = seq->count;
                            for (int idx = 0; idx < anim->gridCols * anim->gridRows && idx < 256; idx++) {
                                if (!anim->cellSelection[idx]) continue;
                                int col = idx % anim->gridCols;
                                int row = idx / anim->gridCols;
                                float fx = (float)(col * anim->spriteWidth);
                                float fy = (float)(row * anim->spriteHeight);
                                float fw = (float)anim->spriteWidth;
                                float fh = (float)anim->spriteHeight;
                                seq->frames[addIdx][0] = fx;
                                seq->frames[addIdx][1] = fy;
                                seq->frames[addIdx][2] = fw;
                                seq->frames[addIdx][3] = fh;
                                SDL_memset(&seq->frameDefs[addIdx], 0, sizeof(SOT_FrameDef));
                                seq->frameDefs[addIdx].rect[0] = fx;
                                seq->frameDefs[addIdx].rect[1] = fy;
                                seq->frameDefs[addIdx].rect[2] = fw;
                                seq->frameDefs[addIdx].rect[3] = fh;
                                addIdx++;
                            }
                            seq->count = newTotal;
                            SDL_memset(anim->cellSelection, 0, sizeof(anim->cellSelection));
                            anim->dirty = true;
                        }
                    }
                }
            }

            igSeparator();

            // Frame list
            for (int f = 0; f < seq->count; f++) {
                igPushID_Int(f);
                char frameLabel[64];
                SDL_snprintf(frameLabel, sizeof(frameLabel), "Frame %d: (%d,%d %dx%d)",
                    f, (int)seq->frames[f][0], (int)seq->frames[f][1],
                    (int)seq->frames[f][2], (int)seq->frames[f][3]);

                bool fSelected = (anim->selectedFrame == f);
                if (igSelectable_Bool(frameLabel, fSelected, 0, (ImVec2_c){0, 0})) {
                    anim->selectedFrame = f;
                }

                // Reorder buttons
                igSameLine(0, 4);
                if (f > 0 && igSmallButton("^##up")) {
                    // Swap with previous
                    vec4 tmpF; SDL_memcpy(tmpF, seq->frames[f - 1], sizeof(vec4));
                    SDL_memcpy(seq->frames[f - 1], seq->frames[f], sizeof(vec4));
                    SDL_memcpy(seq->frames[f], tmpF, sizeof(vec4));
                    SOT_FrameDef tmpD = seq->frameDefs[f - 1];
                    seq->frameDefs[f - 1] = seq->frameDefs[f];
                    seq->frameDefs[f] = tmpD;
                    if (anim->selectedFrame == f) anim->selectedFrame = f - 1;
                    anim->dirty = true;
                }
                igSameLine(0, 2);
                if (f < seq->count - 1 && igSmallButton("v##down")) {
                    vec4 tmpF; SDL_memcpy(tmpF, seq->frames[f + 1], sizeof(vec4));
                    SDL_memcpy(seq->frames[f + 1], seq->frames[f], sizeof(vec4));
                    SDL_memcpy(seq->frames[f], tmpF, sizeof(vec4));
                    SOT_FrameDef tmpD = seq->frameDefs[f + 1];
                    seq->frameDefs[f + 1] = seq->frameDefs[f];
                    seq->frameDefs[f] = tmpD;
                    if (anim->selectedFrame == f) anim->selectedFrame = f + 1;
                    anim->dirty = true;
                }
                igSameLine(0, 2);
                if (igSmallButton("X##del")) {
                    // Remove frame by shifting
                    for (int k = f; k < seq->count - 1; k++) {
                        SDL_memcpy(seq->frames[k], seq->frames[k + 1], sizeof(vec4));
                        seq->frameDefs[k] = seq->frameDefs[k + 1];
                    }
                    seq->count--;
                    if (anim->selectedFrame >= seq->count) anim->selectedFrame = seq->count - 1;
                    anim->dirty = true;
                    igPopID();
                    break;  // list changed, stop iterating
                }

                // Per-frame details when selected
                if (anim->selectedFrame == f) {
                    igIndent(10);
                    int durMs = (int)seq->frameDefs[f].duration_ms;
                    if (igDragInt("Duration (ms)##dur", &durMs, 1, 0, 10000, "%d", 0)) {
                        seq->frameDefs[f].duration_ms = (uint16_t)durMs;
                        // Sync legacy frame rect
                        seq->frameDefs[f].rect[0] = seq->frames[f][0];
                        seq->frameDefs[f].rect[1] = seq->frames[f][1];
                        seq->frameDefs[f].rect[2] = seq->frames[f][2];
                        seq->frameDefs[f].rect[3] = seq->frames[f][3];
                        anim->dirty = true;
                    }
                    igText("(0 = use default %d ms)", info->step_ms);

                    // Frame events
                    igText("Events (%d/%d):", seq->frameDefs[f].eventCount, SOT_MAX_FRAME_EVENTS);
                    for (int e = 0; e < seq->frameDefs[f].eventCount; e++) {
                        igPushID_Int(1000 + e);
                        char evtLabel[48];
                        SDL_snprintf(evtLabel, sizeof(evtLabel), "##evt%d", e);
                        if (igInputText(evtLabel, seq->frameDefs[f].events[e].name,
                                sizeof(seq->frameDefs[f].events[e].name), 0, NULL, NULL)) {
                            anim->dirty = true;
                        }
                        igSameLine(0, 4);
                        if (igSmallButton("X##evtdel")) {
                            // Remove event by shifting
                            for (int k = e; k < seq->frameDefs[f].eventCount - 1; k++) {
                                seq->frameDefs[f].events[k] = seq->frameDefs[f].events[k + 1];
                            }
                            seq->frameDefs[f].eventCount--;
                            anim->dirty = true;
                            igPopID();
                            break;
                        }
                        igPopID();
                    }
                    if (seq->frameDefs[f].eventCount < SOT_MAX_FRAME_EVENTS) {
                        if (igSmallButton("+ Event")) {
                            int ei = seq->frameDefs[f].eventCount;
                            seq->frameDefs[f].events[ei].name[0] = '\0';
                            seq->frameDefs[f].eventCount++;
                            anim->dirty = true;
                        }
                    }
                    igUnindent(10);
                }

                igPopID();
            }
        }
    }
end_seq_details:

    // ---- Preview ----
    if (anim->selectedSequence >= 0 && anim->selectedSequence < info->count) {
        SOT_AnimationSequence *seq = &info->sequences[anim->selectedSequence];
        if (seq->count > 0 && igCollapsingHeader_TreeNodeFlags("Preview", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Playback controls
            if (igSmallButton(anim->previewPlaying ? "Pause" : "Play##prev")) {
                anim->previewPlaying = !anim->previewPlaying;
                if (anim->previewPlaying) {
                    anim->previewLastTick = SDL_GetTicks();
                }
            }
            igSameLine(0, 4);
            if (igSmallButton("|<##prev")) { anim->previewFrame = 0; anim->previewElapsedMs = 0; }
            igSameLine(0, 2);
            if (igSmallButton("<##prev")) {
                if (anim->previewFrame > 0) anim->previewFrame--;
                anim->previewElapsedMs = 0;
            }
            igSameLine(0, 2);
            if (igSmallButton(">##prev")) {
                if (anim->previewFrame < seq->count - 1) anim->previewFrame++;
                anim->previewElapsedMs = 0;
            }
            igSameLine(0, 2);
            if (igSmallButton(">|##prev")) {
                anim->previewFrame = seq->count - 1;
                anim->previewElapsedMs = 0;
            }

            // Frame scrubber
            int frame = anim->previewFrame;
            if (frame >= seq->count) frame = seq->count - 1;
            if (igSliderInt("Frame##prev", &frame, 0, seq->count - 1, "%d", 0)) {
                anim->previewFrame = frame;
                anim->previewElapsedMs = 0;
            }

            // Advance playback
            if (anim->previewPlaying) {
                uint32_t now = SDL_GetTicks();
                uint32_t delta = now - anim->previewLastTick;
                anim->previewLastTick = now;
                anim->previewElapsedMs += delta;

                uint16_t frameDur = seq->frameDefs[anim->previewFrame].duration_ms;
                if (frameDur == 0) frameDur = info->step_ms;
                if (frameDur == 0) frameDur = 75;

                if (anim->previewElapsedMs >= frameDur) {
                    anim->previewElapsedMs = 0;
                    anim->previewFrame++;
                    if (anim->previewFrame >= seq->count) {
                        anim->previewFrame = 0;  // loop
                    }
                }
            }

            // Display current preview frame from atlas
            if (anim->previewFrame < seq->count && anim->atlasTexture) {
                float fx = seq->frames[anim->previewFrame][0];
                float fy = seq->frames[anim->previewFrame][1];
                float fw = seq->frames[anim->previewFrame][2];
                float fh = seq->frames[anim->previewFrame][3];
                float aw = (float)anim->atlasWidth;
                float ah = (float)anim->atlasHeight;

                if (aw > 0 && ah > 0) {
                    ImDrawList *dl = igGetWindowDrawList();
                    ImDrawList_AddCallback(dl, SetSamplerNearest, NULL, 0);

                    ImTextureRef_c texRef;
                    texRef._TexData = NULL;
                    texRef._TexID = (ImTextureID)(uintptr_t)anim->atlasTexture;

                    float scale = 4.0f;
                    ImVec2_c imgSz = { fw * scale, fh * scale };
                    ImVec2_c uvMin = { fx / aw, fy / ah };
                    ImVec2_c uvMax = { (fx + fw) / aw, (fy + fh) / ah };
                    igImage(texRef, imgSz, uvMin, uvMax);

                    ImDrawList_AddCallback(dl, SetSamplerLinear, NULL, 0);
                }
            }
        }
    }
}

static void DrawPropertyInspector(SOT_Editor *editor, SOT_Scene *scene)
{
    igBegin("Inspector", NULL, 0);

    // Animator editor mode: show animation file properties instead
    if (editor->showAnimatorEditor && editor->animator.active) {
        DrawAnimatorInspector(editor);
        igEnd();
        return;
    }

    // Camera inspector
    if (editor->selectedActor == SOT_SELECTED_CAMERA) {
        igText("[Camera]");
        igSeparator();

        // Position
        if (igCollapsingHeader_BoolPtr("Position", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
            float camPos[2] = { scene->worldCamera.cameraInfo.eye[0],
                                scene->worldCamera.cameraInfo.eye[1] };
            if (igDragFloat2("Pos", camPos, 1.0f, 0, 0, "%.1f", 0)) {
                float dx = camPos[0] - scene->worldCamera.cameraInfo.eye[0];
                float dy = camPos[1] - scene->worldCamera.cameraInfo.eye[1];
                scene->worldCamera.cameraInfo.eye[0] = camPos[0];
                scene->worldCamera.cameraInfo.eye[1] = camPos[1];
                scene->worldCamera.cameraInfo.center[0] += dx;
                scene->worldCamera.cameraInfo.center[1] += dy;
                glm_lookat(scene->worldCamera.cameraInfo.eye,
                           scene->worldCamera.cameraInfo.center,
                           scene->worldCamera.cameraInfo.up,
                           scene->worldCamera.view);
                glm_mat4_mul(scene->worldCamera.projection, scene->worldCamera.view,
                             scene->worldCamera.pvMatrix);
            }
        }

        igSeparator();

        // Follow mode
        if (igCollapsingHeader_BoolPtr("Follow Mode", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
            static const char *modeNames[] = { "Free", "Follow (Deadzone)", "Auto Scroll", "Room Snap" };
            int mode = (int)scene->cameraFollow.mode;
            if (igCombo_Str_arr("Mode", &mode, modeNames, 4, 0)) {
                scene->cameraFollow.mode = (SOT_CameraFollowMode)mode;
            }

            if (scene->cameraFollow.mode == SOT_CAM_FOLLOW_DEADZONE) {
                // Target actor dropdown
                const char *targetName = "(none)";
                if (scene->cameraTargetActor >= 0 && scene->cameraTargetActor < scene->actorsCount
                    && scene->actors[scene->cameraTargetActor].actorName[0])
                    targetName = scene->actors[scene->cameraTargetActor].actorName;

                if (igBeginCombo("Target Actor", targetName, 0)) {
                    if (igSelectable_Bool("(none)", scene->cameraTargetActor < 0, 0, (ImVec2_c){0,0}))
                        scene->cameraTargetActor = -1;
                    for (int i = 0; i < scene->actorsCount; i++) {
                        char name[128];
                        SDL_snprintf(name, sizeof(name), "%s [%d]",
                            scene->actors[i].actorName[0] ? scene->actors[i].actorName : "(unnamed)", i);
                        bool sel = (scene->cameraTargetActor == i);
                        if (igSelectable_Bool(name, sel, 0, (ImVec2_c){0,0}))
                            scene->cameraTargetActor = i;
                    }
                    igEndCombo();
                }

                igDragFloat("Deadzone X", &scene->cameraFollow.deadZoneX, 1.0f, 0, 200, "%.0f", 0);
                igDragFloat("Deadzone Y", &scene->cameraFollow.deadZoneY, 1.0f, 0, 200, "%.0f", 0);
                igDragFloat("Smooth Speed", &scene->cameraFollow.smoothSpeed, 0.1f, 0.1f, 50.0f, "%.1f", 0);
            }

            if (scene->cameraFollow.mode == SOT_CAM_AUTO_SCROLL) {
                igDragFloat("Scroll X", &scene->cameraFollow.scrollSpeedX, 1.0f, -500, 500, "%.0f", 0);
                igDragFloat("Scroll Y", &scene->cameraFollow.scrollSpeedY, 1.0f, -500, 500, "%.0f", 0);
            }

            if (scene->cameraFollow.mode == SOT_CAM_ROOM_SNAP) {
                igDragFloat("Room Width", &scene->cameraFollow.roomWidth, 1.0f, 0, 10000, "%.0f", 0);
                igDragFloat("Room Height", &scene->cameraFollow.roomHeight, 1.0f, 0, 10000, "%.0f", 0);
            }
        }

        igSeparator();

        // Bounds
        if (igCollapsingHeader_BoolPtr("Bounds", NULL, 0)) {
            igCheckbox("Enable Bounds", &scene->cameraFollow.hasBounds);
            if (scene->cameraFollow.hasBounds) {
                igDragFloat("Min X", &scene->cameraFollow.boundsMinX, 1.0f, 0, 0, "%.0f", 0);
                igDragFloat("Min Y", &scene->cameraFollow.boundsMinY, 1.0f, 0, 0, "%.0f", 0);
                igDragFloat("Max X", &scene->cameraFollow.boundsMaxX, 1.0f, 0, 0, "%.0f", 0);
                igDragFloat("Max Y", &scene->cameraFollow.boundsMaxY, 1.0f, 0, 0, "%.0f", 0);
            }
        }

        igEnd();
        return;
    }

    if (editor->selectedActor < 0 || editor->selectedActor >= scene->actorsCount) {
        igText("No actor selected");
        igEnd();
        return;
    }

    SOT_Actor *actor = &scene->actors[editor->selectedActor];

    // Load/unload template when selection changes
    if (editor->selectedActor != editor->lastInspectedActor) {
        // Auto-save dirty template before switching
        if (editor->templateDirty && editor->editingTemplate && editor->lastInspectedActor >= 0
            && editor->lastInspectedActor < scene->actorsCount) {
            const char *oldTmplName = scene->actors[editor->lastInspectedActor].templateName;
            if (oldTmplName[0]) {
                SOT_SaveActorTemplate(editor->editingTemplate, oldTmplName);
                SOT_Editor_Log(editor, "Auto-saved template: %s", oldTmplName);
            }
        }
        if (editor->editingTemplate) {
            SOT_FreeActorTemplate(editor->editingTemplate);
            editor->editingTemplate = NULL;
        }
        editor->templateDirty = false;
        if (actor->templateName[0])
            editor->editingTemplate = SOT_LoadActorTemplate(actor->templateName);
        editor->lastInspectedActor = editor->selectedActor;
    }

    igText("Name: %s", actor->actorName[0] ? actor->actorName : "(unnamed)");
    igText("Template: %s", actor->templateName[0] ? actor->templateName : "(none)");
    igText("ID: %d", actor->actorID);
    if (editor->templateDirty) {
        igSameLine(0, 8);
        igTextDisabled("(modified)");
    }

    bool enabled = actor->enabled;
    if (igCheckbox("Enabled", &enabled)) {
        PushUndo(editor, (SOT_UndoEntry){
            .type = SOT_UNDO_ENABLED,
            .actorIndex = editor->selectedActor,
            .old.enabled = actor->enabled
        });
        SOT_Actor_SetEnabled(actor, enabled);
    }

    igSeparator();

    // Transform
    if (igCollapsingHeader_BoolPtr("Transform", NULL, ImGuiTreeNodeFlags_DefaultOpen)) {
        float pos[2] = { actor->transform.position[0], actor->transform.position[1] };
        if (igIsItemActivated()) {
            // Store pre-edit value on first activation
        }
        if (igDragFloat2("Position", pos, 1.0f, 0, 0, "%.1f", 0)) {
            // Push undo only on first change (when widget becomes active)
            if (igIsItemActivated()) {
                PushUndo(editor, (SOT_UndoEntry){
                    .type = SOT_UNDO_POSITION,
                    .actorIndex = editor->selectedActor,
                    .old.position = { actor->transform.position[0], actor->transform.position[1] }
                });
            }
            vec2 newPos = { pos[0], pos[1] };
            SetPosition(actor, newPos);
        }

        float scale[2] = { actor->transform.scale[0], actor->transform.scale[1] };
        if (igDragFloat2("Scale", scale, 0.1f, 0.01f, 100.0f, "%.2f", 0)) {
            if (igIsItemActivated()) {
                PushUndo(editor, (SOT_UndoEntry){
                    .type = SOT_UNDO_SCALE,
                    .actorIndex = editor->selectedActor,
                    .old.scale = { actor->transform.scale[0], actor->transform.scale[1] }
                });
            }
            actor->transform.scale[0] = scale[0];
            actor->transform.scale[1] = scale[1];
        }
    }

    // Animation
    if (igCollapsingHeader_BoolPtr("Animation", NULL, 0)) {
        // Animation file dropdown (template editing)
        if (editor->editingTemplate) {
            const char *currentAnimFile = editor->editingTemplate->animationFile;
            const char *preview = currentAnimFile[0] ? currentAnimFile : "(none)";
            if (igBeginCombo("Anim File", preview, 0)) {
                // "(none)" option
                if (igSelectable_Bool("(none)", !currentAnimFile[0], 0, (ImVec2_c){0, 0})) {
                    editor->editingTemplate->animationFile[0] = '\0';
                    editor->templateDirty = true;
                }
                // List animation .json files
                char animDir[512];
                SDL_snprintf(animDir, sizeof(animDir), "%s", Paths.Animations);
                int aCount = 0;
                char **aFiles = SDL_GlobDirectory(animDir, "*.json", 0, &aCount);
                if (aFiles) {
                    for (int i = 0; i < aCount; i++) {
                        bool sel = SDL_strcmp(aFiles[i], currentAnimFile) == 0;
                        if (igSelectable_Bool(aFiles[i], sel, 0, (ImVec2_c){0, 0})) {
                            SDL_strlcpy(editor->editingTemplate->animationFile, aFiles[i],
                                        sizeof(editor->editingTemplate->animationFile));
                            // Reload animation on the actor
                            actor->animationInfoCount = 0;
                            actor->animationsCount = 0;
                            actor->currentAnimation = 0;
                            SOT_ActorLoadAnimationFile(actor, aFiles[i]);
                            editor->templateDirty = true;
                        }
                    }
                    SDL_free(aFiles);
                }
                igEndCombo();
            }
        }

        // Read-only animation info
        igText("Animations: %d", actor->animationsCount);
        igText("Current: %d", actor->currentAnimation);
        if (actor->currentState[0])
            igText("State: %s", actor->currentState);

        if (actor->animationsCount > 0 && actor->currentAnimation < actor->animationsCount) {
            SOT_Animation *anim = &actor->animations[actor->currentAnimation];
            if (anim->info) {
                SOT_AnimationSequence *seq = &anim->info->sequences[anim->sequenceIndex];
                igText("Sequence: %s", seq->name ? seq->name : "(unnamed)");
                igText("Frame: %d / %d", anim->currentFrame, seq->count);
                igText("Playing: %s", anim->finished ? "No" : "Yes");
            }
        }
    }

    // Physics (template editing)
    if (editor->editingTemplate && igCollapsingHeader_BoolPtr("Physics", NULL, 0)) {
        SOT_ActorTemplate *t = editor->editingTemplate;

        static const char *bodyTypeNames[] = { "None", "Static", "Dynamic", "Kinematic" };
        int bt = (int)t->bodyType;
        if (igCombo_Str_arr("Body Type", &bt, bodyTypeNames, 4, 0)) {
            t->bodyType = (SOT_BodyType)bt;
            editor->templateDirty = true;
        }

        if (t->bodyType != SOT_BODY_NONE) {
            bool fr = t->fixedRotation;
            if (igCheckbox("Fixed Rotation", &fr)) {
                t->fixedRotation = fr;
                editor->templateDirty = true;
            }
            if (igDragFloat("Gravity Scale", &t->gravityScale, 0.1f, 0, 10, "%.1f", 0)) {
                editor->templateDirty = true;
            }
        }
    }

    // Collider (template editing)
    if (editor->editingTemplate && igCollapsingHeader_BoolPtr("Collider", NULL, 0)) {
        SOT_ActorTemplate *t = editor->editingTemplate;

        static const char *colliderTypes[] = { "none", "box", "circle" };
        int cIdx = 0;
        if (SDL_strcmp(t->colliderType, "box") == 0) cIdx = 1;
        else if (SDL_strcmp(t->colliderType, "circle") == 0) cIdx = 2;

        if (igCombo_Str_arr("Shape", &cIdx, colliderTypes, 3, 0)) {
            SDL_strlcpy(t->colliderType, colliderTypes[cIdx], sizeof(t->colliderType));
            editor->templateDirty = true;
        }

        if (cIdx == 1) {
            if (igDragFloat("Half Width", &t->colliderHalfW, 0.5f, 0, 100, "%.1f", 0))
                editor->templateDirty = true;
            if (igDragFloat("Half Height", &t->colliderHalfH, 0.5f, 0, 100, "%.1f", 0))
                editor->templateDirty = true;
        } else if (cIdx == 2) {
            if (igDragFloat("Radius", &t->colliderRadius, 0.5f, 0, 100, "%.1f", 0))
                editor->templateDirty = true;
        }

        if (cIdx > 0) {
            if (igDragFloat("Offset X", &t->colliderOffsetX, 0.5f, -100, 100, "%.1f", 0))
                editor->templateDirty = true;
            if (igDragFloat("Offset Y", &t->colliderOffsetY, 0.5f, -100, 100, "%.1f", 0))
                editor->templateDirty = true;
            bool sensor = t->colliderIsSensor;
            if (igCheckbox("Is Sensor", &sensor)) {
                t->colliderIsSensor = sensor;
                editor->templateDirty = true;
            }
        }
    }

    // States (template editing)
    if (editor->editingTemplate && igCollapsingHeader_BoolPtr("States", NULL, 0)) {
        SOT_ActorTemplate *t = editor->editingTemplate;

        for (int i = 0; i < t->stateCount; i++) {
            igPushID_Int(i);

            igSetNextItemWidth(100);
            if (igInputText("##sname", t->states[i].stateName, 64, 0, NULL, NULL))
                editor->templateDirty = true;

            igSameLine(0, 4);
            igSetNextItemWidth(120);
            // Animation sequence dropdown
            if (igBeginCombo("##sanim", t->states[i].animationName[0] ? t->states[i].animationName : "(select)", 0)) {
                for (int j = 0; j < actor->animationInfoCount; j++) {
                    SOT_AnimationInfo *info = actor->animationInfos[j];
                    if (!info) continue;
                    for (int k = 0; k < info->count; k++) {
                        if (!info->sequences[k].name) continue;
                        bool sel = SDL_strcmp(t->states[i].animationName, info->sequences[k].name) == 0;
                        if (igSelectable_Bool(info->sequences[k].name, sel, 0, (ImVec2_c){0, 0})) {
                            SDL_strlcpy(t->states[i].animationName, info->sequences[k].name, 64);
                            editor->templateDirty = true;
                        }
                    }
                }
                igEndCombo();
            }

            igSameLine(0, 4);
            if (igSmallButton("X##del")) {
                for (int j = i; j < t->stateCount - 1; j++)
                    t->states[j] = t->states[j + 1];
                t->stateCount--;
                editor->templateDirty = true;
            }

            igPopID();
        }

        if (t->stateCount < SOT_ACTOR_MAX_STATES && igSmallButton("+ Add State")) {
            SOT_StateMapping *m = &t->states[t->stateCount];
            SDL_strlcpy(m->stateName, "new_state", sizeof(m->stateName));
            m->animationName[0] = '\0';
            t->stateCount++;
            editor->templateDirty = true;
        }
    }

    // Tags
    if (actor->tagCount > 0 && igCollapsingHeader_BoolPtr("Tags", NULL, 0)) {
        for (int i = 0; i < actor->tagCount; i++) {
            igBulletText("%s", actor->tags[i]);
        }
    }

    // Properties
    if (actor->propertyCount > 0 && igCollapsingHeader_BoolPtr("Properties", NULL, 0)) {
        for (int i = 0; i < actor->propertyCount; i++) {
            SOT_Property *prop = &actor->properties[i];
            switch (prop->type) {
                case SOT_PROP_NUMBER: {
                    float v = prop->value.number;
                    if (igDragFloat(prop->key, &v, 0.1f, 0, 0, "%.2f", 0)) {
                        if (igIsItemActivated()) {
                            PushUndo(editor, (SOT_UndoEntry){
                                .type = SOT_UNDO_PROP_NUMBER,
                                .actorIndex = editor->selectedActor,
                                .propertyIndex = i,
                                .old.number = prop->value.number
                            });
                        }
                        prop->value.number = v;
                    }
                    break;
                }
                case SOT_PROP_STRING:
                    igText("%s: %s", prop->key, prop->value.string);
                    break;
                case SOT_PROP_BOOL: {
                    bool b = prop->value.boolean;
                    if (igCheckbox(prop->key, &b)) {
                        PushUndo(editor, (SOT_UndoEntry){
                            .type = SOT_UNDO_PROP_BOOL,
                            .actorIndex = editor->selectedActor,
                            .propertyIndex = i,
                            .old.boolean = prop->value.boolean
                        });
                        prop->value.boolean = b;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }

    igEnd();
}

static ImVec4_c SeverityColor(SOT_LogSeverity sev)
{
    switch (sev) {
        case SOT_LOG_WARNING: return (ImVec4_c){ 1.0f, 0.9f, 0.3f, 1.0f };  // yellow
        case SOT_LOG_ERROR:   return (ImVec4_c){ 1.0f, 0.3f, 0.3f, 1.0f };  // red
        case SOT_LOG_LUA:     return (ImVec4_c){ 0.4f, 0.9f, 1.0f, 1.0f };  // cyan
        default:              return (ImVec4_c){ 1.0f, 1.0f, 1.0f, 1.0f };  // white
    }
}

static void DrawConsolePanel(SOT_Editor *editor, AppState *as)
{
    igBegin("Console", NULL, 0);

    // Reserve space for footer (input + clear button)
    float footerHeight = igGetStyle()->ItemSpacing.y + igGetFrameHeightWithSpacing() * 2;
    ImVec2_c childSize = { 0, -footerHeight };
    if (igBeginChild_Str("LogArea", childSize, 0, 0)) {
        for (int i = 0; i < editor->logCount; i++) {
            ImVec4_c color = SeverityColor(editor->logSeverity[i]);
            igPushStyleColor_Vec4(ImGuiCol_Text, color);
            igTextUnformatted(editor->logLines[i], NULL);
            igPopStyleColor(1);
        }
        if (igGetScrollY() >= igGetScrollMaxY())
            igSetScrollHereY(1.0f);
    }
    igEndChild();

    igSeparator();

    // Lua input line
    igPushItemWidth(-60);
    ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue;
    if (igInputText("##lua", editor->luaInputBuf, sizeof(editor->luaInputBuf), inputFlags, NULL, NULL)) {
        if (editor->luaInputBuf[0] != '\0') {
            SOT_Editor_LogLua(editor, "> %s", editor->luaInputBuf);
            char resultMsg[512] = {0};
            if (SOT_Lua_DoString(&as->lua, editor->luaInputBuf, resultMsg, sizeof(resultMsg))) {
                if (resultMsg[0])
                    SOT_Editor_LogLua(editor, "%s", resultMsg);
            } else {
                SOT_Editor_LogError(editor, "%s", resultMsg);
            }
            editor->luaInputBuf[0] = '\0';
        }
        igSetKeyboardFocusHere(-1);  // Keep focus on input
    }
    igPopItemWidth();

    igSameLine(0, 4);
    if (igSmallButton("Clear"))
        editor->logCount = 0;

    igEnd();
}

// ---- Animator Editor ----

static void SOT_AnimatorEditor_Close(SOT_AnimatorEditor *animator, SDL_GPUDevice *device)
{
    if (animator->atlasTexture) {
        SDL_ReleaseGPUTexture(device, animator->atlasTexture);
        animator->atlasTexture = NULL;
    }
    if (animator->animInfo) {
        SOT_FreeAnimationInfo(animator->animInfo);
        animator->animInfo = NULL;
    }
    animator->active = false;
    animator->dirty = false;
    animator->selectedSequence = -1;
    animator->selectedFrame = -1;
    animator->previewPlaying = false;
    animator->previewFrame = 0;
    SDL_memset(animator->cellSelection, 0, sizeof(animator->cellSelection));
}

static bool SOT_AnimatorEditor_Open(SOT_AnimatorEditor *animator, const char *filename, SOT_GPU_State *gpu)
{
    // Close any existing session
    if (animator->active) {
        SOT_AnimatorEditor_Close(animator, gpu->device);
    }

    // Load and parse animation JSON
    SOT_AnimationInfo *info = SOT_LoadAnimations((char *)filename);
    if (!info) {
        SDL_Log("AnimatorEditor: Failed to load %s", filename);
        return false;
    }

    animator->animInfo = info;
    SDL_strlcpy(animator->sourceFilename, filename, sizeof(animator->sourceFilename));

    // Load atlas texture for ImGui display
    // The image_path in the JSON is relative to textures dir (e.g. "textures/monkey-sheet-16.png")
    // But GetSurfaceFromImage expects just the filename relative to Paths.Textures
    // The atlasPath stores something like "textures/monkey-sheet-16.png", we need just the filename part
    char *textureName = info->atlasPath;
    // Strip "textures/" prefix if present
    if (textureName && SDL_strncmp(textureName, "textures/", 9) == 0) {
        textureName = textureName + 9;
    }
    // Also try with backslash
    if (textureName && SDL_strncmp(textureName, "textures\\", 9) == 0) {
        textureName = textureName + 9;
    }

    SDL_Surface *surface = NULL;
    if (GetSurfaceFromImage(&surface, textureName) != SDL_APP_CONTINUE || !surface) {
        SDL_Log("AnimatorEditor: Failed to load atlas image %s", textureName);
        SOT_AnimatorEditor_Close(animator, gpu->device);
        return false;
    }

    animator->atlasWidth = surface->w;
    animator->atlasHeight = surface->h;

    // Create GPU texture and upload
    animator->atlasTexture = SDL_CreateGPUTexture(gpu->device, &(SDL_GPUTextureCreateInfo) {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .width = surface->w,
        .height = surface->h,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
    });

    if (!animator->atlasTexture) {
        SDL_Log("AnimatorEditor: Failed to create GPU texture");
        SDL_DestroySurface(surface);
        SOT_AnimatorEditor_Close(animator, gpu->device);
        return false;
    }

    // Upload surface pixels to GPU texture via transfer buffer
    Uint32 dataSize = surface->w * surface->h * 4;
    SDL_GPUTransferBuffer *transferBuf = SDL_CreateGPUTransferBuffer(gpu->device, &(SDL_GPUTransferBufferCreateInfo) {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = dataSize
    });

    if (transferBuf) {
        void *mapped = SDL_MapGPUTransferBuffer(gpu->device, transferBuf, false);
        if (mapped) {
            SDL_memcpy(mapped, surface->pixels, dataSize);
            SDL_UnmapGPUTransferBuffer(gpu->device, transferBuf);
        }

        SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(gpu->device);
        if (cmdbuf) {
            SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(cmdbuf);
            SDL_UploadToGPUTexture(
                copyPass,
                &(SDL_GPUTextureTransferInfo) {
                    .transfer_buffer = transferBuf,
                    .offset = 0
                },
                &(SDL_GPUTextureRegion) {
                    .texture = animator->atlasTexture,
                    .w = surface->w,
                    .h = surface->h,
                    .d = 1
                },
                false
            );
            SDL_EndGPUCopyPass(copyPass);
            SDL_SubmitGPUCommandBuffer(cmdbuf);
        }

        SDL_ReleaseGPUTransferBuffer(gpu->device, transferBuf);
    }

    SDL_DestroySurface(surface);

    // Auto-detect sprite size from first frame
    animator->spriteWidth = 16;
    animator->spriteHeight = 16;
    if (info->count > 0 && info->sequences[0].count > 0 && info->sequences[0].frames) {
        animator->spriteWidth = (int)info->sequences[0].frames[0][2];
        animator->spriteHeight = (int)info->sequences[0].frames[0][3];
        if (animator->spriteWidth < 1) animator->spriteWidth = 16;
        if (animator->spriteHeight < 1) animator->spriteHeight = 16;
    }

    // Compute grid dimensions
    animator->gridCols = (animator->atlasWidth > 0) ? animator->atlasWidth / animator->spriteWidth : 1;
    animator->gridRows = (animator->atlasHeight > 0) ? animator->atlasHeight / animator->spriteHeight : 1;

    // Reset state
    animator->active = true;
    animator->dirty = false;
    animator->selectedSequence = -1;
    animator->selectedFrame = -1;
    animator->zoom = 1.0f;
    animator->panX = 0.0f;
    animator->panY = 0.0f;
    animator->previewPlaying = false;
    animator->previewFrame = 0;
    animator->previewElapsedMs = 0;
    animator->previewLastTick = 0;
    animator->newSequenceName[0] = '\0';
    animator->showNewSequencePopup = false;
    SDL_memset(animator->cellSelection, 0, sizeof(animator->cellSelection));

    SDL_Log("AnimatorEditor: Opened %s (%dx%d atlas, %d sequences, cell %dx%d)",
            filename, animator->atlasWidth, animator->atlasHeight,
            info->count, animator->spriteWidth, animator->spriteHeight);

    return true;
}

static void DrawAnimatorViewport(SOT_Editor *editor, SOT_GPU_State *gpu)
{
    SOT_AnimatorEditor *anim = &editor->animator;

    igBegin("Scene Viewport", NULL, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Toolbar
    if (igSmallButton("Save")) {
        if (SOT_SaveAnimations(anim->animInfo, anim->sourceFilename)) {
            anim->dirty = false;
        }
    }
    igSameLine(0, 4);
    if (igSmallButton("Close Animator")) {
        if (anim->dirty) {
            igOpenPopup_Str("UnsavedChanges", 0);
        } else {
            SOT_AnimatorEditor_Close(anim, gpu->device);
            editor->showAnimatorEditor = false;
            igEnd();
            return;
        }
    }
    // Unsaved changes confirmation popup
    if (igBeginPopupModal("UnsavedChanges", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        igText("You have unsaved changes. Close anyway?");
        if (igButton("Save & Close", (ImVec2_c){0, 0})) {
            SOT_SaveAnimations(anim->animInfo, anim->sourceFilename);
            SOT_AnimatorEditor_Close(anim, gpu->device);
            editor->showAnimatorEditor = false;
            igCloseCurrentPopup();
            igEndPopup();
            igEnd();
            return;
        }
        igSameLine(0, 4);
        if (igButton("Discard", (ImVec2_c){0, 0})) {
            SOT_AnimatorEditor_Close(anim, gpu->device);
            editor->showAnimatorEditor = false;
            igCloseCurrentPopup();
            igEndPopup();
            igEnd();
            return;
        }
        igSameLine(0, 4);
        if (igButton("Cancel", (ImVec2_c){0, 0})) {
            igCloseCurrentPopup();
        }
        igEndPopup();
    }
    igSameLine(0, 10);
    igText("Animator: %s", anim->sourceFilename);
    if (anim->dirty) {
        igSameLine(0, 4);
        igText("*");
    }

    igSeparator();

    ImVec2_c avail = igGetContentRegionAvail();

    if (anim->atlasTexture && avail.x > 0 && avail.y > 0) {
        // Compute display size: fit atlas into available space, scaled by zoom
        float atlasAspect = (float)anim->atlasWidth / (float)anim->atlasHeight;
        float panelAspect = avail.x / avail.y;

        float baseW, baseH;
        if (panelAspect > atlasAspect) {
            baseH = avail.y;
            baseW = baseH * atlasAspect;
        } else {
            baseW = avail.x;
            baseH = baseW / atlasAspect;
        }

        float imgW = baseW * anim->zoom;
        float imgH = baseH * anim->zoom;

        // Center the image
        float padX = (avail.x - imgW) * 0.5f + anim->panX;
        float padY = (avail.y - imgH) * 0.5f + anim->panY;
        igSetCursorPosX(igGetCursorPosX() + padX);
        igSetCursorPosY(igGetCursorPosY() + padY);

        ImVec2_c imgTopLeft = igGetCursorScreenPos();

        // Switch to nearest-neighbor for pixel art
        ImDrawList *dl = igGetWindowDrawList();
        ImDrawList_AddCallback(dl, SetSamplerNearest, NULL, 0);

        // Display the atlas texture
        ImTextureRef_c texRef;
        texRef._TexData = NULL;
        texRef._TexID = (ImTextureID)(uintptr_t)anim->atlasTexture;

        ImVec2_c imgSize = { imgW, imgH };
        ImVec2_c uv0 = { 0, 0 };
        ImVec2_c uv1 = { 1, 1 };
        igImage(texRef, imgSize, uv0, uv1);

        ImDrawList_AddCallback(dl, SetSamplerLinear, NULL, 0);

        // Pixel scale: how many screen pixels per atlas pixel
        float scaleX = imgW / (float)anim->atlasWidth;
        float scaleY = imgH / (float)anim->atlasHeight;

        // ---- Grid overlay ----
        ImU32 gridCol = igColorConvertFloat4ToU32((ImVec4_c){1.0f, 1.0f, 1.0f, 0.25f});
        for (int c = 0; c <= anim->gridCols; c++) {
            float x = imgTopLeft.x + c * anim->spriteWidth * scaleX;
            ImDrawList_AddLine(dl, (ImVec2_c){x, imgTopLeft.y}, (ImVec2_c){x, imgTopLeft.y + imgH}, gridCol, 1.0f);
        }
        for (int r = 0; r <= anim->gridRows; r++) {
            float y = imgTopLeft.y + r * anim->spriteHeight * scaleY;
            ImDrawList_AddLine(dl, (ImVec2_c){imgTopLeft.x, y}, (ImVec2_c){imgTopLeft.x + imgW, y}, gridCol, 1.0f);
        }

        // ---- Highlight frames of selected sequence (blue) ----
        if (anim->selectedSequence >= 0 && anim->selectedSequence < anim->animInfo->count) {
            SOT_AnimationSequence *seq = &anim->animInfo->sequences[anim->selectedSequence];
            ImU32 seqCol = igColorConvertFloat4ToU32((ImVec4_c){0.2f, 0.4f, 1.0f, 0.35f});
            ImU32 seqTextCol = igColorConvertFloat4ToU32((ImVec4_c){1.0f, 1.0f, 1.0f, 0.9f});
            for (int f = 0; f < seq->count; f++) {
                float fx = seq->frames[f][0];
                float fy = seq->frames[f][1];
                float fw = seq->frames[f][2];
                float fh = seq->frames[f][3];
                ImVec2_c rMin = { imgTopLeft.x + fx * scaleX, imgTopLeft.y + fy * scaleY };
                ImVec2_c rMax = { rMin.x + fw * scaleX, rMin.y + fh * scaleY };
                ImDrawList_AddRectFilled(dl, rMin, rMax, seqCol, 0, 0);
                // Frame index label
                char fLabel[8];
                SDL_snprintf(fLabel, sizeof(fLabel), "%d", f);
                ImDrawList_AddText_Vec2(dl, (ImVec2_c){rMin.x + 2, rMin.y + 1}, seqTextCol, fLabel, NULL);
            }
        }

        // ---- Highlight cell selection (green) ----
        ImU32 selCol = igColorConvertFloat4ToU32((ImVec4_c){0.2f, 1.0f, 0.3f, 0.35f});
        for (int idx = 0; idx < anim->gridCols * anim->gridRows && idx < 256; idx++) {
            if (!anim->cellSelection[idx]) continue;
            int col = idx % anim->gridCols;
            int row = idx / anim->gridCols;
            ImVec2_c rMin = {
                imgTopLeft.x + col * anim->spriteWidth * scaleX,
                imgTopLeft.y + row * anim->spriteHeight * scaleY
            };
            ImVec2_c rMax = {
                rMin.x + anim->spriteWidth * scaleX,
                rMin.y + anim->spriteHeight * scaleY
            };
            ImDrawList_AddRectFilled(dl, rMin, rMax, selCol, 0, 0);
        }

        // ---- Mouse interaction ----
        // Zoom with mouse wheel
        if (igIsWindowHovered(0)) {
            ImGuiIO *io = igGetIO();
            if (io->MouseWheel != 0) {
                float oldZoom = anim->zoom;
                anim->zoom += io->MouseWheel * 0.1f;
                if (anim->zoom < 0.25f) anim->zoom = 0.25f;
                if (anim->zoom > 8.0f) anim->zoom = 8.0f;
                // Adjust pan so zoom centers on mouse
                float zoomRatio = anim->zoom / oldZoom;
                ImVec2_c mousePos = igGetMousePos();
                anim->panX = mousePos.x - imgTopLeft.x + anim->panX - (mousePos.x - imgTopLeft.x) * zoomRatio;
                anim->panY = mousePos.y - imgTopLeft.y + anim->panY - (mousePos.y - imgTopLeft.y) * zoomRatio;
            }

            // Pan with middle mouse button
            if (igIsMouseDragging(ImGuiMouseButton_Middle, 0)) {
                ImVec2_c delta = io->MouseDelta;
                anim->panX += delta.x;
                anim->panY += delta.y;
            }

            // Cell click with left mouse button
            if (igIsMouseClicked_Bool(ImGuiMouseButton_Left, false)) {
                ImVec2_c mousePos = igGetMousePos();
                float relX = mousePos.x - imgTopLeft.x;
                float relY = mousePos.y - imgTopLeft.y;
                if (relX >= 0 && relX < imgW && relY >= 0 && relY < imgH) {
                    int cellCol = (int)(relX / (anim->spriteWidth * scaleX));
                    int cellRow = (int)(relY / (anim->spriteHeight * scaleY));
                    if (cellCol >= 0 && cellCol < anim->gridCols && cellRow >= 0 && cellRow < anim->gridRows) {
                        int cellIdx = cellRow * anim->gridCols + cellCol;
                        if (cellIdx < 256) {
                            // Skip if cell already belongs to the selected sequence
                            bool belongsToSeq = false;
                            if (anim->selectedSequence >= 0 && anim->selectedSequence < anim->animInfo->count) {
                                SOT_AnimationSequence *seq = &anim->animInfo->sequences[anim->selectedSequence];
                                float cx = (float)(cellCol * anim->spriteWidth);
                                float cy = (float)(cellRow * anim->spriteHeight);
                                for (int f = 0; f < seq->count; f++) {
                                    if ((int)seq->frames[f][0] == (int)cx && (int)seq->frames[f][1] == (int)cy) {
                                        belongsToSeq = true;
                                        break;
                                    }
                                }
                            }
                            if (!belongsToSeq) {
                                anim->cellSelection[cellIdx] = !anim->cellSelection[cellIdx];
                            }
                        }
                    }
                }
            }
        }
    } else {
        igText("No atlas texture loaded");
    }

    igEnd();
}

// ---- Asset Browser ----

// Cached directory tree node
typedef struct AB_TreeNode {
    char name[128];         // Display name (folder name)
    char path[512];         // Full path relative to assets/ (e.g. "shaders/source")
    struct AB_TreeNode *children;
    int childCount;
    int childCapacity;
} AB_TreeNode;

// Cached file entry for the right panel
typedef struct AB_FileEntry {
    char name[128];
    char fullPath[512];     // Full absolute path
} AB_FileEntry;

static AB_TreeNode abTreeRoot;
static bool abTreeBuilt = false;
static AB_FileEntry abFiles[256];
static int abFileCount = 0;
static int abSelectedFile = -1;

static void AB_FreeTree(AB_TreeNode *node)
{
    for (int i = 0; i < node->childCount; i++) {
        AB_FreeTree(&node->children[i]);
    }
    if (node->children) {
        SDL_free(node->children);
        node->children = NULL;
    }
    node->childCount = 0;
    node->childCapacity = 0;
}

static void AB_AddChild(AB_TreeNode *parent, const char *name, const char *relPath)
{
    if (parent->childCount >= parent->childCapacity) {
        parent->childCapacity = parent->childCapacity == 0 ? 8 : parent->childCapacity * 2;
        parent->children = SDL_realloc(parent->children, parent->childCapacity * sizeof(AB_TreeNode));
    }
    AB_TreeNode *child = &parent->children[parent->childCount++];
    SDL_memset(child, 0, sizeof(AB_TreeNode));
    SDL_strlcpy(child->name, name, sizeof(child->name));
    SDL_strlcpy(child->path, relPath, sizeof(child->path));
}

static void AB_BuildTreeRecursive(AB_TreeNode *node, const char *absPath)
{
    int count = 0;
    char **entries = SDL_GlobDirectory(absPath, "*", 0, &count);
    if (!entries) return;

    // Collect subdirectories
    for (int i = 0; i < count; i++) {
        char fullChild[512];
        SDL_snprintf(fullChild, sizeof(fullChild), "%s/%s", absPath, entries[i]);

        // Check if it's a directory by trying to enumerate it
        int subCount = 0;
        char **subTest = SDL_GlobDirectory(fullChild, "*", 0, &subCount);
        bool isDir = (subTest != NULL);
        if (subTest) SDL_free(subTest);

        // Also skip if it has an extension (likely a file)
        // Directories typically don't have extensions in asset folders
        const char *dot = SDL_strrchr(entries[i], '.');
        if (dot && !isDir) continue;

        // It might still be a file with no extension, so only add if glob succeeded
        if (!isDir) continue;

        char relChild[512];
        if (node->path[0]) {
            SDL_snprintf(relChild, sizeof(relChild), "%s/%s", node->path, entries[i]);
        } else {
            SDL_strlcpy(relChild, entries[i], sizeof(relChild));
        }

        AB_AddChild(node, entries[i], relChild);
        AB_BuildTreeRecursive(&node->children[node->childCount - 1], fullChild);
    }

    SDL_free(entries);
}

static void AB_BuildTree(void)
{
    AB_FreeTree(&abTreeRoot);
    SDL_memset(&abTreeRoot, 0, sizeof(abTreeRoot));
    SDL_strlcpy(abTreeRoot.name, "assets", sizeof(abTreeRoot.name));
    abTreeRoot.path[0] = '\0';

    char assetsDir[512];
    SDL_snprintf(assetsDir, sizeof(assetsDir), "%sassets", Paths.Base);
    AB_BuildTreeRecursive(&abTreeRoot, assetsDir);
    abTreeBuilt = true;
}

static void AB_RefreshFiles(SOT_Editor *editor)
{
    abFileCount = 0;
    abSelectedFile = -1;

    char dir[512];
    if (editor->abSelectedFolder[0]) {
        SDL_snprintf(dir, sizeof(dir), "%sassets/%s", Paths.Base, editor->abSelectedFolder);
    } else {
        SDL_snprintf(dir, sizeof(dir), "%sassets", Paths.Base);
    }

    int count = 0;
    char **entries = SDL_GlobDirectory(dir, "*", 0, &count);
    if (!entries) return;

    for (int i = 0; i < count && abFileCount < 256; i++) {
        // Skip entries that look like directories (have subdirectories)
        char fullChild[512];
        SDL_snprintf(fullChild, sizeof(fullChild), "%s/%s", dir, entries[i]);
        int subCount = 0;
        char **subTest = SDL_GlobDirectory(fullChild, "*", 0, &subCount);
        if (subTest) {
            SDL_free(subTest);
            continue; // Skip directories
        }

        // Apply filter
        if (editor->abFilter[0] != '\0') {
            // Case-insensitive substring match
            char lowerName[128], lowerFilter[64];
            SDL_strlcpy(lowerName, entries[i], sizeof(lowerName));
            SDL_strlcpy(lowerFilter, editor->abFilter, sizeof(lowerFilter));
            for (char *p = lowerName; *p; p++) *p = (char)SDL_tolower((unsigned char)*p);
            for (char *p = lowerFilter; *p; p++) *p = (char)SDL_tolower((unsigned char)*p);
            if (!SDL_strstr(lowerName, lowerFilter)) continue;
        }

        AB_FileEntry *f = &abFiles[abFileCount++];
        SDL_strlcpy(f->name, entries[i], sizeof(f->name));
        SDL_snprintf(f->fullPath, sizeof(f->fullPath), "%s/%s", dir, entries[i]);
    }

    SDL_free(entries);
}

// Get file extension type abbreviation and color (muted, desaturated palette)
static void AB_GetFileTypeInfo(const char *filename, const char **abbr, ImU32 *color)
{
    const char *ext = SDL_strrchr(filename, '.');
    if (!ext) { *abbr = "???"; *color = igGetColorU32_Vec4((ImVec4_c){0.40f,0.40f,0.42f,1}); return; }

    if (SDL_strcasecmp(ext, ".json") == 0) {
        *abbr = "JSON"; *color = igGetColorU32_Vec4((ImVec4_c){0.45f,0.58f,0.68f,1});
    } else if (SDL_strcasecmp(ext, ".lua") == 0) {
        *abbr = "LUA"; *color = igGetColorU32_Vec4((ImVec4_c){0.38f,0.48f,0.70f,1});
    } else if (SDL_strcasecmp(ext, ".png") == 0) {
        *abbr = "PNG"; *color = igGetColorU32_Vec4((ImVec4_c){0.42f,0.62f,0.48f,1});
    } else if (SDL_strcasecmp(ext, ".spv") == 0) {
        *abbr = "SPV"; *color = igGetColorU32_Vec4((ImVec4_c){0.65f,0.50f,0.38f,1});
    } else if (SDL_strcasecmp(ext, ".frag") == 0 || SDL_strcasecmp(ext, ".vert") == 0) {
        *abbr = "GLSL"; *color = igGetColorU32_Vec4((ImVec4_c){0.62f,0.52f,0.35f,1});
    } else if (SDL_strcasecmp(ext, ".wav") == 0 || SDL_strcasecmp(ext, ".ogg") == 0 || SDL_strcasecmp(ext, ".mp3") == 0) {
        *abbr = "SFX"; *color = igGetColorU32_Vec4((ImVec4_c){0.58f,0.40f,0.55f,1});
    } else if (SDL_strcasecmp(ext, ".ttf") == 0 || SDL_strcasecmp(ext, ".otf") == 0) {
        *abbr = "FONT"; *color = igGetColorU32_Vec4((ImVec4_c){0.55f,0.55f,0.40f,1});
    } else {
        *abbr = "FILE"; *color = igGetColorU32_Vec4((ImVec4_c){0.40f,0.40f,0.42f,1});
    }
}

// Find or load a thumbnail texture for a .png file
static SDL_GPUTexture *AB_GetThumbnail(SOT_Editor *editor, SOT_GPU_State *gpu, const char *fullPath, int *outW, int *outH)
{
    // Check cache
    for (int i = 0; i < editor->abThumbnailCount; i++) {
        if (SDL_strcmp(editor->abThumbnails[i].filename, fullPath) == 0) {
            *outW = editor->abThumbnails[i].width;
            *outH = editor->abThumbnails[i].height;
            return editor->abThumbnails[i].texture;
        }
    }

    if (editor->abThumbnailCount >= 128) return NULL;

    // Load image via SDL3_image
    SDL_Surface *surface = IMG_Load(fullPath);
    if (!surface) return NULL;

    // Convert to RGBA8 if needed
    if (surface->format != SDL_PIXELFORMAT_RGBA32) {
        SDL_Surface *converted = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(surface);
        if (!converted) return NULL;
        surface = converted;
    }

    int w = surface->w;
    int h = surface->h;

    SDL_GPUTexture *tex = SDL_CreateGPUTexture(gpu->device, &(SDL_GPUTextureCreateInfo) {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .width = w,
        .height = h,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER
    });

    if (!tex) {
        SDL_DestroySurface(surface);
        return NULL;
    }

    Uint32 dataSize = w * h * 4;
    SDL_GPUTransferBuffer *transferBuf = SDL_CreateGPUTransferBuffer(gpu->device, &(SDL_GPUTransferBufferCreateInfo) {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = dataSize
    });

    if (transferBuf) {
        void *mapped = SDL_MapGPUTransferBuffer(gpu->device, transferBuf, false);
        if (mapped) {
            SDL_memcpy(mapped, surface->pixels, dataSize);
            SDL_UnmapGPUTransferBuffer(gpu->device, transferBuf);
        }

        SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(gpu->device);
        if (cmdbuf) {
            SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(cmdbuf);
            SDL_UploadToGPUTexture(
                copyPass,
                &(SDL_GPUTextureTransferInfo) {
                    .transfer_buffer = transferBuf,
                    .offset = 0
                },
                &(SDL_GPUTextureRegion) {
                    .texture = tex,
                    .w = w,
                    .h = h,
                    .d = 1
                },
                false
            );
            SDL_EndGPUCopyPass(copyPass);
            SDL_SubmitGPUCommandBuffer(cmdbuf);
        }
        SDL_ReleaseGPUTransferBuffer(gpu->device, transferBuf);
    }

    SDL_DestroySurface(surface);

    // Cache it
    int idx = editor->abThumbnailCount++;
    SDL_strlcpy(editor->abThumbnails[idx].filename, fullPath, 128);
    editor->abThumbnails[idx].texture = tex;
    editor->abThumbnails[idx].width = w;
    editor->abThumbnails[idx].height = h;

    *outW = w;
    *outH = h;
    return tex;
}

static void AB_DrawFolderTree(SOT_Editor *editor, AB_TreeNode *node)
{
    for (int i = 0; i < node->childCount; i++) {
        AB_TreeNode *child = &node->children[i];
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

        if (child->childCount == 0) {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        // Highlight selected folder
        if (SDL_strcmp(editor->abSelectedFolder, child->path) == 0) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        bool open = igTreeNodeEx_Str(child->name, flags);

        // Click to select folder
        if (igIsItemClicked(ImGuiMouseButton_Left)) {
            bool changed = SDL_strcmp(editor->abSelectedFolder, child->path) != 0;
            SDL_strlcpy(editor->abSelectedFolder, child->path, sizeof(editor->abSelectedFolder));
            if (changed) AB_RefreshFiles(editor);
        }

        if (open && child->childCount > 0) {
            AB_DrawFolderTree(editor, child);
            igTreePop();
        }
    }
}

static void DrawAssetBrowser(SOT_Editor *editor, SOT_GPU_State *gpu)
{
    if (!editor->showAssetBrowser) return;

    igBegin("Asset Browser", &editor->showAssetBrowser, 0);

    // Build tree on first open or after refresh
    if (!abTreeBuilt || editor->abNeedsRefresh) {
        AB_BuildTree();
        AB_RefreshFiles(editor);
        editor->abNeedsRefresh = false;
    }

    // Refresh button
    if (igSmallButton("Refresh")) {
        editor->abNeedsRefresh = true;
    }

    igSeparator();

    // Get available region for the split
    ImVec2_c avail = igGetContentRegionAvail();

    float leftWidth = avail.x * editor->abSplitRatio;
    float splitterWidth = 4.0f;
    float rightWidth = avail.x - leftWidth - splitterWidth;

    // ---- Left panel: Folder tree ----
    if (igBeginChild_Str("AB_Tree", (ImVec2_c){leftWidth, 0}, ImGuiChildFlags_Borders, 0)) {
        AB_DrawFolderTree(editor, &abTreeRoot);
    }
    igEndChild();

    igSameLine(0, 0);

    // ---- Draggable splitter ----
    igButton("##ABSplit", (ImVec2_c){splitterWidth, avail.y});
    if (igIsItemActive()) {
        ImGuiIO *io = igGetIO();
        float delta = io->MouseDelta.x;
        float newLeft = leftWidth + delta;
        float minW = 60.0f;
        if (newLeft >= minW && (avail.x - newLeft - splitterWidth) >= minW) {
            editor->abSplitRatio = newLeft / avail.x;
        }
    }
    if (igIsItemHovered(0) || igIsItemActive()) {
        igSetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }

    igSameLine(0, 0);

    // ---- Right panel: File icons ----
    if (igBeginChild_Str("AB_Files", (ImVec2_c){rightWidth, 0}, ImGuiChildFlags_Borders, 0)) {
        // Top bar: zoom slider + filter
        igSetNextItemWidth(100);
        igSliderFloat("##ABZoom", &editor->abIconSize, 32.0f, 128.0f, "%.0f px", 0);
        igSameLine(0, 8);
        igSetNextItemWidth(150);
        if (igInputText("##ABFilter", editor->abFilter, sizeof(editor->abFilter), 0, NULL, NULL)) {
            AB_RefreshFiles(editor);
        }
        igSameLine(0, 4);
        if (SDL_strlen(editor->abFilter) > 0) {
            if (igSmallButton("X##ABClear")) {
                editor->abFilter[0] = '\0';
                AB_RefreshFiles(editor);
            }
        }

        // "New Template" button when in actors folder
        if (SDL_strstr(editor->abSelectedFolder, "actors")) {
            igSameLine(0, 12);
            if (igSmallButton("+ New Template")) {
                igOpenPopup_Str("##ABNewTemplate", 0);
            }
            if (igBeginPopup("##ABNewTemplate", 0)) {
                static char abNewTmplName[64] = "";
                igText("New Actor Template");
                igInputText("Name##abtmpl", abNewTmplName, sizeof(abNewTmplName), 0, NULL, NULL);
                if (igSmallButton("Create##abtmpl") && abNewTmplName[0]) {
                    SOT_ActorTemplate defaultTmpl = {0};
                    SDL_strlcpy(defaultTmpl.name, abNewTmplName, sizeof(defaultTmpl.name));
                    defaultTmpl.bodyType = SOT_BODY_NONE;
                    defaultTmpl.gravityScale = 1.0f;
                    SOT_SaveActorTemplate(&defaultTmpl, abNewTmplName);
                    editor->abNeedsRefresh = true;
                    abNewTmplName[0] = '\0';
                    igCloseCurrentPopup();
                }
                igSameLine(0, 4);
                if (igSmallButton("Cancel##abtmpl")) {
                    igCloseCurrentPopup();
                }
                igEndPopup();
            }
        }

        igSeparator();

        // Compute grid layout
        float iconSize = editor->abIconSize;
        float padding = 8.0f;
        float cellSize = iconSize + padding;
        ImVec2_c contentAvail = igGetContentRegionAvail();
        int cols = (int)(contentAvail.x / cellSize);
        if (cols < 1) cols = 1;

        // Draw file icons in a grid
        for (int i = 0; i < abFileCount; i++) {
            int col = i % cols;
            if (col > 0) igSameLine(0, padding);

            igBeginGroup();

            const char *ext = SDL_strrchr(abFiles[i].name, '.');
            bool isPng = ext && SDL_strcasecmp(ext, ".png") == 0;
            bool isSelected = (i == abSelectedFile);

            igPushID_Int(i);

            if (isPng) {
                // Texture thumbnail
                int tw, th;
                SDL_GPUTexture *thumb = AB_GetThumbnail(editor, gpu, abFiles[i].fullPath, &tw, &th);
                if (thumb) {
                    // Draw as image button
                    ImTextureRef_c texRef;
                    texRef._TexData = NULL;
                    texRef._TexID = (ImTextureID)(uintptr_t)thumb;

                    // Fit into square preserving aspect ratio
                    float aspect = (float)tw / (float)th;
                    float drawW = iconSize, drawH = iconSize;
                    if (aspect > 1.0f) drawH = iconSize / aspect;
                    else drawW = iconSize * aspect;

                    // Highlight if selected
                    if (isSelected) {
                        ImVec2_c cursorPos = igGetCursorScreenPos();
                        ImDrawList *dl = igGetWindowDrawList();
                        ImU32 selCol = igGetColorU32_Vec4((ImVec4_c){0.3f,0.5f,0.9f,0.4f});
                        ImDrawList_AddRectFilled(dl, cursorPos,
                            (ImVec2_c){cursorPos.x + iconSize, cursorPos.y + iconSize},
                            selCol, 4.0f, 0);
                    }

                    // Center the image in the icon cell
                    float offX = (iconSize - drawW) * 0.5f;
                    float offY = (iconSize - drawH) * 0.5f;
                    ImVec2_c curPos = igGetCursorPos();
                    igSetCursorPos((ImVec2_c){curPos.x + offX, curPos.y + offY});

                    igImage(texRef, (ImVec2_c){drawW, drawH}, (ImVec2_c){0,0}, (ImVec2_c){1,1});

                    // Reset cursor for label
                    igSetCursorPos((ImVec2_c){curPos.x, curPos.y + iconSize + 2});
                } else {
                    // Fallback: generic icon
                    igDummy((ImVec2_c){iconSize, iconSize});
                }
            } else {
                // Generic file icon: colored rectangle with abbreviation
                const char *abbr;
                ImU32 bgColor;
                AB_GetFileTypeInfo(abFiles[i].name, &abbr, &bgColor);

                ImVec2_c cursorPos = igGetCursorScreenPos();
                ImDrawList *dl = igGetWindowDrawList();

                // Background
                if (isSelected) {
                    ImU32 selCol = igGetColorU32_Vec4((ImVec4_c){0.3f,0.5f,0.9f,0.4f});
                    ImDrawList_AddRectFilled(dl, cursorPos,
                        (ImVec2_c){cursorPos.x + iconSize, cursorPos.y + iconSize},
                        selCol, 4.0f, 0);
                }

                // File type colored rectangle (slightly smaller, centered)
                float inset = iconSize * 0.15f;
                ImVec2_c rectMin = {cursorPos.x + inset, cursorPos.y + inset * 0.5f};
                ImVec2_c rectMax = {cursorPos.x + iconSize - inset, cursorPos.y + iconSize - inset * 1.5f};
                ImDrawList_AddRectFilled(dl, rectMin, rectMax, bgColor, 4.0f, 0);

                // Type abbreviation text centered in rectangle
                ImVec2_c textSize = igCalcTextSize(abbr, NULL, false, 0);
                float tx = rectMin.x + (rectMax.x - rectMin.x - textSize.x) * 0.5f;
                float ty = rectMin.y + (rectMax.y - rectMin.y - textSize.y) * 0.5f;
                ImDrawList_AddText_Vec2(dl, (ImVec2_c){tx, ty},
                    igGetColorU32_Vec4((ImVec4_c){1,1,1,1}), abbr, NULL);

                igDummy((ImVec2_c){iconSize, iconSize});
            }

            // Filename label (truncated to icon width, tooltip on hover)
            {
                ImVec2_c nameSize = igCalcTextSize(abFiles[i].name, NULL, false, 0);
                if (nameSize.x > iconSize) {
                    // Truncate: find how many chars fit, append "..."
                    char truncBuf[128];
                    int len = (int)SDL_strlen(abFiles[i].name);
                    int fit = len;
                    for (int ch = len; ch > 0; ch--) {
                        ImVec2_c sz = igCalcTextSize(abFiles[i].name, abFiles[i].name + ch, false, 0);
                        ImVec2_c dotSz = igCalcTextSize("...", NULL, false, 0);
                        if (sz.x + dotSz.x <= iconSize) { fit = ch; break; }
                    }
                    SDL_snprintf(truncBuf, sizeof(truncBuf), "%.*s...", fit, abFiles[i].name);
                    igText("%s", truncBuf);
                } else {
                    igText("%s", abFiles[i].name);
                }
                if (igIsItemHovered(0)) {
                    igSetTooltip("%s", abFiles[i].name);
                }
            }

            igPopID();
            igEndGroup();

            // Single click: select
            if (igIsItemHovered(0) && igIsMouseClicked_Bool(ImGuiMouseButton_Left, false)) {
                abSelectedFile = i;
            }

            // Double-click: type-specific action
            if (igIsItemHovered(0) && igIsMouseDoubleClicked_Nil(ImGuiMouseButton_Left)) {
                abSelectedFile = i;
                const char *ext2 = SDL_strrchr(abFiles[i].name, '.');

                // Scene loading
                if (ext2 && SDL_strcasecmp(ext2, ".json") == 0 &&
                    SDL_strstr(editor->abSelectedFolder, "scenes")) {
                    SDL_strlcpy(editor->pendingSceneLoad, abFiles[i].name, 128);
                    editor->abNeedsRefresh = true;
                }
                // Animation editor
                if (ext2 && SDL_strcasecmp(ext2, ".json") == 0 &&
                    SDL_strstr(editor->abSelectedFolder, "animations")) {
                    if (SOT_AnimatorEditor_Open(&editor->animator, abFiles[i].name, gpu)) {
                        editor->showAnimatorEditor = true;
                    }
                }
            }
        }

        if (abFileCount == 0) {
            igTextDisabled("(empty)");
        }
    }
    igEndChild();

    igEnd();
}

// ---- Animation Preview ----

static void DrawAnimationPreview(SOT_Editor *editor, SOT_Scene *scene, SOT_GPU_State *gpu)
{
    if (!editor->showAnimPreview) return;

    igBegin("Animation Preview", &editor->showAnimPreview, 0);

    if (editor->selectedActor < 0 || editor->selectedActor >= scene->actorsCount) {
        igText("No actor selected");
        igEnd();
        return;
    }

    SOT_Actor *actor = &scene->actors[editor->selectedActor];
    if (actor->animationsCount <= 0) {
        igText("No animations");
        igEnd();
        return;
    }

    // Sequence selector
    int seqIdx = actor->currentAnimation;
    if (seqIdx >= actor->animationsCount) seqIdx = 0;
    SOT_Animation *anim = &actor->animations[seqIdx];
    if (!anim->info || anim->sequenceIndex >= anim->info->count) {
        igText("Invalid animation data");
        igEnd();
        return;
    }

    SOT_AnimationSequence *seq = &anim->info->sequences[anim->sequenceIndex];
    igText("Sequence: %s", seq->name ? seq->name : "(unnamed)");
    igText("Frame: %d / %d", anim->currentFrame, seq->count);

    // Frame scrub
    int frame = anim->currentFrame;
    if (seq->count > 0) {
        int maxFrame = seq->count - 1;
        if (igSliderInt("Frame", &frame, 0, maxFrame, "%d", 0)) {
            anim->currentFrame = (uint16_t)frame;
            anim->elapsedMs = 0;
        }
    }

    // Controls
    if (igSmallButton(anim->isPlaying ? "Pause" : "Play")) {
        anim->isPlaying = !anim->isPlaying;
    }
    igSameLine(0, 4);
    if (igSmallButton("|<")) {
        anim->currentFrame = 0;
        anim->elapsedMs = 0;
    }
    igSameLine(0, 4);
    if (igSmallButton("<")) {
        if (anim->currentFrame > 0) anim->currentFrame--;
        anim->elapsedMs = 0;
    }
    igSameLine(0, 4);
    if (igSmallButton(">")) {
        if (anim->currentFrame < seq->count - 1) anim->currentFrame++;
        anim->elapsedMs = 0;
    }
    igSameLine(0, 4);
    if (igSmallButton(">|")) {
        if (seq->count > 0) anim->currentFrame = seq->count - 1;
        anim->elapsedMs = 0;
    }

    // Display current frame from atlas
    if (frame < seq->count && seq->frames && anim->atlasIndex < (uint32_t)gpu->buffers[SOT_RP_SPRITE].texturesCount) {
        vec4 *fRect = &seq->frames[frame];
        float fx = (*fRect)[0], fy = (*fRect)[1], fw = (*fRect)[2], fh = (*fRect)[3];
        float aw = (float)anim->atlasSize[0];
        float ah = (float)anim->atlasSize[1];

        if (aw > 0 && ah > 0) {
            ImDrawList *dl = igGetWindowDrawList();
            ImDrawList_AddCallback(dl, SetSamplerNearest, NULL, 0);

            ImTextureRef_c texRef;
            texRef._TexData = NULL;
            texRef._TexID = (ImTextureID)(uintptr_t)gpu->buffers[SOT_RP_SPRITE].textures[anim->atlasIndex];

            float scale = 4.0f;
            ImVec2_c imgSz = { fw * scale, fh * scale };
            ImVec2_c uvMin = { fx / aw, fy / ah };
            ImVec2_c uvMax = { (fx + fw) / aw, (fy + fh) / ah };
            igImage(texRef, imgSz, uvMin, uvMax);

            ImDrawList_AddCallback(dl, SetSamplerLinear, NULL, 0);
        }
    }

    igEnd();
}

// ---- Preferences Window ----

static void DrawPreferencesWindow(SOT_Editor *editor)
{
    if (!editor->showPreferences) return;

    igBegin("Preferences", &editor->showPreferences, ImGuiWindowFlags_AlwaysAutoResize);

    // Theme
    igText("Theme");
    for (int t = 0; t < SOT_THEME_COUNT; t++) {
        if (t > 0) igSameLine(0, 10);
        if (igRadioButton_Bool(SOT_ThemeNames[t], editor->prefs.theme == t)) {
            SOT_ThemeID oldTheme = editor->prefs.theme;
            editor->prefs.theme = (SOT_ThemeID)t;
            SOT_ApplyTheme(editor->prefs.theme);
            // Switching between modern and retro: reset font size and rebuild
            bool wasModern = (oldTheme == SOT_THEME_MODERN);
            bool nowModern = (t == SOT_THEME_MODERN);
            if (wasModern != nowModern) {
                editor->prefs.fontSize = nowModern ? 18.0f : 24.0f;
                editor->prefs.fontDirty = true;
            }
        }
    }

    igSeparator();

    // Grid
    igText("Grid");
    igDragInt("Grid Size", &editor->prefs.gridSize, 1, 4, 128, "%d", 0);
    igColorEdit4("Grid Color", editor->prefs.gridColor, 0);

    igSeparator();

    // Font size — options depend on theme type
    igText("Font Size");
    float modernSizes[] = {12.0f, 14.0f, 16.0f, 18.0f, 20.0f, 22.0f, 24.0f, 28.0f};
    int   modernCount   = 8;
    float retroSizes[]  = {8.0f, 16.0f, 24.0f, 32.0f};
    int   retroCount    = 4;
    bool isModern = (editor->prefs.theme == SOT_THEME_MODERN);
    float *sizes = isModern ? modernSizes : retroSizes;
    int count = isModern ? modernCount : retroCount;
    for (int i = 0; i < count; i++) {
        if (i > 0) igSameLine(0, 6);
        char label[8];
        SDL_snprintf(label, sizeof(label), "%.0f", sizes[i]);
        if (igRadioButton_Bool(label, editor->prefs.fontSize == sizes[i])) {
            editor->prefs.fontSize = sizes[i];
        }
    }
    if (!isModern) igTextDisabled("(pixel-perfect at 8x multiples)");

    igEnd();
}

void SOT_Editor_Render(SOT_Editor *editor, AppState *as, SOT_Scene *scene)
{
    if (!editor->initialized) return;

    // FPS counting
    ImGuiIO *io = igGetIO();
    editor->fpsTimer += io->DeltaTime;
    editor->frameCount++;
    if (editor->fpsTimer >= 1.0f) {
        editor->currentFPS = (float)editor->frameCount / editor->fpsTimer;
        editor->frameCount = 0;
        editor->fpsTimer = 0.0f;
    }

    // Create a fullscreen dockspace
    ImGuiID dockspaceId = igGetID_Str("EditorDockspace");
    ImGuiViewport *viewport = igGetMainViewport();
    igSetNextWindowPos(viewport->WorkPos, 0, (ImVec2_c){0, 0});
    igSetNextWindowSize(viewport->WorkSize, 0);
    igSetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags dockWindowFlags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    igPushStyleVar_Float(ImGuiStyleVar_WindowRounding, 0.0f);
    igPushStyleVar_Float(ImGuiStyleVar_WindowBorderSize, 0.0f);
    igPushStyleVar_Vec2(ImGuiStyleVar_WindowPadding, (ImVec2_c){0, 0});
    igBegin("DockspaceWindow", NULL, dockWindowFlags);
    igPopStyleVar(3);

    igDockSpace(dockspaceId, (ImVec2_c){0, 0}, 0, NULL);
    igEnd();

    // Build default dock layout on first run
    ImGuiDockNode *node = igDockBuilderGetNode(dockspaceId);
    if (node == NULL || node->ChildNodes[0] == NULL) {
        igDockBuilderRemoveNode(dockspaceId);
        igDockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
        igDockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

        // Split bottom for Console (20% height)
        ImGuiID dockMain, dockBottom;
        igDockBuilderSplitNode(dockspaceId, ImGuiDir_Down, 0.20f, &dockBottom, &dockMain);

        // Split left for Hierarchy (12% width)
        ImGuiID dockLeft, dockCenter;
        igDockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.12f, &dockLeft, &dockCenter);

        // Split right for Inspector (15% width)
        ImGuiID dockRight, dockViewport;
        igDockBuilderSplitNode(dockCenter, ImGuiDir_Right, 0.15f, &dockRight, &dockViewport);

        igDockBuilderDockWindow("Hierarchy", dockLeft);
        igDockBuilderDockWindow("Scene Viewport", dockViewport);
        igDockBuilderDockWindow("Inspector", dockRight);
        igDockBuilderDockWindow("Animation Preview", dockRight);  // Tab with Inspector
        igDockBuilderDockWindow("Console", dockBottom);
        igDockBuilderDockWindow("Asset Browser", dockBottom);     // Tab with Console

        igDockBuilderFinish(dockspaceId);
    }

    // Keyboard shortcuts
    ImGuiIO *ioShortcut = igGetIO();

    // Ctrl+S: Save (animator or scene)
    if (ioShortcut->KeyCtrl && igIsKeyPressed_Bool(ImGuiKey_S, false)) {
        if (editor->showAnimatorEditor && editor->animator.active) {
            if (SOT_SaveAnimations(editor->animator.animInfo, editor->animator.sourceFilename)) {
                editor->animator.dirty = false;
                SOT_Editor_Log(editor, "Animation saved: %s", editor->animator.sourceFilename);
            }
        } else {
            // Save dirty actor template
            if (editor->templateDirty && editor->editingTemplate
                && editor->selectedActor >= 0 && editor->selectedActor < scene->actorsCount) {
                const char *tmplName = scene->actors[editor->selectedActor].templateName;
                if (tmplName[0] && SOT_SaveActorTemplate(editor->editingTemplate, tmplName)) {
                    editor->templateDirty = false;
                    SOT_Editor_Log(editor, "Template saved: %s", tmplName);
                }
            }
            // Save scene
            if (scene && scene->name[0]) {
                if (SOT_SaveScene(scene, scene->name))
                    SOT_Editor_Log(editor, "Scene saved: %s", scene->name);
            }
        }
    }

    // Undo/redo
    if (ioShortcut->KeyCtrl && igIsKeyPressed_Bool(ImGuiKey_Z, false)) {
        if (ioShortcut->KeyShift)
            DoRedo(editor, scene);
        else
            DoUndo(editor, scene);
    }
    if (ioShortcut->KeyCtrl && igIsKeyPressed_Bool(ImGuiKey_Y, false)) {
        DoRedo(editor, scene);
    }

    // Draw panels
    DrawMenuBar(editor, as->gpu, scene);

    // Conditional viewport: Animator Editor or Scene Viewport
    if (editor->showAnimatorEditor && editor->animator.active) {
        DrawAnimatorViewport(editor, as->gpu);
    } else {
        DrawSceneViewport(editor, scene, as->gpu);
    }

    DrawSceneHierarchy(editor, scene, as);
    DrawPropertyInspector(editor, scene);
    DrawConsolePanel(editor, as);
    DrawAssetBrowser(editor, as->gpu);
    DrawAnimationPreview(editor, scene, as->gpu);
    DrawPreferencesWindow(editor);
}

void SOT_Editor_EndFrame(SOT_Editor *editor)
{
    if (!editor->initialized) return;
    igRender();
}

// ---- Render ImGui to editor window swapchain ----

void SOT_Editor_RenderToSwapchain(SOT_Editor *editor, SOT_GPU_State *gpu)
{
    if (!editor->initialized) return;

    ImDrawData *drawData = igGetDrawData();
    if (!drawData) return;

    SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(gpu->device);
    if (!cmdbuf) return;

    SDL_GPUTexture *swapchain;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, gpu->window, &swapchain, NULL, NULL)) {
        SDL_Log("Editor: Failed to acquire swapchain: %s", SDL_GetError());
        SDL_SubmitGPUCommandBuffer(cmdbuf);
        return;
    }

    if (swapchain != NULL) {
        ImGui_ImplSDLGPU3_PrepareDrawData(drawData, cmdbuf);

        SDL_GPUColorTargetInfo targetInfo = {
            .texture = swapchain,
            .clear_color = (SDL_FColor){ 0.15f, 0.15f, 0.15f, 1.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        };

        SDL_GPURenderPass *pass = SDL_BeginGPURenderPass(cmdbuf, &targetInfo, 1, NULL);
        ImGui_ImplSDLGPU3_RenderDrawData(drawData, cmdbuf, pass, NULL);
        SDL_EndGPURenderPass(pass);
    }

    SDL_SubmitGPUCommandBuffer(cmdbuf);
}

// ---- Game window management ----

void SOT_Editor_StartGame(SOT_Editor *editor, SOT_GPU_State *gpu, SOT_Scene *scene)
{
    if (editor->playing) return;

    editor->gameWindow = SDL_CreateWindow("SOT Game",
        SOT_INTERNAL_WIDTH * 3, SOT_INTERNAL_HEIGHT * 3,
        SDL_WINDOW_RESIZABLE);

    if (!editor->gameWindow) {
        SOT_Editor_LogError(editor, "Failed to create game window: %s", SDL_GetError());
        return;
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu->device, editor->gameWindow)) {
        SOT_Editor_LogError(editor, "Failed to claim game window: %s", SDL_GetError());
        SDL_DestroyWindow(editor->gameWindow);
        editor->gameWindow = NULL;
        return;
    }

    // Take snapshot for state restoration on stop (step 7)
    if (scene) {
        if (editor->snapshot) SDL_free(editor->snapshot);
        editor->snapshotCount = scene->actorsCount;
        editor->snapshot = (SOT_ActorSnapshot *)SDL_calloc(scene->actorsCount, sizeof(SOT_ActorSnapshot));
        if (editor->snapshot) {
            for (int i = 0; i < scene->actorsCount; i++) {
                SOT_Actor *a = &scene->actors[i];
                editor->snapshot[i].position[0] = a->transform.position[0];
                editor->snapshot[i].position[1] = a->transform.position[1];
                editor->snapshot[i].scale[0] = a->transform.scale[0];
                editor->snapshot[i].scale[1] = a->transform.scale[1];
                editor->snapshot[i].enabled = a->enabled;
                editor->snapshot[i].propertyCount = a->propertyCount;
                if (a->propertyCount > 0) {
                    int count = a->propertyCount < SOT_ACTOR_MAX_PROPERTIES ? a->propertyCount : SOT_ACTOR_MAX_PROPERTIES;
                    size_t sz = count * sizeof(SOT_Property);
                    editor->snapshot[i].propertyData = SDL_malloc(sz);
                    if (editor->snapshot[i].propertyData)
                        SDL_memcpy(editor->snapshot[i].propertyData, a->properties, sz);
                } else {
                    editor->snapshot[i].propertyData = NULL;
                }
            }
        }
    }

    editor->playing = true;
    SOT_Editor_Log(editor, "Game started");
    SDL_Log("Editor: Game window created");
}

void SOT_Editor_StopGame(SOT_Editor *editor, SOT_GPU_State *gpu, SOT_Scene *scene)
{
    if (!editor->playing || !editor->gameWindow) return;

    SDL_ReleaseWindowFromGPUDevice(gpu->device, editor->gameWindow);
    SDL_DestroyWindow(editor->gameWindow);
    editor->gameWindow = NULL;
    editor->playing = false;

    // Restore snapshot (step 7)
    if (scene && editor->snapshot && editor->snapshotCount <= scene->actorsCount) {
        for (int i = 0; i < editor->snapshotCount; i++) {
            SOT_Actor *a = &scene->actors[i];
            a->transform.position[0] = editor->snapshot[i].position[0];
            a->transform.position[1] = editor->snapshot[i].position[1];
            a->transform.scale[0] = editor->snapshot[i].scale[0];
            a->transform.scale[1] = editor->snapshot[i].scale[1];
            SOT_Actor_SetEnabled(a, editor->snapshot[i].enabled);
            a->propertyCount = editor->snapshot[i].propertyCount;
            if (editor->snapshot[i].propertyData && a->propertyCount > 0) {
                int count = a->propertyCount < SOT_ACTOR_MAX_PROPERTIES ? a->propertyCount : SOT_ACTOR_MAX_PROPERTIES;
                SDL_memcpy(a->properties, editor->snapshot[i].propertyData,
                           count * sizeof(SOT_Property));
            }
            // Sync physics body
            if (b2Body_IsValid(a->bodyId)) {
                b2Vec2 bpos = SOT_PixelsToMeters(a->transform.position[0], -a->transform.position[1]);
                b2Body_SetTransform(a->bodyId, bpos, b2Body_GetRotation(a->bodyId));
            }
        }
    }

    if (editor->snapshot) {
        for (int i = 0; i < editor->snapshotCount; i++) {
            if (editor->snapshot[i].propertyData)
                SDL_free(editor->snapshot[i].propertyData);
        }
        SDL_free(editor->snapshot);
        editor->snapshot = NULL;
        editor->snapshotCount = 0;
    }

    SOT_Editor_Log(editor, "Game stopped");
    SDL_Log("Editor: Game window closed");
}

void SOT_Editor_RenderGameWindow(SOT_Editor *editor, SOT_GPU_State *gpu)
{
    if (!editor->playing || !editor->gameWindow) return;

    SDL_GPUCommandBuffer *cmdbuf = SDL_AcquireGPUCommandBuffer(gpu->device);
    if (!cmdbuf) return;

    SDL_GPUTexture *swapchain;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdbuf, editor->gameWindow, &swapchain, NULL, NULL)) {
        SDL_SubmitGPUCommandBuffer(cmdbuf);
        return;
    }

    if (swapchain != NULL) {
        SDL_GPUColorTargetInfo targetInfo = {
            .texture = swapchain,
            .clear_color = (SDL_FColor){ 0.0f, 0.0f, 0.0f, 1.0f },
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        };

        SDL_GPURenderPass *blitPass = SDL_BeginGPURenderPass(cmdbuf, &targetInfo, 1, NULL);

        // Integer-scale the framebuffer to the game window, preserving aspect ratio
        int ww, wh;
        SDL_GetWindowSize(editor->gameWindow, &ww, &wh);

        int scaleX = ww / gpu->display.internalWidth;
        int scaleY = wh / gpu->display.internalHeight;
        int scale = (scaleX < scaleY) ? scaleX : scaleY;
        if (scale < 1) scale = 1;

        int vpW = gpu->display.internalWidth * scale;
        int vpH = gpu->display.internalHeight * scale;
        int vpX = (ww - vpW) / 2;
        int vpY = (wh - vpH) / 2;

        SDL_SetGPUViewport(blitPass, &(SDL_GPUViewport) {
            .x = (float)vpX, .y = (float)vpY,
            .w = (float)vpW, .h = (float)vpH,
            .min_depth = 0.0f, .max_depth = 1.0f,
        });

        SDL_BindGPUGraphicsPipeline(blitPass, gpu->pipeline[SOT_RP_BLIT]);
        SDL_BindGPUFragmentSamplers(blitPass, 0, &(SDL_GPUTextureSamplerBinding) {
            .texture = gpu->display.framebuffer,
            .sampler = gpu->nearestSampler,
        }, 1);
        SDL_DrawGPUPrimitives(blitPass, 3, 1, 0, 0);

        SDL_EndGPURenderPass(blitPass);
    }

    SDL_SubmitGPUCommandBuffer(cmdbuf);
}

// ---- Logging ----

static void SOT_Editor_LogSev(SOT_Editor *editor, SOT_LogSeverity sev, const char *fmt, va_list args)
{
    if (!editor) return;

    if (editor->logCount < 256) {
        SDL_vsnprintf(editor->logLines[editor->logCount], 256, fmt, args);
        editor->logSeverity[editor->logCount] = sev;
        editor->logCount++;
    } else {
        SDL_memmove(editor->logLines[0], editor->logLines[1], 255 * 256);
        SDL_memmove(editor->logSeverity, editor->logSeverity + 1, 255 * sizeof(SOT_LogSeverity));
        SDL_vsnprintf(editor->logLines[255], 256, fmt, args);
        editor->logSeverity[255] = sev;
    }
}

void SOT_Editor_Log(SOT_Editor *editor, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    SOT_Editor_LogSev(editor, SOT_LOG_INFO, fmt, args);
    va_end(args);
}

void SOT_Editor_LogWarn(SOT_Editor *editor, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    SOT_Editor_LogSev(editor, SOT_LOG_WARNING, fmt, args);
    va_end(args);
}

void SOT_Editor_LogError(SOT_Editor *editor, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    SOT_Editor_LogSev(editor, SOT_LOG_ERROR, fmt, args);
    va_end(args);
}

void SOT_Editor_LogLua(SOT_Editor *editor, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    SOT_Editor_LogSev(editor, SOT_LOG_LUA, fmt, args);
    va_end(args);
}

// ---- Preferences persistence ----

static void LoadEditorPrefs(SOT_Editor *editor)
{
    char path[512];
    SDL_snprintf(path, sizeof(path), "%seditor_prefs.json", Paths.Base);

    FILE *fp = fopen(path, "r");
    if (!fp) return;

    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    rewind(fp);
    char *buf = (char *)SDL_calloc(sz + 1, 1);
    fread(buf, 1, sz, fp);
    fclose(fp);

    cJSON *json = cJSON_Parse(buf);
    SDL_free(buf);
    if (!json) return;

    cJSON *item;
    if ((item = cJSON_GetObjectItem(json, "theme")) && cJSON_IsNumber(item))
        editor->prefs.theme = item->valueint;
    if ((item = cJSON_GetObjectItem(json, "fontSize")) && cJSON_IsNumber(item))
        editor->prefs.fontSize = (float)item->valuedouble;
    if ((item = cJSON_GetObjectItem(json, "gridSize")) && cJSON_IsNumber(item))
        editor->prefs.gridSize = item->valueint;

    cJSON *gc = cJSON_GetObjectItem(json, "gridColor");
    if (gc && cJSON_IsArray(gc) && cJSON_GetArraySize(gc) == 4) {
        for (int i = 0; i < 4; i++)
            editor->prefs.gridColor[i] = (float)cJSON_GetArrayItem(gc, i)->valuedouble;
    }

    cJSON_Delete(json);

    // Clamp theme to valid range
    if (editor->prefs.theme < 0 || editor->prefs.theme >= SOT_THEME_COUNT)
        editor->prefs.theme = SOT_THEME_MODERN;

    // Snap fontSize to nearest valid value for the active theme
    {
        float modernSizes[] = {12.0f, 14.0f, 16.0f, 18.0f, 20.0f, 22.0f, 24.0f, 28.0f};
        int   modernCount   = 8;
        float retroSizes[]  = {8.0f, 16.0f, 24.0f, 32.0f};
        int   retroCount    = 4;
        bool isModern = (editor->prefs.theme == SOT_THEME_MODERN);
        float *sizes = isModern ? modernSizes : retroSizes;
        int count = isModern ? modernCount : retroCount;
        float best = sizes[0];
        float bestDist = fabsf(editor->prefs.fontSize - sizes[0]);
        for (int i = 1; i < count; i++) {
            float d = fabsf(editor->prefs.fontSize - sizes[i]);
            if (d < bestDist) { best = sizes[i]; bestDist = d; }
        }
        editor->prefs.fontSize = best;
    }

    // Apply loaded theme
    SOT_ApplyTheme(editor->prefs.theme);

    // Always mark dirty on load to apply the font
    editor->prefs.fontDirty = true;
}

static void SaveEditorPrefs(SOT_Editor *editor)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "theme", editor->prefs.theme);
    cJSON_AddNumberToObject(json, "fontSize", editor->prefs.fontSize);
    cJSON_AddNumberToObject(json, "gridSize", editor->prefs.gridSize);

    cJSON *gc = cJSON_CreateArray();
    for (int i = 0; i < 4; i++)
        cJSON_AddItemToArray(gc, cJSON_CreateNumber(editor->prefs.gridColor[i]));
    cJSON_AddItemToObject(json, "gridColor", gc);

    char *str = cJSON_Print(json);
    cJSON_Delete(json);

    char path[512];
    SDL_snprintf(path, sizeof(path), "%seditor_prefs.json", Paths.Base);
    FILE *fp = fopen(path, "w");
    if (fp) {
        fputs(str, fp);
        fclose(fp);
    }
    SDL_free(str);
}
