/**
  \file   siu_cmd_куызщтыу.c
  \author Aleksey Lekhtin
  \date   Feb  5, 2026
  \brief  The SIU command куызщтыу
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/reboot.h>
#include <unistd.h>

#include "cfg.h"
#include "dbg.h"
#include "enc.h"
#include "siu.h"
#include "siu_socket.h"
#include "utils.h"
#include "hwclock.h"
#include "log.h"

static bool need_to_save_cfg = NO;

//---------------------------------------------------------
uint32_t siu_system_status()
{
    return (uint32_t) siu()->vars.sys_status.status_flag;
}

//---------------------------------------------------------
uint32_t siu_dev_status()
{
    return (uint32_t) siu()->vars.dev_status.led_flag;
}

//---------------------------------------------------------
void siu_update_dev_status(uint32_t status)
{
    siu()->vars.dev_status.led_flag = (LedFlagBits) status;
    siu()->vars.dev_status.PRES1AVAIL = (siu()->vars.dev_status.led_flag & PRES1AVAIL) != 0;
    siu()->vars.dev_status.PRES1ACVTY = (siu()->vars.dev_status.led_flag & PRES1ACVTY) != 0;
    siu()->vars.dev_status.PRES2AVAIL = (siu()->vars.dev_status.led_flag & PRES2AVAIL) != 0;
    siu()->vars.dev_status.PRES2ACVTY = (siu()->vars.dev_status.led_flag & PRES2ACVTY) != 0;
    siu()->vars.dev_status.HKLDAVAIL = (siu()->vars.dev_status.led_flag & HKLDAVAIL) != 0;
    siu()->vars.dev_status.HKLDACVTY = (siu()->vars.dev_status.led_flag & HKLDACVTY) != 0;
    siu()->vars.dev_status.DRUMPOSACVTY = (siu()->vars.dev_status.led_flag & DRUMPOSACVTY) != 0;
    siu()->vars.dev_status.DRUMNEGACVTY = (siu()->vars.dev_status.led_flag & DRUMNEGACVTY) != 0;
    siu()->vars.dev_status.SERDEPTHAVAIL = (siu()->vars.dev_status.led_flag & SERDEPTHAVAIL) != 0;
    siu()->vars.dev_status.SERDEPTHACVTY = (siu()->vars.dev_status.led_flag & SERDEPTHACVTY) != 0;
    siu()->vars.dev_status.TOOLPOWERON = (siu()->vars.dev_status.led_flag & TOOLPOWERON) != 0;
    siu()->vars.dev_status.PUMPSTATUS = (siu()->vars.dev_status.led_flag & PUMPSTATUS) != 0;
    siu()->vars.dev_status.RFDCONNECTED = (siu()->vars.dev_status.led_flag & RFDCONNECTED) != 0;
    siu()->vars.dev_status.TOOLCOMMSACVTY = (siu()->vars.dev_status.led_flag & TOOLCOMMSACVTY) != 0;
    siu()->vars.dev_status.TOOLPOWERFAULT = (siu()->vars.dev_status.led_flag & TOOLPOWERFAULT) != 0;
}

//---------------------------------------------------------
void siu_update_system_status_flags(uint32_t status)
{
    siu()->vars.sys_status.status_flag = (StatusFlagBits) status;
    siu()->vars.sys_status.UNUSED = (siu()->vars.sys_status.status_flag & UNUSED) != 0;
    siu()->vars.sys_status.KTABLEERR = (siu()->vars.sys_status.status_flag & KTABLEERR) != 0;
    siu()->vars.sys_status.STORAGEERR = (siu()->vars.sys_status.status_flag & STORAGEERR) != 0;
    siu()->vars.sys_status.STORAGEFULL = (siu()->vars.sys_status.status_flag & STORAGEFULL) != 0;
    siu()->vars.sys_status.A2DACCESSERR = (siu()->vars.sys_status.status_flag & A2DACCESSERR) != 0;
    siu()->vars.sys_status.A2DFIFOERR = (siu()->vars.sys_status.status_flag & A2DFIFOERR) != 0;
    siu()->vars.sys_status.WTABLEERR = (siu()->vars.sys_status.status_flag & WTABLEERR) != 0;
    siu()->vars.sys_status.DTABLEERR = (siu()->vars.sys_status.status_flag & DTABLEERR) != 0;
    siu()->vars.sys_status.ATABLEERR = (siu()->vars.sys_status.status_flag & ATABLEERR) != 0;
    siu()->vars.sys_status.DEPTHSYSTEMINT = (siu()->vars.sys_status.status_flag & DEPTHSYSTEMINT) != 0;
    siu()->vars.sys_status.DEPTHBOTTOM = (siu()->vars.sys_status.status_flag & DEPTHBOTTOM) != 0;
    siu()->vars.sys_status.DEPTHSLIPS = (siu()->vars.sys_status.status_flag & DEPTHSLIPS) != 0;
    siu()->vars.sys_status.DEPTHFORCING = (siu()->vars.sys_status.status_flag & DEPTHFORCING) != 0;
    siu()->vars.sys_status.DECODERCOMERR = (siu()->vars.sys_status.status_flag & DECODERCOMERR) != 0;
    siu()->vars.sys_status.PLAYBACKON = (siu()->vars.sys_status.status_flag & PLAYBACKON) != 0;
    siu()->vars.sys_status.DECODINGDATA = (siu()->vars.sys_status.status_flag & DECODINGDATA) != 0;
    siu()->vars.sys_status.SIGNALDETECT = (siu()->vars.sys_status.status_flag & SIGNALDETECT) != 0;
    siu()->vars.sys_status.DEPTHSYSTEMEXT = (siu()->vars.sys_status.status_flag & DEPTHSYSTEMEXT) != 0;
    siu()->vars.sys_status.CONFIGDATAERR = (siu()->vars.sys_status.status_flag & CONFIGDATAERR) != 0;
    siu()->vars.sys_status.IODVRACCESSERR = (siu()->vars.sys_status.status_flag & IODVRACCESSERR) != 0;
    siu()->vars.sys_status.KEYPADERROR = (siu()->vars.sys_status.status_flag & KEYPADERROR) != 0;
    siu()->vars.sys_status.IOSTORAGEERR = (siu()->vars.sys_status.status_flag & IOSTORAGEERR) != 0;
    siu()->vars.sys_status.IOSTORAGEFULL = (siu()->vars.sys_status.status_flag & IOSTORAGEFULL) != 0;
    siu()->vars.sys_status.DECODELOGFULL = (siu()->vars.sys_status.status_flag & DECODELOGFULL) != 0;
    siu()->vars.sys_status.DEPTHSYSTEMON = (siu()->vars.sys_status.status_flag & DEPTHSYSTEMON) != 0;
}

/*-------------------------------------------------------*/
/**
 * Выделение одной команды из строки с командами (одной команды)
 * @note указатель на начало команв сдвигается на размер первой выделенной команды
 * @param commands - строка с командами
 * @return первая выделенная команда
 */
static const char *siu_get_item(const char **commands)
{
    // [@VARNAME][@VER:1][@VER:2][@VER:3][@VER:4][@VER:5][@IOINSTALLER] ...
    static char buf[1024];
    char *p = strchr(*commands, ']');
    if (!p) {
        return NULL; // no more items
    }
    int len = (int) ((uintptr_t) p - (uintptr_t) (*commands));
    len++; // ']'
    if (len > (int) sizeof(buf) - 1) {
        len = sizeof(buf) - 1;
    }
    memcpy(buf, *commands, len);
    buf[len] = '\0';
    *commands += len;
    return buf;
}

/*-------------------------------------------------------*/
/**
 * Разбор строки на поля. Поля разделены ':'
 * @param[in] str - строка типа "[\@APSLAN:{0}:{1}:{2}:{3}]"
 * @param[out] nfields - число найденных полей
 * @return список указателей на поля
 */
const char **siu_get_fields(const char *str, int *nfields)
{
    // "[@APSLAN:{0}:{1}:{2}:{3}]"
#define N_FIELDS 128
    static const char *fields[N_FIELDS];
    memset((void *)fields, 0, sizeof(fields));
    *nfields = 0;

    static char buf[1024];
    strncpy(buf, str, sizeof(buf) - 1);
    char *p = buf;
    while (*p && *p != '[') {
        p++;
    }
    if (*p != '[') {
        return NULL; // no fields, incorrect string mode
    }
    *p = '\0';
    p++;

    int idx = 0;
    fields[idx] = p;
    idx++;
    p++;
    while (*p && idx < N_FIELDS) {
        if (*p == ':') {
            *p = '\0';
            fields[idx] = (p + 1);
            idx++;
        } else if (*p == ']') {
            *p = '\0';
        }
        p++;
    }
    *nfields = idx;
    return fields;
}

//---------------------------------------------------------
/**
 * parse string with ADS sync date
 * @param item - string like [\@DMYE:155028.36:030325]
 */
