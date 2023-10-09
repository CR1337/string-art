#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <error.h>
#include <sched.h>

#define NUM_ITERATIONS (1000000000)

typedef struct {
    double timeSpent;
    size_t coreIndex;
} Result;

int compareResults(const void *a, const void *b) {
    return ((Result*)a)->timeSpent - ((Result*)b)->timeSpent;
}

void __attribute__((optimize("O0"))) benchmarkCore(void) {
    size_t x = 0;
    for (size_t i = 0; i < NUM_ITERATIONS; i++) {
        if (i % 2 == 0) {
            x += i * 2;
        } else {
            x -= i / 2;
        }
        x &= 0x0F;
    }
}

void * benchmark(void *arg) {
    size_t coreIndex = *(int*)arg;
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(coreIndex, &cpuSet);
    int error = pthread_setaffinity_np(
        pthread_self(),
        sizeof(cpu_set_t),
        &cpuSet
    );
    if (error) {
        perror("pthread_setaffinity_np");
        exit(EXIT_FAILURE);
    }

    clock_t start = clock();
    benchmarkCore();
    clock_t end = clock();

    double *timeSpent = (double*)malloc(sizeof(double));
    *timeSpent = (double)(end - start) / CLOCKS_PER_SEC;

    pthread_exit(timeSpent);
}

int main(void) {
    size_t coreAmount = sysconf(_SC_NPROCESSORS_ONLN);
    pthread_t threads[coreAmount];
    size_t coreIndices[coreAmount];

    for (size_t i = 0; i < coreAmount; ++i) {
        coreIndices[i] = i;
        int error = pthread_create(
            &threads[i],
            NULL,
            benchmark,
            &coreIndices[i]
        );
        if (error) {
            perror("pthread_create");
            return EXIT_FAILURE;
        }
    }

    Result results[coreAmount];

    for (size_t i = 0; i < coreAmount; ++i) {
        double *timeSpent;
        int error = pthread_join(threads[i], (void**)&timeSpent);
        if (error) {
            perror("pthread_join");
            return EXIT_FAILURE;
        }
        results[i] = (Result){
            .timeSpent = *timeSpent,
            .coreIndex = i
        };
        free(timeSpent);
    }

    qsort(results, coreAmount, sizeof(Result), compareResults);

    for (size_t i = 0; i < coreAmount; ++i) {
        printf(
            "Core\t%ld:\t%.1f\n",
            results[i].coreIndex,
            results[i].timeSpent
        );
    }

    return EXIT_SUCCESS;
}
