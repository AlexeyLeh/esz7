/**
  \file   siu_t.h
  \author Aleksey Lekhtin
  \date   Apr  4, 2025
  \brief  The SIU structures
*/

#ifndef SIU2_T__H_
#define SIU2_T__H_

#include <sys/types.h>

#include "core_t.h"
#include "socket_t.h"
#include "siu_packet_t.h"
#include "siu_vars_t.h"
#include "thr.h"

typedef struct siu_file_t {
    struct timespec created;
} siu_file_t;

typedef struct siu_t siu_t;

typedef struct client_t {
    pthread_mutex_t mutex;
    void *data;
    socket_t sock;
} client_t;

typedef struct siu_cmd_t {
    thr_t *cli_thr;
    pthread_mutex_t mutex;
    socket_t sock;
    int port;
} siu_cmd_t;

typedef struct siu_data_t {
    thr_t *cli_thr;
    pthread_mutex_t mutex;
    socket_t sock;
    bool online;
    int port;
} siu_data_t;

typedef struct siu_t {
    // not all fields using together!
    siu_cmd_t cmd;
    siu_data_t data;
    siu_vars_t vars;
    siu_packet_t packet;
} siu_t;

#endif //SIU2_T__H_
