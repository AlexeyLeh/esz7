/**
  \file   siu_cfg.c
  \author Aleksey Lekhtin
  \date   Jun 17, 2025
  \brief  The SIU that saving configuration
*/

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "cfg.h"
#include "dbg.h"
#include "defines.h"
#include "siu.h"
#include "utils.h"
#include "log.h"

//---------------------------------------------------------
const char *mpt_type_to_string(mpt_encoding_t type)
{
    if (type == Biphase) {
        return "BPH";
    }
    if (type == Miller) {
        return "MLR";
    }
    if (type == RLL) {
        return "RLL";
    }
    if (type == TimeBase3) {
        return "TM3";
    }
    if (type == TimeBase2) {
        return "TM2";
    }
    return "Unknown";
}

//---------------------------------------------------------
mpt_encoding_t string_to_mpt(const char *str)
{
    if (strcasecmp(str, "BPH") == 0) {
        return Biphase;
    } else if (strcasecmp(str, "TM2") == 0) {
        return TimeBase2;
    } else if (strcasecmp(str, "TM3") == 0) {
        return TimeBase3;
    } else if (strcasecmp(str, "RLL") == 0) {
        return RLL;
    } else if (strcasecmp(str, "MLR") == 0) {
        return Miller;
    }
    return MPT_ERR;
}

//---------------------------------------------------------
static bool read_lan_addr(const char *param, char *str)
{
    int idx = 0;
    int num = 0;
    if (strcasecmp(param, "APSLAN") == 0) {
        const char *sep = ":";
        const char *s = strtok(str, sep);
        while (s) {
            if (idx == 0) {
                siu()->vars.apslan.NetApsIpSource = FIXED;
                if (strcasecmp(s, "DHCP") == 0) {
                    siu()->vars.apslan.NetApsIpSource = DHCP;
                }
            } else if (idx == 1) {
                strncpy(siu()->vars.apslan.NetApsIp, s, sizeof(siu()->vars.apslan.NetApsIp) - 1);
            } else if (idx == 2) {
                strncpy(siu()->vars.apslan.NetApsNetmask, s, sizeof(siu()->vars.apslan.NetApsNetmask) - 1);
            } else if (idx == 3) {
                strncpy(siu()->vars.apslan.NetApsGateway, s, sizeof(siu()->vars.apslan.NetApsGateway) - 1);
            }
            idx++;
            s = strtok(NULL, sep);
        }
        return YES;
    }

    if (strcasecmp(param, "RIGLAN") == 0) {
        const char *sep = ":";
        const char *s = strtok(str, sep);
        idx = 0;
        while (s) {
            if (idx == 0) {
                siu()->vars.riglan.NetRigIpSource = FIXED;
                if (strcasecmp(s, "DHCP") == 0) {
                    siu()->vars.riglan.NetRigIpSource = DHCP;
                }
            } else if (idx == 1) {
                strncpy(siu()->vars.riglan.NetRigIp, s, sizeof(siu()->vars.riglan.NetRigIp) - 1);
            } else if (idx == 2) {
                strncpy(siu()->vars.riglan.NetRigNetmask, s, sizeof(siu()->vars.riglan.NetRigNetmask) - 1);
            } else if (idx == 3) {
                strncpy(siu()->vars.riglan.NetRigGateway, s, sizeof(siu()->vars.riglan.NetRigGateway) - 1);
            }
            idx++;
            s = strtok(NULL, sep);
        }
        return YES;
    }

    if (strcasecmp(param, "RFDADR") == 0) {
        //snprintf(innerbuf, sizeof(innerbuf), "RFDADDR=1:%s\n", siu2()->obj.rfd1Ip);
        const char *sep = ":";
        const char *s = strtok(str, sep);
        idx = 0;
        num = 0;
        while (s) {
            if (idx == 0) {
                num = (int) strtol(s, NULL, 10);
            } else {
                if (num == 1) {
                    strncpy(siu()->vars.rfd1Ip, s, sizeof(siu()->vars.rfd1Ip) - 1);
                } else if (num == 2) {
                    strncpy(siu()->vars.rfd2Ip, s, sizeof(siu()->vars.rfd2Ip) - 1);
                }
            }
            idx++;
            s = strtok(NULL, sep);
        }
        return YES;
    }

    return NO;
}

//---------------------------------------------------------
static bool read_WITSUNITS(const char *param, const char *sValue)
{
    if (strcasecmp(param, "WitsDepthUnits") == 0) {
        if (strcasecmp(sValue, "E") == 0 || strcasecmp(sValue, "English") == 0) {
            siu()->vars.WitsDepthUnits = UnitsEnglish;
        }
        return YES;
    }
    if (strcasecmp(param, "WitsFlowUnits") == 0) {
        if (strcasecmp(sValue, "E") == 0 || strcasecmp(sValue, "English") == 0) {
            siu()->vars.WitsFlowUnits = UnitsEnglish;
        }
        return YES;
    }
    if (strcasecmp(param, "WitsHkldUnits") == 0) {
        if (strcasecmp(sValue, "E") == 0 || strcasecmp(sValue, "English") == 0) {
            siu()->vars.WitsHkldUnits = UnitsEnglish;
        }
        return YES;
    }
    if (strcasecmp(param, "WitsTorqueUnits") == 0) {
        if (strcasecmp(sValue, "KFTLB") == 0) {
            siu()->vars.WitsTorqueUnits = TORQUE_KFTLB;
        } else if (strcasecmp(sValue, "FTLB") == 0) {
            siu()->vars.WitsTorqueUnits = TORQUE_FTLB;
        } else if (strcasecmp(sValue, "KNM") == 0) {
            siu()->vars.WitsTorqueUnits = TORQUE_KNM;
        } else if (strcasecmp(sValue, "NM") == 0) {
            siu()->vars.WitsTorqueUnits = TORQUE_NM;
        }
        return YES;
    }

    return NO;
}

//---------------------------------------------------------
static bool read_WITSRAW(const char *param, const char *sValue)
{
    if (strcasecmp(param, "WITSRAW") == 0) {
        siu()->vars.WitsRaw = NO;
        if (strcasecmp(sValue, "ON") == 0 || strcasecmp(sValue, "1") == 0) {
            siu()->vars.WitsRaw = YES;
        }
        return YES;
    }

    return NO;
}

