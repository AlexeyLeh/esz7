/**
    \file   dev.c
    \author Aleksey Lekhtin
    \date   18.09.2025
    \brief  Работа с ESZ7 платой (с XME0724-20 SoC)
*/

#include <stdio.h>

#include "atlas.h"
#include "ads.h"
#include "cfg.h"
#include "dbg.h"
#include "adc.h"
#include "esz7_dev.h"
#include "sram.h"
#include "sram_t.h"
#include "core.h"
#include "log.h"

static void print_adc_registers(int channel);

//---------------------------------------------------------
bool esz7_open()
{
    if (!esz7_memmap_registers()) {
        return FAIL;
    }

    if (!esz7_stop_autoscan()) {
        return FAIL;
    }

    esz7_check_adc_id();

    if (!esz7_init_adc()) {
        return FAIL;
    }

    DBGF(1, "FPGA Firmware Version: %08Xh\n", read_firmware_version());

    //if (!esz7_adjust_scan_period()) {
    //    return FAIL;
    //}

    esz7_setup();

    // TODO if (!esz7_adc_calibration()) {
    //     return FAIL;
    // }

    return OK;
}

/*---------------------------------------------------------*/
void esz7_close()
{
    DBG(3);
    esz7_unmemmap_registers();
}

/*---------------------------------------------------------*/
bool esz7_stop_autoscan()
{
    DBG(8, "check AUTOSCAN mode is off");
    if (!is_dev_adc_scanning()) {
        DBG(2, "AUTOSCAN mode is OFF");
        return OK;
    }

    DBG(2, "\nWARNING! AUTOSCAN mode is ON - operations not permitted. try to stop device timers...");
    dev_stop_channels();
    if (is_dev_adc_scanning()) {
        LOG(ERROR, "can't stop ADC AUTOSCAN mode");
        DBG(0, "ERROR: can't stop ADC AUTOSCAN mode");
        return FAIL;
    }
    DBG(2, "AUTOSCAN mode is OFF");
    return OK;
}

/*---------------------------------------------------------*/
/**
  * Try to get ADC ID to be sure ADC is online
  * @return bool
  * @return OK (1) - success
  * @return FAIL (0) - error
 */
void esz7_check_adc_id()
{
    uint8_t id[4] = {0};

    int channel;
    for (int num = 0; num < 4; num++) {
        channel = num + 1;
        read_adc_id(channel, &id[num]);
    }

    for (int num = 0; num < 4; num++) {
        if (id[num] != 0) {
            continue;
        }
        channel = num + 1;
        DBGF(2, "RESET ADC-%d ... ", channel);
        reset_adc(channel);
    }

    for (int num = 0; num < 4; num++) {
        if (id[num] != 0) {
            continue;
        }
        channel = num + 1;
        read_adc_id(channel, &id[num]);
    }

    DBGNL(1);
    for (int num = 0; num < 4; num++) {
        channel = num + 1;
        DBGF(2, "ADC-%d ID - 0x%02X %s", channel, id[num], (id[num] == 0 ? "can't read ID" : ""));
    }
    DBGNL(1);
}

//---------------------------------------------------------
bool esz7_init_adc()
{
    for (int channel = 1; channel <= 4; channel++) {
        DBGF(2, "\n=== init ADC-%d ", channel);
        if (!init_adc(channel)) {
            return FAIL;
        }
    }
    puts("");

    if (debug_level >= 9) {
        for (int adc_num = 1; adc_num <= 4; adc_num++) {
            print_adc_registers(adc_num);
            puts("");
        }
        puts("");
    }

    return OK;
}

//---------------------------------------------------------
void esz7_setup()
{
    double expected_period_to_1sec = 1.0 / cfg()->atlas_timeshift;
    uint32_t scanperiod = 200000;  // default - 1 sec for 500 points at 100MHz
    if (cfg()->mode == MODE_ATLAS) {
        core()->npnts = atlas()->npnts500;
        scanperiod = cfg()->adc_atlas.scanperiod;
        DBGF(1, "-- set AUTOSCAN period - %u ticks (%u shots/%0.2fsec)",
             scanperiod, (uint32_t) ((double) cfg()->adc_atlas.fpgafreq / scanperiod * expected_period_to_1sec),
             expected_period_to_1sec);

    } else if (cfg()->mode == MODE_APS) {
        core()->npnts = ads()->data.npnts100;
        scanperiod = cfg()->adc_aps.scanperiod;
        DBGF(1, "-- set AUTOSCAN period - %u ticks (%u shots/%0.2fsec)",
             scanperiod, (uint32_t) ((double) cfg()->adc_aps.fpgafreq / scanperiod * expected_period_to_1sec),
             expected_period_to_1sec);
    }

    if (cfg()->test_mode) {
        scanperiod = cfg()->test_scanperiod;
    }

    for (int channel = 1; channel <= 4; channel++) {
        dev_set_adc_scan_period(channel, scanperiod);
    }

    for (int channel = 1; channel <= 4; channel++) {
        sram_clear_bundle_ready(channel, -1); // -1 => all
    }

    sram_init(core()->npnts);
    DBGNL(1);
}

//---------------------------------------------------------
static void print_adc_registers(int channel)
{
    // channel: 1..4
    uint8_t reg_num, value;
    uint8_t regs[] = {0, 1, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 16, 17, 18};
    for (int i = 0; i < (int) sizeof(regs); i++) {
        reg_num = regs[i];
        if (read_adc_register(channel, reg_num, &value)) {
            DBGF(debug_level, "ADC-%d RegNo %2Xh - 0x%02X %s\n", channel, reg_num, value, print_response_details(reg_num, value));
        } else {
            DBGF(debug_level, "ADC-%d RegNo %2Xh - FAILED\n", channel, reg_num);
            return;
        }
    }
}
