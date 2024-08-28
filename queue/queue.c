#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

typedef struct Node {
    struct Node* next;
    char* data;
} Node;

typedef struct {
    Node* head;
    Node* tail;
} LinkedList;

int linked_list_add(Node* node, char* data) {
    Node* currentNode = node;
    while (currentNode->next) {
        currentNode = currentNode->next;
    }
    Node* lastNode = currentNode;

    Node* newNode = malloc(sizeof(Node));
    if (!newNode) {
        fflush(stderr);
        printf("error allocating new node\n");
        return -1;
    }
    newNode->next = NULL;

    char* dataCopy = malloc(strlen(data) * sizeof(char) + 1);
    if (!dataCopy) {
        fflush(stderr);
        printf("error allocating data for new node\n");
        free(newNode);
        return -1;
    }
    strcpy(dataCopy, data);
    newNode->data = dataCopy;

    lastNode->next = newNode;

    return 0;
}

int linked_list_length(Node* node) {
    if (!node) return -1;
    if (!node->next) return 0;

    int length = 0;
    Node* currentNode = node->next;
    while (currentNode) {
        currentNode = currentNode->next;
        length++;
    }
    return length;
}

void linked_list_print(Node* node) {
    if (!node->next) {
        printf("--- empty list ---");
        return;
    }

    Node* currentNode = node->next;
    int len_list = linked_list_length(node);
    printf("Length: %d\n", len_list);
    printf("Head\n");

    while (currentNode) {
        printf("-> '%s'\n", currentNode->data);
        currentNode = currentNode->next;
    }
}

void linked_list_free(Node* node) {
    if (!node->next) {
        return;
    }
    
    Node* currentNode = node->next;
    Node* next = currentNode->next;

    while (currentNode) {
        printf("freeing node with data: '%s'\n", currentNode->data);
        free(currentNode->data);
        free(currentNode);
        currentNode = NULL;

        currentNode = next;
        if (currentNode)
            next = currentNode->next;
    }
}

int main() {
    LinkedList list;
    Node head = {
        .data = NULL, 
        .next = NULL
    };
    if (linked_list_add(&head, "hello world") < 0) {
        printf("error adding to linked list\n");
        goto free;
        return -1;
    }
    
    if (linked_list_add(&head, "hello world") < 0) {
        printf("error adding to linked list\n");
        goto free;
        return -1;
    }

    linked_list_print(&head);
    free:
    linked_list_free(&head);

    return 0;
}