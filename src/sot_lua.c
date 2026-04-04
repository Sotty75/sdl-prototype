#include "sot_lua.h"
#include "sot_actor.h"
#include "sot_scene.h"
#include "sot_input.h"
#include "sot_physics.h"
#include "sot_audio.h"
#include "sot_ui.h"
#include <SDL3/SDL.h>

// ---- Internal: current scene/appstate pointers for API callbacks ----
static struct AppState *s_appState = NULL;
static struct SOT_Scene *s_scene = NULL;

// ============================================================
// VM lifecycle
// ============================================================

bool SOT_Lua_Init(SOT_Lua *lua)
{
    lua->L = luaL_newstate();
    if (!lua->L) {
        SDL_Log("SOT_Lua: Failed to create Lua state");
        return false;
    }
    luaL_openlibs(lua->L);
    lua->initialized = true;

    // Create the global game_state table
    SOT_Lua_EnsureGameState(lua);

    // Create the actor scripts registry: _G["_actor_scripts"] = {}
    lua_newtable(lua->L);
    lua_setglobal(lua->L, "_actor_scripts");

    SDL_Log("SOT_Lua: Lua %s initialized", LUA_VERSION);
    return true;
}

void SOT_Lua_Shutdown(SOT_Lua *lua)
{
    if (lua->L) {
        lua_close(lua->L);
        lua->L = NULL;
    }
    lua->initialized = false;
    s_appState = NULL;
    s_scene = NULL;
    SDL_Log("SOT_Lua: Shutdown");
}

// ============================================================
// Error handling
// ============================================================

void SOT_Lua_ReportError(SOT_Lua *lua, const char *context)
{
    const char *msg = lua_tostring(lua->L, -1);
    SDL_Log("SOT_Lua ERROR [%s]: %s", context, msg ? msg : "(no message)");
    lua_pop(lua->L, 1);
}

// ============================================================
// Script loading
// ============================================================

bool SOT_Lua_LoadScript(SOT_Lua *lua, const char *filepath)
{
    if (luaL_dofile(lua->L, filepath) != LUA_OK) {
        SOT_Lua_ReportError(lua, filepath);
        return false;
    }
    return true;
}

bool SOT_Lua_ReloadScript(SOT_Lua *lua, const char *filepath)
{
    SDL_Log("SOT_Lua: Reloading script '%s'", filepath);
    return SOT_Lua_LoadScript(lua, filepath);
}

// ============================================================
// Game state persistence
// ============================================================

void SOT_Lua_EnsureGameState(SOT_Lua *lua)
{
    lua_getglobal(lua->L, "game_state");
    if (lua_isnil(lua->L, -1)) {
        lua_pop(lua->L, 1);
        lua_newtable(lua->L);
        lua_setglobal(lua->L, "game_state");
    } else {
        lua_pop(lua->L, 1);
    }
}

// ============================================================
// Actor script binding
// ============================================================

// Load an actor's script. The script should return a table with callback functions:
//   return { on_create = function(self) ... end, on_update = function(self, dt) ... end, ... }
// The returned table is stored in _actor_scripts[actorID].
bool SOT_Lua_LoadActorScript(SOT_Lua *lua, SOT_Actor *actor, const char *scriptFile)
{
    char fullPath[512];
    SDL_snprintf(fullPath, sizeof(fullPath), "%sassets/scripts/%s", Paths.Base, scriptFile);

    if (luaL_loadfile(lua->L, fullPath) != LUA_OK) {
        SOT_Lua_ReportError(lua, scriptFile);
        return false;
    }

    // Execute the script chunk — it should return a table
    if (lua_pcall(lua->L, 0, 1, 0) != LUA_OK) {
        SOT_Lua_ReportError(lua, scriptFile);
        return false;
    }

    if (!lua_istable(lua->L, -1)) {
        SDL_Log("SOT_Lua: Script '%s' must return a table", scriptFile);
        lua_pop(lua->L, 1);
        return false;
    }

    // Store in _actor_scripts[actorID]
    lua_getglobal(lua->L, "_actor_scripts");
    lua_pushinteger(lua->L, actor->actorID);
    lua_pushvalue(lua->L, -3);  // copy the script table
    lua_settable(lua->L, -3);   // _actor_scripts[actorID] = scriptTable
    lua_pop(lua->L, 2);         // pop _actor_scripts and the original script table

    SDL_Log("SOT_Lua: Loaded script '%s' for actor '%s' (id=%d)", scriptFile, actor->actorName, actor->actorID);
    return true;
}

