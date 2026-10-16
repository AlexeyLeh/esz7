/**
 \file   adc_t.h
 \brief  Структуры управления АЦП - Версия 3.0 02.12.2025
 \author LAV
 \date   1.03.2024
 \history Версия 0 06.03.2024 - AK
          Версия 1.1 18.07.2024 - LAV
          Версия 1.2 10.10.2024 - AK
          Версия 3.0 02.12.2025 - AK
 */

#ifndef ADC_T_H_
#define ADC_T_H_

#include "esz7_regs.h"

// Register map
typedef enum reg_map_t
{
	RegID      = 0x00,
	RegStatus0 = 0x01,
	RegMode0   = 0x02,
	RegMode1   = 0x03,
	RegMode3   = 0x05,
	RegRef     = 0x06,
	RegOFCAL0  = 0x07,
	RegOFCAL1  = 0x08,
	RegOFCAL2  = 0x09,
	RegFSCAL0  = 0x0A,
	RegFSCAL1  = 0x0B,
	RegFSCAL2  = 0x0C,
	RegMode4   = 0x10,
	RegStatus1 = 0x11,
	RegStatus2 = 0x12,
} reg_map_t;

typedef enum cmdop_t
{
	CMDOP_NOP    = (uint16_t)0,
	CMDOP_RESET  = 0x06,
	CMDOP_START  = 0x08,
	CMDOP_STOP   = 0x0A,
	CMDOP_RDATA  = 0x12,
	CMDOP_OFSCAL = 0x16,
	CMDOP_GANCAL = 0x17,
	CMDOP_RREG   = 0x20,
	CMDOP_WREG   = 0x40,
} regop_t;

// Read Device status Answer:
//	- 4bits Current mode of each ADC Work modes (Cpu or AutoTimer). (0 - Adc is available for CPU).
//	- 4bits Timers condition. (Programmed or not). (0 - default values of period).
//	- 4bits Presence of communication error with ADC.
typedef enum cmd_op_t
{
	CmdReadDevStat = (uint8_t)0x01, // Read Device status
	CmdSetAdcWorkMode = 0x02,       // Set current mode - low 4 bit in CmdParam + AdcChannels.
	CmdSetAdcTmr = 0x04,            // Auto timer control(on/off) - bit 3-0 in CmdParam
	CmdSetDirectCommand = 0x08,     // Direct command to Adc + 24 bit values.
} cmd_op_t;

typedef union __attribute__((packed, aligned(4)))  CmdParam_t
{
	struct {
		uint32_t AdcCommand  : 8;	// Cop
		uint32_t AdcCmdData  : 8;	// Dummy for read, Data for write
		uint32_t AdcCmdCrc   : 8;	// Crc Command + Data
		//uint32_t AdcCmdZero : 8;	// 0 allways
	} bits;
	uint32_t value;
} CmdParam_t;

typedef union __attribute__((packed, aligned(4))) DevCmd_t {
	struct {                        // Dev Command register
		uint32_t CmdParam    : 24;  // 24 bits obj for write belonging to the operation
		uint32_t AdcChannels : 4;   // Channel belonging to the operation
		cmd_op_t CmdOpCode   : 4;   // OpCode (HDL - MSB)
	} bits;
	uint32_t value;
} DevCmd_t;

typedef union __attribute__((packed, aligned(4))) DevStat_t {
	struct {
		uint32_t DevErr  : 4; // Presence of communication error with Dev
		uint32_t AdcErr  : 4; // Presence of communication error with ADC
		uint32_t AdcBusy : 4; // Adc's Busy flag
		uint32_t AdcMode : 4; // Adc's mode 0-Cpu, 1-AutoTimer
	} bits;
	uint32_t value;
} DevStat_t;

typedef union __attribute__((packed, aligned(4))) AdcResp_t {
	struct {
		uint8_t EchoCmd;	// Cmd
		uint8_t EchoData;	// Dummy for read, Data for write
		uint8_t EchoCrc;	// CrcCmd
		uint8_t AdcStatus;	// Adc status
		uint8_t AdcMsbData;	// Data
		uint8_t AdcMidData;	// Data
		uint8_t AdcLsbData;	// Data
		uint8_t AdcCrcResp;	// Crc response
	} bits;
	uint64_t value;
} AdcResp_t;

typedef struct __attribute__((packed, aligned(4))) AdcRequset_t {
	char      errors[1024];
	uint8_t   channel;		// 1..4
	uint8_t   cmdop;
	uint8_t   data;
	uint8_t   crc;
	DevCmd_t  cmd;
	AdcResp_t resp;
} AdcRequset_t;

#endif // ADC_T_H_
