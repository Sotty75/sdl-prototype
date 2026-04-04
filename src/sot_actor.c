#include "sot_actor.h"
#include "cJSON.h"

// ---- Template loading ----

SOT_ActorTemplate *SOT_LoadActorTemplate(const char *templateName)
{
    char *path = NULL;
    SDL_asprintf(&path, "%sassets/actors/%s.json", Paths.Base, templateName);

    FILE *fp = fopen(path, "r");
    if (fp == NULL) {
        SDL_Log("SOT_Actor: Cannot open template '%s' at %s", templateName, path);
        SDL_free(path);
        return NULL;
    }
    SDL_free(path);

    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    rewind(fp);
    char *content = (char *)SDL_calloc(fileSize + 1, 1);
    fread(content, 1, fileSize, fp);
    content[fileSize] = '\0';
    fclose(fp);

    cJSON *json = cJSON_Parse(content);
    SDL_free(content);
    if (json == NULL) {
        SDL_Log("SOT_Actor: Failed to parse template '%s'", templateName);
        return NULL;
    }

    SOT_ActorTemplate *tmpl = (SOT_ActorTemplate *)SDL_calloc(1, sizeof(SOT_ActorTemplate));

    // Name
    SDL_strlcpy(tmpl->name, templateName, sizeof(tmpl->name));

    // Animation file
    cJSON *animItem = cJSON_GetObjectItemCaseSensitive(json, "animation_file");
    if (animItem && cJSON_IsString(animItem))
        SDL_strlcpy(tmpl->animationFile, animItem->valuestring, sizeof(tmpl->animationFile));

    // Body type
    cJSON *bodyItem = cJSON_GetObjectItemCaseSensitive(json, "body_type");
    if (bodyItem && cJSON_IsString(bodyItem)) {
        const char *bt = bodyItem->valuestring;
        if (SDL_strcmp(bt, "dynamic") == 0)       tmpl->bodyType = SOT_BODY_DYNAMIC;
        else if (SDL_strcmp(bt, "static") == 0)    tmpl->bodyType = SOT_BODY_STATIC;
        else if (SDL_strcmp(bt, "kinematic") == 0)  tmpl->bodyType = SOT_BODY_KINEMATIC;
        else                                        tmpl->bodyType = SOT_BODY_NONE;
    }

    // Fixed rotation
    cJSON *fixedRot = cJSON_GetObjectItemCaseSensitive(json, "fixed_rotation");
    tmpl->fixedRotation = (fixedRot && cJSON_IsTrue(fixedRot));

    // Gravity scale (default 1.0)
    cJSON *gravScale = cJSON_GetObjectItemCaseSensitive(json, "gravity_scale");
    tmpl->gravityScale = (gravScale && cJSON_IsNumber(gravScale)) ? (float)gravScale->valuedouble : 1.0f;

    // Collider
    cJSON *collider = cJSON_GetObjectItemCaseSensitive(json, "collider");
    if (collider) {
        cJSON *ctype = cJSON_GetObjectItemCaseSensitive(collider, "type");
        if (ctype && cJSON_IsString(ctype))
            SDL_strlcpy(tmpl->colliderType, ctype->valuestring, sizeof(tmpl->colliderType));

        cJSON *hw = cJSON_GetObjectItemCaseSensitive(collider, "half_width");
        cJSON *hh = cJSON_GetObjectItemCaseSensitive(collider, "half_height");
        cJSON *rad = cJSON_GetObjectItemCaseSensitive(collider, "radius");
        cJSON *ox = cJSON_GetObjectItemCaseSensitive(collider, "offset_x");
        cJSON *oy = cJSON_GetObjectItemCaseSensitive(collider, "offset_y");
        cJSON *sensor = cJSON_GetObjectItemCaseSensitive(collider, "is_sensor");

        if (hw && cJSON_IsNumber(hw)) tmpl->colliderHalfW = (float)hw->valuedouble;
        if (hh && cJSON_IsNumber(hh)) tmpl->colliderHalfH = (float)hh->valuedouble;
        if (rad && cJSON_IsNumber(rad)) tmpl->colliderRadius = (float)rad->valuedouble;
        if (ox && cJSON_IsNumber(ox)) tmpl->colliderOffsetX = (float)ox->valuedouble;
        if (oy && cJSON_IsNumber(oy)) tmpl->colliderOffsetY = (float)oy->valuedouble;
        tmpl->colliderIsSensor = (sensor && cJSON_IsTrue(sensor));

        cJSON *catBits = cJSON_GetObjectItemCaseSensitive(collider, "category_bits");
        cJSON *mskBits = cJSON_GetObjectItemCaseSensitive(collider, "mask_bits");
        tmpl->categoryBits = (catBits && cJSON_IsNumber(catBits)) ? (uint64_t)catBits->valuedouble : SOT_CAT_PLAYER;
        tmpl->maskBits = (mskBits && cJSON_IsNumber(mskBits)) ? (uint64_t)mskBits->valuedouble : UINT64_MAX;
    }

    // Tags
    cJSON *tags = cJSON_GetObjectItemCaseSensitive(json, "tags");
    if (tags && cJSON_IsArray(tags)) {
        cJSON *tag = NULL;
        cJSON_ArrayForEach(tag, tags) {
            if (cJSON_IsString(tag) && tmpl->tagCount < SOT_ACTOR_MAX_TAGS) {
                SDL_strlcpy(tmpl->tags[tmpl->tagCount], tag->valuestring, SOT_ACTOR_MAX_TAG_LEN);
                tmpl->tagCount++;
            }
        }
    }

    // Properties
    cJSON *props = cJSON_GetObjectItemCaseSensitive(json, "properties");
    if (props && cJSON_IsObject(props)) {
        cJSON *prop = NULL;
        cJSON_ArrayForEach(prop, props) {
            if (tmpl->propertyCount >= SOT_ACTOR_MAX_PROPERTIES) break;
            SOT_Property *p = &tmpl->properties[tmpl->propertyCount];
            SDL_strlcpy(p->key, prop->string, sizeof(p->key));

            if (cJSON_IsNumber(prop)) {
                p->type = SOT_PROP_NUMBER;
                p->value.number = (float)prop->valuedouble;
            } else if (cJSON_IsString(prop)) {
                p->type = SOT_PROP_STRING;
                SDL_strlcpy(p->value.string, prop->valuestring, sizeof(p->value.string));
            } else if (cJSON_IsBool(prop)) {
                p->type = SOT_PROP_BOOL;
                p->value.boolean = cJSON_IsTrue(prop);
            } else {
                continue;
            }
            tmpl->propertyCount++;
        }
    }

    // Script file
    cJSON *scriptItem = cJSON_GetObjectItemCaseSensitive(json, "script");
    if (scriptItem && cJSON_IsString(scriptItem))
        SDL_strlcpy(tmpl->scriptFile, scriptItem->valuestring, sizeof(tmpl->scriptFile));

    // State machine (state name → animation sequence name)
    cJSON *states = cJSON_GetObjectItemCaseSensitive(json, "states");
    if (states && cJSON_IsObject(states)) {
        cJSON *st = NULL;
        cJSON_ArrayForEach(st, states) {
            if (tmpl->stateCount >= SOT_ACTOR_MAX_STATES) break;
            if (!cJSON_IsString(st)) continue;
            SOT_StateMapping *m = &tmpl->states[tmpl->stateCount];
            SDL_strlcpy(m->stateName, st->string, sizeof(m->stateName));
            SDL_strlcpy(m->animationName, st->valuestring, sizeof(m->animationName));
            tmpl->stateCount++;
        }
    }

    cJSON_Delete(json);
    SDL_Log("SOT_Actor: Loaded template '%s' (body=%d, anim=%s, tags=%d, props=%d, states=%d)",
            tmpl->name, tmpl->bodyType, tmpl->animationFile, tmpl->tagCount, tmpl->propertyCount, tmpl->stateCount);
    return tmpl;
}

