/**
    \file   adc.c
    \author Aleksey Lekhtin
    \date   18.09.2025
    \brief  Работа с АЦП установленном на ESZ7 плате (с XME0724-20 Soc)
*/

#include <stdio.h>

#include "adc.h"

#include <string.h>

#include "cfg.h"
#include "dbg.h"
#include "calculateCRC.h"
#include "utils.h"
#include "log.h"

static volatile DevCmd_t *dev_ctrl_adr = NULL;
static volatile DevStat_t *dev_stat_adr = NULL;
static volatile uint32_t *adc_regtr_lo = NULL;
static volatile uint32_t *adc_regtr_hi = NULL;
static volatile uint32_t *fpga_ver_reg = NULL;

static uint8_t m_arbitrary = 0;

//---------------------------------------------------------
void setup_registers_address(void *reg_mem_map)
{
    fpga_ver_reg = (volatile uint32_t *) ((char *) reg_mem_map + FMWREG_ADDR);
    dev_ctrl_adr = (volatile DevCmd_t *) ((char *) reg_mem_map + DEVCTRL_ADDR);
    dev_stat_adr = (volatile DevStat_t *) ((char *) reg_mem_map + DEVSTAT_ADDR);
    adc_regtr_lo = (volatile uint32_t *) ((char *) reg_mem_map + ADCREGL_ADDR);
    adc_regtr_hi = (volatile uint32_t *) ((char *) reg_mem_map + ADCREGH_ADDR);
    DBGF(2, "Firmware Vers:     %p (0x%02x)", fpga_ver_reg, FMWREG_ADDR);
    DBGF(2, "Device status:     %p (0x%02x)", dev_stat_adr, DEVSTAT_ADDR);
    DBGF(2, "ADC register LO:   %p (0x%02x)", adc_regtr_lo, ADCREGL_ADDR);
    DBGF(2, "ADC register Hi:   %p (0x%02x)", adc_regtr_hi, ADCREGH_ADDR);
    DBGF(2, "Device control:    %p (0x%02x)", dev_ctrl_adr, DEVCTRL_ADDR);
}

//---------------------------------------------------------
uint32_t read_firmware_version()
{
    volatile const uint32_t data = *fpga_ver_reg;
    return data;
}

//---------------------------------------------------------
static void clear_arbitrary_value()
{
    m_arbitrary = 0;
}

//---------------------------------------------------------
static uint8_t new_arbitrary()
{
    m_arbitrary++;
    if (m_arbitrary == 0) {
        m_arbitrary = 1;
    }
    return m_arbitrary;
}

//---------------------------------------------------------
static bool wait_adc_ready(int channel)
{
    // channel: 1..4
    int counter = 0;
    DevStat_t stat = {0};
    while (1) {
        dev_read_status(&stat);
        uint32_t mask = (1 << (channel - 1));
        bool err = (stat.bits.DevErr & mask ? YES : NO);
        if (err) {
            LOG(ERROR, "ExtDev-%d has an error", channel);
            DBGF(0, "ERROR: ExtDev-%d has an error", channel);
            return FAIL;
        }
        bool busy = (stat.bits.AdcBusy & mask ? YES : NO);
        DBGF(9, "[%d] ADC-%d - mask: 0x%02X, AdcBusy: 0x%08X => busy: %d",
             counter + 1, channel, mask, stat.bits.AdcBusy, busy);
        if (!busy) {
            break;
        }
        counter++;
        if (counter >= 200) {
            LOG(ERROR, "ADC-%d not ready", channel);
            DBGF(0, "ERROR: ADC-%d not ready", channel);
            return FAIL;
        }
        nanosleep(&(struct timespec){.tv_sec = 0, .tv_nsec = TS_100US}, NULL);
    }
    return OK;
}

