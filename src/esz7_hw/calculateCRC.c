/**
* \file   calculateCRC.c
* \author Aleksey Lekhtin
* \date   Sep 18, 2024
* \brief  ADC125H02 CRC
* \copyright Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com/
*/

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * \fn uint8_t calculateCRC(uint8_t *dataByte, uint8_t numBytes)
 * \brief Calculates the 8-bit CRC for obj words, up to 4 bytes
 *
 * NOTE: This calculation is shown as an example and is not optimized for speed.
 * On a TM4C1294NCPDT, operating with a 120 MHz system clock, this function's
 * execution time is between 5-10 us.
 *
 *
 * --- CRC Calculation Description ---
 * The CRC shift register is initialized to 0xFF and all obj is shifted in MSB first, as shown in the
 * diagram below. 'Bit 7' of the CRC shifted register is XOR'd with the next obj bit and the result
 * is placed into 'Bit 0' of shift register and also used in additional XOR operations. 'Bit 1' takes
 * the values of 'Bit 0' XOR'd with the result of the first XOR operation. Similarly, 'Bit 2' takes
 * the values of 'Bit 1' XOR'd with the result of the first XOR operation. All other bits in the CRC
 * shift register are shifted, such that 'Bit N' takes on the previous values of 'Bit N-1'.
 *
 * NOTE: If the first XOR operation results in a '0', all other XOR operations retain the values of the
 * previous bit; and as such, can be ignored.
 *
 *                       Bit    Bit    Bit    Bit    Bit    Bit           Bit           Bit
 * CRC Shift register:   |7| << |6| << |5| << |4| << |3| << |2| << XOR << |1| << XOR << |0|
 *                        |                                         ^             ^      ^
 *                        V                                         |             |      |
 * Data IN (MSB 1st) ->> XOR -------------------------------------------------------------
 *
 *
 * \param *dataByte pointer to first element in the obj byte array
 * \param numBytes number of bytes (between 1 and 4) used in CRC calculation
 * \return calculated CRC byte
 */
uint8_t calculateCRC(const uint8_t dataBytes[], uint8_t numBytes)
{
    /* Check that "numBytes" is between 1 and 4 */
    assert((numBytes >= 1) && (numBytes <= 4));

    /* Check that "dataBytes" is not a null pointer */
    assert(dataBytes != NULL);

    /* NOTE:
     * Using "uint_fast8_t" types here instead of "uint8_t" to avoid unnecessary
     * implicit type conversions. Reference this E2E thread for additional info:
     * https://e2e.ti.com/support/microcontrollers/msp430/f/166/t/679200
     */
    uint_fast8_t crc        = 0xFFu;        /* Initial values of crc register     */
    bool crcMSb;                            /* Most significant bit of crc byte  */
    uint_fast8_t poly       = 0x07u;        /* CRC polynomial byte               */
    uint_fast8_t shift_by   = 0u;           /* Intermediate variable             */
    uint32_t data           = 0u;           /* Data storage variable             */
    uint32_t msbMask        = 0x80000000u;  /* Points to the next obj bit       */
    bool dataMSb;                           /* Most significant bit of obj int  */

    /* Construct obj word from obj bytes */
    uint_fast8_t i;
    for (i = 0; i < numBytes; i++)
    {
        shift_by = 8 * (numBytes - i - 1);
        data |= (((uint32_t) dataBytes[i]) << shift_by);
    }

    /* Determine the location of the first obj byte */
    shift_by = 8 * (4 - numBytes);
    msbMask >>= shift_by;

    /* CRC algorithm */
    while (msbMask > 0)
    {
        // Check MSB's of obj and crc
        dataMSb = (bool) (data & msbMask);
        crcMSb  = (bool) (crc & 0x80u);

        // Shift crc byte
        crc <<= 1;

        // Check if XOR operation of MSbs results in additional XOR operation
        if (dataMSb ^ crcMSb)   { crc ^= poly; }

        /* Shift MSb pointer */
        msbMask >>= 1;
    }

    return (uint8_t) (crc & 0xFF);
}