void SOT_FreeActorTemplate(SOT_ActorTemplate *tmpl)
{
    if (tmpl) SDL_free(tmpl);
}

bool SOT_SaveActorTemplate(const SOT_ActorTemplate *tmpl, const char *templateName)
{
    if (!tmpl || !templateName) return false;

    cJSON *root = cJSON_CreateObject();
    if (!root) return false;

    // Animation file
    if (tmpl->animationFile[0])
        cJSON_AddStringToObject(root, "animation_file", tmpl->animationFile);

    // Body type
    const char *btStr = "none";
    switch (tmpl->bodyType) {
        case SOT_BODY_STATIC:    btStr = "static"; break;
        case SOT_BODY_DYNAMIC:   btStr = "dynamic"; break;
        case SOT_BODY_KINEMATIC: btStr = "kinematic"; break;
        default: break;
    }
    cJSON_AddStringToObject(root, "body_type", btStr);
    cJSON_AddBoolToObject(root, "fixed_rotation", tmpl->fixedRotation);
    cJSON_AddNumberToObject(root, "gravity_scale", tmpl->gravityScale);

    // Collider
    if (tmpl->colliderType[0] && SDL_strcmp(tmpl->colliderType, "none") != 0) {
        cJSON *col = cJSON_AddObjectToObject(root, "collider");
        cJSON_AddStringToObject(col, "type", tmpl->colliderType);
        cJSON_AddNumberToObject(col, "half_width", tmpl->colliderHalfW);
        cJSON_AddNumberToObject(col, "half_height", tmpl->colliderHalfH);
        cJSON_AddNumberToObject(col, "radius", tmpl->colliderRadius);
        cJSON_AddNumberToObject(col, "offset_x", tmpl->colliderOffsetX);
        cJSON_AddNumberToObject(col, "offset_y", tmpl->colliderOffsetY);
        cJSON_AddBoolToObject(col, "is_sensor", tmpl->colliderIsSensor);
        cJSON_AddNumberToObject(col, "category_bits", (double)tmpl->categoryBits);
        cJSON_AddNumberToObject(col, "mask_bits", (double)tmpl->maskBits);
    }

    // Script
    if (tmpl->scriptFile[0])
        cJSON_AddStringToObject(root, "script", tmpl->scriptFile);

    // Tags
    if (tmpl->tagCount > 0) {
        cJSON *tags = cJSON_AddArrayToObject(root, "tags");
        for (int i = 0; i < tmpl->tagCount; i++)
            cJSON_AddItemToArray(tags, cJSON_CreateString(tmpl->tags[i]));
    }

    // Properties
    if (tmpl->propertyCount > 0) {
        cJSON *props = cJSON_AddObjectToObject(root, "properties");
        for (int i = 0; i < tmpl->propertyCount; i++) {
            const SOT_Property *p = &tmpl->properties[i];
            switch (p->type) {
                case SOT_PROP_NUMBER: cJSON_AddNumberToObject(props, p->key, p->value.number); break;
                case SOT_PROP_STRING: cJSON_AddStringToObject(props, p->key, p->value.string); break;
                case SOT_PROP_BOOL:   cJSON_AddBoolToObject(props, p->key, p->value.boolean); break;
                default: break;
            }
        }
    }

    // States
    if (tmpl->stateCount > 0) {
        cJSON *states = cJSON_AddObjectToObject(root, "states");
        for (int i = 0; i < tmpl->stateCount; i++)
            cJSON_AddStringToObject(states, tmpl->states[i].stateName, tmpl->states[i].animationName);
    }

    char *jsonStr = cJSON_Print(root);
    cJSON_Delete(root);
    if (!jsonStr) return false;

    char *savePath = NULL;
    SDL_asprintf(&savePath, "%sassets/actors/%s.json", Paths.Base, templateName);
    FILE *fp = fopen(savePath, "w");
    SDL_free(savePath);
    if (!fp) { SDL_free(jsonStr); return false; }
    fputs(jsonStr, fp);
    fclose(fp);
    SDL_free(jsonStr);

    SDL_Log("SOT_Actor: Saved template '%s'", templateName);
    return true;
}

