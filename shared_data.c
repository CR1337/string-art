#include "shared_data.h"

#include "error_handling.h"
#include "debug.h"

#include <sys/shm.h>

#define SHARED_MEMORY_ACCESS_MODE (0666)

uint8_t * _input_data_initialize(InputData *data, uint8_t *memory) {
    DEBUG_ENTER_FUNC();
    data->header = (InputHeader*)memory;
    memory += sizeof(*(data->header));

    data->threads = (Thread*)memory;
    memory += sizeof(Thread) * data->header->indexer.threadAmount;

    data->threadOrder = (uint64_t*)memory;
    memory += sizeof(uint64_t) * data->header->threadOrderSize;

    data->startPoints = (uint64_t*)memory;
    memory += sizeof(uint64_t) * data->header->indexer.threadAmount;

    const uint64_t imageSize = data->header->imageWidth * data->header->imageWidth;

    data->target = (Color*)memory;
    memory += sizeof(Color) * imageSize;

    data->importance = (double*)memory;
    memory += sizeof(double) * imageSize;

    DEBUG_EXIT_FUNC();
    return memory;
}

void _output_data_initialize(
    OutputData *data,
    uint8_t *memory,
    InputHeader *inputHeader
) {
    DEBUG_ENTER_FUNC();

    data->header = (OutputHeader*)memory;
    memory += sizeof(data->header);

    const uint64_t imageSize = inputHeader->imageWidth * inputHeader->imageWidth;

    data->result = (Color*)memory;
    memory += sizeof(Color) * imageSize;

    data->instructions = (Instruction*)memory;
    memory += sizeof(Instruction) * inputHeader->termination.maxIterations;

    if (inputHeader->debugFlags) {
        const uint64_t debugArraySize = (
            imageSize * inputHeader->termination.maxIterations
        );

        data->debugData.images = (Color*)memory;
        memory += sizeof(Color) * debugArraySize;

        data->debugData.absoluteErrors = (uint64_t*)memory;
        memory += sizeof(uint64_t) * debugArraySize;
    } else {
        data->debugData.images = NULL;
        data->debugData.absoluteErrors = NULL;
    }

    DEBUG_EXIT_FUNC();
}

bool sharedData_attach(SharedData *sharedData, key_t key, size_t size) {
    DEBUG_ENTER_FUNC();

    int sharedMemoryId = shmget(key, size, SHARED_MEMORY_ACCESS_MODE);
    if (sharedMemoryId == -1) {
        PRINT_ERROR("error while getting shared memory id");
        DEBUG_EXIT_FUNC();
        return false;
    }

    sharedData->memory = shmat(sharedMemoryId, NULL, 0);
    if (sharedData->memory == (void*)-1) {
        PRINT_ERROR("error while attaching shared memory")
        DEBUG_EXIT_FUNC();
        return false;
    }

    uint8_t *memory = _input_data_initialize(
        &(sharedData->inputData), (uint8_t*)(sharedData->memory)
    );
    _output_data_initialize(
        &(sharedData->outputData), memory, sharedData->inputData.header
    );

    DEBUG_SET_IMAGE_WIDTH(sharedData->inputData.header->imageWidth);

    DEBUG_EXIT_FUNC();
    return true;
}

bool sharedData_detach(SharedData *sharedData) {
    DEBUG_ENTER_FUNC();
    // if (shmdt(sharedData->memory) == -1) {
    //     PRINT_ERROR("error while detaching shared memory");
    //     DEBUG_EXIT_FUNC();
    //     return false;
    // }
    DEBUG_EXIT_FUNC();
    return true;
}
