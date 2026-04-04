#include "sot_ui.h"
#include "sot_common.h"
#include "cJSON.h"

// ============================================================
// Lifecycle
// ============================================================

bool SOT_UI_Init(SOT_UI *ui)
{
    SDL_memset(ui, 0, sizeof(SOT_UI));
    ui->vertexCapacity = 4096;
    ui->vertices = (float *)SDL_calloc(ui->vertexCapacity, sizeof(float));
    ui->initialized = true;
    SDL_Log("SOT_UI: Initialized");
    return true;
}

void SOT_UI_Shutdown(SOT_UI *ui)
{
    if (ui->vertices) {
        SDL_free(ui->vertices);
        ui->vertices = NULL;
    }
    ui->initialized = false;
    SDL_Log("SOT_UI: Shutdown");
}

// ============================================================
// Font loading (BMFont-compatible JSON format)
// ============================================================

bool SOT_UI_LoadFont(SOT_UI *ui, const char *fontJsonFile)
{
    char fullPath[512];
    SDL_snprintf(fullPath, sizeof(fullPath), "%s\\fonts\\%s", Paths.Base, fontJsonFile);

    FILE *fp = fopen(fullPath, "r");
    if (!fp) {
        SDL_Log("SOT_UI: Cannot open font file '%s'", fullPath);
        return false;
    }

    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    rewind(fp);
    char *content = (char *)SDL_calloc(fileSize + 1, 1);
    fread(content, 1, fileSize, fp);
    fclose(fp);

    cJSON *json = cJSON_Parse(content);
    SDL_free(content);
    if (!json) {
        SDL_Log("SOT_UI: Failed to parse font JSON '%s'", fontJsonFile);
        return false;
    }

    // Font metadata
    cJSON *nameItem = cJSON_GetObjectItemCaseSensitive(json, "name");
    if (nameItem && cJSON_IsString(nameItem))
        SDL_strlcpy(ui->font.name, nameItem->valuestring, sizeof(ui->font.name));

    cJSON *atlasItem = cJSON_GetObjectItemCaseSensitive(json, "atlas");
    if (atlasItem && cJSON_IsString(atlasItem))
        SDL_strlcpy(ui->font.atlasFile, atlasItem->valuestring, sizeof(ui->font.atlasFile));

    cJSON *lineHItem = cJSON_GetObjectItemCaseSensitive(json, "line_height");
    ui->font.lineHeight = (lineHItem && cJSON_IsNumber(lineHItem)) ? lineHItem->valueint : 8;

    cJSON *baseItem = cJSON_GetObjectItemCaseSensitive(json, "base");
    ui->font.base = (baseItem && cJSON_IsNumber(baseItem)) ? baseItem->valueint : 8;

    // Glyphs
    cJSON *glyphs = cJSON_GetObjectItemCaseSensitive(json, "glyphs");
    if (glyphs && cJSON_IsArray(glyphs)) {
        cJSON *g = NULL;
        cJSON_ArrayForEach(g, glyphs) {
            cJSON *idItem = cJSON_GetObjectItemCaseSensitive(g, "id");
            if (!idItem || !cJSON_IsNumber(idItem)) continue;
            int id = idItem->valueint;
            if (id < 0 || id >= SOT_FONT_MAX_GLYPHS) continue;

            SOT_FontGlyph *glyph = &ui->font.glyphs[id];
            cJSON *tmp;
            tmp = cJSON_GetObjectItemCaseSensitive(g, "x");       if (tmp) glyph->x = tmp->valueint;
            tmp = cJSON_GetObjectItemCaseSensitive(g, "y");       if (tmp) glyph->y = tmp->valueint;
            tmp = cJSON_GetObjectItemCaseSensitive(g, "w");       if (tmp) glyph->w = tmp->valueint;
            tmp = cJSON_GetObjectItemCaseSensitive(g, "h");       if (tmp) glyph->h = tmp->valueint;
            tmp = cJSON_GetObjectItemCaseSensitive(g, "xoffset"); if (tmp) glyph->xOffset = tmp->valueint;
            tmp = cJSON_GetObjectItemCaseSensitive(g, "yoffset"); if (tmp) glyph->yOffset = tmp->valueint;
            tmp = cJSON_GetObjectItemCaseSensitive(g, "xadvance"); if (tmp) glyph->xAdvance = tmp->valueint;
        }
    }

    // Fallback: if no glyphs loaded, create a monospace grid font (8x8 ASCII)
    bool hasAnyGlyph = false;
    for (int i = 32; i < 128; i++) {
        if (ui->font.glyphs[i].w > 0) { hasAnyGlyph = true; break; }
    }
    if (!hasAnyGlyph) {
        SDL_Log("SOT_UI: No glyphs found in font JSON, generating monospace 8x8 grid");
        int cellW = 8, cellH = 8, cols = 16;
        for (int i = 0; i < 128; i++) {
            ui->font.glyphs[i].x = (i % cols) * cellW;
            ui->font.glyphs[i].y = (i / cols) * cellH;
            ui->font.glyphs[i].w = cellW;
            ui->font.glyphs[i].h = cellH;
            ui->font.glyphs[i].xAdvance = cellW;
        }
        ui->font.lineHeight = cellH;
    }

    ui->font.loaded = true;
    cJSON_Delete(json);
    SDL_Log("SOT_UI: Loaded font '%s' (lineHeight=%d)", ui->font.name, ui->font.lineHeight);
    return true;
}

