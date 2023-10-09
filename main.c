#include <stdlib.h>
#include <stdbool.h>

#include "shared_data.h"
#include "optimizer.h"
#include "error_handling.h"
#include "debug.h"

#define ARGUMENT_COUNT (2)
#define ARGUMENT_BASE (10)

bool parseArguments(
    int argc,
    char *argv[],
    key_t *sharedMemoryKey,
    size_t *sharedMemorySize
) {
    DEBUG_ENTER_FUNC();
    char buffer[2048];

    if (argc - 1 != ARGUMENT_COUNT) {
        errno = EINVAL;
        snprintf(
            buffer,
            sizeof(buffer),
            "invalid argument count %d, should be %d",
            argc - 1,
            ARGUMENT_COUNT
        );
        PRINT_ERROR(buffer);
        DEBUG_EXIT_FUNC();
        return false;
    }

    errno = 0;
    *sharedMemoryKey = strtoull(argv[1], NULL, ARGUMENT_BASE);
    if (errno) {
        snprintf(
            buffer,
            sizeof(buffer),
            "invalid shared memory key %s",
            argv[1]
        );
        PRINT_ERROR(buffer);
        DEBUG_EXIT_FUNC();
        return false;
    }
    DEBUG_PRINT("Key: %d\n", *sharedMemoryKey);

    errno = 0;
    *sharedMemorySize = strtoull(argv[2], NULL, ARGUMENT_BASE);
    if (errno) {
        snprintf(
            buffer,
            sizeof(buffer),
            "invalid shared memory size %s",
            argv[2]
        );
        PRINT_ERROR(buffer);
        DEBUG_EXIT_FUNC();
        return false;
    }
    DEBUG_PRINT("Size: %ld\n", *sharedMemorySize);

    DEBUG_EXIT_FUNC();
    return true;
}

bool runOptimizer(SharedData *sharedData) {
    DEBUG_ENTER_FUNC();
    Optimizer *optimizer = optimizer_new(sharedData);
    if (!optimizer) {
        DEBUG_EXIT_FUNC();
        return false;
    }
    optimizer_optimize(optimizer);
    optimizer_delete(optimizer);

    DEBUG_EXIT_FUNC();
    return true;
}

int main(int argc, char *argv[]) {
    DEBUG_ENTER_FUNC();
    key_t sharedMemoryKey = 0;
    size_t sharedMemorySize = 0;
    if (!parseArguments(argc, argv, &sharedMemoryKey, &sharedMemorySize)) {
        DEBUG_EXIT_FUNC();
        return EXIT_FAILURE;
    }

    SharedData sharedData;
    if (!sharedData_attach(&sharedData, sharedMemoryKey, sharedMemorySize)) {
        DEBUG_EXIT_FUNC();
        return EXIT_FAILURE;
    }

    if (!runOptimizer(&sharedData)) {
        DEBUG_EXIT_FUNC();
        return EXIT_FAILURE;
    }

    if (!sharedData_detach(&sharedData)) {
        DEBUG_EXIT_FUNC();
        return EXIT_FAILURE;
    }

    DEBUG_EXIT_FUNC();
    return EXIT_SUCCESS;
}