static void parse_DMY(const char *item, struct tm *tm)
{
    *tm = (struct tm) {};
    // [@DMY:142953:040425]
    if (strncasecmp(item, "[@DMY:", 6) != 0) {
        return;
    }
    char buf[256] = {0};
    char bf[32] = {0};
    strncpy(buf, item, sizeof(buf) - 1);
    strncpy(bf, item, sizeof(bf) - 1);
    bf[19] = 0;
    tm->tm_year = strtol(bf + 17, NULL, 10) + 100;
    bf[17] = 0;
    tm->tm_mon = strtol(bf + 15, NULL, 10) - 1;
    bf[15] = 0;
    tm->tm_mday = strtol(bf + 13, NULL, 10);
    bf[13] = 0;
    bf[12] = 0;
    tm->tm_sec = strtol(bf + 10, NULL, 10);
    bf[10] = 0;
    tm->tm_min = strtol(bf + 8, NULL, 10);
    bf[8] = 0;
    tm->tm_hour = strtol(bf + 6, NULL, 10);
}


//---------------------------------------------------------
static void parse_FormatTable(const char *inbuf, char *outbuf, size_t outbuf_size)
{
    // get - [@FMT:00:01]
    // get - [@FMT:00:02]
    // set - [@FMT:00:03:GZX:0012:4095.000:-1.200000:1.200000:G's:08:04:01:01]
    // set - [@FMT:01:01:000E:0001:0001:0001:0002:0000:0000:0000:0001:0002:0003:0004:0005:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF]

    int nflds;
    siu_vars_t *vars = &siu()->vars;
    const char **fields = siu_get_fields(inbuf, &nflds);
    outbuf[0] = 0;

    mpt_format_t table_type = (mpt_format_t) strtoul(fields[1], NULL, 10);
    int idx = (int) strtoul(fields[2], NULL, 10);
    idx--;

    if ((table_type == MPT_DATA && idx > MPT_DATA_SIZE) ||
        (table_type == MPT_FORMAT && idx > MPT_FORMAT_SIZE)) {
        snprintf(outbuf, outbuf_size, "[#FMT]");
        return;
    }

    //-------------------------------------------
    if (nflds == 3) {
        // get
        if (table_type == MPT_DATA) {
            snprintf(outbuf, outbuf_size,
                     "[#FMT:%02d:%02d:%s:%04d:%0.3f:%0.6f:%0.6f:%s:%02d:%02d:%02d:%02d]",
                     (int)table_type, (idx + 1),
                     vars->mpt.mpt_data[idx].mnemonic,
                     vars->mpt.mpt_data[idx].length,
                     vars->mpt.mpt_data[idx].scale_factor,
                     vars->mpt.mpt_data[idx].low_limit,
                     vars->mpt.mpt_data[idx].high_limit,
                     vars->mpt.mpt_data[idx].units,
                     vars->mpt.mpt_data[idx].disp_length,
                     vars->mpt.mpt_data[idx].disp_places,
                     vars->mpt.mpt_data[idx].print_ctrl,
                     vars->mpt.mpt_data[idx].scale_type);
            return;
        }

        if (table_type == MPT_FORMAT) {
            snprintf(outbuf, outbuf_size,
                     "[#FMT:%02d:%02d:%04X:%04X:%04X:%04X:%04X:%04X:%04X:"
                     "%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X]",
                     (int)table_type, (idx + 1),
                     vars->mpt.mpt_format[idx].FID,
                     vars->mpt.mpt_format[idx].FIDType,
                     vars->mpt.mpt_format[idx].enableSync,
                     vars->mpt.mpt_format[idx].repeatCount,
                     vars->mpt.mpt_format[idx].resync,
                     vars->mpt.mpt_format[idx].loopback,
                     vars->mpt.mpt_format[idx].alert,
                     vars->mpt.mpt_format[idx].dataList[0],
                     vars->mpt.mpt_format[idx].dataList[1],
                     vars->mpt.mpt_format[idx].dataList[2],
                     vars->mpt.mpt_format[idx].dataList[3],
                     vars->mpt.mpt_format[idx].dataList[4],
                     vars->mpt.mpt_format[idx].dataList[5],
                     vars->mpt.mpt_format[idx].dataList[6],
                     vars->mpt.mpt_format[idx].dataList[7],
                     vars->mpt.mpt_format[idx].dataList[8],
                     vars->mpt.mpt_format[idx].dataList[9],
                     vars->mpt.mpt_format[idx].dataList[10],
                     vars->mpt.mpt_format[idx].dataList[11]);
        }
        return;
    }

    //-------------------------------------------
    if (nflds == 13 || nflds == 22) {
        // set - [@FMT:00:03:GZX:0012:4095.000:-1.200000:1.200000:G's:08:04:01:01]
        // set - [@FMT:01:01:000E:0001:0001:0001:0002:0000:0000:0000:0001:0002:0003:0004:0005:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF]

        if (table_type == MPT_DATA) {
            memset(&vars->mpt.mpt_data[idx], 0, sizeof(vars->mpt.mpt_data[idx]));
            for (int i = 3; i < nflds; i++) {
                if (i == 3) {
                    strncpy(vars->mpt.mpt_data[idx].mnemonic, fields[i],
                            sizeof(vars->mpt.mpt_data[idx].mnemonic));

                } else if (i == 4 || (i >= 9 && i <= 12)) {
                    errno = 0;
                    uint num = strtol(fields[i], NULL, 10);
                    if (errno == 0) {
                        if (i == 4) {
                            vars->mpt.mpt_data[idx].length = num;
                        } else if (i == 9) {
                            vars->mpt.mpt_data[idx].disp_length = num;
                        } else if (i == 10) {
                            vars->mpt.mpt_data[idx].disp_places = num;
                        } else if (i == 11) {
                            vars->mpt.mpt_data[idx].print_ctrl = num;
                        } else if (i == 12) {
                            vars->mpt.mpt_data[idx].scale_type = num;
                        }
                    }
                } else if (i >= 5 && i <= 7) {
                    errno = 0;
                    float v = strtof(fields[i], NULL);
                    if (errno == 0) {
                        if (i == 5) {
                            vars->mpt.mpt_data[idx].scale_factor = v;
                        } else if (i == 6) {
                            vars->mpt.mpt_data[idx].low_limit = v;
                        } else if (i == 7) {
                            vars->mpt.mpt_data[idx].high_limit = v;
                        }
                    }
                } else if (i == 8) {
                    strncpy(vars->mpt.mpt_data[idx].units, fields[i],
                            sizeof(vars->mpt.mpt_data[idx].units));
                }
            }
            DBGF(1, "+++ Set MPT Data[%d] '%s'", idx + 1, vars->mpt.mpt_data[idx].mnemonic);

        } else if (table_type == MPT_FORMAT) {

            memset(&vars->mpt.mpt_format[idx], 0, sizeof(vars->mpt.mpt_format[idx]));
            for (int i = 3; i < nflds; i++) {
                errno = 0;
                uint num = strtol(fields[i], NULL, 16);
                if (errno == 0) {
                    if (i == 3) {
                        vars->mpt.mpt_format[idx].FID = num;
                    } else if (i == 4) {
                        vars->mpt.mpt_format[idx].FIDType = num;
                    } else if (i == 5) {
                        vars->mpt.mpt_format[idx].enableSync = num;
                    } else if (i == 6) {
                        vars->mpt.mpt_format[idx].repeatCount = num;
                    } else if (i == 7) {
                        vars->mpt.mpt_format[idx].resync = num;
                    } else if (i == 8) {
                        vars->mpt.mpt_format[idx].loopback = num;
                    } else if (i == 9) {
                        vars->mpt.mpt_format[idx].alert = num;
                    } else if (i == 10) {
                        vars->mpt.mpt_format[idx].dataList[0] = num;
                    } else if (i == 11) {
                        vars->mpt.mpt_format[idx].dataList[1] = num;
                    } else if (i == 12) {
                        vars->mpt.mpt_format[idx].dataList[2] = num;
                    } else if (i == 13) {
                        vars->mpt.mpt_format[idx].dataList[3] = num;
                    } else if (i == 14) {
                        vars->mpt.mpt_format[idx].dataList[4] = num;
                    } else if (i == 15) {
                        vars->mpt.mpt_format[idx].dataList[5] = num;
                    } else if (i == 16) {
                        vars->mpt.mpt_format[idx].dataList[6] = num;
                    } else if (i == 17) {
                        vars->mpt.mpt_format[idx].dataList[7] = num;
                    } else if (i == 18) {
                        vars->mpt.mpt_format[idx].dataList[8] = num;
                    } else if (i == 19) {
                        vars->mpt.mpt_format[idx].dataList[9] = num;
                    } else if (i == 20) {
                        vars->mpt.mpt_format[idx].dataList[10] = num;
                    } else if (i == 21) {
                        vars->mpt.mpt_format[idx].dataList[11] = num;
                    }
                }
            }
            DBGF(1, "+++ Set MPT Table[%d] FID: %d", idx + 1, (int16_t)vars->mpt.mpt_format[idx].FID);
        }
        need_to_save_cfg = YES;
    }
    snprintf(outbuf, outbuf_size, "[#FMT]");
}

//---------------------------------------------------------
static void sendout(const char *buf, int (*send_response)(socket_t *, const char *, int), socket_t *sock)
{
    if (buf[0]) {
        DBGF(1, "%s%s%s", (cfg()->shortmsg == 0 ? "--->>> response: " : ""),
             buf, (debug_level == 1 ? "\n": ""));
        send_response(sock, buf, (int) strlen(buf));
    }
    if (need_to_save_cfg) {
        siu_save_data_status();
        need_to_save_cfg = NO;
    }
}


