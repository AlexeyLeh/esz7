/**
  \file   sram.h
  \author Aleksey Lekhtin
  \date   15.07.2024
  \brief  Работа с памятью, хранящей данные с АЦП
 */

#ifndef CHIP_MEM__H_
#define CHIP_MEM__H_

#include "sram_t.h"

typedef struct __attribute__((packed, aligned(4))) SramStat_t {
    uint16_t  BufLen0;
    uint16_t  BufLen1;
    uint16_t  BufLen2;
    uint16_t  BufLen3;
    StatMem_t stat;
} SramStat_t;

sram_t *sram();
int sram_total_channels();
sram_mem_t *sram_mem(int channel);
int sram_mem_size();

void sram_init(int length);
void setup_sram_reg_addr(const mem_cfg_t *mem);
void sram_clear_reg_addr();
void sram_set_length(int channel, int length);

int sram_get_ready_bundle_num(int channel);
const Bundle_t *sram_get_bundle(int channel, int bundlenum);
const Bundle_t *sram_get_ready_bundle(int channel);
void sram_clear_bundle_ready(int channel, int bundle_num);

void sram_mem_clear();

void sram_status_get_status(SramStat_t *sram_stat);
int sram_status_get_len(int channel, const SramStat_t *sram_stat);
int sram_status_get_memEn(int channel, const SramStat_t *sram_stat);
int sram_status_get_bundle_ready(int channel, const SramStat_t *sram_stat);

bool sram_dump_create();
void sram_dump_close();
const char *sram_print_status(const SramStat_t *stat);

#endif
