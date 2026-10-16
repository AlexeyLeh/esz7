/**
    \file   main.c
    \author Aleksey Lekhtin
    \date   Mar 10, 2025
    \brief  Main procedure
*/

#ifndef MAIN__H
#define MAIN__H

#include <stdint.h>

typedef struct stat_t
{
    uint32_t send_ok;
    uint32_t send_error;
} stat_t;

#endif //MAIN__H