//---------------------------------------------------------
static bool CmdDirectCommand(AdcRequset_t *req)
{
    req->resp = (AdcResp_t){0};
    DBGF(9, "-- ADC-%u %s(%u), obj: 0x%02X",
         req->channel, cmd_name(req->cmdop), req->cmdop, req->data);

    // channel: 1..4
    if (req->channel > 4) {
        strncpy(req->errors, "invalid ADC number", sizeof(req->errors) - 1);
        return FAIL;
    }

    if (req->cmdop == CMDOP_RESET) {
        uint32_t value = *adc_regtr_hi; // to reset FPGA State Machine.
        DBGF(9, "fpga state reset - adc_reg_hi: 0x%02X", value);
    }

    if ((req->cmdop & CMDOP_RREG) || (req->cmdop & CMDOP_WREG)) {
        uint8_t reg_num = 0x7F; // let it be wrong number
        if (req->cmdop & CMDOP_RREG) {
            reg_num = req->cmdop & (~CMDOP_RREG);
        } else if (req->cmdop & CMDOP_WREG) {
            reg_num = req->cmdop & (~CMDOP_WREG);
        }
        if (reg_num > 0x12) {
            strncpy(req->errors, "Invalid command - allowed numbers is 0..12h", sizeof(req->errors) - 1);
            return FAIL;
        }
    }

    if (!(req->cmdop & CMDOP_WREG)) {
        req->data = new_arbitrary();
    }

    // 1. send command
    CmdParam_t cmd_param;
    cmd_param.bits.AdcCommand = req->cmdop;
    cmd_param.bits.AdcCmdData = req->data;
    cmd_param.bits.AdcCmdCrc = calculateCRC((uint8_t *) &cmd_param.value, 2);
    req->crc = cmd_param.bits.AdcCmdCrc;

    req->cmd.bits.CmdOpCode = CmdSetDirectCommand;
    //req->cmd.bits.CmdParam = param[0] | (param[1] << 8) | (param[2] << 16);
    req->cmd.bits.CmdParam = cmd_param.value;
    req->cmd.bits.AdcChannels = channel_to_onehot(req->channel);

    DBGF(9, "DevCmd::%p <= 0x%08X (%s:%u, ch:%u, cmdop: 0x%02X (%s), obj: 0x%02X, crc: 0x%02X)",
         dev_ctrl_adr, req->cmd.value, CmdOpCodeName(req->cmd.bits.CmdOpCode), req->cmd.bits.CmdOpCode,
         req->cmd.bits.AdcChannels, (req->cmd.bits.CmdParam >> 0) & 0x00FF,
         cmd_name(req->cmdop), (req->cmd.bits.CmdParam >> 8) & 0x00FF,
         (req->cmd.bits.CmdParam >> 16) & 0x00FF);

    *dev_ctrl_adr = req->cmd;

    // 2.
    DBGF(9, "wait_adc_ready( ADC-%d )", req->channel);
    if (!wait_adc_ready(req->channel)) {
        snprintf(req->errors, sizeof(req->errors), "ADC-%d not ready", req->channel);
        return FAIL;
    }

    // 3. set response
    uint32_t valueLo = *adc_regtr_lo;
    uint32_t valueHi = *adc_regtr_hi;
    DBGF(9, "AdcRegLo::%p => 0x%08X", adc_regtr_lo, valueLo);
    DBGF(9, "AdcRegHi::%p => 0x%08X", adc_regtr_hi, valueHi);
    req->resp.value = ((uint64_t) valueHi << 32) | valueLo;
    return OK; // no errors
}

