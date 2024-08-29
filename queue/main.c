#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "queue.h"

int main() {
    Queue queue;
    queue_init(&queue, QUEUE_STR);

    char* pkt1 = (char*)av_mallocz(sizeof(char)*30);
    strncpy(pkt1, "hello", 30);
    printf("pkt1=%s\n", pkt1);
    queue_put(&queue, (void**)&pkt1);

    char* pkt2 = (char*)av_mallocz(sizeof(char)*30);
    strncpy(pkt2, "world", 30);
    printf("pkt2=%s\n", pkt2);
    queue_put(&queue, TO_VPP(&pkt2));
    
    queue_print(&queue);

    char* pkt3;
    queue_get(&queue, TO_VPP(&pkt3), 0);
    printf("pkt3=%s\n", pkt3);
    queue_print(&queue);
    av_free(pkt3);
    queue_free(&queue);

    Queue intqueue;
    queue_init(&intqueue, QUEUE_INT);

    int* pkt4 = (int*)av_mallocz(sizeof(int));
    *pkt4 = 420;
    printf("pkt4=%d\n", *pkt4);
    queue_put(&intqueue, TO_VPP(&pkt4));

    int* pkt5 = (int*)av_mallocz(sizeof(int));
    *pkt5 = 187;
    printf("pkt4=%d\n", *pkt5);
    queue_put(&intqueue, TO_VPP(&pkt5));

    queue_print(&intqueue);

    int* pkt6;
    queue_get(&intqueue, TO_VPP(&pkt6), 0);
    printf("pkt6=%d\n", *pkt6);
    av_free(pkt6);

    queue_print(&intqueue);

    queue_free(&intqueue);
    return 0;
}