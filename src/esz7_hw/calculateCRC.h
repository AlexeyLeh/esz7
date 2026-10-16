/**
* \file   calculateCRC.h
* \author Aleksey Lekhtin
* \date   Sep 18, 2025
* \brief  calculate CRC for ADC ads125
*/

#ifndef CALC_CRC__H_
#define CALC_CRC__H_

#include <stdint.h>

uint8_t calculateCRC(const uint8_t dataBytes[], uint8_t numBytes);


#endif //CALC_CRC__H_