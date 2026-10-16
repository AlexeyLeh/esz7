/**
  \file   core_t.h
  \author Aleksey Lekhtin
  \date   Mar 10, 2025
  \brief  Структуры данных для работы с ESZ7
*/

#ifndef ESZ7_T__H
#define ESZ7_T__H

#include "core_t.h"
#include "thr_t.h"
#include "socket_t.h"

#define MAX_NPNTS 500

typedef struct points_t {
    int channel;
    int32_t pointsA[MAX_NPNTS];        // точки измерений
    int32_t pointsB[MAX_NPNTS];        // точки измерений
    int32_t *R;
    int32_t *W;
    int64_t sum;
    int32_t avrg;
} points_t;

typedef struct core_data_t
{
    int32_t HKLD24bit;              // A2D для веса (Hookload)
    int16_t HKLD16bit;              // A2D downscaled to 16 bit для веса (Hookload)
    int32_t  encoder1;              // энкодер 1 - для положения (BPOS)
    int32_t  encoder2;              // энкодер 2
    int      max_npnts;             // максимально допустимое число измерений (500)
    // TODO points_t raw2[4];               // точки измерений  64 bit?
    points_t values[4];            // точки измерений
    int toggle[4];
    pthread_mutex_t mutex;
} core_data_t;

typedef struct core_t {
    thr_t *thr;
    int npnts;                      // число измерений, забираемых с АЦП
    core_data_t data;
} core_t;

typedef struct core_cmd_t {
    char name[32];
    int port;
    socket_t sock;
    thr_t *cli_thr;
    pthread_mutex_t mutex;
} core_cmd_t;

#endif //ESZ7_T__H
