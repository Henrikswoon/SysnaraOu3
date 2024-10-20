/**
 * @file du_worker.c
 * @brief Worker thread implementation for disk usage analysis.
 *
 * This file contains the implementation of worker threads responsible 
 * for analyzing disk usage by processing directories and files. 
 * Each thread reads from a shared queue of paths, determines the type 
 * of each path (file, directory, symlink, etc.), and computes the size 
 * of the resources. Results are stored in an array for each thread, 
 * and proper synchronization is ensured using semaphores and atomic 
 * variables.
 *
 * @note The processing of each resource type is handled within the worker thread, 
 *       including error handling for permission issues and unknown resource types.
 *
 * @see queue.h for queue management functions.
 * 
 * @author Melker Henriksson
 * @date 2024/10/20
 */

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
    TYPE_IGNORE             = 1 << 4,
    TYPE_UNKNOWN            = 1 << 5,

    DENIED_FILE             = TYPE_FILE | PERMISSION_DENIED,
    DENIED_DIR              = TYPE_DIR  | PERMISSION_DENIED,
    DENIED_LNK              = TYPE_LNK  | PERMISSION_DENIED, 
} ResourceType;

typedef struct {
    void* resource;
    ResourceType type;
} Resource;

/**
 * @brief The main function executed by each worker thread for disk usage analysis.
 *
 * This function retrieves file paths from a shared queue and processes each path 
 * to calculate disk usage. It handles different resource types, including files, 
 * directories, and symlinks, and updates the results accordingly. 
 * Each thread operates in a loop, waiting for paths to become available in the 
 * queue, and it synchronizes access to shared resources using semaphores and 
 * atomic variables.
 *
 * The function will:
 * - Wait for work to be available by using a semaphore.
 * - Process each path by determining its type and calling the appropriate handling 
 *   function (e.g., `handle_file`, `handle_directory`).
 * - Manage the state of active threads and update the results based on the processed 
 *   paths.
 *
 * @param arg A pointer to a `WorkerArgs` structure containing the worker's 
 *            arguments, including the shared queue, results array, 
 *            and synchronization primitives.
 */
void* du_worker_thread(void* args);

/**
 * @brief Opens a resource at the specified path and determines its type.
 *
 * This function checks the file system object at the given path, retrieves its 
 * status information, and categorizes it into one of several resource types: 
 * regular file, directory, symbolic link, or ignored types (character device, 
 * block device, FIFO). It also checks for read permissions on the resource.
 *
 * The function performs the following operations:
 * - Retrieves the status of the resource using `lstat`.
 * - Determines resource type and permission.
 * - The function updates the `Resource` structure with the appropriate 
 *   type and resource pointer.
 *
 * @param path A pointer to a null-terminated string representing the 
 *             path to the resource to be opened.
 *
 * @return A `Resource` structure containing:
 *         - `resource`: A pointer to the opened directory or a path.
 *         - `type`: The determined type of the resource (e.g., 
 *           `TYPE_FILE`, `TYPE_DIR`, `TYPE_LNK`, or `TYPE_IGNORE`).
 *         The type will also indicate if permission was denied 
 *         using the least significant bit.
 */
Resource open_resource(const char* path);

/**
 * @brief Processes a directory and queues its contents for further handling.
 *
 * This function takes a directory stream and iterates through its entries. For 
 * each entry that is not `.` or `..`, it constructs the full path and adds it 
 * to a queue for processing by worker threads. It also retrieves the size of the 
 * directory based on the provided base path.
 *
 * The function performs the following operations:
 * - Checks if the directory stream is valid.
 * - Retrieves the status of the base path using `lstat` to determine its size.
 * - Iterates through the directory entries using `readdir`.
 * - Constructs the full path for each entry and adds it to the queue via `push_q`.
 *
 * @param dir A pointer to the directory stream to be processed.
 * @param base_path A pointer to a null-terminated string representing the 
 *                  base path of the directory being processed.
 * @param q A pointer to the `Queue` structure where the entries will be queued.
 * @param sem_queue A pointer to a semaphore used to synchronize access to the queue.
 * @param index_working_size An integer representing the index associated with 
 *                           the current working size for the entries being queued.
 *
 * @return The size of the directory in blocks as obtained from the `lstat` call.
 *         Returns 0 if the directory stream is NULL.
 */
int handle_directory(DIR* dir, char* path, Queue* q, sem_t* sem_queue, int index_working_size);

/**
 * @brief Retrieves the size of a file.
 *
 * This function takes the path to a file, retrieves its status using `lstat`,
 * and returns the size of the file in blocks.
 *
 * @param path A pointer to a null-terminated string representing the path 
 *             to the file whose size is to be determined.
 *
 * @return The size of the file in blocks as retrieved from the `getSize` function.
 * 
 */
int handle_file(char* path);

/**
 * @brief Retrieves the size of a file or directory in blocks.
 *
 * This function takes a `stat` structure that contains information about a 
 * file or directory and returns its size in blocks.
 *
 * @param path_stat A `struct stat` that contains information about the 
 *                  file or directory whose size is to be retrieved.
 *
 * @return The size of the file or directory in blocks.
 */
int getSize(struct stat path_stat);

/**
 * @brief Sets the type of a resource based on its permissions.
 *
 * This function assigns a resource type to the given `Resource` structure.
 * If the specified permissions are granted, the type is set directly to the 
 * provided type. If permissions are denied, the `PERMISSION_DENIED` flag 
 * is combined with the provided type to indicate restricted access.
 *
 * @param permission An integer indicating whether the resource has the 
 *                   required permissions (non-zero if granted, zero if denied).
 * @param r A pointer to a `Resource` structure where the type will be set.
 * @param t The `ResourceType` to assign to the resource.
 *
 */
void setType(int permission, Resource* r, ResourceType t);

#endif