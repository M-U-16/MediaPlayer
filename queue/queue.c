#include "queue.h"

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

void queue_print(Queue *q) {
    Node* current_node = q->head;
    printf("Queue:\n");
    while (current_node) {
        if (q->data_type == QUEUE_INT) {
            printf("->%d\n", *(int*)current_node->data);
        } else if (q->data_type == QUEUE_STR) {
            printf("->%s\n", current_node->data);
        }
        current_node = current_node->next;
    }
}

int queue_init(Queue *q, enum queue_data data_type) {
    memset(q, 0, sizeof(Queue));
    q->mutex = SDL_CreateMutex();
    if (!q->mutex) {
        printf("frame_queue_init: SDL_CreateMutex %s\n", SDL_GetError());
        return -1;
    }

    q->cond = SDL_CreateCond();
    if (!q->cond) {
        printf("frame_queue_init: SDL_CreateCond %s\n", SDL_GetError());
        return -1;
    }

    q->data_type = data_type;
    return 0;
}

int queue_put(Queue *q, void** frame) {
    Node* node = av_mallocz(sizeof(Node));
    if (!node) {
        return -1;
    }

    node->data = *frame;
    node->next = NULL;
    *frame = NULL;

    SDL_LockMutex(q->mutex);

    if (!q->tail) {
        q->head = node;
    } else {
        q->tail->next = node;
    }
    q->tail = node;

    q->nb_packets++;
    if (q->data_type == QUEUE_INT) {
        int* data = (int*)node->data;
        q->size += sizeof(int);
    } else if (q->data_type == QUEUE_STR) {
        char* data = (char*)node->data;
        q->size += strlen(data)+1;
    }

    SDL_CondSignal(q->cond);
    SDL_UnlockMutex(q->mutex);

    return 0;
}

int queue_get(Queue *q, void** pkt, int block) {
    int ret;
    Node *pkt1;

    SDL_LockMutex(q->mutex);
    for (;;) {

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

        q->nb_packets--;
        if (q->data_type == QUEUE_INT) {
            int* data = (int*)pkt1->data;
            q->size -= sizeof(int);
        } else if (q->data_type == QUEUE_STR) {
            char* data = (char*)pkt1->data;
            q->size -= strlen(pkt1->data)+1;
        }

        *pkt = pkt1->data;
        av_free(pkt1);
        ret = 0;
        break;
    }

    SDL_UnlockMutex(q->mutex);
    return ret;
}