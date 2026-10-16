/**
  \file   siu_data.c
  \author Aleksey Lekhtin
  \date   Feb 26, 2025
  \brief  The SIU obj interface
*/

#include <errno.h>
#include <libgen.h>
#include <malloc.h>
#include <math.h>
#include <string.h>

#include "cfg.h"
#include "dbg.h"
#include "dtr.h"
#include "core.h"
#include "siu.h"
#include "siu_socket.h"
#include "log.h"

//---------------------------------------------------------
static float siu_calc_block_position(int counts)
{
    pos_calibration_t *table = &siu()->vars.DepthCalibrationTable;
    block_position_t *points = siu()->vars.DepthCalibrationTable.points;

    // checks
    if (table->size == 0) {
        return 0.0f;
    }
    if (counts <= points[0].count) {
        return points[0].value;
    }
    if (counts >= table->points[table->size - 1].count) {
        return table->points[table->size - 1].value;
    }

    // calculation
    float value = 0.0f;
    for (int i = 0; i < table->size - 1; i++) {
        if (counts >= table->points[i].count && counts <= table->points[i + 1].count) {
            int x1 = table->points[i].count;
            int x2 = table->points[i + 1].count;
            float y1 = table->points[i].value;
            float y2 = table->points[i + 1].value;
            value = y1 + (y2 - y1) * (float) (counts - x1) / (float) (x2 - x1);
            break;
        }
    }
    return value;
}

//---------------------------------------------------------
static float siu_calc_hookload_weight(uint32_t adc_value)
{
    hookload_calibration_t *table = &siu()->vars.HKLDCalibrationTable;
    uint32_t n1 = table->a2d1;
    uint32_t n2 = table->a2d2;
    float v1 = table->weight1;
    float v2 = table->weight1;
    if (n1 == 0 || n2 == 0 || fpclassify(v1) == FP_ZERO || fpclassify(v2) == FP_ZERO) {
        siu()->vars.DepthSystemData.Hookload = 0.0f;
        return 0.0f;
    }
    float value = v1 + (v2 - v1) * (float) (adc_value - n1) / (float) (n2 - n1);
    return value;
}

//-----------------------------------------------------------
void siu_data_send(int channel)
{
    if (channel == 1) {
        siu()->vars.DER = core()->data.encoder1; // Depth Encoder Counts
        siu()->vars.DepthSystemData.BlockPosition = siu_calc_block_position(core()->data.encoder1);
    }
    if (channel == 2) {
        siu()->vars.HookloadA2D = core()->data.HKLD16bit;
        siu()->vars.DepthSystemData.Hookload = siu_calc_hookload_weight(siu()->vars.HookloadA2D);
    }

    if (siu()->data.cli_thr) {
        if (channel == 1) {
            DBGF(3, "siu data client thr:  %p, %d, channel: %d", siu()->data.cli_thr, siu()->data.port, channel);
            siu_data_create_packet();
            thr_wakeup(siu()->data.cli_thr);
        } else if (channel == 2) {
            DBGF(3, "siu data client thr:  %p, %d, channel: %d (todo)", siu()->data.cli_thr, siu()->data.port, channel);
        }
    } else {
        DBGF(8, "siu data client thr:  %p", siu()->data.cli_thr);
    }
}

//-----------------------------------------------------------
/**
 * Цикличное ожидание данных от ESZ7 ADC и отправка клиенту
 * @param thr - данные по потоку
 * @param siu - данные по соединению с SureShot
 * @param sock - соединение с реципиентом
 * @return - OK
 * @return - FAIL
 */
static bool siu_data_send_to_client(thr_t *thr, const siu_t *siu, socket_t *sock)
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
        DBGF(3, "=== %s got a shutting down...", thr->name);
        pthread_mutex_unlock(&thr->mutex);
        return FAIL;
    }

    // send points
    if (!siu->packet.bytes || siu->packet.nbytes == 0) {
        LOG(ERROR, "505: INTERNAL ERROR: Nothing to send");
        DBG(0, "\n505: INTERNAL ERROR: Nothing to send\n");
        pthread_mutex_unlock(&thr->mutex);
        return FAIL;
    }

    int ret = socket_send(sock, siu->packet.bytes, siu->packet.nbytes);
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
int siu_data_client_thread(thr_t *thr)
{
    client_t *client = (client_t *) thr->payload;
    siu_t *siu = client->data;
    siu->data.cli_thr = thr;
    socket_t *cli_sock = &client->sock;
    DBGF(3, "-- %s started on socket %d, port %d",
        thr->name, cli_sock->fd, siu->data.port);

    siu->data.online = YES;
    core__wakeup();

    while (1) {
        const bool ok = siu_data_send_to_client(siu->data.cli_thr, siu, cli_sock);
        if (!ok) {
            // either got a 'shuttingdown' event
            // or got socket error (for example - connection lost)
            break;
        }
    }
    DBGF(3, "%s - shuttingdown...", thr->name);

    siu->data.online = NO;
    core__wakeup();

    siu->data.cli_thr = NULL;
    DBGF(3, "--- %s finished %s:%d", thr->name, basename(__FILE__), __LINE__);
    return 0;
}
