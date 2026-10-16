/**
  \file   aps_wits.h
  \author Aleksey Lekhtin
  \date   Oct 28, 2025
  \brief  Работа с wits
*/

#ifndef APS_WITS__H_
#define APS_WITS__H_

#include "aps_wits_t.h"

#include <stdbool.h>

aps_wits_t *aps_wits();
bool aps_wits_run();
int aps_wits_client_thread(thr_t *thr);
void aps_wits_data_send();
#endif //APS_WITS__H_
