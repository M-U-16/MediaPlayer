#include "packet_queue.h"

extern int quit;

void packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt) {
    PacketNode* node = av_malloc(sizeof(PacketNode));
    if (!node) {
        return -1;
    }

    node->packet = *pkt;
    node->next = NULL;

    SDL_LockMutex(q->mutex);

    if (!q->tail) {
        q->head = node;
    } else {
        q->tail->next = node;
    }
    q->tail = node;

    q->nb_packets++;
    q->size += node->packet.size;

    SDL_CondSignal(q->cond);
    SDL_UnlockMutex(q->mutex);

    return 0;
}

int packet_queue_get(
    PacketQueue *q,
    AVPacket* pkt,
    int block
) {
    int ret;
    PacketNode *pkt1;

    SDL_LockMutex(q->mutex);
    for (;;) {
        if (quit) {
            ret = -1;
            break;
        }

        pkt1 = q->head;
        if (pkt1) {
            q->head = pkt1->next;
            if (!q->head) {
                q->tail = NULL;
            }

            q->nb_packets--;
            q->size -= pkt1->packet.size;
            *pkt = pkt1->packet;
            av_free(pkt1);

            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        } else {
            SDL_CondWait(q->cond, q->mutex);
        }
    }

    SDL_UnlockMutex(q->mutex);
    return ret;
}