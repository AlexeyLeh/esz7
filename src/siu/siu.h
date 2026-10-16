/**
  \file   siu.h
  \author Aleksey Lekhtin
  \date   Apr 8, 2025
  \brief  The SIU features
*/

#ifndef SIU2__H_
#define SIU2__H_

#include <stdbool.h>

#include "siu_t.h"

siu_t *siu();
bool siu_run();
int siu_cmd_client_thread(thr_t *thr);
int siu_data_client_thread(thr_t *thr);

const char *make_DSD_record();
void siu_data_create_packet();
void siu_packet_free();
void siu_data_send(int channel);

bool siu_read_data_status();
bool siu_save_data_status();

const char *mpt_type_to_string(mpt_encoding_t type);
mpt_encoding_t string_to_mpt(const char *str);

const char **siu_get_fields(const char *str, int *nfields);
void siu_cmd_response(const char *command, siu_vars_t *vars, int (*send_response)(socket_t *, const char *, int),
                      socket_t *sock);
const char *siu_cmd_brakets_comment(const char *str);

uint32_t siu_dev_status();
uint32_t siu_system_status();
void siu_update_dev_status(uint32_t status);
void siu_update_system_status_flags(uint32_t status);

#endif //SIU2__H_
