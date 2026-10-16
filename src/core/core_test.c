/**
  \file   core_test.c
  \author Aleksey Lekhtin
  \date   04.05.2024
  \brief  The ESZ7 job
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>

#include "cfg.h"
#include "dbg.h"
#include "epl.h"
#include "core.h"
#include "adc.h"
#include "sram.h"
#include "thr.h"
#include "utils.h"
#include "log.h"

//---------------------------------------------------------
static void print_sram_values(const TEST_MODE test, const int channel)
{
    // channel: 1..4
    if (test == TEST1) {
        SramStat_t sram_stat;
        sram_status_get_status(&sram_stat);
        int memlen = -1;
        if (channel == 1)
            memlen = sram_stat.BufLen0;
        else if (channel == 2)
            memlen = sram_stat.BufLen1;
        else if (channel == 3)
            memlen = sram_stat.BufLen2;
        else if (channel == 4)
            memlen = sram_stat.BufLen3;

        int len = core()->npnts * (int) sizeof(AdcRec_t);
        const char *bmem1 = (const char *) sram_mem(channel)->bundle[0].recs;
        const char *bmem2 = (const char *) sram_mem(channel)->bundle[1].recs;
        const char *endmem1 = bmem1 + len;
        const char *endmem2 = bmem2 + len;

        printf("sram-%d, points: %d, dump: %d bytes\n", channel, memlen, len);

        int w = 16;
        if (cfg()->mode == MODE_ATLAS) {
            w = 64;
        }

        int nline = 1;
        int i = 0, n = 0;
        for (i = 0; i < len; i += w) {
            printf("%-2d %-4d: ", nline++, i);

            for (n = 0; n < w; n++) {
                if (n > 0 && (n % 8) == 0) {
                    putchar(' ');
                }
                printf("%02X", (unsigned char) *bmem1);
                bmem1++;
                if (bmem1 >= endmem1) {
                    fwrite(" ", 1, 1, stdout);
                    for (int j = 0; j < ((w - 1) - n); j++) {
                        if (j > 0 && (j % 8) == 0) {
                            fwrite(" ", 1, 1, stdout);
                        }
                        fwrite("  ", 1, 2, stdout);
                    }
                    break;
                }
            }
            fwrite("     ", 1, 5, stdout);

            for (n = 0; n < w; n++) {
                if (n > 0 && (n % 8) == 0) {
                    putchar(' ');
                }
                printf("%02X", (unsigned char) *bmem2);
                bmem2++;
                if (bmem2 >= endmem2) {
                    break;
                }
            }
            puts("");
        }
        printf("   %d\n", i - ((w - 1) - n));
    } else if (test == TEST2) {
        // распечатать дамп sram памяти
        fwrite("\n--------------------\n", 1, 22, stdout);
        printf("Channel: %d, points: %u\n", channel, core()->npnts);
        const int index = channel - 1;
        const int len = core()->npnts * (int) sizeof(core()->data.values[0].R[0]);
        const char *mem = (const char *) core()->data.values[index].R;
        const char *end = mem + len;
        int nline = 1;
        int i = 0, n = 0;
        bool x = false;
        bool stop = false;
        while (!stop) {
            printf("%-2d %-4d: ", nline++, i);
            for (n = 0; n < 32; n++) {
                if (n > 0 && (n % 4) == 0) {
                    fwrite(" ", 1, 1, stdout);
                }
                x = !x;
                if (x) {
                    ; // fwrite(" ", 1, 1, stdout);
                }
                printf("%02X", (unsigned char) *mem);
                mem++;
                i++;
                if (mem >= end) {
                    stop = true;
                    break;
                }
            }
            fwrite("\n", 1, 1, stdout);
            x = 0;
        }
        putchar('\n');
        printf("Encoder-1:   %5d\n", core()->data.encoder1);
        printf("Encoder-2:   %5d\n", core()->data.encoder2);
        if (channel == 2) {
            printf("HookloadA2D: %5d / %-5d\n", core()->data.HKLD24bit, core()->data.HKLD16bit);
        }
        putchar('\n');
    } else if (test == TEST3) {
        // распечатать среднее значение данных
        fwrite("\n--------------------\n", 1, 22, stdout);
        for (int i = 0; i < 4; i++) {
            printf("Channel: %d, points: %u, ", i + 1, core()->npnts);
            printf("  sum: 0x%016llX (%16lld),  avrg: 0x%08X (%10d)\n",
                   core()->data.values[i].sum, core()->data.values[i].sum,
                   core()->data.values[i].avrg, core()->data.values[i].avrg);
        }
        printf("Encoder-1:          %-5d\n", core()->data.encoder1);
        printf("Encoder-2:          %-5d\n", core()->data.encoder2);
        printf("HookloadA2D:        %-5d / %5d\n", core()->data.HKLD24bit, core()->data.HKLD16bit);
        fwrite("\n\n", 1, 2, stdout);
    }
}

//---------------------------------------------------------
/// channel: 1..4
static void print_sram_failures(const int channel, int bundlenum)
{
    int title_printed = 0;
    for (int i = 0; i < core()->npnts; i++) {
        const AdcRec_t rec = sram_mem(channel)->bundle[bundlenum].recs[i];
        const AdcRecStatus_t st = rec.bits.AdcStatus; // expected 0x12550607 (cmd data crc status)
        if ((be32toh(st.value) >> 8u) != 0x125506) {
            // shit!
            if (!title_printed) {
                title_printed = 1;
                printf("\n%s(): sram-%d, bundle-%d, ADC response ERROR (hex)\n", __func__, channel, bundlenum);
                printf("[dec] [hex]  64-bit response   STATUS0     status bits\n");
            }
            int short_message = 1;
            if (st.bits.STATUS0 == 0xFF || st.bits.STATUS0 == 0) {
                short_message = 0;
            }
            printf("[%3d] [%03X]  %016llX ", i, i, be64toh(rec.value));
            print_AdcStatus(channel, st.bits.STATUS0, short_message);
        }
    }
}

//---------------------------------------------------------
static void event_test_processing(int channel)
{
    // channel: 1..4
    const int ch = channel - 1;

    static int wrong_bundle[4] = {0};
    static int bundle_num;

    bundle_num = sram_get_ready_bundle_num(channel);
    DBGF(3, "processing channel: %d, bundle: %d", channel, bundle_num);
    if (bundle_num == -1) {
        if (wrong_bundle[ch] == 0) {
            wrong_bundle[ch] = 1;
            LOG(CRITICAL, "wrong bundle number: bundle: %d", bundle_num);
        }
        DBGF(0, "ERROR: wrong bundle number: bundle: %d\n", bundle_num);
        return;
    }
    wrong_bundle[ch] = 0;

    if (cfg()->do_dump) {
        sram_dump_create();
    }

    get_data_from_sram(channel, bundle_num);
    sram_clear_bundle_ready(channel, bundle_num);
    get_encoders_data();
    get_hookload();

    print_sram_values(cfg()->test_mode, channel);
    if (cfg()->test_mode == TEST1) {
        print_sram_failures(channel, bundle_num);
    }
}

//---------------------------------------------------------
static void core_test_epoll()
{
    static int channel;
    struct epoll_event events[EVENTS_EXPECTED] = {0};

    puts("\n\n\ntype Ctlr+C to cancel\n");
    //printf("%s\n", "\033[2J\033[H"); // ESC
    //fflush(stdout); // clear terminal

    struct timespec ts_current, ts_prev, ts_diff;
    clock_gettime(CLOCK_REALTIME, &ts_prev);

    do {
        // 1.
        const int nEvents = epoll_wait(epl()->fd, events, EVENTS_EXPECTED, -1);
        clock_gettime(CLOCK_REALTIME, &ts_current);
        if (nEvents == -1) {
            LOG(ERROR, "epoll_wait() returns -1, errno:%d (%s)", errno, strerror(errno));
            DBGF(0, "\nERROR: epoll_wait() returns -1, errno:%d (%s)\n\n\n", errno, strerror(errno));
            break;
        }

        // 2.
        timespec_diff(&ts_diff, &ts_prev, &ts_current, NULL);
        ts_prev = ts_current;

        // 3.
        //printf("\033[1;1H"); // Move cursor to row 1, column 1 // ESC
        DBGF(3, "[%ld.%06ld] events:  %d, ",
             ts_diff.tv_sec, ts_diff.tv_nsec / 100000,
             nEvents);

        // 4.
        for (int i = 0; i < nEvents; i++) {
            channel = epl_find_event_file(events[i].data.fd);
            if (channel == -1) {
                DBGF(3, "(0x%02X %s) ", events[i].events, epl_event_name(&events[i]));
                epl_read_event_file(events[i].data.fd); // read event to reset him activation
                continue;
            }
            DBGF(3, "ch:%d (0x%02X %s)  ", channel, events[i].events, epl_event_name(&events[i]));
        }
        DBGNL(3);

        // 5.
        for (int i = 0; i < nEvents; i++) {
            channel = epl_find_event_file(events[i].data.fd);
            if (channel == -1) {
                epl_read_event_file(events[i].data.fd); // read event to reset him activation
                continue;
            }

            if (events[i].events & EPOLLPRI) {
                if (cfg()->test_channel == 0 || channel == cfg()->test_channel) {
                    event_test_processing(channel);
                }
            } else if (events[i].events & EPOLLERR) {
                DBGF(3, "[ev:%d] epoll event got error\n", i);
            }

            epl_read_event_file(events[i].data.fd); // read event to reset him activation
        } // for()
    } while (!core()->thr->shuttingdown);
    DBG(1, "--- core_test_epoll finished");
}

//---------------------------------------------------------
void core_test_mode()
{
    DBG(1, "-- test mode...");
    dev_start_channels();
    DevStat_t status;
    dev_read_status(&status);
    DBGF(1, "AUTOSCAN: 0x%08X - %s", status.value, print_dev_status(&status));

    core_test_epoll();

    dev_stop_channels();
    dev_read_status(&status);
    DBGF(4, "AUTOSCAN: 0x%08X - %s", status.value, print_dev_status(&status));
    sram_dump_close();

    DBGF(3, "--- core_test_mode finished (thr:%p)", core()->thr);
}
