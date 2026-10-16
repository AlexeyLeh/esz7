/**
  \file   cmd_thread.c
  \author Aleksey Lekhtin
  \date   04.05.2024
  \brief  The ESZ7 command server
 */

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <malloc.h>
#include <string.h>

#include "socket.h"
#include "cfg.h"
#include "dbg.h"
#include "core.h"
#include "cmd_pkt.h"
#include "thr.h"
#include "log.h"

//---------------------------------------------------------
static core_cmd_t m_core_cmd = {
        .port = 8100,
        .sock = {},
        .sock.fd = -1,
};

//---------------------------------------------------------
static bool cmd_open_socket(core_cmd_t *esz7cmd)
{
    snprintf(esz7cmd->sock.name, sizeof(esz7cmd->sock.name), "%s", esz7cmd->name);
    esz7cmd->sock.port = esz7cmd->port;
    DBGF(1, "[tid:%08lX] --- %s open port %d",
         pthread_self(), esz7cmd->sock.name, esz7cmd->sock.port);
    if (socket_open(&esz7cmd->sock, SOCK_STREAM, IPPROTO_IP)) {
        if (socket_bind(&esz7cmd->sock)) {
            if (socket_listen(&esz7cmd->sock)) {
                return OK;
            }
        }
    }
    socket_close_fd(&esz7cmd->sock.fd);
    return FAIL;
}

//---------------------------------------------------------
static int cmd_accept(core_cmd_t *cmd)
{
    DBGF(1, "[tid:%08lX] --- %s waiting on port %d",
         pthread_self(), cmd->sock.name, cmd->sock.port);
    int fd = socket_accept(&cmd->sock);
    if (fd != INVALID_SOCKET) {
        if (debug_level > 0) {
            char buf[NI_MAXHOST * 3] = {0};
            if (strcasecmp(cmd->sock.cli_ip, cmd->sock.cli_host) == 0) {
                strncpy(buf, cmd->sock.cli_host, sizeof(buf) - 1);
            } else {
                snprintf(buf, sizeof(buf), "%s %s", cmd->sock.cli_ip, cmd->sock.cli_host);
            }
            DBGF(1, "--- request accepted (%d), cli_fd:%d (from %s)", cmd->sock.port, fd, buf);
        }
    }
    return fd;
}

//---------------------------------------------------------
static int cmd_client_stop(thr_t *thr)
{
    client_t *client = thr->payload;
    socket_t *cli_sock = &client->sock;
    core_cmd_t *cmd = client->data;
    pthread_mutex_t *mutex = &cmd->mutex;
    DBGF(2, "--- %s close client connections %s fd:%d",
         thr->name, cli_sock->name, cli_sock->fd);
    DBGF(3, "lock %s::cmd.mutex", thr->name);
    pthread_mutex_lock(mutex);
    socket_close_fd(&cli_sock->fd);
    DBGF(3, "unlock %s::cmd.mutex", thr->name);
    pthread_mutex_unlock(mutex);
    return 0;
}