//---------------------------------------------------------
 /**
  * Отправка ответа на команду
  * @param command  - команда типа "[\@SERNO:abc:xyz]"
  * @param vars - рабочие данные
  * @param sock - соединение для обмена данными
  * @param send_response - функция отправки данных
  */
 void siu_cmd_response(const char *command, siu_vars_t *vars, int (*send_response)(socket_t *, const char *, int),
                       socket_t *sock)
{
    static char outbuf[1024*2] = {0};
    int   num;
    int   nflds;
    short snum;
    float val = 0.0f;
    const char **fields;
    const char *fmt1 = "%s%s (%s)\n";
    const char *fmt2 = "%s%s%s\n";
    const char *fmt;

    const char *pcmd = strchr(command, '[');
    while (1) {
        const char *inbuf = siu_get_item(&pcmd);
        if (!inbuf) {
            break;
        }
        const char *tagname = siu_cmd_brakets_comment(inbuf);
        fmt = (tagname ? fmt1 : fmt2);
        DBGFNoNL(1, fmt, (cfg()->shortmsg == 0 ? "<<<--- request:  " : ""), inbuf, tagname);
        outbuf[0] = 0;

        if (strcasecmp(inbuf, "[@ERR]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#ERR]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@RBT]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#RBT]");
            need_to_save_cfg = YES;
            sendout(outbuf, send_response, sock);
            DBG(1, "sync()");
            sync();
            setuid(0);
            DBG(1, "reboot()");
            reboot(RB_AUTOBOOT);
            continue;
        }
        if (strcasecmp(inbuf, "[@RST]") == 0) {
            // Reset Decoder - чего делать в SIU2 ?
            snprintf(outbuf, sizeof(outbuf), "[#RST]");
            sendout(outbuf, send_response, sock);
            continue;
        }
        if (strcasecmp(inbuf, "[@SERNUM]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#SERNUM:%s]", vars->serno);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@SERNUM:", 9) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                memset(vars->serno, 0, sizeof(vars->serno));
                strncpy(vars->serno, fields[1], sizeof(vars->serno) - 1);
                memset(cfg()->siu_serno, 0, sizeof(cfg()->siu_serno));
                strncpy(cfg()->siu_serno, fields[1], sizeof(cfg()->siu_serno) - 1);
            }
            snprintf(outbuf, sizeof(outbuf), "[#SERNUM]");
            cfg_save_serial_number(MODE_APS);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@LEDTEST]") == 0) {
            strncpy(outbuf, "[#LEDTEST:1]", sizeof(outbuf) - 1);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@DMY]") == 0) {
            struct timespec ts;
            struct tm tm = clock_get_local_time(&ts);
            snprintf(outbuf, sizeof(outbuf), "[#DMY:%02d%02d%02d:%02d%02d%02d]", tm.tm_hour, tm.tm_min,
                     tm.tm_sec, tm.tm_mday, tm.tm_mon + 1, tm.tm_year - 100);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@DMY:", 6) == 0) {
            // [@DMY:142953:040425]
            struct tm tm = {0};
            struct timespec ts;
            parse_DMY(inbuf, &tm);
            time_t secs = mktime(&tm);
            if (secs == -1) {
                int e = errno;
                LOG(ERROR, "mktime() failed (%s)", strerror(e));
                DBGF(0, "\nERROR: mktime() failed (%s)", strerror(e));
            } else {
                ts.tv_sec = secs;
                ts.tv_nsec = 0;
                int rc = clock_settime(CLOCK_REALTIME, &ts);
                if (rc == -1) {
                    int e = errno;
                    LOG(ERROR, "clock_settime() failed - %s", strerror(e));
                    DBGF(0, "\nERROR: clock_settime() failed - %s", strerror(e));
                } else {
                    const char *stime = timespec_to_dd_mm_yyyy_hh_mm_ss_nz(&ts, 6, NULL);
                    DBGF(1, "\nhost time changed to ADS time - %s", stime);
                    bool ok = hwclock();
                    if (ok) {
                        DBG(1, "hardware clock is updated");
                    } else {
                        DBG(1, "hardware clock update is FAILED");
                    }
                }
            }

            tm = clock_get_local_time(&ts);
            snprintf(outbuf, sizeof(outbuf), "[#DMY:%02d%02d%02d:%02d%02d%02d]", tm.tm_hour, tm.tm_min,
                     tm.tm_sec, tm.tm_mday, tm.tm_mon + 1, tm.tm_year - 100);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@SYNCORR]", 10) == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#SYNCORR:%0.2f:%0.2f:%0.2f:%0.2f]",
                     vars->L1corrPkData,
                     vars->L1corrThresh,
                     vars->L2corrPkData,
                     vars->L2corrThresh);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@RFDADDR]") == 0) {
            // "[@RFDADDR:{0}]"
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                errno = 0;
                num = strtol(fields[1], NULL, 10);
                if (errno == 0) {
                    if (num == 1) {
                        snprintf(outbuf, sizeof(outbuf), "[#RFDADDR:%d:%s]", num, vars->rfd1Ip);
                    } else if (num == 2) {
                        snprintf(outbuf, sizeof(outbuf), "[#RFDADDR:%d:%s]", num, vars->rfd2Ip);
                    }
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#RFDADDR]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@RFDADDR:", 10) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 3) {
                // "[@RFDADDR:{0}:{1}]"
                errno = 0;
                num = (int) strtol(fields[1], NULL, 10);
                if (errno == 0) {
                    if (num == 1) {
                        strncpy(vars->rfd1Ip, fields[2], sizeof(vars->rfd1Ip) - 1);
                        DBGF(1, "+++ Set RFDADDR IP1: %s", vars->rfd1Ip);
                    } else if (num == 2) {
                        strncpy(vars->rfd2Ip, fields[2], sizeof(vars->rfd2Ip) - 1);
                        DBGF(1, "+++ Set RFDADDR IP2: %s", vars->rfd2Ip);
                    }
                    need_to_save_cfg = YES;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#RFDADDR]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@RFDAZTYPE]") == 0) {
            const char *str = "";
            if (vars->RfdAzimuthCorrection == AZCorr_Magnetic) {
                str = "MAG";
            } else if (vars->RfdAzimuthCorrection == AZCorr_True) {
                str = "TRUE";
            } else if (vars->RfdAzimuthCorrection == AZCorr_Grid) {
                str = "GRID";
            }
            if (str[0]) {
                snprintf(outbuf, sizeof(outbuf), "[#RFDAZTYPE:%s]", str);
            } else {
                snprintf(outbuf, sizeof(outbuf), "[#RFDAZTYPE]");
            }
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@RFDAZTYPE:", 12) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                if (strcmp(fields[1], "MAG") == 0) {
                    vars->RfdAzimuthCorrection = AZCorr_Magnetic;
                    DBGF(1, "+++ Set RfdAzimuthCorrection %d", vars->RfdAzimuthCorrection);
                } else if (strcmp(fields[1], "TRUE") == 0) {
                    vars->RfdAzimuthCorrection = AZCorr_True;
                    DBGF(1, "+++ Set RfdAzimuthCorrection %d", vars->RfdAzimuthCorrection);
                } else {
                    vars->RfdAzimuthCorrection = AZCorr_Grid;
                    DBGF(1, "+++ Set RfdAzimuthCorrection %d", vars->RfdAzimuthCorrection);
                }
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#RFDAZTYPE]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@RFDCOMM]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#RFDCOMM:%s]", (vars->RfdCommOn ? "ON" : "OFF"));
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@RFDCOMM", 9) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                vars->RfdCommOn = (strcmp(fields[1], "ON") == 0);
                DBGF(1, "+++ Set RfdCommOn %s", (vars->RfdCommOn ? "ON" : "OFF"));
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#RFDCOMM]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@RFDGRDEC]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#RFDGRDEC:%0.2f]", vars->RfdAzimuthGridCorrection);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@RFDGRDEC:", 11) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                char bf[32];
                snprintf(bf, sizeof(bf), "%s", fields[1]);
                replace_comma_to_dot(bf);

                errno = 0;
                val = strtof(bf, NULL);
                if (errno == 0) {
                    if (val >= -360.0 && val <= 360.0) {
                        vars->RfdAzimuthGridCorrection = val;
                        DBGF(1, "+++ Set RfdAzimuthGridCorrection %0.1f", vars->RfdAzimuthGridCorrection);
                        need_to_save_cfg = YES;
                    } else {
                        LOG(INFO, "%s - Grid correction must be between -360 and 360", inbuf);
                        DBGF(0, "%s - Grid correction must be between -360 and 360", inbuf);
                    }
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#RFDGRDEC]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@RFDMAGDEC]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#RFDMAGDEC:%0.2f]", vars->RfdAzimuthMagneticDeclination);
            replace_dot_to_comma(cfg()->decimal, outbuf);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@RFDMAGDEC:", 12) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                char bf[32];
                snprintf(bf, sizeof(bf), "%s", fields[1]);
                replace_comma_to_dot(bf);

                errno = 0;
                val = strtof(bf, NULL);
                if (errno == 0) {
                    if (val >= -360.0 && val <= 360.0) {
                        vars->RfdAzimuthMagneticDeclination = val;
                        DBGF(1, "+++ Set RfdAzimuthMagneticDeclination %0.1f",
                             vars->RfdAzimuthMagneticDeclination);
                        need_to_save_cfg = YES;
                    } else {
                        LOG(INFO, "%s - Magnetic declination must be between -360 and 360", inbuf);
                        DBGF(0, "%s - Magnetic declination must be between -360 and 360", inbuf);
                    }
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#RFDMAGDEC]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@RFDHBI]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#RFDHBI:%d]", vars->RfdHeartbeatSeconds);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@RFDHBI:", 9) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                errno = 0;
                num = strtol(fields[1], NULL, 10);
                if (errno == 0) {
                    vars->RfdHeartbeatSeconds = strtol(fields[1], NULL, 10);
                    DBGF(1, "+++ Set RfdHeartbeatSeconds %d", vars->RfdHeartbeatSeconds);
                    need_to_save_cfg = YES;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#RFDHBI]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@RFDPORT:", 10) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                errno = 0;
                num = strtol(fields[1], NULL, 10);
                if (errno == 0) {
                    vars->RfdNetworkPort = strtol(fields[1], NULL, 10);
                    DBGF(1, "+++ Set RfdNetworkPort %d", vars->RfdNetworkPort);
                    need_to_save_cfg = YES;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#RFDPORT]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@RFDPORT]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#RFDPORT:%d]", vars->RfdNetworkPort);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@RFDRFSH]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#RFDRFSH]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@RFDTFSC]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#RFDTFSC:%s]",
                     (vars->RfdToolfaceScale == Tool_Gravity ? "GSCL" : "MSCL"));

            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@RFDTFSC:", 10) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                if (strcmp(fields[1], "GSCL") == 0) {
                    vars->RfdToolfaceScale = Tool_Gravity;
                } else if (strcmp(fields[1], "MSCL") == 0) {
                    vars->RfdToolfaceScale = Tool_Magnetic;
                }
                DBGF(1, "+++ Set RfdToolfaceScale %d", vars->RfdToolfaceScale);
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#RFDTFSC]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@RFDTRAFFICLIGHT]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#RFDTRAFFICLIGHT:%s]",
                     vars->RfdTrafficLight ? "ON" : "OFF");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@RFDTRAFFICLIGHT:", 18) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                vars->RfdTrafficLight = (strcmp(fields[1], "ON") == 0);
                DBGF(1, "+++ Set RfdTrafficLight %s", (vars->RfdTrafficLight ? "ON" : "OFF"));
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#RFDTRAFFICLIGHT]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@RFDSCRIBELN]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#RFDSCRIBELN:%0.2f]", vars->RfdScribelineCorrection);
            replace_dot_to_comma(cfg()->decimal, outbuf);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@RFDSCRIBELN:", 14) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds > 1) {
                char bf[32];
                snprintf(bf, sizeof(bf), "%s", fields[1]);
                replace_comma_to_dot(bf);

                errno = 0;
                val = strtof(bf, NULL);
                if (errno == 0) {
                    if (val >= -360.0 && val <= 360.0) {
                        vars->RfdScribelineCorrection = val;
                        DBGF(1, "+++ Set RfdScribelineCorrection %0.1f", vars->RfdScribelineCorrection);
                        need_to_save_cfg = YES;
                    } else {
                        LOG(INFO, "%s - Scribeline correction must be between -360 and 360", inbuf);
                        DBGF(0, "%s - Scribeline correction must be between -360 and 360", inbuf);
                    }
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#RFDSCRIBELN]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@TEMPDEC]") == 0) {
            double temp = vars->tempdec;
            if (vars->SDU == UnitsEnglish) {
                temp = to_Fahrenheit(temp);
            }
            snprintf(outbuf, sizeof(outbuf), "[#TEMPDEC:%0.1f]", temp);
            replace_dot_to_comma(cfg()->decimal, outbuf);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@TEMPIO]") == 0) {
            double temp = vars->tempio;
            if (vars->SDU == UnitsEnglish) {
                temp = to_Fahrenheit(temp);
            }
            snprintf(outbuf, sizeof(outbuf), "[#TEMPIO:%0.1f]", temp);
            replace_dot_to_comma(cfg()->decimal, outbuf);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@DIE]") == 0) {
            // Depth Internal / External
            snprintf(outbuf, sizeof(outbuf), "[#DIE:%s]", vars->DIE);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@DIE:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                if (strcmp(fields[1], "INTERNAL") == 0 || strcmp(fields[1], "EXTERNAL") == 0) {
                    memset(vars->DIE, 0, sizeof(vars->DIE));
                    size_t len = strlen(fields[1]);
                    if (len > sizeof(vars->DIE)) {
                        len = sizeof(vars->DIE);
                    }
                    memcpy(vars->DIE, fields[1], len);
                    DBGF(1, "+++ Set Depth Source: %s", vars->DIE);
                    need_to_save_cfg = YES;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#DIE]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@DEN]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#DEN:%s]", (vars->DEN ? "ON" : "OFF"));
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@DEN:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                vars->DEN = (strcmp(fields[1], "ON") == 0);
                DBGF(1, "+++ Set Dsp Enable: %s", (vars->DEN ? "ON" : "OFF"));
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#DEN]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@DSS]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#DSS:%s]", (vars->DSS ? "ON" : "OFF"));
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@DSS:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                vars->DSS = (strcmp(fields[1], "ON") == 0);
                DBGF(1, "+++ Set Depth Tracking %s", (vars->DSS ? "ON" : "OFF"));
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#DSS]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@EQF]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#EQF:%s]", (vars->EQF ? "ON" : "OFF"));
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@EQF:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                vars->EQF = (strcmp(fields[1], "ON") == 0);
                DBGF(1, "+++ Set Equalizer %s", (vars->EQF ? "ON" : "OFF"));
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#EQF]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@TTR]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#TTR:%s]", (vars->TTR ? "ON" : "OFF"));
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@TTR:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                vars->TTR = (strcmp(fields[1], "ON") == 0);
                DBGF(1, "+++ Set Time Tracking %s", (vars->TTR ? "ON" : "OFF"));
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#TTR]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@ENC]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#ENC:%s]", mpt_type_to_string(vars->MPTENC));
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@ENC:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                mpt_encoding_t mpt = string_to_mpt(fields[1]);
                if (mpt != MPT_ERR) {
                    vars->MPTENC = mpt;
                    DBGF(1, "+++ Set Mpt Encoding: %s", mpt_type_to_string(vars->MPTENC));
                    need_to_save_cfg = YES;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#ENC]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@FLT]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#FLT:%02d]", vars->FLT);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@FLT:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                errno = 0;
                num = strtol(fields[1], NULL, 10);
                if (errno != 0) {
                    num = 0;
                }
                vars->FLT = num;
                DBGF(1, "+++ Set Filter Correlation Type: %02d", vars->FLT);
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#FLT]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@DFS]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#DFS:%02d]  (%s%s%s)",
                     vars->DFS,
                     (vars->DFS == NoForcing ? "NoForcing" : ""),
                     (vars->DFS == ForceInSlips ? "ForceInSlips" : ""),
                     (vars->DFS == ForceOutSlips ? "ForceOutSlips" : ""));
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@DFS:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2 || nflds == 3) {
                errno = 0;
                num = strtol(fields[1], NULL, 10);
                if (errno != 0) {
                    num = 0;
                }
                vars->DFS = num;
                DBGF(1, "+++ Set Force Drill State: %d  (%s%s%s)", vars->DFS,
                     (vars->DFS == NoForcing ? "NoForcing" : ""),
                     (vars->DFS == ForceInSlips ? "ForceInSlips" : ""),
                     (vars->DFS == ForceOutSlips ? "ForceOutSlips" : ""));
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#DFS]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@RTE]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#RTE:%05.2f]", vars->MPTRATE);
            replace_dot_to_comma(cfg()->decimal, outbuf);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@RTE:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                char bf[32];
                snprintf(bf, sizeof(bf), "%s", fields[1]);
                replace_comma_to_dot(bf);

                errno = 0;
                val = strtof(bf, NULL);
                if (errno != 0) {
                    val = 0;
                }
                vars->MPTRATE = val;
                DBGF(1, "+++ Set Mpt Rate: %05.2f", vars->MPTRATE);
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#RTE]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@OBT]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#OBT:%0.2f]", vars->OnBottomThreshold);
            replace_dot_to_comma(cfg()->decimal, outbuf);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@OBT:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                char bf[32];
                snprintf(bf, sizeof(bf), "%s", fields[1]);
                replace_comma_to_dot(bf);
                errno = 0;
                val = strtof(bf, NULL);
                if (errno == 0) {
                    vars->OnBottomThreshold = val;
                    DBGF(1, "+++ Set OnBottom Threshold: %0.2f", vars->OnBottomThreshold);
                    need_to_save_cfg = YES;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#OBT]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@KLD]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#KLD:%0.2f]", vars->DepthSystemData.BlockPosition);
            replace_dot_to_comma(cfg()->decimal, outbuf);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@KLD:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                char bf[32];
                snprintf(bf, sizeof(bf), "%s", fields[1]);
                replace_comma_to_dot(bf);
                errno = 0;
                val = strtof(bf, NULL);
                if (errno == 0) {
                    vars->DepthSystemData.BlockPosition = val;
                    DBGF(1, "+++ Set Block Position: %0.2f", vars->DepthSystemData.BlockPosition);
                    //esz7_set_encoder_position(ENCODER_BLOCKPOS, vars->DSD_Depth_Data.BlockPosition);
                    need_to_save_cfg = YES;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#KLD]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@KDD]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#KDD:%0.2f]", vars->DepthSystemData.KellyDownDepth);
            replace_dot_to_comma(cfg()->decimal, outbuf);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@KDD:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                char bf[32];
                snprintf(bf, sizeof(bf), "%s", fields[1]);
                replace_comma_to_dot(bf);
                errno = 0;
                val = strtof(bf, NULL);
                if (errno == 0) {
                    vars->DepthSystemData.KellyDownDepth = val;
                    DBGF(1, "+++ Set Kelly Down Depth: %0.2f", vars->DepthSystemData.KellyDownDepth);
                    need_to_save_cfg = YES;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#KDD]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@DBT]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#DBT:%0.2f]", vars->DepthSystemData.BitDepth);
            replace_dot_to_comma(cfg()->decimal, outbuf);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@DBT:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                char bf[32];
                snprintf(bf, sizeof(bf), "%s", fields[1]);
                replace_comma_to_dot(bf);
                errno = 0;
                val = strtof(bf, NULL);
                if (errno == 0) {
                    vars->DepthSystemData.BitDepth = val;
                    DBGF(1, "+++ Set Bit Depth: %0.2f feets", vars->DepthSystemData.BitDepth);
                    need_to_save_cfg = YES;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#DBT]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@DME]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#DME:%0.2f]", vars->DepthSystemData.HoleDepth);
            replace_dot_to_comma(cfg()->decimal, outbuf);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@DME:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                char bf[32];
                snprintf(bf, sizeof(bf), "%s", fields[1]);
                replace_comma_to_dot(bf);
                errno = 0;
                val = strtof(bf, NULL);
                if (errno == 0) {
                    vars->DepthSystemData.HoleDepth = val;
                    DBGF(1, "+++ Set Hole Depth: %0.2f", vars->DepthSystemData.HoleDepth);
                    need_to_save_cfg = YES;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#DME]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@HSC:", 6) == 0) {
            bool ok = OK;
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 7) {
                char bf[32];
                for (int i = 1; i < nflds; i++) {
                    snprintf(bf, sizeof(bf), "%s", fields[i]);
                    replace_comma_to_dot(bf);

                    errno = 0;
                    switch (i) {
                        case 1:
                        case 3:
                            num = strtol(bf, NULL, 10);
                            if (errno != 0) {
                                num = 0;
                            }
                            break;
                        default:
                            val = strtof(bf, NULL);
                            if (errno != 0) {
                                val = 0;
                            }
                            break;
                    }
                    if (i == 1) {
                        vars->HKLDCalibrationTable.a2d1 = num;
                    } else if (i == 2) {
                        vars->HKLDCalibrationTable.weight1 = val;
                    } else if (i == 3) {
                        vars->HKLDCalibrationTable.a2d2 = num;
                    } else if (i == 4) {
                        vars->HKLDCalibrationTable.weight2 = val;
                    } else if (i == 5) {
                        vars->HKLDCalibrationTable.threshold = val;
                    } else if (i == 6) {
                        vars->HKLDCalibrationTable.drillStringWeight = val;
                    } else {
                        ok = FAIL;
                    }
                }
                if (ok) {
                    DBGF(1, "+++ Set Hookload Calibration: %05d:%05.1f:%05d:%05.1f:%05.1f:%05.1f",
                         vars->HKLDCalibrationTable.a2d1,
                         vars->HKLDCalibrationTable.weight1,
                         vars->HKLDCalibrationTable.a2d2,
                         vars->HKLDCalibrationTable.weight2,
                         vars->HKLDCalibrationTable.threshold,
                         vars->HKLDCalibrationTable.drillStringWeight);
                    need_to_save_cfg = YES;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#HSC]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@DEC]") == 0) {
            // Depth Encoder Calibration - siu.encoderCalibration(count, height)
            snprintf(outbuf, sizeof(outbuf), "[#DEC]");
            char buf[32];
            int ch = ':';
            strlcpy(outbuf, "[#DEC:", sizeof(outbuf));
            for (int i = 0; i < 32; i++) {
                if (i == 31) {
                    ch = '\0';
                }
                snprintf(buf, sizeof(buf), "%s%05d:%s%06.2f%c",
                         (vars->DepthCalibrationTable.points[i].count >= 0 ? "+" : ""),
                         vars->DepthCalibrationTable.points[i].count,
                         (vars->DepthCalibrationTable.points[i].value >= 0 ? "+" : ""),
                         vars->DepthCalibrationTable.points[i].value,
                         ch);
                strlcat(outbuf, buf, sizeof(outbuf));
            }
            strlcat(outbuf, "]", sizeof(outbuf));
            replace_dot_to_comma(cfg()->decimal, outbuf);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@DEC:", 6) == 0) {
            // Depth Encoder Calibration - siu.encoderCalibration.Add(count, height)
            fields = siu_get_fields(inbuf, &nflds);
            DBGF(3, "nflds: %d %s", nflds, (nflds != 65 ? "(ALERT! expected 65 fields!)" : ""));
            if (nflds == 1 + 32 + 32) {
                char bf[32];
                int pos = 0;
                int idx = 0;
                vars->DepthCalibrationTable.size = 0;
                for (int i = 1; i < nflds; i++) {
                    snprintf(bf, sizeof(bf), "%s", fields[i]);
                    replace_comma_to_dot(bf);

                    errno = 0;
                    if (pos == 0) {
                        pos = 1;
                        num = strtol(bf, NULL, 10);
                        if (errno != 0) {
                            num = 0;
                        }
                        vars->DepthCalibrationTable.points[idx].count = num;
                    } else {
                        pos = 0;
                        val = strtof(bf, NULL);
                        if (errno != 0) {
                            val = 0.0f;
                        }
                        vars->DepthCalibrationTable.points[idx].value = val;
                        idx++;
                        vars->DepthCalibrationTable.size++;
                    }
                }
                if (debug_level >= 3) {
                    char ch = ':';
                    DBGNoNL(3, "+++ set Depth Encoder Calibration: ");
                    for (int i = 0; i < 32; i++) {
                        if (i == 31) {
                            ch = '\0';
                        }
                        printf("%d:%0.2f%c", vars->DepthCalibrationTable.points[i].count, vars->DepthCalibrationTable.points[i].value, ch);
                    }
                    puts("");
                }
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#DEC]");
            sendout(outbuf, send_response, sock);
            continue;
        }

       if (strcasecmp(inbuf, "[@DSD]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#DSD:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%d:%0.2f]",
                     vars->DepthSystemData.BitDepth,
                     vars->DepthSystemData.HoleDepth,
                     vars->DepthSystemData.BlockPosition,
                     vars->DepthSystemData.BlockSpeed,
                     vars->DepthSystemData.Rop1s,
                     vars->DepthSystemData.Rop5s,
                     vars->DepthSystemData.WeightOnBit,
                     vars->DepthSystemData.Hookload,
                     vars->DepthSystemData.DrillState,
                     vars->DepthSystemData.KellyDownDepth);
            replace_dot_to_comma(cfg()->decimal, outbuf);
           sendout(outbuf, send_response, sock);
           continue;
       }

        if (strncasecmp(inbuf, "[@DSD:", 6) == 0) {
            bool ok = OK;
            int nvalue;
            float values[10];
            fields = siu_get_fields(inbuf, &nflds);
            DBGF(3, "nflds: %d %s", nflds, (nflds != 65 ? "(ALERT! expected 11 fields!)" : ""));
            if (nflds == 11) {
                int idx = 0;
                char bf[32];
                for (int i = 1; i < nflds; i++) {
                    snprintf(bf, sizeof(bf), "%s", fields[i]);
                    replace_comma_to_dot(bf);

                    errno = 0;
                    if (idx == 8) { // special case
                        num = strtol(bf, NULL, 10);
                        nvalue = num;
                    } else {
                        val = strtof(bf, NULL);
                        values[idx] = val;
                    }
                    if (errno != 0) {
                        ok = FAIL;
                        break;
                    }
                    idx++;
                }

                if (ok) {
                    vars->DepthSystemData.BitDepth = values[0];
                    vars->DepthSystemData.HoleDepth = values[1];
                    vars->DepthSystemData.BlockPosition = values[2];
                    vars->DepthSystemData.BlockSpeed = values[3];
                    vars->DepthSystemData.Rop1s = values[4];
                    vars->DepthSystemData.Rop5s = values[5];
                    vars->DepthSystemData.WeightOnBit = values[6];
                    vars->DepthSystemData.Hookload = values[7];
                    vars->DepthSystemData.DrillState = nvalue;
                    vars->DepthSystemData.KellyDownDepth = values[9];
                    DBGF(1, "+++ Set Depth Data: %0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%d:%0.2f",
                         vars->DepthSystemData.BitDepth,
                         vars->DepthSystemData.HoleDepth,
                         vars->DepthSystemData.BlockPosition,
                         vars->DepthSystemData.BlockSpeed,
                         vars->DepthSystemData.Rop1s,
                         vars->DepthSystemData.Rop5s,
                         vars->DepthSystemData.WeightOnBit,
                         vars->DepthSystemData.Hookload,
                         vars->DepthSystemData.DrillState,
                         vars->DepthSystemData.KellyDownDepth);
                    need_to_save_cfg = YES;
                }
            }
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@DSDE]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#DSDE:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f]",
                     vars->DSDE.WobAvg,
                     vars->DSDE.HkldAvg,
                     vars->DSDE.Torque,
                     vars->DSDE.RPM,
                     vars->DSDE.PumpStrokeRate1,
                     vars->DSDE.PumpStrokeRate2,
                     vars->DSDE.PumpStrokeRate3,
                     vars->DSDE.MudFlowInAvg,
                     vars->DSDE.MudFlowOutAvg,
                     vars->DSDE.DifferentialPressure);
            replace_dot_to_comma(cfg()->decimal, outbuf);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@DSDE:", 7) == 0) {
            bool ok = OK;
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 11) {
                int idx = 0;
                char bf[32];
                float values[10];
                for (int i = 1; i < nflds; i++) {
                    snprintf(bf, sizeof(bf), "%s", fields[i]);
                    replace_comma_to_dot(bf);

                    errno = 0;
                    val = strtof(bf, NULL);
                    if (errno != 0) {
                        ok = FAIL;
                        break;
                    }
                    values[idx] = val;
                    idx++;
                }
                if (ok) {
                    vars->DSDE.WobAvg = values[0];
                    vars->DSDE.HkldAvg = values[1];
                    vars->DSDE.Torque = values[2];
                    vars->DSDE.RPM = values[3];
                    vars->DSDE.PumpStrokeRate1 = values[4];
                    vars->DSDE.PumpStrokeRate2 = values[5];
                    vars->DSDE.PumpStrokeRate3 = values[6];
                    vars->DSDE.MudFlowInAvg = values[7];
                    vars->DSDE.MudFlowOutAvg = values[8];
                    vars->DSDE.DifferentialPressure = values[9];
                    DBGF(1, "+++ Set Depth Data Ext: %0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f",
                         vars->DSDE.WobAvg,
                         vars->DSDE.HkldAvg,
                         vars->DSDE.Torque,
                         vars->DSDE.RPM,
                         vars->DSDE.PumpStrokeRate1,
                         vars->DSDE.PumpStrokeRate2,
                         vars->DSDE.PumpStrokeRate3,
                         vars->DSDE.MudFlowInAvg,
                         vars->DSDE.MudFlowOutAvg,
                         vars->DSDE.DifferentialPressure);
                    need_to_save_cfg = YES;
                }
            }
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@SPS]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#SPS:%06d]", vars->SPS);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@SPS:", 6) == 0) {
            // [@SPS:012000]
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                num = (int) strtoll(fields[1], NULL, 10);
                if (errno == 0) {
                    vars->SPS = num;
                    DBGF(1, "+++ Set Pressure Scaling: %d", vars->SPS);
                    need_to_save_cfg = YES;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#SPS]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@STS]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#STS:%04X:%04X]", siu_dev_status(), siu_system_status());
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@STS:", 6) == 0) {
            // [@STS:00:00]
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 3) {
                int led_status = (int) strtoll(fields[1], NULL, 10);
                if (led_status != -1) {
                    siu_update_dev_status(led_status);
                }
                int sys_status = (int) strtoll(fields[2], NULL, 10);
                if (sys_status != -1) {
                    siu_update_system_status_flags(sys_status);
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#STS]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@SPP]") == 0) {   // no obj in response
            snprintf(outbuf, sizeof(outbuf), "[#SPP:%02d:%02d]",
                     vars->SPP[0], vars->SPP[1]);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@SPP:", 6) == 0) {
            // [@SPP:00] [@SPP:01:00000] [@SPP:02:00000]
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                errno = 0;
                num = strtol(fields[1], NULL, 10);
                if (errno == 0) {
                    vars->SPP[0] = num;
                    vars->SPP[1] = num;
                }
            } else if (nflds == 3) {
                errno = 0;
                num = strtol(fields[1], NULL, 10);
                if (errno == 0) {
                    errno = 0;
                    int nval = strtol(fields[2], NULL, 10);
                    if (errno == 0) {
                        if (num == 1) {
                            vars->SPP[0] = nval;
                            DBGF(1, "+++ Set Standpipe Pressure Calibration 01:%06d", vars->SPP[0]);
                        } else if (num == 2) {
                            vars->SPP[1] = nval;
                            DBGF(1, "+++ Set Standpipe Pressure Calibration 02:%06d", vars->SPP[1]);
                        }
                        need_to_save_cfg = YES;
                    }
                }
            }
            sendout(outbuf, send_response, sock);
            continue;
        }

       if (strcasecmp(inbuf, "[@POT]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#POT:%05d]", vars->POT);
           sendout(outbuf, send_response, sock);
           continue;
       }

        if (strncasecmp(inbuf, "[@POT:", 6) == 0) {
            //[@POT:00000]
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                errno = 0;
                num = strtol(fields[1], NULL, 10);
                if (errno == 0) {
                    vars->POT = num;
                    DBGF(1, "+++ Set Pumps On Pressure Threshold %d", vars->POT);
                    need_to_save_cfg = YES;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#POT]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@DER]") == 0) {
            // Depth Encoder Counts
            snprintf(outbuf, sizeof(outbuf), "[#DER:%+06d]", vars->DER);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@DER:", 6) == 0) {
            // Depth Encoder Counts
            //[@DER:+00002]
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                errno = 0;
                snum = (short) strtol(fields[1], NULL, 10);
                if (errno == 0) {
                    vars->DER = snum;
                    DBGF(1, "+++ Set Depth Encoder Counts %d", vars->DER);
                    esz7_set_encoder_position(ENCODER_BLOCKPOS, vars->DER);
                    need_to_save_cfg = YES;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#DER]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@HSR]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#HSR:%05d]", vars->HookloadA2D);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@HSC]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#HSC:%05d:%05.1f:%05d:%05.1f:%05.1f:%05.1f]",
                     vars->HKLDCalibrationTable.a2d1,
                     vars->HKLDCalibrationTable.weight1,
                     vars->HKLDCalibrationTable.a2d2,
                     vars->HKLDCalibrationTable.weight2,
                     vars->HKLDCalibrationTable.threshold,
                     vars->HKLDCalibrationTable.drillStringWeight);
            replace_dot_to_comma(cfg()->decimal, outbuf);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@PSS]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#PSS:%s]", (vars->PSS ? "ON" : "OFF"));
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@PSS:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                vars->PSS = (strcmp(fields[1], "ON") == 0);
                DBGF(1, "+++ Set Pump Status: %s", (vars->PSS ? "ON" : "OFF"));
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#PSS]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@SDU]") == 0) {   // no obj in response
            snprintf(outbuf, sizeof(outbuf), "[#SDU:%s]  (%s)",
                     (vars->SDU == UnitsEnglish ? "E" : "M"),
                     (vars->SDU == UnitsEnglish ? "English" : "Metric"));
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@SDU:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                if (fields[1][0] == 'M') {
                    vars->SDU = UnitsMetric;
                } else if (fields[1][0] == 'E') {
                    vars->SDU = UnitsEnglish;
                }
                DBGF(1, "+++ Set System Metrics '%s'", (vars->SDU == UnitsMetric ? "Metric" : "English"));
                // cfg()->save(); // TODO ?
            }
            snprintf(outbuf, sizeof(outbuf), "[#SDU]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@LOG]") == 0) {   // no obj in response
            snprintf(outbuf, sizeof(outbuf), "[#LOG:%s]", (vars->LOG ? "ON" : "OFF"));
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@LOG:", 6) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                vars->LOG = (strcmp(fields[1], "ON") == 0);
                DBGF(1, "+++ Set Log '%s'", (vars->LOG ? "ON" : "OFF"));
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#LOG]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        //===========================================================

        if (strcasecmp(inbuf, "[@WDEPTHUNITS]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#WDEPTHUNITS:%s]",
                     (vars->WitsDepthUnits == UnitsEnglish ? "FEET" : "METERS"));
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@WDEPTHUNITS:", 14) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                vars->WitsDepthUnits = (strcmp(fields[1], "FEET") == 0 ? UnitsMetric : UnitsEnglish);
            }
            snprintf(outbuf, sizeof(outbuf), "[#WDEPTHUNITS]");

            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@WFLOWUNITS]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#WFLOWUNITS:%s]",
                     (vars->WitsFlowUnits == UnitsEnglish ? "GPM" : "LPM"));
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@WFLOWUNITS:", 14) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                vars->WitsFlowUnits = (strcmp(fields[1], "GPM") == 0 ? UnitsMetric : UnitsEnglish);
            }
            snprintf(outbuf, sizeof(outbuf), "[#WFLOWUNITS]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@WHKLDUNITS]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#WHKLDUNITS:%s]",
                     (vars->WitsHkldUnits == UnitsEnglish ? "KDN" : "KLB"));
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@WHKLDUNITS:", 14) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                vars->WitsHkldUnits = (strcmp(fields[1], "KLB") == 0 ? UnitsMetric : UnitsEnglish);
            }
            snprintf(outbuf, sizeof(outbuf), "[#WHKLDUNITS]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@WTORQUNITS]") == 0) {
            const char *str = "NM";
            if (vars->WitsTorqueUnits == TORQUE_KFTLB) {
                str = "KFLB";
            } else if (vars->WitsTorqueUnits == TORQUE_KNM) {
                str = "KNM";
            } else if (vars->WitsTorqueUnits == TORQUE_FTLB) {
                str = "FLB";
            }
            snprintf(outbuf, sizeof(outbuf), "[#WTORQUNITS:%s]", str);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@WTORQUNITS:", 13) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                if (strcmp(fields[1], "FLB") == 0) {
                    vars->WitsTorqueUnits = TORQUE_FTLB;
                } else if (strcmp(fields[1], "TORQUE_KNM") == 0) {
                    vars->WitsTorqueUnits = TORQUE_KNM;
                } else if (strcmp(fields[1], "KFLB") == 0) {
                    vars->WitsTorqueUnits = TORQUE_KFTLB;
                } else {
                    vars->WitsTorqueUnits = TORQUE_NM;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#WTORQUNITS]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@WPRESUNITS]") == 0) {
            const char *str = "KGCM";
            if (vars->PressureUnits == PRESSURE_BAR) {
                str = "BAR";
            } else if (vars->PressureUnits == PRESSURE_ATM) {
                str = "ATM";
            } else if (vars->PressureUnits == PRESSURE_KPA) {
                str = "KPA";
            } else if (vars->PressureUnits == PRESSURE_PSI) {
                str = "PSI";
            }
            snprintf(outbuf, sizeof(outbuf), "[#WPRESUNITS:%s]", str);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@WPRESUNITS:", 13) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                if (strcmp(fields[1], "BAR") == 0) {
                    vars->PressureUnits = PRESSURE_BAR;
                } else if (strcmp(fields[1], "ATM") == 0) {
                    vars->PressureUnits = PRESSURE_ATM;
                } else if (strcmp(fields[1], "KPA") == 0) {
                    vars->PressureUnits = PRESSURE_KPA;
                } else if (strcmp(fields[1], "PSI") == 0) {
                    vars->PressureUnits = PRESSURE_PSI;
                } else {
                    vars->PressureUnits = PRESSURE_KGCM2;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#WPRESUNITS]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@BKLGHT]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#BKLGHT:%d]", vars->backlight);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@BKLGHT:", 9) == 0) {
            // [@BKLGHT:1]
            char *p = strrchr(inbuf, ']');
            if (p) {
                *p = '\0';
            }
            vars->backlight = (int) strtol((inbuf + 9), NULL, 10);
            snprintf(outbuf, sizeof(outbuf), "[#BKLGHT]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@RIGLAN]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#RIGLAN:%s:%s:%s:%s]",
                     (vars->riglan.NetRigIpSource == DHCP ? "DHCP" : "FIXED"),
                     vars->riglan.NetRigIp,
                     vars->riglan.NetRigNetmask,
                     vars->riglan.NetRigGateway);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@RIGLAN:", 9) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 5) {
                vars->riglan.NetRigIpSource = (strcmp(fields[1], "DHCP") == 0 ? DHCP : FIXED);
                strncpy(vars->riglan.NetRigIp, fields[2], sizeof(vars->riglan.NetRigIp) - 1);
                strncpy(vars->riglan.NetRigNetmask, fields[3], sizeof(vars->riglan.NetRigNetmask) - 1);
                strncpy(vars->riglan.NetRigGateway, fields[4], sizeof(vars->riglan.NetRigGateway) - 1);
                DBGF(1, "+++ Set RIGLAN: %s:%s:%s:%s",
                     (vars->riglan.NetRigIpSource == DHCP ? "DHCP" : "FIXED"),
                     vars->riglan.NetRigIp,
                     vars->riglan.NetRigNetmask,
                     vars->riglan.NetRigGateway);
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#RIGLAN]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@APSLAN]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#APSLAN:%s:%s:%s:%s]",
                     (vars->apslan.NetApsIpSource == DHCP ? "DHCP" : "FIXED"),
                     vars->apslan.NetApsIp,
                     vars->apslan.NetApsNetmask,
                     vars->apslan.NetApsGateway);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@APSLAN:", 9) == 0) {
            // "[@APSLAN:{0}:{1}:{2}:{3}]"
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 5) {
                vars->apslan.NetApsIpSource = (strcmp(fields[1], "DHCP") == 0 ? DHCP : FIXED);
                strncpy(vars->apslan.NetApsIp, fields[2], sizeof(vars->apslan.NetApsIp) - 1);
                strncpy(vars->apslan.NetApsNetmask, fields[3], sizeof(vars->apslan.NetApsNetmask) - 1);
                strncpy(vars->apslan.NetApsGateway, fields[4], sizeof(vars->apslan.NetApsGateway) - 1);
                DBGF(1, "+++ Set APSLAN: %s:%s:%s:%s",
                     (vars->apslan.NetApsIpSource == DHCP ? "DHCP" : "FIXED"),
                     vars->apslan.NetApsIp,
                     vars->apslan.NetApsNetmask,
                     vars->apslan.NetApsGateway);
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#APSLAN]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@TPAMPS:", 9) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                vars->toolPowerCurrent = strtof(fields[1], NULL);
            }
            snprintf(outbuf, sizeof(outbuf), "[#TPAMPS]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@TPAMPS]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#TPAMPS:%0.2f]", vars->toolPowerCurrent);
            replace_dot_to_comma(cfg()->decimal, outbuf);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@TPVOLTS:", 10) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                vars->toolPowerVoltage = strtof(fields[1], NULL);
            }
            snprintf(outbuf, sizeof(outbuf), "[#TPVOLTS]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@TPVOLTS]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#TPVOLTS:%0.2f]", vars->toolPowerVoltage);
            replace_dot_to_comma(cfg()->decimal, outbuf);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@WIFICONF:", 11) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 4) {
                strncpy(vars->wifi.WifiSsid, fields[1], sizeof(vars->wifi.WifiSsid) - 1);
                vars->wifi.WifiSsidHidden = (strcmp(fields[2], "HIDDEN") == 0 ? 1 : 0);
                if (strcmp(fields[3], "WEP-OPEN") == 0) {
                    vars->wifi.WifiEncryptionType = ENC_WEPOPEN;
                } else if (strcmp(fields[3], "WEP-SHARED") == 0) {
                    vars->wifi.WifiEncryptionType = ENC_WEPSHARED;
                } else if (strcmp(fields[3], "WPA-AES") == 0) {
                    vars->wifi.WifiEncryptionType = ENC_WPAAES;
                } else if (strcmp(fields[3], "WPA-TKIP") == 0) {
                    vars->wifi.WifiEncryptionType = ENC_WPATKIP;
                } else {
                    vars->wifi.WifiEncryptionType = ENC_NONE;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#WIFICONF]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@WIFICONF]") == 0) {
            const char *str = "NONE";
            if (vars->wifi.WifiEncryptionType == ENC_WEPOPEN) {
                str = "WEP-OPEN";
            } else if (vars->wifi.WifiEncryptionType == ENC_WEPSHARED) {
                str = "WEP-SHARED";
            } else if (vars->wifi.WifiEncryptionType == ENC_WPAAES) {
                str = "WPA-AES";
            } else if (vars->wifi.WifiEncryptionType == ENC_WPATKIP) {
                str = "WPA-TKIP";
            }
            snprintf(outbuf, sizeof(outbuf), "[#WIFICONF:%s:%s:%s]",
                     vars->wifi.WifiSsid,
                     (vars->wifi.WifiSsidHidden == 1 ? "HIDDEN" : "NORMAL"),
                     str);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@WIFIKEY]") == 0) {  // no obj in response
            snprintf(outbuf, sizeof(outbuf), "[#WIFIKEY]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@WIFILAN]") == 0) {  // no obj in response
            snprintf(outbuf, sizeof(outbuf), "[#WIFILAN]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@WITSBAU]") == 0) {  // no obj in response
            snprintf(outbuf, sizeof(outbuf), "[#WITSBAU]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@WITSBITS]") == 0) {  // no obj in response
            snprintf(outbuf, sizeof(outbuf), "[#WITSBITS]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@WITSOUT]") == 0) {  // no obj in response
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                DBGF(3, "WITSOUT: %s", fields[1]);
            }
            snprintf(outbuf, sizeof(outbuf), "[#WITSOUT]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@WITSPAR]") == 0) {  // no obj in response
            snprintf(outbuf, sizeof(outbuf), "[#WITSPAR]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@WITSPORT]") == 0) {  // no obj in response
            snprintf(outbuf, sizeof(outbuf), "[#WITSPORT]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@WITSSTOP]") == 0) {  // no obj in response
            snprintf(outbuf, sizeof(outbuf), "[#WITSSTOP]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@WITSRAW]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#WITSRAW:%s]", (vars->WitsRaw ? "ON" : "OFF"));
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@WITSRAW:", 10) == 0) {
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                vars->WitsRaw = (strcmp(fields[1], "ON") == 0);
                DBGF(1, "+++ Set WitsRaw %s", (vars->WitsRaw ? "ON" : "OFF"));
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#WITSRAW]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@WITSDPCODE]") == 0) {
            if (!vars->WitsDiffPressCode.enabled) {
                snprintf(outbuf, sizeof(outbuf), "[#WITSDPCODE:-1]");
            } else {
                snprintf(outbuf, sizeof(outbuf), "[#WITSDPCODE:ON:%02d:%02d]",
                         vars->WitsDiffPressCode.record,
                         vars->WitsDiffPressCode.item);
            }
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@WITSDPCODE:", 13) == 0) {
            // [@WITSDPCODE:{0}]
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                // TODO // ! WitsRaw - не опечатка, так в исходниках
                vars->WitsDiffPressCode.enabled = (strcmp(fields[1], "ON") == 0);
                DBGF(1, "+++ Set WitsDiffPressCode.enabled %s", (vars->WitsDiffPressCode.enabled ? "ON" : "OFF"));
                need_to_save_cfg = YES;
            }
            snprintf(outbuf, sizeof(outbuf), "[#WITSDPCODE]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@WITSOUT:", 10) == 0) {
            // [@WITSOUT:~<~{WitsMessage}~>~]
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                vars->WitsOutput = (strcmp(fields[1], "ON") == 0);
                DBGF(1, "+++ Set WitsOutput to %s", (vars->WitsOutput ? "ON" : "OFF"));
                need_to_save_cfg = YES;
            } else {
                DBGF(1, "+++ Set WitsOutput error - %s", strerror(errno));
            }
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@NRRLOOP]") == 0) {
            // [@NRRLOOP]
            snprintf(outbuf, sizeof(outbuf), "[#NRRLOOP:%02d]", vars->mpt.MptNRot2Rot);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@NRRLOOP:", 10) == 0) {
            // [@NRRLOOP:02]
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                errno = 0;
                num = strtol(fields[1], NULL, 10);
                if (errno == 0) {
                    vars->mpt.MptNRot2Rot = num;
                    DBGF(1, "+++ Set Loopback - Non-Rotating to Rotating Transistion Index: %u",
                         vars->mpt.MptNRot2Rot);
                    need_to_save_cfg = YES;
                } else {
                    DBGF(1, "+++ Set Loopback error - %s", strerror(errno));
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#NRRLOOP]");
            need_to_save_cfg = true;

            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@RNRLOOP]") == 0) {
            // [@RNRLOOP]
            snprintf(outbuf, sizeof(outbuf), "[#RNRLOOP:%02d]", vars->mpt.MptRot2NRot);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@RNRLOOP:", 10) == 0) {
            // [@RNRLOOP:02]
            snprintf(outbuf, sizeof(outbuf), "[#RNRLOOP:%02d]", vars->mpt.MptRot2NRot);
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds == 2) {
                errno = 0;
                num = strtol(fields[1], NULL, 10);
                if (errno == 0) {
                    vars->mpt.MptRot2NRot = num;
                    DBGF(1, "+++ Set Loopback - Rotating to Non-Rotating Transistion Index: %u",
                         vars->mpt.MptRot2NRot);
                    need_to_save_cfg = YES;
                } else {
                    DBGF(1, "+++ Set Loopback error - %s", strerror(errno));
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#RNRLOOP]");
            need_to_save_cfg = true;
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strncasecmp(inbuf, "[@FMT:", 6) == 0) {
            parse_FormatTable(inbuf, outbuf, sizeof(outbuf));
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@DCN]") == 0) {
            struct timespec ts;
            struct tm tm = clock_get_local_time(&ts);
            snprintf(outbuf, sizeof(outbuf), "[#DCN:%02d%02d%04d:%02d%02d%02d:%s%02ld.%02ld]",
                     tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
                     tm.tm_hour, tm.tm_min, tm.tm_sec,
                     (tm.tm_gmtoff < 0 ? "-" : "+"),
                     tm.tm_gmtoff / 3600, (tm.tm_gmtoff % 3600) / 60);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@THM]") == 0) {   // no obj in response
            snprintf(outbuf, sizeof(outbuf), "[#THM]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@DPT]") == 0) {   // no obj in response
            snprintf(outbuf, sizeof(outbuf), "[#DPT]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@THR]") == 0) {   // no obj in response
            snprintf(outbuf, sizeof(outbuf), "[#THR]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@FCO]") == 0) {   // no obj in response
            snprintf(outbuf, sizeof(outbuf), "[#FCO]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@PSS]") == 0) {   // no obj in response
            snprintf(outbuf, sizeof(outbuf), "[#PSS]");
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@POT]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#POT]"); // no obj in response
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@DSP]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#DSP:%s]", vars->version_dsp_firmware);
            sendout(outbuf, send_response, sock);
            continue;
        }

        if (strcasecmp(inbuf, "[@VER]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#VER:%s]", vars->version_firmware);
            sendout(outbuf, send_response, sock);
            continue;
        }

         {
            DBGF(1, "========== unsupported item %s ==========", inbuf);
            snprintf(outbuf, sizeof(outbuf), "[#%s]", inbuf);
            fields = siu_get_fields(inbuf, &nflds);
            if (nflds > 0) {
                snprintf(outbuf, sizeof(outbuf), "[%s]", fields[0]);
            } else {
                strlcpy(outbuf, inbuf, sizeof(outbuf));
            }
            char *ptr = strchr(outbuf, '@');
            if (ptr) {
                memmove(outbuf, ptr, strlen(ptr) + 1);
            }
            ptr = strrchr(inbuf, ']');
            if (ptr) {
                *ptr = '\0';
            }
             sendout(outbuf, send_response, sock);
        }
    } // while(1)
}

