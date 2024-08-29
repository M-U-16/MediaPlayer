#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H

#include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>

#define TO_VPP(var) ((void**)var)

enum queue_data {
    QUEUE_INT,
    QUEUE_STR
};

typedef struct Node {
    void* data;
    struct Node* next;
} Node;

typedef struct {
    Node *head, *tail;

    int nb_packets;
    int size;

    SDL_mutex* mutex;
    SDL_cond* cond;

    enum queue_data data_type;
} Queue;

void queue_print(Queue *q);
void queue_free(Queue* q);
int queue_put(Queue *q, void** frame);
int queue_get(Queue *q, void** pkt, int block);
int queue_init(Queue *q, enum queue_data data_type);

#endif // FRAME_QUEUE_H