//---------------------------------------------------------
static bool read_WITSDPCODE(const char *param, char *sValue)
{
    if (strcasecmp(param, "WITSDPCODE") == 0) {

        const char *sep = ":";
        const char *s = strtok(sValue, sep);
        int idx = 0;
        while (s) {
            if (idx == 0) {
                siu()->vars.WitsDiffPressCode.enabled = NO;
                if (strcasecmp(sValue, "ON") == 0 || strcasecmp(sValue, "1") == 0) {
                    siu()->vars.WitsDiffPressCode.enabled = YES;
                }
            } else if (idx == 1 || idx == 2) {
                errno = 0;
                int num = strtol(s, NULL, 10);
                if (errno == 0) {
                    if (idx == 1) {
                        siu()->vars.WitsDiffPressCode.record = num;
                    } else {
                        siu()->vars.WitsDiffPressCode.item = num;
                    }
                }
            }
            idx++;
            s = strtok(NULL, sep);
        }
        return YES;
    }

    return NO;
}

//---------------------------------------------------------
static bool read_pressure_units(const char *param, char *sValue)
{
    if (strcasecmp(param, "PressureUnits") == 0) {

        if (strcasecmp(sValue, "KGCM2") == 0) {
            siu()->vars.PressureUnits = PRESSURE_KGCM2;
        } else if (strcasecmp(sValue, "ATM") == 0) {
            siu()->vars.PressureUnits = PRESSURE_ATM;
        } else if (strcasecmp(sValue, "BAR") == 0) {
            siu()->vars.PressureUnits = PRESSURE_BAR;
        } else if (strcasecmp(sValue, "KPA") == 0) {
            siu()->vars.PressureUnits = PRESSURE_KPA;
        } else if (strcasecmp(sValue, "PSI") == 0) {
            siu()->vars.PressureUnits = PRESSURE_PSI;
        }
        return YES;
    }

    return NO;
}

//---------------------------------------------------------
static bool read_calibration(const char *param, char *str)
{
    int idx = 0;
    int num = 0;
    int pos = 0;
    float val;
    if (strcasecmp(param, "DepthCalibrationTable") == 0) {
        // DTR=+00000:+000.10:+00100:+003.94:+02500 ...
        siu()->vars.DepthCalibrationTable.size = 0;
        const char *sep = ":";
        const char *s = strtok(str, sep);
        while (s) {
            errno = 0;
            if (pos == 0) {
                num = strtol(s, NULL, 10);
                if (errno == 0) {
                    siu()->vars.DepthCalibrationTable.points[idx].count = num;
                }
                pos = 1;
            } else {
                val = strtof(s, NULL);
                if (errno == 0) {
                    siu()->vars.DepthCalibrationTable.points[idx].value = val;
                }
                idx++;
                siu()->vars.DepthCalibrationTable.size++;
                pos = 0;
            }
            s = strtok(NULL, sep);
        }
        return YES;
    }

    if (strcasecmp(param, "DSDE") == 0) {
        // DSDE=0.00:0.00:0.00:0.00:0.00:0.00:0.00:0.00:0.00:0.00
        const char *sep = ":";
        const char *s = strtok(str, sep);
        idx = 0;
        while (s) {
            errno = 0;
            val = strtof(s, NULL);
            if (errno == 0) {
                if (idx == 0) {
                    siu()->vars.DSDE.WobAvg = val;
                } else if (idx == 1) {
                    siu()->vars.DSDE.HkldAvg = val;
                } else if (idx == 2) {
                    siu()->vars.DSDE.Torque = val;
                } else if (idx == 3) {
                    siu()->vars.DSDE.RPM = val;
                } else if (idx == 4) {
                    siu()->vars.DSDE.PumpStrokeRate1 = val;
                } else if (idx == 5) {
                    siu()->vars.DSDE.PumpStrokeRate2 = val;
                } else if (idx == 6) {
                    siu()->vars.DSDE.PumpStrokeRate3 = val;
                } else if (idx == 7) {
                    siu()->vars.DSDE.MudFlowInAvg = val;
                } else if (idx == 8) {
                    siu()->vars.DSDE.MudFlowOutAvg = val;
                } else if (idx == 9) {
                    siu()->vars.DSDE.DifferentialPressure = val;
                }
            }
            idx++;
            s = strtok(NULL, sep);
        }
        return YES;
    }

    if (strcasecmp(param, "HKLDCalibrationTable") == 0) {
        // HSC=00000:000.0:00000:000.0:000.0:000.0
        const char *sep = ":";
        const char *s = strtok(str, sep);
        idx = 0;
        while (s) {
            errno = 0;
            if (idx == 0 || idx == 2) {
                num = strtol(s, NULL, 10);
            } else {
                val = strtof(s, NULL);
            }
            if (idx == 0) {
                siu()->vars.HKLDCalibrationTable.a2d1 = num;
            } else if (idx == 1) {
                siu()->vars.HKLDCalibrationTable.weight1 = val;
            } else if (idx == 2) {
                siu()->vars.HKLDCalibrationTable.a2d2 = num;
            } else if (idx == 3) {
                siu()->vars.HKLDCalibrationTable.weight2 = val;
            } else if (idx == 4) {
                siu()->vars.HKLDCalibrationTable.threshold = val;
            } else if (idx == 5) {
                siu()->vars.HKLDCalibrationTable.drillStringWeight = val;
            }
            s = strtok(NULL, sep);
        }
        return YES;
    }

    return NO;
}

