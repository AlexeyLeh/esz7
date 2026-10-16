/**
    \file   enc.c
    \author Aleksey Lekhtin
    \date   13.11.2025
    \brief  Данные с энкодеров на ESZ7 плате (с XME0724-20 Soc)
*/

#include <stdio.h>

#include "dbg.h"
#include "enc.h"

static volatile int32_t *enc0_data_reg = NULL;
static volatile int32_t *enc1_data_reg = NULL;
static volatile int32_t *enc0_pos_reg = NULL;
static volatile int32_t *enc1_pos_reg = NULL;
static volatile uint32_t *encs_deb_reg = NULL;

//---------------------------------------------------------
void setup_encoders_address(void *reg_mem_map)
{
    enc0_data_reg = ((volatile int32_t *) (reg_mem_map + ENC0_ADDR));
    enc1_data_reg = ((volatile int32_t *) (reg_mem_map + ENC1_ADDR));
    encs_deb_reg  = ((volatile uint32_t *) (reg_mem_map + ENCDEB_ADDR));
    enc0_pos_reg  = ((volatile int32_t *) (reg_mem_map + ENCLOAD0_ADDR));
    enc1_pos_reg  = ((volatile int32_t *) (reg_mem_map + ENCLOAD1_ADDR));

    DBGF(2, "Enc0 Data reg:     %p (0x%02x)", enc0_data_reg, ENC0_ADDR);
    DBGF(2, "Enc1 Data reg:     %p (0x%02x)", enc1_data_reg, ENC1_ADDR);
    DBGF(2, "Encs Debounce reg: %p (0x%02x)", encs_deb_reg, ENCDEB_ADDR);
    DBGF(2, "Enc0 Position reg: %p (0x%02x)", enc0_pos_reg, ENCLOAD0_ADDR);
    DBGF(2, "Enc1 Position reg: %p (0x%02x)", enc1_pos_reg, ENCLOAD1_ADDR);
}

//---------------------------------------------------------
inline Encoders_t esz7_get_encoders()
{
    Encoders_t encoders = {0};
    encoders.Enc0Value = *enc0_data_reg;
    encoders.Enc1Value = *enc1_data_reg;
    return encoders;
}

//---------------------------------------------------------
void esz7_set_encoder_position(encoder_type_t enctype, int value)
{
    if (value < 0) {
        value = 0;
    }
    if (enctype == ENCODER_BLOCKPOS) {
        *enc0_pos_reg = value;
    } else if (enctype == ENCODER_HOOKLOAD) {
        *enc1_pos_reg = value;
    }
}
