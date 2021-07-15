/******************************************************************************
 *
 *  Copyright 1999-2012 Broadcom Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at:
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  This file contains the Windowing coeffs for synthesis filter
 *
 ******************************************************************************/

#include "sbc_encoder.h"

#if (SBC_ARM_ASM_OPT == FALSE && SBC_IPAQ_OPT == FALSE)
#if (SBC_IS_64_MULT_IN_WINDOW_ACCU == FALSE)
/*Window coeff for 4 sub band case*/
const int16_t gas32CoeffFor4SBs[] = {
    (int16_t)((int32_t)0x00000000 >> 16), (int16_t)0x00000000,
    (int16_t)((int32_t)0x001194E6 >> 16), (int16_t)0x001194E6,
    (int16_t)((int32_t)0x0030E2D3 >> 16), (int16_t)0x0030E2D3,
    (int16_t)((int32_t)0x00599403 >> 16), (int16_t)0x00599403,
    (int16_t)((int32_t)0x007DBCC8 >> 16), (int16_t)0x007DBCC8,
    (int16_t)((int32_t)0x007F88E4 >> 16), (int16_t)0x007F88E4,
    (int16_t)((int32_t)0x003D239B >> 16), (int16_t)0x003D239B,
    (int16_t)((int32_t)0xFF9BB9D5 >> 16), (int16_t)0xFF9BB9D5,

    (int16_t)((int32_t)0x01659F45 >> 16), (int16_t)0x01659F45,
    (int16_t)((int32_t)0x029DBAA3 >> 16), (int16_t)0x029DBAA3,
    (int16_t)((int32_t)0x03B23341 >> 16), (int16_t)0x03B23341,
    (int16_t)((int32_t)0x041EEE40 >> 16), (int16_t)0x041EEE40,
    (int16_t)((int32_t)0x034FEE2C >> 16), (int16_t)0x034FEE2C,
    (int16_t)((int32_t)0x00C8F2BC >> 16), (int16_t)0x00C8F2BC,
    (int16_t)((int32_t)0xFC4F91D4 >> 16), (int16_t)0xFC4F91D4,
    (int16_t)((int32_t)0xF60FAF37 >> 16), (int16_t)0xF60FAF37,

    (int16_t)((int32_t)0x115B1ED2 >> 16), (int16_t)0x115B1ED2,
    (int16_t)((int32_t)0x18F55C90 >> 16), (int16_t)0x18F55C90,
    (int16_t)((int32_t)0x1F91CA46 >> 16), (int16_t)0x1F91CA46,
    (int16_t)((int32_t)0x2412F251 >> 16), (int16_t)0x2412F251,
    (int16_t)((int32_t)0x25AC1FF2 >> 16), (int16_t)0x25AC1FF2,
    (int16_t)((int32_t)0x2412F251 >> 16), (int16_t)0x2412F251,
    (int16_t)((int32_t)0x1F91CA46 >> 16), (int16_t)0x1F91CA46,
    (int16_t)((int32_t)0x18F55C90 >> 16), (int16_t)0x18F55C90,

    (int16_t)((int32_t)0xEEA4E12E >> 16), (int16_t)0xEEA4E12E,
    (int16_t)((int32_t)0xF60FAF37 >> 16), (int16_t)0xF60FAF37,
    (int16_t)((int32_t)0xFC4F91D4 >> 16), (int16_t)0xFC4F91D4,
    (int16_t)((int32_t)0x00C8F2BC >> 16), (int16_t)0x00C8F2BC,
    (int16_t)((int32_t)0x034FEE2C >> 16), (int16_t)0x034FEE2C,
    (int16_t)((int32_t)0x041EEE40 >> 16), (int16_t)0x041EEE40,
    (int16_t)((int32_t)0x03B23341 >> 16), (int16_t)0x03B23341,
    (int16_t)((int32_t)0x029DBAA3 >> 16), (int16_t)0x029DBAA3,

    (int16_t)((int32_t)0xFE9A60BB >> 16), (int16_t)0xFE9A60BB,
    (int16_t)((int32_t)0xFF9BB9D5 >> 16), (int16_t)0xFF9BB9D5,
    (int16_t)((int32_t)0x003D239B >> 16), (int16_t)0x003D239B,
    (int16_t)((int32_t)0x007F88E4 >> 16), (int16_t)0x007F88E4,
    (int16_t)((int32_t)0x007DBCC8 >> 16), (int16_t)0x007DBCC8,
    (int16_t)((int32_t)0x00599403 >> 16), (int16_t)0x00599403,
    (int16_t)((int32_t)0x0030E2D3 >> 16), (int16_t)0x0030E2D3,
    (int16_t)((int32_t)0x001194E6 >> 16), (int16_t)0x001194E6};

