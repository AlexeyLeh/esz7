/**
  \file   thr.c
  \author Aleksey Lekhtin
  \date   Jul 26, 2024
  \brief  Threads
*/

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>

#include "cfg.h"
#include "dbg.h"
#include "thr.h"
#include "utils.h"
#include "log.h"

static thrs_t m_thrs = {};
static int m_thr_cnt = 0;
static pthread_mutex_t m_mutex;

//---------------------------------------------------------
static void thr__print_info()
{
    if (debug_level >= 4) {
        DBG_LOCK();
        for (int i = 0; i < NTHREAD; i++) {
            if (m_thrs.thrs[i]) {
                DBGFNoNL(5, "[%d] %-35s (%p) \n", i, m_thrs.thrs[i]->name, m_thrs.thrs[i]);
            }
        }
        fflush(stdout);
        DBG_UNLOCK();
    }
}

//---------------------------------------------------------
static void *thr__control(void *t)
{
    pthread_t tid = pthread_self();
    int ret = -1;
    thr_t *thr = t;
    if (thr->start_func) {
        ret = thr->start_func(thr);
    }
    if (ret == -1) {
        LOG(ERROR, "505: INTERNAL ERROR: start func returned %d - SOMETHING WRONG", ret);
        DBGF(0, "\n505: INTERNAL ERROR: start func returned %d - SOMETHING WRONG\n", ret);
    }

    DBGF(8, "\n[tid:%08lX] lock thr::m_mutex", tid);
    pthread_mutex_lock(&m_mutex);
    thr__print_info();
    DBGF(8, "[tid:%08lX] finishing for '%s'", tid, thr->name);
    for (int i = 0; i < NTHREAD; i++) {
        if (m_thrs.thrs[i] == thr) {
            DBGF(8, "[tid:%08lX] take [%d]%s(%p) ",
                 tid, i, m_thrs.thrs[i]->name, m_thrs.thrs[i]);
            m_thrs.thrs[i] = NULL;
            m_thrs.total--;
            break;
        }
    }

    if (thr->free_payload) {
        DBGF(8, "[tid:%08lX] free(thr::payload:%p)", tid, thr->payload);
        free(thr->payload);
        thr->payload = NULL;
    }

    DBGF(5, "[tid:%08lX] %s finished (%p) - %s:%d", tid, thr->name, (void *)thr, basename(__FILE__), __LINE__);
    DBGF(8, "[tid:%08lX] free(thr:%p)", tid, (void *)thr);
    free(thr);
    thr__print_info();
    DBGF(8, "[tid:%08lX] done - unlock thr::m_mutex\n", tid);
    pthread_mutex_unlock(&m_mutex);
    return NULL;
}

//---------------------------------------------------------
static void thr__stop(thr_t *thr)
{
    if (!thr) {
        return;
    }
    pthread_t tid = pthread_self();
    thr->shuttingdown = YES;
    thr->wakeup = YES;
    DBGF(8, "\n[tid:%08lX] === %s id:%d group:%d wakeup:%d, shuttingdown:%d, emit pthread_cond_signal()",
         tid, thr->name, thr->id, thr->group_id, thr->wakeup, thr->shuttingdown);
    pthread_cond_signal(&thr->cond);

    if (thr->stop_func) {
        DBGF(8, "[tid:%08lX] %s thr::stop_func", tid, thr->name);
        thr->stop_func(thr);
    }
}

//---------------------------------------------------------
thr_t *thr_create(const thr_prop_t *thr_prop)
{
    assert(m_thrs.total < NTHREAD);

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&m_mutex, &attr);

    thr_t *thr = calloc(1, sizeof(thr_t));
    if (thr_prop->name[0]) {
        snprintf(thr->name, sizeof(thr->name), "%s", thr_prop->name);
    } else {
        snprintf(thr->name, sizeof(thr->name), "id%d", thr->id);
    }
    thr->id = ++m_thr_cnt;
    thr->group_id = thr_prop->group_id;
    thr->type = thr_prop->type;
    thr->start_func = thr_prop->start_func;
    thr->stop_func = thr_prop->stop_func;
    thr->payload = thr_prop->payload;
    thr->free_payload = thr_prop->free_payload;

    DBGF(5, "\n[tid:%09lX] %s   thr->payload: %p", pthread_self(), thr->name, thr->payload);

    thr->mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
    pthread_condattr_init(&thr->condattr);
#ifdef __linux__
    pthread_condattr_setclock(&thr->condattr, CLOCK_MONOTONIC);
#endif
    pthread_cond_init(&thr->cond, &thr->condattr);
    pthread_attr_init(&thr->attr);

    int ret = pthread_create(&thr->pthread, &thr->attr, thr__control, thr);
    if (ret != 0) {
        int e = errno;
        LOG(ERROR, "Can't create thread %s (%s)", thr->name, strerror(e));
        DBGF(0, "ERROR: Can't create thread %s (%s)", thr->name, strerror(e));
        free(thr);
        return NULL;
    }
    DBGF(5, "[tid:%09lX] thread created: %p, id: %d - %s (payload:%p)",
         pthread_self(), (void *) thr, thr->id, thr->name, thr->payload);
    thr->wakeup = NO;
    thr->shuttingdown = NO;

    // save in list
    pthread_mutex_lock(&m_mutex);
    for (int i = 0; i < NTHREAD; i++) {
        if (m_thrs.thrs[i] == NULL) {
            m_thrs.thrs[i] = thr;
            m_thrs.total++;
            break;
        }
    }
    thr__print_info();
    pthread_mutex_unlock(&m_mutex);

    if (thr_prop->mode == THR_DETACH) {
        ret = pthread_detach(thr->pthread);
    } else {
        ret = pthread_join(thr->pthread, NULL);
    }

    if (ret != 0) {
        const int e = errno;
        LOG(ERROR, "Can't core_run thread %s (%s)", thr->name, strerror(e));
        DBGF(0, "ERROR: Can't core_run thread %s (%s)", thr->name, strerror(e));
        thr_stop(thr);
    }
    return thr;
}