//---------------------------------------------------------
static bool check_adc_response(AdcRequset_t *req)
{
    AdcResp_t *resp = &req->resp;

    // 1. check for errors
    bool err = NO;
    if (resp->bits.EchoCmd != req->cmdop)
        err = YES;
    if (resp->bits.EchoData != req->data)
        err = YES;
    if (resp->bits.EchoCrc != req->crc)
        err = YES;
    if (resp->bits.AdcStatus & 0x40)
        err = YES; // CRC1 Error
    if (resp->bits.AdcStatus & 0x10)
        err = YES; // STAT12 Error Flag

    if (err) {
        if (debug_level == 0) {
            DBGF(5, "DevCmd::%p <= 0x%08X (%s:%u, ch:%u, cmdop: 0x%02X (%s), obj: 0x%02X, crc: 0x%02X)",
                 dev_ctrl_adr,
                 req->cmd.value,
                 CmdOpCodeName(req->cmd.bits.CmdOpCode),
                 req->cmd.bits.CmdOpCode,
                 req->cmd.bits.AdcChannels,
                 (req->cmd.bits.CmdParam >> 0) & 0x00FF,
                 cmd_name(req->cmdop),
                 (req->cmd.bits.CmdParam >> 8) & 0x00FF,
                 (req->cmd.bits.CmdParam >> 16) & 0x00FF);
        }
        if (debug_level >= 4) {
            print_AdcResp(&req->resp);
        }
    }

    // 2. check echo error
    if (err) {
        bool echo_error = NO;
        if (resp->bits.EchoCmd != req->cmdop) {
            echo_error = YES;
            snprintf(req->errors, sizeof(req->errors),
                     "ERROR: Echo CMD  mismatch (0x%02X != 0x%02X)",
                     resp->bits.EchoCmd, req->cmdop);
        } else if (resp->bits.EchoData != req->data) {
            echo_error = YES;
            snprintf(req->errors, sizeof(req->errors),
                     "ERROR: Echo DATA mismatch (0x%02X != 0x%02X)",
                     resp->bits.EchoData, req->data);
        } else if (resp->bits.EchoCrc != req->crc) {
            echo_error = YES;
            if (resp->bits.EchoCrc == (uint8_t) ~req->crc) {
                uint8_t param[3];
                param[0] = req->cmdop;
                param[1] = req->data;
                uint8_t crc = calculateCRC(param, 2);
                snprintf(req->errors, sizeof(req->errors),
                         "ERROR: Echo CRC  mismatch (0x%02X != 0x%02X)\n"
                         "       error in request CRC (0x%02X, expected 0x%02X)",
                         resp->bits.EchoCrc, req->crc,
                         resp->bits.EchoCrc, crc);
            } else {
                snprintf(req->errors, sizeof(req->errors),
                         "ERROR: Echo CRC  mismatch (0x%02X != 0x%02X)",
                         resp->bits.EchoCrc, req->crc);
            }
        }

        if (!echo_error) {
            if (debug_level >= 4) {
                // channel: 1..4
                print_AdcStatus(req->channel, resp->bits.AdcStatus, 0);
            }
        }

        return FAIL;
    }

    // 6. check response obj CRC and update errors description
    uint8_t res[4] = {0};
    uint8_t crc;

    if (req->cmdop == CMDOP_RDATA) {
        res[0] = resp->bits.AdcStatus;
        res[1] = resp->bits.AdcMsbData;
        res[2] = resp->bits.AdcMidData;
        res[3] = resp->bits.AdcLsbData;
        crc = calculateCRC(res, 4);
        if (crc != resp->bits.AdcCrcResp) {
            err = YES;
            if (debug_level >= 4) {
                print_AdcResp(resp);
            }
            snprintf(req->errors, sizeof(req->errors),
                     "ERROR: CRC mismatch (0x%02X != 0x%02X)\n"
                     "Verification CRC for '%02X %02X %02X %02X' => 0x%02X",
                     crc, resp->bits.AdcCrcResp, res[0], res[1], res[2], res[3], crc);
        }
    } else if (req->cmdop & CMDOP_RREG) {
        res[0] = resp->bits.AdcLsbData;
        crc = calculateCRC(res, 1);
        if (crc != resp->bits.AdcCrcResp) {
            err = YES;
            if (debug_level >= 4) {
                print_AdcResp(resp);
            }
            snprintf(req->errors, sizeof(req->errors),
                     "Verification CRC for 'Lsb: %02X' => 0x%02X\n"
                     "ERROR: CRC mismatch (0x%02X != Out CRC:0x%02X)",
                     res[0], crc, crc, resp->bits.AdcCrcResp);
        }
    } else if (req->cmdop & CMDOP_WREG) {
        if (resp->bits.EchoCrc != req->crc) {
            err = YES;
            if (debug_level >= 4) {
                print_AdcResp(resp);
            }
            snprintf(req->errors, sizeof(req->errors),
                     "ERROR: CRC mismatch (0x%02X != 0x%02X)",
                     resp->bits.EchoData, req->crc);
        }
    } else {
        if (debug_level >= 4) {
            print_AdcResp(resp);
        }
    }

    //
    if (err) {
        return FAIL;
    }
    return OK;
}

//---------------------------------------------------------
bool adc_direct_command(AdcRequset_t *req)
{
    if (!CmdDirectCommand(req)) {
        return FAIL;
    }

    if (!check_adc_response(req)) {
        return FAIL;
    }
    return OK;
}

//---------------------------------------------------------
bool read_adc_register(int channel, uint8_t reg_num, uint8_t *value)
{
    // channel: 1..4
    AdcRequset_t req = {0};
    req.channel = channel;
    req.cmdop = CMDOP_RREG + reg_num;

    if (!adc_direct_command(&req)) {
        LOG(ERROR, "%s", req.errors);
        DBGF(0, "\n%s", req.errors);
        return FAIL;
    }
    if (value) {
        *value = req.resp.bits.AdcLsbData;
    }
    return OK;
}

//---------------------------------------------------------
bool write_adc_register(int channel, uint8_t reg_num, uint8_t value)
{
    // channel: 1..4
    AdcRequset_t req = {0};
    req.channel = channel;
    req.cmdop = CMDOP_WREG + reg_num;
    req.data = value;

    if (!adc_direct_command(&req)) {
        LOG(ERROR, "FAILED - %s", req.errors);
        return FAIL;
    }

    // check re-read
    req.cmdop = CMDOP_RREG + reg_num;
    if (!adc_direct_command(&req)) {
        LOG(ERROR, "FAILED - %s", req.errors);
        DBGF(0, "\nFAILED - %s\n", req.errors);
        return FAIL;
    }
    if (req.resp.bits.AdcLsbData != value) {
        LOG(ERROR, "writing 0x%02X to register %Xh FAILED - re-reading returns 0x%02X\n", value, reg_num,
            req.resp.bits.AdcLsbData);
        DBGF(0, "\nwriting 0x%02X to register %Xh FAILED - re-reading returns 0x%02X\n", value, reg_num,
             req.resp.bits.AdcLsbData);
        return FAIL;
    }
    return OK;
}

