/**
  \file   socket.h
  \author Aleksey Lekhtin
  \date   Feb 28, 2005
  \brief  Socket operations
*/

#ifndef SOCKET__H_
#define SOCKET__H_

#include <stdint.h>
#include <stdbool.h>

#include "defines.h"
#include "socket_t.h"
#include "wsa.h"

bool socket_open(socket_t *sock, int type, int proto);
void socket_close_fd(SOCKET *fd);
bool socket_bind(socket_t *sock);
bool socket_listen(socket_t *sock);
int  socket_accept(socket_t *sock);
int socket_recv(socket_t *sock, char *buf, int buflen);
int socket_send(socket_t *sock, const char *data, size_t datalen);
const char *socket_type(int type);
const char *socket_proto(int proto);

#endif // SOCKET__H_
