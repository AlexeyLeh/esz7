/**
  \file   cmd_pkt_clone.h
  \author Aleksey Lekhtin
  \date   Jan 27, 2025
  \brief  The packet content
  */
#ifndef ESZ7_PACKET__H_
#define ESZ7_PACKET__H_

#include <stdbool.h>

#include "cmd_pkt_t.h"

enum {
    MAX_PCKT_SIZE = 1024
};

#ifdef __cplusplus
extern "C" {
#endif

cmd_pkt_t *cmd_pkt_clone(const cmd_pkt_t *packet);
void cmd_pkt_ntoh(cmd_pkt_t *packet);
void cmd_pkt_hton(cmd_pkt_t *packet);
cmd_pkt_t *cmd_pkt_create(pckt_cmd_t cmd, const char *data, size_t datalen);
void *cmd_pkt_ptr(const cmd_pkt_t *packet);
const cmd_pkt_hdr_t *cmd_pkt_header(const cmd_pkt_t *packet);
bool cmd_pkt_check(const char *bytes, size_t byteslen, char *errormsg, size_t errmsgsize); // errormsg - maybe null
const char *cmd_pkt_data(const cmd_pkt_t *packet, uint32_t *datalen);
cmd_pkt_t *cmd_pkt_make_cmd(pckt_cmd_t cmd, const char *data, size_t datalen);
const char *cmd_pkt_cmd_string(pckt_cmd_t cmd);

void cmd_pkt_dbg_print(const cmd_pkt_t *packet);
void data_hexdump(const char *data, size_t datalen);

#ifdef __cplusplus
}
#endif


#endif // ESZ7_PACKET__H_