//---------------------------------------------------------
void thr_stop(thr_t *thr)
{
    if (!thr) {
        return;
    }
    DBGF(8, "lock thr::m_mutex (for %s)", thr->name);
    pthread_mutex_lock(&m_mutex);
    thr__stop(thr);
    DBG(8, "unlock thr::m_mutex");
    pthread_mutex_unlock(&m_mutex);
}

//---------------------------------------------------------
void thr_stop_all()
{
    pthread_t tid = pthread_self();
    DBGF(8, "\n[tid:%08lX] lock thr::m_mutex", tid);
    pthread_mutex_lock(&m_mutex);
    thr__print_info();
    for (int i = NTHREAD - 1; i >= 0; i--) {
        thr_t *thr = m_thrs.thrs[i];
        if (thr) {
            thr__stop(thr);
        }
    }
    DBGF(8, "[tid:%08lX] unlock thr::m_mutex\n", tid);
    pthread_mutex_unlock(&m_mutex);
}

//---------------------------------------------------------
void thr_wait_all_done(uint32_t sec)
{
    pthread_t tid = pthread_self();
    thr__print_info();
    uint32_t counter = sec * TS_1SEC / TS_100MS;
    while (m_thrs.total > 0) {
        DBGF(2, "[tid:%08lX] %-2d: thrs.total: %d", tid, counter, m_thrs.total);
        DBG_NSLEEP(TS_100MS);
        counter--;
        if (counter == 0) {
            thr__print_info();
            break;
        }
    }
    DBGF(2, "[tid:%08lX] thrs.total: %d", tid, m_thrs.total);
}

//---------------------------------------------------------
void thr_stop_group(const int group_id)
{
    if (group_id == 0) {
        return;
    }
    DBG(8, "lock thr::m_mutex");
    pthread_mutex_lock(&m_mutex);
    thr__print_info();
    for (int i = 0; i < NTHREAD; i++) {
        thr_t *thr = m_thrs.thrs[i];
        if (thr) {
            if (thr->group_id == group_id) {
                DBGF(8, "[%d] going to be stop %s group id:%d", i, thr->name, group_id);
                thr__stop(thr);
            }
        }
    }
    DBG(8, "unlock `thr::m_mutex");
    pthread_mutex_unlock(&m_mutex);
}

//---------------------------------------------------------
void thr_wakeup(thr_t *thr)
{
    if (!thr) {
        DBGF(5, "[tid:%08lX] thr:%p [not started yet]", pthread_self(), thr);
        return;
    }
    pthread_mutex_lock(&thr->mutex);
    if (debug_level < 6) {
        DBGF(5, "%s - emit pthread_cond_signal()", thr->name);
    }
    thr->wakeup = YES;
    pthread_cond_signal(&thr->cond);
    pthread_mutex_unlock(&thr->mutex);
}

//---------------------------------------------------------
void thr_wakeup_by_type(const thr_type_t type)
{
    pthread_mutex_lock(&m_mutex);
    DBGF(5, "type: %d", type);
    for (int i = 0; i < NTHREAD; i++) {
        thr_t *thr = m_thrs.thrs[i];
        if (thr) {
            if (thr->type == type) {
                pthread_mutex_lock(&thr->mutex);
                DBGF(5, "%s - emit pthread_cond_signal()", thr->name);
                thr->wakeup = YES;
                pthread_cond_signal(&thr->cond);
                pthread_mutex_unlock(&thr->mutex);
            } else {
                DBGF(6, "skip thr: %p type: %d '%s'", thr, (thr ? thr->type : 0), (thr ? thr->name : ""));
            }
        }
    }
    pthread_mutex_unlock(&m_mutex);
}

//---------------------------------------------------------
void thr_asleep(thr_t *thr)
{
    if (thr) {
        pthread_mutex_lock(&thr->mutex);
        thr->wakeup = NO;
        DBGF(5, "%s to sleep - wakeup: %d, shuttingdown: %d",
             thr->name, thr->wakeup, thr->shuttingdown);
        pthread_mutex_unlock(&thr->mutex);
    }
}

//---------------------------------------------------------
const char *thr_type_to_string(thr_type_t type)
{
    if (type == THR_TYPE_UNKN) return "THR_TYPE_UNKN";
    if (type == THR_TYPE_ATLAS) return "THR_TYPE_ATLAS";
    if (type == THR_TYPE_WITS) return "THR_TYPE_WITS";
    if (type == THR_TYPE_WITS_CLIENT) return "THR_TYPE_WITS_CLIENT";
    if (type == THR_TYPE_CORE) return "THR_TYPE_ESZ7";
    if (type == THR_TYPE_ESZ7_CMD) return "THR_TYPE_ESZ7_CMD";
    if (type == THR_TYPE_ADS_CMD) return "THR_TYPE_ADS_CMD";
    if (type == THR_TYPE_ADS_DATA) return "THR_TYPE_ADS_DATA";
    if (type == THR_TYPE_SIU_CMD) return "THR_TYPE_SIU_CMD";
    if (type == THR_TYPE_SIU_DATA) return "THR_TYPE_SIU_DATA";
    if (type == THR_TYPE_DTR_CMD) return "THR_TYPE_DTR_CMD";
    if (type == THR_TYPE_DTR_DATA) return "THR_TYPE_DTR_DATA";
    if (type == THR_TYPE_APS_WITS) return "THR_TYPE_APS_WITS";
    return "UNKN";
}
