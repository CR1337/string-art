#ifndef __WORKER_POOL_H__
#define __WORKER_POOL_H__

#define _GNU_SOURCE
#include <stddef.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>

typedef void (*Task)(
    void *argument,
    size_t workerIndex,
    size_t workerAmount
);

typedef struct WorkerPool WorkerPool;

typedef struct {
    WorkerPool *workerPool;
    size_t workerIndex;
} WorkerFunctionContext;

struct WorkerPool {
    size_t workerAmount;
    Task task;
    void *argument;
    pthread_t *workers;
    pthread_barrier_t barrier;
    sigset_t signalSet;
    WorkerFunctionContext *workerFunctionContexts;
    bool lockCores;
};

WorkerPool *workerPool_new(size_t workerAmount, bool lockCores);
void workerPool_delete(WorkerPool *self);

void workerPool_setTask(WorkerPool *self, Task task);
void workerPool_setArgument(WorkerPool *self, void *argument);

void workerPool_start(WorkerPool *self);
void workerPool_runTask(WorkerPool *self);
void workerPool_stop(WorkerPool *self);

size_t workerPool_coreAmount(void);

#endif // __WORKER_POOL_H__
