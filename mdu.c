#include "mdu.h"
int main(int argc, char* argv[]){
    int nthreads = 1;
    sem_t sem_queue;
    int optind = handle_user_input(argc, argv, &nthreads);
    extended_Thread workers[nthreads];
    atomic_short active_threads = nthreads;


    if(sem_init(&sem_queue, 0, 0) == -1){
        perror("semaphore");
        exit(EXIT_FAILURE);
    }

    Queue* queued_entries = create_q();
    
    int npaths = argc - optind;
    char* paths[npaths];
    slice(argv, optind, argc, paths);
    atomic_long results[npaths];
    queue_initialize(
        queued_entries, 
        paths, 
        npaths, 
        &sem_queue
    );

    worker_state_initialize(    
        workers, 
        nthreads, 
        &active_threads,
        results, 
        &sem_queue, 
        queued_entries
    );

    worker_join(workers, nthreads);
    destroy_q(queued_entries);

    for(int i = 0; i < npaths; i++){
        printf("arg: %s, size: %ld\n", paths[i], results[i]);
    } 
}

void worker_state_initialize(
        extended_Thread workers[], 
        int nthreads, 
        atomic_short* active_threads, 
        atomic_long results[], 
        sem_t* sem_queue, 
        Queue* queued_entries
    ){
    for(int i = 0; i < nthreads; i++){
        workers[i].state = RUNNING;

        WorkerArgs* args = malloc(sizeof(WorkerArgs));
        if(args == NULL){
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        args->results           = results;
        args->active_threads    = active_threads;
        args->sem_queue         = sem_queue;
        args->queued_entries    = queued_entries;
        args->self              = &workers[i];
        args->nthreads          = nthreads;
    
        int result = pthread_create(&workers[i].threadID, NULL, du_worker_thread, (void*)args);
        if(result != 0){
            //fprintf(stderr, "Error creating worker thread: %d", workers[i].threadID);
            exit(EXIT_FAILURE);
        }
    }
}

void queue_initialize(Queue* q, char* path[], int size, sem_t* sem_queue){
    for(int i = 0; i < size; i++) push_q(q, path[i], sem_queue, i);
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