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

void DrawCollidersDebugInfo(SOT_GPU_State *gpu, sot_collider_t pCollider) {
   
    switch (pCollider.type) {
        case C2_TYPE_AABB:
            {
                vec3 p0 = {pCollider.shape.AABB.min.x, pCollider.shape.AABB.min.y, 0};
                vec3 p1 = {pCollider.shape.AABB.max.x, pCollider.shape.AABB.min.y, 0};
                vec3 p2 = {pCollider.shape.AABB.max.x, pCollider.shape.AABB.max.y, 0};
                vec3 p3 = {pCollider.shape.AABB.min.x, pCollider.shape.AABB.max.y, 0};
                SOT_GPU_AddLine(gpu, p0, p1);
                SOT_GPU_AddLine(gpu, p1, p2);
                SOT_GPU_AddLine(gpu, p2, p3);
                SOT_GPU_AddLine(gpu, p3, p0);
            } break;
        case C2_TYPE_CIRCLE:
            {
                c2v position = pCollider.shape.circle.p;
                float radius = pCollider.shape.circle.r;

                // creates a "circle" centered in the collider position approximate, with 36 segments
                SDL_FPoint circle[45];
                for (int i = 0; i <= 44; i++) {
                    double angle_radians = (8.0 / 180.0) * M_PI * i ;
                    circle[i].x = position.x + radius*cos(angle_radians);
                    circle[i].y = position.y + radius*sin(angle_radians);
                }

                for (int i = 0; i < 44; i++) {
                    double angle_radians = (8.0 / 180.0) * M_PI * i ;
                    vec3 p0 = {circle[i].x, circle[i].y, 0};
                    vec3 p1 = {circle[i+1].x, circle[i+1].y, 0};
                    SOT_GPU_AddLine(gpu, p0, p1);
                }
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

                    // Render Normals
                    SOT_GPU_AddLine(gpu, (vec3) {nPos.x, nPos.y,0}, (vec3) {nVector.x, nVector.y,0});
                }

                // Render Edges
                for (int i = 0; i < size; i = i+1) {
                    SOT_GPU_AddLine(gpu, (vec3) {poly[i].x, poly[i].y,0}, (vec3) {poly[i+1].x, poly[i+1].y,0});
                }
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
