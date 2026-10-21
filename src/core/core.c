/**
  \file   core.c
  \author Aleksey Lekhtin
  \date   04.05.2024
  \brief  The ESZ7 job
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/epoll.h>

#include "ads.h"
#include "atlas.h"
#include "cfg.h"
#include "dbg.h"
#include "dtr.h"
#include "epl.h"
#include "core.h"
#include "adc.h"
#include "enc.h"
#include "sram.h"
#include "siu.h"
#include "thr.h"
#include "utils.h"
#include "aps_wits.h"
#include "log.h"
#include "wits.h"

static core_t m_core = {0};

//---------------------------------------------------------
inline core_t *core()
{
    return &m_core;
}

//---------------------------------------------------------
void core__wakeup()
{
    DBGF(8, "job online: ads: %u, siu: %u, dtr: %u, wits: %u",
         ads()->data.online,
         siu()->data.online,
         dtr()->data.online,
         aps_wits()->online);

    if (!ads()->data.online && !siu()->data.online && !dtr()->data.online && !aps_wits()->online) {
        DBG(4, "core going to be sleep");
        thr_asleep(m_core.thr);
        return;
    }
    DBG(4, "core waking up...");
    thr_wakeup(m_core.thr);
}

//---------------------------------------------------------
void toggle_points_buffer_ch(int ch)
{
    // ch: 0 - 3
    core()->data.toggle[ch] = !core()->data.toggle[ch];
    if (core()->data.toggle[ch] == 1) {
        core()->data.values[ch].W = core()->data.values[ch].pointsA;
        core()->data.values[ch].R = core()->data.values[ch].pointsB;
    } else {
        core()->data.values[ch].W = core()->data.values[ch].pointsB;
        core()->data.values[ch].R = core()->data.values[ch].pointsA;
    }
}

//---------------------------------------------------------
/// @param channel - [1..4]
static void reset_STAT12(int channel)
{
    // Detect PGA out-of-range operating conditions by polling the STAT12 bit (bit 4 of the STATUS conversion byte or
    // STATUS0 register). The STAT12 bit is the logical OR of all PGA error flags with the CRC2 error flag. When the
    // STAT12 bit asserts, poll the STATUS1 and STATUS2 registers (address 11h and 12h) to determine the source of
    // the STAT12 error. The PGA out-of-range flags latch in the STATUS1 register and remain latched after the
    // overload condition is removed. Read the STATUS1 register to clear the PGA out-of-range bits (clear-on-read
    // operation). The PGA overload flags and the CRC2 flag must be reset in order for the STAT12 bit to clear. See
    // the STATUS1 register for a description of the PGA overload bits.

    uint8_t value;
    read_adc_register(channel, RegStatus1, &value);
    bool ok = read_adc_register(channel, RegStatus2, &value);
    if (ok) {
        // STAT12
        if (value & ~0x10u) {
            value &= 0x10u;
            ok = write_adc_register(channel, RegStatus1, value);
        }
    }
    if (ok) {
        // CRC2
        if (value & 0x10u) {
            // The CRC error is latched until cleared by the user. Write 0 to clear the error.
            value &= ~0x10u;
            ok = write_adc_register(channel, RegStatus2, value);
        }
    }
}

//---------------------------------------------------------
static int32_t sram_value(const AdcRec_t *restrict rec, const uint32_t index, int *restrict crcerr)
{
    *crcerr = 0;
    uint64_t v = rec[index].value;
    const uint32_t status = (uint32_t) (v & 0xFFFFFFFF); // AdcStatus

    // 0x07065512 - the special fixed adc command constant (STATUS0 CRC DAT CMD) (07 - good status)
    if ((status & 0x00FFFFFFu) != 0x00065512) {
        const uint32_t st = __builtin_bswap32(status);
        LOG(CRITICAL, "v[%03d]:%016llX status:0x%08X (%06X <> 125506)", index, v, st, st >> 8u);
        DBGF(0, "v[%03d]:%016llX status:0x%08X (%06X <> 125506)", index, v, st, st >> 8u);
        *crcerr = 1;
        if (index > 0) {
            v = rec[index - 1].value;
        } else {
            v = 0;
        }
    }

    // TODO v.bits.ValueCRC;
    const uint32_t raw = (uint32_t) (v >> 32u);
    const uint32_t raw_rev = __builtin_bswap32(raw) >> 8u; // ARM Cortex-A9 specific bswap32()
    int32_t value = (int32_t) raw_rev;
    value <<= 8;
    value >>= 8;
    return value;

    // for negative!
    // TODO - что делать с отрицательными всплесами на униполярном подключении?
    // Актуально-ли делать калибровку от помех на мелочевке в микровольтах?
}

//---------------------------------------------------------
/// @brief Формирование массива данных из SRAM-памяти ESZ7.
/// @param channel    номер блока sram соответствующий номеру АЦП (канал: 1..4)
/// @param bundle_num  массив точек измерений
///
void get_data_from_sram(const int channel, const int bundle_num)
{
    // channel: 1..4
    const int ch = channel - 1; // channale: 1-4 => ch: 0-3

    const AdcRec_t *recs = sram_mem(channel)->bundle[bundle_num].recs;
    toggle_points_buffer_ch(ch);

    int shifts = 0;
    if (channel == 1) {
        // только для первого канала
        const int dt = 1000 / core()->data.max_npnts;
        shifts = cfg()->dpt_delay / dt;
        DBGF(3, "channel: %d, dpt_delay: %d, dt: %d, shifts: %d", channel, cfg()->dpt_delay, dt, shifts);
    }

    bool stat12 = false;
    for (int i = 0; i < core()->npnts; i++) {
        if (recs[i].bits.AdcStatus.bits.STATUS0 & 0x10u) {
            // STAT12
            stat12 = true;
        }
    }

    int crcerr = 0;
    const int npnts = core()->npnts;

    points_t *restrict pnts = &core()->data.values[ch];
    int32_t *restrict R = pnts->R;
    int32_t *restrict W = pnts->W;

    if (shifts > 0) {
        // i < shifts
        const int num = npnts - shifts;
        for (int i = 0; i < shifts; i++) {
            const int off = num + i;
            R[i] = sram_value(recs, off, &crcerr);
        }

        // i >= shifts
        for (int i = shifts; i < npnts; i++) {
            const int off = i - shifts;
            W[i] = sram_value(recs, off, &crcerr);
        }
    } else {
        for (int i = 0; i < npnts; i++) {
            W[i] = sram_value(recs, i, &crcerr);
        }
    }

    pnts->sum = 0;
    for (int i = 0; i < npnts; i++) {
        pnts->sum += pnts->R[i];
    }
    pnts->avrg = pnts->sum / npnts;

    //-------------------------------------------
    // pulse mode - подключение инклинометра "напрямую"
    // FYI: special action for investigation...
    //
    if (cfg()->mode == MODE_APS && cfg()->pulsethreshold) {
        int32_t value = 0;
        for (int i = 0; i < npnts; i++) {
            value = pnts->W[i];
            if (value > 0xFFFF) {
                value = (int32_t) ((double) cfg()->pressure[0].pumps_max / cfg()->pressure[0].scale);
            } else {
                value = (int32_t) ((double) cfg()->pressure[0].pumps_min / cfg()->pressure[0].scale);
            }
            pnts->W[i] = value;
        }
    }

    if (stat12) {
        reset_STAT12(channel);
    }
}

//---------------------------------------------------------
static void get_adc_data(const int channel)
{
    // event_num: 0..3
    static int bundle_num;
    DBGF(4, "channel: %d", channel);

    bundle_num = sram_get_ready_bundle_num(channel);
    if (bundle_num == -1) {
        return;
    }
    sram_dump_create();
    get_data_from_sram(channel, bundle_num);
    sram_clear_bundle_ready(channel, bundle_num);
}

//---------------------------------------------------------
inline void get_hookload()
{
    int idx = 1; // ch1 ch2 (ps1 hkld)
    if (cfg()->atlas_proto_ver == 1) {
        idx = 2; // ch1 ch2 ch3 (ps1 ps2 hkld)
    } else if (cfg()->atlas_proto_ver == 2) {
        idx = 3; // ch1 ch2 ch4 ch3 (ps1 ps2 ch4 hkld)
    }
    core()->data.HKLD24bit = core()->data.values[idx].avrg;
    if (core()->data.HKLD24bit < 0) core()->data.HKLD24bit = 0;
    core()->data.HKLD16bit = (int16_t) ((uint32_t)core()->data.HKLD24bit >> 8u); // приведение к 16 бит
}

//---------------------------------------------------------
inline void get_encoders_data()
{
    // Считать данные на энкодерах из ESZ7-board
    Encoders_t enc = esz7_get_encoders();
    core()->data.encoder1 = enc.Enc0Value;
    core()->data.encoder2 = enc.Enc1Value;
    DBGF(3, "%d %d (0x%08X 0x%08X)", core()->data.encoder1, core()->data.encoder2, core()->data.encoder1,
         core()->data.encoder2);
}

//---------------------------------------------------------
static void core_epoll()
{
    static int channel;
    static struct timespec ts, ts_current, ts_start_measu, ts_total;
    static struct timespec ts_prev[4] = {0};
    static uint8_t got_channels = 0u;

    // ожидание данных с каналов
    struct epoll_event events[EVENTS_EXPECTED];
    while (m_core.thr->wakeup == YES && !m_core.thr->shuttingdown) {
        clock_get_local_time(&ts);

        DBGNL(4);
        if (debug_level >= 1) {
            if (ts_prev[0].tv_sec == 0) {
                ts_prev[0] = ts;
                ts_prev[1] = ts;
                ts_prev[2] = ts;
                ts_prev[3] = ts;
                ts_total = ts;
            }
            if (got_channels == 0) {
                ts_start_measu = ts;
                DBGF(4, "time start measu: %s", timespec_to_str(&ts_start_measu, NULL));
            }
        }
        DBGF(4, "---- epoll_wait(epl()->fd:%d, events: %d, -1);", epl()->fd, EVENTS_EXPECTED);
        const int nEvents = epoll_wait(epl()->fd, events, EVENTS_EXPECTED, -1);
        clock_get_local_time(&ts_current);
        DBGF(4, "got event: %s", timespec_to_str(&ts_current, NULL));

        if (nEvents == -1) {
            LOG(ERROR, "505: INTERNAL ERROR: epoll_wait() returns -1, errno:%d (%s)", errno, strerror(errno));
            DBGF(0, "\n505: INTERNAL ERROR:  epoll_wait() returns -1, errno:%d (%s)", errno, strerror(errno));
            break;
        }

        for (int idx = 0; idx < nEvents; idx++) {
            epl_read_event_file(events[idx].data.fd); // read event to reset him activation
            if (m_core.thr->shuttingdown) {
                DBG(1, "=== got shuttingdown [1]");
                break;
            }
            DBGF(4, "event[%d]: 0x%08X (fd:%d)", idx, events[idx].events, events[idx].data.fd);

            channel = epl_find_event_file(events[idx].data.fd);
            if (channel == -1) {
                LOG(ERROR, "505: INTERNAL ERROR: got unwanted event");
                DBG(0, "505: INTERNAL ERROR: got unwanted event");
                continue;
            }

            if (events[idx].events & EPOLLPRI) {
                if (debug_level >= 1) {
                    timespec_diff(&ts, &ts_prev[channel - 1], &ts_current, NULL);
                    if (debug_level == 1) {
                        DBGF(2, "channel: %d - from last: %ld.%09ld",
                             channel, ts.tv_sec, ts.tv_nsec);
                    } else {
                        DBGF(3, "event %d/%d - channel: %d - from last: %ld.%09ld",
                             idx + 1, nEvents, channel, ts.tv_sec, ts.tv_nsec);
                    }
                    ts_prev[channel - 1] = ts_current;
                    ts_prev[channel - 1] = ts_current;
                }

                if (m_core.thr->wakeup == YES && !m_core.thr->shuttingdown) {
                    get_adc_data(channel); // (!)

                    got_channels |= 1u << (channel - 1u);
                    DBGF(4, "got channels: %s %s %s %s", (got_channels & 0x01u) == 0 ? "0" : "1",
                         (got_channels & 0x02u) == 0 ? "0" : "2",
                         (got_channels & 0x04u) == 0 ? "0" : "3", (got_channels & 0x08u) == 0 ? "0" : "4");
                }
            } else if (events[idx].events & EPOLLERR) {
                DBGF(1, "epoll event[%d] - got error\n", idx);
            }
        } // for()

        if (m_core.thr->shuttingdown) {
            break;
        }

        uint8_t req_channels = 0x0F;
        // data processing and send out
        if (got_channels == req_channels) {
            DBG(4, "all channels captured");
            got_channels = 0;

            get_encoders_data(); // (!)
            get_hookload();

            if (cfg()->mode == MODE_ATLAS) {
                atlas_data_send(); // (!)
                wits_send_data(); // (!)
                atlas_write_telemetry();
            } else if (cfg()->mode == MODE_APS) {
                ads_data_send(channel); // (!)
                siu_data_send(channel);
                dtr_data_send(); // (!)
                aps_wits_data_send();
                aps_write_stat();
            }


            if (debug_level >= 3) {
                static char buf1[TIMEBUFLEN];
                static char buf2[TIMEBUFLEN];
                static struct timespec ts1, ts2;
                timespec_diff(&ts1, &ts_start_measu, &ts_current, NULL);
                clock_get_local_time(&ts_current);
                timespec_diff(&ts2, &ts_total, &ts_current, NULL);
                DBGF(3, "measurement: %s, total: %s\n\n", timespec_to_str(&ts1, buf1),
                     timespec_to_str(&ts2, buf2));
            }
        }
    } // while()
    DBG(1, "--- core_epoll finished");
}

//---------------------------------------------------------
static int thread_stop(thr_t *thr)
{
    (void) thr; // unused now
    // core_t *core = (core_t *) thr->payload;
    sram_dump_close();
    return 0;
}

//---------------------------------------------------------
///
/// Main procedure that does some real job with ESZ7 ADC
///
static int thread_start(thr_t *thr)
{
    static char timebuf[TIMEBUFLEN];
    struct timespec ts_start, ts_current;

    // init
    m_core.thr = thr;
    m_core.data.max_npnts = MAX_NPNTS;
    for (int ch = 0; ch < 4; ch++) {
        m_core.data.toggle[ch] = 0;
        toggle_points_buffer_ch(ch);
    }

    DBGF(1, "\n[tid:%08lX] --- %s (%s) going to be started...",
         pthread_self(), thr->name, thr_type_to_string(thr->type));

    do {
        if (epl_open()) {
            break;
        }
        DBG(1, "try restart...");
        nsleep(TS_1SEC * 2);
    } while (!m_core.thr->shuttingdown);

    if (m_core.thr->shuttingdown) {
        DBG(1, "--- core thread shutting down... [2]");
        m_core.thr = NULL;
        epl_close();
        return 0;
    }

    //---------------------------------------------
    if (cfg()->test_mode) {
        core_test_mode();
        m_core.thr = NULL;
        return 0;
    }

    //---------------------------------------------
    DBG(1, "start ESZ7 ADC autoscan...");
    dev_start_channels();
    DevStat_t status;
    dev_read_status(&status);
    DBGF(1, "AUTOSCAN: 0x%08X - %s", status.value, print_dev_status(&status));

    //---------------------------------------------
    if (cfg()->mode == MODE_ATLAS) {
        DBG(1, "==== Atlas mode was selected ====");
        // посылать данные в UDP-порт всегда, не ожидая какой-либо готовности
        thr_wakeup(m_core.thr);
    } else if (cfg()->mode == MODE_APS) {
        DBG(1, "==== APS mode was selected ====");
#if 1
        // ждать разрешения для выдачи данных от других потоков
        thr_asleep(m_core.thr);
        DBGF(3, "-- %s paused...", m_core.thr->name);
#else
        thr_wakeup(m_core.thr);
#endif
    }

    nanosleep(&(struct timespec){.tv_sec = 0, .tv_nsec = TS_10MS}, NULL);

    //---------------------------------------------
    do {
        clock_get_local_time(&ts_start);

        pthread_mutex_lock(&m_core.thr->mutex);
        int rc = 0;
        while (m_core.thr->wakeup != YES && rc == 0) {
            DBGF(8, "%s pthread_cond_wait() - waiting for event", m_core.thr->name);
            rc = pthread_cond_wait(&m_core.thr->cond, &m_core.thr->mutex);
            DBGF(4, "%s pthread_cond_wait() - enabled to work ( rc:%d %s)",
                 m_core.thr->name, rc, (rc != 0 ? strerror(rc) : ""));
        }
        if (m_core.thr->shuttingdown) {
            DBG(3, "got shutting down...");
            pthread_mutex_unlock(&m_core.thr->mutex);
            break;
        }
        pthread_mutex_unlock(&m_core.thr->mutex);

        core_epoll(); // main job here...
    } while (!m_core.thr->shuttingdown);

    DBG(1, "--- core thread shutting down...");
    m_core.thr = NULL;

    //---------------------------------------------
    epl_close();
    dev_stop_channels();
    dev_read_status(&status);
    DBGF(1, "AUTOSCAN: 0x%08X - %s", status.value, print_dev_status(&status));

    clock_get_local_time(&ts_current);
    struct timespec ts;
    timespec_diff(&ts, &ts_start, &ts_current, NULL);
    char timebuf2[TIMEBUFLEN], timebuf3[TIMEBUFLEN];
    LOG(INFO, "work time: %s sec [%s  -  %s]",
        timespec_to_str(&ts, timebuf3),
        timespec_to_dd_mm_yyyy_hh_mm_ss_nz(&ts_start, 0, timebuf),
        timespec_to_dd_mm_yyyy_hh_mm_ss_nz(&ts_current, 0, timebuf2));
    DBGF(0, "work time: %s sec [%s  -  %s]",
         timespec_to_str(&ts, timebuf3),
         timespec_to_dd_mm_yyyy_hh_mm_ss_nz(&ts_start, 0, timebuf),
         timespec_to_dd_mm_yyyy_hh_mm_ss_nz(&ts_current, 0, timebuf2));
    DBG(3, "--- core_thread finished ---");
    return 0;
}

//---------------------------------------------------------
bool core_run()
{
    THR_START(thr, THR_JOIN, thread_start, thread_stop, &m_core, false, "Core");
    if (!thr) {
        return FAIL;
    }
    thr->type = THR_TYPE_CORE;
    DBG_NSLEEP(TS_500MS);
    return OK;
}
