#ifndef STATE__H
#define STATE__H

#include "utils.h"

typedef struct __state_t
{
    int remote;
    bool localEcho;
    bool quiet;
} state_t;

#endif