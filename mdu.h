/**
 *
 * This file contains the implementation of a multi-threaded application designed 
 * to process files concurrently. The application allows users to specify the number 
 * of threads to use for processing through command line arguments.
 *
 * The main components of the application include:
 * - **Queue**: A thread-safe queue for managing file paths to be processed.
 * - **Workers**: Threads that execute file processing tasks, utilizing the queue.
 * - **Main thread**: Manages workers using user input and displays the result.
 * 
 * The program processes files by splitting the workload across multiple threads, 
 * which enhances performance for large datasets. The results of processing are 
 * stored in an array, and the final output is printed upon successful completion 
 * of all tasks.
 *
 * @file mdu.c
 * @brief Multi-threaded file processing application.
 * @note The program requires the `-j` option to specify the number of threads. 
 *       If not provided, it defaults to one thread.
 *
 * To run:
 *   ./mdu [-j number_threads] file1 file2 ...
 *
 * @see queue.h for queue implementation details.
 * @see worker.h for worker thread management.
 *
 * This implementation is based on the MIT-licensed work found at:
 * https://github.com/petercrona/StsQueue/tree/master
 * 
 * @author Melker Henriksson
 * @date 2024/10/20
 */



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

/**
 * @brief Parses and validates user input for thread count and files.
 *
 * This function processes command-line arguments, extracts the number of threads 
 * specified with the `-j` option, and ensures the provided value is a valid positive 
 * integer. If the input is invalid or a usage error occurs, an error message is 
 * displayed and the program exits. The remaining command-line arguments after the 
 * `-j` option are considered file inputs.
 *
 * @param argc      The argument count, representing the total number of command-line arguments.
 * @param argv      Array of command-line arguments.
 * @param nthreads  Pointer to an integer that will store the number of threads specified by the user.
 *
 * @return The index of the first non-option argument (file).
 *
 * @note The function terminates the program if an invalid number of threads is provided 
 *       or if the number of threads is less than 1.
 */
int handle_user_input(int argc, char* argv[], int* nthreads);

/**
 * @brief Initializes worker threads and their arguments.
 *
 * Allocates and initializes the necessary structures for each worker thread, including 
 * argument data, and starts the threads. Each worker is associated with a shared mutex 
 * and various synchronization primitives for managing results and queue entries.
 *
 * @param workers          Array of extended_Thread structures representing the workers.
 * @param nthreads         Number of worker threads to initialize.
 * @param active_threads   Atomic variable tracking the number of active threads.
 * @param results          Array for storing results from the worker threads.
 * @param sem_queue        Semaphore for controlling access to the queue.
 * @param queued_entries   Pointer to the queue of entries to be processed by workers.
 *
 * @note A reference to allocated arguments is stored in `workers[i].args`. 
 *       This memory must be managed appropriately by the caller to prevent memory leaks.
 * @note The shared mutex is allocated within this function at 'workers[i].args.shared_mutex'
 *       and should be destroyed by the caller once all worker threads have completed execution.
 */

void worker_state_initialize(extended_Thread workers[], int nthreads, atomic_short* active_threads, atomic_long results[], sem_t* sem_queue, Queue* queued_entries );

/**
 * @brief Initializes a queue with a list of paths.
 *
 * Populates the queue by pushing each string from the `path` array into the queue.
 *
 * @param q          Pointer to the Queue structure to initialize.
 * @param path       Array of strings to be pushed into the queue.
 * @param size       Number of elements in the path array.
 * @param sem_queue  Semaphore used to manage access to the queue.
 * 
 * @note    push_q posts to the semaphore sem_queue 
*           dynamically initializing sem_queue
 *          
 */
void queue_initialize(Queue* q, char* path[], int size, sem_t* sem_queue);

/**
 * @brief Joins an array of worker threads, handles errors, and releases resources.
 *
 * This function waits for the specified worker threads to finish their execution, 
 * retrieves their return values. Status is assumed to be EXIT_SUCCESS on entry and
 * if an EXIT_FAILURE is detected it is assigned as status and no more checks are done. 
 * Associated resources are freed
 *
 * @param workers   Array of extended_Thread structures representing the worker threads.
 * @param nthreads  Number of worker threads to join.
 * @param status    Pointer to an integer where the exit status will be updated 
 *                  based on the return values of the worker threads. If any thread 
 *                  returns a non-success status, this will reflect that status.
 *
 * @note If a thread fails to join, the function prints an error message to stderr
 *       and terminates the program. The shared mutex is destroyed after all threads 
 *       are joined.
 */

void worker_join(extended_Thread workers[], int nthreads, int* status);

/**
 * @brief Extracts a slice of strings from the input array.
 *
 * Copies a subset of strings from the `strings` array, starting at the specified index
 * and spanning the given length, into the `result` array.
 *
 * @param strings  Array of input strings to slice.
 * @param start    Starting index for the slice.
 * @param len      Length of the slice (exclusive end index).
 * @param result   Array where the sliced strings will be stored.
 *
 * @note The caller must ensure that the `result` array is large enough to hold the slice.
 */
void slice(char* strings[], int start, int len, char* result[]);

#endif