//---------------------------------------------------------
static int cmd_recv(socket_t *sock, SOCKET fd, char *buf, size_t buflen)
{
    int nread, total = 0;
    int fix_data_size = (int) sizeof(cmd_pkt_t);
    int buf_remain = fix_data_size;
    char *pbuf = buf;
    DBGF(1, "\n--- expected %d bytes (%s socket:%d)", fix_data_size, sock->name, fd);
    for (;;) {
        // step 1 - receiving fixed part of packet
        nread = (int) recv(fd, pbuf, (size_t) buf_remain, 0);
        DBGF(1, "--- got %d bytes", nread);
        if (nread == -1) {
            set_socket_errno(sock);
            LOG(ERROR, "%s (socket:%d) - %s", sock->name, fd, socket_strerror(sock->err));
            DBGF(0, "%s (socket:%d) - ERROR: %s", sock->name, fd, socket_strerror(sock->err));
            return -1;
        }
        pbuf += nread;
        total += nread;

        if (nread == 0) {
            set_socket_errno(sock);
            if (sock->err != 0) {
                if (sock->err != ENOENT) {
                    DBGF(1, "%s (socket:%d) ISSUE: %s", sock->name, fd, socket_strerror(sock->err));
                }
            }
            return 0;
        }

        buf_remain = fix_data_size - total;
        if (buf_remain <= 0) {
            break;
        }
        DBGF(1, "--- remain %d bytes", fix_data_size - total);
    }

    // simple test of recieved data
    bool ok = OK;
    cmd_pkt_t *packet = (cmd_pkt_t *) buf;
    cmd_pkt_hdr_t *hdr = &packet->header;

    if (debug_level >= 4) {
        DBG(2, "--- Raw bytes ---");
        DBGF(2, "sign:        %.7s", (const char *)packet->header.sign);
        DBGF(2, "ver:         0x%02X", packet->header.ver);
        DBGF(2, "hdr  size:   %u", packet->header.hdr_size);
        DBGF(2, "data length: %u", packet->header.data_len);
        DBGF(2, "pckt size:   %u", packet->header.pckt_size);
        DBGF(2, "make:        %02X (exp:@)", packet->header.mark);
        DBGF(2, "cmd:         %d %s", packet->cmd, cmd_pkt_cmd_string(packet->cmd));
        DBGF(2, "value:       %llu (0x%016lluX)", packet->value, packet->value);
    }

    cmd_pkt_ntoh(packet);

    if (debug_level >= 1) {
        DBG(1, "--- Network to Host was converted");
        DBGF(1, "sign:        %.7s", (const char *)packet->header.sign);
        DBGF(1, "ver:         0x%02X", packet->header.ver);
        DBGF(1, "hdr  size:   %u (%s)", packet->header.hdr_size,
             (packet->header.hdr_size == sizeof(cmd_pkt_hdr_t) ? "OK" : "ERR"));
        DBGF(1, "data length: %u", packet->header.data_len);
        size_t stuff_size = (sizeof(cmd_pkt_t) - sizeof(cmd_pkt_hdr_t));
        size_t pckt_size = packet->header.hdr_size + stuff_size + packet->header.data_len;
        DBGF(1, "pckt size:   %u (%s)", packet->header.pckt_size,
             (packet->header.pckt_size == pckt_size ? "OK" : "ERR"));
        DBGF(1, "make:        %02X (exp:@)", packet->header.mark);
        DBGF(1, "cmd:         %d %s", packet->cmd, cmd_pkt_cmd_string(packet->cmd));
        DBGF(1, "value:       %llu", packet->value);
    }

    if (memcmp(hdr->sign, PKT_SIGN, sizeof(PKT_SIGN)) != 0) {
        LOG(ERROR, "BAD PACKET - incorrect signature (0x%X%X%X%X)", hdr->sign[0], hdr->sign[1], hdr->sign[2],
            hdr->sign[3]);
        DBGF(0, "BAD PACKET - incorrect signature (0x%X%X%X%X)", hdr->sign[0], hdr->sign[1], hdr->sign[2],
             hdr->sign[3]);
        ok = FAIL;
    }
    if (ok) {
        size_t sz = packet->header.hdr_size;
        if (sz != sizeof(cmd_pkt_hdr_t)) {
            LOG(ERROR, "BAD PACKET - unexpected header size (has:%d, expected:%d)", sz, sizeof(cmd_pkt_hdr_t));
            DBGF(0, "BAD PACKET - unexpected header size (has:%d, expected:%d)", sz, sizeof(cmd_pkt_hdr_t));
            ok = FAIL;
        }
    }

    int data_len;
    if (ok) {
        int remainbuflen = (int) buflen - total;
        data_len = (int) packet->header.data_len;
        if (data_len > remainbuflen) {
            LOG(ERROR, "BAD PACKET - The expected packet size %d exceeds the receive buffer size %d",
                 fix_data_size + data_len, buflen);
            DBGF(0, "BAD PACKET - The expected packet size %d exceeds the receive buffer size %d",
                 fix_data_size + data_len, buflen);
            ok = FAIL;
        }
    }

    if (!ok) {
        DBG(1, "data ignored\n");
        return -2;
    }

    if (data_len == 0) {
        DBGF(1, "OK. total received %d bytes\n", total);
        return total;
    }

    DBGF(1, "next expecting data: %d bytes", data_len);
    buf_remain = data_len;
    int data_got_len = 0;
    for (;;) {
        // step 2 - receiving packet data (payload)
        nread = (int) recv(fd, pbuf, buf_remain, 0);
        if (nread == -1) {
            set_socket_errno(sock);
            LOG(ERROR, "%s (socket:%d) - %s", sock->name, fd, socket_strerror(sock->err));
            DBGF(0, "%s (socket:%d) - ERROR: %s", sock->name, fd, socket_strerror(sock->err));
            return -1;
        }
        data_got_len += nread;
        total += nread;
        pbuf += nread;
        DBGF(1, "got %3d bytes, data total: %d, total: %d\n", nread, data_got_len, total);

        if (nread == 0) {
            LOG(INFO, "INFO: no data received. EOF?");
            return 0;
        }

        buf_remain = data_len - data_got_len;
        if (buf_remain == 0) {
            break;
        }
    }
    DBGF(1, "OK. got %d bytes in total\n", total);
    return total;
}