//---------------------------------------------------------
static bool read_mpt_table(const char *param, char *str)
{
    float val;
    if (strcasecmp(param, "MPTENC") == 0) {
        // Mpt Type
        mpt_encoding_t mpt = string_to_mpt(str);
        if (mpt != MPT_ERR) {
            siu()->vars.MPTENC = mpt;
        }
        return YES;
    }

    if (strcasecmp(param, "MPTRATE") == 0) {
        siu()->vars.MPTRATE = 0.0;    // Mpt Rate
        errno = 0;
        val = strtof(str, NULL);
        if (errno == 0) {
            siu()->vars.MPTRATE = val;
        }
        return YES;
    }

    if (strcasecmp(param, "MPTFMT") == 0) {
        mpt_format_t fmt = MPT_UNKN;
        int idx = 0;
        const char *sep = ":";
        const char *s = strtok(str, sep);
        if (s) {
            fmt = (mpt_format_t) strtol(s, NULL, 10);
            if (fmt != MPT_FORMAT) {
                return YES; // incorrect data - skip
            }
            s = strtok(NULL, sep);
            if (s) {
                idx = strtol(s, NULL, 10);
                if (idx < 1) {
                    return YES; // incorrect index - skip
                }
                idx--;
                s = strtok(NULL, sep);
            }
        }

        int fldnum = 0;
        while (s) {
            if (fldnum == 0) {
                siu()->vars.mpt.mpt_format[idx].FID = strtol(s, NULL, 16);
            } else if (fldnum == 1) {
                siu()->vars.mpt.mpt_format[idx].FIDType = strtol(s, NULL, 16);
            } else if (fldnum == 2) {
                siu()->vars.mpt.mpt_format[idx].enableSync = strtol(s, NULL, 16);
            } else if (fldnum == 3) {
                siu()->vars.mpt.mpt_format[idx].repeatCount = strtol(s, NULL, 16);
            } else if (fldnum == 4) {
                siu()->vars.mpt.mpt_format[idx].resync = strtol(s, NULL, 16);
            } else if (fldnum == 5) {
                siu()->vars.mpt.mpt_format[idx].loopback = strtol(s, NULL, 16);
            } else if (fldnum == 6) {
                siu()->vars.mpt.mpt_format[idx].alert = strtol(s, NULL, 16);
            } else if (fldnum == 7) {
                siu()->vars.mpt.mpt_format[idx].dataList[0] = strtol(s, NULL, 16);
            } else if (fldnum == 8) {
                siu()->vars.mpt.mpt_format[idx].dataList[1] = strtol(s, NULL, 16);
            } else if (fldnum == 9) {
                siu()->vars.mpt.mpt_format[idx].dataList[2] = strtol(s, NULL, 16);
            } else if (fldnum == 10) {
                siu()->vars.mpt.mpt_format[idx].dataList[3] = strtol(s, NULL, 16);
            } else if (fldnum == 11) {
                siu()->vars.mpt.mpt_format[idx].dataList[4] = strtol(s, NULL, 16);
            } else if (fldnum == 12) {
                siu()->vars.mpt.mpt_format[idx].dataList[5] = strtol(s, NULL, 16);
            } else if (fldnum == 13) {
                siu()->vars.mpt.mpt_format[idx].dataList[6] = strtol(s, NULL, 16);
            } else if (fldnum == 14) {
                siu()->vars.mpt.mpt_format[idx].dataList[7] = strtol(s, NULL, 16);
            } else if (fldnum == 15) {
                siu()->vars.mpt.mpt_format[idx].dataList[8] = strtol(s, NULL, 16);
            } else if (fldnum == 16) {
                siu()->vars.mpt.mpt_format[idx].dataList[9] = strtol(s, NULL, 16);
            } else if (fldnum == 17) {
                siu()->vars.mpt.mpt_format[idx].dataList[10] = strtol(s, NULL, 16);
            } else if (fldnum == 18) {
                siu()->vars.mpt.mpt_format[idx].dataList[11] = strtol(s, NULL, 16);
            }
            fldnum++;
            s = strtok(NULL, sep);
        }
        return YES;
    }

    if (strcasecmp(param, "MPTDAT") == 0) {
        int idx = 0;
        mpt_format_t fmt = MPT_UNKN;
        const char *sep = ":";
        const char *s = strtok(str, sep);
        if (s) {
            fmt = (mpt_format_t) strtol(s, NULL, 10);
            if (fmt != MPT_DATA) {
                return YES; // incorrect data - skip
            }
            s = strtok(NULL, sep);
            if (s) {
                idx = strtol(s, NULL, 10);
                if (idx < 1) {
                    return YES; // incorrect index - skip
                }
                idx--;
                s = strtok(NULL, sep);
            }
        }
        int fldnum = 0;
        while (s) {
            if (fldnum == 0) {
                strncpy(siu()->vars.mpt.mpt_data[idx].mnemonic, s,
                        sizeof(siu()->vars.mpt.mpt_data[idx].mnemonic));
            } else if (fldnum == 1) {
                siu()->vars.mpt.mpt_data[idx].length = strtol(s, NULL, 10);
            } else if (fldnum == 2) {
                siu()->vars.mpt.mpt_data[idx].scale_factor = strtof(s, NULL);
            } else if (fldnum == 3) {
                siu()->vars.mpt.mpt_data[idx].low_limit = strtof(s, NULL);
            } else if (fldnum == 4) {
                siu()->vars.mpt.mpt_data[idx].high_limit = strtof(s, NULL);
            } else if (fldnum == 5) {
                strncpy(siu()->vars.mpt.mpt_data[idx].units, s,
                        sizeof(siu()->vars.mpt.mpt_data[idx].units));
            } else if (fldnum == 6) {
                siu()->vars.mpt.mpt_data[idx].disp_length = strtol(s, NULL, 10);
            } else if (fldnum == 7) {
                siu()->vars.mpt.mpt_data[idx].disp_places = strtol(s, NULL, 10);
            } else if (fldnum == 8) {
                siu()->vars.mpt.mpt_data[idx].print_ctrl = strtol(s, NULL, 10);
            } else if (fldnum == 9) {
                siu()->vars.mpt.mpt_data[idx].scale_type = strtol(s, NULL, 10);
            }
            fldnum++;
            s = strtok(NULL, sep);
        }
        return YES;
    }

    return NO;
}

