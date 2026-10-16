/**
  \file   aps_wits_packet.c
  \author Aleksey Lekhtin
  \date   Feb 01, 2024
  \brief  The APS WITS packet makeing procedure
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "cfg.h"
#include "dbg.h"
#include "siu.h"
#include "utils.h"
#include "aps_wits.h"
#include "aps_wits_packet.h"
#include "dump_siu_text_packet.h"

//---------------------------------------------------------
void aps_wits_packet_init()
{
    aps_wits_packet_free();
    aps_wits()->packet.nbytes = 0;
    aps_wits()->recnum = 0;
    aps_wits()->seqnum = 0;
    aps_wits()->tm = clock_get_local_time(&aps_wits()->timestamp);
}

//---------------------------------------------------------
void aps_wits_packet_free()
{
    DBG(8);
    if (aps_wits()->packet.bytes) {
        free(aps_wits()->packet.bytes);
    }
    aps_wits()->packet.bytes = NULL;
    aps_wits()->packet.nbytes = 0;
    aps_wits()->packet.nmem = 0;
}

//---------------------------------------------------------
static int Record_01_GeneralTimeBased(char *msg, int msg_sz)
{
    // RECORD # 1 : GENERAL TIME-BASED
    // Record ID 1
    // Logical Record Type 151
    // Auto / Manual AUTOMATIC
    // Trigger [TIME] Transmit at a specified time interval (secs)
    // Data Source Data acquired in real-time and computed over the trigger interval;
    // record transmitted and computation reset when triggering interval occurs.
    //
    //  Item Description        Long       Short   Type Length Metric FPS
    //                          Mnemonic   Mnemonic            Units  Units
    // 1    Well Identifier     WELLID     WELLID      A   16     ----   ----
    // 2    Sidetrack/Hole      STKNUM     SKNO     S   2      ----   ----
    //      Sect No.
    // 3    Record Identifier   RECID      RID      S   2      ----   ----
    // 4    Sequence Identifier SEQID      SQID     L   4      ----   ----
    // 5    Date                DATE       DATE     L   4      ----   ----
    // 6    Time                TIME       TIME     L   4      ----   ----
    // 7    Activity Code       ACTCOD     ACTC     S   2      ----   ----
    // 8    Depth Bit (meas)    DEPTBITM   DBTM     F   4      M      F
    // 9    Depth Bit (vert)    DEPTBITV   DBTV     F   4      M      F
    // 10   Depth Hole (meas)   DEPTMEAS   DMEA     F   4      M      F
    // 11   Depth Hole (vert)   DEPTVERT   DVER     F   4      M      F
    // 12   Block Position      BLKPOS     BPOS     F   4      M      F
    // 13   Rate of Penetration ROPA       ROPA     F   4      M/HR   F/HR
    //      (avg)
    // 14   Hookload (avg)      HKLA       HKLA     F   4      KDN    KLB
    // 16   Weight-on-Bit       WOBA       WOBA     F   4      KDN    KLB
    //      (surf, avg)
    // 21   Standpipe Pressure  SPPA       SPPA     F   4      KPA    PSI
    //      (avg)

    struct tm *tm = &aps_wits()->tm;
    const char *WELLID = "ESZ7";
    uint16_t STKNUM = 0;
    uint16_t RECID = aps_wits()->recnum;
    uint32_t SEQID = aps_wits()->seqnum;
    float    DEPTBITM = siu()->vars.DepthSystemData.BitDepth;
    float    DEPTBITV = siu()->vars.DepthSystemData.BitDepth;
    float    DEPTMEAS = siu()->vars.DepthSystemData.BitDepth;
    float    DEPTVERT = siu()->vars.DepthSystemData.BitDepth;
    float    BLKPOS = siu()->vars.DepthSystemData.BlockPosition;
    float    ROPA = siu()->vars.DepthSystemData.BlockSpeed;
    float    HKLA = siu()->vars.DepthSystemData.Hookload;
    float    WOBA = siu()->vars.DepthSystemData.WeightOnBit;
    float    SPPA = siu()->vars.DepthSystemData.Rop5s;

    int len = snprintf(msg, msg_sz,
                       "&&\n"
                       "0101%s\n"
                       "0102%u\n"
                       "0103%u\n"
                       "0104%u\n"
                       "0105%02d%02d%02d\n"
                       "0106%02d%02d%02d\n"
                       "0107%u\n"
                       "0108%0.2f\n"
                       "0109%0.2f\n"
                       "0110%0.2f\n"
                       "0111%0.2f\n"
                       "0112%0.2f\n"
                       "0113%0.2f\n"
                       "0114%0.2f\n"
                       "0116%0.2f\n"
                       "0121%0.2f\n"
                       "!!\n",
                       WELLID,
                       STKNUM,
                       RECID,
                       SEQID,
                       tm->tm_year, tm->tm_mon+1, tm->tm_mday,
                       tm->tm_hour, tm->tm_min, tm->tm_sec,
                       aps_wits()->ACTCOD,
                       DEPTBITM,
                       DEPTBITV,
                       DEPTMEAS,
                       DEPTVERT,
                       BLKPOS,
                       ROPA,
                       HKLA,
                       WOBA,
                       SPPA);
    replace_dot_to_comma(cfg()->decimal, msg);
    return len;
}

//---------------------------------------------------------
__attribute__((unused))
static int Record_02_Drilling_Depth_Based(char *msg, int msg_sz)
{
    // RECORD # 2 : DRILLING DEPTH-BASED
    // Record ID 2
    // Logical Record Type 152
    // Auto / Manual AUTOMATIC
    // Trigger [DEPTH] Transmit at a specified depth interval (feet or meters)
    // Data Source Data acquired in real-time when on-bottom drilling NEW hole and computed over the trigger interval.
    //
    //  Item Description            Long        Short   Type Length Metric FPS
    //                              Mnemonic    Mnemonic            Units  Units
    // 1 Well Identifier            WELLID      WID      A   16     ----   ----
    // 2 Sidetrack/Hole Sect No.    STKNUM      SKNO     S    2     ----   ----
    // 3 Record Identifier          RECID       RID      S    2     ----   ----
    // 4 Sequence Identifier        SEQID       SQID     L    4     ----   ----
    // 5 Date                       DATE        DATE     L    4     ----   ----
    // 6 Time                       TIME        TIME     L    4     ----   ----
    // 7 Activity Code              ACTCOD      ACTC     S    2     ----   ----
    // 8 Depth Hole (meas)          DEPTMEAS    DMEA     F    4     M      F
    // 9 Depth Hole (vert)          DEPTVERT    DVER     F    4     M      F
    // 10 Rate of Penetration (avg) ROPA        ROPA     F    4     M/HR   F/HR
    // 11 Weight-on-Bit (surf, avg) WOBA        WOBA     F    4     KDN    KLB
    // 12 Hookload (avg)            HKLA        HKLA     F    4     KDN    KLB
    // 13 Standpipe Pressure (avg)  SPPA        SPPA     F    4     KPA    PSI
    // 28 Block position            BLKPOS      BLKP     F    4     M      F

    struct tm *tm = &aps_wits()->tm;

    const char *WELLID = "ESZ7";
    uint16_t STKNUM = 0;
    uint16_t RECID = aps_wits()->recnum;
    uint32_t SEQID = aps_wits()->seqnum;
    float DEPTMEAS = 129.0f;
    float DEPTVERT = 130.0f;
    float BLKPOS = 15.0f;
    float ROPA = 60.0f;
    float HKLA = 30.0f;
    float WOBA = 20.0f;
    float SPPA = 150.0f;

    int len = snprintf(msg, msg_sz,
                       "&&\n"
                       "0201%s\n"
                       "0202%u\n"
                       "0203%u\n"
                       "0204%u\n"
                       "0205%02d%02d%02d\n"
                       "0206%02d%02d%02d\n"
                       "0207%u\n"
                       "0208%0.2f\n"
                       "0209%0.2f\n"
                       "0210%0.2f\n"
                       "0211%0.2f\n"
                       "0212%0.2f\n"
                       "0213%0.2f\n"
                       "0228%0.2f\n"
                       "!!\n",
                       WELLID,
                       STKNUM,
                       RECID,
                       SEQID,
                       tm->tm_year, tm->tm_mon+1, tm->tm_mday,
                       tm->tm_hour, tm->tm_min, tm->tm_sec,
                       aps_wits()->ACTCOD,

                       DEPTMEAS,
                       DEPTVERT,
                       ROPA,
                       WOBA,
                       HKLA,
                       SPPA,
                       BLKPOS);
    replace_dot_to_comma(cfg()->decimal, msg);
    return len;
}


//---------------------------------------------------------
__attribute__((unused))
static int Record_08_MWD_FormationEvaluation(char *msg, int msg_sz)
{
    // RECORD # 8 : MWD FORMATION EVALUATION
    // Record ID 8
    // Logical Record Type 158
    // Auto / Manual AUTO(MWD)/MANUAL
    // Trigger [TIME], [DEPTH], [TIME/DEPTH], [AUTO], or [MANUAL] Transmit at:
    // specified time interval (secs), depth interval (feet or meters) based on hole depth
    // when drilling new hole or making a new pass, a combination of these,
    // when new data values are received, or when manually triggered by operator.
    // Data Source Data acquired in real-time and computed over the trigger interval.
    // Record transmitted and computations reset when triggering interval occurs.
    // If DEPTH-triggered, then only on-bottom data is included in computations.
    // Depths of MWD sensors transmitted in the record are the depths for the last valid reading for that sensor.
    // Record is not sent if not circulating or tool is not operating.
    //
    //  Item Description               Long       Short   Type Length Metric FPS
    //                                 Mnemonic   Mnemonic            Units  Units
    // 1 Well Identifier               WELLID     WID      A  16      ----   ----
    // 2 Sidetrack/Hole Sect No.       STKNUM     SKNO     S   2      ----   ----
    // 3 Record Identifier             RECID      RID      S   2      ----   ----
    // 4 Sequence Identifier           SEQID      SQID     L   4      ----   ----
    // 5 Date                          DATE       DATE     L   4      ----   ----
    // 6 Time                          TIME       TIME     L   4      ----   ----
    // 7 Activity Code                 ACTCOD     ACTC     S   2      ----   ----
    // 8 Depth Hole (meas)             DEPTMEAS   DMEA     F   4      M      F
    // 9 Depth Hole (vert)             DEPTVERT   DVER     F   4      M      F
    // 10 Depth Bit (meas)             DEPTBITM   DBTM     F   4      M      F
    // 11 Depth Bit (vert)             DEPTBITV   DBTV     F   4      M      F
    // 12 Pass Number                  PASSNUM    PASS     S   2      ----   ----
    // 13 Depth Resis 1 sensor(meas)   DEPTRS1M   DR1M     F   4      M      F
    // 14 Depth Resis 1 sensor(vert)   DEPTRS1V   DR1V     F   4      M      F
    // 15 Resis 1 reading              MR1        MR1      F   4      OHMM   OHMM
    // 16 Resis 1(borehole corr)       MR1C       MR1C     F   4      OHMM   OHMM
    // 17 Depth Resis 2 sensor(meas)   DEPTRS2M   DR2M     F   4      M      F
    // 18 Depth Resis 2 sensor(vert)   DEPTRS2V   DR2V     F   4      M      F
    // 19 Resis 2 reading              MR2        MR2      F   4      OHMM   OHMM
    // 20 Resis 2(borehole corr)       MR2C       MR2C     F   4      OHMM   OHMM
    // 21 Depth G. Ray 1sensor(meas)   DEPTGR1M   DG1M     F   4      M      F
    // 22 Depth G. Ray 1sensor(vert)   DEPTGR1V   DG1V     F   4      M      F
    // 23 Gamma Ray 1 reading          MG1        MG1      F   4      API    API
    // 24 Gamma Ray 1 (borehole corr)  MG1C       MG1C     F   4      API    API
    // 48 Gamma Approved (0/1)         GAPRVD     GAPRVD   S   1      ----   ----
    // 49 WPR Approved (0/1)           WAPRVD     WAPRVD   S   1      ----   ----

    struct tm *tm = &aps_wits()->tm;

    const char *WELLID = "ESZ7";
    uint16_t STKNUM = 0;
    uint16_t RECID = aps_wits()->recnum;
    uint32_t SEQID = aps_wits()->seqnum;
    float DEPTMEAS = 129.0f;
    float DEPTVERT = 130.0f;
    float DEPTBITM = 25.0f;
    float DEPTBITV = 26.0f;
    uint16_t PASSNUM = 0;
    float DEPTRS1M = 123.0f;
    float DEPTRS1V  = 123.0f;
    float MR1  = 123.0f;
    float MR1C  = 123.0f;
    float DEPTRS2M  = 123.0f;
    float DEPTRS2V  = 123.0f;
    float MR2  = 123.0f;
    float MR2C  = 123.0f;
    float DEPTGR1M  = 123.0f;
    float DEPTGR1V  = 123.0f;
    float MG1  = 123.0f;
    float MG1C  = 123.0f;
    uint16_t GAPRVD = 0;
    uint16_t WAPRVD = 0;

    int len = snprintf(msg, msg_sz,
                       "&&\n"
                       "0801%s\n"
                       "0802%u\n"
                       "0803%u\n"
                       "0804%u\n"
                       "0805%02d%02d%02d\n"
                       "0806%02d%02d%02d\n"
                       "0807%u\n"
                       "0808%0.2f\n"
                       "0809%0.2f\n"
                       "0810%0.2f\n"
                       "0811%0.2f\n"
                       "0812%u\n"
                       "0813%0.2f\n"
                       "0814%0.2f\n"
                       "0815%0.2f\n"
                       "0816%0.2f\n"
                       "0817%0.2f\n"
                       "0818%0.2f\n"
                       "0819%0.2f\n"
                       "0820%0.2f\n"
                       "0821%0.2f\n"
                       "0822%0.2f\n"
                       "0823%0.2f\n"
                       "0824%0.2f\n"
                       "0848%u\n"
                       "0849%u\n"
                       "!!\n",
                       WELLID,
                       STKNUM,
                       RECID,
                       SEQID,
                       tm->tm_year, tm->tm_mon+1, tm->tm_mday,
                       tm->tm_hour, tm->tm_min, tm->tm_sec,
                       aps_wits()->ACTCOD,
                       DEPTMEAS,
                       DEPTVERT,
                       DEPTBITM,
                       DEPTBITV,
                       PASSNUM,
                       DEPTRS1M,
                       DEPTRS1V,
                       MR1,
                       MR1C,
                       DEPTRS2M,
                       DEPTRS2V,
                       MR2,
                       MR2C,
                       DEPTGR1M,
                       DEPTGR1V,
                       MG1,
                       MG1C,
                       GAPRVD,
                       WAPRVD);
    replace_dot_to_comma(cfg()->decimal, msg);
    return len;
}

//---------------------------------------------------------
void aps_wits_create_packet()
{
    DBG(3);
    aps_wits()->tm = clock_get_local_time(&aps_wits()->timestamp);
    aps_wits()->recnum++;
    aps_wits()->seqnum = 1;
    aps_wits()->ACTCOD = 2;

    //---------------------------------
    char msg[1][256];
    int msglen[1] = {0};
    int nmgs = sizeof(msg)/sizeof(msg[1]);

    msglen[0] = Record_01_GeneralTimeBased(msg[0], sizeof(msg[0]));

    aps_wits()->packet.nbytes = 0;
    for(int i = 0; i < nmgs; i++) {
        aps_wits()->packet.nbytes += msglen[i];
    }

    //---------------------------------
    if (aps_wits()->packet.nbytes > aps_wits()->packet.nmem) {
        aps_wits()->packet.nmem = aps_wits()->packet.nbytes;
        aps_wits()->packet.bytes = realloc(aps_wits()->packet.bytes, aps_wits()->packet.nmem);
    }

    //---------------------------------
    size_t off = 0;
    for(int i = 0; i < nmgs; i++) {
        memcpy(aps_wits()->packet.bytes + off, msg, msglen[i]);
        off += msglen[i];
    }
    DBGF(3, "len:%d %s", aps_wits()->packet.nbytes, aps_wits()->packet.bytes);
    dump_text_packet(3, aps_wits()->packet.bytes, aps_wits()->packet.nbytes);

    //---------------------------------
    if (debug_level == 3) {
        char *str = strdup(aps_wits()->packet.bytes);
        replace(str, '\n', ' ');
        if (strlen(str) > 25) {
            strcpy(str + 20, "...");
            str[23] = '\0';
        }
        DBGF(debug_level, "%s", str);
        free(str);
    }
}
