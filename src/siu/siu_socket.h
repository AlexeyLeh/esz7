/**
  \file   siu_socket.h
  \author Aleksey Lekhtin
  \date   Apr  9, 2025
  \brief  Socket abstract operation
*/

#ifndef SIU2_SOCKET__H_
#define SIU2_SOCKET__H_

#include <stdint.h>
#include <stdbool.h>

#include "socket.h"

bool siu_cmd_open_socket(void *obj);
void siu_cmd_close_socket(void *obj);
int siu_cmd_accept(void *obj);
int siu_cmd_socket_send(socket_t *sock, const char *bytes, int nbytes);

bool siu_data_open_socket(void *obj);
void siu_data_close_socket(void *obj);
int siu_data_accept(void *obj);

#endif //SIU2_SOCKET__H_
