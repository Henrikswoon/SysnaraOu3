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

typedef struct extended_Thread extended_Thread;

typedef struct {
    atomic_long* results;
    atomic_short* active_threads;
    sem_t* sem_queue;
    Queue* queued_entries;
    extended_Thread* self;
    int nthreads;
    pthread_mutex_t* shared_mutex;
} WorkerArgs;

struct extended_Thread {
    pthread_t threadID;
    WorkerArgs* args;
};

/**
 * @note Permission denied occupies the initial bit and determines whether permission was allowed. 
 */
typedef enum {
    PERMISSION_DENIED       = 1 << 0,
    TYPE_FILE               = 1 << 1,
    TYPE_DIR                = 1 << 2,
    TYPE_LNK                = 1 << 3,
    TYPE_UNKNOWN            = 1 << 4,

    DENIED_FILE             = TYPE_FILE | PERMISSION_DENIED,
    DENIED_DIR              = TYPE_DIR  | PERMISSION_DENIED,
    DENIED_LNK              = TYPE_LNK  | PERMISSION_DENIED, 
} ResourceType;

typedef struct {
    void* resource;
    ResourceType type;
} Resource;

void* du_worker_thread(void* args);
Resource open_resource(const char* path);
int handle_directory(DIR* dir, char* path, Queue* q, sem_t* sem_queue, int index_working_size);
int handle_file(char* path);
int getSize(struct stat path_stat);
void setType(int permission, Resource* r, ResourceType t);


#endif