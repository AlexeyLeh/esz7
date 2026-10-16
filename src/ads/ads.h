/**
  \file   ads.h
  \author Aleksey Lekhtin
  \date   Feb 26, 2025
  \brief  The ADS feature
*/

#ifndef ADS__H_
#define ADS__H_

#include <stdbool.h>

#include "ads_t.h"

ads_t *ads();

bool ads_run();
int ads_cmd_client_thread(thr_t *thr);
int ads_data_client_thread(thr_t *thr);

void ads_data_send(int channel);
void ads_data_create_packet();
void aps_write_stat();

void ads_init_file();
void ads_new_file(int sensor);
void ads_reset_file(int sensor);

void ads_packet_init();
void ads_packet_free();

void ads_packet_make_binary();

bool dump_ads_open();
void dump_ads_close();

#endif //ADS__H_
