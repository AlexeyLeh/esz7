/**
  \file   wits_t.h
  \author Aleksey Lekhtin
  \date   2026/08/25
  \brief  Структуры данных WITS для работы с Atlas
*/

#ifndef ATLAS_WITS_T_H
#define ATLAS_WITS_T_H

#include "core_t.h"
#include "main.h"

#include <time.h>

typedef enum WITS_REC_ITEM : uint8_t {
    WITS_REC_NONE = 0,
    WITS_WELL_ID = 1,
    WITS_REC_TYPE,
    WITS_REC_PS1,
    WITS_REC_PS2,
    WITS_REC_HKLD,
    WITS_REC_ENC1,
    WITS_REC_ENC2
} WITS_REC_ITEM;

typedef struct wits_packet_t
{
    char *bytes;
    int nmem;
    int nbytes;
} wits_packet_t;

typedef struct wits_cfg_rec_t
{
    WITS_REC_ITEM item;
    int code;
    char text[32];
} wits_cfg_rec_t;

typedef struct wits_config_t
{
    char fname[512];
    int nrecs;
    wits_cfg_rec_t recs[32];
} wits_config_t;

typedef struct wits_t
{
    const char *host;
    int port;
    socket_t sock;
    pthread_mutex_t mutex;

    wits_config_t config;
    wits_packet_t packet;

    struct timespec timestamp;
    struct tm tm;
    uint32_t seqnum;
    uint32_t record_id;
    uint32_t logical_record_type;

    stat_t stat;
} wits_t;

#endif // ATLAS_WITS_T_H
