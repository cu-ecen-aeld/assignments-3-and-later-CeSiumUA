#include "thread_queue.h"

static node_t *head = NULL;

bool queue_enqueue(void *data){
    node_t *new_node = (node_t *) malloc(sizeof(node_t));
    if(new_node == NULL){
        syslog(LOG_ERR, "Error allocating memory for new node");
        return false;
    }
    new_node->data = data;
    new_node->next = NULL;

    if(head == NULL){
        head = new_node;
        return true;
    }

    node_t *current = head;
    while(current->next != NULL){
        current = current->next;
    }

    current->next = new_node;

    return true;
}

void *queue_dequeue(void){
    if(head == NULL){
        return NULL;
    }

    node_t *node = head;
    head = head->next;
    void *data = node->data;
    free(node);

    return data;
}

bool queue_remove(void *data){
    if(head == NULL){
        return false;
    }

    node_t *current = head;
    node_t *prev = NULL;

    while(current != NULL){
        if(current->data == data){
            if(prev == NULL){
                head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            return true;
        }
        prev = current;
        current = current->next;
    }

    return false;
}

node_t *queue_get_head(void){
    return head;
}