/*!
  \file   sram.c
  \author Aleksey Lekhtin
  \date   15.07.2024
  \brief  Работа с памятью, хранящей данные с АЦП
 */


#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include "defines.h"
#include "dbg.h"
#include "sram.h"
#include "utils.h"
#include "log.h"

static volatile CtrlMem_t *CtrlMem = ((volatile CtrlMem_t *) (ESZ7_ADDR + SRAM_ADCDEV_ADDR));
static volatile StatMem_t *StatMem = ((volatile StatMem_t *) (ESZ7_ADDR + SRAM_STATREG_ADDR));
static volatile LenMem_t *Mem0Len = ((volatile LenMem_t *) (ESZ7_ADDR + SRAM_BUFLEN0_ADDR));
static volatile LenMem_t *Mem1Len = ((volatile LenMem_t *) (ESZ7_ADDR + SRAM_BUFLEN1_ADDR));
static const int m_sram_addr[4] = {SRAM_ADDR0, SRAM_ADDR1, SRAM_ADDR2, SRAM_ADDR3};

static sram_t m_sram = {
        .numChannels = sizeof(m_sram.mem) / sizeof(m_sram.mem[0]),
        .max_recs = SRAM_MAX_RECS,
        .mem_size = sizeof(sram_mem_t),
        .mem[0] = (sram_mem_t *) m_sram_addr[0],
        .mem[1] = (sram_mem_t *) m_sram_addr[1],
        .mem[2] = (sram_mem_t *) m_sram_addr[2],
        .mem[3] = (sram_mem_t *) m_sram_addr[3]
};

//---------------------------------------------------------
static void sram_read_stat_reg(StatMem_t *stat)
{
    *stat = *StatMem;
}

//---------------------------------------------------------
static void sram_write_ctrl_reg(const CtrlMem_t *ctrl)
{
    *CtrlMem = *ctrl;
}

//---------------------------------------------------------
inline sram_t *sram()
{
    return &m_sram;
}

//---------------------------------------------------------
inline int sram_total_channels()
{
    return m_sram.numChannels;
}

//---------------------------------------------------------
inline int sram_mem_size()
{
    return m_sram.mem_size;
}

//---------------------------------------------------------
/// channel: [1..4]
inline sram_mem_t *sram_mem(int channel)
{
    int idx = channel - 1;
    return m_sram.mem[idx];
}

//---------------------------------------------------------
void sram_mem_clear()
{
    for (int i = 0; i < sram_total_channels(); i++) {
        DBGF(3, "clear mem::%p of %d (0x%X) bytes", m_sram.mem[i], sizeof(sram_mem_t), sizeof(sram_mem_t));
        memset((void *) m_sram.mem[i], 0, sizeof(sram_mem_t));
    }
}

//---------------------------------------------------------
void setup_sram_reg_addr(const mem_cfg_t *mem)
{
    CtrlMem = (volatile CtrlMem_t *) (mem->reg_mem_map + SRAM_ADCDEV_ADDR);
    StatMem = (volatile StatMem_t *) (mem->reg_mem_map + SRAM_STATREG_ADDR);
    Mem0Len = ((volatile LenMem_t *) (mem->reg_mem_map + SRAM_BUFLEN0_ADDR));
    Mem1Len = ((volatile LenMem_t *) (mem->reg_mem_map + SRAM_BUFLEN1_ADDR));

    m_sram.mem[0] = (sram_mem_t *) mem->sram_map[0];
    m_sram.mem[1] = (sram_mem_t *) mem->sram_map[1];
    m_sram.mem[2] = (sram_mem_t *) mem->sram_map[2];
    m_sram.mem[3] = (sram_mem_t *) mem->sram_map[3];
    DBGF(2, "SRAM Control:    %p (0x%02x)", CtrlMem, SRAM_ADCDEV_ADDR);
    DBGF(2, "SRAM Status:     %p (0x%02x)", StatMem, SRAM_STATREG_ADDR);
    DBGF(2, "SRAM memory:     %p %p %p %p", m_sram.mem[0], m_sram.mem[1], m_sram.mem[2], m_sram.mem[3]);
}

//---------------------------------------------------------
void sram_clear_reg_addr()
{
    CtrlMem = NULL;
    StatMem = NULL;
    Mem0Len = NULL;
    Mem1Len = NULL;
    m_sram.mem[0] = NULL;
    m_sram.mem[1] = NULL;
    m_sram.mem[2] = NULL;
    m_sram.mem[3] = NULL;
}

//---------------------------------------------------------
void sram_status_get_status(SramStat_t *sram_stat)
{
    sram_stat->stat = *StatMem;
    LenMem_t len0 = *Mem0Len;
    sram_stat->BufLen0 = len0.GivenBufLenEven;
    sram_stat->BufLen1 = len0.GivenBufLenOdd;
    LenMem_t len1 = *Mem1Len;
    sram_stat->BufLen2 = len1.GivenBufLenEven;
    sram_stat->BufLen3 = len1.GivenBufLenOdd;
}

