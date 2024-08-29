#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H

#include <string.h>
#include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>

enum QUEUE_DATA {
    QUEUE_INT,
    QUEUE_STR
};

typedef struct PacketNode {
    char* packet;
    struct PacketNode* next;
} PacketNode;

typedef struct {
    PacketNode *head, *tail;

    int nb_packets;
    int size;

    SDL_mutex* mutex;
    SDL_cond* cond;
} PacketQueue;

int packet_queue_init(PacketQueue *q);
void packet_queue_free(PacketQueue *q);
void packet_queue_print(PacketQueue *q);
int packet_queue_put(PacketQueue *q, char** pkt);
int packet_queue_get(PacketQueue *q, char** pkt, int block);

#endif