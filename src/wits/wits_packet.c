//
// Created by LAV on 25.08.2026.
//


#include "dbg.h"
#include "utils.h"
#include "wits.h"

#include <stdlib.h>
#include <string.h>

#include "core.h"

//---------------------------------------------------------
void wits_packet_init()
{
    wits_packet_free();
    wits()->packet.nbytes = 0;
    wits()->seqnum = 0;
    wits()->tm = clock_get_local_time(&wits()->timestamp);
}

//---------------------------------------------------------
void wits_packet_free()
{
    DBG(8);
    if (wits()->packet.bytes) {
        free(wits()->packet.bytes);
    }
    wits()->packet.bytes = NULL;
    wits()->packet.nbytes = 0;
    wits()->packet.nmem = 0;
}

//---------------------------------------------------------
static int find_cfg_item(WITS_REC_ITEM item)
{
    for (int i = 0; i < wits()->config.nrecs; i++) {
        if (wits()->config.recs[i].item == item) {
            return i;;
        };
    }
    return -1;
}


//---------------------------------------------------------
static int make_wits_record(char *msg, int msg_sz)
{
    msg[0] = '\0';
    /*
    # Updated: Thu Sep  3 14:44:23 2026
    # Created: Wed Aug 26 22:10:37 2026
    #
    rec01=The Well      # 1. Идентификатор скважины
    rec03=151           # 3. Идентификатор записи
                        # 4. Идентификатор последовательности записи
                        # 5. Дата
                        # 6. Время
    pressure1=21        # 21. Давление раствора на входе (среднее)
    pressure2=42        # 41. Давление раствора на входе датчик #2 (среднее)
    hookload=14         # 14. Вес на крюке (средний)
    encoder1=12         # 12. Положение талевого блока
    encoder2=0          #
    */

    const double PS1 = core()->data.values[0].avrg * cfg()->adc_scale; // Pressure Sensor #1
    const double PS2 = core()->data.values[1].avrg * cfg()->adc_scale; // Pressure Sensor #2
    const double HKLD = core()->data.HKLD24bit * cfg()->adc_scale; // Hookload
    const int32_t ENC1 = core()->data.encoder1; // Weight
    const int32_t ENC2 = core()->data.encoder2; // ?

    const char *wellid = "The Well";
    int idx = find_cfg_item(WITS_WELL_ID);
    if (idx != -1) {
        wellid = wits()->config.recs[idx].text;
    }

    const char *rectype = "151";
    idx = find_cfg_item(WITS_REC_TYPE);
    if (idx != -1) {
        rectype = wits()->config.recs[idx].text;
    }

    char dat[16];
    char tim[16];
    const int recid = 1;
    strftime(dat, sizeof(dat), "%y%m%d", &wits()->tm);
    strftime(tim, sizeof(tim), "%H%M%S", &wits()->tm);
    int len = snprintf(msg, msg_sz, "&&\n%02d01%s\n%02d03%s\n%02d04%d\n%02d05%s\n%02d06%s\n",
        recid, wellid, recid, rectype, recid, wits()->seqnum,
        recid, dat, recid, tim);
    msg_sz -= len;

    for (int i = 0; i < wits()->config.nrecs && msg_sz > 3; i++) {
        if (wits()->config.recs[i].item <= WITS_REC_TYPE || wits()->config.recs[i].code == 0) {
            continue;
        }

        int ln = 0;
        if (wits()->config.recs[i].item == WITS_REC_PS1) {
            ln = snprintf(msg + len, msg_sz, "%02d%d%0.02f\n", recid, wits()->config.recs[i].code, PS1);
        } else if (wits()->config.recs[i].item == WITS_REC_PS2) {
            ln = snprintf(msg + len, msg_sz, "%02d%d%0.02f\n", recid, wits()->config.recs[i].code, PS2);
        } else if (wits()->config.recs[i].item == WITS_REC_HKLD) {
            ln = snprintf(msg + len, msg_sz, "%02d%d%0.02f\n", recid, wits()->config.recs[i].code, HKLD);
        } else if (wits()->config.recs[i].item == WITS_REC_ENC1) {
            ln = snprintf(msg + len, msg_sz, "%02d%d%d\n", recid, wits()->config.recs[i].code, ENC1);
        } else if (wits()->config.recs[i].item == WITS_REC_ENC2) {
            ln = snprintf(msg + len, msg_sz, "%02d%d%d\n", recid, wits()->config.recs[i].code, ENC2);
        }
        msg_sz -= ln;
        len += ln;
    }
    strcat(msg, "!!\n");
    replace_dot_to_comma(cfg()->decimal, msg);

    if (debug_level >= 3) {
        char *str = strdup(msg);
        if (str) {
            replace(str, '\n', ' ');
            DBGF(3, "%s", str);
            free(str);
        }
    }
    return (int) strlen(msg);
}

//---------------------------------------------------------
void wits_create_packet()
{
    if (wits()->packet.nmem == 0) {
        wits()->packet.nmem = 256;
        wits()->packet.bytes = malloc(wits()->packet.nmem);
    }
    wits()->tm = clock_get_local_time(&wits()->timestamp);
    wits()->seqnum++;
    wits()->packet.nbytes = make_wits_record(wits()->packet.bytes, wits()->packet.nmem);
    DBGF(3, "created %d bytes message", wits()->packet.nbytes);
}