//---------------------------------------------------------
bool init_adc(int channel)
{
    // channel: 1..4
    uint8_t mode0_value; /* MODE 0 - DataRate & Filter */
    uint8_t mode1_value; /* MODE 1 - Conversion Mode & Conversion Start Delay */
    uint8_t mode3_value; /* MODE 3 - STATUS0 Byte Enable */
    uint8_t mode4_value; /* MODE 4 - Input Switch & PGA Gain */
    uint8_t ref_value; /* REF - Reference Input Multiplexer */

    if (cfg()->mode == MODE_ATLAS) {
        mode0_value = cfg()->adc_atlas.reg_mode0;
        mode1_value = cfg()->adc_atlas.reg_mode1;
        mode3_value = cfg()->adc_atlas.reg_mode3;
        mode4_value = cfg()->adc_atlas.reg_mode4;
        ref_value = cfg()->adc_atlas.reg_ref;
    } else if (cfg()->mode == MODE_APS) {
        mode0_value = cfg()->adc_aps.reg_mode0;
        mode1_value = cfg()->adc_aps.reg_mode1;
        mode3_value = cfg()->adc_aps.reg_mode3;
        mode4_value = cfg()->adc_aps.reg_mode4;
        ref_value = cfg()->adc_aps.reg_ref;
    } else {
        return FAIL;
    }
    int reg_num = RegMode0;
    DBGF(2, "ADC-%d Reg:%3Xh (%-5s) - 0x%02X", channel, reg_num, reg_name(reg_num), mode0_value);
    if (!write_adc_register(channel, reg_num, mode0_value)) {
        return FAIL;
    }

    reg_num = RegMode1;
    DBGF(2, "ADC-%d Reg:%3Xh (%-5s) - 0x%02X", channel, reg_num, reg_name(reg_num), mode1_value);
    if (!write_adc_register(channel, RegMode1, mode1_value)) {
        return FAIL;
    }

    reg_num = RegMode3;
    DBGF(2, "ADC-%d Reg:%3Xh (%-5s) - 0x%02X", channel, reg_num, reg_name(reg_num), mode3_value);
    if (!write_adc_register(channel, RegMode3, mode3_value)) {
        return FAIL;
    }

    reg_num = RegMode4;
    DBGF(2, "ADC-%d Reg:%3Xh (%-5s) - 0x%02X", channel, reg_num, reg_name(reg_num), mode4_value);
    if (!write_adc_register(channel, RegMode4, mode4_value)) {
        return FAIL;
    }

    reg_num = RegRef;
    DBGF(2, "ADC-%d Reg:%3Xh (%-5s) - 0x%02X", channel, reg_num, reg_name(reg_num), ref_value);
    if (!write_adc_register(channel, RegRef, ref_value)) {
        return FAIL;
    }

    return OK;
}

//---------------------------------------------------------
bool reset_adc(int channel)
{
    // channel: 1..4
    AdcRequset_t req = {0};
    req.channel = channel;
    req.cmdop = CMDOP_RESET;

    if (!adc_direct_command(&req)) {
        clear_arbitrary_value();
        return FAIL;
    }
    clear_arbitrary_value();
    return OK;
}

//---------------------------------------------------------
bool read_adc_id(int channel, uint8_t *id)
{
    // channel: 1..4
    if (id)
        *id = 0;
    AdcRequset_t req = {0};
    req.channel = channel;
    req.cmdop = CMDOP_RREG + RegID;
    if (!adc_direct_command(&req)) {
        return FAIL;
    }
    if (id)
        *id = req.resp.bits.AdcLsbData;
    DBGF(8, "id: 0x%02X", *id);
    return OK;
}

//---------------------------------------------------------
void dev_read_status(DevStat_t *status)
{
    if (!dev_ctrl_adr) {
        status->value = 0;
        return;
    }
    DevCmd_t cmd = {0};
    cmd.bits.CmdOpCode = CmdReadDevStat;
    cmd.bits.CmdParam = 0;
    DBGF(9, "  DevCmd:: %p <= 0x%08X", dev_ctrl_adr, cmd.value);
    *dev_ctrl_adr = cmd;

    *status = *(DevStat_t *) dev_stat_adr;
    DBGF(9, "  DevStat::%p => 0x%08X %s",
         dev_stat_adr, status->value, print_dev_status(status));
}