//---------------------------------------------------------
bool siu_read_data_status()
{
    DBGF(1, "read %s", cfg()->siu_ini_fname);

    int fd = open(cfg()->siu_ini_fname, O_RDONLY, 0640);
    if (fd == -1) {
        siu_save_data_status();
        fd = open(cfg()->siu_ini_fname, O_RDONLY, 0640);
        if (fd == -1) {
            int err = errno;
            LOG(ERROR, "can't create file %s (%s)", cfg()->siu_ini_fname, strerror(err));
            DBGF(0, "ERROR: can't create file %s (%s)", cfg()->siu_ini_fname, strerror(err));
            return FAIL;
        }
    }
    char cfgtext[4 * 1024] = {0};
    read(fd, cfgtext, sizeof(cfgtext) - 1);
    close(fd);

    int nflds;
    const char **fields;
    float val;
    int num;
    char *p, *p1, *p2;

    if (strncasecmp(cfgtext, "[SIU]\n", 6) != 0) {
        DBGF(1, "%s is incorrect - skip", cfg()->siu_ini_fname);
        return OK;
    }
    p2 = cfgtext + 5;

    do {
        if (!p2) {
            break;
        }
        p1 = p2 + 1;

        p2 = strchr(p1, '\n');
        if (p2) {
            *p2 = '\0';
        }

        p = strchr(p1, '#');
        if (p) {
            *p = '\0';
        }
        alltrim(p1);

        char *param = p1;
        char *str = NULL;
        p = strpbrk(p1, "=");
        if (!p) {
            continue;
        }

        *p = '\0';
        p++;
        str = p;

        if (read_pressure_units(param, str)) {
            continue;
        }

        if (read_calibration(param, str)) {
            continue;
        }

        if (read_mpt_table(param, str)) {
            continue;
        }

        if (read_lan_addr(param, str)) {
            continue;
        }
        if (read_WITSUNITS(param, str)) {
            continue;
        }
        if (read_WITSRAW(param, str)) {
            continue;
        }
        if (read_WITSDPCODE(param, str)) {
            continue;
        }

        // TODO - переместить все в отдельныйе функции, иначе глаза разбегаются... :(

        if (strcasecmp(param, "LOG") == 0) {
            siu()->vars.LOG = NO;
            if (strcasecmp(str, "ON") == 0 || strcasecmp(str, "1") == 0) {
                siu()->vars.LOG = YES;
            }

        } else if (strcasecmp(param, "DEN") == 0) {
            siu()->vars.DEN = NO;
            if (strcasecmp(str, "ON") == 0 || strcasecmp(str, "1") == 0) {
                siu()->vars.DEN = YES;    // Dsp Enable
            }

        } else if (strcasecmp(param, "DIE") == 0) {
            strncpy(siu()->vars.DIE, "INTERNAL", sizeof(siu()->vars.DIE));
            if (strcasecmp(str, "EXTERNAL") == 0) {
                strncpy(siu()->vars.DIE, "EXTERNAL", sizeof(siu()->vars.DIE));
            }

        } else if (strcasecmp(param, "DSS") == 0) {
            siu()->vars.DSS = NO;    // DepthTracking OFF
            if (strcasecmp(str, "ON") == 0 || strcasecmp(str, "1") == 0) {
                siu()->vars.DSS = YES;    // DepthTracking ON
            }

        } else if (strcasecmp(param, "EQF") == 0) {
            // Equalizer OFF
            siu()->vars.EQF = NO;
            if (strcasecmp(str, "ON") == 0 || strcasecmp(str, "1") == 0) {
                siu()->vars.EQF = YES;
            }

        } else if (strcasecmp(param, "TTR") == 0) {
            // Time Tracking
            siu()->vars.TTR = NO;
            if (strcasecmp(str, "ON") == 0 || strcasecmp(str, "1") == 0) {
                siu()->vars.TTR = YES;
            }

        } else if (strcasecmp(param, "FLT") == 0) {
            // Filter Correlation Type (01 / 02)
            siu()->vars.FLT = 1;
            errno = 0;
            num = strtol(str, NULL, 10);
            if (errno == 0) {
                if (num == 2) {
                    siu()->vars.FLT = num;
                }
            }

        } else if (strcasecmp(param, "SPS") == 0) {
            // Pressure Scale
            errno = 0;
            num = strtol(str, NULL, 10);
            if (errno == 0) {
                siu()->vars.SPS = num;
            }

        } else if (strcasecmp(param, "SPP") == 0) {
            // StandpipePressureCalibration (01 / 02)
            // SPP=01:02 => [@SPP:01:02]
            char item[512] = {0};
            strlcpy(item, "[@", sizeof(item));
            strncat(item + 2, str, sizeof(item) - 3);
            char *ptr = strchr(item, '=');
            if (ptr) {
                *ptr = ':';
            }
            fields = siu_get_fields(item, &nflds); // TO DO FIXIT
            if (nflds == 3) {
                errno = 0;
                num = strtol(fields[1], NULL, 16);
                if (errno == 0) {
                    siu()->vars.SPP[0] = num;
                }
                errno = 0;
                num = strtol(fields[2], NULL, 16);
                if (errno == 0) {
                    siu()->vars.SPP[1] = num;
                }
            }

        } else if (strcasecmp(param, "POT") == 0) {
            // Pumps On Pressure
            errno = 0;
            num = strtol(str, NULL, 10);
            if (errno == 0) {
                siu()->vars.POT = num;
            }

        } else if (strcasecmp(param, "PSS") == 0) {
            // Pump Status
            siu()->vars.PSS = NO;
            if (strcasecmp(str, "ON") == 0 || strcasecmp(str, "1") == 0) {
                siu()->vars.PSS = YES;
            }

        // TODO TOFIX } else if (strcasecmp(param, "DEC") == 0) {
        // TODO TOFIX     // DEC / Depth Encoder Calibration
        // TODO TOFIX     errno = 0;
        // TODO TOFIX     snum = (short) strtol(str, NULL, 10);
        // TODO TOFIX     if (errno == 0) {
        // TODO TOFIX         siu()->vars.? = snum;
        // TODO TOFIX     }

        } else if (strcasecmp(param, "DFS") == 0) {
            // Force Drill States
            siu()->vars.DFS = NoForcing;
            errno = 0;
            num = strtol(str, NULL, 10);
            if (errno == 0) {
                if (num == NoForcing || num == ForceInSlips || num == ForceOutSlips) {
                    siu()->vars.DFS = num;
                }
            }

        } else if (strcasecmp(param, "OnBottomThreshold") == 0) {
            siu()->vars.OnBottomThreshold = 0.0;    // OnBottom Threshold
            errno = 0;
            val = strtof(str, NULL);
            if (errno == 0) {
                siu()->vars.OnBottomThreshold = val;
            }

        } else if (strcasecmp(param, "BitDepth") == 0) {
            siu()->vars.DepthSystemData.BitDepth = 0.0f;
            errno = 0;
            val = strtof(str, NULL);
            if (errno == 0) {
                siu()->vars.DepthSystemData.BitDepth = val;
            }

        } else if (strcasecmp(param, "HoleDepth") == 0) {
            siu()->vars.DepthSystemData.HoleDepth = 0.0f;
            errno = 0;
            val = strtof(str, NULL);
            if (errno == 0) {
                siu()->vars.DepthSystemData.HoleDepth = val;
            }

        } else if (strcasecmp(param, "KellyDownDepth") == 0) {
            siu()->vars.DepthSystemData.KellyDownDepth = 0.0f;
            errno = 0;
            val = strtof(str, NULL);
            if (errno == 0) {
                siu()->vars.DepthSystemData.KellyDownDepth = val;
            }

        } else if (strcasecmp(param, "BlockPosition") == 0) {
            siu()->vars.DepthSystemData.BlockPosition = 0.0f;
            errno = 0;
            val = strtof(str, NULL);
            if (errno == 0) {
                siu()->vars.DepthSystemData.BlockPosition = val;
            }

        } else if (strcasecmp(param, "BlockSpeed") == 0) {
            siu()->vars.DepthSystemData.BlockSpeed = 0.0f;
            errno = 0;
            val = strtof(str, NULL);
            if (errno == 0) {
                siu()->vars.DepthSystemData.BlockSpeed = val;
            }

        } else if (strcasecmp(param, "Rop1s") == 0) {
            siu()->vars.DepthSystemData.Rop1s = 0.0f;
            errno = 0;
            val = strtof(str, NULL);
            if (errno == 0) {
                siu()->vars.DepthSystemData.Rop1s = val;
            }

        } else if (strcasecmp(param, "Rop5s") == 0) {
            siu()->vars.DepthSystemData.Rop5s = 0.0f;
            errno = 0;
            val = strtof(str, NULL);
            if (errno == 0) {
                siu()->vars.DepthSystemData.Rop5s = val;
            }

        } else if (strcasecmp(param, "WeightOnBit") == 0) {
            siu()->vars.DepthSystemData.WeightOnBit = 0.0f;
            errno = 0;
            val = strtof(str, NULL);
            if (errno == 0) {
                siu()->vars.DepthSystemData.WeightOnBit = val;
            }

        } else if (strcasecmp(param, "Hookload") == 0) {
            // Hookload Weights
            errno = 0;
            val = strtof(str, NULL);
            if (errno == 0) {
                if (fpclassify(val) != FP_NORMAL) {
                    val = 0.0f;
                }
                siu()->vars.DepthSystemData.Hookload = val;
            }

        } else if (strcasecmp(param, "DrillState") == 0) {
            siu()->vars.DepthSystemData.DrillState = 0;
            errno = 0;
            num = strtol(str, NULL, 10);
            if (errno == 0) {
                siu()->vars.DepthSystemData.DrillState = num;
            }

        } else if (strcasecmp(param, "OnBottom") == 0) {
            siu()->vars.DepthSystemData.OnBottom = 0;
            errno = 0;
            num = strtol(str, NULL, 10);
            if (errno == 0) {
                siu()->vars.DepthSystemData.OnBottom = num;
            }

        } else if (strcasecmp(param, "InSlips") == 0) {
            siu()->vars.DepthSystemData.InSlips = 0;
            errno = 0;
            num = strtol(str, NULL, 10);
            if (errno == 0) {
                siu()->vars.DepthSystemData.InSlips = num;
            }

        } else if (strcasecmp(param, "HookloadA2D") == 0) {
            // Hookload from ADC
            errno = 0;
            num = strtol(str, NULL, 10);
            if (errno == 0) {
                siu()->vars.HookloadA2D = num;
            }

            //} else if (strcasecmp(param, "SDU") == 0) {
            //    // System Uniots
            //    if (str[0] == 'M') {
            //        siu2()->vars.SDU = UnitsMetric;
            //    } else if (str[0] == 'E') {
            //        siu2()->vars.SDU = UnitsEnglish;
            //    }
            //
            //} else if (strcasecmp(param, "STS") == 0) {
            //    // STS=FF:FF => [@STS:FF:FF]
            //    const char *sep = ":";
            //    const char *s = strtok(str, sep);
            //    idx = 0;
            //    uint32_t status;
            //    while (s) {
            //        if (idx == 0) {
            //            errno = 0;
            //            status = strtol(s, NULL, 16);
            //            if (errno == 0) {
            //                siu_update_dev_status(status);
            //            }
            //        } else if (idx == 1) {
            //            errno = 0;
            //            status = strtol(s, NULL, 16);
            //            if (errno == 0) {
            //                siu_update_system_status_flags(status);
            //            }
            //        }
            //        idx++;
            //        s = strtok(NULL, sep);
            //    }
        }

    } while (1);

    if (debug_level >= 8) {
        int dbgn = 8;
        DBGF(dbgn, "LOG:          %s           ", (siu()->vars.LOG ? "ON" : "OFF"));
        DBGF(dbgn, "SDU:          %s           ", (siu()->vars.SDU == UnitsMetric ? "Metric" : "English"));
        DBGF(dbgn, "DEN:          %s           ", (siu()->vars.DEN ? "ON" : "OFF"));
        DBGF(dbgn, "DSS:          %s           ", (siu()->vars.DSS ? "ON" : "OFF"));
        DBGF(dbgn, "EQF:          %s           ", (siu()->vars.EQF ? "ON" : "OFF"));
        DBGF(dbgn, "TTR:          %s           ", (siu()->vars.TTR ? "ON" : "OFF"));
        DBGF(dbgn, "DIE:          %s           ", siu()->vars.DIE);
        DBGF(dbgn, "FLT:          %02d         ", siu()->vars.FLT);
        DBGF(dbgn, "SPS:          %d           ", siu()->vars.SPS);
        DBGF(dbgn, "STS:          x%08X x%08X  ", siu_dev_status(), siu_system_status());
        DBGF(dbgn, "DFS:          %d           ", siu()->vars.DFS);
        DBGF(dbgn, "PSS:          %s           ", (siu()->vars.PSS ? "ON" : "OFF"));
        DBGF(dbgn, "POT:          %d           ", siu()->vars.POT);
        DBGF(dbgn, "DER:          %u (Depth Encoder Counts) ", siu()->vars.DER);
        DBGF(dbgn, "HookloadA2D:  %05d         ", siu()->vars.HookloadA2D);
        DBGNoNL(dbgn, "DepthCalibrationTable: ");
        for (int i = 0; i < 32; i++) {
            printf("%d:%0.2f ", siu()->vars.DepthCalibrationTable.points[i].count,
                   siu()->vars.DepthCalibrationTable.points[i].value);
        }
        puts("");
        DBGNL(dbgn);

        DBGF(dbgn, "OnBottomThreshold:    %0.2f", siu()->vars.OnBottomThreshold);
        DBGF(dbgn, "Pressure Units:       %s%s%s%s%s",
             (siu()->vars.PressureUnits == PRESSURE_PSI ? "PSI" : ""),
             (siu()->vars.PressureUnits == PRESSURE_KPA ? "KPA" : ""),
             (siu()->vars.PressureUnits == PRESSURE_BAR ? "BAR" : ""),
             (siu()->vars.PressureUnits == PRESSURE_ATM ? "ATM" : ""),
             (siu()->vars.PressureUnits == PRESSURE_KGCM2 ? "KGCM2" : "") );
        DBGNL(dbgn);

        DBGF(dbgn, "HSC.a2d1:             %05d  ", siu()->vars.HKLDCalibrationTable.a2d1);
        DBGF(dbgn, "HSC.weight1:          %05.1f", siu()->vars.HKLDCalibrationTable.weight1);
        DBGF(dbgn, "HSC.a2d2:             %05d  ", siu()->vars.HKLDCalibrationTable.a2d2);
        DBGF(dbgn, "HSC.weight2:          %05.1f", siu()->vars.HKLDCalibrationTable.weight2);
        DBGF(dbgn, "HSC.threshold:        %05.1f", siu()->vars.HKLDCalibrationTable.threshold);
        DBGF(dbgn, "HSC.drillWeight:      %05.1f", siu()->vars.HKLDCalibrationTable.drillStringWeight);
        DBGNL(dbgn);

        DBGF(dbgn, "MPTENC:               %s           ", mpt_type_to_string(siu()->vars.MPTENC));
        DBGF(dbgn, "MPTRATE:              %05.2f       ", siu()->vars.MPTRATE);
        DBGNL(dbgn);

        DBG(dbgn, "DepthSystemData (DSD):");
        DBGF(dbgn, "DSD.BitDepth:         %0.2f", siu()->vars.DepthSystemData.BitDepth);
        DBGF(dbgn, "DSD.HoleDepth:        %0.2f", siu()->vars.DepthSystemData.HoleDepth);
        DBGF(dbgn, "DSD.BlockPosition:    %0.2f", siu()->vars.DepthSystemData.BlockPosition);
        DBGF(dbgn, "DSD.BlockSpeed:       %0.2f", siu()->vars.DepthSystemData.BlockSpeed);
        DBGF(dbgn, "DSD.Rop1s:            %0.2f", siu()->vars.DepthSystemData.Rop1s);
        DBGF(dbgn, "DSD.Rop5s:            %0.2f", siu()->vars.DepthSystemData.Rop5s);
        DBGF(dbgn, "DSD.WeightOnBit:      %0.2f", siu()->vars.DepthSystemData.WeightOnBit);
        DBGF(dbgn, "DSD.Hookload:         %0.2f", siu()->vars.DepthSystemData.Hookload);
        DBGF(dbgn, "DSD.DrillState:       %d   ", siu()->vars.DepthSystemData.DrillState);
        DBGF(dbgn, "DSD.KellyDownDepth:   %0.2f", siu()->vars.DepthSystemData.KellyDownDepth);
        DBGNL(dbgn);

        DBGF(dbgn, "DSDE.WobAvg:          %0.2f", siu()->vars.DSDE.WobAvg);
        DBGF(dbgn, "DSDE.HkldAvg:         %0.2f", siu()->vars.DSDE.HkldAvg);
        DBGF(dbgn, "DSDE.Torque:          %0.2f", siu()->vars.DSDE.Torque);
        DBGF(dbgn, "DSDE.RPM:             %0.2f", siu()->vars.DSDE.RPM);
        DBGF(dbgn, "DSDE.PumpStrokeRate1: %0.2f", siu()->vars.DSDE.PumpStrokeRate1);
        DBGF(dbgn, "DSDE.PumpStrokeRate2: %0.2f", siu()->vars.DSDE.PumpStrokeRate2);
        DBGF(dbgn, "DSDE.PumpStrokeRate3: %0.2f", siu()->vars.DSDE.PumpStrokeRate3);
        DBGF(dbgn, "DSDE.MudFlowInAvg:    %0.2f", siu()->vars.DSDE.MudFlowInAvg);
        DBGF(dbgn, "DSDE.MudFlowOutAvg:   %0.2f", siu()->vars.DSDE.MudFlowOutAvg);
        DBGF(dbgn, "DSDE.DiffPressure:    %0.2f", siu()->vars.DSDE.DifferentialPressure);
        DBGNL(dbgn);

        DBGF(dbgn, "WitsDepthUnits:       %s", (siu()->vars.WitsDepthUnits == UnitsEnglish ? "FEET" : "METERS"));
        DBGF(dbgn, "WitsFlowUnits:        %s", (siu()->vars.WitsFlowUnits == UnitsEnglish ? "GPM" : "LPM"));
        DBGF(dbgn, "WitsHkldUnits:        %s", (siu()->vars.WitsHkldUnits == UnitsEnglish ? "KLB" : "KDN"));
        DBGF(dbgn, "WitsTorqueUnits:      %s%s%s%s",
             (siu()->vars.WitsTorqueUnits == TORQUE_NM ? "NM" : ""),
             (siu()->vars.WitsTorqueUnits == TORQUE_KNM ? "KNM" : ""),
             (siu()->vars.WitsTorqueUnits == TORQUE_FTLB ? "FTLB" : ""),
             (siu()->vars.WitsTorqueUnits == TORQUE_KFTLB ? "KFTLB" : "") );
        DBGF(dbgn, "WitsRaw:              %s ", (siu()->vars.WitsRaw ? "ON" : "OFF"));
        DBGF(dbgn, "WitsOutput:           %s ", (siu()->vars.WitsOutput ? "ON" : "OFF"));
        DBGNL(dbgn);
    }
    return OK;
}

