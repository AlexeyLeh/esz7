/**
  \file   dtr_packet_t.h
  \author Aleksey Lekhtin
  \date   Feb 25, 2024
  \brief  The Depth Tracker packet
*/

#ifndef DTR_PACKET_T__H_
#define DTR_PACKET_T__H_

#include <stdint.h>

typedef struct dtr_packet_t
{
    char *bytes;
    int memsz;
    int nbytes;
} __attribute__((packed, aligned(1))) dtr_packet_t;

#endif //DTR_PACKET_T__H_