//---------------------------------------------------------
void dev_set_adc_scan_period(int channel, uint32_t number)
{
    // channel: 1..4
    DevCmd_t cmd = {0};
    cmd.bits.CmdOpCode = CmdSetAdcTmr;
    cmd.bits.AdcChannels = channel_to_onehot(channel);
    cmd.bits.CmdParam = number;
    DBGF(5, "ADC-%d: %u", channel, number);
    DBGF(9, "%p <= 0x%08X", dev_ctrl_adr, cmd.value);
    *dev_ctrl_adr = cmd;
}

//---------------------------------------------------------
bool is_dev_adc_scanning()
{
    static DevStat_t status;
    dev_read_status(&status);
    DBGF(8, "  ADC AUTOSCAN %s", status.bits.AdcMode ? "on" : "off");
    return (status.bits.AdcMode != 0);
}

//---------------------------------------------------------
bool dev_start_channels()
{
    DBGF(2, "%s", "-- start AUTOSCAN mode ... ");
    uint8_t adc_channels = 0x0F;
    return dev_start_adc(adc_channels);
}

//---------------------------------------------------------
bool dev_start_adc(int channels)
{
    // channel: 1..4
    DevCmd_t cmd = {0};
    cmd.bits.CmdOpCode = CmdSetAdcWorkMode;
    cmd.bits.AdcChannels = channels;
    cmd.bits.CmdParam = 0x0F;
    DBGF(5, "ADC channels: 0x%02X (%d %d %d %d)", channels, (channels & 0x08) >> 3, (channels & 0x04) >> 2,
         (channels & 0x02) >> 1,
         (channels & 0x01));
    DBGF(5, "DevCmd:: %p <= 0x%08X", dev_ctrl_adr, cmd.value);

    *dev_ctrl_adr = cmd;

    DBG(9, "wait_adc_ready()");
    if (channels & 0x01) {
        if (!wait_adc_ready(1)) {
            return FAIL;
        }
    }
    if (channels & 0x02) {
        if (!wait_adc_ready(2)) {
            return FAIL;
        }
    }
    if (channels & 0x04) {
        if (!wait_adc_ready(4)) {
            return FAIL;
        }
    }
    if (channels & 0x08) {
        if (!wait_adc_ready(4)) {
            return FAIL;
        }
    }
    return OK;
}

//---------------------------------------------------------
bool dev_stop_channels()
{
    bool ok[4];
    for (int i = 0; i < 4; i++) {
        DBGF(5, "channel: %d", i);
        ok[i] |= dev_adc_stop_scan(i + 1);
    }
    return ok[0] || ok[1] || ok[2] || ok[3];
}

//---------------------------------------------------------
bool dev_adc_stop_scan(int channel)
{
    // channel: 1..4
    DevCmd_t cmd = {0};
    cmd.bits.CmdOpCode = CmdSetAdcWorkMode;
    cmd.bits.AdcChannels = channel_to_onehot(channel);
    cmd.bits.CmdParam = 0;
    DBGF(5, "ADC-%d", channel);
    DBGF(9, "DevCmd::%p <= 0x%08X", dev_ctrl_adr, cmd.value);

    *dev_ctrl_adr = cmd;

    DBGF(9, "wait_adc_ready( ADC-%d )", channel);
    if (!wait_adc_ready(channel)) {
        return FAIL;
    }
    // to reset FPGA State Machine.
    uint32_t valueLo = *adc_regtr_lo;
    uint32_t valueHi = *adc_regtr_hi;
    DBGF(9, "adc reg hi-low: %08X %08X\n", valueHi, valueLo);
    return OK;
}

//---------------------------------------------------------
const char *CmdOpCodeName(cmd_op_t cmdcode)
{
    if (cmdcode == CmdReadDevStat)
        return "CmdReadDevStat";
    if (cmdcode == CmdSetAdcWorkMode)
        return "CmdSetAdcWorkMode";
    if (cmdcode == CmdSetAdcTmr)
        return "CmdSetAdcTmr";
    if (cmdcode == CmdSetDirectCommand)
        return "CmdSetDirectCommand";
    return "UNKN";
}

//---------------------------------------------------------
const char *reg_name(uint8_t reg_num)
{
    if (reg_num == 0x00)
        return "ID";
    if (reg_num == 0x01)
        return "STATUS0";
    if (reg_num == 0x02)
        return "MODE0";
    if (reg_num == 0x03)
        return "MODE1";
    if (reg_num == 0x04)
        return "RESRVD";
    if (reg_num == 0x05)
        return "MODE3";
    if (reg_num == 0x06)
        return "REF";
    if (reg_num == 0x07)
        return "OFCAL0";
    if (reg_num == 0x08)
        return "OFCAL1";
    if (reg_num == 0x09)
        return "OFCAL2";
    if (reg_num == 0x0a)
        return "FSCAL0";
    if (reg_num == 0x0b)
        return "FSCAL1";
    if (reg_num == 0x0c)
        return "FSCAL2";
    if (reg_num == 0x0d)
        return "RESRVD";
    if (reg_num == 0x0e)
        return "RESRVD";
    if (reg_num == 0x0f)
        return "RESRVD";
    if (reg_num == 0x10)
        return "MODE4";
    if (reg_num == 0x11)
        return "STATUS1";
    if (reg_num == 0x12)
        return "STATUS2";
    return "UNKN";
}

