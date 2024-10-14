#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

typedef struct Entry {
    struct Entry *next;
    char* path;
} Entry;

typedef struct Queue {
    Entry *head;
    Entry *tail;
    pthread_mutex_t mutex;
} Queue;

Queue* create_q(void);
void destroy_q(Queue *header);
void push_q(Queue *header, char* entry, sem_t* sem);
char* pop_q(Queue *header);
int is_queue_empty(Queue* header);

#endif