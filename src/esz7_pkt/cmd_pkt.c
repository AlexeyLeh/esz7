/**
  \file   packet.c
  \author Aleksey Lekhtin
  \date   April 1, 2024
  \brief  The packet content
  */

#include <assert.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#ifdef __linux__

#include <ctype.h>
#include <netinet/in.h>

#endif

#ifdef _WIN32
#include <winsock.h>
#endif

#include "dbg.h"
#include "defines.h"
#include "cmd_pkt.h"
#include "log.h"

enum {
    PKT_HDR_SIZE = 64
};

//---------------------------------------------------------
static_assert(sizeof(cmd_pkt_hdr_t) == PKT_HDR_SIZE, "Wrong packet header size. "__FILE__":26");

//---------------------------------------------------------
/**
 * @brief Create copy of packet
 * @param packet - incoming packet
 * @return created copy of packet
 */
cmd_pkt_t *cmd_pkt_clone(const cmd_pkt_t *packet)
{
    uint32_t packetsize = packet->header.pckt_size;
    cmd_pkt_t *pkt = (cmd_pkt_t *) malloc(packetsize);
    assert(pkt != NULL);
    memcpy(pkt, packet, packetsize);
    return pkt;
}

//---------------------------------------------------------
void cmd_pkt_ntoh(cmd_pkt_t *packet)
{
    if (!packet) {
        return;
    }
    packet->header.hdr_size = ntohl(packet->header.hdr_size);
    packet->header.pckt_size = ntohl(packet->header.pckt_size);
    packet->header.data_len = ntohl(packet->header.data_len);
    packet->value = be64toh(packet->value);
}

//---------------------------------------------------------
void cmd_pkt_hton(cmd_pkt_t *packet)
{
    if (!packet) {
        return;
    }
    packet->header.hdr_size = htonl(packet->header.hdr_size);
    packet->header.pckt_size = htonl(packet->header.pckt_size);
    packet->header.data_len = htonl(packet->header.data_len);
    packet->value = htobe64(packet->value);
}

//---------------------------------------------------------
cmd_pkt_t *cmd_pkt_create(pckt_cmd_t cmd, const char *data, size_t datalen)
{
    bool ok = true;
    if (datalen > MAX_PCKT_SIZE) {
        printf("data is too huge (%d > %d) will be сuted", datalen, MAX_PCKT_SIZE);
        datalen = MAX_PCKT_SIZE;
        ok = false;
    }

    size_t packetsize = sizeof(cmd_pkt_t) + datalen;
    cmd_pkt_t *pckt = (cmd_pkt_t *) calloc(1, packetsize);
    pckt->cmd = cmd;
    pckt->header = (cmd_pkt_hdr_t) {
            .sign        = {},
            .ver         = PKT_VER,
            .hdr_size    = (uint32_t) sizeof(cmd_pkt_hdr_t),
            .data_len    = datalen,
            .pckt_size   = packetsize,
            .errcode     = (ok ? PKT_OK : PKT_WRONG),
            .reserv      = {},
            .mark        = '@',
            .zero        = 0,
    };
    memcpy(&pckt->header.sign, PKT_SIGN, sizeof(PKT_SIGN));

    for (int i = 0; i < (int) sizeof(pckt->header.reserv); i++) {
        pckt->header.reserv[i] = (i % 2 ? 0x77 : 0x55);
    }
    pckt->header.reserv[0] = 0xAA;

    if (datalen > 0) {
        char *data_ptr = (char *) pckt + sizeof(cmd_pkt_t);
        memcpy(data_ptr, data, datalen);
    }

    DBGF(4, "hdr_t: %d, stuff: %d, data: %d - total size of packet: %d (%d)",
         (int) sizeof(cmd_pkt_hdr_t),
         (int) (sizeof(cmd_pkt_t) - sizeof(cmd_pkt_hdr_t)),
         pckt->header.data_len,
         (int) (sizeof(cmd_pkt_t) + pckt->header.data_len),
         pckt->header.pckt_size);
    return pckt;
}

//---------------------------------------------------------
cmd_pkt_t *cmd_pkt_make_cmd(pckt_cmd_t cmd, const char *data, size_t datalen)
{
    DBG(4);
    cmd_pkt_t *packet = cmd_pkt_create(cmd, data, datalen);
    DBGF(4, "cmd: %u (%s)", packet->cmd, cmd_pkt_cmd_string(packet->cmd));
    return packet;
}

