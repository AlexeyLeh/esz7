/**
  \file   wsa.h
  \author Aleksey Lekhtin
  \date   Jan 27, 2025
  \brief  WSA procedures
*/

#ifndef WSA__H_
#define WSA__H_

#include <stdbool.h>

#include "socket_t.h"

#ifdef __cplusplus
namespace WSA {
extern "C" {
#endif

bool wsa_startup();
void wsa_cleanup();

int set_socket_errno(socket_t *sock);
int socket_errno(socket_t *sock);
const char *socket_strerror(int err);

#ifdef __cplusplus
} // namespace WSA
} // extern "C"
#endif


#endif // WSA__H_
