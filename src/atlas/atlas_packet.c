/**
  \file   atlas_packet.c
  \author Aleksey Lekhtin
  \date   Feb 01, 2024
  \brief  The Atlas packet procedures
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __linux__
#include <netinet/in.h>
#endif

#include "atlas.h"
#include "cfg.h"
#include "dbg.h"
#include "atlas_packet.h"
#include "core.h"
#include "log.h"

/*-------------------------------------------------------*/
static void make_rtp_part()
{
    atlas_packet_t *pkt = &atlas()->packet;

    const uint8_t ver = 2;
    const uint8_t pad = 0;
    const uint8_t ext = 1;
    const uint8_t cc = 1;
    const uint8_t mark = 0;
    const uint8_t ptype = 71;

    memset(&pkt->rtp.fixed_part, 0, sizeof(rtp_fixed_part_t));
    pkt->rtp_size = sizeof(rtp_fixed_part_t);

    // 1. make fixed part of RTP
    pkt->rtp.fixed_part.bits |= (ver << 14u);
    pkt->rtp.fixed_part.bits |= (pad << 13u);
    pkt->rtp.fixed_part.bits |= (ext << 12u);
    pkt->rtp.fixed_part.bits |= (cc << 8u);
    pkt->rtp.fixed_part.bits |= (mark << 7u);
    pkt->rtp.fixed_part.bits |= ptype;
    // print_int32_bin(pkt->rtp.fixed_part.bits);

    pkt->rtp.fixed_part.seqnum = 0;
    pkt->rtp.fixed_part.timerange = 0;
    pkt->rtp.fixed_part.SSRC = 846930886; // Случайное число сформированное при инициализации ББИ(БТО, МТОД)

    // 2. add list of CSRC identifiers
    pkt->rtp.CSRC_count = (pkt->rtp.fixed_part.bits >> 8u) & 0x0F;
    for (int i = 0; i < pkt->rtp.CSRC_count && i < 15; ++i) {
        pkt->rtp.CSRC[i] = 12569;      // 0x3119 - Младший байт в пакете Старшая часть слова
    }
    pkt->rtp_size += (pkt->rtp.CSRC_count * sizeof(pkt->rtp.CSRC[0]));

    // 3. add header extension
    //ext = (pkt->rtp.fixed_part.bits >> 12) & 0x01;
    if (ext) {
        // currently for example only ...
        int total_data = 2;
        pkt->rtp.ext_header.implementation = 27;
        pkt->rtp.ext_header.total_recs = total_data;
        pkt->rtp.ext_data = malloc(total_data * sizeof(uint32_t));
        for (int i = 0; i < total_data; ++i) {
            *(pkt->rtp.ext_data + i) = 0xa5ebaca + i;
        }
        pkt->rtp_size += sizeof(ext_hdr_t);
        pkt->rtp_size += pkt->rtp.ext_header.total_recs * sizeof(uint32_t);
    }
}

/*-------------------------------------------------------*/
static void make_data_header()
{
    atlas_packet_t *pkt = &atlas()->packet;
    memset(&pkt->data_header, 0, sizeof(data_hdr_t));
    pkt->data_header.data_size = atlas()->npnts500 * sizeof(uint32_t);
}

/*-------------------------------------------------------*/
void atlas_packet_free()
{
    DBG(8);
    if (atlas()->packet.rtp.ext_data) {
        free(atlas()->packet.rtp.ext_data);
    }
    if (atlas()->packet.bytes) {
        free(atlas()->packet.bytes);
    }
    memset(&atlas()->packet, 0, sizeof(atlas_packet_t));
}

