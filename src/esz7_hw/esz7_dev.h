/**
    \file   dev.h
    \author Aleksey Lekhtin
    \date   18.09.2025
    \brief  Работа с ESZ7 платой (с XME0724-20 SoC)
*/

#ifndef DEV__H_
#define DEV__H_

#include <stdbool.h>
#include <time.h>

bool esz7_open();
void esz7_close();

void esz7_check_adc_id();
bool esz7_init_adc();

void esz7_setup();
bool esz7_stop_autoscan();
bool esz7_memmap_registers();
void esz7_unmemmap_registers();
bool esz7_adc_calibration();

#endif //DEV__H_