//---------------------------------------------------------
const char *cmd_pkt_data(const cmd_pkt_t *packet, uint32_t *datalen)
{
    assert(packet != NULL);
    uint32_t len = packet->header.data_len;
    if (datalen) {
        *datalen = len;
    }
    if (len == 0) {
        return NULL;
    }
    char *ptr = (char *) packet + sizeof(cmd_pkt_t);
    return ptr;
}

//---------------------------------------------------------
bool cmd_pkt_check(const char *bytes, size_t byteslen, char *errormsg, size_t errmsgsize)
{
    char buf[256] = {0};
    if (byteslen < (int) sizeof(cmd_pkt_hdr_t)) {
        snprintf(buf, sizeof(buf),
                 "BAD DATA - a less than the required header (size:%d < header:%d)",
                 byteslen, (int) sizeof(cmd_pkt_hdr_t));
        if (errormsg && errmsgsize > 0) {
            strncpy(errormsg, buf, errmsgsize - 1);
        }
        LOG(ERROR, "%s", buf);
        DBGF(0, "%s", buf);
        return FAIL;
    }

    cmd_pkt_t *packet = (cmd_pkt_t *) bytes;

    size_t hdr_size = packet->header.hdr_size;
    if (hdr_size != (int) sizeof(cmd_pkt_hdr_t)) {
        data_hexdump(bytes, byteslen);
        snprintf(buf, sizeof(buf),
                 "BAD PACKET - unexpected header size (got size:%d != header_t:%d)",
                 hdr_size, (int) sizeof(cmd_pkt_hdr_t));
        if (errormsg && errmsgsize > 0) {
            strncpy(errormsg, buf, errmsgsize - 1);
        }
        LOG(ERROR, "%s", buf);
        DBGF(0, "%s", buf);
        return FAIL;
    }

    size_t pkt_size = packet->header.pckt_size;
    if (pkt_size > byteslen) {
        data_hexdump(bytes, byteslen);
        snprintf(buf, sizeof(buf),
                 "BAD PACKET - size in header is greater than number of bytes (size:%d > bytes:%d)",
                 pkt_size, byteslen);
        if (errormsg && errmsgsize > 0) {
            strncpy(errormsg, buf, errmsgsize - 1);
        }
        LOG(ERROR, "%s", buf);
        DBGF(0, "%s", buf);
        return FAIL;
    }

    if (pkt_size < (int) sizeof(cmd_pkt_hdr_t)) {
        data_hexdump(bytes, byteslen);
        snprintf(buf, sizeof(buf),
                 "BAD PACKET - size in header a less than require (size:%d < header:%d)",
                 pkt_size, (int) sizeof(cmd_pkt_hdr_t));
        if (errormsg && errmsgsize > 0) {
            strncpy(errormsg, buf, errmsgsize - 1);
        }
        LOG(ERROR, "%s", buf);
        DBGF(0, "%s", buf);
        return FAIL;
    }

    size_t datalen;
    const char *data = cmd_pkt_data(packet, &datalen);

    if (((int) sizeof(cmd_pkt_t) + datalen) != byteslen) {
        data_hexdump(bytes, byteslen);
        snprintf(buf, sizeof(buf), "BAD PACKET (fix packet part:%d + data bytes:%d != packet size:%d)",
                 sizeof(cmd_pkt_t), datalen, byteslen);
        if (errormsg && errmsgsize > 0) {
            strncpy(errormsg, buf, errmsgsize - 1);
        }
        LOG(ERROR, "%s", buf);
        DBGF(0, "%s", buf);
        return FAIL;
    }

    if (data) {
        if (data != (char *) packet + (int) sizeof(cmd_pkt_t)) {
            data_hexdump(bytes, byteslen);
            snprintf(buf, sizeof(buf), "BAD PACKET (data pointer points to incorrect space)");
            if (errormsg && errmsgsize > 0) {
                strncpy(errormsg, buf, errmsgsize - 1);
            }
            DBGF(3, "%s", buf);
            return FAIL;
        }
    }

    DBGF(4, "OK (header: %d, bytes: %d, total size: %d)",
         (int) sizeof(cmd_pkt_hdr_t), datalen, pkt_size);
    return OK;
}