/*-------------------------------------------------------*/
void atlas_packet_init()
{
    atlas_packet_free();
    make_rtp_part();
    make_data_header();
    atlas()->packet.nbytes = (int) atlas()->packet.rtp_size;
    atlas()->packet.nbytes += (int) sizeof(data_hdr_t);

    int total_channels = 2;  // канал с 'Давлением №1' + канал 'Вес' (вем пишется всегда последним)
    if (cfg()->atlas_proto_ver == 1) {
        total_channels = 3; // плюс канал с 'Давлением №2'
    }
    if (cfg()->atlas_proto_ver == 2) {
        total_channels = 4; // плюс запасной 4-ый канал
    }
    DBGF(1, "protocol version: %d", cfg()->atlas_proto_ver);
    DBGF(1, "channels: %d", total_channels);
    atlas()->packet.nbytes += (int) (atlas()->packet.data_header.data_size * total_channels);

    if (atlas()->packet.bytes) {
        free(atlas()->packet.bytes);
    }
    atlas()->packet.bytes = malloc(atlas()->packet.nbytes);
    if (!atlas()->packet.bytes) {
        LOG(FATAL, "can't allocate memory (%u bytes)", atlas()->packet.nbytes);
        DBGF(0, "\nOu Shit! ERROR: can't allocate memory (%u bytes)\n\n", atlas()->packet.nbytes);
        exit(0);
    }

    atlas()->samplenum = 1;
    atlas()->timerange = 0;
    atlas()->timerange = cfg()->atlas_timeshift;
}

/*-------------------------------------------------------*/
inline static void atlas_set_packet_attrb(uint32_t seq, uint32_t timerange)
{
    atlas_packet_t *pkt = &atlas()->packet;
    pkt->rtp.fixed_part.seqnum = seq;
    pkt->rtp.fixed_part.timerange = timerange;
}

/*-------------------------------------------------------*/
void atlas_make_binary_packet()
{
    atlas_packet_t *pkt = &atlas()->packet;
    char *p = atlas()->packet.bytes;

    // fill RTP space
    *(uint16_t *) p = htons(pkt->rtp.fixed_part.bits);
    p += sizeof(uint16_t);

    *(uint16_t *) p = htons(pkt->rtp.fixed_part.seqnum);
    p += sizeof(uint16_t);

    *(uint32_t *) p = htonl(pkt->rtp.fixed_part.timerange);
    p += sizeof(uint32_t);

    *(uint32_t *) p = htonl(pkt->rtp.fixed_part.SSRC);
    p += sizeof(uint32_t);

    for (int i = 0; i < pkt->rtp.CSRC_count && i < 15; ++i) {
        *(uint32_t *) p = htonl(pkt->rtp.CSRC[i]);
        p += sizeof(uint32_t);
    }

    if (pkt->rtp.ext_header.total_recs) {
        *(uint16_t *) p = htons(pkt->rtp.ext_header.implementation);
        p += sizeof(uint16_t);
        *(uint16_t *) p = htons(pkt->rtp.ext_header.total_recs);
        p += sizeof(uint16_t);
        for (int i = 0; i < pkt->rtp.ext_header.total_recs; ++i) {
            *(uint32_t *) p = htonl(pkt->rtp.ext_data[i]);
            p += sizeof(uint32_t);
        }
    }

    p = atlas()->packet.bytes + pkt->rtp_size;

    // fill payload space
    uint32_t proto_ver = cfg()->atlas_proto_ver;
    pkt->data_header.misc.bits.proto_ver |= (proto_ver & 0x03u);

    pkt->data_header.encoder1 = core()->data.encoder1;
    pkt->data_header.encoder2 = core()->data.encoder2;

    *(data_hdr_t *) p = pkt->data_header;
    p += sizeof(data_hdr_t);

    // pressure #1
    int index = 0;
    if (cfg()->swap_channels12) {
        index = 1;
    }
    for (int i = 0; i < atlas()->npnts500; i++) {
        *(int32_t *) p = core()->data.values[index].R[i];
        p += sizeof(int32_t);
    }

    // pressure #2
    index = 1;
    if (cfg()->swap_channels12) {
        index = 0;
    }
    if (proto_ver == 1 || proto_ver == 2) {
        for (int i = 0; i < atlas()->npnts500; i++) {
            *(int32_t *) p = core()->data.values[index].R[i];
            p += sizeof(int32_t);
        }
        ++index;
    }

    index = 2;
    if (proto_ver == 2) {
        for (int i = 0; i < atlas()->npnts500; i++) {
            *(int32_t *) p = core()->data.values[index].R[i];
            p += sizeof(int32_t);
        }
        ++index;
    }

    // hookload (always last in packet)
    for (int i = 0; i < atlas()->npnts500; i++) {
        *(int32_t *) p = core()->data.values[index].R[i];
        p += sizeof(int32_t);
    }

    atlas()->samplenum++;
    atlas()->timerange = cfg()->atlas_timeshift;
    atlas_set_packet_attrb(atlas()->samplenum, atlas()->timerange);

    DBGF(3, "protocol: %u (0x%02X), dpt delay: %d, swap channels12: %u, timerange: %u (0x%X)",
         pkt->data_header.misc.bits.proto_ver,
         pkt->data_header.misc.dword,
         cfg()->dpt_delay,
         cfg()->swap_channels12,
         pkt->rtp.fixed_part.timerange,
         pkt->rtp.fixed_part.timerange);

    atlas_dbg_print_binary_packet(atlas()->packet.bytes, atlas()->packet.nbytes);
}

