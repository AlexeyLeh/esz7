/**
  \file   sig.h
  \author Aleksey Lekhtin
  \date   April 4, 2024
  \brief  catch signals
*/

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include "cfg.h"
#include "dbg.h"
#include "epl.h"
#include "core.h"
#include "sig.h"
#include "log.h"

#ifdef WIN32
typedef __p_sig_fn_ef_ sighandler_t;
#endif

/*-------------------------------------------------------*/
static void try_exit_gracefully();
static const char *signal_name(int snum);
static int sigs[] = {SIGINT, SIGABRT, SIGTERM, SIGSEGV, SIGUSR1, SIGUSR2, SIGQUIT, SIGPIPE, 0, 0};
static int lockstatus = 0;

/*-------------------------------------------------------*/
void sigaction_handler(int snum)
{
    //if (cfg()->test_mode) {
    //    printf("%s", "\033[2J\033[H"); // clear terminal  // ESC
    //    printf("\033[200B\n"); // jump to terminal bottom // ESC
    //}

    LOG(INFO, "========== Signal %s (lock:%d) ==========",
        signal_name(snum), lockstatus);
    DBGF(0, "%s========== Signal %s (lock:%d) ==========",
         (debug_level > 0 ? "\n" : ""),
         signal_name(snum), lockstatus);

    if (lockstatus == 1) {
        LOG(CRITICAL, "again signal. Oh shit! Something really wrong!");
        try_exit_gracefully();
        nanosleep(&(struct timespec) {.tv_sec = 0, .tv_nsec = TS_100MS}, NULL);
        LOG(INFO, "terminated");
        DBG(0, "terminated");
        exit(0);
    }
    lockstatus = 1;
    int currentdbg = debug_level;

    if (snum == SIGSEGV || snum == SIGABRT) {
        try_exit_gracefully();
        nanosleep(&(struct timespec) {.tv_sec = 0, .tv_nsec = TS_100MS}, NULL);
        LOG(INFO, "terminated");
        DBG(0, "terminated");
        exit(0);
    }

    if (snum == SIGTERM) {
        try_exit_gracefully();
        return;
    }

    try_exit_gracefully();
    set_sig_action();
    debug_level = currentdbg;
}

/*-------------------------------------------------------*/
bool set_sig_action()
{
#ifdef __linux__
    struct sigaction old_sa = {0};
    struct sigaction sa = {0};
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = sigaction_handler;
#endif

#ifdef SIGBREAK
    int sz = sizeof(sigs)/sizeof(sigs[0]);
    sigs[sz-2] = SIGBREAK;
#endif
    for (int i = 0; sigs[i]; ++i) {
#ifdef __linux__
        if (sigaction(sigs[i], &sa, &old_sa) == -1) {
            return 0;
        }
#endif
#ifdef WIN32
        __p_sig_fn_ef_ old_sig = signal(sigs[i], sigHandler);
        if (old_sig == SIG_ERR) return 0;
#endif
    }

    return 1;
}

/*-------------------------------------------------------*/
static const char *signal_name(int snum)
{
    if (snum == SIGINT)
        return "SIGINT";
    if (snum == SIGABRT)
        return "SIGABRT";
    if (snum == SIGTERM)
        return "SIGTERM";
    if (snum == SIGPIPE)
        return "SIGSEGV";
    if (snum == SIGSEGV)
        return "SIGSEGV";
    if (snum == SIGUSR1)
        return "SIGUSR1";
    if (snum == SIGUSR2)
        return "SIGUSR2";
    if (snum == SIGQUIT)
        return "SIGQIUT";
#ifdef SIGBREAK
    if (snum == SIGBREAK) return "SIGBREAK";
#endif
    static char buf[32];
    snprintf(buf, sizeof(buf), "%d", snum);
    return buf;
}

/*-------------------------------------------------------*/
static void try_exit_gracefully()
{
    DBGF(1, "\n%s", __func__);
    THR_STOP(core()->thr);
    //if (cfg()->test_mode) {
    //    printf("%s", "\033[200B"); // jump to terminal bottom // ESC
    //}
    nanosleep(&(struct timespec) {.tv_nsec = TS_200MS}, NULL);
    DBG(3, "done");
}
