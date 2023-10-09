#ifndef __ERROR_HANDLING_H__
#define __ERROR_HANDLING_H__

#include <stdio.h>
#include <errno.h>
#include <string.h>

#define __FILENAME__ (strrchr(__FILE__, '/') \
    ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define PRINT_ERROR(message) \
    { \
        char _print_error_buffer[2048]; \
        snprintf(_print_error_buffer, sizeof(_print_error_buffer), \
            "%s:%s:%d> %s", __FILENAME__, __func__, __LINE__, message); \
        perror(_print_error_buffer); \
    }
#define PRINT_ERROR_WITH_NUMBER(message, errorNumber) \
    { \
        errno = errorNumber; \
        PRINT_ERROR(message); \
    }
#define EXIT(returnCode) exit(returnCode)

#endif // __ERROR_HANDLING_H__
