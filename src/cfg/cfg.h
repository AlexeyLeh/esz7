/**
  \file   cfg.h
  \author Aleksey Lekhtin
  \date   Apr 05, 2024
  \brief  Config variables
*/

#ifndef CONFIG__H_
#define CONFIG__H_

#include <stdbool.h>

#include "cfg_t.h"

#define TS_100US (1000*100)
#define TS_1MS   (1000*1000*1)
#define TS_10MS  (1000*1000*10)
#define TS_50MS  (1000*1000*50)
#define TS_100MS (1000*1000*100)
#define TS_200MS (1000*1000*200)
#define TS_500MS (1000*1000*500)
#define TS_1SEC  (1000*1000*1000)

#define DEF_ESZ7_CMD_PORT 8100

#define DEF_ATLAS_SERNO_FNAME ".serno_atlas"
#define DEF_ATLAS_SERNO       "ATLAS000"
#define DEF_ATLAS_PORT 16000

#define DEF_ADS_SERNO_FNAME ".serno_ads"
#define DEF_ADS_SERNO       "SIU00000"
#define DEF_ADS_PORT_CMD 8220
#define DEF_ADS_PORT_DATA 8110

#define DEF_SIU2_SERNO_FNAME ".serno_siu"
#define DEF_SIU2_SERNO       "S0000"
#define DEF_SIU2_PORT_CMD 7111
#define DEF_SIU2_PORT_DATA 8111

#define DEF_DTR_SERNO_FNAME ".serno_dtr"
#define DEF_DTR_SERNO       "D0000"
#define DEF_DTR_PORT_CMD 7110
#define DEF_DTR_PORT_DATA 8210

#define DEF_WITS_SERNO       "W0000"
#define DEF_WITS_PORT 5005

extern const char *msg_esz7_not_loaded;

cfg_t *cfg();

void cfg_get_app_path(int ac, char *av[]);
bool cfg_read();
void cfg_check_defaults();
void parse_help_args(int ac, char *av[]);
bool cfg_parse_args(int ac, char *av[]);
bool cfg_save_esz7_status();
bool cfg_read_esz7_status();
void cfg_print_configuration();
void cfg_save_serial_number(work_mode_t mode);
const char *cfg_mode_string(work_mode_t mode);
const char *version();

#endif // CONFIG__H_