/*Window coeff for 8 sub band case*/
const int16_t gas32CoeffFor8SBs[] = {
    (int16_t)((int32_t)0x00000000 >> 16), (int16_t)0x00000000,
    (int16_t)((int32_t)0x00052173 >> 16), (int16_t)0x00052173,
    (int16_t)((int32_t)0x000B3F71 >> 16), (int16_t)0x000B3F71,
    (int16_t)((int32_t)0x00122C7D >> 16), (int16_t)0x00122C7D,
    (int16_t)((int32_t)0x001AFF89 >> 16), (int16_t)0x001AFF89,
    (int16_t)((int32_t)0x00255A62 >> 16), (int16_t)0x00255A62,
    (int16_t)((int32_t)0x003060F4 >> 16), (int16_t)0x003060F4,
    (int16_t)((int32_t)0x003A72E7 >> 16), (int16_t)0x003A72E7,

    (int16_t)((int32_t)0x0041EC6A >> 16), (int16_t)0x0041EC6A, /* 8 */
    (int16_t)((int32_t)0x0044EF48 >> 16), (int16_t)0x0044EF48,
    (int16_t)((int32_t)0x00415B75 >> 16), (int16_t)0x00415B75,
    (int16_t)((int32_t)0x0034F8B6 >> 16), (int16_t)0x0034F8B6,
    (int16_t)((int32_t)0x001D8FD2 >> 16), (int16_t)0x001D8FD2,
    (int16_t)((int32_t)0xFFFA2413 >> 16), (int16_t)0xFFFA2413,
    (int16_t)((int32_t)0xFFC9F10E >> 16), (int16_t)0xFFC9F10E,
    (int16_t)((int32_t)0xFF8D6793 >> 16), (int16_t)0xFF8D6793,

    (int16_t)((int32_t)0x00B97348 >> 16), (int16_t)0x00B97348, /* 16 */
    (int16_t)((int32_t)0x01071B96 >> 16), (int16_t)0x01071B96,
    (int16_t)((int32_t)0x0156B3CA >> 16), (int16_t)0x0156B3CA,
    (int16_t)((int32_t)0x01A1B38B >> 16), (int16_t)0x01A1B38B,
    (int16_t)((int32_t)0x01E0224C >> 16), (int16_t)0x01E0224C,
    (int16_t)((int32_t)0x0209291F >> 16), (int16_t)0x0209291F,
    (int16_t)((int32_t)0x02138653 >> 16), (int16_t)0x02138653,
    (int16_t)((int32_t)0x01F5F424 >> 16), (int16_t)0x01F5F424,

    (int16_t)((int32_t)0x01A7ECEF >> 16), (int16_t)0x01A7ECEF, /* 24 */
    (int16_t)((int32_t)0x01223EBA >> 16), (int16_t)0x01223EBA,
    (int16_t)((int32_t)0x005FD0FF >> 16), (int16_t)0x005FD0FF,
    (int16_t)((int32_t)0xFF5EEB73 >> 16), (int16_t)0xFF5EEB73,
    (int16_t)((int32_t)0xFE20435D >> 16), (int16_t)0xFE20435D,
    (int16_t)((int32_t)0xFCA86E7E >> 16), (int16_t)0xFCA86E7E,
    (int16_t)((int32_t)0xFAFF95FC >> 16), (int16_t)0xFAFF95FC,
    (int16_t)((int32_t)0xF9312891 >> 16), (int16_t)0xF9312891,

    (int16_t)((int32_t)0x08B4307A >> 16), (int16_t)0x08B4307A, /* 32 */
    (int16_t)((int32_t)0x0A9F3E9A >> 16), (int16_t)0x0A9F3E9A,
    (int16_t)((int32_t)0x0C7D59B6 >> 16), (int16_t)0x0C7D59B6,
    (int16_t)((int32_t)0x0E3BB16F >> 16), (int16_t)0x0E3BB16F,
    (int16_t)((int32_t)0x0FC721F9 >> 16), (int16_t)0x0FC721F9,
    (int16_t)((int32_t)0x110ECEF0 >> 16), (int16_t)0x110ECEF0,
    (int16_t)((int32_t)0x120435FA >> 16), (int16_t)0x120435FA,
    (int16_t)((int32_t)0x129C226F >> 16), (int16_t)0x129C226F,

    (int16_t)((int32_t)0x12CF6C75 >> 16), (int16_t)0x12CF6C75, /* 40 */
    (int16_t)((int32_t)0x129C226F >> 16), (int16_t)0x129C226F,
    (int16_t)((int32_t)0x120435FA >> 16), (int16_t)0x120435FA,
    (int16_t)((int32_t)0x110ECEF0 >> 16), (int16_t)0x110ECEF0,
    (int16_t)((int32_t)0x0FC721F9 >> 16), (int16_t)0x0FC721F9,
    (int16_t)((int32_t)0x0E3BB16F >> 16), (int16_t)0x0E3BB16F,
    (int16_t)((int32_t)0x0C7D59B6 >> 16), (int16_t)0x0C7D59B6,
    (int16_t)((int32_t)0x0A9F3E9A >> 16), (int16_t)0x0A9F3E9A,

    (int16_t)((int32_t)0xF74BCF86 >> 16), (int16_t)0xF74BCF86, /* 48 */
    (int16_t)((int32_t)0xF9312891 >> 16), (int16_t)0xF9312891,
    (int16_t)((int32_t)0xFAFF95FC >> 16), (int16_t)0xFAFF95FC,
    (int16_t)((int32_t)0xFCA86E7E >> 16), (int16_t)0xFCA86E7E,
    (int16_t)((int32_t)0xFE20435D >> 16), (int16_t)0xFE20435D,
    (int16_t)((int32_t)0xFF5EEB73 >> 16), (int16_t)0xFF5EEB73,
    (int16_t)((int32_t)0x005FD0FF >> 16), (int16_t)0x005FD0FF,
    (int16_t)((int32_t)0x01223EBA >> 16), (int16_t)0x01223EBA,

    (int16_t)((int32_t)0x01A7ECEF >> 16), (int16_t)0x01A7ECEF, /* 56 */
    (int16_t)((int32_t)0x01F5F424 >> 16), (int16_t)0x01F5F424,
    (int16_t)((int32_t)0x02138653 >> 16), (int16_t)0x02138653,
    (int16_t)((int32_t)0x0209291F >> 16), (int16_t)0x0209291F,
    (int16_t)((int32_t)0x01E0224C >> 16), (int16_t)0x01E0224C,
    (int16_t)((int32_t)0x01A1B38B >> 16), (int16_t)0x01A1B38B,
    (int16_t)((int32_t)0x0156B3CA >> 16), (int16_t)0x0156B3CA,
    (int16_t)((int32_t)0x01071B96 >> 16), (int16_t)0x01071B96,

    (int16_t)((int32_t)0xFF468CB8 >> 16), (int16_t)0xFF468CB8, /* 64 */
    (int16_t)((int32_t)0xFF8D6793 >> 16), (int16_t)0xFF8D6793,
    (int16_t)((int32_t)0xFFC9F10E >> 16), (int16_t)0xFFC9F10E,
    (int16_t)((int32_t)0xFFFA2413 >> 16), (int16_t)0xFFFA2413,
    (int16_t)((int32_t)0x001D8FD2 >> 16), (int16_t)0x001D8FD2,
    (int16_t)((int32_t)0x0034F8B6 >> 16), (int16_t)0x0034F8B6,
    (int16_t)((int32_t)0x00415B75 >> 16), (int16_t)0x00415B75,
    (int16_t)((int32_t)0x0044EF48 >> 16), (int16_t)0x0044EF48,

    (int16_t)((int32_t)0x0041EC6A >> 16), (int16_t)0x0041EC6A, /* 72 */
    (int16_t)((int32_t)0x003A72E7 >> 16), (int16_t)0x003A72E7,
    (int16_t)((int32_t)0x003060F4 >> 16), (int16_t)0x003060F4,
    (int16_t)((int32_t)0x00255A62 >> 16), (int16_t)0x00255A62,
    (int16_t)((int32_t)0x001AFF89 >> 16), (int16_t)0x001AFF89,
    (int16_t)((int32_t)0x00122C7D >> 16), (int16_t)0x00122C7D,
    (int16_t)((int32_t)0x000B3F71 >> 16), (int16_t)0x000B3F71,
    (int16_t)((int32_t)0x00052173 >> 16), (int16_t)0x00052173};