/*-------------------------------------------------------*/
bool atlas_dbg_parse_binary_packet(atlas_packet_t *packet, const char *bytes, int nbytes)
{
    if (!packet || !bytes || nbytes == 0) {
        return FAIL;
    }
    const char *p = bytes;
    packet->rtp.fixed_part.bits = ntohs(*(uint16_t *) p);
    p += sizeof(uint16_t);

    packet->rtp.fixed_part.seqnum = ntohs(*(uint16_t *) p);
    p += sizeof(uint16_t);

    packet->rtp.fixed_part.timerange = ntohl(*(uint32_t *) p);
    p += sizeof(uint32_t);

    packet->rtp.fixed_part.SSRC = ntohl(*(uint32_t *) p);
    p += sizeof(uint32_t);

    packet->rtp.CSRC_count = (packet->rtp.fixed_part.bits >> 8) & 0x0F;
    for (int i = 0; i < packet->rtp.CSRC_count && i < 15; i++) {
        packet->rtp.CSRC[i] = ntohl(*(uint32_t *) p);
        p += sizeof(uint32_t);
    }

    const int ext = (packet->rtp.fixed_part.bits >> 12) & 0x01;
    if (ext) {
        packet->rtp.ext_header.implementation = ntohs(*(uint16_t *) p);
        p += sizeof(uint16_t);
        packet->rtp.ext_header.total_recs = ntohs(*(uint16_t *) p);
        p += sizeof(uint16_t);
        packet->rtp.ext_data = (uint32_t *) malloc(packet->rtp.ext_header.total_recs * sizeof(uint32_t));
        for (int i = 0; i < packet->rtp.ext_header.total_recs; i++) {
            *(packet->rtp.ext_data + i) = ntohl(*(uint32_t *) p);
            p += sizeof(uint32_t);
        }
    }

    packet->rtp_size = p - bytes;
    packet->data_header = *(data_hdr_t *) p;
    //p += sizeof(data_hdr_t);
    return OK;
}