//---------------------------------------------------------
const char *cmd_name(uint8_t cmdcode)
{
    static char buf[32];
    if (cmdcode == 0)
        return "NOP";
    if (cmdcode == 0x06)
        return "RESET";
    if (cmdcode == 0x08)
        return "START";
    if (cmdcode == 0x0A)
        return "STOP";
    if (cmdcode == 0x12)
        return "RDATA";
    if (cmdcode == 0x16)
        return "OFSCAL";
    if (cmdcode == 0x17)
        return "GANCAL";

    if (cmdcode >= 0x20 && cmdcode <= 0x20 + 0x12) {
        snprintf(buf, sizeof(buf), "Read  %s", reg_name(cmdcode - 0x20));
        return buf;
    }

    if (cmdcode >= 0x40 && cmdcode <= 0x40 + 0x12) {
        snprintf(buf, sizeof(buf), "Write %s", reg_name(cmdcode - 0x40));
        return buf;
    }

    return "UNKN";
}

//---------------------------------------------------------
const char *print_dev_status(const DevStat_t *status)
{
    static char buf[256] = {0};
    snprintf(buf, sizeof(buf) - 1, "AdcMode: 0x%X (%u %u %u %u), AdcBusy: %u %u %u %u, AdcErr:0x%X, DevErr:0x%X",
             status->bits.AdcMode,
             (status->bits.AdcMode >> 3) & 0x01,
             (status->bits.AdcMode >> 2) & 0x01,
             (status->bits.AdcMode >> 1) & 0x01,
             (status->bits.AdcMode >> 0) & 0x01,
             (status->bits.AdcBusy >> 3) & 0x01,
             (status->bits.AdcBusy >> 2) & 0x01,
             (status->bits.AdcBusy >> 1) & 0x01,
             (status->bits.AdcBusy >> 0) & 0x01,
             status->bits.AdcErr,
             status->bits.DevErr);
    return buf;
}

//---------------------------------------------------------
/// @param channel  1..4
/// @param status0  STATUS0
void print_AdcStatus(int channel, uint8_t status0, int shortmsg)
{
    uint8_t status1 = 0;
    uint8_t status2 = 0;

    char msg0[1024];
    print_status0(status0, msg0, sizeof(msg0), shortmsg);
    printf("%s", msg0);

    // CRC1 STAT12
    AdcRequset_t req = {0};
    req.channel = channel;
    req.cmdop = CMDOP_RREG + RegStatus1;

    if (CmdDirectCommand(&req)) {
        status1 = req.resp.bits.AdcLsbData;
        char msg1[1024];
        print_status1(status1, msg1, sizeof(msg1), shortmsg);
        printf("  %s", msg1);
    }

    req.cmdop = CMDOP_RREG + RegStatus2;
    if (CmdDirectCommand(&req)) {
        status2 = req.resp.bits.AdcLsbData;
        char msg2[1024];
        print_status2(status2, msg2, sizeof(msg2), shortmsg);
        printf("  %s", msg2);
    }
    putchar('\n');
}

//---------------------------------------------------------
void print_AdcResp(const AdcResp_t *resp)
{
    printf("AdcResp: Echo cmdop: 0x%02X, obj: 0x%02X CRC: 0x%02X\n",
           resp->bits.EchoCmd, resp->bits.EchoData, resp->bits.EchoCrc);
    printf("         Status: 0x%02X, "
           "Msb: 0x%02X, Mid: 0x%02X, Lsb: 0x%02X, Out CRC: 0x%02X\n",
           resp->bits.AdcStatus, resp->bits.AdcMsbData, resp->bits.AdcMidData,
           resp->bits.AdcLsbData, resp->bits.AdcCrcResp);
}

