/**
  \file   ads_packet_t.h
  \author Aleksey Lekhtin
  \date   Feb 25, 2024
  \brief  The points structure
*/

#ifndef ADC_PACKET_T__H_
#define ADC_PACKET_T__H_

#include <stdint.h>

#include "core_t.h"

typedef struct ads_packet_hdr_t
{
    uint8_t sign[3];
    uint8_t msg60;
    uint8_t sensor;
    uint32_t timestamp_sec;
    uint32_t timestamp_usec;
    uint32_t filenum;           // номер архивного файла
    uint32_t samplenum;         // номер первого сэмпла (step:100)
} __attribute__((packed, aligned(1))) ads_packet_hdr_t;

typedef struct ads_packet_t
{
    ads_packet_hdr_t header;
    char *bytes;
    int nbytes;
} __attribute__((packed, aligned(1))) ads_packet_t;

#endif //ADC_PACKET_T__H_
