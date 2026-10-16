/**
  \file   aps_wits.c
  \author Aleksey Lekhtin
  \date   Oct 28, 2025
  \brief  Работа с APS WITS
*/

#include <libgen.h>
#include <stdio.h>
#include <string.h>

#include "aps_wits.h"
#include "cfg.h"
#include "dbg.h"
#include "errno.h"
#include "core.h"
#include "siu.h"
#include "thr.h"
#include "thr_callbacks.h"
#include "utils.h"
#include "log.h"

/*---------------------------------------------------------*/
static aps_wits_t m_aps_wits = {
    .packet.nmem = 0,
    .packet.bytes = NULL,
    .packet.nbytes = 0,

    .online = NO,
    .sock = {.fd = INVALID_SOCKET},

    .vars.Timestamp = {0},
    .vars.HoleDepth = -999.25,
    .vars.BitDepth = -999.25,
    .vars.BlockPosition = -999.25,
    .vars.BlockVelocity = -999.25,
    .vars.ROP1S = -999.25,
    .vars.ROP5S = -999.25,
    .vars.Hookload = -999.25,
    .vars.WeightOnBit = -999.25,
    .vars.Torque = -999.25,
    .vars.RPM = -999.25,
    .vars.StandpipePressure = -999.25,
    .vars.MudFlowIn = -999.25,
    .vars.MudFlowOut = -999.25,
    .vars.StrokesPerMin1 = -999.25,
    .vars.StrokesPerMin2 = -999.25,
    .vars.StrokesPerMin3 = -999.25,
    .vars.DifferentialPressure = -999.25,
    .vars.OnBottom = false,
    .vars.InSlips = false,
    .vars.Approved = false,
    .vars.DrillState = 0,
    .vars.KellyDownDepth = -999.25,
};

/*---------------------------------------------------------*/
inline aps_wits_t *aps_wits()
{
    return &m_aps_wits;
}

/*---------------------------------------------------------*/
bool aps_wits_run()
{
    DBG(1);
    strncpy(m_aps_wits.serno, cfg()->aps_wits_serno, sizeof(m_aps_wits.serno) - 1);
    m_aps_wits.host = cfg()->aps_wits_host;
    m_aps_wits.port = cfg()->aps_wits_port;

    aps_wits_packet_init();

    //-------------------------------------------
    thr_prop_t prop = {
        .name = "wits",
        .type = THR_TYPE_APS_WITS,
        .mode = THR_DETACH,
        .group_id = 4,
        .start_func = thread_start,
        .stop_func = thread_stop,
        .payload = &m_aps_wits,
    };
    THR_PROP_START(thr_cmd, &prop);
    if (!thr_cmd) {
        return FAIL;
    }
    DBG_NSLEEP(TS_100MS);
    return OK;
}

//-----------------------------------------------------------
void aps_wits_data_send()
{
    if (m_aps_wits.cli_thr) {
        DBGF(3, "wits data client thr: %p", m_aps_wits.cli_thr);
        aps_wits_create_packet();
        thr_wakeup(m_aps_wits.cli_thr);
    } else {
        DBGF(8, "wits data client thr: %p", m_aps_wits.cli_thr);
    }
}

//---------------------------------------------------------
static bool aps_wits_send_to_client(thr_t *thr, const aps_wits_t *wits, socket_t *sock)
{
    int rc = 0;
    pthread_mutex_lock(&thr->mutex);
    while (thr->wakeup != YES && rc == 0) {
        DBGF(8, "%s pthread_cond_wait() - waiting for event", thr->name);
        rc = pthread_cond_wait(&thr->cond, &thr->mutex);
        DBGF(4, "%s pthread_cond_wait() - enabled to work ( rc:%d %s)",
             thr->name, rc, (rc != 0 ? strerror(rc) : ""));
    }
    thr->wakeup = NO;
    if (thr->shuttingdown) {
        DBG(3, "=== got a shutting down...");
        pthread_mutex_unlock(&thr->mutex);
        return FAIL;
    }

    // send points
    if (!wits->packet.bytes || wits->packet.nbytes == 0) {
        LOG(ERROR, "505: INTERNAL ERROR: Nothing to send");
        DBG(0, "\n505: INTERNAL ERROR: Nothing to send\n");
        pthread_mutex_unlock(&thr->mutex);
        return FAIL;
    }

    int ret = socket_send(sock, wits->packet.bytes, wits->packet.nbytes);
    if (ret == -1) {
        LOG(ERROR, "%s (cli_fd:%d) - send failed - '%s'", sock->name, sock->fd, socket_strerror(sock->err));
        DBGF(0, "%s (cli_fd:%d) - send failed - '%s'", sock->name, sock->fd, socket_strerror(sock->err));
        if (sock->err != EPIPE && sock->err != ECONNRESET) {
            ;
        }
        pthread_mutex_unlock(&thr->mutex);
        return FAIL;
    }
    DBGF(4, "%s (cli_fd:%d) - sent %d bytes to %s:%d", sock->name, sock->fd,
         ret, inet_ntoa(sock->sockaddrin.sin_addr), sock->port);
    pthread_mutex_unlock(&thr->mutex);
    return OK;
}

//---------------------------------------------------------
int aps_wits_client_thread(thr_t *thr)
{
    client_t *client = (client_t *) thr->payload;
    aps_wits_t *wits = client->data;
    wits->cli_thr = thr;
    socket_t *cli_sock = &client->sock;
    DBGF(3, "-- %s started on socket %d, port %d",
         thr->name, wits->sock.fd, wits->port);

    wits->online = YES;
    core__wakeup();
    while (1) {
        const bool ok = aps_wits_send_to_client(wits->cli_thr, wits, cli_sock);
        if (!ok) {
            // either got a 'shuttingdown' event
            // or got socket error (for example - connection lost)
            break;
        }
    }
    DBGF(3, "%s - shuttingdown...", thr->name);

    wits->online = NO;
    core__wakeup();

    wits->cli_thr = NULL;
    DBGF(3, "---  finished %s:%d", basename(__FILE__), __LINE__);
    return 0;
}
