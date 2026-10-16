/**
  \file   core.h
  \author Aleksey Lekhtin
  \date   Okt  3, 2025
  \brief  Работы с CORE
*/

#ifndef CORE__H
#define CORE__H

#include <stdint.h>
#include <stdbool.h>

core_t *core();
bool core_run();
bool core_cmd_run();
void core__wakeup();

// private methods
void core_test_mode();
void toggle_points_buffer_ch(int ch);
void get_data_from_sram(int channel, int bundle_num);
void get_encoders_data();
void get_hookload();

#endif //CORE__H
