#include "worker_pool.h"
#include "error_handling.h"
#include "debug.h"

#include <stdlib.h>
#include <unistd.h>
#include <sched.h>

#define WAKEUP_SIGNAL (SIGUSR1)
#define TERMINATION_SIGNAL (SIGUSR2)

WorkerPool *workerPool_new(size_t workerAmount, bool lockCores) {
    DEBUG_ENTER_FUNC();
    WorkerPool *workerPool = (WorkerPool*)malloc(sizeof(WorkerPool));
    if (!workerPool) {
        PRINT_ERROR("error allocating workerPool");
        DEBUG_EXIT_FUNC();
        return NULL;
    }

    workerPool->workerAmount = workerAmount;
    workerPool->task = NULL;
    workerPool->argument = NULL;
    workerPool->lockCores = lockCores;

    workerPool->workers = (pthread_t*)calloc(workerAmount, sizeof(pthread_t));
    if (!workerPool->workers) {
        free(workerPool);
        PRINT_ERROR("error allocating workerPool->workers");
        DEBUG_EXIT_FUNC();
        return NULL;
    }

    workerPool->workerFunctionContexts = (WorkerFunctionContext*)malloc(workerAmount * sizeof(WorkerFunctionContext));
    if (!workerPool->workerFunctionContexts) {
        free(workerPool->workers);
        free(workerPool);
        PRINT_ERROR("error allocating workerPool->workerFunctionContexts");
        DEBUG_EXIT_FUNC();
        return NULL;
    }

    int error = pthread_barrier_init(&(workerPool->barrier), NULL, workerAmount + 1);
    if (error) {
        free(workerPool->workerFunctionContexts);
        free(workerPool->workers);
        free(workerPool);
        PRINT_ERROR_WITH_NUMBER("error initializing workerPool->barrier", error);
        DEBUG_EXIT_FUNC();
        return NULL;
    }

    sigemptyset(&(workerPool->signalSet));
    sigaddset(&(workerPool->signalSet), WAKEUP_SIGNAL);
    sigaddset(&(workerPool->signalSet), TERMINATION_SIGNAL);
    error = pthread_sigmask(SIG_BLOCK, &(workerPool->signalSet), NULL);
    if (error) {
        pthread_barrier_destroy(&(workerPool->barrier));
        free(workerPool->workerFunctionContexts);
        free(workerPool->workers);
        free(workerPool);
        PRINT_ERROR_WITH_NUMBER("error setting signal mask", error);
        DEBUG_EXIT_FUNC();
        return NULL;
    }

    for (size_t i = 0; i < workerAmount; ++i) {
        workerPool->workerFunctionContexts[i] = (WorkerFunctionContext){
            .workerPool = workerPool,
            .workerIndex = i
        };
    }

    DEBUG_EXIT_FUNC();
    return workerPool;
}

void workerPool_delete(WorkerPool *self) {
    DEBUG_ENTER_FUNC();
    pthread_barrier_destroy(&(self->barrier));
    free(self->workerFunctionContexts);
    free(self->workers);
    free(self);
    DEBUG_EXIT_FUNC();
}

void workerPool_setTask(WorkerPool *self, Task task) {
    DEBUG_ENTER_FUNC();
    self->task = task;
    DEBUG_EXIT_FUNC();
}

void workerPool_setArgument(WorkerPool *self, void *argument) {
    DEBUG_ENTER_FUNC();
    self->argument = argument;
    DEBUG_EXIT_FUNC();
}

void * _workerPool_workerFunction(void *context) {
    DEBUG_ENTER_FUNC();
    WorkerPool *self = ((WorkerFunctionContext*)context)->workerPool;
    size_t workerIndex = ((WorkerFunctionContext*)context)->workerIndex;

    if (self->lockCores) {
        cpu_set_t cpuSet;
        CPU_ZERO(&cpuSet);
        CPU_SET(workerIndex, &cpuSet);
        int error = pthread_setaffinity_np(
            self->workers[workerIndex],
            sizeof(cpu_set_t),
            &cpuSet
        );
        if (error) {
            PRINT_ERROR_WITH_NUMBER("error setting CPU affinity", error);
            DEBUG_EXIT_FUNC();
            EXIT(EXIT_FAILURE);
        }
    }

    int signal = 0;
    while (true) {
        int error = sigwait(&(self->signalSet), &signal);
        if (error) {
            PRINT_ERROR_WITH_NUMBER("error waiting for signal", error);
            DEBUG_EXIT_FUNC();
            EXIT(EXIT_FAILURE);
        }
        switch (signal) {
            case WAKEUP_SIGNAL:
                self->task(self->argument, workerIndex, self->workerAmount);
                printf("Task Done\n");
                break;
            case TERMINATION_SIGNAL:
                DEBUG_EXIT_FUNC();
                return NULL;
        }
        DEBUG_PRINT("encountered barrier %ld\n", workerIndex);
        pthread_barrier_wait(&(self->barrier));
    }

    DEBUG_EXIT_FUNC();
    return NULL;
}

void workerPool_start(WorkerPool *self) {
    DEBUG_ENTER_FUNC();
    for (size_t i = 0; i < self->workerAmount; ++i) {
        int error = pthread_create(
            self->workers + i,
            NULL,
            _workerPool_workerFunction,
            self->workerFunctionContexts + i
        );
        if (error) {
            PRINT_ERROR_WITH_NUMBER("error creating thread", error);
            DEBUG_EXIT_FUNC();
            EXIT(EXIT_FAILURE);
        }
    }
    DEBUG_EXIT_FUNC();
}

void workerPool_runTask(WorkerPool *self) {
    DEBUG_ENTER_FUNC();
    DEBUG_ENTER_WORKER_MODE();
    for (size_t i = 0; i < self->workerAmount; ++i) {
        int error = pthread_kill(self->workers[i], WAKEUP_SIGNAL);
        if (error) {
            PRINT_ERROR_WITH_NUMBER("error sending WAKEUP_SIGNAL", error);
            DEBUG_ENTER_MAIN_THREAD_MODE();
            DEBUG_EXIT_FUNC();
            EXIT(EXIT_FAILURE);
        }
    }
    int error = pthread_barrier_wait(&(self->barrier));
    if (error) {
        PRINT_ERROR_WITH_NUMBER("error waiting for barrier in main thread", error);
        DEBUG_ENTER_MAIN_THREAD_MODE();
        DEBUG_EXIT_FUNC();
        EXIT(EXIT_FAILURE);
    }
    DEBUG_ENTER_MAIN_THREAD_MODE();
    DEBUG_EXIT_FUNC();
}

void workerPool_stop(WorkerPool *self) {
    DEBUG_ENTER_FUNC();
    for (size_t i = 0; i < self->workerAmount; ++i) {
        int error = pthread_kill(self->workers[i], TERMINATION_SIGNAL);
        if (error) {
            PRINT_ERROR_WITH_NUMBER("error sending TERMINATION_SIGNAL", error);
            DEBUG_EXIT_FUNC();
            EXIT(EXIT_FAILURE);
        }
    }
    for (size_t i = 0; i < self->workerAmount; ++i) {
        int error = pthread_join(self->workers[i], NULL);
        if (error) {
            PRINT_ERROR_WITH_NUMBER("error joining worker", error);
            DEBUG_EXIT_FUNC();
            EXIT(EXIT_FAILURE);
        }
    }
    DEBUG_EXIT_FUNC();
}

size_t workerPool_coreAmount(void) {
    DEBUG_ENTER_FUNC();
    size_t result = sysconf(_SC_NPROCESSORS_ONLN);
    DEBUG_EXIT_FUNC();
    return result;
}