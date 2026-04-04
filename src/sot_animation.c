#include "sot_animation.h"
#include "sot_actor.h"

// ---- Helper: parse play mode string ----
static SOT_PlayMode ParsePlayMode(const char *str)
{
    if (!str) return SOT_PLAY_LOOP;
    if (SDL_strcmp(str, "once") == 0)          return SOT_PLAY_ONCE;
    if (SDL_strcmp(str, "ping_pong") == 0)     return SOT_PLAY_PING_PONG;
    if (SDL_strcmp(str, "once_destroy") == 0)  return SOT_PLAY_ONCE_DESTROY;
    return SOT_PLAY_LOOP;
}

// ---- Loading ----

SOT_AnimationInfo* SOT_LoadAnimations(char *animationsFilename)
{
    char *fullPath;
    SDL_asprintf(&fullPath, "%s\\%s", Paths.Animations, animationsFilename);

    FILE *fp = fopen(fullPath, "r");
    if (fp == NULL) {
        SDL_Log("Error: Unable to open the animations file %s.\n", fullPath);
        SDL_free(fullPath);
        return NULL;
    }
    SDL_free(fullPath);

    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    rewind(fp);
    char *content = (char *)SDL_calloc(fileSize + 1, 1);
    fread(content, 1, fileSize, fp);
    fclose(fp);

    cJSON *json = cJSON_Parse(content);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
            SDL_Log("Error: %s\n", error_ptr);
        cJSON_Delete(json);
        SDL_free(content);
        return NULL;
    }

    SOT_AnimationInfo *animationInfo = (SOT_AnimationInfo *)SDL_calloc(1, sizeof(SOT_AnimationInfo));

    cJSON *animations = cJSON_GetObjectItemCaseSensitive(json, "animations");
    int sequenceCount = cJSON_GetArraySize(animations);
    animationInfo->sequences = (SOT_AnimationSequence *)SDL_calloc(sequenceCount, sizeof(SOT_AnimationSequence));
    if (animationInfo->sequences == NULL) {
        SDL_Log("Error: Failed to allocate sequences array");
        SDL_free(animationInfo);
        cJSON_Delete(json);
        SDL_free(content);
        return NULL;
    }

    // Atlas name
    cJSON *atlas_name = cJSON_GetObjectItemCaseSensitive(json, "atlas_name");
    if (cJSON_IsString(atlas_name) && atlas_name->valuestring) {
        int sl = SDL_strlen(atlas_name->valuestring) + 1;
        animationInfo->atlasName = (char *)SDL_malloc(sl);
        SDL_strlcpy(animationInfo->atlasName, atlas_name->valuestring, sl);
    }

    // Image path
    cJSON *image_path = cJSON_GetObjectItemCaseSensitive(json, "image_path");
    if (cJSON_IsString(image_path) && image_path->valuestring) {
        int sl = SDL_strlen(image_path->valuestring) + 1;
        animationInfo->atlasPath = (char *)SDL_malloc(sl);
        SDL_strlcpy(animationInfo->atlasPath, image_path->valuestring, sl);
    }

    // Collider
    cJSON *collider = cJSON_GetObjectItemCaseSensitive(json, "collider");
    if (cJSON_IsString(collider) && collider->valuestring) {
        int sl = SDL_strlen(collider->valuestring) + 1;
        animationInfo->collider = (char *)SDL_malloc(sl);
        SDL_strlcpy(animationInfo->collider, collider->valuestring, sl);
    }

    // Default step_ms
    cJSON *step_ms = cJSON_GetObjectItemCaseSensitive(json, "step_ms");
    animationInfo->step_ms = (cJSON_IsNumber(step_ms)) ? (uint16_t)step_ms->valueint : 75;

    animationInfo->count = 0;
    cJSON *animation = NULL;

    cJSON_ArrayForEach(animation, animations)
    {
        int i = animationInfo->count;
        SOT_AnimationSequence *seq = &animationInfo->sequences[i];

        // Sequence name
        if (animation->string != NULL) {
            int sl = SDL_strlen(animation->string) + 1;
            seq->name = (char *)SDL_malloc(sl);
            SDL_strlcpy(seq->name, animation->string, sl);
        }

        // Play mode
        cJSON *playModeItem = cJSON_GetObjectItemCaseSensitive(animation, "play_mode");
        seq->playMode = ParsePlayMode(playModeItem ? playModeItem->valuestring : NULL);

        // Frame count
        cJSON *frame_count = cJSON_GetObjectItemCaseSensitive(animation, "frame_count");
        if (frame_count == NULL || !cJSON_IsNumber(frame_count)) continue;
        seq->count = frame_count->valueint;

        // Allocate both legacy frames and enhanced frameDefs
        seq->frames = (vec4 *)SDL_malloc(seq->count * sizeof(vec4));
        seq->frameDefs = (SOT_FrameDef *)SDL_calloc(seq->count, sizeof(SOT_FrameDef));

        int j = 0;
        cJSON *frame = NULL;
        cJSON *frames = cJSON_GetObjectItemCaseSensitive(animation, "frames");
        cJSON_ArrayForEach(frame, frames)
        {
            if (j >= seq->count) break;

            cJSON *x_value = cJSON_GetObjectItemCaseSensitive(frame, "x");
            cJSON *y_value = cJSON_GetObjectItemCaseSensitive(frame, "y");
            cJSON *w_value = cJSON_GetObjectItemCaseSensitive(frame, "w");
            cJSON *h_value = cJSON_GetObjectItemCaseSensitive(frame, "h");
            if (!x_value || !y_value || !w_value || !h_value) continue;

            // Legacy frames array
            seq->frames[j][0] = x_value->valueint;
            seq->frames[j][1] = y_value->valueint;
            seq->frames[j][2] = w_value->valueint;
            seq->frames[j][3] = h_value->valueint;

            // Enhanced frameDef
            seq->frameDefs[j].rect[0] = x_value->valueint;
            seq->frameDefs[j].rect[1] = y_value->valueint;
            seq->frameDefs[j].rect[2] = w_value->valueint;
            seq->frameDefs[j].rect[3] = h_value->valueint;

            // Per-frame duration override
            cJSON *dur = cJSON_GetObjectItemCaseSensitive(frame, "duration_ms");
            seq->frameDefs[j].duration_ms = (dur && cJSON_IsNumber(dur)) ? (uint16_t)dur->valueint : 0;

            // Per-frame events
            cJSON *events = cJSON_GetObjectItemCaseSensitive(frame, "events");
            if (events && cJSON_IsArray(events)) {
                cJSON *evt = NULL;
                cJSON_ArrayForEach(evt, events) {
                    if (seq->frameDefs[j].eventCount >= SOT_MAX_FRAME_EVENTS) break;
                    cJSON *evtName = cJSON_GetObjectItemCaseSensitive(evt, "name");
                    if (evtName && cJSON_IsString(evtName)) {
                        SDL_strlcpy(seq->frameDefs[j].events[seq->frameDefs[j].eventCount].name,
                                    evtName->valuestring, sizeof(seq->frameDefs[j].events[0].name));
                        seq->frameDefs[j].eventCount++;
                    }
                }
            }

            j++;
        }

        animationInfo->count++;
    }

    cJSON_Delete(json);
    SDL_free(content);

    return animationInfo;
}

