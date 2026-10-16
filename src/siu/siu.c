/**
  \file   siu.c
  \author Aleksey Lekhtin
  \date   Apr  8, 2025
  \brief  The SIU feature
*/

#include <string.h>
#include <sys/stat.h>

#include "cfg.h"
#include "ads.h"
#include "dbg.h"
#include "siu.h"
#include "siu_socket.h"
#include "thr_callbacks.h"
#include "utils.h"

static siu_t m_siu = (siu_t){
    .cmd = {},
    .cmd.sock = {.fd = INVALID_SOCKET},

    .data = {},
    .data.online = NO,
    .data.sock = {.fd = INVALID_SOCKET},

    .packet.bytes = NULL,
    .packet.nbytes = 421,

    .vars = {},
    .vars.version_firmware = "SIU2000001",
    .vars.version_dsp_firmware = "0000000001",
    .vars.tempio = 25.0, // celcium
    .vars.tempdec = 26.0,
    .vars.apslan.NetApsIp = "0.0.0.0",
    .vars.apslan.NetApsNetmask = "0.0.0.0",
    .vars.apslan.NetApsGateway = "0.0.0.0",
    .vars.riglan.NetRigIp = "0.0.0.0",
    .vars.riglan.NetRigNetmask = "0.0.0.0",
    .vars.riglan.NetRigGateway = "0.0.0.0",
    .vars.rfd1Ip = "0.0.0.0",
    .vars.rfd2Ip = "0.0.0.0",
    .vars.WitsDepthUnits = UnitsMetric,
    .vars.WitsFlowUnits = UnitsMetric,
    .vars.WitsHkldUnits = UnitsMetric,
    .vars.WitsTorqueUnits = TORQUE_NM,
    .vars.PressureUnits = PRESSURE_BAR,

    .vars.SDU = UnitsMetric,
    .vars.DIE = "INTERNAL",
    .vars.SPS = 3000, // pci (200 atm)
};

//---------------------------------------------------------
inline siu_t *siu()
{
    return &m_siu;
}

//---------------------------------------------------------
bool siu_run()
{
    DBG(1);
    strncpy(m_siu.vars.serno, cfg()->siu_serno, sizeof(m_siu.vars.serno) - 1);
    m_siu.cmd.port = cfg()->siu_cmd_port;
    m_siu.data.port = cfg()->siu_data_port;

    uint32_t status = siu_dev_status();
    if (cfg()->sensor_connected[0]) {
        status |= PRES1AVAIL;
    }
    if (cfg()->sensor_connected[1]) {
        status |= PRES2AVAIL;
    }
    if (cfg()->sensor_connected[2]) {
        status |= HKLDAVAIL;
    }
    siu_update_dev_status(status);

    //-------------------------------------------
    thr_prop_t prop = {
        .name = "siu-cmd",
        .type = THR_TYPE_SIU_CMD,
        .mode = THR_DETACH,
        .start_func = thread_start,
        .stop_func = thread_stop,
        .payload = &m_siu,
    };
    THR_PROP_START(thr_cmd, &prop);
    if (!thr_cmd) {
        return FAIL;
    }
    DBG_NSLEEP(TS_100MS);

    //-------------------------------------------
    prop = (thr_prop_t){
        .name = "siu-data",
        .type = THR_TYPE_SIU_DATA,
        .mode = THR_DETACH,
        .start_func = thread_start,
        .stop_func = thread_stop,
        .payload = &m_siu,
    };
    THR_PROP_START(thr_data, &prop);
    if (!thr_data) {
        return FAIL;
    }
    DBG_NSLEEP(TS_100MS);
    return OK;
}