// ---- Actor creation ----

SOT_Actor SOT_CreateActor(AppState *appState, char *name, vec2 pos, char *animationsFile)
{
    SOT_Actor actor = {0};

    if (name) SDL_strlcpy(actor.actorName, name, sizeof(actor.actorName));
    actor.templateName[0] = '\0';
    actor.enabled = true;
    actor.bodyId = b2_nullBodyId;

    actor.transform.position[0] = pos[0];
    actor.transform.position[1] = pos[1];

    SOT_AnimationInfo *animationInfo = SOT_LoadAnimations(animationsFile);
    SOT_ActorBindAnimations(&actor, animationInfo);

    actor.animationInfos[0] = animationInfo;
    actor.animationInfoCount = 1;
    return actor;
}

SOT_Actor SOT_CreateActorFromTemplate(AppState *appState, SOT_ActorTemplate *tmpl, vec2 pos,
                                       const char *instanceName)
{
    SOT_Actor actor = {0};

    if (instanceName) SDL_strlcpy(actor.actorName, instanceName, sizeof(actor.actorName));
    SDL_strlcpy(actor.templateName, tmpl->name, sizeof(actor.templateName));
    actor.enabled = true;
    actor.bodyId = b2_nullBodyId;

    actor.transform.position[0] = pos[0];
    actor.transform.position[1] = pos[1];

    // Load animations if specified
    if (tmpl->animationFile[0] != '\0') {
        SOT_AnimationInfo *animationInfo = SOT_LoadAnimations(tmpl->animationFile);
        if (animationInfo) {
            SOT_ActorBindAnimations(&actor, animationInfo);
            actor.animationInfos[0] = animationInfo;
            actor.animationInfoCount = 1;
        }
    }

    // Copy tags from template
    actor.tagCount = tmpl->tagCount;
    for (int i = 0; i < tmpl->tagCount; i++)
        SDL_strlcpy(actor.tags[i], tmpl->tags[i], SOT_ACTOR_MAX_TAG_LEN);

    // Copy properties from template
    actor.propertyCount = tmpl->propertyCount;
    SDL_memcpy(actor.properties, tmpl->properties, sizeof(SOT_Property) * tmpl->propertyCount);

    // Copy state machine from template
    actor.stateCount = tmpl->stateCount;
    SDL_memcpy(actor.states, tmpl->states, sizeof(SOT_StateMapping) * tmpl->stateCount);
    actor.currentState[0] = '\0';

    return actor;
}

