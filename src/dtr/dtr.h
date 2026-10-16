/**
  \file   dtr.h
  \author Aleksey Lekhtin
  \date   Oct 27, 2025
  \brief  The Depth Tracking features
*/

#ifndef DTR__H_
#define DTR__H_

#include <stdbool.h>

#include "dtr_t.h"
#include "dtr_socket.h"

dtr_t *dtr();

bool dtr_run();
int dtr_cmd_client_thread(thr_t *thr);
int dtr_data_client_thread(thr_t *thr);
void dtr_data_send();
void dtr_data_create_packet();
void dtr_packet_free();

#endif //DTR__H_
