/**
    \file   adc.h
    \author Aleksey Lekhtin
    \date   18.09.2025
    \brief  Работа с АЦП установленном на ESZ7 плате (с XME0724-20 Soc)
*/
#ifndef ADC__H_
#define ADC__H_

#include "adc_t.h"

#define MODE0_ATLAS_DEF 0x70    /** 0x70 - 19.2kSPS & Sinc1 */
#define MODE1_ATLAS_DEF 0x11    /** Pulse & 50us */
#define MODE3_ATLAS_DEF 0x40    /** STATUS0 enabled */
#define MODE4_ATLAS_DEF 0x04    /** Input Switch/Gain - External / Gain:1.0 */
#define REF_ATLAS_DEF   0x0A    /** External */

#define MODE0_ADS_DEF   0x70    /** 0x70 - 19.2kSPS & Sinc1 */
#define MODE1_ADS_DEF   0x11    /** Pulse & 50us */
#define MODE3_ADS_DEF   0x40    /** STATUS0 enabled */
#define MODE4_ADS_DEF   0x04    /** Input Switch/Gain - External / Gain:1.0 */
#define REF_ADS_DEF     0x0A    /** External */
void setup_registers_address(void *reg_mem_map);

bool read_adc_register(int channel, uint8_t reg_num, uint8_t *value);
bool write_adc_register(int channel, uint8_t reg_num, uint8_t value);

bool init_adc(int channel);
bool reset_adc(int channel);
bool read_adc_id(int channel, uint8_t *id);
void dev_read_status(DevStat_t *status);
uint32_t read_firmware_version();

void dev_set_adc_scan_period(int channel, uint32_t number);
bool is_dev_adc_scanning();
bool dev_start_channels();
bool dev_stop_channels();
bool dev_start_adc(int channels);
bool dev_adc_stop_scan(int channel);
const char *CmdOpCodeName(cmd_op_t cmdcode);
const char *reg_name(uint8_t reg_num);
const char *cmd_name(uint8_t cmdcode);

const char *print_dev_status(const DevStat_t *status);
void print_AdcStatus(int channel, uint8_t status0, int shortmsg);
void print_AdcResp(const AdcResp_t *resp);
const char *print_response_details(uint8_t reg_num, uint8_t value);
void print_status0(uint8_t status0, char *msg, int msg_len, int shortmsg);
void print_status1(uint8_t status1, char *msg, int msg_len, int shortmsg);
void print_status2(uint8_t status2, char *msg, int msg_len, int shortmsg);

#endif //ADC__H_
