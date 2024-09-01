#include "queue.h"

void queue_free(Queue* q) {
    Node* current_node = q->head;
    Node* prev_node;

    while (current_node) {
        if (q->data_type == QUEUE_FRAME) {
            av_frame_free((AVFrame**)&current_node->data);
        } else if (q->data_type == QUEUE_PACKET) {
            av_packet_free((AVPacket**)&current_node->data);
        }
        prev_node = current_node;
        current_node = current_node->next;
        av_free(prev_node);
    }
}

int queue_length(Queue* q) {
    return q->length;
}

int queue_init(Queue *q, enum queue_data queue_type, int* exit, char* name, int max_length) {
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
    q->exit = exit;
    q->length = 0;
    q->head = NULL;
    q->tail = NULL;
    q->max_length = max_length;

    strncpy(q->name, name, 6);
}

int queue_put(Queue *q, void** data) {

    int ret;
    SDL_LockMutex(q->mutex);
    if (q->length >= q->max_length) {
        SDL_UnlockMutex(q->mutex);
        if (q->data_type == QUEUE_FRAME) {
            AVFrame** frame = (AVFrame**)data;
    	    av_frame_free(frame);
        } else if (q->data_type == QUEUE_PACKET) {
            AVPacket** packet = (AVPacket**)data;
            av_packet_free(packet);
        }
        return -1;
    }
    
    Node* node = av_mallocz(sizeof(Node));
    if (!node) {
        ret = -1;
        goto exit;
    }

    node->data = *data;
    node->next = NULL;
    *data = NULL;

    if (!q->tail) {
        q->head = node;
    } else {
        q->tail->next = node;
    }
    q->tail = node;
    q->length++;

    SDL_CondSignal(q->cond);
    SDL_UnlockMutex(q->mutex);

    exit:
    if (DEBUG) {
        printf(
            "queue_get: type=%s, name=%s, len=%d\n",
            q->data_type == QUEUE_FRAME ? "frame" : "packet",
            q->name, q->length
        );
    }
    return ret;
}

int queue_get(Queue *q, void** data, int block) {

    int ret;
    Node *pkt1;

    if (SDL_LockMutex(q->mutex) < 0) {
        printf("queue_get: SDL_LockMutex -1\n");
        return -1;
    }
    for (;;) {
        if (*q->exit) {
            ret = -1;
            break;
        }

        pkt1 = q->head;
        if (!pkt1) {
            if (!block) {
                ret = 1;
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
        *data = pkt1->data;
        av_free(pkt1);
        ret = 0;
        break;
    }

    SDL_UnlockMutex(q->mutex);
    if (DEBUG) {
        printf(
            "queue_get: type=%s, name=%s, len=%d\n",
            q->data_type == QUEUE_FRAME ? "frame" : "packet",
            q->name, q->length
        );
    }
    return ret;
}