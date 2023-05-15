/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
  Simulator for the RDS02UF rangefinder
*/

#include "SIM_RF_RDS02UF.h"

#include <stdio.h>

using namespace SITL;


    const uint8_t crc8_table[256] = {
    0x93,0x98,0xE4,0x46,0xEB,0xBA,0x04,0x4C,
    0xFA,0x40,0xB8,0x96,0x0E,0xB2,0xB7,0xC0,
    0x0C,0x32,0x9B,0x80,0xFF,0x30,0x7F,0x9D,
    0xB3,0x81,0x58,0xE7,0xF1,0x19,0x7E,0xB6,
    0xCD,0xF7,0xB4,0xCB,0xBC,0x5C,0xD6,0x09,
    0x20,0x0A,0xE0,0x37,0x51,0x67,0x24,0x95,
    0xE1,0x62,0xF8,0x5E,0x38,0x15,0x54,0x77,
    0x63,0x57,0x6D,0xE9,0x89,0x76,0xBE,0x41,
    0x5D,0xF9,0xB1,0x4D,0x6C,0x53,0x9C,0xA2,
    0x23,0xC4,0x8E,0xC8,0x05,0x42,0x61,0x71,
    0xC5,0x00,0x18,0x6F,0x5F,0xFB,0x7B,0x11,
    0x65,0x2D,0x8C,0xED,0x14,0xAB,0x88,0xD5,
    0xD9,0xC2,0x36,0x34,0x7C,0x5B,0x3C,0xF6,
    0x48,0x0B,0xEE,0x02,0x83,0x79,0x17,0xE6,
    0xA8,0x78,0xF5,0xD3,0x4E,0x50,0x52,0x91,
    0xD8,0xC6,0x22,0xEC,0x3B,0xE5,0x3F,0x86,
    0x06,0xCF,0x2B,0x2F,0x3D,0x59,0x1C,0x87,
    0xEF,0x4F,0x10,0xD2,0x7D,0xDA,0x72,0xA0,
    0x9F,0xDE,0x6B,0x75,0x56,0xBD,0xC7,0xC1,
    0x70,0x1D,0x25,0x92,0xA5,0x31,0xE2,0xD7,
    0xD0,0x9A,0xAF,0xA9,0xC9,0x97,0x08,0x33,
    0x5A,0x99,0xC3,0x16,0x84,0x82,0x8A,0xF3,
    0x4A,0xCE,0xDB,0x29,0x0F,0xAE,0x6E,0xE3,
    0x8B,0x07,0x3A,0x74,0x47,0xB0,0xBB,0xB5,
    0x7A,0xAA,0x2C,0xD4,0x03,0x3E,0x1A,0xA7,
    0x27,0x64,0x06,0xBF,0x55,0x73,0x1E,0xFE,
    0x49,0x01,0x39,0x28,0xF4,0x26,0xDF,0xDD,
    0x44,0x0D,0x21,0xF2,0x85,0xB9,0xEA,0x4B,
    0xDC,0x6A,0xCA,0xAC,0x12,0xFC,0x2E,0x2A,
    0xA3,0xF0,0x66,0xE8,0x60,0x45,0xA1,0x8D,
    0x68,0x35,0xFD,0x8F,0x9E,0x1F,0x13,0xD1,
    0xAD,0x69,0xCC,0xA4,0x94,0x90,0x1B,0x43,
    };
uint8_t xcrc8(uint8_t* pbuf, int32_t len)
{
     uint8_t* data = pbuf;
     uint8_t crc = 0;
     while ( len-- )
     crc = crc8_table[crc^*(data++)];
     return crc;
}


uint32_t RF_RDS02UF::packet_for_alt(uint16_t alt_cm, uint8_t *buffer, uint8_t buflen)
{
    const uint16_t fc_code = 0x3ff;  // NFI what this means
    const uint16_t data_fc = 0x70c;  // NFI what this means

    // bodgy fixed-length response to keep things simple:
    union response_t {
        struct {
            uint8_t header1;
            uint8_t header2;
            uint8_t address;
            uint8_t error_code;
            uint8_t fc_code_l;
            uint8_t fc_code_h;
            uint8_t length_l;
            uint8_t length_h;
            uint8_t data0;  // used
            uint8_t data1;  // used
            uint8_t data2;
            uint8_t data3;
            uint8_t data4;
            uint8_t data5;  // distance-low
            uint8_t data6;  // distance-high
            uint8_t data7;
            uint8_t data8;
            uint8_t data9;
            uint8_t crc8;
            uint8_t footer1;
            uint8_t footer2;
        };
        uint8_t buffer[21];
    } response{};

    response.header1 = 0x55;
    response.header2 = 0x55;
    response.fc_code_l = fc_code & 0xff;
    response.fc_code_h = fc_code >> 8;
    response.length_l = 10;
    response.length_h = 0;
    response.data0 = data_fc & 0xff;
    response.data1 = data_fc >> 8;
    response.data5 = alt_cm  & 0xff;
    response.data6 = alt_cm >> 8;
    response.crc8 = xcrc8(&response.buffer[2], 16);
    response.footer1 = 0xAA;
    response.footer2 = 0xAA;

    if (buflen < ARRAY_SIZE(response.buffer)) {
        AP_HAL::panic("Too short a buffer");
    }

    memcpy(buffer, response.buffer, ARRAY_SIZE(response.buffer));

    return ARRAY_SIZE(response.buffer);
}