// ============================================================
// Per-frame
// ============================================================

void SOT_UI_BeginFrame(SOT_UI *ui)
{
    ui->commandCount = 0;
    ui->vertexCount = 0;
}

// ============================================================
// Text measurement
// ============================================================

int SOT_UI_MeasureTextWidth(const SOT_UI *ui, const char *text)
{
    if (!ui->font.loaded || !text) return 0;
    int width = 0;
    for (const char *p = text; *p; p++) {
        int c = (unsigned char)*p;
        if (c >= SOT_FONT_MAX_GLYPHS) c = '?';
        width += ui->font.glyphs[c].xAdvance;
    }
    return width;
}

// ============================================================
// Immediate-mode draw commands
// ============================================================

void SOT_UI_DrawText(SOT_UI *ui, const char *text, float x, float y, vec4 color, SOT_TextAlign align)
{
    if (ui->commandCount >= SOT_UI_MAX_COMMANDS) return;
    SOT_UICommand *cmd = &ui->commands[ui->commandCount++];
    cmd->type = SOT_UI_CMD_TEXT;
    cmd->x = x;
    cmd->y = y;
    glm_vec4_copy(color, cmd->color);
    cmd->align = align;
    SDL_strlcpy(cmd->text, text, sizeof(cmd->text));
}

void SOT_UI_DrawRect(SOT_UI *ui, float x, float y, float w, float h, vec4 color)
{
    if (ui->commandCount >= SOT_UI_MAX_COMMANDS) return;
    SOT_UICommand *cmd = &ui->commands[ui->commandCount++];
    cmd->type = SOT_UI_CMD_RECT;
    cmd->x = x;
    cmd->y = y;
    cmd->w = w;
    cmd->h = h;
    glm_vec4_copy(color, cmd->color);
}

// ============================================================
// Dialog box
// ============================================================

void SOT_UI_ShowDialog(SOT_UI *ui, const char *text, const char *speaker)
{
    SOT_DialogBox *dlg = &ui->dialog;
    dlg->active = true;
    SDL_strlcpy(dlg->text, text, sizeof(dlg->text));
    if (speaker)
        SDL_strlcpy(dlg->speaker, speaker, sizeof(dlg->speaker));
    else
        dlg->speaker[0] = '\0';
    dlg->revealIndex = 0;
    dlg->revealTimer = 0.0f;
    dlg->revealSpeed = 30.0f; // characters per second
    dlg->fullyRevealed = false;
    dlg->choiceCount = 0;
    dlg->selectedChoice = 0;
    dlg->waitingForChoice = false;
}

