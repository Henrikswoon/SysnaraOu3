#include "queue.h"
Queue* create_q(void)
{
    Queue *q = malloc(sizeof(Queue));
    if(q == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    q->head = NULL;
    q->tail = NULL;

    if (pthread_mutex_init(&q->mutex, NULL) != 0){
        free(q);
        exit(EXIT_FAILURE);
    }
    
    return q;
}

void destroy_q(Queue *header)
{
    while(pop_q(header, NULL) != NULL);
    if(header != NULL){
        pthread_mutex_destroy(&header->mutex);
        free(header);
    } 
    header = NULL;
}

void push_q(Queue *header, char* entry, sem_t* sem, int index_working_size)
{
    printf("[q]\tPushing '%s' to queue.\n", entry);
    Entry *e = malloc(sizeof(Entry));
    if(e == NULL){
        destroy_q(header);
        exit(EXIT_FAILURE);
    }

    e->index_working_size = index_working_size;
    e->path = strdup(entry);
    e->next = NULL;

    pthread_mutex_lock(&header->mutex);
    if (header->head == NULL)
    {
        header->head = e;
        header->tail = e;
    }
    else
    {
        Entry *previous_tail = header->tail;
        previous_tail->next = e;
        header->tail = e;
    }
    sem_post(sem);
    pthread_mutex_unlock(&header->mutex);
}

char* pop_q(Queue *header, int* index_working_size)
{
    pthread_mutex_lock(&header->mutex);
    Entry *head = header->head;
    if(index_working_size != NULL && head != NULL){
        *index_working_size = head -> index_working_size;  
    }
    if(head == NULL){
        pthread_mutex_unlock(&header->mutex);
        return NULL;
    }
    else{
        header -> head = head -> next;
        char* d = head -> path;

        printf("[q]\tPopping '%s' from the queue.\n", d);
        free(head);
        pthread_mutex_unlock(&header->mutex);
        return d;
    }
}

int is_queue_empty(Queue* header){
    pthread_mutex_lock(&header->mutex);
    int is_empty = (header->head == NULL);
    pthread_mutex_unlock(&header->mutex);

    return is_empty;
}