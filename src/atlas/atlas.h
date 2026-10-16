/**
  \file   atlas.h
  \author Aleksey Lekhtin
  \date   2024/02/01
  \brief  Работа с Atlas
*/

#ifndef ATLAS__H_
#define ATLAS__H_

#include <stdbool.h>
#include <time.h>

#include "atlas_t.h"

atlas_t *atlas();
bool atlas_run();
void atlas_stop();
void atlas_data_send();
void atlas_write_telemetry();
#endif //ATLAS__H_
