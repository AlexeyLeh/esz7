/**
    \file   main.c
    \author Aleksey Lekhtin
    \date   May 04, 2024
    \brief  Main procedure
*/

#include <stdio.h>

#include "ads.h"
#include "atlas.h"
#include "cfg.h"
#include "core.h"
#include "dbg.h"
#include "dtr.h"
#include "epl.h"
#include "esz7_dev.h"
#include "log.h"
#include "sig.h"
#include "siu.h"
#include "thr.h"
#include "wsa.h"
#include "utils.h"
#include "aps_wits.h"
#include "wits.h"

static bool get_environment(int ac, char *av[]);

static bool services_start();

static void services_stop();

//---------------------------------------------------------
/**
 * Start procedure
 * @param ac - number of command line arguments
 * @param av - list of command line arguments
 * @return 0 - no error
 * @return 1 - error
 */
int main(int ac, char *av[])
{
    tzset();
    log_set_enabled(true);
#ifdef WIN32
    // SetConsoleOutputCP(CP_UTF8);
    SetConsoleOutputCP(1251);
    setbuf(stdout, 0);
#endif

    //-----------------------------------
    parse_help_args(ac, av);

    if (!get_environment(ac, av)) {
        return 1;
    }

    //-----------------------------------
    if (!epl_checkin()) {
        return 2;
    }

    //-----------------------------------
    if (!esz7_open()) {
        LOG(FATAL, "ERROR: Can't start ADC board");
        DBG(0, "ERROR: Can't start ADC board");
        return 3;
    }

    //-----------------------------------
    set_sig_action();
    wsa_startup();

    bool ok = services_start();
    if (ok) {
        if (!cfg()->test_mode) {
            ok = core_cmd_run(); // запуск tcp-порта для приема команд
        }
    }
    if (ok) {
        core_run(); // запуск процедур работы с АЦП ESZ7-board
    }

    //-----------------------------------
    DBG(1, "=== finishing ===");
    siu_save_data_status();
    services_stop();
    DBG(1, "===  exiting  ===");
    thr_wait_all_done(2);
    DBG(0, "done\n");
    LOG(INFO, "done\n");
    return 0;
}

//---------------------------------------------------------
static bool get_environment(int ac, char *av[])
{
    cfg_get_app_path(ac, av);
    log_open_file(cfg()->logfname);

    LOG(INFO, "run: %s/%s %s", cfg()->appdir, cfg()->appname, cfg()->args);
    LOG(INFO, "%s", version());
    check_byteorder();

    bool ok = cfg_read();
    if (!ok) {
        return FAIL;
    }
    ok = cfg_parse_args(ac, av);
    if (!ok) {
        return FAIL;
    }
    ok = cfg_read_esz7_status();
    if (!ok) {
        return FAIL;
    }

    cfg_check_defaults();
    cfg_print_configuration();

    LOG(INFO, "mode:  %s", cfg_mode_string(cfg()->mode));
    DBGF(1, "mode:  %s", cfg_mode_string(cfg()->mode));
    DBGF(1, "debug: %d", debug_level);
    return OK;
}

//---------------------------------------------------------
static void services_stop()
{
    epl_close();
    thr_stop_all();
    wsa_cleanup();
    ads_packet_free();
    siu_packet_free();
    dtr_packet_free();
    aps_wits_packet_free();
    atlas_stop();
    close_log();
}

//---------------------------------------------------------
static bool services_start()
{
    if (cfg()->test_mode > TEST_NONE) {
        return OK;
    }

    //-----------------------------------
    if (cfg()->mode == MODE_ATLAS) {
        if (!atlas_run()) {
            return FAIL;
        }
        if (!wits_run()) {
            return FAIL;
        }
        return OK;
    }

    //-----------------------------------
    if (cfg()->mode == MODE_APS) {
        siu_read_data_status();

        if (!ads_run()) {
            return FAIL;
        }
        if (!siu_run()) {
            return FAIL;
        }
        if (!dtr_run()) {
            return FAIL;
        }
        if (!aps_wits_run()) {
            return FAIL;
        }
        return OK;
    }

    LOG(ERROR, "========== UNKNOWN WORK MODE DETECTED ==========");
    DBG(0, "ERROR: ========== UNKNOWN WORK MODE DETECTED ==========\n");
    return FAIL;
}