// ---- Helper: push actor table onto stack for 'self' parameter ----
static void PushActorTable(lua_State *L, SOT_Actor *actor)
{
    lua_newtable(L);
    lua_pushinteger(L, actor->actorID);
    lua_setfield(L, -2, "id");
    lua_pushstring(L, actor->actorName[0] ? actor->actorName : "");
    lua_setfield(L, -2, "name");
    lua_pushnumber(L, actor->transform.position[0]);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, actor->transform.position[1]);
    lua_setfield(L, -2, "y");
    lua_pushboolean(L, actor->enabled);
    lua_setfield(L, -2, "enabled");
}

// ---- Helper: get a callback function from the actor's script table ----
// Returns true if function was pushed, false if not found
static bool PushActorCallback(lua_State *L, int actorID, const char *callbackName)
{
    lua_getglobal(L, "_actor_scripts");
    lua_pushinteger(L, actorID);
    lua_gettable(L, -2);       // _actor_scripts[actorID]

    if (!lua_istable(L, -1)) {
        lua_pop(L, 2);        // pop nil + _actor_scripts
        return false;
    }

    lua_getfield(L, -1, callbackName);
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 3);        // pop nil + scriptTable + _actor_scripts
        return false;
    }

    // Remove _actor_scripts and scriptTable from under the function
    lua_remove(L, -2);  // remove scriptTable
    lua_remove(L, -2);  // remove _actor_scripts
    return true;
}

// ============================================================
// Actor lifecycle callbacks
// ============================================================

void SOT_Lua_CallOnCreate(SOT_Lua *lua, SOT_Actor *actor)
{
    if (!PushActorCallback(lua->L, actor->actorID, "on_create")) return;
    PushActorTable(lua->L, actor);
    if (lua_pcall(lua->L, 1, 0, 0) != LUA_OK)
        SOT_Lua_ReportError(lua, "on_create");
}

void SOT_Lua_CallOnUpdate(SOT_Lua *lua, SOT_Actor *actor, float deltaTime)
{
    if (!PushActorCallback(lua->L, actor->actorID, "on_update")) return;
    PushActorTable(lua->L, actor);
    lua_pushnumber(lua->L, deltaTime);
    if (lua_pcall(lua->L, 2, 0, 0) != LUA_OK)
        SOT_Lua_ReportError(lua, "on_update");
}

void SOT_Lua_CallOnDestroy(SOT_Lua *lua, SOT_Actor *actor)
{
    if (!PushActorCallback(lua->L, actor->actorID, "on_destroy")) return;
    PushActorTable(lua->L, actor);
    if (lua_pcall(lua->L, 1, 0, 0) != LUA_OK)
        SOT_Lua_ReportError(lua, "on_destroy");
}

void SOT_Lua_CallOnCollision(SOT_Lua *lua, SOT_Actor *self, SOT_Actor *other)
{
    if (!PushActorCallback(lua->L, self->actorID, "on_collision")) return;
    PushActorTable(lua->L, self);
    PushActorTable(lua->L, other);
    if (lua_pcall(lua->L, 2, 0, 0) != LUA_OK)
        SOT_Lua_ReportError(lua, "on_collision");
}

void SOT_Lua_CallOnTriggerEnter(SOT_Lua *lua, SOT_Actor *self, SOT_Actor *other)
{
    if (!PushActorCallback(lua->L, self->actorID, "on_trigger_enter")) return;
    PushActorTable(lua->L, self);
    PushActorTable(lua->L, other);
    if (lua_pcall(lua->L, 2, 0, 0) != LUA_OK)
        SOT_Lua_ReportError(lua, "on_trigger_enter");
}

void SOT_Lua_CallOnTriggerExit(SOT_Lua *lua, SOT_Actor *self, SOT_Actor *other)
{
    if (!PushActorCallback(lua->L, self->actorID, "on_trigger_exit")) return;
    PushActorTable(lua->L, self);
    PushActorTable(lua->L, other);
    if (lua_pcall(lua->L, 2, 0, 0) != LUA_OK)
        SOT_Lua_ReportError(lua, "on_trigger_exit");
}

