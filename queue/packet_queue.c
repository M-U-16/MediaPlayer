#include "packet_queue.h"

int quit = 0;

void packet_queue_print(PacketQueue *q) {
    PacketNode* current_node = q->head;
    printf("Packet Queue:\n");
    while (current_node) {
        printf("->%s\n", current_node->packet);
        current_node = current_node->next;
    }
}

int packet_queue_init(PacketQueue *q) {
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    if (!q->mutex) {
        printf("packet_queue_init: SDL_CreateMutex %s\n", SDL_GetError());
        return -1;
    }

    q->cond = SDL_CreateCond();
    if (!q->cond) {
        printf("packet_queue_init: SDL_CreateCond %s\n", SDL_GetError());
        return -1;
    }
}

int packet_queue_put(PacketQueue *q, char** pkt) {
    PacketNode* node = av_mallocz(sizeof(PacketNode));
    if (!node) {
        return -1;
    }
    node->packet = *pkt;
    *pkt = NULL;
    node->next = NULL;

    SDL_LockMutex(q->mutex);

    if (!q->tail) {
        q->head = node;
    } else {
        q->tail->next = node;
    }
    q->tail = node;

    q->nb_packets++;
    q->size += strlen(node->packet) + 1;

    SDL_CondSignal(q->cond);
    SDL_UnlockMutex(q->mutex);

    return 0;
}

int packet_queue_get(
    PacketQueue *q,
    char** pkt,
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
            q->size -= strlen(pkt1->packet) - 1;
            *pkt = pkt1->packet;

            av_free(pkt1);
            ret = 1;
            break;
        } else if (!block) {
            ret = 0;
            break;
        }

        SDL_CondWait(q->cond, q->mutex);
    }

    SDL_UnlockMutex(q->mutex);
    return ret;
}

void packet_queue_free(PacketQueue* q) {
    PacketNode* current_node = q->head;
    PacketNode* prev_node;

    while (current_node) {
        av_free(current_node->packet);
        prev_node = current_node;
        current_node = current_node->next;
        av_free(prev_node);
    }
}