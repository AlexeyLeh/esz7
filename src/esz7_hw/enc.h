/**
    \file   enc.h
    \author Aleksey Lekhtin
    \date   03.12.2025
    \brief  Данные с энкодеров на ESZ7 плате (с XME0724-20 Soc)
*/
#ifndef ENC__H_
#define ENC__H_

#include "enc_t.h"

void setup_encoders_address(void *reg_mem_map);
void esz7_set_encoder_position(encoder_type_t enctype, int value);
Encoders_t esz7_get_encoders();

#endif //ENC__H_
