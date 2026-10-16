/**
  \file   atlas.c
  \author Aleksey Lekhtin
  \date   Feb 26, 2025
  \brief  Работа с Atlas
*/

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "atlas.h"
#include "atlas_socket.h"
#include "cfg.h"
#include "dbg.h"
#include "log.h"
#include "core.h"
#include "utils.h"
#include "wits.h"

/*---------------------------------------------------------*/
static atlas_t m_atlas = {
    .hosts = {
        {
            .host = NULL,
            .port1 = 0,
            .port2 = 0,
            .sock1 = {.fd = INVALID_SOCKET},
            .sock2 = {.fd = INVALID_SOCKET}
        },
        {
            .host = NULL,
            .port1 = 0,
            .port2 = 0,
            .sock1 = {.fd = INVALID_SOCKET},
            .sock2 = {.fd = INVALID_SOCKET}
        },
        {
            .host = NULL,
            .port1 = 0,
            .port2 = 0,
            .sock1 = {.fd = INVALID_SOCKET},
            .sock2 = {.fd = INVALID_SOCKET}
        },
    },
    .hosts_size = sizeof(m_atlas.hosts) / sizeof(m_atlas.hosts[0]),
    .npnts500 = 500,
    .packet = {},
    .packet.nbytes = 0,
    .packet.bytes = NULL,
    .stat = {},
    .fd_stat = -1,
};

/*---------------------------------------------------------*/
inline atlas_t *atlas()
{
    return &m_atlas;
}

/*---------------------------------------------------------*/
bool atlas_run()
{
    if (cfg()->atlas_host1[0] != '\0') {
        m_atlas.hosts[0].host = cfg()->atlas_host1;
        m_atlas.hosts[0].port1 = cfg()->atlas_port11;
        m_atlas.hosts[0].port2 = cfg()->atlas_port12;
    }

    if (cfg()->atlas_host2[0] != '\0') {
        m_atlas.hosts[1].host = cfg()->atlas_host2;
        m_atlas.hosts[1].port1 = cfg()->atlas_port21;
        m_atlas.hosts[1].port2 = cfg()->atlas_port22;
    }

    if (cfg()->atlas_host3[0] != '\0') {
        m_atlas.hosts[2].host = cfg()->atlas_host3;
        m_atlas.hosts[2].port1 = cfg()->atlas_port31;
        m_atlas.hosts[2].port2 = cfg()->atlas_port32;
    }

    strncpy(m_atlas.serno, cfg()->atlas_serno, sizeof(m_atlas.serno) - 1);

    atlas_packet_init();
    if (!atlas_open_udp_socket()) {
        return FAIL;
    }

    if (atlas()->fd_stat == -1) {
        char fname[256 + 32];
        snprintf(fname, sizeof(fname), "%s/stat.txt", cfg()->appdir);
        atlas()->fd_stat = open(fname, O_CREAT | O_TRUNC | O_WRONLY, 0640);
        if (atlas()->fd_stat == -1) {
            LOG(INFO, "can't create telemetry file '%s'", fname);
            DBGF(0, "WARNING: can't create telemetry file '%s' - SKIP - (%d) %s", fname, errno, strerror(errno));
        }
    }

    DBG(1, "started");
    return OK;
}

/*---------------------------------------------------------*/
void atlas_stop()
{
    DBG(4);
    atlas_socket_close();
    atlas_packet_free();
    if (atlas()->fd_stat == -1) {
        close(atlas()->fd_stat);
        atlas()->fd_stat = -1;
    }
}

//---------------------------------------------------------
inline void atlas_data_send()
{
    atlas_make_binary_packet();

    DBGF(2, ">>> Atlas packet #%-2d - %u ms (size: %d)", atlas()->packet.rtp.fixed_part.seqnum,
         atlas()->packet.rtp.fixed_part.timerange,
         atlas()->packet.nbytes);

    bool ok = atlas_socket_send(atlas()->packet.bytes, atlas()->packet.nbytes);
    if (!ok) {
        atlas()->stat.send_error++;
    } else {
        atlas()->stat.send_ok++;
    }
}

//---------------------------------------------------------
inline void atlas_write_telemetry()
{
    if (atlas()->fd_stat == -1) {
        return;
    }
    static char buf[512];
    snprintf(buf, sizeof(buf), "enc: %d %d\n"
             "sent: %d %d\n"
             "wits: %d %d\n"
             "adcref: %0.04f %0.20f\n"
             "ch1avrg: %10d %08X %11.8f\n"
             "ch2avrg: %10d %08X %11.8f\n"
             "ch3avrg: %10d %08X %11.8f\n"
             "ch4avrg: %10d %08X %11.8f\n\n",
             core()->data.encoder1, core()->data.encoder2,
             atlas()->stat.send_ok, atlas()->stat.send_error,
             wits()->stat.send_ok, wits()->stat.send_error,
             cfg()->adc_ref,
             cfg()->adc_scale,
             core()->data.values[0].avrg, core()->data.values[0].avrg, core()->data.values[0].avrg * cfg()->adc_scale,
             core()->data.values[1].avrg, core()->data.values[1].avrg, core()->data.values[1].avrg * cfg()->adc_scale,
             core()->data.values[2].avrg, core()->data.values[2].avrg, core()->data.values[2].avrg * cfg()->adc_scale,
             core()->data.values[3].avrg, core()->data.values[3].avrg, core()->data.values[3].avrg * cfg()->adc_scale);
    lseek(atlas()->fd_stat, 0, SEEK_SET);
    write(atlas()->fd_stat, buf, strlen(buf));
    DBGF(4, "stat: %s", replace(buf, '\n', ' '));
}
