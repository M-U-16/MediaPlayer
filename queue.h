#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H

#include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>

#define TO_VPP(var) ((void**)var)

typedef struct Node {
    void* data;
    struct Node* next;
} Node;

typedef struct {
    Node *head, *tail;

    int length;
    int size;

    SDL_mutex* mutex;
    SDL_cond* cond;
    
    enum queue_data {
        QUEUE_FRAME,
        QUEUE_PACKET
    } data_type;
} Queue;

void queue_free(Queue* q);
int queue_put(Queue *q, void** data);
int queue_init(Queue *q, enum queue_data queue_type);
int queue_get(Queue *q, void** data, int block);

#endif // FRAME_QUEUE_H