//---------------------------------------------------------
const char *print_response_details(uint8_t reg_num, uint8_t value)
{
    static char buf[128];
    buf[0] = '\0';

    if (reg_num == RegID) {
        snprintf(buf, sizeof(buf), "%-7s", reg_name(reg_num));
        return buf;
    }

    if (reg_num == RegStatus0) {
        uint8_t CRC1 = (value & 0x40) >> 6;
        uint8_t STAT12 = (value & 0x10) >> 4;
        uint8_t REFALM = (value & 0x08) >> 4;
        uint8_t DRDY = (value & 0x04) >> 2;
        uint8_t CLOCK = (value & 0x02) >> 1;
        uint8_t RESET = (value & 0x01);
        snprintf(buf, sizeof(buf), "%-7s - CRC1:%u, STAT12:%u, REFALM:%u, DRDY:%u, CLOCK:%u, RESET:%u",
                 reg_name(reg_num), CRC1, STAT12, REFALM, DRDY, CLOCK, RESET);
        return buf;
    }
    if (reg_num == RegStatus1) {
        snprintf(buf, sizeof(buf), "%-7s", reg_name(reg_num));
        return buf;
    }
    if (reg_num == RegStatus2) {
        uint8_t CRC2 = (value & 0x10) >> 4;
        uint8_t ID2 = (value & 0x0F);
        snprintf(buf, sizeof(buf), "%-7s - CRC2:%u, REV_ID2:%Xh", reg_name(reg_num), CRC2, ID2);
        return buf;
    }
    if (reg_num == RegMode0) {
        uint8_t DRT = ((value >> 3) & 0x1F);
        uint8_t FLT = (value & 0x07);
        snprintf(buf, sizeof(buf), "%-7s - Data Rate: 0x%02X, Filter: 0x%02X", reg_name(reg_num), DRT, FLT);
        return buf;
    }
    if (reg_num == RegMode1) {
        uint8_t CNV = ((value >> 4) & 0x01);
        uint8_t DLY = (value & 0x0F);
        snprintf(buf, sizeof(buf), "%-7s - Conversion Mode:%u (%s), Delay: 0x%02X",
                 reg_name(reg_num), CNV, (CNV == 1 ? "Pulse" : "Conts"), DLY);
        return buf;
    }
    if (reg_num == RegMode3) {
        uint8_t STATENB = (value >> 6) & 0x01;
        snprintf(buf, sizeof(buf), "%-7s - STATENB:%u", reg_name(reg_num), STATENB);
        return buf;
    }
    if (reg_num == RegMode4) {
        uint8_t MUX = (value >> 4) & 0x07;
        uint8_t GAIN = (value & 0x0F);
        snprintf(buf, sizeof(buf), "%-7s - MUX: 0x%02X (Input %s%s), Gain: 0x%02X",
                 reg_name(reg_num),
                 MUX, (MUX == 0x00 ? "External" : ""),
                 (MUX == 0x05 ? "Internal" : ""), GAIN);
        return buf;
    }
    if (reg_num == RegRef) {
        uint8_t RMUX = (value & 0x0F);
        snprintf(buf, sizeof(buf), "%-7s - RMUX: 0x%02X (Reference %s%s)",
                 reg_name(reg_num),
                 RMUX, (RMUX == 0x05 ? "AVDD" : ""), (RMUX == 0x0A ? "External" : ""));
        return buf;
    }
    if (reg_num == RegOFCAL0) {
        snprintf(buf, sizeof(buf), "%-7s", reg_name(reg_num));
        return buf;
    }
    if (reg_num == RegOFCAL1) {
        snprintf(buf, sizeof(buf), "%-7s", reg_name(reg_num));
        return buf;
    }
    if (reg_num == RegOFCAL2) {
        snprintf(buf, sizeof(buf), "%-7s", reg_name(reg_num));
        return buf;
    }
    if (reg_num == RegFSCAL0) {
        snprintf(buf, sizeof(buf), "%-7s", reg_name(reg_num));
        return buf;
    }
    if (reg_num == RegFSCAL1) {
        snprintf(buf, sizeof(buf), "%-7s", reg_name(reg_num));
        return buf;
    }
    if (reg_num == RegFSCAL2) {
        snprintf(buf, sizeof(buf), "%-7s", reg_name(reg_num));
        return buf;
    }
    snprintf(buf, sizeof(buf), "%-7s", "RESRVD");
    return buf;
}