// ---- Saving ----

static const char* PlayModeToString(SOT_PlayMode mode)
{
    switch (mode) {
        case SOT_PLAY_ONCE:          return "once";
        case SOT_PLAY_PING_PONG:     return "ping_pong";
        case SOT_PLAY_ONCE_DESTROY:  return "once_destroy";
        default:                     return "loop";
    }
}

bool SOT_SaveAnimations(const SOT_AnimationInfo *info, const char *filename)
{
    if (!info || !filename) return false;

    cJSON *root = cJSON_CreateObject();
    if (!root) return false;

    if (info->atlasName) cJSON_AddStringToObject(root, "atlas_name", info->atlasName);
    if (info->atlasPath) cJSON_AddStringToObject(root, "image_path", info->atlasPath);
    if (info->collider)  cJSON_AddStringToObject(root, "collider", info->collider);
    if (info->step_ms != 75) cJSON_AddNumberToObject(root, "step_ms", info->step_ms);

    cJSON *animations = cJSON_AddObjectToObject(root, "animations");

    for (int i = 0; i < info->count; i++) {
        const SOT_AnimationSequence *seq = &info->sequences[i];
        cJSON *seqObj = cJSON_CreateObject();
        cJSON_AddNumberToObject(seqObj, "frame_count", seq->count);

        if (seq->playMode != SOT_PLAY_LOOP) {
            cJSON_AddStringToObject(seqObj, "play_mode", PlayModeToString(seq->playMode));
        }

        cJSON *framesArr = cJSON_AddArrayToObject(seqObj, "frames");
        for (int f = 0; f < seq->count; f++) {
            cJSON *frameObj = cJSON_CreateObject();
            cJSON_AddNumberToObject(frameObj, "x", (int)seq->frames[f][0]);
            cJSON_AddNumberToObject(frameObj, "y", (int)seq->frames[f][1]);
            cJSON_AddNumberToObject(frameObj, "w", (int)seq->frames[f][2]);
            cJSON_AddNumberToObject(frameObj, "h", (int)seq->frames[f][3]);

            // Optional per-frame duration
            if (seq->frameDefs && seq->frameDefs[f].duration_ms > 0) {
                cJSON_AddNumberToObject(frameObj, "duration_ms", seq->frameDefs[f].duration_ms);
            }

            // Optional per-frame events
            if (seq->frameDefs && seq->frameDefs[f].eventCount > 0) {
                cJSON *evtsArr = cJSON_AddArrayToObject(frameObj, "events");
                for (int e = 0; e < seq->frameDefs[f].eventCount; e++) {
                    cJSON *evtObj = cJSON_CreateObject();
                    cJSON_AddStringToObject(evtObj, "name", seq->frameDefs[f].events[e].name);
                    cJSON_AddItemToArray(evtsArr, evtObj);
                }
            }

            cJSON_AddItemToArray(framesArr, frameObj);
        }

        cJSON_AddItemToObject(animations, seq->name ? seq->name : "unnamed", seqObj);
    }

    char *jsonStr = cJSON_Print(root);
    cJSON_Delete(root);

    if (!jsonStr) return false;

    char *fullPath;
    SDL_asprintf(&fullPath, "%s\\%s", Paths.Animations, filename);

    FILE *fp = fopen(fullPath, "w");
    SDL_free(fullPath);
    if (!fp) {
        SDL_free(jsonStr);
        return false;
    }

    fputs(jsonStr, fp);
    fclose(fp);
    SDL_free(jsonStr);

    SDL_Log("Saved animation: %s", filename);
    return true;
}

