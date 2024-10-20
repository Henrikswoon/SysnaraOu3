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
    memset(results, 0, sizeof(atomic_long) * npaths);

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

    int status = EXIT_SUCCESS;
    worker_join(workers, nthreads, &status);
    sem_destroy(&sem_queue);
    destroy_q(queued_entries);

    for(int i = 0; i < npaths; i++){
        printf("%ld\t%s\n", results[i], paths[i]);
    } 
    return status;
}

void worker_state_initialize(
        extended_Thread workers[], 
        int nthreads, 
        atomic_short* active_threads, 
        atomic_long results[], 
        sem_t* sem_queue, 
        Queue* queued_entries
    ){
    pthread_mutex_t* shared_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(shared_mutex, NULL);
    for(int i = 0; i < nthreads; i++){
        workers[i].args = (WorkerArgs*) malloc(sizeof(WorkerArgs));
        if(workers[i].args == NULL){
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        workers[i].args->results            = results;
        workers[i].args->active_threads     = active_threads;
        workers[i].args->sem_queue          = sem_queue;
        workers[i].args->queued_entries     = queued_entries;
        workers[i].args->self               = &workers[i];
        workers[i].args->nthreads           = nthreads;
        workers[i].args->shared_mutex       = shared_mutex;
    
        int result = pthread_create(&workers[i].threadID, NULL, du_worker_thread, (void*) workers[i].args);
        if(result != 0){
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


void worker_join(extended_Thread workers[], int nthreads, int* status){
    pthread_mutex_t* shared_mutex = workers[0].args->shared_mutex;
    for(int i = 0; i < nthreads; i++){
        void* ret_val;
        int err;
        if((err = pthread_join(workers[i].threadID, &ret_val)) != 0){
            fprintf(stderr, "Failed to join thread %lu, err: %d", workers[i].threadID, err);
            exit(EXIT_FAILURE);
        }
        free(workers[i].args);
        if (ret_val != NULL) {
            //If a non-success has been detected, quit inspecting.
            if(*status == EXIT_SUCCESS)
                *status = *(int*)ret_val;
            free(ret_val);
        }
    }
    pthread_mutex_destroy(shared_mutex);
    free(shared_mutex);
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