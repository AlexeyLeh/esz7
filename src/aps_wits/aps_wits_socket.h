/**
  \file   aps_wits_socket.h
  \author Aleksey Lekhtin
  \date   2024/02/01
  \brief  Socket operations
*/

#ifndef APS_WITS_SOCKET__H_
#define APS_WITS_SOCKET__H_

#include <stdbool.h>

bool aps_wits_open_socket(void *obj);
void aps_wits_close_socket(void *obj);
int aps_wits_accept(void *obj);

#endif // APS_WITS_SOCKET__HAPS__
