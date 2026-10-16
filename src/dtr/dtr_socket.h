/**
  \file   dtr_socket.h
  \author Aleksey Lekhtin
  \date   Apr  9, 2025
  \brief  Socket abstract operation
*/

#ifndef DTR_SOCKET__H_
#define DTR_SOCKET__H_

#include <stdint.h>
#include <stdbool.h>

#include "socket.h"

const socket_t *dtr_cmd_socket();
const socket_t *dtr_data_socket();

bool dtr_cmd_open_socket(void *obj);
void dtr_cmd_close_socket(void *obj);
int dtr_cmd_accept(void *obj);
void dtr_cmd_cli_socket_close();
int dtr_cmd_socket_send(socket_t *sock, const char *bytes, int nbytes);
int dtr_cmd_socket_recv(char *buf, int nbuf);

bool dtr_data_open_socket(void *obj);
void dtr_data_close_socket(void *obj);
int dtr_data_accept(void *obj);
bool dtr_data_socket_send(const char *bytes, int nbytes);
int dtr_data_socket_recv(char *buf, int nbuf);

#endif //DTR_SOCKET__H_
