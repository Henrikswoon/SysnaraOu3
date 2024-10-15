#include "du_worker.h"

/**
 * @param args 
 * - atomic_long Tracks the total calculated size
 * - sem_t* sem_queue allows worker to wait for items to be added to the queue
 * - Queue* Thread safe queue
 * @brief
 *  du_worker_thread waits for items to be added to the queue
 *  When an item is added it handles it appropriately depending on type
 *  exits when signaled from main thread
 */
void* du_worker_thread(void* arg){
    WorkerArgs* args = (WorkerArgs*)arg;
    int i = 0;
    int finished = 0;
    while(++i){

        //Assume thread is going to wait.
        atomic_fetch_add(args->active_threads,-1);
        printf("\n\nactive threads: %hu\nis_queue_empty: %d\n\n", *args->active_threads, is_queue_empty(args->queued_entries));
        if (is_queue_empty(args->queued_entries) && *args -> active_threads == 0) {
            finished = 1;
            for(int i = 0; i < args->nthreads; i++) sem_post(args->sem_queue);
        }

        printf("===\n[%lu] waiting... iteration %d\n", (unsigned long) args->self->threadID, i);
        sem_wait(args->sem_queue);
        if(finished) pthread_exit(0);


        //Set thread as active.
        atomic_fetch_add(args->active_threads,+1);


        printf("[%lu] entering... iteration %d\n===\n", (unsigned long) args->self->threadID, i);
        args->self->state = RUNNING;
        
        char* path;

        //If the queue is empty, wait
        int index_working_size = 0;
        if((path = pop_q(args->queued_entries, &index_working_size)) == NULL){
            printf("[!q] queue was null, continue.\n");
            continue;
        } 

        Resource r = open_resource(path);
        if(r.type == TYPE_UNKNOWN){
            fprintf(stderr,"---\nFUCK!!!!!!!\n");
            exit(EXIT_FAILURE);
        } 

        int size;
        DIR* dir; 
        FILE* file;
        switch (r.type) {
            case TYPE_DIR:
                dir = (DIR*) r.resource;
                size = handle_directory(dir, path, args->queued_entries, args->sem_queue, index_working_size);
                closedir(dir);

                printf("[!!!!!] index_working_size: %d\n", index_working_size);
                atomic_fetch_add(&(args -> results[index_working_size]), size);
                break;
        
            case TYPE_FILE:
                file = (FILE*) r.resource;
                size = handle_file(path);
                fclose(file);
                
                atomic_fetch_add(&(args -> results[index_working_size]), size);
                break;

            default:
                fprintf(stderr, "Resource wasn't assigned a type, exiting.");
                exit(EXIT_FAILURE);
                break;
        }
    }
    free(args);
    return NULL; //Todo competent return
}

/**
 * Should push discovered files and directories to the queue
 */
int handle_directory(DIR* dir, char* base_path, Queue* q, sem_t* sem_queue, int index_working_size){
    printf("[worker] Handle directory: %s\n", base_path);
    int dir_size;
    struct dirent *dp;
    struct stat stat;
    if(lstat(base_path, &stat) == -1){
        perror("lstat");
        exit(EXIT_FAILURE);
    }
    dir_size = stat.st_size;

    while((dp = readdir(dir)) != NULL){
        //skip '.','..'
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0){
            continue;
        } 

        char full_path[pathconf("/", _PC_PATH_MAX)];
        snprintf(full_path, sizeof(full_path), "%s/%s", base_path, dp->d_name);

        free(base_path);
        push_q(q, full_path, sem_queue, index_working_size);
    }
    return dir_size;
}

int handle_file(char* path){
    struct stat stat;
    if(lstat(path, &stat) == -1){
        perror("lstat");
        exit(EXIT_FAILURE);
    }
    printf("[RESULT!] file at: %s, is size %ld\n", path, stat.st_blocks);
    free(path);
    return stat.st_size;
}


Resource open_resource(const char* path){
    printf("[worker] Opening resource: %s", path);
    struct stat path_stat;
    Resource r;
    r.resource  = NULL;
    r.type      = TYPE_UNKNOWN;

    if(lstat(path, &path_stat) == -1){
        perror("lstat");
        exit(EXIT_FAILURE);
    }

    if(S_ISDIR(path_stat.st_mode)){
        r.resource = opendir(path);
        if(r.resource == NULL){
            perror("opendir");
            exit(EXIT_FAILURE);
        } 
        else {
            printf(", TYPE_DIR\n");
            r.type = TYPE_DIR;
            return r;
        }
    }
    
    if(S_ISREG(path_stat.st_mode)){
        r.resource = fopen(path, "r");
        if(r.resource == NULL){
            perror("fopen");
            exit(EXIT_FAILURE);
        } 
        else {
            printf(", TYPE_FILE\n");
            r.type = TYPE_FILE;
            return r;
        }
    }
    
    printf("UNKNOWN!\n");
    return r;
 }

