#ifndef MDU_H
#define MDU_H

#include "du_worker.h"
#include "queue.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>

void handle_new_dir();
int handle_user_input(int argc, char* argv[], int* nthreads);
void worker_state_initialize(extended_Thread workers[], int nthreads, atomic_short* active_threads, atomic_long results[], sem_t* sem_queue, Queue* queued_entries );
void queue_initialize(Queue* q, char* path[], int size, sem_t* sem_queue);
pthread_t get_worker_thread(extended_Thread workers[], int size);
int all_workers_idle(extended_Thread workers[], int size);
void worker_join(extended_Thread workers[], int nthreads);
void slice(char* strings[], int start, int len, char* result[]);

#endif