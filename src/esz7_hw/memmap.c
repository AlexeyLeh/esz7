/**
  \file   memmap.c
  \author Aleksey Lekhtin
  \date   11.04.2024
  \brief  Mapping register memory to application user space.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "cfg.h"
#include "dbg.h"
#include "adc.h"
#include "enc.h"
#include "sram.h"
#include "log.h"

/*-------------------------------------------------------*/
/**
 * Отображение памяти регистров в пользовательское пространство программы
 * @return bool
 * @return OK - успешно выполнено (1)
 * @return FAIL - ошибка (0)
 */
static bool memmap_esz7dev_registers()
{
    cfg()->mem.reg_mem_fd = -1;
    cfg()->mem.reg_mem_map = NULL;

    int memfd = open("/dev/mem", O_RDWR);
    if (memfd == -1) {
        LOG(FATAL, "can't open /dev/mem (%s)", strerror(errno));
        DBGF(0, "ERROR: can't open /dev/mem (%s)", strerror(errno));
        return FAIL;
    }

    void *mem_map = mmap(0, ESZ7_ADDR_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, ESZ7_ADDR);
    if (mem_map == MAP_FAILED) {
        LOG(FATAL, "%p mapping failed (%s)", (void *) ESZ7_ADDR, strerror(errno));
        DBGF(0, "ERROR: %p mapping failed (%s)", (void *) ESZ7_ADDR, strerror(errno));
        close(cfg()->mem.reg_mem_fd);
        cfg()->mem.reg_mem_fd = -1;
        return FAIL;
    }
    DBGF(2, "%p mapped to %p, /dev/mem fd:%d", (void *) ESZ7_ADDR, mem_map, memfd);

    cfg()->mem.reg_mem_fd = memfd;
    cfg()->mem.reg_mem_map = mem_map;
    setup_registers_address(cfg()->mem.reg_mem_map);
    setup_encoders_address(cfg()->mem.reg_mem_map);
    return OK;
}

/*-------------------------------------------------------*/
/**
 * Отменить отображение памяти регистров в пользовательское пространство программы
 */
static bool unmap_esz7dev_registers()
{
    bool ret = OK; // success assumed
    if (cfg()->mem.reg_mem_map) {
        DBGF(8, "mem_map: %p", cfg()->mem.reg_mem_map);
        if (munmap(cfg()->mem.reg_mem_map, ESZ7_ADDR_SIZE) == -1) {
            LOG(ERROR, "%p unmapping failed (%s)", cfg()->mem.reg_mem_map, strerror(errno));
            DBGF(0, "%p unmapping failed (%s)", cfg()->mem.reg_mem_map, strerror(errno));
            ret = FAIL;
        }
    }
    cfg()->mem.reg_mem_map = NULL;

    if (cfg()->mem.reg_mem_fd > 0) {
        DBGF(8, "mem_fd:%d", cfg()->mem.reg_mem_fd);
        if (close(cfg()->mem.reg_mem_fd) == -1) {
            LOG(ERROR, "close mem_fd:%d. errno:%d (%s)", cfg()->mem.reg_mem_fd, errno, strerror(errno));
            DBGF(0, "ERROR: close mem_fd:%d. errno:%d (%s)", cfg()->mem.reg_mem_fd, errno, strerror(errno));
            ret = FAIL;
        }
    }
    cfg()->mem.reg_mem_fd = -1;
    return ret;
}

/*-------------------------------------------------------*/
/**
 * Отменить отображение памяти RAM в пользовательское пространство программы
 * \return bool
 * \return 1 - успешно выполнено
 * \return 0 - ошибка
 */
static bool unmap_sram()
{
    sram_clear_reg_addr();
    int ret = OK; // success assumed
    for (int i = 0; i < sram_total_channels(); i++) {
        if (cfg()->mem.sram_map[i]) {
            DBGF(8, "mem.sram_map: %p", cfg()->mem.sram_map[i]);
            if (munmap(cfg()->mem.sram_map[i], sram_mem_size()) == -1) {
                LOG(ERROR, "%p unmapping failed (%s)", cfg()->mem.sram_map[i], strerror(errno));
                DBGF(0, "%p unmapping failed (%s)", cfg()->mem.sram_map[i], strerror(errno));
                ret = FAIL;
            }
        }
        cfg()->mem.sram_map[i] = NULL;

        if (cfg()->mem.sram_fd[i] > 0) {
            DBGF(8, "mem.mem_fd:%d", cfg()->mem.sram_fd[i]);
            if (close(cfg()->mem.sram_fd[i]) == -1) {
                LOG(ERROR, "fd:%d close error (%s)", cfg()->mem.sram_fd[i], strerror(errno));
                DBGF(0, "fd:%d close error (%s)", cfg()->mem.sram_fd[i], strerror(errno));
                ret = FAIL;
            }
        }
        cfg()->mem.sram_fd[i] = -1;
    }
    return ret;
}

/*-------------------------------------------------------*/
/**
 * Отображение памяти RAM в пользовательское пространство программы
 * \return bool
 * \return 1 - успешно выполнено
 * \return 0 - ошибка
 */
static bool memmap_sram()
{
    memset(cfg()->mem.sram_fd, -1, sizeof(cfg()->mem.sram_fd));
    memset((void *) cfg()->mem.sram_map, 0, sizeof(cfg()->mem.sram_map));

    for (int i = 0; i < sram_total_channels(); i++) {
        int mem_fd = open("/dev/mem", O_RDWR);
        if (mem_fd == -1) {
            LOG(FATAL, "can't open /dev/mem (%s)", strerror(errno));
            DBGF(0, "ERROR: can't open /dev/mem (%s)", strerror(errno));
            unmap_sram();
            return FAIL;
        }

        int channel = i + 1;
        void *mem_map = mmap(0, sram_mem_size(), PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, (off_t) sram_mem(channel));
        if (mem_map == MAP_FAILED) {
            LOG(ERROR, "%p mapping failed (%s)", sram_mem(channel), strerror(errno));
            DBGF(0, "ERROR: %p mapping failed (%s)", sram_mem(channel), strerror(errno));
            unmap_sram();
            return FAIL;
        }
        DBGF(2, "%p mapped to %p, /dev/mem fd:%d", sram_mem(channel), mem_map, mem_fd);

        cfg()->mem.sram_fd[i] = mem_fd;
        cfg()->mem.sram_map[i] = mem_map;
    }
    setup_sram_reg_addr(&cfg()->mem);
    return OK;
}

/*-------------------------------------------------------*/
void esz7_unmemmap_registers()
{
    unmap_esz7dev_registers();
    unmap_sram();
}

/*-------------------------------------------------------*/
bool esz7_memmap_registers()
{
    if (!memmap_esz7dev_registers()) {
        return FAIL;
    }
    if (!memmap_sram()) {
        return FAIL;
    }
    return OK;
}