// ---- Actor physics body setup (call after adding actor to scene with physics world) ----

void SOT_Actor_CreatePhysicsBody(SOT_Actor *actor, SOT_PhysicsWorld *world, SOT_ActorTemplate *tmpl)
{
    if (tmpl->bodyType == SOT_BODY_NONE) return;

    float px = actor->transform.position[0];
    float py = actor->transform.position[1];

    switch (tmpl->bodyType) {
        case SOT_BODY_STATIC:
            actor->bodyId = SOT_Physics_CreateStaticBody(world, px, py);
            break;
        case SOT_BODY_DYNAMIC:
            actor->bodyId = SOT_Physics_CreateDynamicBody(world, px, py, tmpl->fixedRotation);
            break;
        case SOT_BODY_KINEMATIC:
            actor->bodyId = SOT_Physics_CreateKinematicBody(world, px, py);
            break;
        default:
            return;
    }

    // Set gravity scale
    if (tmpl->gravityScale != 1.0f)
        b2Body_SetGravityScale(actor->bodyId, tmpl->gravityScale);

    // Add collider shape
    b2Filter filter = b2DefaultFilter();
    filter.categoryBits = tmpl->categoryBits;
    filter.maskBits = tmpl->maskBits;

    if (SDL_strcmp(tmpl->colliderType, "box") == 0) {
        SOT_Physics_AddBoxShape(actor->bodyId,
            tmpl->colliderHalfW, tmpl->colliderHalfH,
            tmpl->colliderOffsetX, tmpl->colliderOffsetY,
            filter, tmpl->colliderIsSensor);
    } else if (SDL_strcmp(tmpl->colliderType, "circle") == 0) {
        SOT_Physics_AddCircleShape(actor->bodyId,
            tmpl->colliderRadius,
            tmpl->colliderOffsetX, tmpl->colliderOffsetY,
            filter, tmpl->colliderIsSensor);
    }

    SDL_Log("SOT_Actor: Created %s physics body for '%s'",
            tmpl->bodyType == SOT_BODY_DYNAMIC ? "dynamic" :
            tmpl->bodyType == SOT_BODY_STATIC ? "static" : "kinematic",
            actor->actorName);
}

void SOT_ActorBindAnimations(SOT_Actor *actor, SOT_AnimationInfo *animationInfo)
{
    for (int i = 0; i < animationInfo->count; ++i)
    {
        SOT_Animation *anim = &(actor->animations[i]);

        anim->id = i;
        anim->info = animationInfo;
        anim->sequenceIndex = i;
        anim->currentFrame = 0;
        anim->elapsedMs = 0;
        anim->isPlaying = true;
    }
    actor->animationsCount = animationInfo->count;
}

// ---- Property bag API ----

static SOT_Property *FindProperty(SOT_Actor *actor, const char *key)
{
    for (int i = 0; i < actor->propertyCount; i++) {
        if (SDL_strcmp(actor->properties[i].key, key) == 0)
            return &actor->properties[i];
    }
    return NULL;
}

