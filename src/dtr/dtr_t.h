/**
  \file   dtr_t.h
  \author Aleksey Lekhtin
  \date   Apr  4, 2025
  \brief  The Depth Tracker structures
*/

#ifndef DTR_T__H_
#define DTR_T__H_

#include <sys/types.h>

#include "dtr_packet_t.h"
#include "socket_t.h"
#include "thr.h"
#include "main.h"

typedef struct dtr_data_t
{
    thr_t *thr;
    thr_t *cli_thr;
    pthread_mutex_t mutex;
    socket_t sock;
    bool online;
    int port;
} dtr_data_t;

typedef struct dtr_cmd_t
{
    thr_t *thr;
    thr_t *cli_thr;
    pthread_mutex_t mutex;
    socket_t sock;
    bool online;
    int port;
} dtr_cmd_t;

typedef struct dtr_t
{
    char serno[32];
    dtr_cmd_t  cmd;
    dtr_data_t data;
    dtr_packet_t packet;
} dtr_t;

#endif //DTR_T__H_
