/**
  \file   dtr.c
  \author Aleksey Lekhtin
  \date   Apr  8, 2025
  \brief  The Depth Tracker feature
*/

#include <string.h>
#include <sys/stat.h>

#include "cfg.h"
#include "dbg.h"
#include "dtr.h"
#include "dtr_socket.h"
#include "thr_callbacks.h"
#include "utils.h"

static dtr_t m_dtr = (dtr_t){
    .cmd = {},
    .cmd.sock = {.fd = INVALID_SOCKET},

    .packet.memsz = 0,
    .packet.bytes = NULL,
    .packet.nbytes = 0,

    .data.online = NO,
    .data.sock = {.fd = INVALID_SOCKET},
};

/*-------------------------------------------------------*/
dtr_t *dtr()
{
    return &m_dtr;
}

/*-------------------------------------------------------*/
bool dtr_run()
{
    DBG(1);
    strncpy(m_dtr.serno, cfg()->dtr_serno, sizeof(m_dtr.serno) - 1);
    m_dtr.cmd.port = cfg()->dtr_cmd_port;
    m_dtr.data.port = cfg()->dtr_data_port;

    //-------------------------------------------
    thr_prop_t prop = {
        .name = "dtr-cmd",
        .type = THR_TYPE_DTR_CMD,
        .mode = THR_DETACH,
        .start_func = thread_start,
        .stop_func = thread_stop,
        .payload = &m_dtr,
    };
    THR_PROP_START(thr_cmd, &prop);
    if (!thr_cmd) {
        return FAIL;
    }
    DBG_NSLEEP(TS_100MS);

    //-------------------------------------------
    prop = (thr_prop_t){
        .name = "dtr-data",
        .type = THR_TYPE_DTR_DATA,
        .mode = THR_DETACH,
        .start_func = thread_start,
        .stop_func = thread_stop,
        .payload = &m_dtr,
    };
    THR_PROP_START(thr_data, &prop);
    if (!thr_data) {
        return FAIL;
    }
    DBG_NSLEEP(TS_100MS);
    return OK;
}