void SOT_FreeAnimationInfo(SOT_AnimationInfo *info)
{
    if (!info) return;
    for (int i = 0; i < info->count; i++) {
        SOT_AnimationSequence *seq = &info->sequences[i];
        if (seq->name) SDL_free(seq->name);
        if (seq->frames) SDL_free(seq->frames);
        if (seq->frameDefs) SDL_free(seq->frameDefs);
    }
    if (info->sequences) SDL_free(info->sequences);
    if (info->atlasName) SDL_free(info->atlasName);
    if (info->atlasPath) SDL_free(info->atlasPath);
    if (info->collider) SDL_free(info->collider);
    SDL_free(info);
}

// ---- Named lookup ----

bool SOT_Animation_Play(struct SOT_Actor *actor, const char *animName)
{
    if (!actor || !animName) return false;

    // Search across all loaded animation infos
    for (int i = 0; i < actor->animationInfoCount; i++) {
        SOT_AnimationInfo *info = actor->animationInfos[i];
        if (!info) continue;
        for (int j = 0; j < info->count; j++) {
            if (info->sequences[j].name && SDL_strcmp(info->sequences[j].name, animName) == 0) {
                // Find or create a runtime animation pointing to this sequence
                SOT_Animation *anim = &actor->animations[0]; // reuse slot 0 for simplicity
                anim->info = info;
                anim->sequenceIndex = j;
                anim->currentFrame = 0;
                anim->elapsedMs = 0;
                anim->isPlaying = true;
                anim->finished = false;
                anim->pingPongReverse = false;
                actor->currentAnimation = 0;
                return true;
            }
        }
    }
    return false;
}

