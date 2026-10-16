/**
    \file   enc_t.h
    \author Aleksey Lekhtin
    \date   03.12.2025
    \brief  Данные с энкодеров на ESZ7 плате (с XME0724-20 Soc)
*/
#ifndef ENC_T__H_
#define ENC_T__H_

#include "esz7_regs.h"

typedef enum encoder_type_t
{
    ENCODER_BLOCKPOS = (uint8_t)0,
    ENCODER_HOOKLOAD
} encoder_type_t;

typedef struct Encoders_t
{
    int32_t Enc0Value;
    int32_t Enc1Value;
    int32_t Enc0SetValue;
    int32_t Enc1SetValue;
    uint32_t EncsDebounce;
} Encoders_t;

#endif //ENC_T__H_
