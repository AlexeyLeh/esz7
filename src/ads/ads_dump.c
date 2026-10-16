/**
  \file   ads_dump.c
  \author Aleksey Lekhtin
  \date   Mar  3, 2026
  \brief  The ADS dump feature
*/

#include "ads.h"

#include <defines.h>
#include <unistd.h>

//---------------------------------------------------------
 bool dump_ads_open()
{
    return FAIL;
#if 0
    //    if (m_ads.dump_ads_data_fd != -1) {
    //        return OK;
    //    }
    //
    //    static int dump_counter = 0;
    //    static char fn[1024] = {0};
    //    static const char *bf;
    //
    //    static const char *dirname = "dump";
    //    mkdir(dirname, 0777);
    //
    //    static struct timespec timestamp;
    //    clock_get_local_time(&timestamp);
    //    bf = timespec_to_gmt_yymmdd_hhmmss_nz(&timestamp, 6);
    //        char *ptr = strchr(bf, '.');
    //        if (ptr) {
    //            *ptr = '_';
    //        }
    //
    //        snprintf(fn, sizeof(fn), "%s/pckt-%03d-%s.dump", dirname, m_ads.data.packet.header.filenum, bf);
    //
    //    const char *fname = "dump_ads_data.dump";
    //        m_ads.dump_ads_data_fd = open(fname, O_CREAT | O_TRUNC | O_WRONLY, 0640);
    //        if (m_ads.dump_ads_data_fd == -1) {
    //            LOG(ERROR, "WARNING: can't create dump directory '%s'", fname);
    //            DBGF(0, "WARNING: can't create dump directory '%s'", fname);
    //            LOG(ERROR, "(%d) %s", errno, strerror(errno));
    //            DBGF(0, "(%d) %s", errno, strerror(errno));
    //            cfg()->do_dump = MODE_MAIN;
    //            return FAIL;
    //        }
#endif
}

//---------------------------------------------------------
void dump_ads_close()
{
    if (ads()->dump_ads_data_fd != -1) {
        close(ads()->dump_ads_data_fd);
        ads()->dump_ads_data_fd = -1;
    }
}
