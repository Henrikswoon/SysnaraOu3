#ifndef DU_WORKER_H
#define DU_WORKER_H

#include "queue.h"
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdatomic.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <limits.h>

typedef enum {
    NOT_RUNNING,
    RUNNING,
} ThreadState;

typedef struct {
    pthread_t threadID;
    ThreadState state;
} extended_Thread;

typedef struct {
    atomic_long* results;
    atomic_short* active_threads;
    sem_t* sem_queue;
    Queue* queued_entries;
    extended_Thread* self;
    int nthreads;
} WorkerArgs;

typedef enum {
    TYPE_FILE,
    TYPE_DIR,
    TYPE_UNKNOWN
} ResourceType;

typedef struct {
    void* resource;
    ResourceType type;
} Resource;

void* du_worker_thread(void* args);
Resource open_resource(const char* path);
int handle_directory(DIR* dir, char* path, Queue* q, sem_t* sem_queue, int index_working_size);
int handle_file(char* path);
int calculate();

#endif