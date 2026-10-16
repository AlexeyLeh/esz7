/**
  \file   siu_vars_t.h
  \author Aleksey Lekhtin
  \date   Oct 22, 2025
  \brief  The SureShot payload variables
*/

#ifndef SIU2_VARS_T__H_
#define SIU2_VARS_T__H_

#include <stdint.h>
#include <stdbool.h>
#include "mpt_t.h"

typedef enum units_t
{
    UnitsEnglish = (uint8_t)0,
    UnitsMetric
} units_t;

typedef enum units_torque_t
{
    TORQUE_NM = (uint8_t)0,
    TORQUE_KNM,
    TORQUE_FTLB,
    TORQUE_KFTLB
} units_torque_t;

typedef enum units_pressure_t
{
    PRESSURE_BAR = (uint8_t) 0,
    PRESSURE_ATM,
    PRESSURE_KPA,
    PRESSURE_KGCM2,
    PRESSURE_PSI
} units_pressure_t;

typedef enum force_frill_t
{
    NoForcing = (uint8_t) 0,
    ForceInSlips,
    ForceOutSlips
} force_drill_t;

typedef enum LedFlagBits : uint32_t
{
    PRES1AVAIL = 1u,
    PRES1ACVTY = 2u,
    PRES2AVAIL = 4u,
    PRES2ACVTY = 8u,
    HKLDAVAIL = 0x10u,
    HKLDACVTY = 0x20u,
    DRUMPOSACVTY = 0x40u,
    DRUMNEGACVTY = 0x80u,
    SERDEPTHAVAIL = 0x100u,
    SERDEPTHACVTY = 0x200u,
    TOOLPOWERON = 0x400u,
    PUMPSTATUS = 0x800u,
    RFDCONNECTED = 0x1000u,
    TOOLCOMMSACVTY = 0x2000u,
    TOOLPOWERFAULT = 0x4000u,
    pad = 0x8000u
} LedFlagBits;

typedef enum StatusFlagBits : uint32_t
{
    UNUSED = 1u,
    KTABLEERR = 2u,
    STORAGEERR = 8u,
    STORAGEFULL = 0x10u,
    A2DACCESSERR = 0x20u,
    A2DFIFOERR = 0x40u,
    WTABLEERR = 0x200u,
    DTABLEERR = 0x400u,
    ATABLEERR = 0x800u,
    DEPTHSYSTEMINT = 0x1000u,
    DEPTHBOTTOM = 0x2000u,
    DEPTHSLIPS = 0x4000u,
    DEPTHFORCING = 0x8000u,
    DECODERCOMERR = 0x10000u,
    PLAYBACKON = 0x20000u,
    DECODINGDATA = 0x40000u,
    SIGNALDETECT = 0x80000u,
    DEPTHSYSTEMEXT = 0x100000u,
    CONFIGDATAERR = 0x200000u,
    IODVRACCESSERR = 0x400000u,
    KEYPADERROR = 0x800000u,
    IOSTORAGEERR = 0x1000000u,
    IOSTORAGEFULL = 0x2000000u,
    DECODELOGFULL = 0x4000000u,
    DEPTHSYSTEMON = 0x8000000u
} StatusFlagBits;

typedef struct led_flags_t
{
    bool PRES1AVAIL;
    bool PRES1ACVTY;
    bool PRES2AVAIL;
    bool PRES2ACVTY;
    bool HKLDAVAIL;
    bool HKLDACVTY;
    bool DRUMPOSACVTY;
    bool DRUMNEGACVTY;
    bool SERDEPTHAVAIL;
    bool SERDEPTHACVTY;
    bool TOOLPOWERON;
    bool PUMPSTATUS;
    bool RFDCONNECTED;
    bool TOOLCOMMSACVTY;
    bool TOOLPOWERFAULT;
    bool pad;
    LedFlagBits led_flag;
} led_flags_t;

typedef struct status_flags_t
{
    bool UNUSED;
    bool KTABLEERR;
    bool STORAGEERR;
    bool STORAGEFULL;
    bool A2DACCESSERR;
    bool A2DFIFOERR;
    bool WTABLEERR;
    bool DTABLEERR;
    bool ATABLEERR;
    bool DEPTHSYSTEMINT;
    bool DEPTHBOTTOM;
    bool DEPTHSLIPS;
    bool DEPTHFORCING;
    bool DECODERCOMERR;
    bool PLAYBACKON;
    bool DECODINGDATA;
    bool SIGNALDETECT;
    bool DEPTHSYSTEMEXT;
    bool CONFIGDATAERR;
    bool IODVRACCESSERR;
    bool KEYPADERROR;
    bool IOSTORAGEERR;
    bool IOSTORAGEFULL;
    bool DECODELOGFULL;
    bool DEPTHSYSTEMON;
    StatusFlagBits status_flag;
} sys_flags_t;

typedef enum RfdAzimuthCorrections_t
{
    AZCorr_Magnetic,
    AZCorr_True,
    AZCorr_Grid
} RfdAzimuthCorrections_t;

