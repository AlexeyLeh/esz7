/**
  \file   siu_packet_t.h
  \author Aleksey Lekhtin
  \date   Feb 25, 2024
  \brief  The SureShot procotocl structure
*/

#ifndef SIU2_PACKET_T__H_
#define SIU2_PACKET_T__H_

#include <stdint.h>

typedef struct siu_packet_t
{
    char *bytes;
    int memsz;
    int nbytes;
} __attribute__((packed, aligned(1))) siu_packet_t;

#endif //SIU2_PACKET_T__H_
