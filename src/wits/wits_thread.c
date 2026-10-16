//
// Created by LAV on 25.08.2026.
//

#include "dbg.h"
#include "log.h"
#include "socket.h"
#include "utils.h"
#include "wits.h"

#include <libgen.h>
#include <stdlib.h>

//---------------------------------------------------------
static bool wits_send_data_to_client(thr_t *thr, void *data, socket_t *sock)
{
    wits_t *wits = data;
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
        return OK;
    }

    DBGF(3, "%s (cli_fd:%d) - send %d bytes to %s:%d", sock->name, sock->fd, wits->packet.nbytes,
         inet_ntoa(sock->sockaddrin.sin_addr), sock->port);

    const int ret = socket_send(sock, wits->packet.bytes, wits->packet.nbytes);
    if (ret == -1) {
        LOG(ERROR, "%s (cli_fd:%d) - send failed - '%s'", sock->name, sock->fd, socket_strerror(sock->err));
        DBGF(0, "%s (cli_fd:%d) - send failed - '%s'", sock->name, sock->fd, socket_strerror(sock->err));
        if (sock->err != EPIPE && sock->err != ECONNRESET) {
            ;
        }
        wits->stat.send_error++;
        pthread_mutex_unlock(&thr->mutex);
        return FAIL;
    }
    wits->stat.send_ok++;
    DBGF(5, "%s (cli_fd:%d) - sent %d bytes to %s:%d", sock->name, sock->fd,
         ret, inet_ntoa(sock->sockaddrin.sin_addr), sock->port);
    pthread_mutex_unlock(&thr->mutex);
    return OK;
}

//---------------------------------------------------------
static int wits_client_thread_stop(thr_t *thr)
{
    client_t *client = thr->payload;
    socket_t *cli_sock = &client->sock;
    wits_t *wits = client->data;
    pthread_mutex_t *mutex = &wits->mutex;
    pthread_mutex_lock(mutex);
    DBGF(3, "--- %s close client connections %s fd:%d",
         thr->name, cli_sock->name, cli_sock->fd);
    DBGF(5, "lock %s::cmd.mutex", thr->name);
    socket_close_fd(&cli_sock->fd);
    DBGF(5, "unlock %s::cmd.mutex", thr->name);
    pthread_mutex_unlock(mutex);
    return 0;
}

//---------------------------------------------------------
static int wits_client_thread(thr_t *thr)
{
    client_t *client = thr->payload;
    DBGF(3, "-- %s started on socket %d, port %d",
         thr->name, client->sock.fd, client->sock.port);

    while (1) {
        const bool ok = wits_send_data_to_client(thr, client->data, &client->sock);
        if (!ok) {
            // either got a 'shuttingdown' event
            // or got socket error (for example - connection lost)
            break;
        }
    }
    DBGF(3, "---  finished %s:%d", basename(__FILE__), __LINE__);
    return 0;
}

//---------------------------------------------------------
int wits_thread_stop(thr_t *thr)
{
    wits_t *wits = thr->payload;
    socket_t *sock = &wits->sock;
    DBGF(3, "[tid:%08lX] %s going to close connection fd:%d", pthread_self(), thr->name, sock->fd);
    socket_close_fd(&sock->fd);
    wits_packet_free();
    nsleep(TS_50MS);
    return 0;
}

//---------------------------------------------------------
int wits_thread_start(thr_t *thr)
{
    DBGF(3, "[tid:%08lX] --- %s (%s) going to be started...",
         pthread_self(), thr->name, thr_type_to_string(thr->type));


    wits_t *wits = thr->payload;
    wits->sock.port = wits->port;
    socket_t *sock = &wits->sock;
    snprintf(sock->name, sizeof(sock->name), "%s", thr->name);
    LOG(INFO, "%s started on port %d", thr->name, sock->port);

    do {
        if (!wits_open_socket(sock)) {
            if (thr->shuttingdown) {
                break;
            }
            ssleep(TS_1SEC * 2);
            continue;
        }

        do {
            const int cli_fd = wits_accept(&wits->sock);
            if (cli_fd == INVALID_SOCKET || cli_fd == 0) {
                nsleep(TS_100MS);
                break;
            }
            client_t *client = malloc(sizeof(client_t)); // TODO memory leak ?
            client->data = wits;
            client->sock = *sock;
            client->sock.fd = cli_fd;
            snprintf(client->sock.name, sizeof(client->sock.name), "%s-cli-%d", thr->name, client->sock.fd);

            THR_START(thr_cli, THR_DETACH, wits_client_thread, wits_client_thread_stop, client, true,
                      client->sock.name);
            if (!thr_cli) {
                break;
            }
            thr_cli->type = THR_TYPE_WITS_CLIENT;
        } while (!thr->shuttingdown);
    } while (!thr->shuttingdown);

    DBGF(3, "[tid:%08lX] --- finished %s %s:%d",
         pthread_self(), thr->name, basename(__FILE__), __LINE__);
    return 0;
}