//---------------------------------------------------------
static char *command_processing(const char *bytes, size_t nbytes)
{
    char errmsg[256] = {};
    cmd_pkt_t *packet = (cmd_pkt_t *) bytes;
    cmd_pkt_dbg_print((cmd_pkt_t *) bytes);

    bool ok = cmd_pkt_check(bytes, nbytes, errmsg, sizeof(errmsg));
    if (!ok) {
        LOG(INFO, "invalid packet ignored");
        DBG(0, "invalid packet ignored");
        const char *msg = "Oh, Shit!";
        cmd_pkt_t *resp = cmd_pkt_make_cmd(CMD_NONE, msg, strlen(msg));
        packet->header.errcode = PKT_WRONG;
        return (char *) resp;
    }

    if (packet->cmd == CMD_PING) {
        cmd_pkt_t *resp = cmd_pkt_make_cmd(CMD_PONG, 0, 0);
        size_t bytesnum = packet->header.pckt_size;
        data_hexdump((const char *) resp, bytesnum);
        return (char *) resp;
    }

    if (packet->cmd == CMD_GET_DPT_DELAY) {
        cmd_pkt_t *resp = cmd_pkt_make_cmd(packet->cmd, 0, 0);
        resp->value = cfg()->dpt_delay;
        return (char *) resp;
    }

    if (packet->cmd == CMD_SET_DPT_DELAY) {
        uint32_t value = (uint32_t) packet->value;
        cmd_pkt_t *resp = cmd_pkt_make_cmd(packet->cmd, 0, 0);
        if (value > 50) {
            packet->header.errcode = PKT_WRONG;
        } else {
            cfg()->dpt_delay = (int) value;
            if (cfg()->dpt_delay < 0) {
                cfg()->dpt_delay = 0;
            }
            cfg_save_esz7_status();
        }
        return (char *) resp;
    }

    if (packet->cmd == CMD_GET_CH_SWAP) {
        cmd_pkt_t *resp = cmd_pkt_make_cmd(packet->cmd, 0, 0);
        resp->value = cfg()->swap_channels12;
        DBGF(1, "current swap channels-12: %u", (uint32_t) resp->value);
        return (char *) resp;
    }

    if (packet->cmd == CMD_SET_CH_SWAP) {
        int value = (int) packet->value;
        cmd_pkt_t *resp = cmd_pkt_make_cmd(packet->cmd, 0, 0);
        cfg()->swap_channels12 = (value == 1 ? 1 : 0);
        DBGF(1, "set swap channels-12: %u", cfg()->swap_channels12);
        cfg_save_esz7_status();
        return (char *) resp;
    }

    // if (packet->cmd == CMD_READ_REG) {
    //     int buflen;
    //     const char *buf = esz7_pkt_data(packet, &buflen);
    //     if (buf)  {
    //         dbg2("%s(): %.*s\n", __func__, buflen, buf);
    //     }
    //     uint32_t value = 0;
    //     bool ok = adc_register_cmd(packet->cmd, packet->regno, &value);
    //     if (ok) {
    //         send_adc_response(socket_cli(), packet, value);
    //         return ok;
    //     }
    //     send_fail_message(socket_cli(), (adc_error_message[0] == 0 ? deferrmsg : adc_error_message));
    //     return ok;
    // }

    // if (packet->cmd == CMD_WRITE_REG) {
    //     int buflen;
    //     const char *buf = esz7_pkt_data(packet, &buflen);
    //     if (buf)  {
    //         dbg2("%s(): %.*s\n", __func__, buflen, buf);
    //     }
    //     uint32_t value = packet->value;
    //     bool ok = adc_register_cmd(packet->cmd, packet->regno, &value);
    //     if (ok) {
    //         send_adc_response(socket_cli(), packet, value);
    //         return ok;
    //     }
    //     send_fail_message(socket_cli(), (adc_error_message[0] == 0 ? deferrmsg : adc_error_message));
    //     return ok;
    // }

    LOG(INFO, "command 0x%0X ignored", packet->cmd);
    DBGF(0, "command 0x%0X ignored", packet->cmd);
    const char *msg = "Oh, Shit!";
    cmd_pkt_t *resp = cmd_pkt_make_cmd(CMD_NONE, msg, strlen(msg));
    packet->header.errcode = PKT_WRONG;
    return (char *) resp;
}