void SOT_UI_AddDialogChoice(SOT_UI *ui, const char *choiceText)
{
    SOT_DialogBox *dlg = &ui->dialog;
    if (dlg->choiceCount >= 4) return;
    SDL_strlcpy(dlg->choices[dlg->choiceCount], choiceText, sizeof(dlg->choices[0]));
    dlg->choiceCount++;
    dlg->waitingForChoice = true;
}

void SOT_UI_HideDialog(SOT_UI *ui)
{
    ui->dialog.active = false;
}

bool SOT_UI_IsDialogActive(const SOT_UI *ui)
{
    return ui->dialog.active;
}

int SOT_UI_GetDialogChoice(const SOT_UI *ui)
{
    return ui->dialog.selectedChoice;
}

// ============================================================
// Menu
// ============================================================

void SOT_UI_ShowMenu(SOT_UI *ui, const char *title)
{
    SOT_Menu *menu = &ui->menu;
    menu->active = true;
    SDL_strlcpy(menu->title, title, sizeof(menu->title));
    menu->itemCount = 0;
    menu->selectedIndex = 0;
}

void SOT_UI_AddMenuItem(SOT_UI *ui, const char *label, bool enabled)
{
    SOT_Menu *menu = &ui->menu;
    if (menu->itemCount >= SOT_MENU_MAX_ITEMS) return;
    SDL_strlcpy(menu->items[menu->itemCount].label, label, sizeof(menu->items[0].label));
    menu->items[menu->itemCount].enabled = enabled;
    menu->itemCount++;
}

void SOT_UI_HideMenu(SOT_UI *ui)
{
    ui->menu.active = false;
}

bool SOT_UI_IsMenuActive(const SOT_UI *ui)
{
    return ui->menu.active;
}

int SOT_UI_GetMenuSelection(const SOT_UI *ui)
{
    return ui->menu.selectedIndex;
}

// ============================================================
// Input handling for dialog and menu navigation
// ============================================================

void SOT_UI_HandleInput(SOT_UI *ui, bool up, bool down, bool confirm, bool cancel)
{
    // Dialog input
    if (ui->dialog.active) {
        if (confirm) {
            if (!ui->dialog.fullyRevealed) {
                // Skip typewriter — reveal all text
                ui->dialog.revealIndex = (int)SDL_strlen(ui->dialog.text);
                ui->dialog.fullyRevealed = true;
            } else if (ui->dialog.waitingForChoice) {
                // Confirm choice — dialog will be dismissed by Lua callback
            } else {
                // Dismiss dialog
                SOT_UI_HideDialog(ui);
            }
        }
        if (ui->dialog.waitingForChoice && ui->dialog.fullyRevealed) {
            if (up && ui->dialog.selectedChoice > 0)
                ui->dialog.selectedChoice--;
            if (down && ui->dialog.selectedChoice < ui->dialog.choiceCount - 1)
                ui->dialog.selectedChoice++;
        }
        return; // Dialog consumes input
    }

    // Menu input
    if (ui->menu.active) {
        if (up && ui->menu.selectedIndex > 0) {
            ui->menu.selectedIndex--;
            // Skip disabled items
            while (ui->menu.selectedIndex > 0 && !ui->menu.items[ui->menu.selectedIndex].enabled)
                ui->menu.selectedIndex--;
        }
        if (down && ui->menu.selectedIndex < ui->menu.itemCount - 1) {
            ui->menu.selectedIndex++;
            while (ui->menu.selectedIndex < ui->menu.itemCount - 1 && !ui->menu.items[ui->menu.selectedIndex].enabled)
                ui->menu.selectedIndex++;
        }
        if (cancel) {
            SOT_UI_HideMenu(ui);
        }
        return;
    }
}

// ============================================================
// Update (typewriter effect, etc.)
// ============================================================

