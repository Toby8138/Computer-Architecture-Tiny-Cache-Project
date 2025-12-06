#ifndef RESULT_H
#define RESULT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct Result {
    uint32_t cycles;
    uint32_t misses;
    uint32_t hits;
};

#ifdef __cplusplus
}
#endif

#endif