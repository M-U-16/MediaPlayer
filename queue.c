#include "queue.h"

extern int quit;

void queue_free(Queue* q) {
    Node* current_node = q->head;
    Node* prev_node;

    while (current_node) {
        av_free(current_node->data);
        prev_node = current_node;
        current_node = current_node->next;
        av_free(prev_node);
    }
}

int queue_init(Queue *q, enum queue_data queue_type) {
    memset(q, 0, sizeof(Queue));
    q->mutex = SDL_CreateMutex();
    if (!q->mutex) {
        printf("queue_init: SDL_CreateMutex %s\n", SDL_GetError());
        return -1;
    }

    q->cond = SDL_CreateCond();
    if (!q->cond) {
        printf("queue_init: SDL_CreateCond %s\n", SDL_GetError());
        return -1;
    }

    q->data_type = queue_type;
}

int queue_put(Queue *q, void** data) {
    Node* node = av_mallocz(sizeof(Node));
    if (!node) {
        return -1;
    }

    node->data = *data;
    node->next = NULL;
    *data = NULL;

    SDL_LockMutex(q->mutex);

    if (!q->tail) {
        q->head = node;
    } else {
        q->tail->next = node;
    }
    q->tail = node;

    q->length++;
    if (q->data_type == QUEUE_FRAME) {
        AVFrame* data = (AVFrame*)node->data;
        q->size += data->pkt_size;
    } else if (q->data_type == QUEUE_PACKET) {
        AVPacket* packet = (AVPacket*)node->data;
        q->size += packet->size;
    }

    SDL_CondSignal(q->cond);
    SDL_UnlockMutex(q->mutex);

    return 0;
}

int queue_get(Queue *q, void** data, int block) {
    int ret;
    Node *pkt1;

    SDL_LockMutex(q->mutex);
    for (;;) {
        if (quit) {
            ret = -1;
            break;
        }

        pkt1 = q->head;
        if (!pkt1) {
            if (!block) {
                ret = 0;
                break;
            }

            SDL_CondWait(q->cond, q->mutex);
            continue;
        }
        
        q->head = pkt1->next;
        if (!q->head) {
            q->tail = NULL;
        }

        q->length--;
        if (q->data_type == QUEUE_FRAME) {
            AVFrame* data = (AVFrame*)pkt1->data;
            q->size -= data->pkt_size;
        } else if (q->data_type == QUEUE_PACKET) {
            AVPacket* packet = (AVPacket*)pkt1->data;
            q->size -= packet->size;
        }

        *data = pkt1->data;
        av_free(pkt1);
        ret = 1;
        break;
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}