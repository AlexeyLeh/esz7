/**
  \file   packet_atlas.h
  \author Aleksey Lekhtin
  \date   Feb 01, 2024
  \brief  The packet structure
*/

#ifndef PACKET_ATLAS_T__H_
#define PACKET_ATLAS_T__H_

#include <stdint.h>

/**
 * RFC3550 RTP: A Transport Protocol for Real-Time Applications
 * @ref https://datatracker.ietf.org/doc/html/rfc3550
 */
typedef struct __attribute__((packed, aligned(1))) rtp_hdr_t
{
    // The first octet
    uint16_t bits;                  // ver/pad/ext/cc/mark/payload type
    uint16_t seqnum;                // sequence number
    // The second octet
    uint32_t timerange;             // ms
    // The third octet
    uint32_t SSRC;                  // synchronization source
} rtp_fixed_part_t;

typedef struct __attribute__((packed, aligned(2))) ext_hdr_t
{
    uint16_t implementation: 16;
    uint16_t total_recs: 16;
} ext_hdr_t;

typedef struct rtp_t
{
    rtp_fixed_part_t fixed_part;
    int CSRC_count;
    uint32_t CSRC[15];
    ext_hdr_t ext_header;
    uint32_t *ext_data;
} rtp_t;

typedef union __attribute__((packed, aligned(4))) misc_t {
    struct {
        uint32_t proto_ver:2;
        uint32_t unused:30;
    } bits;
    uint32_t dword;
} misc_t;

typedef struct __attribute__((packed, aligned(4))) data_header_t
{
    int32_t encoder1;
    uint32_t data_size; // 500 points x sizeof(uint32_t)
    uint32_t encoder2;  // former rezerv_1
    misc_t misc;        // former rezerv_2
    uint32_t rezerv_3;
} data_hdr_t;

/**
 * Данные для формирования пакета RTP
 */
typedef struct atlas_packet_t
{
    rtp_t rtp;
    uint32_t rtp_size;
    data_hdr_t data_header;
    char *bytes; // pressure1 pressure2 hookload
    int nbytes;
} atlas_packet_t;

#endif // PACKET_ATLAS_T__H_