//---------------------------------------------------------
inline void *cmd_pkt_ptr(const cmd_pkt_t *packet)
{
    assert(packet != NULL);
    return (char *) packet;
}

//---------------------------------------------------------
inline const cmd_pkt_hdr_t *cmd_pkt_header(const cmd_pkt_t *packet)
{
    assert(packet != NULL);
    return (cmd_pkt_hdr_t *) packet;
}

//---------------------------------------------------------
void cmd_pkt_dbg_print(const cmd_pkt_t *packet)
{
    if (debug_level <= 4)
        return;

    size_t pktsize = packet->header.pckt_size;
    const char *pkt = cmd_pkt_ptr(packet);
    if (!pkt) {
        DBG(debug_level, "just no pkt");
        return;
    }

    data_hexdump((char *) packet, pktsize);

    const cmd_pkt_hdr_t *hdr = cmd_pkt_header(packet);
    DBGF(debug_level, "sign:     0x%X%X%X%X %c%c%c%c",
         hdr->sign[0], hdr->sign[1], hdr->sign[2], hdr->sign[3],
         hdr->sign[0], hdr->sign[1], hdr->sign[2], hdr->sign[3]);
    DBGF(debug_level, "ver:      0x%02X", hdr->ver);
    DBGF(debug_level, "hdr size: %d", ntohl(hdr->hdr_size));
    DBGF(debug_level, "data len: %d", ntohl(hdr->data_len));
    DBGF(debug_level, "pkt_size: %d", ntohl(hdr->pckt_size));
    DBGF(debug_level, "error:    %d", hdr->errcode);
    DBGF(debug_level, "mark:     %c", hdr->mark);
    DBGF(debug_level, "cmd:      %u (%s)", packet->cmd, cmd_pkt_cmd_string(packet->cmd));
    DBGF(debug_level, "value:    0x%016llX", packet->value);

    uint32_t datalen;
    const char *data = cmd_pkt_data(packet, &datalen);
    if (datalen > 0) {
        DBGF(debug_level, "data %d bytes: ", datalen);
        for (uint32_t i = 0; i < datalen; i++) {
            char ch = data[i];
            printf("%c", (isascii(ch) ? ch : '.'));
        }
        puts("");
    }
}

// //---------------------------------------------------------
// const char *packet_str_error(int err)
// {
//     if (err == PKT_OK)
//         return "success";
//     if (err == PKT_WRONG)
//         return "wrong format";
//     if (err == PKT_CHECK_ERROR)
//         return "check error";
//     if (err == PKT_SEND_ERROR)
//         return "send error";
//     if (err == PKT_RECV_ERROR)
//         return "receive error";
//     return "unknown error";
// }

//---------------------------------------------------------
const char *cmd_pkt_cmd_string(pckt_cmd_t cmd)
{
    if (cmd == CMD_NONE)
        return "NONE";
    if (cmd == CMD_MESG)
        return "MSG";
    if (cmd == CMD_PING)
        return "PING";
    if (cmd == CMD_PONG)
        return "PONG";
    if (cmd == CMD_GET_DPT_DELAY)
        return "GET_DPT_DELAY";
    if (cmd == CMD_SET_DPT_DELAY)
        return "SET_DPT_DELAY";
    if (cmd == CMD_GET_CH_SWAP)
        return "CMD_GET_CH_SWAP";
    if (cmd == CMD_SET_CH_SWAP)
        return "CMD_SET_CH_SWAP";
    if (cmd == CMD_READ_REG)
        return "READ-REG";
    if (cmd == CMD_WRITE_REG)
        return "WRITE-REG";
    return "UNK";
}

//---------------------------------------------------------
/// Just print data in hex form
/// @param[in] data - byte sequence
/// @param[in] datalen - length of sequence
///
void data_hexdump(const char *data, size_t datalen)
{
    if (debug_level < 5) {
        return;
    }

    if (!data) {
        DBGF(debug_level, "[%d] data: 0x%08x. skip", debug_level, (uintptr_t) data);
        return;
    }

    DBGF(debug_level, "%d bytes:", datalen);
    printf("0   : ");
    for (size_t i = 0; i < datalen; i++) {
        if (i > 0 && (i % 16) == 0) {
            printf("\n%-4d: ", i);
        }
        printf("%02X ", (unsigned char) data[i]);
    }
    puts("");
}
