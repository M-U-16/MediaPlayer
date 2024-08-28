#ifndef PACKET_QUEUE_H
#define PACKET_QUEUE_H

#include <SDL2/SDL.h>
#include <libavcodec/avcodec.h>

typedef struct PacketNode {
    AVPacket packet;
    struct PacketNode* next;
} PacketNode;

typedef struct {
    PacketNode *head, *tail;

    int nb_packets;
    int size;

    SDL_mutex* mutex;
    SDL_cond* cond;
} PacketQueue;

void packet_queue_init(PacketQueue *q);
void packet_queue_put(PacketQueue *q, AVPacket *pkt);
int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block);

#endif