//---------------------------------------------------------
static int cmd_client_thread(thr_t *thr)
{
    client_t *client = (client_t *) thr->payload;
    core_cmd_t *cmd = (core_cmd_t *) client->data;
    cmd->cli_thr = thr;
    DBGF(1, "--- %s started on socket %d, port %d",
         thr->name, client->sock.fd, cmd->port);

    char command[MAX_PCKT_SIZE] = {0};
    int length = sizeof(command) - 1;
    do {
        int nread = cmd_recv(&client->sock, client->sock.fd, command, length);
        if (nread > 0) {
            command[nread] = '\0';
            DBGF(1, "got %d bytes", nread);

            char *response = command_processing(command, nread);
            if (response) {
                size_t sz = ((cmd_pkt_t *) response)->header.pckt_size;
                uint64_t value = ((cmd_pkt_t *) response)->value;
                DBGF(1, "--->>> response: 0x%08X size: %u '%s' value: 0x%016lluX", (uintptr_t) response, sz, response,
                     value);
                cmd_pkt_hton((cmd_pkt_t *) response);
                socket_send(&client->sock, response, sz);
                free(response);
            }
        } else if (!thr->shuttingdown) {
            // against DDOS :)
            nanosleep(&(struct timespec) {.tv_sec = TS_1SEC, .tv_nsec = 0}, NULL);
        }

    } while (!thr->shuttingdown);
    DBGF(3, "%s - shuttingdown...", thr->name);

    cmd->cli_thr = thr;
    DBGF(2, "---  finished %s:%d", basename(__FILE__), __LINE__);
    return 0;
}

//---------------------------------------------------------
static int thread_stop(thr_t *thr)
{
    core_cmd_t *cmd = thr->payload;
    DBGF(1, "[tid:%08lX] %s going to close connection fd:%d", pthread_self(), thr->name, cmd->sock.fd);
    socket_close_fd(&cmd->sock.fd);
    nanosleep(&(struct timespec) {.tv_nsec = TS_50MS}, NULL);
    return 0;
}

//---------------------------------------------------------
static int thread_start(thr_t *thr)
{
    DBGF(1, "\n[tid:%08lX] --- %s (%s) going to be started...",
         pthread_self(), thr->name, thr_type_to_string(thr->type));

    uint32_t counter[8] = {0};
    uint32_t *cnt = NULL;

    cnt = &counter[0];

    core_cmd_t *cmd = thr->payload;
    snprintf(cmd->name, sizeof(cmd->name), "%s", thr->name);

    do {
        if (!cmd_open_socket(cmd)) {
            if (thr->shuttingdown) {
                break;
            }
            // reconnect
            nanosleep(&(struct timespec) {.tv_sec = 2}, NULL);
            continue;
        }

        do {
            const int group_id = 1;
            char *cli_name = "cmd-cli";
            const int cli_fd = cmd_accept(cmd);
            if (cli_fd == INVALID_SOCKET || cli_fd == 0) {
                // re-accept
                nanosleep(&(struct timespec) {.tv_sec = 0, .tv_nsec = TS_100MS}, NULL);
                break;
            }

            client_t *client = malloc(sizeof(client_t));
            client->data = thr->payload;
            client->sock = cmd->sock;
            client->sock.fd = cli_fd;
            (*cnt)++;
            snprintf(client->sock.name, sizeof(client->sock.name),
                     "%s-%02d-%02u-%d", cli_name, group_id, *cnt, client->sock.fd);

            THR_PROP_CLONE(prop, thr, THR_DETACH, cmd_client_thread, cmd_client_stop, client, true, client->sock.name);
            THR_PROP_START(thr_cli, &prop);
            if (!thr_cli) {
                break;
            }

        } while (!thr->shuttingdown);
    } while (!thr->shuttingdown);
    DBGF(1, "[tid:%08lX] --- finished %s %s:%d",

         pthread_self(), thr->name, basename(__FILE__), __LINE__);

    return 0;
}

//---------------------------------------------------------
bool core_cmd_run()
{
    m_core_cmd.port = cfg()->cmd_port;

    thr_prop_t thr_prop = {
            .name = "esz7_cmd",
            .type = THR_TYPE_ESZ7_CMD,
            .mode = THR_DETACH,
            .start_func = thread_start,
            .stop_func = thread_stop,
            .payload = &m_core_cmd,
    };
    THR_PROP_START(thr, &thr_prop);
    if (!thr) {
        return FAIL;
    }
    nanosleep(&(struct timespec) {.tv_sec = 0, .tv_nsec = TS_100MS}, NULL);
    return OK;
}
