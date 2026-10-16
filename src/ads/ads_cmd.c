/**
  \file   ads_cmd.c
  \author Aleksey Lekhtin
  \date   Feb 26, 2025
  \brief  The ADS command interface
*/

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ads.h"
#include "ads_socket.h"
#include "cfg.h"
#include "dbg.h"
#include "thr.h"
#include "utils.h"
#include "log.h"

//---------------------------------------------------------
/**
 * Extract command item from received command
 * @param str - command
 * @return one of followed item:
 *		UNKNOWN
 *		VER
 *		SERNUM
 *		IOINSTALLER
 *		DECINSTALLER
 *		TEMPIO
 *		TEMPDEC
 *		WIFISTS
 *		ARCHSTS
 *		LEDTEST
 *		RBT
 *		ARCHIVECLR
 *		ARCHIVENEW
 *		DMYE
 *		SPS
 *		SPS2
 */
static const char *get_item(const char **str)
{
    // [@SERNUM][@VER:1][@VER:2][@VER:3][@VER:4][@VER:5][@IOINSTALLER] ...
    static char buf[64];
    char *p = strchr(*str, ']');
    if (!p) {
        return NULL; // no more items
    }
    int len = (int) ((uintptr_t) p - (uintptr_t) (*str));
    len++; // ']'
    if (len > (int) sizeof(buf) - 1) {
        len = sizeof(buf) - 1;
    }
    memcpy(buf, *str, len);
    buf[len] = '\0';
    *str += len;
    return buf;
}

//---------------------------------------------------------
/**
 * parse string with ADS sync date
 * @param item - string like [\@DMYE:155028.36:030325]
 * @param tm -  ISO C `broken-down time' structure.
 * @param ms - extra milliseconds to add to time
 */
static void parse_DMYE(const char *item, struct tm *tm, uint32_t *ms)
{
    //[@DMYE:155028.36:030325]
    if (strncasecmp(item, "[@DMYE:", 7) != 0) {
        return;
    }
    *ms = 0;
    *tm = (struct tm){};
    char buf[256] = {0};
    char bf[32] = {0};
    strncpy(buf, item, sizeof(buf) - 1);
    strncpy(bf, item, sizeof(bf) - 1);
    bf[23] = 0;
    tm->tm_year = strtol(bf + 21, NULL, 10) + 100;
    bf[21] = 0;
    tm->tm_mon = strtol(bf + 19, NULL, 10) - 1;
    bf[19] = 0;
    tm->tm_mday = strtol(bf + 17, NULL, 10);
    bf[16] = 0;
    *ms = strtol(bf + 14, NULL, 10);
    *ms *= 10;
    bf[13] = 0;
    tm->tm_sec = strtol(bf + 11, NULL, 10);
    bf[11] = 0;
    tm->tm_min = strtol(bf + 9, NULL, 10);
    bf[9] = 0;
    tm->tm_hour = strtol(bf + 7, NULL, 10);
}

