#define CUTE_C2_IMPLEMENTATION

#include "sot_collider.h"


void AppendCollider(sot_collider_node_t* list, sot_collider_t *collider) {
    sot_collider_node_t *new_node = malloc(sizeof(sot_collider_node_t));
    new_node->collider = collider;
    new_node->next = NULL;

    if (list == NULL)
        list = new_node;
    else {
        sot_collider_node_t *last_node = list;
        while (last_node->next != NULL)
            last_node = last_node->next;

        last_node->next = new_node;
    }

    return;
}

void AppendCollidersList(sot_collider_node_t** destination, sot_collider_node_t* colliders) {
    
    if (*destination == NULL)
        *destination = colliders;
    else {
        while ((*destination)->next != NULL) 
            (*destination) = (*destination)->next;
        (*destination)->next = colliders;
    } 

    return;
}

// Returns the total number of colliders currently present in the list.
int CollidersCount(sot_collider_node_t* colliders) {
    int count = 0;

    while (colliders != NULL) {
        colliders++;
        colliders = colliders->next;
    }

    return count;
}

void DrawCollidersDebugInfo(sot_collider_t pCollider, const AppState* appState) {
    if (!appState->debugInfo.displayColliders)
        return;

    // Debug info is rendered in red color
    SDL_SetRenderDrawColor(appState->pRenderer, 255, 0, 0, SDL_ALPHA_OPAQUE);

    switch (pCollider.type) {
        case C2_TYPE_AABB:
            {
                SDL_FRect rect = 
                {
                    .x = pCollider.shape.AABB.min.x,
                    .y = pCollider.shape.AABB.min.y,
                    .w = pCollider.shape.AABB.max.x - pCollider.shape.AABB.min.x,
                    .h = pCollider.shape.AABB.max.y - pCollider.shape.AABB.min.y
                };

                SDL_RenderRect(appState->pRenderer, &rect);
            } break;
        case C2_TYPE_CIRCLE:
            {
                c2v position = pCollider.shape.circle.p;
                float radius = pCollider.shape.circle.r;

                // creates a "circ7le" centered in the collider position approximate, with 36 segments
                SDL_FPoint circle[45];
                for (int i = 0; i <= 44; i++) {
                    double angle_radians = (8.0 / 180.0) * M_PI * i ;
                    circle[i].x = position.x + radius*cos(angle_radians);
                    circle[i].y = position.y + radius*sin(angle_radians);
                }
                SDL_RenderPoint(appState->pRenderer, position.x, position.y);
                SDL_RenderLines(appState->pRenderer, (const SDL_FPoint *)&circle, 45);
            } break;
        case C2_TYPE_POLY:
            {
                int size = pCollider.shape.poly.count;
                SDL_FPoint poly[size + 1];
                for (int i = 0; i < size; i++) {
                    poly[i].x = pCollider.shape.poly.verts[i].x;
                    poly[i].y = pCollider.shape.poly.verts[i].y;
                }

                poly[size].x = poly[0].x;
                poly[size].y = poly[0].y;

                for (int i = 0; i < size; i++) {
                    float min_x = glm_min(poly[i+1].x, poly[i].x);
                    float max_x = glm_max(poly[i+1].x, poly[i].x);
                    float min_y = glm_min(poly[i+1].y, poly[i].y);
                    float max_y = glm_max(poly[i+1].y, poly[i].y);
                    
                    SDL_FPoint nPos = {
                        .x = min_x + (max_x - min_x) / 2,
                        .y = min_y + (max_y - min_y) / 2,
                    };
                    SDL_FPoint nVector = {
                        .x = nPos.x + 10 * pCollider.shape.poly.norms[i].x,
                        .y = nPos.y + 10 * pCollider.shape.poly.norms[i].y
                    };

                    SDL_RenderLine(appState->pRenderer, nPos.x, nPos.y, nVector.x, nVector.y);
                }

                // render edges
                SDL_RenderLines(appState->pRenderer, (const SDL_FPoint *)&poly, size + 1);

            } break;
        default:
            break;
    }
    
}



void DestroyColliders(sot_collider_node_t* list) {

     if (list != NULL) {
        sot_collider_node_t *current_node = list;
        while (current_node != NULL) {
            sot_collider_node_t *next_node = current_node->next;
            free(current_node);
            current_node=next_node;
        }
    }
}