typedef enum IpSource_t
{
    FIXED,
    DHCP
} IpSource_t;

typedef struct apslan_t
{
    IpSource_t NetApsIpSource;
    char NetApsIp[16];
    char NetApsNetmask[16];
    char NetApsGateway[16];
} apslan_t;

typedef struct riglan_t
{
    IpSource_t NetRigIpSource;
    char NetRigIp[16];
    char NetRigNetmask[16];
    char NetRigGateway[16];
} riglan_t;

typedef enum RfdToolfaceScales_t
{
    Tool_Magnetic,
    Tool_Gravity
} RfdToolfaceScales_t;

typedef struct wits_code_t
{
    bool enabled;
    int record;
    int item;
} wits_code_t;

typedef enum wifi_encryption_types_t
{
    ENC_NONE,
    ENC_WEPOPEN,
    ENC_WEPSHARED,
    ENC_WPATKIP,
    ENC_WPAAES
} wifi_encryption_types_t;

typedef struct wifi_t
{
    char WifiSsid[64];
    int WifiSsidHidden;
    wifi_encryption_types_t WifiEncryptionType;
} wifi_t;

typedef struct hookload_calibration_table_t
{
    uint32_t a2d1;
    uint32_t a2d2;
    float weight1;
    float weight2;
    float threshold;
    float drillStringWeight;
} hookload_calibration_t;

typedef struct depthdata_t
{
    float BitDepth;         // in feets
    float HoleDepth;        // in feets
    float KellyDownDepth;   // in feets
    float BlockPosition;
    float BlockSpeed;
    float Rop1s;
    float Rop5s;
    float WeightOnBit;
    float Hookload;
    int   DrillState;
    bool  OnBottom;
    bool  InSlips;
} depthdata_t;

typedef struct depthdata_ext_t
{
    float WobAvg;
    float HkldAvg;
    float Torque;
    float RPM;
    float PumpStrokeRate1;
    float PumpStrokeRate2;
    float PumpStrokeRate3;
    float MudFlowInAvg;
    float MudFlowOutAvg;
    float DifferentialPressure;
} depthdata_ext_t;

typedef struct block_position_t
{
    int count;
    float value;
} block_position_t;

typedef struct pos_calibration_t {
    int size;
    block_position_t points[32];
} pos_calibration_t;

typedef struct siu_vars_t
{
    char serno[32];
    char version_firmware[11];
    char version_dsp_firmware[12];  // len:0 || "N/A" || xxx

    units_pressure_t PressureUnits;

    units_t WitsDepthUnits;
    units_t WitsFlowUnits;
    units_t WitsHkldUnits;
    units_torque_t WitsTorqueUnits;
    wits_code_t WitsDiffPressCode;
    bool WitsRaw;
    bool WitsOutput;

    wifi_t wifi;

    bool dss_on;
    int backlight;
    bool RfdCommOn;
    double tempio;          //Celcium
    double tempdec;
    double L1corrPkData;
    double L2corrPkData;
    double L1corrThresh;
    double L2corrThresh;
    double toolPowerCurrent;
    double toolPowerVoltage;

    char rfd1Ip[16];
    char rfd2Ip[16];

    apslan_t apslan;
    riglan_t riglan;

    RfdAzimuthCorrections_t RfdAzimuthCorrection;
    double RfdAzimuthGridCorrection;
    double RfdAzimuthMagneticDeclination;
    double RfdScribelineCorrection;
    int    RfdHeartbeatSeconds;
    int    RfdNetworkPort;
    int    RfdToolfaceScale;
    bool   RfdTrafficLight;

    mpt_t mpt;

    char DIE[9];                        // (INTERNAL / EXTERNAL)
    bool LOG;                           // (ON / OFF)
    bool DEN;                           // Dsp Enable
    bool DSS;                           // DepthTracking (ON / OFF)
    bool EQF;                           // Equalizer
    bool PSS;                           // Pump Status
    bool TTR;                           // Time Tracking
    int  POT;                           // Pumps On Pressure Threshold
    int  FLT;                           // Filter Correlation Type (01 / 02)
    int  SPS;                           // Pressure Scaling
    int  SPP[2];                        // Standpipe Pressure Calibration
    double OnBottomThreshold;           // OnBottom Threshold

    int DER;                            // Depth Encoder Counts
    pos_calibration_t DepthCalibrationTable; // Depth Encoder Calibration

    uint16_t HookloadA2D;                        // Hookload value from ADC
    hookload_calibration_t HKLDCalibrationTable; // Hookload Calibration

    depthdata_t DepthSystemData;             // Depth data
    depthdata_ext_t DSDE;               // Depth data extended info

    mpt_encoding_t MPTENC;              // Mpt Type
    double MPTRATE;                     // Mpt Rate
    force_drill_t DFS;                  // Force Drill State
    units_t SDU;                        // System Units
    led_flags_t dev_status;
    sys_flags_t sys_status;
} siu_vars_t;

#endif //SIU2_VARS_T__H_