//---------------------------------------------------------
void print_status0(uint8_t status0, char *msg, int msg_len, int shortmsg)
{
    int num;
    *msg = '\0';
    if (shortmsg) {
        num = snprintf(msg, msg_len, "STATUS0:0x%02X", status0);
        msg += num;
        msg_len -= num;
        if (status0 & 0x40) {
            num = snprintf(msg, msg_len, " CRC1");
            msg += num;
            msg_len -= num;
        }
        if (status0 & 0x10) {
            num = snprintf(msg, msg_len, " STAT12");
            msg += num;
            msg_len -= num;
        }
        if (status0 & 0x08) {
            num = snprintf(msg, msg_len, " REFALM");
            msg += num;
            msg_len -= num;
        }
        if (status0 & 0x04) {
            num = snprintf(msg, msg_len, " DRDY");
            msg += num;
            msg_len -= num;
        }
        if (status0 & 0x02) {
            num = snprintf(msg, msg_len, " CLOCK");
            msg += num;
            msg_len -= num;
        }
        if (status0 & 0x01) {
            num = snprintf(msg, msg_len, " RESET");
            msg += num;
            msg_len -= num;
        }
        return;
    }

    num = snprintf(msg, msg_len, "STATUS0: 0x%02X -", status0);
    msg += num;
    msg_len -= num;

    if (status0 & 0x40) {
        num = snprintf(msg, msg_len, " CRC1:CRC error occurred during commands using CS1, ");
        msg += num;
        msg_len -= num;
    }
    if (status0 & 0x10) {
        num = snprintf(msg, msg_len, " STAT12:Error logged in the STATUS1 or STATUS2 registers, ");
        msg += num;
        msg_len -= num;
    }
    if (status0 & 0x08) {
        num = snprintf(msg, msg_len, " REFALM:Reference low alarm, ");
        msg += num;
        msg_len -= num;
    }
    if (status0 & 0x04) {
        num = snprintf(msg, msg_len, " DRDY:Conversion obj are new from the last obj read, ");
        msg += num;
        msg_len -= num;
    }
    if (status0 & 0x02) {
        num = snprintf(msg, msg_len, " CLOCK:ADC clock is external, ");
        msg += num;
        msg_len -= num;
    }
    if (status0 & 0x01) {
        snprintf(msg, msg_len, " RESET:Indicates an ADC reset has occurred");
    }
}

//---------------------------------------------------------
void print_status1(uint8_t status1, char *msg, int msg_len, int shortmsg)
{
    int num;
    *msg = '\0';
    num = snprintf(msg, msg_len, "STATUS1:0x%02X", status1);
    msg += num;
    msg_len -= num;
    if (status1 == 0) {
        return;
    }
    if (status1 & 0x80) {
        if (shortmsg) {
            num = snprintf(msg, msg_len, " PGA_ONL");
        } else {
            num = snprintf(msg, msg_len, "  PGA_ONL: (PGA Output Negative Low Alarm)");
        }
        msg += num;
        msg_len -= num;
    }
    if (status1 & 0x40) {
        if (shortmsg) {
            num = snprintf(msg, msg_len, " PGA_ONH");
        } else {
            num = snprintf(msg, msg_len, "  PGA_ONH: (PGA Output Negative High Alarm)");
        }
        msg += num;
        msg_len -= num;
    }
    if (status1 & 0x20) {
        if (shortmsg) {
            num = snprintf(msg, msg_len, "  PGA_OPL");
        } else {
            num = snprintf(msg, msg_len, "  PGA_OPL: (PGA Output Positive Low Alarm)");
        }
        msg += num;
        msg_len -= num;
    }
    if (status1 & 0x10) {
        if (shortmsg) {
            num = snprintf(msg, msg_len, "  PGA_OPH");
        } else {
            num = snprintf(msg, msg_len, "  PGA_OPH: (PGA Output Positive High Alarm)");
        }
        msg += num;
        msg_len -= num;
    }
    if (status1 & 0x08) {
        if (shortmsg) {
            num = snprintf(msg, msg_len, "  PGA_INL");
        } else {
            num = snprintf(msg, msg_len, "  PGA_INL: (PGA Input Negative Low Alarm)");
        }
        msg += num;
        msg_len -= num;
    }
    if (status1 & 0x04) {
        if (shortmsg) {
            num = snprintf(msg, msg_len, "  PGA_INH");
        } else {
            num = snprintf(msg, msg_len, "  PGA_INH: (PGA Input Negative High Alarm)");
        }
        msg += num;
        msg_len -= num;
    }
    if (status1 & 0x02) {
        if (shortmsg) {
            num = snprintf(msg, msg_len, "  PGA_IPL");
        } else {
            num = snprintf(msg, msg_len, "  PGA_IPL: (PGA Input Positive Low Alarm)");
        }
        msg += num;
        msg_len -= num;
    }
    if (status1 & 0x01) {
        if (shortmsg) {
            num = snprintf(msg, msg_len, "  PGA_IPH");
        } else {
            num = snprintf(msg, msg_len, "  PGA_IPH: (PGA Input Positive High Alarm)");
        }
        msg += num;
        msg_len -= num;
    }
}

//---------------------------------------------------------
void print_status2(uint8_t status2, char *msg, int msg_len, int shortmsg)
{
    int num;
    *msg = '\0';
    num = snprintf(msg, msg_len, "STATUS2:0x%02X", status2);
    msg += num;
    msg_len -= num;
    if (status2 == 0) {
        return;
    }
    if (shortmsg) {
        num = snprintf(msg, msg_len, "  CRC2");
        msg += num;
        msg_len -= num;
        return;
    }
    if (status2 & 0x10) {
        num = snprintf(msg, msg_len, "  CRC2:CRC error occurred during commands with use of CS2");
        msg += num;
        msg_len -= num;
    }
}
