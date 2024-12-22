#ifndef FRAME_QUEUE_H
#define FRAME_QUEUE_H

#include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>

#include "defs.h"

#define TO_VPP(var) ((void**)var)

typedef struct Node {
    void* data;
    struct Node* next;
} Node;

typedef struct {
    Node *head, *tail;

    int length;
    int size;
    int max_length;
    int* exit;
    char name[6];

    SDL_mutex* mutex;
    SDL_cond* cond_get;
    
    enum queue_data {
        QUEUE_FRAME,
        QUEUE_PACKET
    } data_type;
} Queue;

void queue_signal(Queue* q);

/**
* Initialize given Queue
* @param q      the queue to be initialized
* @param queue_type type of queue (QUEUE_FRAME/QUEUE_PACKET)
* @param exit   pointer to exit condition
* @param name   name of the queue (audio, video)
* @param max_length maximum length of the queue
*/
int queue_init(
    Queue *q,
    enum queue_data queue_type, 
    int* exit, char* name, int max_length
);

/**
* Frees allocated resources by queue_init.
* @param q  the queue to be freeded
*/
void queue_free(Queue* q);

/**
* Put given data into queue and NULL given pointer.
* @param q      the queue where to put the data
* @param data   void pointer pointer to data (either AVFrame or AVPacket)
*/
int queue_put(Queue *q, void** data);

/**
* Get data from queue and if block is true wait until
* data is available.
* @param q      the queue from where to get data
* @param data   pointer to put data on
* @param block  wait until data is available
*/
int queue_get(Queue *q, void** data, int block);

#endif // FRAME_QUEUE_H