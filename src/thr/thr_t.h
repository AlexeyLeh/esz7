/**
  \file   thr_t.h
  \author Aleksey Lekhtin
  \date   Jan 29, 2025
  \brief  Threads
*/

#ifndef THR_STRUCT__H_
#define THR_STRUCT__H_

#include <pthread.h>
#ifdef __linux__
#include <threads.h>
#endif
#include <stdint.h>

#ifndef thread_local
# if __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
#  define thread_local _Thread_local
# elif defined _WIN32 && ( \
       defined _MSC_VER || \
       defined __ICL || \
       defined __DMC__ || \
       defined __BORLANDC__ )
#  define thread_local __declspec(pthread)
/* note that ICC (linux) and Clang are covered by __GNUC__ */
# elif defined __GNUC__ || \
       defined __SUNPRO_C || \
       defined __hpux || \
       defined __xlC__
#  define thread_local __thread
# else
#  error "Cannot define thread_local"
# endif
#endif

#define NTHREAD 20

//-----------------------------------------------
typedef struct thr_t thr_t;

typedef int (*thr_func_t)(thr_t *thr);

//-----------------------------------------------
typedef enum thr_type_t : uint8_t
{
    THR_TYPE_UNKN = 0,
    THR_TYPE_CORE,
    THR_TYPE_ATLAS,
    THR_TYPE_WITS,
    THR_TYPE_WITS_CLIENT,
    THR_TYPE_ESZ7_CMD,
    THR_TYPE_ADS_CMD,
    THR_TYPE_ADS_DATA,
    THR_TYPE_SIU_CMD,
    THR_TYPE_SIU_DATA,
    THR_TYPE_DTR_CMD,
    THR_TYPE_DTR_DATA,
    THR_TYPE_APS_WITS,
} thr_type_t;

//-----------------------------------------------
typedef enum thr_mode_t : uint8_t
{
    THR_DETACH = 1,
    THR_JOIN = 2
} thr_mode_t;

//-----------------------------------------------
///
/// Параметры для создания потока
///
typedef struct thr_prop_t
{
    thr_type_t type;
    thr_mode_t mode;
    thr_func_t start_func;
    thr_func_t stop_func;
    void *payload;

    // private:
    char name[32];
    int group_id;
    bool free_payload;
} thr_prop_t;


//-----------------------------------------------
typedef struct thr_t
{
    pthread_t pthread;
    pthread_attr_t attr;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    pthread_condattr_t condattr;
    thr_type_t type;
    thr_mode_t mode;
    bool shuttingdown;
    bool wakeup;
    int id;
    int group_id;
    //
    char name[32];
    thr_func_t start_func;
    thr_func_t stop_func;
    void *payload;
    bool free_payload;
} thr_t;

typedef struct thrs_t
{
    int total;
    thr_t *thrs[NTHREAD];
} thrs_t;

#endif // THR_STRUCT__H_