//---------------------------------------------------------
int sram_status_get_len(int channel, const SramStat_t *sram_stat)
{
    if (channel == 1) {
        return sram_stat->BufLen0;
    }
    if (channel == 2) {
        return sram_stat->BufLen1;
    }
    if (channel == 3) {
        return sram_stat->BufLen2;
    }
    if (channel == 4) {
        return sram_stat->BufLen3;
    }
    return 0;
}

//---------------------------------------------------------
int sram_status_get_memEn(int channel, const SramStat_t *sram_stat)
{
    if (channel == 1) {
        return sram_stat->stat.MemEn0;
    }
    if (channel == 2) {
        return sram_stat->stat.MemEn1;
    }
    if (channel == 3) {
        return sram_stat->stat.MemEn2;
    }
    if (channel == 4) {
        return sram_stat->stat.MemEn3;
    }
    return 0;
}

//---------------------------------------------------------
int sram_status_get_bundle_ready(int channel, const SramStat_t *sram_stat)
{
    if (channel == 1) {
        if (sram_stat->stat.Bundl0Rdy0)
            return 0;
        if (sram_stat->stat.Bundl1Rdy0)
            return 1;
        return -1;
    }
    if (channel == 2) {
        if (sram_stat->stat.Bundl0Rdy1)
            return 0;
        if (sram_stat->stat.Bundl1Rdy1)
            return 1;
        return -1;
    }
    if (channel == 3) {
        if (sram_stat->stat.Bundl0Rdy2)
            return 0;
        if (sram_stat->stat.Bundl1Rdy2)
            return 1;
        return -1;
    }
    if (channel == 4) {
        if (sram_stat->stat.Bundl0Rdy3)
            return 0;
        if (sram_stat->stat.Bundl1Rdy3)
            return 1;
        return -1;
    }
    return -1;
}

//---------------------------------------------------------
#if 0
static int sram_read_len(const int channel)
{
    LenMem_t buflen;
    if (channel == 1) {
        buflen = *Mem0Len;
        return buflen.GivenBufLenEven;
    }
    if (channel == 2) {
        buflen = *Mem0Len;
        return buflen.GivenBufLenOdd;
    }
    if (channel == 3) {
        buflen = *Mem1Len;
        return buflen.GivenBufLenEven;
    }
    if (channel == 4) {
        buflen = *Mem1Len;
        return buflen.GivenBufLenOdd;
    }
    return 0;
}
#endif

//---------------------------------------------------------
void sram_init(int length)
{
    DBG(1, "\n");
    sram_mem_clear();
    CtrlMem_t ctrl = {0};
    for (int channel = 1; channel <= sram_total_channels(); channel++) {
        ctrl.Channel = channel_to_onehot(channel);
        ctrl.GvnLenBuf = length;
        ctrl.ClrRdy0 = 1;
        ctrl.ClrRdy1 = 1;
        DBGF(1, "set sram-%d length: %u", channel, ctrl.GvnLenBuf);
        sram_write_ctrl_reg(&ctrl);
    }
    SramStat_t sram_stat;
    sram_status_get_status(&sram_stat);
    DBGF(1, "sram status: %s", sram_print_status(&sram_stat));
}

//---------------------------------------------------------
void sram_set_length(const int channel, const int length)
{
    CtrlMem_t ctrl = {0};
    ctrl.Channel = channel_to_onehot(channel);
    ctrl.GvnLenBuf = length;
    DBGF(1, "set sram-%d length: %u", channel, ctrl.GvnLenBuf);
    sram_write_ctrl_reg(&ctrl);
}

//---------------------------------------------------------
int sram_get_ready_bundle_num(int channel)
{
    // channel: 1..4
    StatMem_t status;
    sram_read_stat_reg(&status);
    if (channel == 1) {
        if (status.Bundl0Rdy0) {
            return 0;
        }
        if (status.Bundl1Rdy0) {
            return 1;
        }
    } else if (channel == 2) {
        if (status.Bundl0Rdy1) {
            return 0;
        }
        if (status.Bundl1Rdy1) {
            return 1;
        }
    } else if (channel == 3) {
        if (status.Bundl0Rdy2) {
            return 0;
        }
        if (status.Bundl1Rdy2) {
            return 1;
        }
    } else if (channel == 4) {
        if (status.Bundl0Rdy3) {
            return 0;
        }
        if (status.Bundl1Rdy3) {
            return 1;
        }
    }
    return -1;
}

//---------------------------------------------------------
const Bundle_t *sram_get_bundle(const int channel, const int bundlenum)
{
    if (channel < 0 || channel > 3) {
        return NULL;
    }
    if (bundlenum != 0 && bundlenum != 1) {
        return NULL;
    }
    return &m_sram.mem[channel]->bundle[bundlenum];
}