#else

/*Window coeff for 4 sub band case*/
const int32_t gas32CoeffFor4SBs[] = {
    (int32_t)0x00000000, (int32_t)0x001194E6, (int32_t)0x0030E2D3,
    (int32_t)0x00599403, (int32_t)0x007DBCC8, (int32_t)0x007F88E4,
    (int32_t)0x003D239B, (int32_t)0xFF9BB9D5,

    (int32_t)0x01659F45, (int32_t)0x029DBAA3, (int32_t)0x03B23341,
    (int32_t)0x041EEE40, (int32_t)0x034FEE2C, (int32_t)0x00C8F2BC,
    (int32_t)0xFC4F91D4, (int32_t)0xF60FAF37,

    (int32_t)0x115B1ED2, (int32_t)0x18F55C90, (int32_t)0x1F91CA46,
    (int32_t)0x2412F251, (int32_t)0x25AC1FF2, (int32_t)0x2412F251,
    (int32_t)0x1F91CA46, (int32_t)0x18F55C90,

    (int32_t)0xEEA4E12E, (int32_t)0xF60FAF37, (int32_t)0xFC4F91D4,
    (int32_t)0x00C8F2BC, (int32_t)0x034FEE2C, (int32_t)0x041EEE40,
    (int32_t)0x03B23341, (int32_t)0x029DBAA3,

    (int32_t)0xFE9A60BB, (int32_t)0xFF9BB9D5, (int32_t)0x003D239B,
    (int32_t)0x007F88E4, (int32_t)0x007DBCC8, (int32_t)0x00599403,
    (int32_t)0x0030E2D3, (int32_t)0x001194E6};

