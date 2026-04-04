#ifndef SOT_LUA_H_
#define SOT_LUA_H_

#include <stdbool.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

// Forward declarations
struct AppState;
struct SOT_Scene;
struct SOT_Actor;

// ---- Lua VM state ----

typedef struct SOT_Lua {
    lua_State *L;
    bool initialized;
} SOT_Lua;

// ---- VM lifecycle ----
bool SOT_Lua_Init(SOT_Lua *lua);
void SOT_Lua_Shutdown(SOT_Lua *lua);

// ---- Script loading ----
bool SOT_Lua_LoadScript(SOT_Lua *lua, const char *filepath);
bool SOT_Lua_ReloadScript(SOT_Lua *lua, const char *filepath);

// ---- Register engine API modules ----
void SOT_Lua_RegisterAllAPIs(SOT_Lua *lua, struct AppState *appState, struct SOT_Scene *scene);

// ---- Actor script binding ----
// Load an actor's script file and store its callback references
bool SOT_Lua_LoadActorScript(SOT_Lua *lua, struct SOT_Actor *actor, const char *scriptFile);

// ---- Call actor lifecycle callbacks ----
void SOT_Lua_CallOnCreate(SOT_Lua *lua, struct SOT_Actor *actor);
void SOT_Lua_CallOnUpdate(SOT_Lua *lua, struct SOT_Actor *actor, float deltaTime);
void SOT_Lua_CallOnDestroy(SOT_Lua *lua, struct SOT_Actor *actor);
void SOT_Lua_CallOnCollision(SOT_Lua *lua, struct SOT_Actor *self, struct SOT_Actor *other);
void SOT_Lua_CallOnTriggerEnter(SOT_Lua *lua, struct SOT_Actor *self, struct SOT_Actor *other);
void SOT_Lua_CallOnTriggerExit(SOT_Lua *lua, struct SOT_Actor *self, struct SOT_Actor *other);

// ---- Game state persistence ----
// Push the global game_state table; creates if not exists
void SOT_Lua_EnsureGameState(SOT_Lua *lua);

// ---- Error handling ----
void SOT_Lua_ReportError(SOT_Lua *lua, const char *context);

// ---- Editor integration ----
struct SOT_Editor;
bool SOT_Lua_DoString(SOT_Lua *lua, const char *code, char *outMsg, int outMsgSize);
void SOT_Lua_RedirectPrint(SOT_Lua *lua, struct SOT_Editor *editor);

#endif