//---------------------------------------------------------
/// channel: 1..4
const Bundle_t *sram_get_ready_bundle(const int channel)
{
    int num = sram_get_ready_bundle_num(channel);
    if (num == -1)
        return NULL;
    return &sram_mem(channel)->bundle[num];
}

//---------------------------------------------------------
void sram_clear_bundle_ready(const int channel, const int bundle_num)
{
    // channel: 1..4
    CtrlMem_t ctrl = {0};
    ctrl.Channel = channel_to_onehot(channel);
    switch (bundle_num) {
        case 0:
            ctrl.ClrRdy0 = 1;
            ctrl.ClrRdy1 = 0;
            break;
        case 1:
            ctrl.ClrRdy0 = 0;
            ctrl.ClrRdy1 = 1;
            break;
        default: // for safe
            ctrl.ClrRdy0 = 1;
            ctrl.ClrRdy1 = 1;
            break;
    }
    DBGF(3, "sram[%d]: ClrRdy0:%u, ClrRdy1:%u", channel - 1, ctrl.ClrRdy0, ctrl.ClrRdy1);
    sram_write_ctrl_reg(&ctrl);
}

//---------------------------------------------------------
const char *sram_print_status(const SramStat_t *stat)
{
    static char buf[128] = {0};
    snprintf(buf, sizeof(buf) - 1,
             "(Len-Enb-B0Rdy-B1Rdy) [0] %u-%u-%u-%u "
             "[1] %u-%u-%u-%u "
             "[2] %u-%u-%u-%u "
             "[3] %u-%u-%u-%u ",
             stat->BufLen0, stat->stat.MemEn0, stat->stat.Bundl0Rdy0, stat->stat.Bundl1Rdy0,
             stat->BufLen1, stat->stat.MemEn1, stat->stat.Bundl0Rdy1, stat->stat.Bundl1Rdy1,
             stat->BufLen2, stat->stat.MemEn2, stat->stat.Bundl0Rdy2, stat->stat.Bundl1Rdy2,
             stat->BufLen3, stat->stat.MemEn3, stat->stat.Bundl0Rdy3, stat->stat.Bundl1Rdy3);
    return buf;
}

//---------------------------------------------------------
static char sram_dump_fn[8][32] = {0};
static int sram_dump_fd[8];
static int sram_dump_recs = 0;
static int SRAM_DUMP_MAX_RECS = 10;

//---------------------------------------------------------
void sram_dump_close()
{
    for (int i = 0; i < 8; i++) {
        if (sram_dump_fd[i] > -1) {
            close(sram_dump_fd[i]);
        }
        sram_dump_fd[i] = -1;
    }
}

//---------------------------------------------------------
bool sram_dump_create()
{
    if (!cfg()->do_dump) {
        return OK;
    }
    DBG(4);

    if (sram_dump_recs > SRAM_DUMP_MAX_RECS) {
        return OK;
    }

    // open files
    if (sram_dump_fn[0][0] == 0) {
        sram_dump_recs = 0;
        memset(sram_dump_fn, 0, sizeof(sram_dump_fn));
        memset(sram_dump_fd, -1, sizeof(sram_dump_fd));
        int idx = 0;
        for (int i = 0; i < sram_total_channels(); i++) {
            if (cfg()->test_channel > 0 && (i + 1) != cfg()->test_channel) {
                continue;
            }
            snprintf(sram_dump_fn[idx], sizeof(sram_dump_fn[0]), "dump-sram%d-b1.dump", i + 1);
            snprintf(sram_dump_fn[idx + 1], sizeof(sram_dump_fn[0]), "dump-sram%d-b2.dump", i + 1);
            idx += 2;
        }

        for (int i = 0; i < sram_total_channels(); i++) {
            if (sram_dump_fn[i][0] != 0 && sram_dump_fd[i] == -1) {
                sram_dump_fd[i] = open(sram_dump_fn[i], O_CREAT | O_TRUNC | O_WRONLY, 0600);
                if (sram_dump_fd[i] == -1) {
                    LOG(ERROR, "can't create file %s (%s)", sram_dump_fn[i], strerror(errno));
                    DBGF(0, "ERROR: can't create file %s (%s)", sram_dump_fn[i], strerror(errno));
                    cfg()->do_dump = 0;
                    return FAIL;
                }
            }
        }
    }
    sram_dump_recs++;

    for (int i = 0; i < sram_total_channels(); i++) {
        //lseek(f_d[i], 0, SEEK_SET);
        int idx = i * 2;
        if (sram_dump_fd[idx] == -1) {
            continue;
        }
        write(sram_dump_fd[idx], m_sram.mem[i]->bundle[0].recs, sizeof(m_sram.mem[i]->bundle[0].recs));
        write(sram_dump_fd[idx], "\r\n\r\n", 4);
        idx++;
        write(sram_dump_fd[idx], m_sram.mem[i]->bundle[1].recs, sizeof(m_sram.mem[i]->bundle[0].recs));
        write(sram_dump_fd[idx], "\r\n\r\n", 4);
    }

    return OK;
}
