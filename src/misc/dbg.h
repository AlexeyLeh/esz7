/**
  \file   dbg.h
  \author Aleksey Lekhtin
  \date   2024/02/01
  \brief  debug features
*/

#ifndef DBG__H_
#define DBG__H_

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "cfg.h"

extern int debug_level;

#define USE_DEBUG
#define USE_DEBUG_TIME

#ifdef USE_DEBUG

const char *dbg_funcname(const char *funcname, char *outbuf, int outbuflen);

//const char *gettime();
const char *dbg_gettime__y_m_d__h_m_s();

void DBG_LOCK();
void DBG_UNLOCK();

/*-------------------------------------------------------*/
#define DBGNoNL(LEVEL, ...)  \
do { \
    if (debug_level >= (LEVEL)) { \
        char _func_name_[64];     \
        const char *_t_i_m_ = dbg_gettime__y_m_d__h_m_s(); \
        dbg_funcname(__func__, _func_name_, sizeof(_func_name_)); \
        char *_p_ = _func_name_ + strlen(_func_name_); \
        while (*_p_ == ' ' || *_p_ == '\t') { \
            *_p_ = '\0'; \
            --_p_; \
        } \
        if (*#__VA_ARGS__) {    \
            const char *_msg_ptr_ = __VA_ARGS__""; \
            int _o_f_f_ = 0; \
            if (_msg_ptr_[0] == '\n') { \
                puts(""); \
                _o_f_f_ = 1; \
            } \
            if (cfg()->shortmsg == 0) { \
                printf("%s [%d] %s %s", _t_i_m_, LEVEL, _func_name_, _msg_ptr_ + _o_f_f_); \
            } else { \
                printf("%s %s", _t_i_m_, _msg_ptr_ + _o_f_f_); \
            } \
        } else { \
            if (cfg()->shortmsg == 0) { \
                printf("%s [%d] %s", _t_i_m_, LEVEL, _func_name_); \
            } else { \
                ; \
            } \
        } \
        fflush(stdout); \
    } \
} while(0)

/*-------------------------------------------------------*/
#define DBG(LEVEL, ...) \
    do { \
        if (debug_level >= (LEVEL)) { \
            DBG_LOCK(); \
            if (*#__VA_ARGS__) {    \
                DBGNoNL(LEVEL, __VA_ARGS__ "\n"); \
            } else { \
                DBGNoNL(LEVEL, " \n"); \
            } \
            DBG_UNLOCK(); \
        } \
    } while(0)

/*-------------------------------------------------------*/
#define DBGFNoNL(LEVEL, FORMAT, ...)  \
do { \
    if (debug_level >= (LEVEL)) { \
        const char *_t_i_m_ = dbg_gettime__y_m_d__h_m_s(); \
        char _buf_[1024*64]; \
        char _func_name_[64]; \
        dbg_funcname(__func__, _func_name_, sizeof(_func_name_)); \
        char *p_t_r = _func_name_ + strlen(_func_name_); \
        while (*p_t_r == ' ' || *p_t_r == '\t') { \
            *p_t_r = '\0'; \
            p_t_r--; \
        } \
        snprintf(_buf_, sizeof(_buf_), FORMAT, __VA_ARGS__);      \
        int _o_f_f_ = 0; \
        if (_buf_[0] == '\n') { \
            puts(""); \
            _o_f_f_ = 1; \
        } \
        if (cfg()->shortmsg == 0) { \
            printf("%s [%d] %s %s", _t_i_m_, LEVEL, _func_name_, _buf_ + _o_f_f_); \
        } else { \
            printf("%s %s", _t_i_m_, _buf_ + _o_f_f_); \
        } \
        fflush(stdout); \
    } \
} while(0)

/*-------------------------------------------------------*/
#define DBGF(LEVEL, FORMAT, ...) \
    do { \
        if (debug_level >= (LEVEL)) { \
            DBG_LOCK(); \
            DBGFNoNL(LEVEL, FORMAT "\n", __VA_ARGS__); \
            DBG_UNLOCK(); \
        } \
    } while(0)

/*-------------------------------------------------------*/
#define DBG_PRINT(LEVEL, FORMAT, ...) \
do { \
    if (debug_level >= (LEVEL)) { \
        static char buf[4096] = {0}; \
        char func_name[64]; \
        const char *tim = dbg_gettime__y_m_d__h_m_s(); \
        dbg_funcname(__func__, func_name, sizeof(func_name)); \
        char *_p_ = func_name + strlen(func_name); \
        while (*_p_ == ' ' || *_p_ == '\t') { \
            *_p_ = '\0'; \
            --_p_; \
        } \
        snprintf(buf, sizeof(buf), FORMAT, __VA_ARGS__); \
        int off = 0; \
        if (buf[0] == '\n') { \
            puts(""); \
            off = 1; \
        } \
        printf("%s [%d] %s %s", tim, LEVEL, func_name, buf + off); \
        fflush(stdout); \
    } \
} while(0)

/*-------------------------------------------------------*/
#define DBGNL(LEVEL) \
do { \
    if (debug_level >= (LEVEL)) { \
        DBG_LOCK(); \
        puts(""); \
        DBG_UNLOCK(); \
    } \
} while(0)

//---------------------------------------------------------
#define DBG_NSLEEP(NSEC) \
do { \
    if (debug_level > 0) { \
        const struct timespec *reqtime = &(struct timespec){.tv_sec = 0, .tv_nsec = (NSEC)}; \
        int ret = nanosleep(reqtime, NULL); \
        if (ret == -1) { \
            int e = errno; \
            DBGF(0, "505: INTERNAL ERROR: %s:%d failed - (%d), %s", __func__, __LINE__, e, strerror(e)); \
        } \
    } \
} while(0)


#else

/*-------------------------------------------------------*/
#define DBGNNL(LEVEL, ...)
#define DBG(LEVEL, ...)
#define DBGFNNL(LEVEL, FORMAT, ...)
#define DBGF(LEVEL, FORMAT, ...)
#define DBGNL(LEVEL)

#endif // USE_DEBUG

#endif
