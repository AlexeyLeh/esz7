//
// Created by LAV on 25.08.2026.
//


#include "dbg.h"
#include "utils.h"
#include "wits.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*---------------------------------------------------------*/
static wits_t m_wits = {
    .packet.nmem = 0,
    .packet.bytes = NULL,
    .packet.nbytes = 0,
    .sock = {.fd = INVALID_SOCKET},
    .record_id = 1,
    .logical_record_type = 151,
    .config = {.fname = "wits.cfg"}
};

/*---------------------------------------------------------*/
inline wits_t *wits()
{
    return &m_wits;
}

//---------------------------------------------------------
static int compare(const void *a, const void *b)
{
    const wits_cfg_rec_t *r1 = (wits_cfg_rec_t *) a;
    const wits_cfg_rec_t *r2 = (wits_cfg_rec_t *) b;
    return r1->code > r2->code;
}

//---------------------------------------------------------
static void wits_read_config()
{
    m_wits.config.nrecs = 0;

    char fname[1024];
    snprintf(fname, sizeof(fname), "%s/%s", cfg()->appdir, m_wits.config.fname);
    const int fd = open(fname, O_RDONLY, 0640);
    if (fd == -1) {
        DBGF(0, "\nWARNING: Can't read config file '%s'\n", fname);
        return;
    }
    char text[1024 * 2] = {0};
    const size_t nread = read(fd, text, sizeof(text) - 1);
    close(fd);
    if (nread == 0) {
        const int e = errno;
        DBGF(0, "\nERROR: Can't read config file - '%s' (%s)\n", fname, strerror(e));
        return;
    }
    /*
        # Updated: Wed Aug 26 22:13:23 2026
        # Created: Wed Aug 26 22:10:37 2026
        #
        rec01=01        # 1. Идентификатор скважины
        rec02=0         # 2.
        rectype=151     # 3. Идентификатор записи
        rec04=0         # 4. Идентификатор последовательности записи
        recdate=0       # 5. Дата
        rectime=0       # 6. Время
        hookload=07     # 14. Вес на крюке (средний)
        encoder1=12     # 12. Положение талевого блока
        encoder2=0      #
        pressure1=21    # 21. Давление раствора на входе (среднее)
        pressure2=42    # 41. Давление раствора на входе датчик #2 (среднее)
      */
    char *p0 = text;
    const char *pend = text + nread;
    do {
        char *p1 = p0;
        char *p2 = p0;

        while (*p2 != '\n' && *p2 != '\0') { p2++; }
        *p2 = '\0';
        p0 = p2 + 1;

        char *p = p1;
        while (*p != '\0' && *p != '#') { p++; }
        *p = '\0';

        p = p1;
        while (p < p2 && *p != '=') { p++; }
        if (*p == '=') {
            *p = '\0';
            p++;
        }

        m_wits.config.recs[m_wits.config.nrecs].item = WITS_REC_NONE;
        if (strcasecmp(p1, "rec01") == 0) {
            // wellid
            m_wits.config.recs[m_wits.config.nrecs].item = WITS_WELL_ID;
            m_wits.config.recs[m_wits.config.nrecs].code = 1;
            strcpy(m_wits.config.recs[m_wits.config.nrecs].text, p);
        } else if (strcasecmp(p1, "rec03") == 0) {
            // type
            m_wits.config.recs[m_wits.config.nrecs].item = WITS_REC_TYPE;
            m_wits.config.recs[m_wits.config.nrecs].code = 3;
            strcpy(m_wits.config.recs[m_wits.config.nrecs].text, p);
        } else {
            const int code = (int) strtol(p, NULL, 10);
            if (code > 0) {
                if (strcasecmp(p1, "pressure1") == 0) {
                    m_wits.config.recs[m_wits.config.nrecs].item = WITS_REC_PS1;
                } else if (strcasecmp(p1, "pressure2") == 0) {
                    m_wits.config.recs[m_wits.config.nrecs].item = WITS_REC_PS2;
                } else if (strcasecmp(p1, "hookload") == 0) {
                    m_wits.config.recs[m_wits.config.nrecs].item = WITS_REC_HKLD;
                } else if (strcasecmp(p1, "encoder1") == 0) {
                    m_wits.config.recs[m_wits.config.nrecs].item = WITS_REC_ENC1;
                } else if (strcasecmp(p1, "encoder2") == 0) {
                    m_wits.config.recs[m_wits.config.nrecs].item = WITS_REC_ENC2;
                }
            }
            if (m_wits.config.recs[m_wits.config.nrecs].item != WITS_REC_NONE) {
                m_wits.config.recs[m_wits.config.nrecs].code = code;
            }
        }
        if (m_wits.config.recs[m_wits.config.nrecs].item != WITS_REC_NONE) {
            m_wits.config.nrecs++;
        }
    } while (p0 < pend);

    qsort(m_wits.config.recs, m_wits.config.nrecs, sizeof(m_wits.config.recs[0]), compare);

    if (debug_level >= 3) {
        for (int i = 0; i < wits()->config.nrecs; i++) {
            DBGF(3, "%d - %-3d - '%s'", wits()->config.recs[i].item, wits()->config.recs[i].code,
                 wits()->config.recs[i].text);
        }
    }
}


//---------------------------------------------------------
inline void wits_send_data()
{
    pthread_mutex_lock(&wits()->mutex);
    wits_create_packet();
    DBGF(2, ">>> Wits packet #%-2d (size: %d)", wits()->seqnum, wits()->packet.nbytes);
    thr_wakeup_by_type(THR_TYPE_WITS_CLIENT);
    pthread_mutex_unlock(&wits()->mutex);
}

/*---------------------------------------------------------*/
bool wits_run()
{
    DBG(1);
    wits_read_config();
    m_wits.host = cfg()->wits_host;
    m_wits.port = cfg()->wits_port;
    wits_packet_init();
    THR_START(thr, THR_DETACH, wits_thread_start, wits_thread_stop, &m_wits, false, "Wits");
    if (!thr) {
        return FAIL;
    }
    thr->type = THR_TYPE_WITS;
    DBG_NSLEEP(TS_100MS);
    return OK;
}
