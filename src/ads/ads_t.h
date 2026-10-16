/**
  \file   ads_t.h
  \author Aleksey Lekhtin
  \date   Feb 25, 2024
  \brief  The points structure
*/

#ifndef ADS_T__H_
#define ADS_T__H_

#include "ads_packet_t.h"
#include "socket_t.h"
#include "thr.h"

typedef struct ads_file_t
{
    int sensor;
    int number;
    uint32_t samplenum;
    struct timespec created;
} ads_file_t;

typedef struct ads_cmd_t
{
    thr_t *cli_thr;
    socket_t sock;
    pthread_mutex_t mutex;
    int port;

    int pressure1_scale;
    int pressure2_scale;
    const char *VerComunications;
    const char *VerSensorBoard;
    const char *VerADDriver;
    const char *VerIODriver;
    const char *VerGUI;
    int IOINSTALLER;
    int DECINSTALLER;
} ads_cmd_t;

typedef struct ads_data_t
{
    thr_t *cli_thr;
    pthread_mutex_t mutex;
    socket_t sock;
    bool online;

    int port;
    ads_file_t fileS1;
    ads_file_t fileS2;

    int npnts100;       // 100
    ads_packet_t packet;
} ads_data_t;

typedef struct ads_t
{
    char serno[32];
    ads_cmd_t cmd;
    ads_data_t data;
    int dump_ads_data_fd;
} ads_t;

#endif //ADS_T__H_
