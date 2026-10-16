/**
  \file   atlas_t.h
  \author Aleksey Lekhtin
  \date   2024/02/01
  \brief  Структуры данных для работы с Atlas
*/

#ifndef ATLAS_T__H_
#define ATLAS_T__H_

#include <stdbool.h>
#include <time.h>

#include "atlas_packet.h"
#include "core_t.h"
#include "main.h"
#include "socket_t.h"

typedef struct host_t {
    const char *host;
    uint16_t port1;
    uint16_t port2;
    socket_t sock1;
    socket_t sock2;
} host_t;

typedef struct atlas_t
{
    char serno[32];
    host_t hosts[3];
    size_t hosts_size;

    const int npnts500;     // 500
    atlas_packet_t packet;

    uint32_t samplenum;
    uint32_t timerange;
    stat_t stat;
    int fd_stat;            // статистика по данных
} atlas_t;

#endif //ATLAS_T__H_
