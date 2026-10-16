/**
  \file   packet_atlas.h
  \author Aleksey Lekhtin
  \date   Apr 20, 2024
  \brief  The Atlas packet procedures
*/

#ifndef PACKET_ATLAS__H_
#define PACKET_ATLAS__H_

#include "atlas_packet_t.h"

void atlas_packet_init();
void atlas_packet_free();

void atlas_make_binary_packet();
void atlas_dbg_print_binary_packet(const char *bytes, int nbytes);
bool atlas_dbg_parse_binary_packet(atlas_packet_t *packet, const char *bytes, int nbytes);

#endif
