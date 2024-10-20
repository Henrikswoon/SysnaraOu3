/**
 *
 * This file defines a thread safe queue data structure 
 * It includes functions for creating, destroying, 
 * pushing, and popping entries from the queue.
 *
 * This implementation is based on the MIT-licensed work found at:
 * https://github.com/petercrona/StsQueue/tree/master
 *
 * @file queue.h
 * @author Melker Henriksson
 * @date 2024/10/20
 * @brief Implementation of a thread-safe queue.
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <stdatomic.h>

typedef struct Entry {
    struct Entry *next;
    int index_working_size;
    char* path;
} Entry;

typedef struct Queue {
    Entry *head;
    Entry *tail;
    pthread_mutex_t mutex;
} Queue;


/**
 * @brief Creates and initializes a new queue.
 *
 * Allocates memory for a `Queue` structure, initializes its head and tail pointers, 
 * and sets up a mutex for thread-safe access to the queue.
 *
 * @return A pointer to the newly created `Queue`. 
 * 
 * @note The caller is responsible for freeing queue, mutex with `destroy_q` when 
 *       it is no longer needed to prevent memory leaks.
 */
Queue* create_q(void);

/**
 * @brief Destroys a queue and frees its resources.
 *
 * This function removes all entries from the queue by repeatedly calling `pop_q`, 
 * ensuring that all dynamically allocated memory for the entries is freed. 
 * After emptying the queue, it destroys the mutex associated with the queue 
 * and frees the queue structure itself.
 *
 * @param header Pointer to the `Queue` to be destroyed.
 * 
 */
void destroy_q(Queue *header);

/**
 * @brief Adds a new entry to the end of the queue.
 *
 * This function creates a new `Entry` from the provided string and adds it to the 
 * end of the queue. It ensures thread safety by locking the queue's mutex during 
 * the operation. If memory allocation for the new entry fails, the function 
 * destroys the queue and terminates the program.
 *
 * @param header           Pointer to the `Queue` where the entry will be added.
 * @param entry            The string to be added to the queue. This string will be 
 *                        duplicated and stored in the new entry.
 * @param sem              Pointer to the semaphore used to signal that a new entry 
 *                        has been added to the queue.
 * @param index_working_size The index associated with the entry, used for tracking 
 *                           purposes within the queue.
 *
 * @note The caller is responsible for ensuring that the `sem` semaphore is properly 
 *       initialized and managed. The memory allocated for the entry will be freed 
 *       when the entry is removed from the queue using `pop_q`.
 */
void push_q(Queue *header, char* entry, sem_t* sem, int index_working_size);

/**
 * @brief Removes and returns the entry at the front of the queue.
 *
 * This function retrieves the first entry from the queue, updates the queue's head 
 * pointer, and returns the string associated with the entry. The function ensures 
 * thread safety by locking the queue's mutex during the operation. If the queue is 
 * empty, it returns `NULL`. If `index_working_size` is not `NULL`, it will be set 
 * to the index associated with the entry being removed.
 *
 * @param header                Pointer to the `Queue` from which to pop the entry.
 * @param index_working_size    Pointer to the index which calculated result will be written to
 *
 * @return A pointer to the string of the removed entry, or `NULL` if the queue is empty.
 *
 * @note The caller is responsible for freeing the memory returned by this function 
 *       when it is no longer needed. The mutex must be properly managed to ensure 
 *       thread safety during queue operations.
 */
char* pop_q(Queue *header, int* index_working_size);

/**
 * @brief Checks if the queue is empty.
 *
 * This function determines whether the queue contains any entries by checking 
 * if the head pointer is `NULL`. It ensures thread safety by locking the queue's 
 * mutex during the check.
 *
 * @param header Pointer to the `Queue` to be checked.
 *
 * @return 1 if the queue is empty; 0 otherwise.
 */
int is_queue_empty(Queue* header);

#endif