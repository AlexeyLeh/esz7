/**
  \file   thr_callbacks.h
  \author Aleksey Lekhtin
  \date   Mar  5, 2026
  \brief  Типовые обработчики разных потоков (ads/siu/dtr/wits)
*/

#ifndef THR_CALLBACKS__H_
#define THR_CALLBACKS__H_

#include "thr_t.h"

int thread_start(thr_t *thr);
int thread_stop(thr_t *thr);

#endif //THR_CALLBACKS__H_
