/**
  \file   dbg.c
  \author Aleksey Lekhtin
  \date   2024/02/01
  \brief  debug features
*/

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "dbg.h"
#include "utils.h"
#include "thr_t.h"

int debug_level = 0;
static thread_local char str[32];
static thread_local char bf[64];

static pthread_mutex_t dbg_mutex = PTHREAD_MUTEX_INITIALIZER;

//---------------------------------------------------------
inline void DBG_LOCK()
{
    pthread_mutex_lock(&dbg_mutex);
}

//---------------------------------------------------------
inline void DBG_UNLOCK()
{
    pthread_mutex_unlock(&dbg_mutex);
}

// //---------------------------------------------------------
// static const char *gettime()
// {
//     static struct timespec ts;
//     struct tm tm = clock_get_local_time(&ts);
//     strftime(str, sizeof(str), "%d.%m.%Y %H:%M:%S", &tm);
//     snprintf(bf, sizeof(bf), "%s.%06ld", str, ts.tv_nsec / 1000);
//     return bf;
// }

//---------------------------------------------------------
const char *dbg_gettime__y_m_d__h_m_s()
{
    static struct timespec ts;
    struct tm tm = clock_get_local_time(&ts);
    strftime(str, sizeof(str), "%Y-%m-%d %H:%M:%S", &tm);
    snprintf(bf, sizeof(bf), "%s.%03ld", str, ts.tv_nsec / 1000000);
    return bf;
}

//---------------------------------------------------------
const char *dbg_funcname(const char *funcname, char *outbuf, int outbuflen)
{
    static int swidth = 25;
    strncpy(outbuf, funcname, outbuflen-3);
    //strcat(outbuf, "()");

    int sz = (int)strlen(outbuf);
    int ln = swidth - sz;
    if (ln < 0) ln = 0;
    int i;
    for(i = 0; i < ln; i++) {
        outbuf[sz + i] = ' ';
    }
    outbuf[sz + i] = '\0';
    return outbuf;
}
