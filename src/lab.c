#include "lab.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

// Structure definition for the queue monitor
typedef struct queue {
    void **data;               // Array to hold queue elements
    int capacity;              // Maximum number of elements in the queue
    int front;                 // Index of the front element
    int rear;                  // Index for next insert position
    int count;                 // Current number of elements in the queue
    bool shutdown;             // Flag to indicate shutdown
    pthread_mutex_t lock;      // Mutex to ensure mutual exclusion
    pthread_cond_t not_full;   // Condition variable to signal queue is not full
    pthread_cond_t not_empty;  // Condition variable to signal queue is not empty
} *queue_t;

// Initializes the queue with a given capacity
queue_t queue_init(int capacity) {
    queue_t q = malloc(sizeof(struct queue));
    if (!q) return NULL;
    q->data = malloc(sizeof(void*) * capacity);
    if (!q->data) {
        free(q);
        return NULL;
    }
    q->capacity = capacity;
    q->front = 0;
    q->rear = 0;
    q->count = 0;
    q->shutdown = false;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_full, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    return q;
}

// Destroys the queue and frees all associated memory
void queue_destroy(queue_t q) {
    if (!q) return;
    pthread_mutex_lock(&q->lock);
    q->shutdown = true;
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    pthread_mutex_destroy(&q->lock);
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);
    free(q->data);
    free(q);
}

// Adds an element to the rear of the queue
void enqueue(queue_t q, void *data) {
    pthread_mutex_lock(&q->lock);
    // Wait while the queue is full and not shutting down
    while (q->count == q->capacity && !q->shutdown)
        pthread_cond_wait(&q->not_full, &q->lock);
    if (q->shutdown) {
        pthread_mutex_unlock(&q->lock);
        return;
    }
    q->data[q->rear] = data;
    q->rear = (q->rear + 1) % q->capacity;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

// Removes and returns the front element of the queue
void *dequeue(queue_t q) {
    pthread_mutex_lock(&q->lock);
    // Wait while the queue is empty and not shutting down
    while (q->count == 0 && !q->shutdown)
        pthread_cond_wait(&q->not_empty, &q->lock);
    if (q->count == 0 && q->shutdown) {
        pthread_mutex_unlock(&q->lock);
        return NULL;
    }
    void *data = q->data[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return data;
}

// Signals shutdown, waking all waiting threads
void queue_shutdown(queue_t q) {
    pthread_mutex_lock(&q->lock);
    q->shutdown = true;
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->lock);
}

// Checks if the queue is empty
bool is_empty(queue_t q) {
    pthread_mutex_lock(&q->lock);
    bool empty = (q->count == 0);
    pthread_mutex_unlock(&q->lock);
    return empty;
}

// Checks if the queue has been shutdown
bool is_shutdown(queue_t q) {
    pthread_mutex_lock(&q->lock);
    bool result = q->shutdown;
    pthread_mutex_unlock(&q->lock);
    return result;
}