static SOT_Property *FindOrCreateProperty(SOT_Actor *actor, const char *key)
{
    SOT_Property *p = FindProperty(actor, key);
    if (p) return p;
    if (actor->propertyCount >= SOT_ACTOR_MAX_PROPERTIES) return NULL;
    p = &actor->properties[actor->propertyCount++];
    SDL_strlcpy(p->key, key, sizeof(p->key));
    return p;
}

void SOT_Actor_SetPropertyNumber(SOT_Actor *actor, const char *key, float value)
{
    SOT_Property *p = FindOrCreateProperty(actor, key);
    if (p) { p->type = SOT_PROP_NUMBER; p->value.number = value; }
}

void SOT_Actor_SetPropertyString(SOT_Actor *actor, const char *key, const char *value)
{
    SOT_Property *p = FindOrCreateProperty(actor, key);
    if (p) { p->type = SOT_PROP_STRING; SDL_strlcpy(p->value.string, value, sizeof(p->value.string)); }
}

void SOT_Actor_SetPropertyBool(SOT_Actor *actor, const char *key, bool value)
{
    SOT_Property *p = FindOrCreateProperty(actor, key);
    if (p) { p->type = SOT_PROP_BOOL; p->value.boolean = value; }
}

bool SOT_Actor_GetPropertyNumber(const SOT_Actor *actor, const char *key, float *outValue)
{
    for (int i = 0; i < actor->propertyCount; i++) {
        if (SDL_strcmp(actor->properties[i].key, key) == 0 && actor->properties[i].type == SOT_PROP_NUMBER) {
            *outValue = actor->properties[i].value.number;
            return true;
        }
    }
    return false;
}

bool SOT_Actor_GetPropertyString(const SOT_Actor *actor, const char *key, const char **outValue)
{
    for (int i = 0; i < actor->propertyCount; i++) {
        if (SDL_strcmp(actor->properties[i].key, key) == 0 && actor->properties[i].type == SOT_PROP_STRING) {
            *outValue = actor->properties[i].value.string;
            return true;
        }
    }
    return false;
}

bool SOT_Actor_GetPropertyBool(const SOT_Actor *actor, const char *key, bool *outValue)
{
    for (int i = 0; i < actor->propertyCount; i++) {
        if (SDL_strcmp(actor->properties[i].key, key) == 0 && actor->properties[i].type == SOT_PROP_BOOL) {
            *outValue = actor->properties[i].value.boolean;
            return true;
        }
    }
    return false;
}

// ---- Tag API ----

void SOT_Actor_AddTag(SOT_Actor *actor, const char *tag)
{
    if (SOT_Actor_HasTag(actor, tag)) return;
    if (actor->tagCount >= SOT_ACTOR_MAX_TAGS) return;
    SDL_strlcpy(actor->tags[actor->tagCount], tag, SOT_ACTOR_MAX_TAG_LEN);
    actor->tagCount++;
}

bool SOT_Actor_HasTag(const SOT_Actor *actor, const char *tag)
{
    for (int i = 0; i < actor->tagCount; i++) {
        if (SDL_strcmp(actor->tags[i], tag) == 0) return true;
    }
    return false;
}

// ---- State machine ----

bool SOT_Actor_SetState(SOT_Actor *actor, const char *stateName)
{
    if (!actor || !stateName) return false;

    for (int i = 0; i < actor->stateCount; i++) {
        if (SDL_strcmp(actor->states[i].stateName, stateName) == 0) {
            SDL_strlcpy(actor->currentState, stateName, sizeof(actor->currentState));
            return SOT_Animation_Play(actor, actor->states[i].animationName);
        }
    }
    SDL_Log("SOT_Actor: State '%s' not found on actor '%s'", stateName, actor->actorName);
    return false;
}

// ---- Enable/disable ----

void SOT_Actor_SetEnabled(SOT_Actor *actor, bool enabled)
{
    actor->enabled = enabled;

    // Disable/enable Box2D body to stop physics simulation
    if (B2_IS_NON_NULL(actor->bodyId)) {
        if (enabled) b2Body_Enable(actor->bodyId);
        else         b2Body_Disable(actor->bodyId);
    }
}

// ---- Transform and physics ----

void SetPosition(SOT_Actor *actor, vec2 pos) {
    glm_vec2_copy(pos, actor->transform.position);

    if (B2_IS_NON_NULL(actor->bodyId)) {
        b2Body_SetTransform(actor->bodyId,
            SOT_PixelsToMeters(pos[0], pos[1]),
            b2MakeRot(0.0f));
    }
}