//---------------------------------------------------------
static void ads_cmd_response(const char *command, ads_t *ads, int (*send_response)(socket_t *, const char *, int),
                             socket_t *sock)
{
    static char outbuf[1024 * 2] = {0};
    ads_cmd_t *ads_cmd = &ads->cmd;

    const char *pcmd = strchr(command, '[');
    while (1) {
        const char *item = get_item(&pcmd);
        if (!item) {
            break;
        }
        DBGF(1, "%s%s", (cfg()->shortmsg == 0 ? "<<<--- request:  " : ""), item);
        outbuf[0] = 0;

        if (strncasecmp(item, "[@DMYE:", 7) == 0) {
            //[@DMYE:155028.36:030325]
            struct tm tm = {0};
            uint32_t ms = 0;
            parse_DMYE(item, &tm, &ms);
            time_t secs = mktime(&tm);
            if (secs == -1) {
                int err = errno;
                LOG(ERROR, "mktime() failed (%s)", strerror(err));
                DBGF(0, "ERROR: mktime() failed (%s)\n", strerror(err));
            } else {
                struct timespec ts = (struct timespec){
                    .tv_sec = secs,
                    .tv_nsec = (long) ms * 1000000
                };
                int ret = clock_settime(CLOCK_REALTIME, &ts);
                if (ret == -1) {
                    int e = errno;
                    LOG(ERROR, "clock_settime() failed - %s", strerror(e));
                    DBGF(0, "ERROR: clock_settime() failed - %s\n", strerror(e));
                } else {
                    const char *stime = timespec_to_dd_mm_yyyy_hh_mm_ss_nz(&ts, 6, NULL);
                    DBGF(1, "SIU time changed to ADS time - %s", stime);
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#DMYE]");
        } else if (strcasecmp(item, "[@SERNUM]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#SERNUM:%s]", ads->serno);
        } else if (strcasecmp(item, "[@VER:1]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#VER:1:%s]", ads_cmd->VerComunications);
        } else if (strcasecmp(item, "[@VER:2]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#VER:2:%s]", ads_cmd->VerSensorBoard);
        } else if (strcasecmp(item, "[@VER:3]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#VER:3:%s]", ads_cmd->VerADDriver);
        } else if (strcasecmp(item, "[@VER:4]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#VER:4:%s]", ads_cmd->VerIODriver);
        } else if (strcasecmp(item, "[@VER:5]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#VER:5:%s]", ads_cmd->VerGUI);
        } else if (strcasecmp(item, "[@IOINSTALLER]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#IOINSTALLER:%d]", ads_cmd->IOINSTALLER);
        } else if (strcasecmp(item, "[@DECINSTALLER]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#DECINSTALLER:%d]", ads_cmd->DECINSTALLER);
        } else if (strcasecmp(item, "[@LEDTEST]") == 0) {
            strncpy(outbuf, "[#LEDTEST:1]", sizeof(outbuf) - 1);
        } else if (strcasecmp(item, "[@SPS]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#SPS:%d]", ads_cmd->pressure1_scale);
        } else if (strncasecmp(item, "[@SPS:", 6) == 0) {
            //[@SPS:012000]
            const char *ptr = strchr(item, ':');
            if (ptr) {
                int num = (int) strtoll(ptr, NULL, 10);
                if (errno == 0) {
                    ads_cmd->pressure1_scale = num;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#SPS]");
        } else if (strcasecmp(item, "[@SPS2]") == 0) {
            snprintf(outbuf, sizeof(outbuf), "[#SPS2:%d]", ads_cmd->pressure2_scale);
        } else if (strncasecmp(item, "[@SPS2:", 7) == 0) {
            //[@SPS2:012000]
            const char *ptr = strchr(item, ':');
            if (ptr) {
                int num = (int) strtoll(ptr, NULL, 10);
                if (errno == 0) {
                    ads_cmd->pressure2_scale = num;
                }
            }
            snprintf(outbuf, sizeof(outbuf), "[#SPS2]");
        } else if (strcasecmp(item, "[@ARCHIVENEW]") == 0) {
            ads_new_file(1);
            ads_new_file(2);
            snprintf(outbuf, sizeof(outbuf), "[#ARCHIVENEW]");
        } else if (strcasecmp(item, "[@ARCHIVECLR]") == 0) {
            ads_reset_file(1);
            ads_reset_file(2);
            snprintf(outbuf, sizeof(outbuf), "[#ARCHIVECLR]");
        } else if (strncasecmp(item, "[@ARCHSTS:", 10) == 0) {
            int sensornum = 1;
            int filenum = 1;
            char bf[256] = {0};
            strncpy(bf, item, sizeof(bf) - 1);
            char *ptr = bf;
            while (*ptr != '\0') {
                if (*ptr == ':' || *ptr == ']') {
                    *ptr = '\0';
                }
                ptr++;
            }

            ptr = bf;
            while (*ptr != '\0') {
                ptr++;
            }
            ptr++;
            if (*ptr != '\0') {
                sensornum = strtol(ptr, NULL, 10);
            }

            while (*ptr != '\0') {
                ptr++;
            }
            ptr++;
            if (*ptr != '\0') {
                filenum = strtol(ptr, NULL, 10);
            }

            DBGF(1, "ARCHSTS::sensor:  %d", sensornum);
            DBGF(1, "ARCHSTS::filenum: %d", filenum);
#if 1
            snprintf(outbuf, sizeof(outbuf), "[#ARCHSTS:%03d:%d:NOTAVAIL:::]", sensornum, filenum);
#else
            struct timespec ts;
            if (sensornum == 1) {
                ts = ads->fileS1.created;
                filenum = ads->fileS1.number;
            } else if (sensornum == 2) {
                ts = ads->fileS2.created;
                filenum = ads->fileS1.number;
            }

            strncpy(bf, timespec_to_yymmdd_hhmmss_zzzzzz(&ts), sizeof(bf) - 1);
            printf("1::bf: %s\n", bf);
            ptr = strchr(bf, '.');
            if (ptr) {
                *ptr = '_';
            }
            printf("2::bf: %s\n", bf);
            char filename[128];
            snprintf(filename, sizeof(filename), "local_%s_%010d_%s.s%03d_400_pressure",
                     ads->serno, filenum, bf, sensornum);
            //snprintf(outbuf, sizeof(outbuf), "[#ARCHSTS:%03d:%d:NOTAVAIL:%ld:%.06ld:%s]",
            // sensornum, filenum, (long) ts.tv_sec, ts.tv_nsec / 1000, filename);
            snprintf(outbuf, sizeof(outbuf), "[#ARCHSTS:%03d:%d:NOTAVAIL:::]", sensornum, filenum);
#endif
        } else {
            // un-registered command
            DBGF(1, "\n--- WARNING! Unknown item - '%s'\n", item);
            strncpy(outbuf, item, sizeof(outbuf) - 2);
            char *p = strchr(outbuf, '@');
            if (p) {
                *p = '#';
            }
            p = strchr(outbuf, ':');
            if (p) {
                *p = ']';
                *(p + 1) = '\0';
            }
        }

        DBGF(1, "%s --->>> response: (%d bytes)", outbuf, (int)strlen(outbuf));

        if (outbuf[0]) {
#if 0
            if (ads->dump_ads_cmd_fd != -1 && ads->dump_ads_cmd_counter < 1000000) {
                static char buf[1024];
                int ln = snprintf(buf, sizeof(buf), "%s %-6d: %s - %s\n",
                                  get_date_time(), ads->dump_ads_cmd_counter, item, outbuf);
                write(ads->dump_ads_cmd_fd, buf, ln);
                ads->dump_ads_cmd_counter++;
            }
#endif
            DBGF(1, "%s%s%s", (cfg()->shortmsg == 0 ? "--->>> response: " : ""),
                 outbuf, (debug_level == 1 ? "\n": ""));
            send_response(sock, outbuf, (int) strlen(outbuf));
        }
    }
}

//---------------------------------------------------------
int ads_cmd_client_thread(thr_t *thr)
{
    client_t *client = (client_t *) thr->payload;
    ads_t *ads = (ads_t *) client->data;
    ads->cmd.cli_thr = thr;
    socket_t *cli_sock = &client->sock;
    DBGF(3, "--- %s started on socket %d, port %d",
         thr->name, cli_sock->fd, ads->data.port);

    char command[1024 * 16] = {0};
    int length = sizeof(command) - 1;
    do {
        int nread = socket_recv(cli_sock, command, length);
        if (nread <= 0) {
            break; // error or EOF(client disconnected)
        }
        command[nread] = '\0';
        DBGF(4, "got %d bytes", nread);
        ads_cmd_response(command, ads, ads_cmd_socket_send, cli_sock);
    } while (!thr->shuttingdown);
    DBGF(3, "%s - shuttingdown...", thr->name);

    ads->cmd.cli_thr = thr;
    DBGF(3, "---  finished %s:%d", basename(__FILE__), __LINE__);
    return 0;
}
