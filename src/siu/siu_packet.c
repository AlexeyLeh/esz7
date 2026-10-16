/**
  \file   shureshot_packet.c
  \author Aleksey Lekhtin
  \date   Feb 26, 2025
  \brief  The SIU obj interface
*/

#include <malloc.h>
#include <string.h>

#include "dbg.h"
#include "siu.h"
#include "utils.h"
#include "dump_siu_text_packet.h"

//---------------------------------------------------------
const char *make_DSD_record()
{
    static char buf[256] = {0};
    snprintf(buf, sizeof(buf),
             "[#DSD:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%0.2f:%02d:%0.2f]",
             siu()->vars.DepthSystemData.BitDepth,
             siu()->vars.DepthSystemData.HoleDepth,
             siu()->vars.DepthSystemData.BlockPosition,
             siu()->vars.DepthSystemData.BlockSpeed,
             siu()->vars.DepthSystemData.Rop1s, // ignored in SureShot
             siu()->vars.DepthSystemData.Rop5s,
             siu()->vars.DepthSystemData.WeightOnBit,
             siu()->vars.DepthSystemData.Hookload,
             siu()->vars.DepthSystemData.DrillState,
             siu()->vars.DepthSystemData.KellyDownDepth);
    snprintf(buf, sizeof(buf), "[#DSD:100.00:100.00:100.00:100.00:100.00:100.00:100.00:100.00:00:100.00]");
    return buf;
}

//-------------------------------------------------------//
void siu_packet_free()
{
    if (siu()->packet.bytes) {
        free(siu()->packet.bytes);
    }
    siu()->packet.bytes = NULL;
    siu()->packet.nbytes = 0;
    siu()->packet.memsz = 0;
}

//-----------------------------------------------------------
void siu_data_create_packet()
{
    const char *rec = make_DSD_record();
    int reclen = (int) strlen(rec);
    siu()->packet.nbytes = reclen;
    if (siu()->packet.nbytes > siu()->packet.memsz) {
        siu()->packet.memsz = (siu()->packet.nbytes < 256 ? 256 : siu()->packet.nbytes * 2);
        siu()->packet.bytes = realloc(siu()->packet.bytes, siu()->packet.memsz);
    }
    memcpy(siu()->packet.bytes, rec, reclen);
    DBGF(2, "len:%d %s", siu()->packet.nbytes, siu()->packet.bytes);
    dump_text_packet(1, siu()->packet.bytes, siu()->packet.nbytes);
}