void SOT_Animation_Stop(struct SOT_Actor *actor)
{
    if (!actor) return;
    SOT_Animation *anim = &actor->animations[actor->currentAnimation];
    anim->isPlaying = false;
}

bool SOT_Animation_IsPlaying(const struct SOT_Actor *actor)
{
    if (!actor) return false;
    const SOT_Animation *anim = &actor->animations[actor->currentAnimation];
    return anim->isPlaying;
}

void SOT_Animation_SetSpeed(struct SOT_Actor *actor, float multiplier)
{
    // Speed multiplier is stored as a modification to the effective step_ms.
    // For now, we adjust step_ms directly (simple approach).
    // A more sophisticated approach would store the multiplier per-animation instance.
    if (!actor) return;
    SOT_Animation *anim = &actor->animations[actor->currentAnimation];
    if (anim->info && multiplier > 0.0f)
        anim->info->step_ms = (uint16_t)(75.0f / multiplier);
}

// ---- Animation update with play modes and frame events ----

void SOT_Animation_Update(struct SOT_Actor *actor, uint32_t deltaMs, SOT_FrameEventCallback callback)
{
    if (!actor) return;
    SOT_Animation *anim = &actor->animations[actor->currentAnimation];
    if (!anim->isPlaying || anim->finished) return;

    SOT_AnimationSequence *seq = &anim->info->sequences[anim->sequenceIndex];
    if (seq->count == 0) return;

    anim->elapsedMs += deltaMs;

    // Determine frame duration: per-frame override or sequence default
    uint16_t frameDur = anim->info->step_ms;
    if (seq->frameDefs && seq->frameDefs[anim->currentFrame].duration_ms > 0)
        frameDur = seq->frameDefs[anim->currentFrame].duration_ms;

    if (anim->elapsedMs < frameDur) return;
    anim->elapsedMs -= frameDur;

    // Advance frame based on play mode
    uint16_t prevFrame = anim->currentFrame;

    switch (seq->playMode) {
        case SOT_PLAY_LOOP:
            anim->currentFrame = (anim->currentFrame + 1) % seq->count;
            break;

        case SOT_PLAY_ONCE:
            if (anim->currentFrame + 1 >= seq->count) {
                anim->finished = true;
                anim->isPlaying = false;
            } else {
                anim->currentFrame++;
            }
            break;

        case SOT_PLAY_ONCE_DESTROY:
            if (anim->currentFrame + 1 >= seq->count) {
                anim->finished = true;
                anim->isPlaying = false;
                // Signal destruction via a special frame event
                if (callback) callback(actor, "_destroy");
            } else {
                anim->currentFrame++;
            }
            break;

        case SOT_PLAY_PING_PONG:
            if (!anim->pingPongReverse) {
                if (anim->currentFrame + 1 >= seq->count) {
                    anim->pingPongReverse = true;
                    if (anim->currentFrame > 0) anim->currentFrame--;
                } else {
                    anim->currentFrame++;
                }
            } else {
                if (anim->currentFrame == 0) {
                    anim->pingPongReverse = false;
                    anim->currentFrame++;
                } else {
                    anim->currentFrame--;
                }
            }
            break;
    }

    // Fire frame events for the new frame
    if (callback && seq->frameDefs && anim->currentFrame != prevFrame) {
        SOT_FrameDef *fd = &seq->frameDefs[anim->currentFrame];
        for (int e = 0; e < fd->eventCount; e++) {
            callback(actor, fd->events[e].name);
        }
    }
}
