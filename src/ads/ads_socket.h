/**
  \file   ads_socket.h
  \author Aleksey Lekhtin
  \date   Mar 10, 2025
  \brief  Socket abstract operation
*/

#ifndef APS_SOCKET__H_
#define APS_SOCKET__H_

#include <stdbool.h>

#include "socket.h"

bool ads_cmd_open_socket(void *obj);
void ads_cmd_close_socket(void *obj);
void ads_set_cmd_cli_socket(int fd);
int  ads_cmd_accept(void *obj);
int  ads_cmd_socket_send(socket_t *sock, const char *bytes, int nbytes);
int  ads_cmd_socket_recv(char *buf, int nbuf);

bool ads_data_open_socket(void *obj);
void ads_data_close_socket(void *obj);
int  ads_data_accept(void *obj);

#endif //APS_SOCKET__H_
