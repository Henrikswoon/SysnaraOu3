#include "du_worker.h"
void* du_worker_thread(void* arg){
    WorkerArgs* args = (WorkerArgs*)arg;
    int finished = 0;
    char* path = NULL;
    int* status = (int*)malloc(sizeof(int));
    *status = EXIT_SUCCESS;
    while(1){

        //Assume thread is going to wait.
        atomic_fetch_add(args->active_threads,-1);
        if (is_queue_empty(args->queued_entries) && *args -> active_threads == 0) {
            finished = 1;
            for(int i = 0; i < args->nthreads; i++) sem_post(args->sem_queue);
        }

        sem_wait(args->sem_queue);
        
        if(finished){
            free(path);
            pthread_exit((void*)status);
        } 

        //Set thread as active.
        atomic_fetch_add(args->active_threads,+1);

        //If the queue is empty, wait
        int index_working_size = 0;
        if(path != NULL) free(path);
        if((path = pop_q(args->queued_entries, &index_working_size)) == NULL){
            continue;
        } 

        Resource r = open_resource(path);
        if(r.type == TYPE_UNKNOWN){
            fprintf(stderr,"resource at %s was of an unexpected type, exiting.\n", path);
            exit(EXIT_FAILURE);
        } 

        int size;
        DIR* dir; 
        switch (r.type) {
            case TYPE_DIR:
                dir = (DIR*) r.resource;
                size = handle_directory(dir, path, args->queued_entries, args->sem_queue, index_working_size);
                closedir(dir);
                atomic_fetch_add(&(args -> results[index_working_size]), size);
                break;
            
            case DENIED_DIR:
                fprintf(stderr, "du: cannot read directory '%s': Permission denied\n", path);
                size = handle_file(path);
                atomic_fetch_add(&(args -> results[index_working_size]), size);
                (*status) = EXIT_FAILURE;
                break;


            case TYPE_FILE:
                size = handle_file(path);                
                atomic_fetch_add(&(args -> results[index_working_size]), size);
                break;

            case DENIED_FILE:
                size = handle_file(path);
                atomic_fetch_add(&(args -> results[index_working_size]), size);
                break;

            case TYPE_LNK:
                size = handle_file(path);
                atomic_fetch_add(&(args -> results[index_working_size]), size);
                break;

            case DENIED_LNK:
                size = handle_file(path);
                atomic_fetch_add(&(args -> results[index_working_size]), size);
                break;

            case TYPE_IGNORE:
                break;

            default:
                fprintf(stderr, "Resource wasn't assigned a type, exiting.\n");
                exit(EXIT_FAILURE);
                break;
        }
    }
    free(args);
    return NULL; //Todo competent return
}

int handle_directory(DIR* dir, char* base_path, Queue* q, sem_t* sem_queue, int index_working_size){
    if (dir == NULL) return 0;
    struct dirent *dp;

    while((dp = readdir(dir)) != NULL){
        //skip '.','..'
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0){
            continue;
        } 

        char full_path[pathconf("/", _PC_PATH_MAX)];
        snprintf(full_path, sizeof(full_path), "%s/%s", base_path, dp->d_name);

        push_q(q, full_path, sem_queue, index_working_size);
    }

    int dir_size;
    struct stat stat;
    if(lstat(base_path, &stat) == -1){
        perror("lstat");
        exit(EXIT_FAILURE);
    }
    dir_size = getSize(stat);
    return dir_size;
}

int handle_file(char* path){
    struct stat stat;
    if(lstat(path, &stat) == -1){
        perror("lstat");
        exit(EXIT_FAILURE);
    }
    return getSize(stat);
}

Resource open_resource(const char* path){
    struct stat path_stat;
    Resource r;
    int permission = access(path, R_OK) == 0;     
    r.resource  = NULL;
    r.type      = TYPE_UNKNOWN;
    
    if(lstat(path, &path_stat) == -1){
        perror("lstat");
        exit(EXIT_FAILURE);
    }
    
    else if(S_ISLNK(path_stat.st_mode)){
        setType(permission, &r, TYPE_LNK);
        return r;
    }
    
    else if(S_ISDIR(path_stat.st_mode)){
        if(!permission){
            setType(permission, &r, TYPE_DIR);
            return r;
        }else{
            r.resource = opendir(path);
            if(r.resource == NULL){
                perror("opendir");
                exit(EXIT_FAILURE);
            } 
            else {
                setType(permission, &r, TYPE_DIR);
                return r;
            }
        } 
    }
    
    else if(S_ISREG(path_stat.st_mode)){
        setType(permission, &r, TYPE_FILE);
        return r;
    }

    //Ignore CHR,BLK,FIFO.

    else if(S_ISCHR(path_stat.st_mode)  || S_ISBLK(path_stat.st_mode) ||
            S_ISFIFO(path_stat.st_mode)){
        setType(permission, &r, TYPE_IGNORE);
        return r;
    }

    return r;
}

inline void setType(int permission, Resource* r, ResourceType t){
    if(permission) r->type = t;
    else r->type = t | PERMISSION_DENIED; 
}

inline int getSize(struct stat path_stat){
    return path_stat.st_blocks;
}