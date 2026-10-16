/**
  \file   npt_t.h
  \author Aleksey Lekhtin
  \date   Jul  7, 2025
  \brief MPT table
*/

#ifndef MPT_T__H_
#define MPT_T__H_

#include <stdint.h>
#include <stdint.h>
#include <sys/types.h>

typedef enum mpt_format_t
{
    MPT_DATA = (uint8_t)0,
    MPT_FORMAT = 1,
    MPT_UNKN = 0xFF
} mpt_format_t;

typedef enum mpt_encoding_t
{
    MPT_ERR = (int8_t)-1,
    Biphase = 0,
    TimeBase2,
    RLL,
    TimeBase3,
    Miller
} mpt_encoding_t;

typedef struct mpt_data_table_t // type:00
{
    char mnemonic[8];
    uint length;
    float scale_factor;
    float low_limit;
    float high_limit;
    char units[8];
    uint disp_length;
    uint disp_places;
    uint print_ctrl;
    uint scale_type;
}  mpt_data_table_t;

typedef struct mpt_format_table_t // type:01
{
    uint16_t FID;
    uint16_t FIDType;
    uint16_t enableSync;    // 1 - ON, 2 - OFF
    uint16_t repeatCount;
    uint16_t resync;        // 1 - ON, 2 - OFF
    uint16_t loopback;      // ex: 2
    uint16_t alert;         // 0 - OFF
    uint16_t dataList[12];
} mpt_format_table_t;

#define MPT_DATA_SIZE 56
#define MPT_FORMAT_SIZE 16

typedef struct mpt_t
{
    mpt_data_table_t mpt_data[MPT_DATA_SIZE];
    mpt_format_table_t mpt_format[MPT_FORMAT_SIZE];

    // Loopback: 2 Non-Rotating to Rotating Transistion Index: 2
    uint MptNRot2Rot;  // [@NRRLOOP:02]

    // Loopback: 2 Rotating to Non-Rotating Transistion Index: 2
    uint MptRot2NRot;  // [@RNRLOOP:02]

} mpt_t;

//void MptFormatTableReadDataProcess(const char *item);
//void MptFormatTableReadFormatsProcess(const char *item);

#endif //MPT_T__H_
