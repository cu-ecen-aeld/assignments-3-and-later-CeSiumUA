#ifndef THREAD_QUEUE_H
#define THREAD_QUEUE_H

#include <pthread.h>
#include <stdbool.h>
#include <syslog.h>

typedef struct node {
    void *data;
    node_t *next;
} node_t;

bool queue_enqueue(void *data);
void *queue_dequeue(void);
bool queue_remove(void *data);
node_t *queue_get_head(void);

#endif // THREAD_QUEUE_H