#include "mdu.h"
int main(int argc, char* argv[]){
    int nthreads = 1;
    sem_t sem_queue;
    atomic_long dir_size = 0;
    int optind = handle_user_input(argc, argv, &nthreads);
    extended_Thread workers[nthreads];

    if(sem_init(&sem_queue, 0, 0) == -1){
        perror("semaphore");
        exit(EXIT_FAILURE);
    }

    Queue* queued_entries = create_q();
    
    int npaths = argc - optind;
    char* paths[npaths];
    slice(argv, optind, argc, paths);
    queue_initialize(
        queued_entries, 
        paths, 
        npaths, 
        &sem_queue
    );
    
    int sem_value;
    if (sem_getvalue(&sem_queue, &sem_value) == -1) {
        perror("sem_getvalue");
    } else {
        printf("Semaphore current value: %d\n", sem_value);
    }

    worker_state_initialize(    
        workers, 
        nthreads, 
        &dir_size, 
        &sem_queue, 
        queued_entries
    );

    worker_join(workers, nthreads);
    destroy_q(queued_entries);
    
    printf("Total file size: %ld\n", dir_size);
}

void worker_state_initialize(extended_Thread workers[], int nthreads, atomic_long* dir_size, sem_t* sem_queue, Queue* queued_entries){
    for(int i = 0; i < nthreads; i++){
        workers[i].state = RUNNING;

        WorkerArgs* args = malloc(sizeof(WorkerArgs));
        if(args == NULL){
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        args->dir_size          = dir_size;
        args->sem_queue         = sem_queue;
        args->queued_entries    = queued_entries;
        args->self              = &workers[i];
    
        int result = pthread_create(&workers[i].threadID, NULL, du_worker_thread, (void*)args);
        if(result != 0){
            //fprintf(stderr, "Error creating worker thread: %d", workers[i].threadID);
            exit(EXIT_FAILURE);
        }
    }
}

void queue_initialize(Queue* q, char* path[], int size, sem_t* sem_queue){
    for(int i = 0; i < size; i++) push_q(q, path[i], sem_queue);
}

void slice(char* strings[], int start, int len, char* result[]){
    int size = len - start;

    for(int i = 0; i < size; i++){
        result[i] = strings[start + i];
    } 
}

//Todo! Fix catching results correctly.
void worker_join(extended_Thread workers[], int nthreads){
    for(int i = 0; i < nthreads; i++){
        void* status;
        pthread_join(workers[i].threadID, &status);
        printf("Joining %lu\n", (unsigned long) workers[i].threadID);
        if (status != 0) {
            exit(EXIT_FAILURE);
        }
    }
}

/*
pthread_t get_worker_thread(extended_Thread workers[], int size){
    int i = 0;
    while(workers[i].state != RUNNING){
        workers[i].state = RUNNING;
        return workers[i].threadID;
    }
    return NULL;
}
*/

int handle_user_input(int argc, char* argv[], int* nthreads){
    int opt;
    int i, isNum;
    isNum = 1;
    while((opt = getopt(argc, argv, "j:")) != -1){
        switch (opt)
        {
        case 'j':
            i = strlen(optarg) - 1;
            while(i >= 0){
                isNum = isdigit(optarg[i]);
                if (isNum) i--; 
                else i = -1;
            } 
            
            if(isNum) (*nthreads) = atoi(optarg);
            else{
                fprintf(stderr, "Provided number of threads was not a number, %s\n", optarg);
                exit(EXIT_FAILURE);
            }
            
            if((*nthreads) < 1){
                fprintf(stderr, "Number of threads must be greater than 1. \n");
                exit(EXIT_FAILURE);
            }
            break;
        
        default:
            fprintf(stderr, "Usage: mdu [-j number_threads] file ... \n");
            exit(EXIT_FAILURE);
        }
    }
    return optind;
}

int all_workers_idle(extended_Thread workers[], int size){
    for (int i = 0; i < size; i++) if(workers[i].state == RUNNING) return false;
    return true;
}