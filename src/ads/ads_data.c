/**
  \file   ads_data.c
  \author Aleksey Lekhtin
  \date   Feb 26, 2025
  \brief  The ADS obj interface
*/

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>

#include "ads.h"
#include "ads_socket.h"
#include "cfg.h"
#include "dbg.h"
#include "core.h"
#include "log.h"

//-----------------------------------------------------------
inline void aps_write_stat()
{
    ;
    /*
    if (ads()->fd_stat != -1) {
        char buf[32];
        snprintf(buf, sizeof(buf), "enc: %d %d\nsent: %d %d\n",
                 core()->data.encoder1, core()->data.encoder2,
                 ads()->stat.send_ok, ads()->stat.send_error);
        lseek(ads()->fd_stat, 0, SEEK_SET);
        write(ads()->fd_stat, buf, strlen(buf));
        DBGF(4, "stat: %s", replace(buf, '\n', ' '));
    }
    */
}

//-----------------------------------------------------------
/**
 * @brief Формирование масива данных из данных ESZ7 ADC.
 *
 * ADS-данные это 100 точек на давление и 100 на вес, за 250ms. \n
 * (и пока только давление) \n
 * Каждая точка - uint32_t \n
 */
void ads_data_send(int channel)
{
    if (ads()->data.cli_thr) {
        DBGF(3, "ads data client thr:  %p, channel: %d", ads()->data.cli_thr, channel);
        if (channel == 1) {
            ads_data_create_packet();
            thr_wakeup(ads()->data.cli_thr);
        }
    } else {
        DBGF(8, "ads data client thr:  %p", ads()->data.cli_thr);
    }
}

//-----------------------------------------------------------
/**
 * Цикличное ожидание данных от ESZ7 ADC и отправка клиенту
 * @param thr - данные по потоку
 * @param ads - данные по соединению с ADS
 * @param sock - соединение с реципиентом
 * @return - OK
 * @return - FAIL
 */
static bool ads_data_send_to_client(thr_t *thr, const ads_t *ads, socket_t *sock)
{
    pthread_mutex_lock(&thr->mutex);
    int rc = 0;
    while (thr->wakeup != YES && rc == 0) {
        DBGF(9, "%s pthread_cond_wait() - waiting for event", thr->name);
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
    if (!ads->data.packet.bytes || ads->data.packet.nbytes == 0) {
        LOG(ERROR, "500:INTERNAl ERROR: Nothing to send");
        DBG(0, "\n500:INTERNAl ERROR: Nothing to send\n");
        pthread_mutex_unlock(&thr->mutex);
        return FAIL;
    }

    int ret = socket_send(sock, ads->data.packet.bytes, ads->data.packet.nbytes);
    if (ret == -1) {
        LOG(ERROR, "%s (cli_fd:%d '%s') - send failed - '%s'", sock->name, sock->fd, sock->cli_host,
            socket_strerror(sock->err));
        DBGF(0, "%s (cli_fd:%d '%s') - send failed - '%s'", sock->name, sock->fd, sock->cli_host,
             socket_strerror(sock->err));
        if (sock->err != EPIPE && sock->err != ECONNRESET) { ;
        }
        pthread_mutex_unlock(&thr->mutex);
        return FAIL;
    }

    DBGF(4, "%s (cli_fd:%d) - sent %d bytes to %s:%d", sock->name, sock->fd,
         ret, inet_ntoa(sock->sockaddrin.sin_addr), sock->port);
    pthread_mutex_unlock(&thr->mutex);
    return OK;
}

//-----------------------------------------------------------
int ads_data_client_thread(thr_t *thr)
{
    client_t *cli_payload = (client_t *) thr->payload;
    ads_t *ads = cli_payload->data;
    ads->data.cli_thr = thr;
    socket_t *cli_sock = &cli_payload->sock;
    DBGF(3, "--- %s started on socket %d, port %d",
         thr->name, cli_sock->fd, ads->data.port);

    ads->data.online = YES;
    core__wakeup();

    while (1) {
        const bool ok = ads_data_send_to_client(ads->data.cli_thr, ads, cli_sock);
        if (!ok) {
            // either got a 'shuttingdown' event
            // or got socket error (for example - connection lost)
            break;
        }
    }
    DBGF(3, "%s - shuttingdown...", thr->name);

    ads->data.online = NO;
    core__wakeup();

    ads->data.cli_thr = NULL;
    DBGF(3, "--- %s finished %s:%d", thr->name, basename(__FILE__), __LINE__);
    return 0;
}
