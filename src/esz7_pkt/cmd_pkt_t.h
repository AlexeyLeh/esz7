/**
  \file   cmd_pkt_t.h
  \author Aleksey Lekhtin
  \date   Jan 27, 2025
  \brief  The packet structure
*/

#ifndef ESZ7_PACKET_STRUCT__H_
#define ESZ7_PACKET_STRUCT__H_

#include <stdint.h>

#define PKT_SIGN "ESZ7CMD"
#define PKT_VER 0x01

typedef enum pckt_error_t : uint8_t
{
    PKT_OK = 0,
    PKT_WRONG,
    PKT_CHECK_ERROR,
    PKT_SEND_ERROR,
    PKT_RECV_ERROR,
} pckt_error_t;

typedef enum pckt_cmd_t : uint8_t
{
    CMD_NONE = 0,
    CMD_MESG,
    CMD_PING,
    CMD_PONG,
    CMD_GET_DPT_DELAY,
    CMD_SET_DPT_DELAY,
    CMD_GET_CH_SWAP,
    CMD_SET_CH_SWAP,
    CMD_READ_REG,
    CMD_WRITE_REG,
} pckt_cmd_t;

/**
 * Header of the packet being sent.
 */
typedef struct cmd_pkt_hdr_t
{
    uint8_t      sign[8];
    uint8_t      ver;
    uint32_t     hdr_size;       // должен совпадать с sizeof(esz7_pkt_hdr_t)
    uint32_t     data_len;       // размер данных, "снизу" esz7_pkt_t (если есть)
    uint32_t     pckt_size;      // сумма: hdr_size + data_len + (sizeof(esz7_pkt_t) - sizeof(esz7_pkt_hdr_t))
    pckt_error_t errcode;        // error code if data is not valid
    uint8_t      reserv[40];
    uint8_t      mark;
    uint8_t      zero;
} __attribute__((packed, aligned(1))) cmd_pkt_hdr_t;

#pragma pack(push, 1)
typedef struct  cmd_pkt_t
{
    cmd_pkt_hdr_t  header;
    pckt_cmd_t     cmd;
    uint64_t       value;
    uint32_t       reserv1;
    uint32_t       reserv2;
    uint32_t       reserv3;
    uint32_t       reserv4;
    // any data bytes followed by esz7_pkt_t with length in esz7_pkt_hdr_t::data_len
} cmd_pkt_t;
#pragma pack(pop)

#endif // ESZ7_PACKET_STRUCT__H_