// ============================================================
// Engine API: sot.actor
// ============================================================

static int l_actor_get_position(lua_State *L)
{
    int actorID = (int)luaL_checkinteger(L, 1);
    if (!s_scene || actorID < 0 || actorID >= s_scene->actorsCount) {
        lua_pushnil(L); lua_pushnil(L); return 2;
    }
    SOT_Actor *a = &s_scene->actors[actorID];
    lua_pushnumber(L, a->transform.position[0]);
    lua_pushnumber(L, a->transform.position[1]);
    return 2;
}

static int l_actor_set_position(lua_State *L)
{
    int actorID = (int)luaL_checkinteger(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    if (!s_scene || actorID < 0 || actorID >= s_scene->actorsCount) return 0;
    vec2 pos = { x, y };
    SetPosition(&s_scene->actors[actorID], pos);
    return 0;
}

static int l_actor_get_property(lua_State *L)
{
    int actorID = (int)luaL_checkinteger(L, 1);
    const char *key = luaL_checkstring(L, 2);
    if (!s_scene || actorID < 0 || actorID >= s_scene->actorsCount) {
        lua_pushnil(L); return 1;
    }
    SOT_Actor *a = &s_scene->actors[actorID];

    float num; const char *str; bool b;
    if (SOT_Actor_GetPropertyNumber(a, key, &num)) {
        lua_pushnumber(L, num);
    } else if (SOT_Actor_GetPropertyString(a, key, &str)) {
        lua_pushstring(L, str);
    } else if (SOT_Actor_GetPropertyBool(a, key, &b)) {
        lua_pushboolean(L, b);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

static int l_actor_set_property(lua_State *L)
{
    int actorID = (int)luaL_checkinteger(L, 1);
    const char *key = luaL_checkstring(L, 2);
    if (!s_scene || actorID < 0 || actorID >= s_scene->actorsCount) return 0;
    SOT_Actor *a = &s_scene->actors[actorID];

    if (lua_isnumber(L, 3)) {
        SOT_Actor_SetPropertyNumber(a, key, (float)lua_tonumber(L, 3));
    } else if (lua_isstring(L, 3)) {
        SOT_Actor_SetPropertyString(a, key, lua_tostring(L, 3));
    } else if (lua_isboolean(L, 3)) {
        SOT_Actor_SetPropertyBool(a, key, lua_toboolean(L, 3));
    }
    return 0;
}

static int l_actor_find_by_tag(lua_State *L)
{
    const char *tag = luaL_checkstring(L, 1);
    lua_newtable(L);
    if (!s_scene) return 1;
    int idx = 1;
    for (int i = 0; i < s_scene->actorsCount; i++) {
        if (SOT_Actor_HasTag(&s_scene->actors[i], tag)) {
            lua_pushinteger(L, i);
            lua_rawseti(L, -2, idx++);
        }
    }
    return 1;
}

static int l_actor_find_by_name(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    if (!s_scene) { lua_pushnil(L); return 1; }
    for (int i = 0; i < s_scene->actorsCount; i++) {
        if (s_scene->actors[i].actorName[0] && SDL_strcmp(s_scene->actors[i].actorName, name) == 0) {
            lua_pushinteger(L, i);
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}

static int l_actor_set_enabled(lua_State *L)
{
    int actorID = (int)luaL_checkinteger(L, 1);
    bool enabled = lua_toboolean(L, 2);
    if (!s_scene || actorID < 0 || actorID >= s_scene->actorsCount) return 0;
    SOT_Actor_SetEnabled(&s_scene->actors[actorID], enabled);
    return 0;
}

static const luaL_Reg actor_lib[] = {
    {"get_position",  l_actor_get_position},
    {"set_position",  l_actor_set_position},
    {"get_property",  l_actor_get_property},
    {"set_property",  l_actor_set_property},
    {"find_by_tag",   l_actor_find_by_tag},
    {"find_by_name",  l_actor_find_by_name},
    {"set_enabled",   l_actor_set_enabled},
    {NULL, NULL}
};

// ============================================================
// Engine API: sot.anim
// ============================================================

static int l_anim_play(lua_State *L)
{
    int actorID = (int)luaL_checkinteger(L, 1);
    const char *animName = luaL_checkstring(L, 2);
    if (!s_scene || actorID < 0 || actorID >= s_scene->actorsCount) {
        lua_pushboolean(L, false); return 1;
    }
    lua_pushboolean(L, SOT_Animation_Play(&s_scene->actors[actorID], animName));
    return 1;
}

static int l_anim_stop(lua_State *L)
{
    int actorID = (int)luaL_checkinteger(L, 1);
    if (!s_scene || actorID < 0 || actorID >= s_scene->actorsCount) return 0;
    SOT_Animation_Stop(&s_scene->actors[actorID]);
    return 0;
}

static int l_anim_is_playing(lua_State *L)
{
    int actorID = (int)luaL_checkinteger(L, 1);
    if (!s_scene || actorID < 0 || actorID >= s_scene->actorsCount) {
        lua_pushboolean(L, false); return 1;
    }
    lua_pushboolean(L, SOT_Animation_IsPlaying(&s_scene->actors[actorID]));
    return 1;
}

static int l_anim_set_speed(lua_State *L)
{
    int actorID = (int)luaL_checkinteger(L, 1);
    float multiplier = (float)luaL_checknumber(L, 2);
    if (!s_scene || actorID < 0 || actorID >= s_scene->actorsCount) return 0;
    SOT_Animation_SetSpeed(&s_scene->actors[actorID], multiplier);
    return 0;
}

static const luaL_Reg anim_lib[] = {
    {"play",       l_anim_play},
    {"stop",       l_anim_stop},
    {"is_playing", l_anim_is_playing},
    {"set_speed",  l_anim_set_speed},
    {NULL, NULL}
};

// ============================================================
// Engine API: sot.physics
// ============================================================

static int l_physics_set_velocity(lua_State *L)
{
    int actorID = (int)luaL_checkinteger(L, 1);
    float vx = (float)luaL_checknumber(L, 2);
    float vy = (float)luaL_checknumber(L, 3);
    if (!s_scene || actorID < 0 || actorID >= s_scene->actorsCount) return 0;
    vec2 vel = { vx, vy };
    SetVelocity(&s_scene->actors[actorID], vel);
    return 0;
}

static int l_physics_get_velocity(lua_State *L)
{
    int actorID = (int)luaL_checkinteger(L, 1);
    if (!s_scene || actorID < 0 || actorID >= s_scene->actorsCount) {
        lua_pushnumber(L, 0); lua_pushnumber(L, 0); return 2;
    }
    SOT_Actor *a = &s_scene->actors[actorID];
    if (B2_IS_NON_NULL(a->bodyId)) {
        b2Vec2 v = b2Body_GetLinearVelocity(a->bodyId);
        lua_pushnumber(L, v.x * SOT_PIXELS_PER_METER);
        lua_pushnumber(L, v.y * SOT_PIXELS_PER_METER);
    } else {
        lua_pushnumber(L, 0);
        lua_pushnumber(L, 0);
    }
    return 2;
}

static int l_physics_apply_impulse(lua_State *L)
{
    int actorID = (int)luaL_checkinteger(L, 1);
    float ix = (float)luaL_checknumber(L, 2);
    float iy = (float)luaL_checknumber(L, 3);
    if (!s_scene || actorID < 0 || actorID >= s_scene->actorsCount) return 0;
    SOT_Actor *a = &s_scene->actors[actorID];
    if (B2_IS_NON_NULL(a->bodyId)) {
        b2Vec2 impulse = { ix * SOT_METERS_PER_PIXEL, iy * SOT_METERS_PER_PIXEL };
        b2Body_ApplyLinearImpulseToCenter(a->bodyId, impulse, true);
    }
    return 0;
}

static int l_physics_set_gravity_scale(lua_State *L)
{
    int actorID = (int)luaL_checkinteger(L, 1);
    float scale = (float)luaL_checknumber(L, 2);
    if (!s_scene || actorID < 0 || actorID >= s_scene->actorsCount) return 0;
    SOT_Actor *a = &s_scene->actors[actorID];
    if (B2_IS_NON_NULL(a->bodyId))
        b2Body_SetGravityScale(a->bodyId, scale);
    return 0;
}

static const luaL_Reg physics_lib[] = {
    {"set_velocity",      l_physics_set_velocity},
    {"get_velocity",      l_physics_get_velocity},
    {"apply_impulse",     l_physics_apply_impulse},
    {"set_gravity_scale", l_physics_set_gravity_scale},
    {NULL, NULL}
};

// ============================================================
// Engine API: sot.input
// ============================================================

static int l_input_is_pressed(lua_State *L)
{
    const char *action = luaL_checkstring(L, 1);
    lua_pushboolean(L, s_appState ? SOT_Input_IsPressed(&s_appState->input, action) : false);
    return 1;
}

static int l_input_is_just_pressed(lua_State *L)
{
    const char *action = luaL_checkstring(L, 1);
    lua_pushboolean(L, s_appState ? SOT_Input_IsJustPressed(&s_appState->input, action) : false);
    return 1;
}

static int l_input_is_just_released(lua_State *L)
{
    const char *action = luaL_checkstring(L, 1);
    lua_pushboolean(L, s_appState ? SOT_Input_IsJustReleased(&s_appState->input, action) : false);
    return 1;
}

static int l_input_get_axis(lua_State *L)
{
    const char *action = luaL_checkstring(L, 1);
    lua_pushnumber(L, s_appState ? SOT_Input_GetAxis(&s_appState->input, action) : 0.0f);
    return 1;
}

static const luaL_Reg input_lib[] = {
    {"is_pressed",       l_input_is_pressed},
    {"is_just_pressed",  l_input_is_just_pressed},
    {"is_just_released", l_input_is_just_released},
    {"get_axis",         l_input_get_axis},
    {NULL, NULL}
};

// ============================================================
// Engine API: sot.audio
// ============================================================

static int l_audio_play_music(lua_State *L)
{
    const char *filename = luaL_checkstring(L, 1);
    bool loop = lua_isboolean(L, 2) ? lua_toboolean(L, 2) : true;
    lua_pushboolean(L, s_appState ? SOT_Audio_PlayMusic(&s_appState->audio, filename, loop) : false);
    return 1;
}

static int l_audio_stop_music(lua_State *L)
{
    if (s_appState) SOT_Audio_StopMusic(&s_appState->audio);
    return 0;
}

static int l_audio_play_sfx(lua_State *L)
{
    const char *filename = luaL_checkstring(L, 1);
    float volume = lua_isnumber(L, 2) ? (float)lua_tonumber(L, 2) : 1.0f;
    int ch = s_appState ? SOT_Audio_PlaySFX(&s_appState->audio, filename, volume) : -1;
    lua_pushinteger(L, ch);
    return 1;
}

static int l_audio_set_volume(lua_State *L)
{
    const char *channel = luaL_checkstring(L, 1);
    float volume = (float)luaL_checknumber(L, 2);
    if (!s_appState) return 0;
    if (SDL_strcmp(channel, "master") == 0)     SOT_Audio_SetMasterVolume(&s_appState->audio, volume);
    else if (SDL_strcmp(channel, "music") == 0) SOT_Audio_SetMusicVolume(&s_appState->audio, volume);
    else if (SDL_strcmp(channel, "sfx") == 0)   SOT_Audio_SetSFXVolume(&s_appState->audio, volume);
    return 0;
}

static const luaL_Reg audio_lib[] = {
    {"play_music", l_audio_play_music},
    {"stop_music", l_audio_stop_music},
    {"play_sfx",   l_audio_play_sfx},
    {"set_volume", l_audio_set_volume},
    {NULL, NULL}
};

// ============================================================
// Engine API: sot.ui
// ============================================================

static int l_ui_show_dialog(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    const char *speaker = lua_isstring(L, 2) ? lua_tostring(L, 2) : NULL;
    if (s_appState) SOT_UI_ShowDialog(&s_appState->ui, text, speaker);

    // Add choices if provided as table in arg 3
    if (lua_istable(L, 3) && s_appState) {
        lua_pushnil(L);
        while (lua_next(L, 3) != 0) {
            if (lua_isstring(L, -1))
                SOT_UI_AddDialogChoice(&s_appState->ui, lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }
    return 0;
}

static int l_ui_hide_dialog(lua_State *L)
{
    if (s_appState) SOT_UI_HideDialog(&s_appState->ui);
    return 0;
}

static int l_ui_is_dialog_active(lua_State *L)
{
    lua_pushboolean(L, s_appState ? SOT_UI_IsDialogActive(&s_appState->ui) : false);
    return 1;
}

static int l_ui_get_dialog_choice(lua_State *L)
{
    lua_pushinteger(L, s_appState ? SOT_UI_GetDialogChoice(&s_appState->ui) : 0);
    return 1;
}

static int l_ui_show_menu(lua_State *L)
{
    const char *title = luaL_checkstring(L, 1);
    if (s_appState) SOT_UI_ShowMenu(&s_appState->ui, title);

    // Items from table in arg 2
    if (lua_istable(L, 2) && s_appState) {
        lua_pushnil(L);
        while (lua_next(L, 2) != 0) {
            if (lua_isstring(L, -1))
                SOT_UI_AddMenuItem(&s_appState->ui, lua_tostring(L, -1), true);
            lua_pop(L, 1);
        }
    }
    return 0;
}

static int l_ui_hide_menu(lua_State *L)
{
    if (s_appState) SOT_UI_HideMenu(&s_appState->ui);
    return 0;
}

static int l_ui_draw_text(lua_State *L)
{
    const char *text = luaL_checkstring(L, 1);
    float x = (float)luaL_checknumber(L, 2);
    float y = (float)luaL_checknumber(L, 3);
    float r = lua_isnumber(L, 4) ? (float)lua_tonumber(L, 4) : 1.0f;
    float g = lua_isnumber(L, 5) ? (float)lua_tonumber(L, 5) : 1.0f;
    float b = lua_isnumber(L, 6) ? (float)lua_tonumber(L, 6) : 1.0f;
    if (s_appState)
        SOT_UI_DrawText(&s_appState->ui, text, x, y, (vec4){r, g, b, 1.0f}, SOT_ALIGN_LEFT);
    return 0;
}

static const luaL_Reg ui_lib[] = {
    {"show_dialog",      l_ui_show_dialog},
    {"hide_dialog",      l_ui_hide_dialog},
    {"is_dialog_active", l_ui_is_dialog_active},
    {"get_dialog_choice", l_ui_get_dialog_choice},
    {"show_menu",        l_ui_show_menu},
    {"hide_menu",        l_ui_hide_menu},
    {"draw_text",        l_ui_draw_text},
    {NULL, NULL}
};

// ============================================================
// Engine API: sot.scene
// ============================================================

static SOT_SceneManager *s_sceneManager = NULL;

static int l_scene_get_current(lua_State *L)
{
    if (s_scene)
        lua_pushstring(L, s_scene->name);
    else
        lua_pushnil(L);
    return 1;
}

static int l_scene_set_camera_target(lua_State *L)
{
    int actorID = (int)luaL_checkinteger(L, 1);
    if (s_scene) s_scene->cameraTargetActor = actorID;
    return 0;
}

static int l_scene_set_camera_mode(lua_State *L)
{
    const char *mode = luaL_checkstring(L, 1);
    if (!s_scene) return 0;

    if (SDL_strcmp(mode, "deadzone") == 0) {
        s_scene->cameraFollow.mode = SOT_CAM_FOLLOW_DEADZONE;
        s_scene->cameraFollow.deadZoneX = lua_isnumber(L, 2) ? (float)lua_tonumber(L, 2) : 40.0f;
        s_scene->cameraFollow.deadZoneY = lua_isnumber(L, 3) ? (float)lua_tonumber(L, 3) : 30.0f;
        s_scene->cameraFollow.smoothSpeed = lua_isnumber(L, 4) ? (float)lua_tonumber(L, 4) : 5.0f;
    } else if (SDL_strcmp(mode, "auto_scroll") == 0) {
        s_scene->cameraFollow.mode = SOT_CAM_AUTO_SCROLL;
        s_scene->cameraFollow.scrollSpeedX = lua_isnumber(L, 2) ? (float)lua_tonumber(L, 2) : 30.0f;
        s_scene->cameraFollow.scrollSpeedY = lua_isnumber(L, 3) ? (float)lua_tonumber(L, 3) : 0.0f;
    } else if (SDL_strcmp(mode, "room_snap") == 0) {
        s_scene->cameraFollow.mode = SOT_CAM_ROOM_SNAP;
        s_scene->cameraFollow.roomWidth = lua_isnumber(L, 2) ? (float)lua_tonumber(L, 2) : 320.0f;
        s_scene->cameraFollow.roomHeight = lua_isnumber(L, 3) ? (float)lua_tonumber(L, 3) : 240.0f;
    } else {
        s_scene->cameraFollow.mode = SOT_CAM_FREE;
    }
    return 0;
}

static const luaL_Reg scene_lib[] = {
    {"get_current",       l_scene_get_current},
    {"set_camera_target", l_scene_set_camera_target},
    {"set_camera_mode",   l_scene_set_camera_mode},
    {NULL, NULL}
};

// ============================================================
// Engine API: sot.log (utility)
// ============================================================

static int l_log(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    SDL_Log("Lua: %s", msg);
    return 0;
}

// ============================================================
// Register all APIs
// ============================================================

void SOT_Lua_RegisterAllAPIs(SOT_Lua *lua, struct AppState *appState, struct SOT_Scene *scene)
{
    s_appState = appState;
    s_scene = scene;

    lua_State *L = lua->L;

    // Create the top-level "sot" table
    lua_newtable(L);

    // sot.actor
    luaL_newlib(L, actor_lib);
    lua_setfield(L, -2, "actor");

    // sot.anim
    luaL_newlib(L, anim_lib);
    lua_setfield(L, -2, "anim");

    // sot.physics
    luaL_newlib(L, physics_lib);
    lua_setfield(L, -2, "physics");

    // sot.input
    luaL_newlib(L, input_lib);
    lua_setfield(L, -2, "input");

    // sot.audio
    luaL_newlib(L, audio_lib);
    lua_setfield(L, -2, "audio");

    // sot.scene
    luaL_newlib(L, scene_lib);
    lua_setfield(L, -2, "scene");

    // sot.ui
    luaL_newlib(L, ui_lib);
    lua_setfield(L, -2, "ui");

    // sot.log
    lua_pushcfunction(L, l_log);
    lua_setfield(L, -2, "log");

    // Set sot as global
    lua_setglobal(L, "sot");

    SDL_Log("SOT_Lua: Engine APIs registered (sot.actor, sot.anim, sot.physics, sot.input, sot.audio)");
}

// ---- Editor integration ----

#include "sot_editor.h"

bool SOT_Lua_DoString(SOT_Lua *lua, const char *code, char *outMsg, int outMsgSize)
{
    if (!lua || !lua->initialized || !code) return false;

    int result = luaL_dostring(lua->L, code);
    if (result != LUA_OK) {
        const char *err = lua_tostring(lua->L, -1);
        if (outMsg && outMsgSize > 0)
            SDL_strlcpy(outMsg, err ? err : "Unknown error", outMsgSize);
        lua_pop(lua->L, 1);
        return false;
    }

    // If there's a return value, convert to string
    if (lua_gettop(lua->L) > 0) {
        const char *s = lua_tostring(lua->L, -1);
        if (s && outMsg && outMsgSize > 0)
            SDL_strlcpy(outMsg, s, outMsgSize);
        lua_settop(lua->L, 0);
    } else if (outMsg && outMsgSize > 0) {
        outMsg[0] = '\0';
    }

    return true;
}

// C function that replaces Lua's print()
static int lua_editor_print(lua_State *L)
{
    SOT_Editor *editor = (SOT_Editor *)lua_touserdata(L, lua_upvalueindex(1));
    int n = lua_gettop(L);
    char buf[512] = {0};
    int offset = 0;

    for (int i = 1; i <= n; i++) {
        const char *s = luaL_tolstring(L, i, NULL);
        if (i > 1) {
            if (offset < 511) buf[offset++] = '\t';
        }
        if (s) {
            int len = (int)SDL_strlen(s);
            if (offset + len > 511) len = 511 - offset;
            SDL_memcpy(buf + offset, s, len);
            offset += len;
        }
        lua_pop(L, 1);  // pop the tolstring result
    }
    buf[offset] = '\0';

    SOT_Editor_LogLua(editor, "%s", buf);
    return 0;
}

void SOT_Lua_RedirectPrint(SOT_Lua *lua, SOT_Editor *editor)
{
    if (!lua || !lua->initialized || !editor) return;

    lua_pushlightuserdata(lua->L, editor);
    lua_pushcclosure(lua->L, lua_editor_print, 1);
    lua_setglobal(lua->L, "print");

    SDL_Log("SOT_Lua: print() redirected to editor console");
}
