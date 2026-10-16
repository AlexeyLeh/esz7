/**
  \file   ads_packet.c
  \author Aleksey Lekhtin
  \date   Feb 25, 2025
  \brief  The ADS points procedures
*/

#include <string.h>
#include <stdio.h>

#ifdef __linux__

#include <netinet/in.h>
#include <malloc.h>
#include <sys/stat.h>
#include <assert.h>

#endif

#ifdef _WIN32
#include <winsock2.h>
#endif

#include "ads.h"
#include "cfg.h"
#include "dbg.h"
#include "core.h"
#include "utils.h"

//-------------------------------------------------------//
static void ads_packet_dbg_print()
{
    const ads_packet_t *pkt = &ads()->data.packet;
    DBGF(debug_level, "sign:%.3s, msg60:%u, sec:%u usec:%.3u, sensor: %u, size: %d",
         (const char *)pkt->header.sign,
         pkt->header.msg60,
         pkt->header.timestamp_sec,
         pkt->header.timestamp_usec,
         pkt->header.sensor,
         ads()->data.packet.nbytes);
    /*-------------------------------*/
    int i;
    printf("header:\n0   : ");
    uint8_t *ptr = (uint8_t *)ads()->data.packet.bytes;
    for (i = 0; i < 21; i++, ptr++) {
        if (i > 0 && (i % 32) == 0) {
            printf("\n%-3d : ", i);
        }
        printf("%02X ", *ptr);
    }
    //printf("\n%-3d\n", i);
    puts("");

    //printf("points:\n    : ");
    int n = i;
    while ((n % 32) > 0) {
        printf("   ");
        --n;
    }

    for (; i < 421; i++, ptr++) {
        if (i > 0 && (i % 32) == 0) {
            printf("\n%-3d : ", i);
        }
        printf("%02X ", *ptr);
    }
    printf("\n%-3d\n", i);
#if 0
    /*-------------------------------*/
        DBG(debug_level, "values dump:");
        printf("%-3d : ", 0);
        int i;
        for (i = 0; i < ads()->data.npnts100; i++) {
            if (i > 0 && (i % 20) == 0) {
                printf("\n%-3d : ", i);
            }
            printf("%06X ", ads()->data.points->values[PRESSURE][i]);
        }
        printf("\n%-3d\n", i);

    /*-------------------------------*/
    if (debug_level >= 5) {
        DBG(debug_level, "float values dump:");
        printf("%-3d : ", 0);
        int i;
        for (i = 0; i < ads()->data.npnts100; i++) {
            if (i > 0 && (i % 10) == 0) {
                printf("\n%-3d : ", i);
            }
            printf("%11.2f ", ads()->data.points->float_values[PRESSURE][i]);
        }
        printf("\n%-3d\n", i);
    }
#endif
}

///*---------------------------------------------------------*/
//static void dump_ads(const char *fname, const char *bytes, int nbytes)
//{
//    DBGF(3, "%s", fname);
//    int dump_text_packet_fd = open(fname, O_CREAT | O_TRUNC | O_WRONLY, 0640);
//    if (dump_text_packet_fd == -1) {
//        LOG(ERROR, "WARNING: can't create dump file '%s'", fname);
//        DBGF(0, "WARNING: can't create dump file '%s'", fname);
//        LOG(ERROR, "(%d) %s\n", errno, strerror(errno));
//        DBGF(0, "(%d) %s\n", errno, strerror(errno));
//        return;
//    }
//    write(dump_text_packet_fd, bytes, nbytes);
//    close(dump_text_packet_fd);
//}

//-------------------------------------------------------//
void ads_packet_init()
{
    ads()->data.packet.nbytes = (int)sizeof(ads()->data.packet.header);
    ads()->data.packet.nbytes += (int)(ads()->data.npnts100 * sizeof(uint32_t));
    ads()->data.packet.bytes = malloc(ads()->data.packet.nbytes);
    assert(ads()->data.packet.bytes);
}

