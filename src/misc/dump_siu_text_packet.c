//
// Created by LAV on 04.08.2026.
//

#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "cfg.h"
#include "dbg.h"
#include "log.h"

//---------------------------------------------------------
static int dump_text_packet_fd[4];

//---------------------------------------------------------
void dump_text_packet(int mode, const char *data, int data_len)
{
    if (!cfg()->do_dump) {
        return;
    }

    static bool inited = NO;
    if (!inited) {
        inited = YES;
        memset(dump_text_packet_fd, -1, sizeof(dump_text_packet_fd));
    }

    static char *fname;
    if (mode == 1) {
        fname = "dump-siu-dsd.dump";
    } else if (mode == 2) {
        fname = "dump-dtr-dsd.dump";
    } else if (mode == 3) {
        fname = "dump-wits-rec01.dump";
    } else  {
        return;
    }

    if (dump_text_packet_fd[mode-1] == -1) {
        dump_text_packet_fd[mode-1] = open(fname, O_CREAT | O_TRUNC | O_WRONLY);
        if (dump_text_packet_fd[mode-1] == -1) {
            LOG(ERROR, "can't create file %s (%s)", fname, strerror(errno));
            DBGF(0, "ERROR: can't create file %s (%s)", fname, strerror(errno));
            return;
        }
    }
    DBGF(3, "%s", fname);
    lseek(dump_text_packet_fd[mode-1], 0, SEEK_SET);
    write(dump_text_packet_fd[mode-1], data, data_len);
    fsync(dump_text_packet_fd[mode-1]);
}

//---------------------------------------------------------
void dump_text_packet_close()
{
    size_t sz = sizeof(dump_text_packet_fd) / sizeof(dump_text_packet_fd[0]);
    for(size_t i =0; i < sz; i++) {
        if (dump_text_packet_fd[i] == -1) {
            continue;
        }
        close(dump_text_packet_fd[i]);
    }
}
