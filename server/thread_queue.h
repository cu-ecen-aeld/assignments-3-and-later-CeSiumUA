#ifndef THREAD_QUEUE_H
#define THREAD_QUEUE_H

#include <sys/queue.h>
#include <pthread.h>

typedef struct node {
    void *data;
    TAILQ_ENTRY(node) nodes;
} node_t;

#endif // THREAD_QUEUE_H