//---------------------------------------------------------
bool siu_save_data_status()
{
    const char *pname = NULL;
    char buf[1024] = {0};
    DBGF(3, "save %s", cfg()->siu_ini_fname);
    int fd = open(cfg()->siu_ini_fname, O_CREAT | O_TRUNC | O_WRONLY, 0640);
    if (fd == -1) {
        int err = errno;
        LOG(ERROR, "can't open file %s (%s)", cfg()->siu_ini_fname, strerror(err));
        DBGF(0, "ERROR: can't open file %s (%s)\n", cfg()->siu_ini_fname, strerror(err));
        return FAIL;
    }
    write(fd, "[SIU]\n", 6);

    snprintf(buf, sizeof(buf), "SDU=%s\n", (siu()->vars.SDU == UnitsMetric ? "M" : "E"));
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "APSLAN=%s:%s:%s:%s\n",
             (siu()->vars.apslan.NetApsIpSource == DHCP ? "DHCP" : "FIXED"),
             siu()->vars.apslan.NetApsIp,
             siu()->vars.apslan.NetApsNetmask,
             siu()->vars.apslan.NetApsGateway);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "RIGLAN=%s:%s:%s:%s\n",
             (siu()->vars.riglan.NetRigIpSource == DHCP ? "DHCP" : "FIXED"),
             siu()->vars.riglan.NetRigIp,
             siu()->vars.riglan.NetRigNetmask,
             siu()->vars.riglan.NetRigGateway);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "RFDADDR=1:%s\n", siu()->vars.rfd1Ip);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "RFDADDR=2:%s\n", siu()->vars.rfd2Ip);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "DIE=%s\n", siu()->vars.DIE);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "DSS=%s\n", (siu()->vars.DSS ? "ON" : "OFF"));
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "DEN=%s\n", (siu()->vars.DEN ? "ON" : "OFF"));
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "EQF=%s\n", (siu()->vars.EQF ? "ON" : "OFF"));
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "LOG=%s\n", (siu()->vars.LOG ? "ON" : "OFF"));
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "TTR=%s\n", (siu()->vars.TTR ? "ON" : "OFF"));
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "PSS=%s\n", (siu()->vars.PSS ? "ON" : "OFF"));
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "POT=%d\n", siu()->vars.POT);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "FLT=%d\n", siu()->vars.FLT);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "DFS=%d\n", siu()->vars.DFS);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "SPS=%d\n", siu()->vars.SPS);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "STS=%02X:%02X\n", siu_dev_status(), siu_system_status());
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "BitDepth=%0.2f\n", siu()->vars.DepthSystemData.BitDepth);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "HoleDepth=%0.2f\n", siu()->vars.DepthSystemData.HoleDepth);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "KellyDownDepth=%0.2f\n", siu()->vars.DepthSystemData.KellyDownDepth);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "BlockPosition=%0.2f\n", siu()->vars.DepthSystemData.BlockPosition);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "BlockSpeed=%0.2f\n", siu()->vars.DepthSystemData.BlockSpeed);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "Rop1s=%0.2f\n", siu()->vars.DepthSystemData.Rop1s);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "Rop5s=%0.2f\n", siu()->vars.DepthSystemData.Rop5s);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "WeightOnBit=%0.2f\n", siu()->vars.DepthSystemData.WeightOnBit);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    float val = siu()->vars.DepthSystemData.Hookload;
    if (fpclassify(val) != FP_NORMAL) {
        val = 0.0f;
    }
    snprintf(buf, sizeof(buf), "Hookload=%0.2f\n", val);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "DrillState=%d\n", siu()->vars.DepthSystemData.DrillState);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "OnBottom=%d\n", siu()->vars.DepthSystemData.OnBottom);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "InSlips=%d\n", siu()->vars.DepthSystemData.InSlips);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "OnBottomThreshold=%0.2f\n", siu()->vars.OnBottomThreshold);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "PressureUnits=%s%s%s%s%s\n",
             (siu()->vars.PressureUnits == PRESSURE_KGCM2 ? "KGCM2" : ""),
             (siu()->vars.PressureUnits == PRESSURE_ATM ? "ATM" : ""),
             (siu()->vars.PressureUnits == PRESSURE_BAR ? "BAR" : ""),
             (siu()->vars.PressureUnits == PRESSURE_KPA ? "KPA" : ""),
             (siu()->vars.PressureUnits == PRESSURE_PSI ? "PSI" : "") );
    DBGF(9, "%s", buf);
    write(fd, buf, strlen(buf));

    // DEC
    pname = "DepthCalibrationTable=";
    DBGFNoNL(9, "%s", pname);
    write(fd, pname, strlen(pname));
    char ch = ':';
    for (int i = 0; i < 32; i++) {
        if (i == 31) {
            ch = '\0';
        }
        //  [@DEC:+00001:+001.00:+00100:+003.94: ...
        snprintf(buf, sizeof(buf), "%s%05d:%s%06.2f%c",
                 (siu()->vars.DepthCalibrationTable.points[i].count >= 0 ? "+" : ""),
                 siu()->vars.DepthCalibrationTable.points[i].count,
                 (siu()->vars.DepthCalibrationTable.points[i].value >= 0 ? "+" : ""),
                 siu()->vars.DepthCalibrationTable.points[i].value,
                 ch);
        write(fd, buf, strlen(buf));
        if (debug_level >= 9) {
            printf("%s", buf);
        }
    }
    DBGNL(9);

    //---------------------------------
    write(fd, "# HSC\n", 6);
    pname = "HKLDCalibrationTable=";
    DBGFNoNL(9, "%s", pname);
    write(fd, pname, strlen(pname));
    snprintf(buf, sizeof(buf), "%05d:%05.1f:%05d:%05.1f:%05.1f:%05.1f",
             siu()->vars.HKLDCalibrationTable.a2d1,
             siu()->vars.HKLDCalibrationTable.weight1,
             siu()->vars.HKLDCalibrationTable.a2d2,
             siu()->vars.HKLDCalibrationTable.weight2,
             siu()->vars.HKLDCalibrationTable.threshold,
             siu()->vars.HKLDCalibrationTable.drillStringWeight);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));
    DBGNL(9);

    //---------------------------------
    pname = "\n\n[WITS]\n";
    DBGFNoNL(9, "%s", pname);
    write(fd, pname, strlen(pname));

    //---------------------------------
    snprintf(buf, sizeof(buf), "WitsDepthUnits=%s\n", (siu()->vars.WitsDepthUnits == UnitsEnglish ? "E" : "M"));
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "WitsFlowUnits=%s\n", (siu()->vars.WitsFlowUnits == UnitsEnglish ? "E" : "M"));
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "WitsHkldUnits=%s\n", (siu()->vars.WitsHkldUnits == UnitsEnglish ? "E" : "M"));
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "WitsTorqueUnits=%s%s%s%s\n",
             (siu()->vars.WitsTorqueUnits == TORQUE_KFTLB ? "KFTLB   # Torque" : ""),
             (siu()->vars.WitsTorqueUnits == TORQUE_FTLB ? "FTLB    # Torque" : ""),
             (siu()->vars.WitsTorqueUnits == TORQUE_KNM ? "KNM     # Torque" : ""),
             (siu()->vars.WitsTorqueUnits == TORQUE_NM ? "NM      # Torque" : "")
    );
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));


    snprintf(buf, sizeof(buf), "WITSRAW=%s\n", (siu()->vars.WitsRaw ? "ON" : "OFF"));
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "WITSDPCODE=%s:%02d:%02d\n",
             (siu()->vars.WitsDiffPressCode.enabled ? "ON" : "OFF"),
             siu()->vars.WitsDiffPressCode.record,
             siu()->vars.WitsDiffPressCode.item);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    //---------------------------------
    pname = "\n[MPT]\n";
    DBGFNoNL(9, "%s", pname);
    write(fd, pname, strlen(pname));

    //---------------------------------
    snprintf(buf, sizeof(buf), "MPTENC=%s\n", mpt_type_to_string(siu()->vars.MPTENC));
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    snprintf(buf, sizeof(buf), "MPTRATE=%0.2f\n", siu()->vars.MPTRATE);
    DBGFNoNL(9, "%s", buf);
    write(fd, buf, strlen(buf));

    for (int idx = 0; idx < MPT_FORMAT_SIZE; idx++) {
        snprintf(buf, sizeof(buf),
                 "MPTFMT=01:%02d:%04X:%04X:%04X:%04X:%04X:%04X:%04X:"
                 "%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X\n",
                 (idx + 1),
                 siu()->vars.mpt.mpt_format[idx].FID,
                 siu()->vars.mpt.mpt_format[idx].FIDType,
                 siu()->vars.mpt.mpt_format[idx].enableSync,
                 siu()->vars.mpt.mpt_format[idx].repeatCount,
                 siu()->vars.mpt.mpt_format[idx].resync,
                 siu()->vars.mpt.mpt_format[idx].loopback,
                 siu()->vars.mpt.mpt_format[idx].alert,
                 siu()->vars.mpt.mpt_format[idx].dataList[0],
                 siu()->vars.mpt.mpt_format[idx].dataList[1],
                 siu()->vars.mpt.mpt_format[idx].dataList[2],
                 siu()->vars.mpt.mpt_format[idx].dataList[3],
                 siu()->vars.mpt.mpt_format[idx].dataList[4],
                 siu()->vars.mpt.mpt_format[idx].dataList[5],
                 siu()->vars.mpt.mpt_format[idx].dataList[6],
                 siu()->vars.mpt.mpt_format[idx].dataList[7],
                 siu()->vars.mpt.mpt_format[idx].dataList[8],
                 siu()->vars.mpt.mpt_format[idx].dataList[9],
                 siu()->vars.mpt.mpt_format[idx].dataList[10],
                 siu()->vars.mpt.mpt_format[idx].dataList[11]);
        write(fd, buf, strlen(buf));
    }

    for (int idx = 0; idx < MPT_DATA_SIZE; idx++) {
        snprintf(buf, sizeof(buf),
                 "MPTDAT=00:%02d:%s:%04d:%0.3f:%0.6f:%0.6f:%s:%02d:%02d:%02d:%02d\n",
                 (idx + 1),
                 siu()->vars.mpt.mpt_data[idx].mnemonic,
                 siu()->vars.mpt.mpt_data[idx].length,
                 siu()->vars.mpt.mpt_data[idx].scale_factor,
                 siu()->vars.mpt.mpt_data[idx].low_limit,
                 siu()->vars.mpt.mpt_data[idx].high_limit,
                 siu()->vars.mpt.mpt_data[idx].units,
                 siu()->vars.mpt.mpt_data[idx].disp_length,
                 siu()->vars.mpt.mpt_data[idx].disp_places,
                 siu()->vars.mpt.mpt_data[idx].print_ctrl,
                 siu()->vars.mpt.mpt_data[idx].scale_type);
        write(fd, buf, strlen(buf));
    }

    //---------------------------------
    write(fd, "\n", 1);
    close(fd);
    return OK;
}