/*-------------------------------------------------------*/
void atlas_dbg_print_binary_packet(const char *bytes, const int nbytes)
{
    if (debug_level < 6) {
        return;
    }

    if (!bytes || nbytes == 0) {
        return;
    }
    atlas_packet_t pkt = {0};
    atlas_dbg_parse_binary_packet(&pkt, bytes, nbytes);
    static char buf[32 * 1024] = {0};

    int ln = snprintf(buf, sizeof(buf),
                      "\nRTP header: 0x%04X %04X %08X %08X\n"
                      "    msg60:%u, pad:%u, ext:%u, CSRC count:%u, marker:%u, payload type:%u\n"
                      "    seq:%u, timerange:%u(0x%X), SSRC:%u(0x%08X)\n",
                      pkt.rtp.fixed_part.bits,
                      pkt.rtp.fixed_part.seqnum,
                      pkt.rtp.fixed_part.timerange,
                      pkt.rtp.fixed_part.SSRC,

                      (pkt.rtp.fixed_part.bits >> 14) & 0x03,
                      (pkt.rtp.fixed_part.bits >> 13) & 0x01,
                      (pkt.rtp.fixed_part.bits >> 12) & 0x01,
                      (pkt.rtp.fixed_part.bits >> 8) & 0x0F,
                      (pkt.rtp.fixed_part.bits >> 7) & 0x01,
                      (pkt.rtp.fixed_part.bits) & 0x7F,

                      pkt.rtp.fixed_part.seqnum,
                      pkt.rtp.fixed_part.timerange,
                      pkt.rtp.fixed_part.timerange,
                      pkt.rtp.fixed_part.SSRC,
                      pkt.rtp.fixed_part.SSRC);

    if (pkt.rtp.CSRC_count) {
        snprintf(buf + ln, sizeof(buf) - strlen(buf), "CSRC:");
        for (int i = 0; i < pkt.rtp.CSRC_count && i < 15; ++i) {
            ln = (int) strlen(buf);
            snprintf(buf + ln, sizeof(buf) - strlen(buf), "%u(0x%X) ", pkt.rtp.CSRC[i], pkt.rtp.CSRC[i]);
        }
    }

    const uint8_t ext = (pkt.rtp.fixed_part.bits >> 12) & 0x01;
    if (ext) {
        ln = (int) strlen(buf);
        snprintf(buf + ln, sizeof(buf) - strlen(buf), "\nEXT: impl: %d(0x%X), max_npnts: %d\n",
                 pkt.rtp.ext_header.implementation,
                 pkt.rtp.ext_header.implementation,
                 pkt.rtp.ext_header.total_recs);
        for (int i = 0; i < pkt.rtp.ext_header.total_recs; ++i) {
            ln = (int) strlen(buf);
            snprintf(buf + ln, sizeof(buf) - strlen(buf), "     [ %u (0x%08X) ]\n", pkt.rtp.ext_data[i],
                     pkt.rtp.ext_data[i]);
        }
    }
    ln = (int) strlen(buf);
    snprintf(buf + ln, sizeof(buf) - strlen(buf), "rtp size: %u\n", pkt.rtp_size);
    ln = (int) strlen(buf);

    snprintf(buf + ln, sizeof(buf) - strlen(buf), "encoder1:              %d\n", pkt.data_header.encoder1);
    ln = (int) strlen(buf);

    snprintf(buf + ln, sizeof(buf) - strlen(buf), "data block size:       %d\n", (int) pkt.data_header.data_size);
    ln = (int) strlen(buf);

    snprintf(buf + ln, sizeof(buf) - strlen(buf), "encoder2:              %d\n", pkt.data_header.encoder2);
    ln = (int) strlen(buf);

    if ((pkt.data_header.misc.bits.proto_ver & 0x03u) == 0) {
        snprintf(buf + ln, sizeof(buf) - strlen(buf), "rezerv2:               0x%08X", pkt.data_header.misc.dword);
        ln = (int) strlen(buf);
    } else {
        snprintf(buf + ln, sizeof(buf) - strlen(buf), "ver:                   0x%08X", pkt.data_header.misc.dword);
        ln = (int) strlen(buf);
        snprintf(buf + ln, sizeof(buf) - strlen(buf), " (proto ver: %u)",
                 (pkt.data_header.misc.bits.proto_ver & 0x03u));
    }

    snprintf(buf + ln, sizeof(buf) - strlen(buf), "\nrezerv3:               0x%08X\n", pkt.data_header.rezerv_3);
    ln = (int) strlen(buf);

    snprintf(buf + ln, sizeof(buf) - strlen(buf), "Total: %d", atlas()->packet.nbytes); // 4048
    DBGF(5, "\n%s", buf);

    if (debug_level < 6) {
        return;
    }
    DBG(6, "hexdump: ");
    for (int i = 0; i < atlas()->packet.nbytes;) {
        printf("%-4d : ", i);
        buf[0] = 0;
        char *p = buf;
        int n = 0;
        for (; n < 48; n++) {
            if (i + n >= atlas()->packet.nbytes) {
                break;
            }
            if (n > 0 && (n % 8) == 0) {
                *p = ' ';
                p++;
            }
            ln = snprintf(p, 4, "%02X", (uint8_t) atlas()->packet.bytes[i + n]);
            p += ln;

            if (i == atlas()->packet.nbytes) {
                break;
            }
        }
        i += n;
        printf("%s\n", buf);
    }
    fwrite("\n", 1, 1, stdout);

    for (int i = 0; i < 4; i++) {
        printf("Channel: %d  sum: 0x%06llX (%7lld),  avrg: 0x%08X (%10d)\n", i+1,
               core()->data.values[i].sum, core()->data.values[i].sum,
               core()->data.values[i].avrg, core()->data.values[i].avrg);
    }
    printf("Encoder-1:          %-5d\n", core()->data.encoder1);
    printf("Encoder-2:          %-5d\n", core()->data.encoder2);
    printf("HookloadA2D (ch:2): %-5d / %5d\n\n", core()->data.HKLD24bit, core()->data.HKLD16bit);
}
