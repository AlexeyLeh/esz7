/**
  \file   thr.h
  \author Aleksey Lekhtin
  \date   Jul 26, 2024
  \brief  Threads
*/
#ifndef THR__H_
#define THR__H_

#include "thr_t.h"

/* Typical using
 *
 * int group_id = 0; // typical value - using for destroy bucket of thread
 *
 * THR_CREATE(thr, group_id, "thread_01", main_thread_func, stop_thread_func, &data_for_thread);
 * THR_RUN_DETACH(thr);
 * if (!thr) {
 *    return FAIL;
 * }
 *
 * THR_CREATE(thr, group_id, "thread_02", main_thread_func, stop_thread_func, &data_for_thread);
 * THR_JOIN(thr);
 * if (!thr) {
 *    return FAIL;
 * }
 *
 * note: вероятность того, что thr будет null крайне мала -
 * это может случиться только если система совсем сдохла.
 */

#define THR_PROP_START(_thr, _thr_prop) thr_t *_thr = thr_create(_thr_prop)
#define THR_STOP(t) thr_stop(t)
#define THR_STOP_GROUP(_group_id) thr_stop_group(_group_id)

#define THR_START(_thr, _mode, _func_start, _func_stop, _payload, _free_payload, _name) \
        thr_prop_t _prop = {                                                            \
            .mode = (_mode),                                                            \
            .start_func = (_func_start),                                                \
            .stop_func = (_func_stop),                                                  \
            .payload = (_payload),                                                      \
            .name = {},                                                                 \
            .free_payload = (_free_payload)                                             \
        };                                                                              \
        if (_name) {                                                                    \
            strlcpy((_prop).name, (_name), sizeof((_prop).name));                       \
        }                                                                               \
        thr_t *(_thr) = thr_create(&_prop)

#define THR_PROP_CLONE(_new_prop, _thr, _type, _func_start, _func_stop, _payload, _free_payload, _name) \
        thr_prop_t _new_prop = {                                                                        \
            .type = (_thr)->type,                                                                       \
            .mode = (_type),                                                                            \
            .start_func = (_func_start),                                                                \
            .stop_func = (_func_stop),                                                                  \
            .payload = (_payload),                                                                      \
            .name = {},                                                                                 \
            .group_id = (_thr)->group_id,                                                               \
            .free_payload = (_free_payload)                                                             \
        };                                                                                              \
        if (_name) {                                                                                    \
            strlcpy((_new_prop).name, (_name), sizeof((_new_prop).name));                               \
        }

thr_t *thr_create(const thr_prop_t *thr);
void thr_stop(thr_t *thr);
void thr_stop_all();
void thr_wait_all_done(uint32_t sec);
void thr_stop_group(int group_id);
void thr_wakeup(thr_t *thr);
void thr_wakeup_by_type(thr_type_t type);
void thr_asleep(thr_t *thr);

const char *thr_type_to_string(thr_type_t type);

#endif // THR__H_