//---------------------------------------------------------
const char *siu_cmd_brakets_comment(const char *str)
{
    if (strncasecmp(str, "[@DBT", 5) == 0) {
        return "Bit Depth";
    }
    if (strncasecmp(str, "[@DEC", 5) == 0) {
        return "Depth Encoder calibration";
    }
    if (strncasecmp(str, "[@DER", 5) == 0) {
        return "Depth Encoder Counts / Энкодер глубины";
    }
    if (strncasecmp(str, "[@DFS", 5) == 0) {
        return "Force Drill State";
    }
    if (strncasecmp(str, "[@DIE", 5) == 0) {
        return "Depth Internal/External";
    }
    if (strncasecmp(str, "[@DMY", 5) == 0) {
        return "Date Month Year";
    }
    if (strncasecmp(str, "[@DME", 5) == 0) {
        return "Hole Depth / Глубина скажины";
    }
    else if (strncasecmp(str, "[@DMY", 5) == 0) {
        return "Date Month Year";
    }
    if (strncasecmp(str, "[@DSS", 5) == 0) {
        return "Depth Tracking";
    }
    if (strncasecmp(str, "[@DSD", 5) == 0) {
        return "Depth Data / Данные по глубине";
    }
    if (strncasecmp(str, "[@DSDE", 6) == 0) {
        return "Depth Data Ext / Доп. данные по глубине";
    }
    if (strncasecmp(str, "[@HSC", 5) == 0) {
        return "Hookload calibration";
    }
    if (strncasecmp(str, "[@HSR", 5) == 0) {
        return "Hookload Counts";
    }
    if (strncasecmp(str, "[@KDD", 5) == 0) {
        return "Kelly Down Depth / Длина колонны";
    }
    if (strncasecmp(str, "[@KLD", 5) == 0) {
        return "Block Position / Позиция блока";
    }
    if (strncasecmp(str, "[@LEDTEST", 9) == 0) {
        return "Индикация на устройстве";
    }
    if (strncasecmp(str, "[@POT", 5) == 0) {
        return "Pumps On Pressure Threshold";
    }
    if (strncasecmp(str, "[@OBT", 5) == 0) {
        return "OnBottom Threshold / Допуск на вес внизу";
    }
    if (strncasecmp(str, "[@RTE", 5) == 0) {
        return "MPT Rate / Скорость передачи данных";
    }
    if (strncasecmp(str, "[@SDU", 5) == 0) {
        return "System Metrics";
    }
    if (strncasecmp(str, "[@SPP", 5) == 0) {
        return "Standpipe Pressure Calibration";
    }
    if (strncasecmp(str, "[@SPS", 5) == 0) {
        return "Pressure Scaling";
    }
    if (strncasecmp(str, "[@STS", 5) == 0) {
        return "System Status";
    }
    if (strncasecmp(str, "[@TEMPDEC", 5) == 0) {
        return "Temperature Decoder";
    }
    if (strncasecmp(str, "[@TEMPIO", 5) == 0) {
        return "Temperature IO";
    }
    if (strncasecmp(str, "[@TTR", 5) == 0) {
        return "Time Tracking";
    }
    if (strncasecmp(str, "[@WDEPTHUNITS", 5) == 0) {
        return "WITS Depth Units";
    }
    if (strncasecmp(str, "[@WFLOWUNITS", 5) == 0) {
        return "WITS Flow Units";
    }
    if (strncasecmp(str, "[@WHKLDUNITS", 5) == 0) {
        return "WITS Hookload Units";
    }
    if (strncasecmp(str, "[@WTORQUNITS", 5) == 0) {
        return "WITS Torque Units";
    }
    if (strncasecmp(str, "[@WITSRAW", 5) == 0) {
        return "WITS Raw";
    }
    if (strncasecmp(str, "[@WITSDPCODE", 5) == 0) {
        return "WITS Differenitial Pressure Code";
    }

    return "";
}
