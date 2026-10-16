/**
  \file   ads.c
  \author Aleksey Lekhtin
  \date   Feb 26, 2025
  \brief  The ADS feature
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ads.h"
#include "ads_socket.h"
#include "cfg.h"
#include "dbg.h"
#include "siu.h"
#include "thr_callbacks.h"
#include "utils.h"

//---------------------------------------------------------
static ads_t m_ads = (ads_t){
    .cmd = {},
    .cmd.sock = {.fd = INVALID_SOCKET},
    .cmd.VerComunications = "0000100001",
    .cmd.VerSensorBoard = "0000100002",
    .cmd.VerADDriver = "0000100003",
    .cmd.VerIODriver = "0000100004",
    .cmd.VerGUI = "0000100005",
    .cmd.IOINSTALLER = 1,
    .cmd.DECINSTALLER = 1,
    .dump_ads_data_fd = -1,
    .cmd.pressure1_scale = 3000, // pci (200 atm)
    .cmd.pressure2_scale = 3000,

    .data = {},
    .data.online = NO,
    .data.sock = {.fd = INVALID_SOCKET},
    .data.npnts100 = 100,
    .data.packet.header.sign = {'A', 'P', 'S'},
    .data.packet.header.msg60 = '<', // dec:60
    .data.packet.nbytes = 421,
};

//---------------------------------------------------------
ads_t *ads()
{
    return &m_ads;
}

//---------------------------------------------------------
bool ads_run()
{
    DBG(3);
    strncpy(m_ads.serno, cfg()->ads_serno, sizeof(m_ads.serno) - 1);
    m_ads.cmd.port = cfg()->ads_cmd_port;
    m_ads.data.port = cfg()->ads_data_port;

    ads_init_file();
    ads_packet_init();
    dump_ads_open();

    //-------------------------------------------
    thr_prop_t prop = {
        .name = "ads-cmd",
        .type = THR_TYPE_ADS_CMD,
        .mode = THR_DETACH,
        .start_func = thread_start,
        .stop_func = thread_stop,
        .payload = &m_ads,
    };
    THR_PROP_START(thr_cmd, &prop);
    if (!thr_cmd) {
        return FAIL;
    }
    DBG_NSLEEP(TS_100MS);

    //-------------------------------------------
    prop = (thr_prop_t){
        .name = "ads-data",
        .type = THR_TYPE_ADS_DATA,
        .mode = THR_DETACH,
        .start_func = thread_start,
        .stop_func = thread_stop,
        .payload = &m_ads,
    };
    THR_PROP_START(thr_data, &prop);
    if (!thr_data) {
        return FAIL;
    }
    DBG_NSLEEP(TS_100MS);
    return OK;
}

//---------------------------------------------------------
void ads_init_file()
{
    pthread_mutex_lock(&m_ads.data.mutex);
    struct timespec ts;
    clock_get_local_time(&ts);
    m_ads.data.fileS1.sensor = 1;
    m_ads.data.fileS1.number = 1;
    m_ads.data.fileS1.samplenum = 0;
    m_ads.data.fileS1.created = ts;
    DBGF(2, ">>> ADS sensor:%d, file:%u, sample:%u, %s",
         m_ads.data.fileS1.sensor,
         m_ads.data.fileS1.number,
         m_ads.data.fileS1.samplenum,
         timespec_to_dd_mm_yyyy_hh_mm_ss_nz(&m_ads.data.fileS1.created, 6, NULL));

    m_ads.data.fileS2.sensor = 2;
    m_ads.data.fileS2.number = 1;
    m_ads.data.fileS2.samplenum = 0;
    m_ads.data.fileS2.created = ts;
    DBGF(2, ">>> ADS sensor:%d, file:%u, sample:%u, %s",
         m_ads.data.fileS2.sensor,
         m_ads.data.fileS2.number,
         m_ads.data.fileS2.samplenum,
         timespec_to_dd_mm_yyyy_hh_mm_ss_nz(&m_ads.data.fileS2.created, 6, NULL));
    pthread_mutex_unlock(&m_ads.data.mutex);
}

//---------------------------------------------------------
void ads_new_file(int sensor)
{
    pthread_mutex_lock(&m_ads.data.mutex);
    DBG(3);
    struct timespec ts;
    clock_get_local_time(&ts);
    if (sensor == 1) {
        m_ads.data.fileS1.number++;
        m_ads.data.fileS1.samplenum = 0;
        m_ads.data.fileS1.created = ts;
        DBGF(2, ">>> ADS sensor:%d, file:%u, sample:%u, %s",
             m_ads.data.fileS1.sensor,
             m_ads.data.fileS1.number,
             m_ads.data.fileS1.samplenum,
             timespec_to_dd_mm_yyyy_hh_mm_ss_nz(&m_ads.data.fileS1.created, 6, NULL));
    } else if (sensor == 2) {
        m_ads.data.fileS2.number++;
        m_ads.data.fileS2.samplenum = 0;
        m_ads.data.fileS2.created = ts;
        DBGF(2, ">>> ADS sensor:%d, file:%u, sample:%u, %s",
             m_ads.data.fileS2.sensor,
             m_ads.data.fileS2.number,
             m_ads.data.fileS2.samplenum,
             timespec_to_dd_mm_yyyy_hh_mm_ss_nz(&m_ads.data.fileS2.created, 6, NULL));
    }
    pthread_mutex_unlock(&m_ads.data.mutex);
}

//---------------------------------------------------------
void ads_reset_file(int sensor)
{
    pthread_mutex_lock(&m_ads.data.mutex);
    DBG(3);
    struct timespec ts;
    clock_get_local_time(&ts);
    if (sensor == 1) {
        m_ads.data.fileS1.sensor = 1;
        m_ads.data.fileS1.number = 1;
        m_ads.data.fileS1.samplenum = 0;
        m_ads.data.fileS1.created = ts;
        DBGF(2, ">>> ADS sensor:%d, file:%u, sample:%u, %s",
             m_ads.data.fileS1.sensor,
             m_ads.data.fileS1.number,
             m_ads.data.fileS1.samplenum,
             timespec_to_dd_mm_yyyy_hh_mm_ss_nz(&m_ads.data.fileS1.created, 6, NULL));
    } else if (sensor == 2) {
        m_ads.data.fileS2.sensor = 2;
        m_ads.data.fileS2.number = 1;
        m_ads.data.fileS2.samplenum = 0;
        m_ads.data.fileS2.created = ts;
        DBGF(2, ">>> ADS sensor:%d, file:%u, sample:%u, %s",
             m_ads.data.fileS2.sensor,
             m_ads.data.fileS2.number,
             m_ads.data.fileS2.samplenum,
             timespec_to_dd_mm_yyyy_hh_mm_ss_nz(&m_ads.data.fileS2.created, 6, NULL));
    }
    pthread_mutex_unlock(&m_ads.data.mutex);
}
