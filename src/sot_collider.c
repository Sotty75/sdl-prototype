#define CUTE_C2_IMPLEMENTATION

#include <stdlib.h>
#include "sot_collider.h"

void AddCollider(sot_collider_node_t* list, sot_collider_t *collider) {
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

void DrawCollidersDebugInfo(sot_collider_t* pCollider, AppState* appState) {

    // Debug info is rendered in red color
    SDL_SetRenderDrawColor(appState->pRenderer, 255, 0, 0, SDL_ALPHA_OPAQUE);

    switch (pCollider->type) {
        case C2_TYPE_AABB:
            {
                SDL_FRect rect = 
                {
                    .x = pCollider->shape.AABB.min.x,
                    .y = pCollider->shape.AABB.min.y,
                    .w = pCollider->shape.AABB.max.x - pCollider->shape.AABB.min.x,
                    .h = pCollider->shape.AABB.max.y - pCollider->shape.AABB.min.y
                };

                SDL_RenderRect(appState->pRenderer, &rect);
            } break;
        case C2_TYPE_CIRCLE:
            {
                c2v position = pCollider->shape.circle.p;
                float radius = pCollider->shape.circle.r;

                // creates a "circ7le" centered in the collider position approximate, with 36 segments
                SDL_FPoint circle[2];
                for (int i = 0; i <= 1; i++) {
                    double angle_radians = 10*i * M_PI / 180.0;
                    circle[i].x = position.x + radius*cos(angle_radians);
                    circle[i].y = position.y + radius*sin(angle_radians);
                }
                SDL_RenderPoint(appState->pRenderer, position.x, position.y);
                // SDL_RenderLines(appState->pRenderer, (const SDL_FPoint *)&circle, 37);
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
