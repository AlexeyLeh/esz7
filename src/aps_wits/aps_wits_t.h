/**
  \file   aps_wits_t.h
  \author Aleksey Lekhtin
  \date   2024/02/01
  \brief  Структуры данных для работы с Atlas
*/

#ifndef APS_WITS_T__H_
#define APS_WITS_T__H_

#include <stdbool.h>
#include <time.h>

#include "core_t.h"
#include "main.h"
#include "socket.h"
#include "thr.h"
#include "aps_wits_packet.h"

typedef struct aps_wits_vars_t
{
    /*
    public static SurfaceData.SurfaceDataPacket ConvertToSurfaceDataPacket(SurfaceDisplay dataToConvert, SureShot units)
    {
        SurfaceData.SurfaceDataPacket surfaceDataPacket = new SurfaceData.SurfaceDataPacket();
        surfaceDataPacket.Timestamp = dataToConvert.TS_R;
        surfaceDataPacket.HoleDepth = dataToConvert.HDTH_R;
        surfaceDataPacket.BitDepth = dataToConvert.BDTH_R;
        surfaceDataPacket.BlockPosition = units.ConvertToUsWithNulls(SureShot.Unit.Length, dataToConvert.BLKP);
        surfaceDataPacket.BlockVelocity = units.ConvertToUsWithNulls(SureShot.Unit.Length, dataToConvert.BLKV);
        surfaceDataPacket.ROP1S = units.ConvertToUsWithNulls(SureShot.Unit.Length, dataToConvert.ROP);
        surfaceDataPacket.ROP5S = surfaceDataPacket.ROP1S;
        surfaceDataPacket.Hookload = units.ConvertToUsWithNulls(SureShot.Unit.Force, dataToConvert.HKLD);
        surfaceDataPacket.WeightOnBit = units.ConvertToUsWithNulls(SureShot.Unit.Force, dataToConvert.SWOB);
        surfaceDataPacket.Torque = units.ConvertToUsWithNulls(SureShot.Unit.Torque, dataToConvert.TORQ);
        surfaceDataPacket.RPM = dataToConvert.RPM;
        surfaceDataPacket.StandpipePressure = dataToConvert.SPP_R;
        surfaceDataPacket.MudFlowIn = dataToConvert.MFI_R;
        surfaceDataPacket.MudFlowOut = dataToConvert.MFO_R;
        surfaceDataPacket.StrokesPerMin1 = dataToConvert.SPM1;
        surfaceDataPacket.StrokesPerMin2 = dataToConvert.SPM2;
        surfaceDataPacket.StrokesPerMin3 = dataToConvert.SPM3;
        surfaceDataPacket.DifferentialPressure = units.ConvertToUsWithNulls(SureShot.Unit.Pressure, dataToConvert.DIFFPRES);
        surfaceDataPacket.OnBottom = dataToConvert.ONBTM;
        surfaceDataPacket.InSlips = dataToConvert.SLIP;
        surfaceDataPacket.Approved = dataToConvert.APPROVED;
        return surfaceDataPacket;
    }
     */

    struct timespec Timestamp;
    double HoleDepth;
    double BitDepth;
    double BlockPosition;
    double BlockVelocity;
    double ROP1S;
    double ROP5S;
    double Hookload;
    double WeightOnBit;
    double Torque;
    double RPM;
    double StandpipePressure;
    double MudFlowIn;
    double MudFlowOut;
    double StrokesPerMin1;
    double StrokesPerMin2;
    double StrokesPerMin3;
    double DifferentialPressure;
    bool OnBottom;
    bool InSlips;
    bool Approved;
    uint8_t DrillState;
    double KellyDownDepth;
} aps_wits_vars_t;

typedef struct aps_wits_packet_t
{
    char *bytes;
    int nmem;
    int nbytes;
} aps_wits_packet_t;

typedef struct aps_wits_t
{
    thr_t *thr;
    thr_t *cli_thr;
    socket_t sock;
    pthread_mutex_t mutex;
    bool online;

    int port;
    char serno[32];
    const char *host;

    aps_wits_vars_t vars;
    aps_wits_packet_t packet;

    struct timespec timestamp;
    struct tm tm;
    uint32_t recnum;
    uint32_t seqnum;
    uint16_t ACTCOD;

    stat_t stat;
} aps_wits_t;

#endif //APS_WITS_T__H_
