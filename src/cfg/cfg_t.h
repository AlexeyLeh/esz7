/**
  \file   cfg_t.h
  \author Aleksey Lekhtin
  \date   Apr 05, 2024
  \brief  Config variables
*/

#ifndef CONFIG_STRUCT__H_
#define CONFIG_STRUCT__H_

#include "defines.h"
#include "siu_t.h"

typedef enum TEST_MODE : uint8_t
{
    TEST_NONE = 0,
    TEST1 = 1,
    TEST2 = 2,
    TEST3 = 3,
} TEST_MODE;

typedef enum work_mode_t
{
    MODE_UNKN = (int8_t)0,
    MODE_ATLAS = 1,
    MODE_APS = 2,
} work_mode_t;

typedef enum decimal_delimiter_t
{
    DECIMAL_DOT = (uint8_t)0,
    DECIMAL_COMMA
} decimal_delimiter_t;

typedef struct mem_cfg_t
{
    int  reg_mem_fd;
    void *reg_mem_map;
    int  sram_fd[4];
    void *sram_map[4];
} mem_cfg_t;

typedef struct adc_cfg_t
{
    uint32_t fpgafreq;      // Gz
    uint8_t reg_mode0;
    uint8_t reg_mode1;
    uint8_t reg_mode3;
    uint8_t reg_mode4;
    uint8_t reg_ref;
    uint32_t scanperiod;    // ticks until fpgafreq above zero
} adc_cfg_t;

typedef struct pressure_cfg_t
{
    uint16_t max_scale;
    uint16_t pumps_max;
    uint16_t pumps_min;
    uint16_t pumps_on_threshold;
    double scale;
} pressure_cfg_t;

typedef struct cfg_t
{
    char args[128];
    char appname[64];
    char appdir[256];
    char logfname[256];
    //
    work_mode_t mode;
    decimal_delimiter_t decimal;
    char locale[64];        // en_US ru_RU ...
    char lang_cli[64];      // en ru
    char lang_srv[64];      // en ru
    char rawfname[256];
    char siu_ini_fname[512];
    char esz7_cfg_fname[512];
    TEST_MODE test_mode;
    int  test_channel;
    work_mode_t do_dump;
    int shortmsg;  // 0 - everything send to terminal, 1 - short message
    units_t system_units;   // System Units
    uint32_t test_scanperiod;

    // esz7 and ADC resources
    int cmd_port;
    mem_cfg_t mem;
    adc_cfg_t adc_atlas;
    adc_cfg_t adc_aps;
    //bool adc_calibration;
    double adc_ref;
    double adc_scale;

    // Atlas mode data
    char atlas_host1[64];
    char atlas_host2[64];
    char atlas_host3[64];
    int  atlas_port11;
    int  atlas_port12;
    int  atlas_port21;
    int  atlas_port22;
    int  atlas_port31;
    int  atlas_port32;

    char atlas_serno[11];
    int  atlas_timeshift;
    int  atlas_proto_ver;

    // ADS mode data
    int  ads_cmd_port;
    int  ads_data_port;
    char ads_serno[11];

    // SureShot mode data
    int  siu_cmd_port;
    int  siu_data_port;
    char siu_serno[6];

    // Depth Tracker mode data
    int  dtr_cmd_port;
    int  dtr_data_port;
    char dtr_serno[6];

    char aps_wits_host[64];
    int  aps_wits_port;
    char aps_wits_serno[11];

    // Atlas wits
    char wits_host[64];
    int  wits_port;

    // Dual Pressure Transducer
    int dpt_delay;

    // some hardware conditions
    pressure_cfg_t pressure[2];
    bool sensor_connected[4];
    int swap_channels12;

    // specific envestigation options
    uint32_t pulsethreshold;

} cfg_t;

#endif // CONFIG_STRUCT__H_
