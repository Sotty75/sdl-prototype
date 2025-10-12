#define CUTE_C2_IMPLEMENTATION

#include <stdlib.h>
#include "sot_collider.h"

void AddCollider(sot_colliders_list_t* list, sot_collider_t collider) {
    sot_collider_node_t *new_node = malloc(sizeof(sot_collider_node_t));
    new_node->collider = collider;
    new_node->next = NULL;

    if (list->colliderNode == NULL)
        list->colliderNode = new_node;
    else {
        sot_collider_node_t *last_node = list->colliderNode;
        while (last_node->next != NULL)
            last_node = last_node->next;

        last_node->next = new_node;
    }

    return;
}

void ClearList(sot_colliders_list_t* list) {
    
    if (list->colliderNode != NULL) {
        sot_collider_node_t *current_node = list->colliderNode;
        while (current_node != NULL) {
            sot_collider_node_t *next_node = current_node->next;
            free(current_node);
            current_node=next_node;
        }
    }

    return;
}

void DestroyList(sot_colliders_list_t* list) {
    ClearList(list);
    free(list);
}
