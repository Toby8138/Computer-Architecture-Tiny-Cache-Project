#ifndef REQUEST_H
#define REQUEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct Request {
    uint32_t addr;
    uint32_t data;
    uint8_t w;
};

#ifdef __cplusplus
}
#endif

#endif