//-------------------------------------------------------//
void ads_packet_free()
{
    if (ads()->data.packet.bytes) {
        free(ads()->data.packet.bytes);
    }
    ads()->data.packet.bytes = NULL;
    ads()->data.packet.nbytes = 0;
}

//-------------------------------------------------------//
void ads_packet_make_binary()
{
    DBG(9);
    core_data_t *points = &core()->data;
    ads_packet_t *pkt = &ads()->data.packet;
    char *p = ads()->data.packet.bytes;
    memcpy(p, pkt->header.sign, 3);
    p += 3;
    *(uint8_t *) p = pkt->header.msg60;
    p++;
    *(uint8_t *) p = pkt->header.sensor;
    p++;
    *(uint32_t *) p = htonl(pkt->header.timestamp_sec);
    p += sizeof(uint32_t);
    *(uint32_t *) p = htonl(pkt->header.timestamp_usec);
    p += sizeof(uint32_t);
    *(uint32_t *) p = htonl(pkt->header.filenum);
    p += sizeof(uint32_t);
    *(uint32_t *) p = htonl(pkt->header.samplenum);
    p += sizeof(uint32_t);
    uint32_t *intptr = (uint32_t *) p;
    for (int i = 0; i < ads()->data.npnts100; i++, intptr++) {
        float val = (float)points->values[0].R[i];
        int32_t *ptr = (int32_t *)&val;
        *intptr = htonl(*ptr);
    }
}

/*---------------------------------------------------------*/
void ads_data_create_packet()
{
    pthread_mutex_lock(&ads()->data.mutex);

    static struct timespec timestamp;
    static struct timespec timestamp_prev = {0};
    static char timebuf[TIMEBUFLEN];
    clock_get_local_time(&timestamp);
    if (timestamp_prev.tv_sec == 0) {
        timestamp_prev = timestamp;
    }
    struct timespec between_measurement;
    timespec_diff(&between_measurement, &timestamp_prev, &timestamp, NULL);
    timestamp_prev = timestamp;

    core_data_t *points = &core()->data;

    // 15 минут - 0.250 x 4 (блока по 100 точек) = 1сек.
    // 60 сек (т.е. 1 минута) => 60 x 4 = 240 блоков
    // 240 x 15 мин = 3600 блоков => 360000 точек
    // 360000 x 4 = 1440000 байт макс. файла размер допускается ADS!
    // Иначе ошибка - "Wacky Offset: "
    if (ads()->data.fileS1.samplenum > 360000) {
        ads()->data.fileS1.created = timestamp;
        ads()->data.fileS1.number++;
        ads()->data.fileS1.samplenum = 0;
    }

    ads()->data.fileS1.samplenum += ads()->data.npnts100;

    ads()->data.packet.header.sensor = ads()->data.fileS1.sensor;
    ads()->data.packet.header.filenum = ads()->data.fileS1.number;
    ads()->data.packet.header.samplenum = ads()->data.fileS1.samplenum;
    ads()->data.packet.header.timestamp_sec = ads()->data.fileS1.created.tv_sec;
    ads()->data.packet.header.timestamp_usec = ads()->data.fileS1.created.tv_nsec / 1000;

    struct timespec total_file_time;
    timespec_diff(&total_file_time, &ads()->data.fileS1.created, &timestamp, NULL);
    DBGF(3, "sensor:%d, file:%u, enc: %d/%d (elapsed:%s), sample:%u (%ld.%06ld)",
         ads()->data.packet.header.sensor,
         ads()->data.packet.header.filenum,
         points->encoder1,
         points->encoder1,
         timespec_to_str(&total_file_time, timebuf),
         ads()->data.packet.header.samplenum,
         between_measurement.tv_sec, between_measurement.tv_nsec / 1000);

    ads_packet_make_binary();

    if (debug_level == 6) {
        ads_packet_dbg_print();
    }
    pthread_mutex_unlock(&ads()->data.mutex);
}