void SOT_UI_Update(SOT_UI *ui, float deltaTime)
{
    if (!ui->initialized) return;

    // Typewriter effect for dialog
    if (ui->dialog.active && !ui->dialog.fullyRevealed) {
        ui->dialog.revealTimer += deltaTime * ui->dialog.revealSpeed;
        int textLen = (int)SDL_strlen(ui->dialog.text);
        while (ui->dialog.revealTimer >= 1.0f && ui->dialog.revealIndex < textLen) {
            ui->dialog.revealIndex++;
            ui->dialog.revealTimer -= 1.0f;
        }
        if (ui->dialog.revealIndex >= textLen)
            ui->dialog.fullyRevealed = true;
    }

    // Build draw commands for active dialog
    if (ui->dialog.active) {
        // Dialog background panel
        float dlgX = 8, dlgY = SCREEN_HEIGHT - 64 - 8;
        float dlgW = SCREEN_WIDTH - 16, dlgH = 64;
        SOT_UI_DrawRect(ui, dlgX, dlgY, dlgW, dlgH, (vec4){0.0f, 0.0f, 0.2f, 0.85f});

        // Speaker name
        if (ui->dialog.speaker[0] != '\0') {
            SOT_UI_DrawText(ui, ui->dialog.speaker, dlgX + 4, dlgY + 2,
                           (vec4){1.0f, 1.0f, 0.0f, 1.0f}, SOT_ALIGN_LEFT);
        }

        // Revealed text (truncate to revealIndex)
        char revealedText[1024];
        int copyLen = ui->dialog.revealIndex;
        if (copyLen > (int)sizeof(revealedText) - 1) copyLen = (int)sizeof(revealedText) - 1;
        SDL_memcpy(revealedText, ui->dialog.text, copyLen);
        revealedText[copyLen] = '\0';

        float textY = (ui->dialog.speaker[0] != '\0') ? dlgY + 14 : dlgY + 4;
        SOT_UI_DrawText(ui, revealedText, dlgX + 4, textY,
                       (vec4){1.0f, 1.0f, 1.0f, 1.0f}, SOT_ALIGN_LEFT);

        // Choices
        if (ui->dialog.waitingForChoice && ui->dialog.fullyRevealed) {
            for (int i = 0; i < ui->dialog.choiceCount; i++) {
                char choiceLine[140];
                SDL_snprintf(choiceLine, sizeof(choiceLine), "%s %s",
                    (i == ui->dialog.selectedChoice) ? ">" : " ",
                    ui->dialog.choices[i]);
                float cy = dlgY + dlgH + 2 + i * 10;
                SOT_UI_DrawText(ui, choiceLine, dlgX + 4, cy,
                    (vec4){1.0f, 1.0f, 1.0f, 1.0f}, SOT_ALIGN_LEFT);
            }
        }
    }

    // Build draw commands for active menu
    if (ui->menu.active) {
        float menuW = 120, menuH = 16 + ui->menu.itemCount * 12;
        float menuX = (SCREEN_WIDTH - menuW) * 0.5f;
        float menuY = (SCREEN_HEIGHT - menuH) * 0.5f;

        SOT_UI_DrawRect(ui, menuX, menuY, menuW, menuH, (vec4){0.1f, 0.1f, 0.15f, 0.9f});

        // Title
        SOT_UI_DrawText(ui, ui->menu.title, menuX + menuW * 0.5f, menuY + 2,
                       (vec4){1.0f, 1.0f, 0.0f, 1.0f}, SOT_ALIGN_CENTER);

        // Items
        for (int i = 0; i < ui->menu.itemCount; i++) {
            char itemLine[80];
            SDL_snprintf(itemLine, sizeof(itemLine), "%s %s",
                (i == ui->menu.selectedIndex) ? ">" : " ",
                ui->menu.items[i].label);
            vec4 itemColor;
            if (ui->menu.items[i].enabled)
                glm_vec4_copy((vec4){1.0f, 1.0f, 1.0f, 1.0f}, itemColor);
            else
                glm_vec4_copy((vec4){0.5f, 0.5f, 0.5f, 1.0f}, itemColor);
            SOT_UI_DrawText(ui, itemLine, menuX + 8, menuY + 14 + i * 12, itemColor, SOT_ALIGN_LEFT);
        }
    }
}