/*Window coeff for 8 sub band case*/
const int32_t gas32CoeffFor8SBs[] = {
    (int32_t)0x00000000, (int32_t)0x00052173, (int32_t)0x000B3F71,
    (int32_t)0x00122C7D, (int32_t)0x001AFF89, (int32_t)0x00255A62,
    (int32_t)0x003060F4, (int32_t)0x003A72E7,

    (int32_t)0x0041EC6A, /* 8 */
    (int32_t)0x0044EF48, (int32_t)0x00415B75, (int32_t)0x0034F8B6,
    (int32_t)0x001D8FD2, (int32_t)0xFFFA2413, (int32_t)0xFFC9F10E,
    (int32_t)0xFF8D6793,

    (int32_t)0x00B97348, /* 16 */
    (int32_t)0x01071B96, (int32_t)0x0156B3CA, (int32_t)0x01A1B38B,
    (int32_t)0x01E0224C, (int32_t)0x0209291F, (int32_t)0x02138653,
    (int32_t)0x01F5F424,

    (int32_t)0x01A7ECEF, /* 24 */
    (int32_t)0x01223EBA, (int32_t)0x005FD0FF, (int32_t)0xFF5EEB73,
    (int32_t)0xFE20435D, (int32_t)0xFCA86E7E, (int32_t)0xFAFF95FC,
    (int32_t)0xF9312891,

    (int32_t)0x08B4307A, /* 32 */
    (int32_t)0x0A9F3E9A, (int32_t)0x0C7D59B6, (int32_t)0x0E3BB16F,
    (int32_t)0x0FC721F9, (int32_t)0x110ECEF0, (int32_t)0x120435FA,
    (int32_t)0x129C226F,

    (int32_t)0x12CF6C75, /* 40 */
    (int32_t)0x129C226F, (int32_t)0x120435FA, (int32_t)0x110ECEF0,
    (int32_t)0x0FC721F9, (int32_t)0x0E3BB16F, (int32_t)0x0C7D59B6,
    (int32_t)0x0A9F3E9A,

    (int32_t)0xF74BCF86, /* 48 */
    (int32_t)0xF9312891, (int32_t)0xFAFF95FC, (int32_t)0xFCA86E7E,
    (int32_t)0xFE20435D, (int32_t)0xFF5EEB73, (int32_t)0x005FD0FF,
    (int32_t)0x01223EBA,

    (int32_t)0x01A7ECEF, /* 56 */
    (int32_t)0x01F5F424, (int32_t)0x02138653, (int32_t)0x0209291F,
    (int32_t)0x01E0224C, (int32_t)0x01A1B38B, (int32_t)0x0156B3CA,
    (int32_t)0x01071B96,

    (int32_t)0xFF468CB8, /* 64 */
    (int32_t)0xFF8D6793, (int32_t)0xFFC9F10E, (int32_t)0xFFFA2413,
    (int32_t)0x001D8FD2, (int32_t)0x0034F8B6, (int32_t)0x00415B75,
    (int32_t)0x0044EF48,

    (int32_t)0x0041EC6A, /* 72 */
    (int32_t)0x003A72E7, (int32_t)0x003060F4, (int32_t)0x00255A62,
    (int32_t)0x001AFF89, (int32_t)0x00122C7D, (int32_t)0x000B3F71,
    (int32_t)0x00052173};

#endif
#endif
