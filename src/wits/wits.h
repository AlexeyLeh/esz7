//
// Created by LAV on 25.08.2026.
//

#ifndef ATLAS_WITS_H
#define ATLAS_WITS_H

#include "wits_t.h"

wits_t *wits();
bool wits_run();
void wits_packet_free();
void wits_packet_init();
bool wits_open_socket(socket_t *sock);
int wits_accept(socket_t *sock);
void wits_create_packet();
void wits_send_data();

int wits_thread_start(thr_t *thr);
int wits_thread_stop(thr_t *thr);

#endif // ATLAS_WITS_H