void SetVelocity(SOT_Actor *actor, vec2 vel) {
    if (B2_IS_NON_NULL(actor->bodyId)) {
        b2Body_SetLinearVelocity(actor->bodyId,
            (b2Vec2){ vel[0] * SOT_METERS_PER_PIXEL, vel[1] * SOT_METERS_PER_PIXEL });
    }
}

void SetCollider(SOT_Actor *actor, sot_collider_t collider) {
    actor->collider = collider;
}

void UpdateCollider(SOT_Actor *actor, vec2 deltaPos) {
    sot_collider_t *collider = &(actor->collider);

    switch (collider->type) {
        case C2_TYPE_CIRCLE:
            collider->shape.circle.p.x += deltaPos[0];
            collider->shape.circle.p.y += deltaPos[1];
            break;
        case C2_TYPE_AABB:
            collider->shape.AABB.min.x += deltaPos[0];
            collider->shape.AABB.min.y += deltaPos[1];
            collider->shape.AABB.max.x += deltaPos[0];
            collider->shape.AABB.max.y += deltaPos[1];
            break;
        default:
            break;
    }
}

void SetRenderPosition(SOT_Actor *actor)
{
}

void RenderActor(const AppState *as, SOT_Actor *actor) {
    SOT_Animation *animation = &(actor->animations[actor->currentAnimation]);
    SOT_AnimationSequence *seq = &(animation->info->sequences[animation->sequenceIndex]);

    const Uint64 now = SDL_GetTicks();

    if ((now - as->last_step) >= animation->info->step_ms)
    {
        animation->currentFrame = (animation->currentFrame + 1 >= seq->count) ? 0 : animation->currentFrame + 1;
    }

    DrawCollidersDebugInfo(as->gpu, actor->collider);
}

// Legacy function -- will be replaced by input system + Box2D velocity
void MoveActor(SOT_Actor *actor, SDL_Event *event)
{
}

// Legacy function -- will be replaced by Box2D contact events
void Hit(const AppState *as, SOT_Actor *actor) {
}

void UpdateActor(const AppState *as, SOT_Actor *actor, float deltaTime) {
    if (actor == NULL || !actor->enabled) return;

    // Sync actor transform from Box2D body position (if physics-enabled)
    if (B2_IS_NON_NULL(actor->bodyId)) {
        b2Vec2 pos = b2Body_GetPosition(actor->bodyId);
        SOT_MetersToPixels(pos, &actor->transform.position[0], &actor->transform.position[1]);
    }

    // Fire on_update callback if set
    if (actor->callbacks.on_update)
        actor->callbacks.on_update(actor, deltaTime);
}

void DestroyActor(SOT_Actor *actor) {
    if (actor == NULL) return;

    // Fire on_destroy callback if set
    if (actor->callbacks.on_destroy)
        actor->callbacks.on_destroy(actor);

    // Destroy Box2D body if it exists
    if (B2_IS_NON_NULL(actor->bodyId)) {
        b2DestroyBody(actor->bodyId);
        actor->bodyId = b2_nullBodyId;
    }

    // Free all owned SOT_AnimationInfo structures
    for (int i = 0; i < actor->animationInfoCount; ++i) {
        SOT_AnimationInfo *info = actor->animationInfos[i];
        if (info == NULL) continue;

        for (int j = 0; j < info->count; ++j) {
            SDL_free(info->sequences[j].name);
            SDL_free(info->sequences[j].frames);
            SDL_free(info->sequences[j].frameDefs);
        }

        SDL_free(info->sequences);
        SDL_free(info->atlasName);
        SDL_free(info->atlasPath);
        SDL_free(info->collider);
        SDL_free(info);
    }
}

SDL_AppResult SOT_ActorLoadAnimationFile(SOT_Actor *actor, char *animationFile) {
    if (actor == NULL || animationFile == NULL) return SDL_APP_FAILURE;
    if (actor->animationInfoCount >= 16) return SDL_APP_FAILURE;

    SOT_AnimationInfo *info = SOT_LoadAnimations(animationFile);
    if (info == NULL) return SDL_APP_FAILURE;

    actor->animationInfos[actor->animationInfoCount] = info;
    actor->animationInfoCount++;

    SOT_ActorBindAnimations(actor, info);
    return SDL_APP_SUCCESS;
}
