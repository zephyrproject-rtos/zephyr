/******************************************************************************
*  Filename:       pka.c
*  Revised:        2018-07-19 15:07:05 +0200 (Thu, 19 Jul 2018)
*  Revision:       52294
*
*  Description:    Driver for the PKA module
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

#include "pka.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  PKAClearPkaRam
    #define PKAClearPkaRam                  NOROM_PKAClearPkaRam
    #undef  PKAGetOpsStatus
    #define PKAGetOpsStatus                 NOROM_PKAGetOpsStatus
    #undef  PKAArrayAllZeros
    #define PKAArrayAllZeros                NOROM_PKAArrayAllZeros
    #undef  PKAZeroOutArray
    #define PKAZeroOutArray                 NOROM_PKAZeroOutArray
    #undef  PKABigNumModStart
    #define PKABigNumModStart               NOROM_PKABigNumModStart
    #undef  PKABigNumModGetResult
    #define PKABigNumModGetResult           NOROM_PKABigNumModGetResult
    #undef  PKABigNumDivideStart
    #define PKABigNumDivideStart            NOROM_PKABigNumDivideStart
    #undef  PKABigNumDivideGetQuotient
    #define PKABigNumDivideGetQuotient      NOROM_PKABigNumDivideGetQuotient
    #undef  PKABigNumDivideGetRemainder
    #define PKABigNumDivideGetRemainder     NOROM_PKABigNumDivideGetRemainder
    #undef  PKABigNumCmpStart
    #define PKABigNumCmpStart               NOROM_PKABigNumCmpStart
    #undef  PKABigNumCmpGetResult
    #define PKABigNumCmpGetResult           NOROM_PKABigNumCmpGetResult
    #undef  PKABigNumInvModStart
    #define PKABigNumInvModStart            NOROM_PKABigNumInvModStart
    #undef  PKABigNumInvModGetResult
    #define PKABigNumInvModGetResult        NOROM_PKABigNumInvModGetResult
    #undef  PKABigNumMultiplyStart
    #define PKABigNumMultiplyStart          NOROM_PKABigNumMultiplyStart
    #undef  PKABigNumMultGetResult
    #define PKABigNumMultGetResult          NOROM_PKABigNumMultGetResult
    #undef  PKABigNumAddStart
    #define PKABigNumAddStart               NOROM_PKABigNumAddStart
    #undef  PKABigNumAddGetResult
    #define PKABigNumAddGetResult           NOROM_PKABigNumAddGetResult
    #undef  PKABigNumSubStart
    #define PKABigNumSubStart               NOROM_PKABigNumSubStart
    #undef  PKABigNumSubGetResult
    #define PKABigNumSubGetResult           NOROM_PKABigNumSubGetResult
    #undef  PKAEccMultiplyStart
    #define PKAEccMultiplyStart             NOROM_PKAEccMultiplyStart
    #undef  PKAEccMontgomeryMultiplyStart
    #define PKAEccMontgomeryMultiplyStart   NOROM_PKAEccMontgomeryMultiplyStart
    #undef  PKAEccMultiplyGetResult
    #define PKAEccMultiplyGetResult         NOROM_PKAEccMultiplyGetResult
    #undef  PKAEccAddStart
    #define PKAEccAddStart                  NOROM_PKAEccAddStart
    #undef  PKAEccAddGetResult
    #define PKAEccAddGetResult              NOROM_PKAEccAddGetResult
    #undef  PKAEccVerifyPublicKeyWeierstrassStart
    #define PKAEccVerifyPublicKeyWeierstrassStart NOROM_PKAEccVerifyPublicKeyWeierstrassStart
#endif

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  PKAClearPkaRam
    #define PKAClearPkaRam                  NOROM_PKAClearPkaRam
    #undef  PKAGetOpsStatus
    #define PKAGetOpsStatus                 NOROM_PKAGetOpsStatus
    #undef  PKAArrayAllZeros
    #define PKAArrayAllZeros                NOROM_PKAArrayAllZeros
    #undef  PKAZeroOutArray
    #define PKAZeroOutArray                 NOROM_PKAZeroOutArray
    #undef  PKABigNumModStart
    #define PKABigNumModStart               NOROM_PKABigNumModStart
    #undef  PKABigNumModGetResult
    #define PKABigNumModGetResult           NOROM_PKABigNumModGetResult
    #undef  PKABigNumDivideStart
    #define PKABigNumDivideStart            NOROM_PKABigNumDivideStart
    #undef  PKABigNumDivideGetQuotient
    #define PKABigNumDivideGetQuotient      NOROM_PKABigNumDivideGetQuotient
    #undef  PKABigNumDivideGetRemainder
    #define PKABigNumDivideGetRemainder     NOROM_PKABigNumDivideGetRemainder
    #undef  PKABigNumCmpStart
    #define PKABigNumCmpStart               NOROM_PKABigNumCmpStart
    #undef  PKABigNumCmpGetResult
    #define PKABigNumCmpGetResult           NOROM_PKABigNumCmpGetResult
    #undef  PKABigNumInvModStart
    #define PKABigNumInvModStart            NOROM_PKABigNumInvModStart
    #undef  PKABigNumInvModGetResult
    #define PKABigNumInvModGetResult        NOROM_PKABigNumInvModGetResult
    #undef  PKABigNumMultiplyStart
    #define PKABigNumMultiplyStart          NOROM_PKABigNumMultiplyStart
    #undef  PKABigNumMultGetResult
    #define PKABigNumMultGetResult          NOROM_PKABigNumMultGetResult
    #undef  PKABigNumAddStart
    #define PKABigNumAddStart               NOROM_PKABigNumAddStart
    #undef  PKABigNumAddGetResult
    #define PKABigNumAddGetResult           NOROM_PKABigNumAddGetResult
    #undef  PKABigNumSubStart
    #define PKABigNumSubStart               NOROM_PKABigNumSubStart
    #undef  PKABigNumSubGetResult
    #define PKABigNumSubGetResult           NOROM_PKABigNumSubGetResult
    #undef  PKAEccMultiplyStart
    #define PKAEccMultiplyStart             NOROM_PKAEccMultiplyStart
    #undef  PKAEccMontgomeryMultiplyStart
    #define PKAEccMontgomeryMultiplyStart   NOROM_PKAEccMontgomeryMultiplyStart
    #undef  PKAEccMultiplyGetResult
    #define PKAEccMultiplyGetResult         NOROM_PKAEccMultiplyGetResult
    #undef  PKAEccAddStart
    #define PKAEccAddStart                  NOROM_PKAEccAddStart
    #undef  PKAEccAddGetResult
    #define PKAEccAddGetResult              NOROM_PKAEccAddGetResult
    #undef  PKAEccVerifyPublicKeyWeierstrassStart
    #define PKAEccVerifyPublicKeyWeierstrassStart NOROM_PKAEccVerifyPublicKeyWeierstrassStart
#endif



#define MAX(x,y)            (((x) > (y)) ?  (x) : (y))
#define MIN(x,y)            (((x) < (y)) ?  (x) : (y))
#define INRANGE(x,y,z)      ((x) > (y) && (x) < (z))


//*****************************************************************************
//
// Define for the maximum curve size supported by the PKA module in 32 bit
// word.
// \note PKA hardware module can support up to 384 bit curve size due to the
//       2K of PKA RAM.
//
//*****************************************************************************
#define PKA_MAX_CURVE_SIZE_32_BIT_WORD  12

//*****************************************************************************
//
// Define for the maximum length of the big number supported by the PKA module
// in 32 bit word.
//
//*****************************************************************************
#define PKA_MAX_LEN_IN_32_BIT_WORD  PKA_MAX_CURVE_SIZE_32_BIT_WORD

//*****************************************************************************
//
// Used in PKAWritePkaParam() and PKAWritePkaParamExtraOffset() to specify that
// the base address of the parameter should not be written to a NPTR register.
//
//*****************************************************************************
#define PKA_NO_POINTER_REG 0xFF

//*****************************************************************************
//
// NIST P224 constants in little endian format. byte[0] is the least
// significant byte and byte[NISTP224_PARAM_SIZE_BYTES - 1] is the most
// significant.
//
//*****************************************************************************
const PKA_EccPoint224 NISTP224_generator = {
    .x = {.byte = {0x21, 0x1D, 0x5C, 0x11, 0xD6, 0x80, 0x32, 0x34,
                   0x22, 0x11, 0xC2, 0x56, 0xD3, 0xC1, 0x03, 0x4A,
                   0xB9, 0x90, 0x13, 0x32, 0x7F, 0xBF, 0xB4, 0x6B,
                   0xBD, 0x0C, 0x0E, 0xB7, }},
    .y = {.byte = {0x34, 0x7E, 0x00, 0x85, 0x99, 0x81, 0xD5, 0x44,
                   0x64, 0x47, 0x07, 0x5A, 0xA0, 0x75, 0x43, 0xCD,
                   0xE6, 0xDF, 0x22, 0x4C, 0xFB, 0x23, 0xF7, 0xB5,
                   0x88, 0x63, 0x37, 0xBD, }},
};

const PKA_EccParam224 NISTP224_prime       = {.byte = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                       0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
                                                       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                       0xFF, 0xFF, 0xFF, 0xFF}};

const PKA_EccParam224 NISTP224_a           = {.byte = {0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                       0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF,
                                                       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                       0xFF, 0xFF, 0xFF, 0xFF}};

const PKA_EccParam224 NISTP224_b           = {.byte = {0xB4, 0xFF, 0x55, 0x23, 0x43, 0x39, 0x0B, 0x27,
                                                       0xBA, 0xD8, 0xBF, 0xD7, 0xB7, 0xB0, 0x44, 0x50,
                                                       0x56, 0x32, 0x41, 0xF5, 0xAB, 0xB3, 0x04, 0x0C,
                                                       0x85, 0x0A, 0x05, 0xB4}};

const PKA_EccParam224 NISTP224_order       = {.byte = {0x3D, 0x2A, 0x5C, 0x5C, 0x45, 0x29, 0xDD, 0x13,
                                                       0x3E, 0xF0, 0xB8, 0xE0, 0xA2, 0x16, 0xFF, 0xFF,
                                                       0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                                       0xFF, 0xFF, 0xFF, 0xFF}};

//*****************************************************************************
//
// NIST P256 constants in little endian format. byte[0] is the least
// significant byte and byte[NISTP256_PARAM_SIZE_BYTES - 1] is the most
// significant.
//
//*****************************************************************************
const PKA_EccPoint256 NISTP256_generator = {
    .x = {.byte = {0x96, 0xc2, 0x98, 0xd8, 0x45, 0x39, 0xa1, 0xf4,
                   0xa0, 0x33, 0xeb, 0x2d, 0x81, 0x7d, 0x03, 0x77,
                   0xf2, 0x40, 0xa4, 0x63, 0xe5, 0xe6, 0xbc, 0xf8,
                   0x47, 0x42, 0x2c, 0xe1, 0xf2, 0xd1, 0x17, 0x6b}},
    .y = {.byte = {0xf5, 0x51, 0xbf, 0x37, 0x68, 0x40, 0xb6, 0xcb,
                   0xce, 0x5e, 0x31, 0x6b, 0x57, 0x33, 0xce, 0x2b,
                   0x16, 0x9e, 0x0f, 0x7c, 0x4a, 0xeb, 0xe7, 0x8e,
                   0x9b, 0x7f, 0x1a, 0xfe, 0xe2, 0x42, 0xe3, 0x4f}},
};

const PKA_EccParam256 NISTP256_prime       = {.byte = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
                                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                    0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff}};

const PKA_EccParam256 NISTP256_a           = {.byte = {0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
                                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                    0x01, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff}};

const PKA_EccParam256 NISTP256_b           = {.byte = {0x4b, 0x60, 0xd2, 0x27, 0x3e, 0x3c, 0xce, 0x3b,
                                                    0xf6, 0xb0, 0x53, 0xcc, 0xb0, 0x06, 0x1d, 0x65,
                                                    0xbc, 0x86, 0x98, 0x76, 0x55, 0xbd, 0xeb, 0xb3,
                                                    0xe7, 0x93, 0x3a, 0xaa, 0xd8, 0x35, 0xc6, 0x5a}};

const PKA_EccParam256 NISTP256_order       = {.byte = {0x51, 0x25, 0x63, 0xfc, 0xc2, 0xca, 0xb9, 0xf3,
                                                    0x84, 0x9e, 0x17, 0xa7, 0xad, 0xfa, 0xe6, 0xbc,
                                                    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                    0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff}};

//*****************************************************************************
//
// NIST P384 constants in little endian format. byte[0] is the least
// significant byte and byte[NISTP384_PARAM_SIZE_BYTES - 1] is the most
// significant.
//
//*****************************************************************************
const PKA_EccPoint384 NISTP384_generator = {
    .x = {.byte = {0xb7, 0x0a, 0x76, 0x72, 0x38, 0x5e, 0x54, 0x3a,
                   0x6c, 0x29, 0x55, 0xbf, 0x5d, 0xf2, 0x02, 0x55,
                   0x38, 0x2a, 0x54, 0x82, 0xe0, 0x41, 0xf7, 0x59,
                   0x98, 0x9b, 0xa7, 0x8b, 0x62, 0x3b, 0x1d, 0x6e,
                   0x74, 0xad, 0x20, 0xf3, 0x1e, 0xc7, 0xb1, 0x8e,
                   0x37, 0x05, 0x8b, 0xbe, 0x22, 0xca, 0x87, 0xaa}},
    .y = {.byte = {0x5f, 0x0e, 0xea, 0x90, 0x7c, 0x1d, 0x43, 0x7a,
                   0x9d, 0x81, 0x7e, 0x1d, 0xce, 0xb1, 0x60, 0x0a,
                   0xc0, 0xb8, 0xf0, 0xb5, 0x13, 0x31, 0xda, 0xe9,
                   0x7c, 0x14, 0x9a, 0x28, 0xbd, 0x1d, 0xf4, 0xf8,
                   0x29, 0xdc, 0x92, 0x92, 0xbf, 0x98, 0x9e, 0x5d,
                   0x6f, 0x2c, 0x26, 0x96, 0x4a, 0xde, 0x17, 0x36,}},
};

const PKA_EccParam384 NISTP384_prime       = {.byte = {0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
                                                       0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
                                                       0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};

const PKA_EccParam384 NISTP384_a           = {.byte = {0xfc, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00,
                                                       0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
                                                       0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};

const PKA_EccParam384 NISTP384_b           = {.byte = {0xef, 0x2a, 0xec, 0xd3, 0xed, 0xc8, 0x85, 0x2a,
                                                       0x9d, 0xd1, 0x2e, 0x8a, 0x8d, 0x39, 0x56, 0xc6,
                                                       0x5a, 0x87, 0x13, 0x50, 0x8f, 0x08, 0x14, 0x03,
                                                       0x12, 0x41, 0x81, 0xfe, 0x6e, 0x9c, 0x1d, 0x18,
                                                       0x19, 0x2d, 0xf8, 0xe3, 0x6b, 0x05, 0x8e, 0x98,
                                                       0xe4, 0xe7, 0x3e, 0xe2, 0xa7, 0x2f, 0x31, 0xb3}};

const PKA_EccParam384 NISTP384_order       = {.byte = {0x73, 0x29, 0xc5, 0xcc, 0x6a, 0x19, 0xec, 0xec,
                                                       0x7a, 0xa7, 0xb0, 0x48, 0xb2, 0x0d, 0x1a, 0x58,
                                                       0xdf, 0x2d, 0x37, 0xf4, 0x81, 0x4d, 0x63, 0xc7,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};


//*****************************************************************************
//
// NIST P521 constants in little endian format. byte[0] is the least
// significant byte and byte[NISTP521_PARAM_SIZE_BYTES - 1] is the most
// significant.
//
//*****************************************************************************
const PKA_EccPoint521 NISTP521_generator = {
    .x = {.byte = {0x66, 0xbd, 0xe5, 0xc2, 0x31, 0x7e, 0x7e, 0xf9,
                   0x9b, 0x42, 0x6a, 0x85, 0xc1, 0xb3, 0x48, 0x33,
                   0xde, 0xa8, 0xff, 0xa2, 0x27, 0xc1, 0x1d, 0xfe,
                   0x28, 0x59, 0xe7, 0xef, 0x77, 0x5e, 0x4b, 0xa1,
                   0xba, 0x3d, 0x4d, 0x6b, 0x60, 0xaf, 0x28, 0xf8,
                   0x21, 0xb5, 0x3f, 0x05, 0x39, 0x81, 0x64, 0x9c,
                   0x42, 0xb4, 0x95, 0x23, 0x66, 0xcb, 0x3e, 0x9e,
                   0xcd, 0xe9, 0x04, 0x04, 0xb7, 0x06, 0x8e, 0x85,
                   0xc6, 0x00}},
    .y = {.byte = {0x50, 0x66, 0xd1, 0x9f, 0x76, 0x94, 0xbe, 0x88,
                   0x40, 0xc2, 0x72, 0xa2, 0x86, 0x70, 0x3c, 0x35,
                   0x61, 0x07, 0xad, 0x3f, 0x01, 0xb9, 0x50, 0xc5,
                   0x40, 0x26, 0xf4, 0x5e, 0x99, 0x72, 0xee, 0x97,
                   0x2c, 0x66, 0x3e, 0x27, 0x17, 0xbd, 0xaf, 0x17,
                   0x68, 0x44, 0x9b, 0x57, 0x49, 0x44, 0xf5, 0x98,
                   0xd9, 0x1b, 0x7d, 0x2c, 0xb4, 0x5f, 0x8a, 0x5c,
                   0x04, 0xc0, 0x3b, 0x9a, 0x78, 0x6a, 0x29, 0x39,
                   0x18, 0x01}},
};

const PKA_EccParam521 NISTP521_prime       = {.byte = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0x01}};

const PKA_EccParam521 NISTP521_a           = {.byte = {0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0x01}};

const PKA_EccParam521 NISTP521_b           = {.byte = {0x00, 0x3f, 0x50, 0x6b, 0xd4, 0x1f, 0x45, 0xef,
                                                       0xf1, 0x34, 0x2c, 0x3d, 0x88, 0xdf, 0x73, 0x35,
                                                       0x07, 0xbf, 0xb1, 0x3b, 0xbd, 0xc0, 0x52, 0x16,
                                                       0x7b, 0x93, 0x7e, 0xec, 0x51, 0x39, 0x19, 0x56,
                                                       0xe1, 0x09, 0xf1, 0x8e, 0x91, 0x89, 0xb4, 0xb8,
                                                       0xf3, 0x15, 0xb3, 0x99, 0x5b, 0x72, 0xda, 0xa2,
                                                       0xee, 0x40, 0x85, 0xb6, 0xa0, 0x21, 0x9a, 0x92,
                                                       0x1f, 0x9a, 0x1c, 0x8e, 0x61, 0xb9, 0x3e, 0x95,
                                                       0x51, 0x00}};

const PKA_EccParam521 NISTP521_order       = {.byte = {0x09, 0x64, 0x38, 0x91, 0x1e, 0xb7, 0x6f, 0xbb,
                                                       0xae, 0x47, 0x9c, 0x89, 0xb8, 0xc9, 0xb5, 0x3b,
                                                       0xd0, 0xa5, 0x09, 0xf7, 0x48, 0x01, 0xcc, 0x7f,
                                                       0x6b, 0x96, 0x2f, 0xbf, 0x83, 0x87, 0x86, 0x51,
                                                       0xfa, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                       0xff, 0x01}};


//*****************************************************************************
//
// Brainpool P256r1 constants in little endian format. byte[0] is the least
// significant byte and byte[BrainpoolP256R1_PARAM_SIZE_BYTES - 1] is the most
// significant.
//
//*****************************************************************************
const PKA_EccPoint256 BrainpoolP256R1_generator = {
    .x = {.byte = {0x62, 0x32, 0xCE, 0x9A, 0xBD, 0x53, 0x44, 0x3A,
                   0xC2, 0x23, 0xBD, 0xE3, 0xE1, 0x27, 0xDE, 0xB9,
                   0xAF, 0xB7, 0x81, 0xFC, 0x2F, 0x48, 0x4B, 0x2C,
                   0xCB, 0x57, 0x7E, 0xCB, 0xB9, 0xAE, 0xD2, 0x8B}},
    .y = {.byte = {0x97, 0x69, 0x04, 0x2F, 0xC7, 0x54, 0x1D, 0x5C,
                   0x54, 0x8E, 0xED, 0x2D, 0x13, 0x45, 0x77, 0xC2,
                   0xC9, 0x1D, 0x61, 0x14, 0x1A, 0x46, 0xF8, 0x97,
                   0xFD, 0xC4, 0xDA, 0xC3, 0x35, 0xF8, 0x7E, 0x54}},
};

const PKA_EccParam256 BrainpoolP256R1_prime       = {.byte = {0x77, 0x53, 0x6E, 0x1F, 0x1D, 0x48, 0x13, 0x20,
                                                              0x28, 0x20, 0x26, 0xD5, 0x23, 0xF6, 0x3B, 0x6E,
                                                              0x72, 0x8D, 0x83, 0x9D, 0x90, 0x0A, 0x66, 0x3E,
                                                              0xBC, 0xA9, 0xEE, 0xA1, 0xDB, 0x57, 0xFB, 0xA9}};

const PKA_EccParam256 BrainpoolP256R1_a           = {.byte = {0xD9, 0xB5, 0x30, 0xF3, 0x44, 0x4B, 0x4A, 0xE9,
                                                              0x6C, 0x5C, 0xDC, 0x26, 0xC1, 0x55, 0x80, 0xFB,
                                                              0xE7, 0xFF, 0x7A, 0x41, 0x30, 0x75, 0xF6, 0xEE,
                                                              0x57, 0x30, 0x2C, 0xFC, 0x75, 0x09, 0x5A, 0x7D}};

const PKA_EccParam256 BrainpoolP256R1_b           = {.byte = {0xB6, 0x07, 0x8C, 0xFF, 0x18, 0xDC, 0xCC, 0x6B,
                                                              0xCE, 0xE1, 0xF7, 0x5C, 0x29, 0x16, 0x84, 0x95,
                                                              0xBF, 0x7C, 0xD7, 0xBB, 0xD9, 0xB5, 0x30, 0xF3,
                                                              0x44, 0x4B, 0x4A, 0xE9, 0x6C, 0x5C, 0xDC, 0x26,}};

const PKA_EccParam256 BrainpoolP256R1_order       = {.byte = {0xA7, 0x56, 0x48, 0x97, 0x82, 0x0E, 0x1E, 0x90,
                                                              0xF7, 0xA6, 0x61, 0xB5, 0xA3, 0x7A, 0x39, 0x8C,
                                                              0x71, 0x8D, 0x83, 0x9D, 0x90, 0x0A, 0x66, 0x3E,
                                                              0xBC, 0xA9, 0xEE, 0xA1, 0xDB, 0x57, 0xFB, 0xA9}};

//*****************************************************************************
//
// Brainpool P384r1 constants in little endian format. byte[0] is the least
// significant byte and byte[BrainpoolP384R1_PARAM_SIZE_BYTES - 1] is the most
// significant.
//
//*****************************************************************************
const PKA_EccPoint384 BrainpoolP384R1_generator = {
    .x = {.byte = {0x1E, 0xAF, 0xD4, 0x47, 0xE2, 0xB2, 0x87, 0xEF,
                   0xAA, 0x46, 0xD6, 0x36, 0x34, 0xE0, 0x26, 0xE8,
                   0xE8, 0x10, 0xBD, 0x0C, 0xFE, 0xCA, 0x7F, 0xDB,
                   0xE3, 0x4F, 0xF1, 0x7E, 0xE7, 0xA3, 0x47, 0x88,
                   0x6B, 0x3F, 0xC1, 0xB7, 0x81, 0x3A, 0xA6, 0xA2,
                   0xFF, 0x45, 0xCF, 0x68, 0xF0, 0x64, 0x1C, 0x1D}},
    .y = {.byte = {0x15, 0x53, 0x3C, 0x26, 0x41, 0x03, 0x82, 0x42,
                   0x11, 0x81, 0x91, 0x77, 0x21, 0x46, 0x46, 0x0E,
                   0x28, 0x29, 0x91, 0xF9, 0x4F, 0x05, 0x9C, 0xE1,
                   0x64, 0x58, 0xEC, 0xFE, 0x29, 0x0B, 0xB7, 0x62,
                   0x52, 0xD5, 0xCF, 0x95, 0x8E, 0xEB, 0xB1, 0x5C,
                   0xA4, 0xC2, 0xF9, 0x20, 0x75, 0x1D, 0xBE, 0x8A}},
};

const PKA_EccParam384 BrainpoolP384R1_prime       = {.byte = {0x53, 0xEC, 0x07, 0x31, 0x13, 0x00, 0x47, 0x87,
                                                              0x71, 0x1A, 0x1D, 0x90, 0x29, 0xA7, 0xD3, 0xAC,
                                                              0x23, 0x11, 0xB7, 0x7F, 0x19, 0xDA, 0xB1, 0x12,
                                                              0xB4, 0x56, 0x54, 0xED, 0x09, 0x71, 0x2F, 0x15,
                                                              0xDF, 0x41, 0xE6, 0x50, 0x7E, 0x6F, 0x5D, 0x0F,
                                                              0x28, 0x6D, 0x38, 0xA3, 0x82, 0x1E, 0xB9, 0x8C}};

const PKA_EccParam384 BrainpoolP384R1_a           = {.byte = {0x26, 0x28, 0xCE, 0x22, 0xDD, 0xC7, 0xA8, 0x04,
                                                              0xEB, 0xD4, 0x3A, 0x50, 0x4A, 0x81, 0xA5, 0x8A,
                                                              0x0F, 0xF9, 0x91, 0xBA, 0xEF, 0x65, 0x91, 0x13,
                                                              0x87, 0x27, 0xB2, 0x4F, 0x8E, 0xA2, 0xBE, 0xC2,
                                                              0xA0, 0xAF, 0x05, 0xCE, 0x0A, 0x08, 0x72, 0x3C,
                                                              0x0C, 0x15, 0x8C, 0x3D, 0xC6, 0x82, 0xC3, 0x7B}};

const PKA_EccParam384 BrainpoolP384R1_b           = {.byte = {0x11, 0x4C, 0x50, 0xFA, 0x96, 0x86, 0xB7, 0x3A,
                                                              0x94, 0xC9, 0xDB, 0x95, 0x02, 0x39, 0xB4, 0x7C,
                                                              0xD5, 0x62, 0xEB, 0x3E, 0xA5, 0x0E, 0x88, 0x2E,
                                                              0xA6, 0xD2, 0xDC, 0x07, 0xE1, 0x7D, 0xB7, 0x2F,
                                                              0x7C, 0x44, 0xF0, 0x16, 0x54, 0xB5, 0x39, 0x8B,
                                                              0x26, 0x28, 0xCE, 0x22, 0xDD, 0xC7, 0xA8, 0x04}};

const PKA_EccParam384 BrainpoolP384R1_order       = {.byte = {0x65, 0x65, 0x04, 0xE9, 0x02, 0x32, 0x88, 0x3B,
                                                              0x10, 0xC3, 0x7F, 0x6B, 0xAF, 0xB6, 0x3A, 0xCF,
                                                              0xA7, 0x25, 0x04, 0xAC, 0x6C, 0x6E, 0x16, 0x1F,
                                                              0xB3, 0x56, 0x54, 0xED, 0x09, 0x71, 0x2F, 0x15,
                                                              0xDF, 0x41, 0xE6, 0x50, 0x7E, 0x6F, 0x5D, 0x0F,
                                                              0x28, 0x6D, 0x38, 0xA3, 0x82, 0x1E, 0xB9, 0x8C}};

//*****************************************************************************
//
// Brainpool P512r1 constants in little endian format. byte[0] is the least
// significant byte and byte[BrainpoolP512R1_PARAM_SIZE_BYTES - 1] is the most
// significant.
//
//*****************************************************************************
const PKA_EccPoint512 BrainpoolP512R1_generator = {
    .x = {.byte = {0x22, 0xF8, 0xB9, 0xBC, 0x09, 0x22, 0x35, 0x8B,
                   0x68, 0x5E, 0x6A, 0x40, 0x47, 0x50, 0x6D, 0x7C,
                   0x5F, 0x7D, 0xB9, 0x93, 0x7B, 0x68, 0xD1, 0x50,
                   0x8D, 0xD4, 0xD0, 0xE2, 0x78, 0x1F, 0x3B, 0xFF,
                   0x8E, 0x09, 0xD0, 0xF4, 0xEE, 0x62, 0x3B, 0xB4,
                   0xC1, 0x16, 0xD9, 0xB5, 0x70, 0x9F, 0xED, 0x85,
                   0x93, 0x6A, 0x4C, 0x9C, 0x2E, 0x32, 0x21, 0x5A,
                   0x64, 0xD9, 0x2E, 0xD8, 0xBD, 0xE4, 0xAE, 0x81}},
    .y = {.byte = {0x92, 0x08, 0xD8, 0x3A, 0x0F, 0x1E, 0xCD, 0x78,
                   0x06, 0x54, 0xF0, 0xA8, 0x2F, 0x2B, 0xCA, 0xD1,
                   0xAE, 0x63, 0x27, 0x8A, 0xD8, 0x4B, 0xCA, 0x5B,
                   0x5E, 0x48, 0x5F, 0x4A, 0x49, 0xDE, 0xDC, 0xB2,
                   0x11, 0x81, 0x1F, 0x88, 0x5B, 0xC5, 0x00, 0xA0,
                   0x1A, 0x7B, 0xA5, 0x24, 0x00, 0xF7, 0x09, 0xF2,
                   0xFD, 0x22, 0x78, 0xCF, 0xA9, 0xBF, 0xEA, 0xC0,
                   0xEC, 0x32, 0x63, 0x56, 0x5D, 0x38, 0xDE, 0x7D}},
};

const PKA_EccParam512 BrainpoolP512R1_prime       = {.byte = {0xF3, 0x48, 0x3A, 0x58, 0x56, 0x60, 0xAA, 0x28,
                                                              0x85, 0xC6, 0x82, 0x2D, 0x2F, 0xFF, 0x81, 0x28,
                                                              0xE6, 0x80, 0xA3, 0xE6, 0x2A, 0xA1, 0xCD, 0xAE,
                                                              0x42, 0x68, 0xC6, 0x9B, 0x00, 0x9B, 0x4D, 0x7D,
                                                              0x71, 0x08, 0x33, 0x70, 0xCA, 0x9C, 0x63, 0xD6,
                                                              0x0E, 0xD2, 0xC9, 0xB3, 0xB3, 0x8D, 0x30, 0xCB,
                                                              0x07, 0xFC, 0xC9, 0x33, 0xAE, 0xE6, 0xD4, 0x3F,
                                                              0x8B, 0xC4, 0xE9, 0xDB, 0xB8, 0x9D, 0xDD, 0xAA}};

const PKA_EccParam512 BrainpoolP512R1_a           = {.byte = {0xCA, 0x94, 0xFC, 0x77, 0x4D, 0xAC, 0xC1, 0xE7,
                                                              0xB9, 0xC7, 0xF2, 0x2B, 0xA7, 0x17, 0x11, 0x7F,
                                                              0xB5, 0xC8, 0x9A, 0x8B, 0xC9, 0xF1, 0x2E, 0x0A,
                                                              0xA1, 0x3A, 0x25, 0xA8, 0x5A, 0x5D, 0xED, 0x2D,
                                                              0xBC, 0x63, 0x98, 0xEA, 0xCA, 0x41, 0x34, 0xA8,
                                                              0x10, 0x16, 0xF9, 0x3D, 0x8D, 0xDD, 0xCB, 0x94,
                                                              0xC5, 0x4C, 0x23, 0xAC, 0x45, 0x71, 0x32, 0xE2,
                                                              0x89, 0x3B, 0x60, 0x8B, 0x31, 0xA3, 0x30, 0x78}};

const PKA_EccParam512 BrainpoolP512R1_b           = {.byte = {0x23, 0xF7, 0x16, 0x80, 0x63, 0xBD, 0x09, 0x28,
                                                              0xDD, 0xE5, 0xBA, 0x5E, 0xB7, 0x50, 0x40, 0x98,
                                                              0x67, 0x3E, 0x08, 0xDC, 0xCA, 0x94, 0xFC, 0x77,
                                                              0x4D, 0xAC, 0xC1, 0xE7, 0xB9, 0xC7, 0xF2, 0x2B,
                                                              0xA7, 0x17, 0x11, 0x7F, 0xB5, 0xC8, 0x9A, 0x8B,
                                                              0xC9, 0xF1, 0x2E, 0x0A, 0xA1, 0x3A, 0x25, 0xA8,
                                                              0x5A, 0x5D, 0xED, 0x2D, 0xBC, 0x63, 0x98, 0xEA,
                                                              0xCA, 0x41, 0x34, 0xA8, 0x10, 0x16, 0xF9, 0x3D}};

const PKA_EccParam512 BrainpoolP512R1_order       = {.byte = {0x69, 0x00, 0xA9, 0x9C, 0x82, 0x96, 0x87, 0xB5,
                                                              0xDD, 0xDA, 0x5D, 0x08, 0x81, 0xD3, 0xB1, 0x1D,
                                                              0x47, 0x10, 0xAC, 0x7F, 0x19, 0x61, 0x86, 0x41,
                                                              0x19, 0x26, 0xA9, 0x4C, 0x41, 0x5C, 0x3E, 0x55,
                                                              0x70, 0x08, 0x33, 0x70, 0xCA, 0x9C, 0x63, 0xD6,
                                                              0x0E, 0xD2, 0xC9, 0xB3, 0xB3, 0x8D, 0x30, 0xCB,
                                                              0x07, 0xFC, 0xC9, 0x33, 0xAE, 0xE6, 0xD4, 0x3F,
                                                              0x8B, 0xC4, 0xE9, 0xDB, 0xB8, 0x9D, 0xDD, 0xAA}};

//*****************************************************************************
//
// Curve25519 constants in little endian format. byte[0] is the least
// significant byte and byte[Curve25519_PARAM_SIZE_BYTES - 1] is the most
// significant.
//
//*****************************************************************************
const PKA_EccPoint256 Curve25519_generator = {
    .x = {.byte = {0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}},
    .y = {.byte = {0xd9, 0xd3, 0xce, 0x7e, 0xa2, 0xc5, 0xe9, 0x29,
                   0xb2, 0x61, 0x7c, 0x6d, 0x7e, 0x4d, 0x3d, 0x92,
                   0x4c, 0xd1, 0x48, 0x77, 0x2c, 0xdd, 0x1e, 0xe0,
                   0xb4, 0x86, 0xa0, 0xb8, 0xa1, 0x19, 0xae, 0x20}},
};

const PKA_EccParam256 Curve25519_prime       = {.byte = {0xed, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                                         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f}};

const PKA_EccParam256 Curve25519_a           = {.byte = {0x06, 0x6d, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}};

const PKA_EccParam256 Curve25519_b           = {.byte = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}};

const PKA_EccParam256 Curve25519_order       = {.byte = {0xb9, 0xdc, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58,
                                                         0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14,
                                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,}};


//*****************************************************************************
//
// Zeroize PKA RAM. Not threadsafe.
//
//*****************************************************************************
void PKAClearPkaRam(void){
    // Get initial state
    uint32_t secdmaclkgr = HWREG(PRCM_BASE + PRCM_O_SECDMACLKGR);

    // OR in zeroize bit
    secdmaclkgr |= PRCM_SECDMACLKGR_PKA_ZERIOZE_RESET_N;

    // Start zeroization
    HWREG(PRCM_BASE + PRCM_O_SECDMACLKGR) = secdmaclkgr;

    // Wait 256 cycles for PKA RAM to be cleared
    CPUdelay(256 / 4);

    // Turn off zeroization
    HWREG(PRCM_BASE + PRCM_O_SECDMACLKGR) = secdmaclkgr & (~PRCM_SECDMACLKGR_PKA_ZERIOZE_RESET_N);
}

//*****************************************************************************
//
// Write a PKA parameter to the PKA module, set required registers, and add an offset.
//
//*****************************************************************************
static uint32_t PKAWritePkaParam(const uint8_t *param, uint32_t paramLength, uint32_t paramOffset, uint32_t ptrRegOffset)
{
    uint32_t i;
    uint32_t *paramWordAlias = (uint32_t *)param;
    // Take the floor of paramLength in 32-bit words
    uint32_t paramLengthInWords = paramLength / sizeof(uint32_t);

    // Only copy data if it is specified. We may wish to simply allocate another buffer and get
    // the required offset.
    if (param) {
        // Load the number in PKA RAM
        for (i = 0; i < paramLengthInWords; i++) {
            HWREG(PKA_RAM_BASE + paramOffset + sizeof(uint32_t) * i) = paramWordAlias[i];
        }

        // If the length is not a word-multiple, fill up a temporary word and copy that in
        // to avoid a bus error. The extra zeros at the end should not matter, as the large
        // number is little-endian and thus has no effect.
        // We could have correctly calculated ceiling(paramLength / sizeof(uint32_t)) above.
        // However, we would not have been able to zero-out the extra few most significant
        // bytes of the most significant word. That would have resulted in doing maths operations
        // on whatever follows param in RAM.
        if (paramLength % sizeof(uint32_t)) {
            uint32_t temp = 0;
            uint8_t j;

            // Load the entire word line of the param remainder
            temp = paramWordAlias[i];

            // Zero-out all bytes beyond the end of the param
            for (j = paramLength % sizeof(uint32_t); j < sizeof(uint32_t); j++) {
                ((uint8_t *)&temp)[j] = 0;
            }

            HWREG(PKA_RAM_BASE + paramOffset + sizeof(uint32_t) * i) = temp;

            // Increment paramLengthInWords since we take the ceiling of length / sizeof(uint32_t)
            paramLengthInWords++;
        }
    }

    // Update the A, B, C, or D pointer with the offset address of the PKA RAM location
    // where the number will be stored.
    switch (ptrRegOffset) {
        case PKA_O_APTR:
            HWREG(PKA_BASE + PKA_O_APTR) = paramOffset >> 2;
            HWREG(PKA_BASE + PKA_O_ALENGTH) = paramLengthInWords;
            break;
        case PKA_O_BPTR:
            HWREG(PKA_BASE + PKA_O_BPTR) = paramOffset >> 2;
            HWREG(PKA_BASE + PKA_O_BLENGTH) = paramLengthInWords;
            break;
        case PKA_O_CPTR:
            HWREG(PKA_BASE + PKA_O_CPTR) = paramOffset >> 2;
            break;
        case PKA_O_DPTR:
            HWREG(PKA_BASE + PKA_O_DPTR) = paramOffset >> 2;
            break;
    }

    // Ensure 8-byte alignment of next parameter.
    // Returns the offset for the next parameter.
    return paramOffset + sizeof(uint32_t) * (paramLengthInWords + (paramLengthInWords % 2));
}

//*****************************************************************************
//
// Write a PKA parameter to the PKA module but return a larger offset.
//
//*****************************************************************************
static uint32_t PKAWritePkaParamExtraOffset(const uint8_t *param, uint32_t paramLength, uint32_t paramOffset, uint32_t ptrRegOffset)
{
    // Ensure 16-byte alignment.
    return  (sizeof(uint32_t) * 2) + PKAWritePkaParam(param, paramLength, paramOffset, ptrRegOffset);
}

//*****************************************************************************
//
// Writes the result of a large number arithmetic operation to a provided buffer.
//
//*****************************************************************************
static uint32_t PKAGetBigNumResult(uint8_t *resultBuf, uint32_t *resultLength, uint32_t resultPKAMemAddr)
{
    uint32_t mswOffset;
    uint32_t lswOffset;
    uint32_t lengthInWords;
    uint32_t i;
    uint32_t *resultWordAlias = (uint32_t *)resultBuf;

    // Check the arguments.
    ASSERT(resultBuf);
    ASSERT((resultPKAMemAddr > PKA_RAM_BASE) &&
           (resultPKAMemAddr < (PKA_RAM_BASE + PKA_RAM_TOT_BYTE_SIZE)));

    // Verify that the operation is complete.
    if (HWREG(PKA_BASE + PKA_O_FUNCTION) & PKA_FUNCTION_RUN) {
        return PKA_STATUS_OPERATION_BUSY;
    }

    // Get the MSW register value.
    mswOffset = HWREG(PKA_BASE + PKA_O_MSW);

    // If the result vector is zero, write back one zero byte so the caller does not need
    // to handle a special error for the perhaps valid result of zero.
    // They will only get the error status if they do not provide a buffer
    if (mswOffset & PKA_MSW_RESULT_IS_ZERO_M) {
        if (*resultLength){
            if(resultBuf){
                resultBuf[0] = 0;
            }

            *resultLength = 1;

            return PKA_STATUS_SUCCESS;
        }
        else {
            return PKA_STATUS_BUF_UNDERFLOW;
        }
    }

    // Get the length of the result
    mswOffset = ((mswOffset & PKA_MSW_MSW_ADDRESS_M) + 1);
    lswOffset = ((resultPKAMemAddr - PKA_RAM_BASE) >> 2);

    if (mswOffset >= lswOffset) {
        lengthInWords = mswOffset - lswOffset;
    }
    else {
        return PKA_STATUS_RESULT_ADDRESS_INCORRECT;
    }

    // Check if the provided buffer length is adequate to store the result data.
    if (*resultLength < lengthInWords * sizeof(uint32_t)) {
        return PKA_STATUS_BUF_UNDERFLOW;
    }

    // Copy the resultant length.
    *resultLength = lengthInWords * sizeof(uint32_t);


    if (resultBuf) {
        // Copy the result into the resultBuf.
        for (i = 0; i < lengthInWords; i++) {
            resultWordAlias[i]= HWREG(resultPKAMemAddr + sizeof(uint32_t) * i);
        }
    }

    return PKA_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Retrieve the result of a modulo operation or the remainder of a division.
//
//*****************************************************************************
static uint32_t PKAGetBigNumResultRemainder(uint8_t *resultBuf, uint32_t *resultLength, uint32_t resultPKAMemAddr)
{
    uint32_t regMSWVal;
    uint32_t lengthInWords;
    uint32_t i;
    uint32_t *resultWordAlias = (uint32_t *)resultBuf;

    // Check the arguments.
    ASSERT(resultBuf);
    ASSERT((resultPKAMemAddr > PKA_RAM_BASE) &&
           (resultPKAMemAddr < (PKA_RAM_BASE + PKA_RAM_TOT_BYTE_SIZE)));

    // Verify that the operation is complete.
    if (HWREG(PKA_BASE + PKA_O_FUNCTION) & PKA_FUNCTION_RUN) {
        return PKA_STATUS_OPERATION_BUSY;
    }

    // Get the MSW register value.
    regMSWVal = HWREG(PKA_BASE + PKA_O_DIVMSW);

    // If the result vector is zero, write back one zero byte so the caller does not need
    // to handle a special error for the perhaps valid result of zero.
    // They will only get the error status if they do not provide a buffer
    if (regMSWVal & PKA_DIVMSW_RESULT_IS_ZERO_M) {
        if (*resultLength){
            if(resultBuf){
                resultBuf[0] = 0;
            }

            *resultLength = 1;

            return PKA_STATUS_SUCCESS;
        }
        else {
            return PKA_STATUS_BUF_UNDERFLOW;
        }
    }

    // Get the length of the result
    lengthInWords = ((regMSWVal & PKA_DIVMSW_MSW_ADDRESS_M) + 1) - ((resultPKAMemAddr - PKA_RAM_BASE) >> 2);

    // Check if the provided buffer length is adequate to store the result data.
    if (*resultLength < lengthInWords * sizeof(uint32_t)) {
        return PKA_STATUS_BUF_UNDERFLOW;
    }

    // Copy the resultant length.
    *resultLength = lengthInWords * sizeof(uint32_t);

    if (resultBuf) {
        // Copy the result into the resultBuf.
        for (i = 0; i < lengthInWords; i++) {
            resultWordAlias[i] = HWREG(resultPKAMemAddr + sizeof(uint32_t) * i);
        }
    }

    return PKA_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Writes the resultant curve point of an ECC operation to the provided buffer.
//
//*****************************************************************************
static uint32_t PKAGetECCResult(uint8_t *curvePointX, uint8_t *curvePointY, uint32_t resultPKAMemAddr, uint32_t length)
{
    uint32_t i = 0;
    uint32_t *xWordAlias = (uint32_t *)curvePointX;
    uint32_t *yWordAlias = (uint32_t *)curvePointY;
    uint32_t lengthInWordsCeiling = 0;

    // Check for the arguments.
    ASSERT(curvePointX);
    ASSERT(curvePointY);
    ASSERT((resultPKAMemAddr > PKA_RAM_BASE) &&
           (resultPKAMemAddr < (PKA_RAM_BASE + PKA_RAM_TOT_BYTE_SIZE)));

    // Verify that the operation is completed.
    if (HWREG(PKA_BASE + PKA_O_FUNCTION) & PKA_FUNCTION_RUN) {
        return PKA_STATUS_OPERATION_BUSY;
    }

    if (HWREG(PKA_BASE + PKA_O_SHIFT)) {
         return PKA_STATUS_FAILURE;
    }

    // Check to make sure that the result vector is not the point at infinity.
    if (HWREG(PKA_BASE + PKA_O_MSW) & PKA_MSW_RESULT_IS_ZERO) {
        return PKA_STATUS_POINT_AT_INFINITY;
    }

    if (curvePointX != NULL) {
        // Copy the x co-ordinate value of the result from vector D into
        // the curvePoint.
        for (i = 0; i < (length / sizeof(uint32_t)); i++) {
            xWordAlias[i] = HWREG(resultPKAMemAddr + sizeof(uint32_t) * i);
        }

        // If the length is not a word-multiple, fill up a temporary word and copy that in
        // to avoid a bus error.
        if (length % sizeof(uint32_t)) {
            uint32_t temp = 0;
            uint8_t j;

            // Load the entire word line of the coordinate remainder
            temp = HWREG(resultPKAMemAddr + sizeof(uint32_t) * i);

            // Write all remaining bytes to the coordinate
            for (j = 0; j < length % sizeof(uint32_t); j++) {
                curvePointX[i * sizeof(uint32_t) + j] = ((uint8_t *)&temp)[j];
            }

        }
    }

    lengthInWordsCeiling = (length % sizeof(uint32_t)) ? length / sizeof(uint32_t) + 1 : length / sizeof(uint32_t);

    resultPKAMemAddr += sizeof(uint32_t) * (2 + lengthInWordsCeiling + (lengthInWordsCeiling % 2));

    if (curvePointY != NULL) {
        // Copy the y co-ordinate value of the result from vector D into
        // the curvePoint.
        for (i = 0; i < (length / sizeof(uint32_t)); i++) {
            yWordAlias[i] = HWREG(resultPKAMemAddr + sizeof(uint32_t) * i);
        }

        // If the length is not a word-multiple, fill up a temporary word and copy that in
        // to avoid a bus error.
        if (length % sizeof(uint32_t)) {
            uint32_t temp = 0;
            uint8_t j;

            // Load the entire word line of the coordinate remainder
            temp = HWREG(resultPKAMemAddr + sizeof(uint32_t) * i);

            // Write all remaining bytes to the coordinate
            for (j = 0; j < length % sizeof(uint32_t); j++) {
                curvePointY[i * sizeof(uint32_t) + j] = ((uint8_t *)&temp)[j];
            }
        }
    }


    return PKA_STATUS_SUCCESS;
}


//*****************************************************************************
//
// Provides the PKA operation status.
//
//*****************************************************************************
uint32_t PKAGetOpsStatus(void)
{
    if (HWREG(PKA_BASE + PKA_O_FUNCTION) & PKA_FUNCTION_RUN_M) {
        return PKA_STATUS_OPERATION_BUSY;
    }
    else {
        return PKA_STATUS_OPERATION_RDY;
    }
}

//*****************************************************************************
//
// Check if an array consists only of zeros.
//
//*****************************************************************************
bool PKAArrayAllZeros(const uint8_t *array, uint32_t arrayLength)
{
    uint32_t i;
    uint8_t arrayBits = 0;

    // We could speed things up by comparing word-wise rather than byte-wise.
    // However, this extra overhead is inconsequential compared to running an
    // actual PKA operation. Especially ECC operations.
    for (i = 0; i < arrayLength; i++) {
        arrayBits |= array[i];
    }

    if (arrayBits) {
        return false;
    }
    else {
        return true;
    }

}

//*****************************************************************************
//
// Fill an array with zeros
//
//*****************************************************************************
void PKAZeroOutArray(const uint8_t *array, uint32_t arrayLength)
{
    uint32_t i;
    // Take the floor of paramLength in 32-bit words
    uint32_t arrayLengthInWords = arrayLength / sizeof(uint32_t);

    // Zero-out the array word-wise until i >= arrayLength
    for (i = 0; i < arrayLengthInWords * sizeof(uint32_t); i += 4) {
        HWREG(array + i) = 0;
    }

    // If i != arrayLength, there are some remaining bytes to zero-out
    if (arrayLength % sizeof(uint32_t)) {
        // Subtract 4 from i, since i has already overshot the array
        for (i -= 4; i < arrayLength; i++) {
            HWREGB(array + i * sizeof(uint32_t));
        }
    }
}

//*****************************************************************************
//
// Start the big number modulus operation.
//
//*****************************************************************************
uint32_t PKABigNumModStart(const uint8_t *bigNum, uint32_t bigNumLength, const uint8_t *modulus, uint32_t modulusLength, uint32_t *resultPKAMemAddr)
{
    uint32_t offset = 0;

    // Check the arguments.
    ASSERT(bigNum);
    ASSERT(modulus);
    ASSERT(resultPKAMemAddr);

    // Make sure no operation is in progress.
    if (HWREG(PKA_BASE + PKA_O_FUNCTION) & PKA_FUNCTION_RUN) {
        return PKA_STATUS_OPERATION_BUSY;
    }

    offset = PKAWritePkaParam(bigNum, bigNumLength, offset, PKA_O_APTR);

    offset = PKAWritePkaParamExtraOffset(modulus, modulusLength, offset, PKA_O_BPTR);

    // Copy the result vector address location.
    *resultPKAMemAddr = PKA_RAM_BASE + offset;

    // Load C pointer with the result location in PKA RAM
    HWREG(PKA_BASE + PKA_O_CPTR) = offset >> 2;

    // Start the PKCP modulo operation by setting the PKA Function register.
    HWREG(PKA_BASE + PKA_O_FUNCTION) = (PKA_FUNCTION_RUN | PKA_FUNCTION_MODULO);

    return PKA_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Get the result of the big number modulus operation.
//
//*****************************************************************************
uint32_t PKABigNumModGetResult(uint8_t *resultBuf, uint32_t length, uint32_t resultPKAMemAddr)
{
    // Zero-out array in case modulo result is shorter than length
    PKAZeroOutArray(resultBuf, length);

    return PKAGetBigNumResultRemainder(resultBuf, &length, resultPKAMemAddr);
}

//*****************************************************************************
//
// Start the big number divide operation.
//
//*****************************************************************************
uint32_t PKABigNumDivideStart(const uint8_t *dividend, uint32_t dividendLength, const uint8_t *divisor, uint32_t divisorLength, uint32_t *resultQuotientMemAddr, uint32_t *resultRemainderMemAddr)
{
    uint32_t offset = 0;

    // Check the arguments.
    ASSERT(dividend);
    ASSERT(divisor);
    ASSERT(resultQuotientMemAddr);
    ASSERT(resultRemainderMemAddr);

    // Make sure no operation is in progress.
    if (HWREG(PKA_BASE + PKA_O_FUNCTION) & PKA_FUNCTION_RUN) {
        return PKA_STATUS_OPERATION_BUSY;
    }

    offset = PKAWritePkaParam(dividend, dividendLength, offset, PKA_O_APTR);

    offset = PKAWritePkaParamExtraOffset(divisor, divisorLength, offset, PKA_O_BPTR);

    // Copy the remainder result vector address location.
    if (resultRemainderMemAddr) {
        *resultRemainderMemAddr = PKA_RAM_BASE + offset;
    }

    // The remainder cannot ever be larger than the divisor. It should fit inside
    // a buffer of that size.
    offset = PKAWritePkaParamExtraOffset(0, divisorLength, offset, PKA_O_CPTR);

    // Copy the remainder result vector address location.
    if (resultQuotientMemAddr) {
        *resultQuotientMemAddr = PKA_RAM_BASE + offset;
    }

    // Load D pointer with the quotient location in PKA RAM
    HWREG(PKA_BASE + PKA_O_DPTR) = offset >> 2;

    // Start the PKCP modulo operation by setting the PKA Function register.
    HWREG(PKA_BASE + PKA_O_FUNCTION) = (PKA_FUNCTION_RUN | PKA_FUNCTION_DIVIDE);

    return PKA_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Get the quotient of the big number divide operation.
//
//*****************************************************************************
uint32_t PKABigNumDivideGetQuotient(uint8_t *resultBuf, uint32_t *length, uint32_t resultQuotientMemAddr)
{
    return PKAGetBigNumResult(resultBuf, length, resultQuotientMemAddr);
}

//*****************************************************************************
//
// Get the remainder of the big number divide operation.
//
//*****************************************************************************
uint32_t PKABigNumDivideGetRemainder(uint8_t *resultBuf, uint32_t *length, uint32_t resultQuotientMemAddr)
{
    return PKAGetBigNumResultRemainder(resultBuf, length, resultQuotientMemAddr);
}


//*****************************************************************************
//
// Start the comparison of two big numbers.
//
//*****************************************************************************
uint32_t PKABigNumCmpStart(const uint8_t *bigNum1, const uint8_t *bigNum2, uint32_t length)
{
    uint32_t offset = 0;

    // Check the arguments.
    ASSERT(bigNum1);
    ASSERT(bigNum2);

    // Make sure no operation is in progress.
    if (HWREG(PKA_BASE + PKA_O_FUNCTION) & PKA_FUNCTION_RUN) {
        return PKA_STATUS_OPERATION_BUSY;
    }

    offset = PKAWritePkaParam(bigNum1, length, offset, PKA_O_APTR);

    offset = PKAWritePkaParam(bigNum2, length, offset, PKA_O_BPTR);

    // Set the PKA Function register for the Compare operation
    // and start the operation.
    HWREG(PKA_BASE + PKA_O_FUNCTION) = (PKA_FUNCTION_RUN | PKA_FUNCTION_COMPARE);

    return PKA_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Get the result of the comparison operation of two big numbers.
//
//*****************************************************************************
uint32_t PKABigNumCmpGetResult(void)
{
    uint32_t  status;

    // verify that the operation is complete.
    if (HWREG(PKA_BASE + PKA_O_FUNCTION) & PKA_FUNCTION_RUN) {
        return PKA_STATUS_OPERATION_BUSY;
    }

    // Check the COMPARE register.
    switch(HWREG(PKA_BASE + PKA_O_COMPARE)) {
        case PKA_COMPARE_A_EQUALS_B:
            status = PKA_STATUS_EQUAL;
            break;

        case PKA_COMPARE_A_GREATER_THAN_B:
            status = PKA_STATUS_A_GREATER_THAN_B;
            break;

        case PKA_COMPARE_A_LESS_THAN_B:
            status = PKA_STATUS_A_LESS_THAN_B;
            break;

        default:
            status = PKA_STATUS_FAILURE;
            break;
    }

    return status;
}

//*****************************************************************************
//
// Start the big number inverse modulo operation.
//
//*****************************************************************************
uint32_t PKABigNumInvModStart(const uint8_t *bigNum, uint32_t bigNumLength, const uint8_t *modulus, uint32_t modulusLength, uint32_t *resultPKAMemAddr)
{
    uint32_t offset = 0;

    // Check the arguments.
    ASSERT(bigNum);
    ASSERT(modulus);
    ASSERT(resultPKAMemAddr);

    // Make sure no operation is in progress.
    if (HWREG(PKA_BASE + PKA_O_FUNCTION) & PKA_FUNCTION_RUN) {
        return PKA_STATUS_OPERATION_BUSY;
    }

    offset = PKAWritePkaParam(bigNum, bigNumLength, offset, PKA_O_APTR);

    offset = PKAWritePkaParam(modulus, modulusLength, offset, PKA_O_BPTR);

    // Copy the result vector address location.
    *resultPKAMemAddr = PKA_RAM_BASE + offset;

    // Load D pointer with the result location in PKA RAM.
    HWREG(PKA_BASE + PKA_O_DPTR) = offset >> 2;

    // set the PKA function to InvMod operation and the start the operation.
    HWREG(PKA_BASE + PKA_O_FUNCTION) = 0x0000F000;

    return PKA_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Get the result of the big number inverse modulo operation.
//
//*****************************************************************************
uint32_t PKABigNumInvModGetResult(uint8_t *resultBuf, uint32_t length, uint32_t resultPKAMemAddr)
{
    // Zero-out array in case modulo result is shorter than length
    PKAZeroOutArray(resultBuf, length);

    return PKAGetBigNumResult(resultBuf, &length, resultPKAMemAddr);
}

//*****************************************************************************
//
// Start the big number multiplication.
//
//*****************************************************************************
uint32_t PKABigNumMultiplyStart(const uint8_t *multiplicand, uint32_t multiplicandLength, const uint8_t *multiplier, uint32_t multiplierLength, uint32_t *resultPKAMemAddr)
{
    uint32_t offset = 0;

    // Check for the arguments.
    ASSERT(multiplicand);
    ASSERT(multiplier);
    ASSERT(resultPKAMemAddr);

    // Make sure no operation is in progress.
    if (HWREG(PKA_BASE + PKA_O_FUNCTION) & PKA_FUNCTION_RUN) {
        return PKA_STATUS_OPERATION_BUSY;
    }

    offset = PKAWritePkaParam(multiplicand, multiplicandLength, offset, PKA_O_APTR);

    offset = PKAWritePkaParam(multiplier, multiplierLength, offset, PKA_O_BPTR);


    // Copy the result vector address location.
    *resultPKAMemAddr = PKA_RAM_BASE + offset;

    // Load C pointer with the result location in PKA RAM.
    HWREG(PKA_BASE + PKA_O_CPTR) = offset >> 2;

    // Set the PKA function to the multiplication and start it.
    HWREG(PKA_BASE + PKA_O_FUNCTION) = (PKA_FUNCTION_RUN | PKA_FUNCTION_MULTIPLY);

    return PKA_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Get the results of the big number multiplication.
//
//*****************************************************************************
uint32_t PKABigNumMultGetResult(uint8_t *resultBuf, uint32_t *resultLength, uint32_t resultPKAMemAddr)
{
     return PKAGetBigNumResult(resultBuf, resultLength, resultPKAMemAddr);
}

//*****************************************************************************
//
// Start the addition of two big number.
//
//*****************************************************************************
uint32_t PKABigNumAddStart(const uint8_t *bigNum1, uint32_t bigNum1Length, const uint8_t *bigNum2, uint32_t bigNum2Length, uint32_t *resultPKAMemAddr)
{
    uint32_t offset = 0;

    // Check for arguments.
    ASSERT(bigNum1);
    ASSERT(bigNum2);
    ASSERT(resultPKAMemAddr);

    // Make sure no operation is in progress.
    if (HWREG(PKA_BASE + PKA_O_FUNCTION) & PKA_FUNCTION_RUN) {
        return PKA_STATUS_OPERATION_BUSY;
    }

    offset = PKAWritePkaParam(bigNum1, bigNum1Length, offset, PKA_O_APTR);

    offset = PKAWritePkaParam(bigNum2, bigNum2Length, offset, PKA_O_BPTR);

    // Copy the result vector address location.
    *resultPKAMemAddr = PKA_RAM_BASE + offset;

    // Load C pointer with the result location in PKA RAM.
    HWREG(PKA_BASE + PKA_O_CPTR) = offset >> 2;

    // Set the function for the add operation and start the operation.
    HWREG(PKA_BASE + PKA_O_FUNCTION) = (PKA_FUNCTION_RUN | PKA_FUNCTION_ADD);

    return PKA_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Get the result of the addition operation on two big number.
//
//*****************************************************************************
uint32_t PKABigNumSubGetResult(uint8_t *resultBuf, uint32_t *resultLength, uint32_t resultPKAMemAddr)
{
    return PKAGetBigNumResult(resultBuf, resultLength, resultPKAMemAddr);
}

//*****************************************************************************
//
// Start the addition of two big number.
//
//*****************************************************************************
uint32_t PKABigNumSubStart(const uint8_t *minuend, uint32_t minuendLength, const uint8_t *subtrahend, uint32_t subtrahendLength, uint32_t *resultPKAMemAddr)
{
    uint32_t offset = 0;

    // Check for arguments.
    ASSERT(minuend);
    ASSERT(subtrahend);
    ASSERT(resultPKAMemAddr);


    // Make sure no operation is in progress.
    if (HWREG(PKA_BASE + PKA_O_FUNCTION) & PKA_FUNCTION_RUN) {
        return PKA_STATUS_OPERATION_BUSY;
    }

    offset = PKAWritePkaParam(minuend, minuendLength, offset, PKA_O_APTR);

    offset = PKAWritePkaParam(subtrahend, subtrahendLength, offset, PKA_O_BPTR);

    // Copy the result vector address location.
    *resultPKAMemAddr = PKA_RAM_BASE + offset;

    // Load C pointer with the result location in PKA RAM.
    HWREG(PKA_BASE + PKA_O_CPTR) = offset >> 2;

    // Set the function for the add operation and start the operation.
    HWREG(PKA_BASE + PKA_O_FUNCTION) = (PKA_FUNCTION_RUN | PKA_FUNCTION_SUBTRACT);

    return PKA_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Get the result of the addition operation on two big number.
//
//*****************************************************************************
uint32_t PKABigNumAddGetResult(uint8_t *resultBuf, uint32_t *resultLength, uint32_t resultPKAMemAddr)
{
    return PKAGetBigNumResult(resultBuf, resultLength, resultPKAMemAddr);
}


//*****************************************************************************
//
// Start ECC Multiplication.
//
//*****************************************************************************
uint32_t PKAEccMultiplyStart(const uint8_t *scalar, const uint8_t *curvePointX, const uint8_t *curvePointY, const uint8_t *prime, const uint8_t *a, const uint8_t *b, uint32_t length, uint32_t *resultPKAMemAddr)
{
    uint32_t offset = 0;

    // Check for the arguments.
    ASSERT(scalar);
    ASSERT(curvePointX);
    ASSERT(curvePointY);
    ASSERT(prime);
    ASSERT(a);
    ASSERT(b);
    ASSERT(length <= PKA_MAX_CURVE_SIZE_32_BIT_WORD * sizeof(uint32_t));
    ASSERT(resultPKAMemAddr);

    // Make sure no PKA operation is in progress.
    if (HWREG(PKA_BASE + PKA_O_FUNCTION) & PKA_FUNCTION_RUN) {
        return PKA_STATUS_OPERATION_BUSY;
    }

    offset = PKAWritePkaParam(scalar, length, offset, PKA_O_APTR);

    offset = PKAWritePkaParamExtraOffset(prime, length, offset, PKA_O_BPTR);
    offset = PKAWritePkaParamExtraOffset(a, length, offset, PKA_NO_POINTER_REG);
    offset = PKAWritePkaParamExtraOffset(b, length, offset, PKA_NO_POINTER_REG);

    offset = PKAWritePkaParamExtraOffset(curvePointX, length, offset, PKA_O_CPTR);
    offset = PKAWritePkaParamExtraOffset(curvePointY, length, offset, PKA_NO_POINTER_REG);

    // Update the result location.
    // The resultPKAMemAddr may be 0 if we only want to check that we generated the point at infinity
    if (resultPKAMemAddr) {
        *resultPKAMemAddr =  PKA_RAM_BASE + offset;
    }

    // Load D pointer with the result location in PKA RAM.
    HWREG(PKA_BASE + PKA_O_DPTR) = offset >> 2;

    // Set the PKA function to ECC-MULT and start the operation.
    HWREG(PKA_BASE + PKA_O_FUNCTION) = PKA_FUNCTION_RUN_M | (0x05 << PKA_FUNCTION_SEQUENCER_OPERATIONS_S);

    return PKA_STATUS_SUCCESS;
}


//*****************************************************************************
//
// Start ECC Montgomery Multiplication.
//
//*****************************************************************************
uint32_t PKAEccMontgomeryMultiplyStart(const uint8_t *scalar, const uint8_t *curvePointX, const uint8_t *prime, const uint8_t *a, uint32_t length, uint32_t *resultPKAMemAddr)
{
    uint32_t offset = 0;

    // Check for the arguments.
    ASSERT(scalar);
    ASSERT(curvePointX);
    ASSERT(prime);
    ASSERT(a);
    ASSERT(length <= PKA_MAX_CURVE_SIZE_32_BIT_WORD * sizeof(uint32_t));
    ASSERT(resultPKAMemAddr);

    // Make sure no PKA operation is in progress.
    if (HWREG(PKA_BASE + PKA_O_FUNCTION) & PKA_FUNCTION_RUN) {
        return PKA_STATUS_OPERATION_BUSY;
    }

    offset = PKAWritePkaParam(scalar, length, offset, PKA_O_APTR);

    offset = PKAWritePkaParamExtraOffset(prime, length, offset, PKA_O_BPTR);
    offset = PKAWritePkaParamExtraOffset(a, length, offset, PKA_NO_POINTER_REG);

    offset = PKAWritePkaParamExtraOffset(curvePointX, length, offset, PKA_O_CPTR);

    // Update the result location.
    // The resultPKAMemAddr may be 0 if we only want to check that we generated the point at infinity
    if (resultPKAMemAddr) {
        *resultPKAMemAddr =  PKA_RAM_BASE + offset;
    }

    // Load D pointer with the result location in PKA RAM.
    HWREG(PKA_BASE + PKA_O_DPTR) = offset >> 2;

    // Set the PKA function to Montgomery ECC-MULT and start the operation.
    HWREG(PKA_BASE + PKA_O_FUNCTION) = PKA_FUNCTION_RUN_M | (0x02 << PKA_FUNCTION_SEQUENCER_OPERATIONS_S);

    return PKA_STATUS_SUCCESS;
}


//*****************************************************************************
//
// Get the result of ECC Multiplication
//
//*****************************************************************************
uint32_t PKAEccMultiplyGetResult(uint8_t *curvePointX, uint8_t *curvePointY, uint32_t resultPKAMemAddr, uint32_t length)
{
    return PKAGetECCResult(curvePointX, curvePointY, resultPKAMemAddr, length);
}

//*****************************************************************************
//
// Start the ECC Addition.
//
//*****************************************************************************
uint32_t PKAEccAddStart(const uint8_t *curvePoint1X, const uint8_t *curvePoint1Y, const uint8_t *curvePoint2X, const uint8_t *curvePoint2Y, const uint8_t *prime, const uint8_t *a, uint32_t length, uint32_t *resultPKAMemAddr)
{
    uint32_t offset = 0;

    // Check for the arguments.
    ASSERT(curvePoint1X);
    ASSERT(curvePoint1Y);
    ASSERT(curvePoint2X);
    ASSERT(curvePoint2Y);
    ASSERT(prime);
    ASSERT(a);
    ASSERT(resultPKAMemAddr);

    // Make sure no operation is in progress.
    if (HWREG(PKA_BASE + PKA_O_FUNCTION) & PKA_FUNCTION_RUN) {
        return PKA_STATUS_OPERATION_BUSY;
    }

    offset = PKAWritePkaParamExtraOffset(curvePoint1X, length, offset, PKA_O_APTR);
    offset = PKAWritePkaParamExtraOffset(curvePoint1Y, length, offset, PKA_NO_POINTER_REG);


    offset = PKAWritePkaParamExtraOffset(prime, length, offset, PKA_O_BPTR);
    offset = PKAWritePkaParamExtraOffset(a, length, offset, PKA_NO_POINTER_REG);

    offset = PKAWritePkaParamExtraOffset(curvePoint2X, length, offset, PKA_O_CPTR);
    offset = PKAWritePkaParamExtraOffset(curvePoint2Y, length, offset, PKA_NO_POINTER_REG);

    // Copy the result vector location.
    *resultPKAMemAddr = PKA_RAM_BASE + offset;

    // Load D pointer with the result location in PKA RAM.
    HWREG(PKA_BASE + PKA_O_DPTR) = offset >> 2;

    // Set the PKA Function to ECC-ADD and start the operation.
    HWREG(PKA_BASE + PKA_O_FUNCTION ) = PKA_FUNCTION_RUN_M | (0x03 << PKA_FUNCTION_SEQUENCER_OPERATIONS_S);

    return PKA_STATUS_SUCCESS;
}

//*****************************************************************************
//
// Get the result of the ECC Addition
//
//*****************************************************************************
uint32_t PKAEccAddGetResult(uint8_t *curvePointX, uint8_t *curvePointY, uint32_t resultPKAMemAddr, uint32_t length)
{
    return PKAGetECCResult(curvePointX, curvePointY, resultPKAMemAddr, length);
}

//*****************************************************************************
//
// Verify a public key against the supplied elliptic curve equation
//
//*****************************************************************************
uint32_t PKAEccVerifyPublicKeyWeierstrassStart(const uint8_t *curvePointX, const uint8_t *curvePointY, const uint8_t *prime, const uint8_t *a, const uint8_t *b, const uint8_t *order, uint32_t length)
{
    uint32_t pkaResult;
    uint32_t resultAddress;
    uint32_t resultLength;
    uint8_t *scratchBuffer = (uint8_t *)(PKA_RAM_BASE + PKA_RAM_TOT_BYTE_SIZE / 2);
    uint8_t *scratchBuffer2 = scratchBuffer + 512;


    // Verify X in range [0, prime - 1]
    PKABigNumCmpStart(curvePointX,
                      prime,
                      length);

    while(PKAGetOpsStatus() == PKA_STATUS_OPERATION_BUSY);

    pkaResult = PKABigNumCmpGetResult();

    if (pkaResult != PKA_STATUS_A_LESS_THAN_B) {
        return PKA_STATUS_X_LARGER_THAN_PRIME;
    }

    // Verify Y in range [0, prime - 1]
    PKABigNumCmpStart(curvePointY,
                      prime,
                      length);

    while(PKAGetOpsStatus() == PKA_STATUS_OPERATION_BUSY);

    pkaResult = PKABigNumCmpGetResult();

    if (pkaResult != PKA_STATUS_A_LESS_THAN_B) {
        return PKA_STATUS_Y_LARGER_THAN_PRIME;
    }

    // Verify point on curve
    // Short-Weierstrass equation: Y ^ 2 = X ^3 + a * X + b mod P
    // Reduced: Y ^ 2 = X * (X ^ 2 + a) + b

    // tmp = X ^ 2
    PKABigNumMultiplyStart(curvePointX, length, curvePointX, length, &resultAddress);

    while(PKAGetOpsStatus() == PKA_STATUS_OPERATION_BUSY);

    resultLength = 200;
    pkaResult = PKABigNumMultGetResult(scratchBuffer, &resultLength, resultAddress);

    if (pkaResult != PKA_STATUS_SUCCESS) {
        return PKA_STATUS_FAILURE;
    }

    // tmp += a
    PKABigNumAddStart(scratchBuffer, resultLength, a, length, &resultAddress);

    while(PKAGetOpsStatus() == PKA_STATUS_OPERATION_BUSY);

    resultLength = 200;
    pkaResult = PKABigNumAddGetResult(scratchBuffer, &resultLength, resultAddress);

    if (pkaResult != PKA_STATUS_SUCCESS) {
        return PKA_STATUS_FAILURE;
    }

    // tmp *= x
    PKABigNumMultiplyStart(scratchBuffer, resultLength, curvePointX, length, &resultAddress);

    while(PKAGetOpsStatus() == PKA_STATUS_OPERATION_BUSY);

    resultLength = 200;
    pkaResult = PKABigNumMultGetResult(scratchBuffer, &resultLength, resultAddress);

    if (pkaResult != PKA_STATUS_SUCCESS) {
        return PKA_STATUS_FAILURE;
    }

    // tmp += b
     PKABigNumAddStart(scratchBuffer, resultLength, b, length, &resultAddress);

    while(PKAGetOpsStatus() == PKA_STATUS_OPERATION_BUSY);

    resultLength = 200;
    pkaResult = PKABigNumAddGetResult(scratchBuffer, &resultLength, resultAddress);

    if (pkaResult != PKA_STATUS_SUCCESS) {
        return PKA_STATUS_FAILURE;
    }


    // tmp2 = tmp % prime to ensure we have no fraction in the division.
    // The number will only shrink from here on out.
    PKABigNumModStart(scratchBuffer, resultLength, prime, length, &resultAddress);

    while(PKAGetOpsStatus() == PKA_STATUS_OPERATION_BUSY);

    // If the result is not a multiple of the word-length, the PKA HW will round up
    // because it deals in words only. That means that using 'length' directly
    // would cause and underflow, since length refers to the actual length in bytes of
    // the curve parameters while the PKA HW reports that rounded up to the next
    // word boundary.
    // Use 200 as the resultLength instead since we are copying to the scratch buffer
    // anyway.
    // Practically, this only happens with curves such as NIST-P521 that are not word
    // multiples.
    resultLength = 200;
    pkaResult = PKABigNumModGetResult(scratchBuffer2, resultLength, resultAddress);

    if (pkaResult != PKA_STATUS_SUCCESS) {
        return PKA_STATUS_FAILURE;
    }

    // tmp = y^2
    PKABigNumMultiplyStart(curvePointY, length, curvePointY, length, &resultAddress);

    while(PKAGetOpsStatus() == PKA_STATUS_OPERATION_BUSY);

    resultLength = 200;
    pkaResult = PKABigNumMultGetResult(scratchBuffer, &resultLength, resultAddress);

    if (pkaResult != PKA_STATUS_SUCCESS) {
        return PKA_STATUS_FAILURE;
    }

    // tmp %= prime
    PKABigNumModStart(scratchBuffer, resultLength, prime, length, &resultAddress);

    while(PKAGetOpsStatus() == PKA_STATUS_OPERATION_BUSY);

    // If the result is not a multiple of the word-length, the PKA HW will round up
    // because it deals in words only. That means that using 'length' directly
    // would cause and underflow, since length refers to the actual length in bytes of
    // the curve parameters while the PKA HW reports that rounded up to the next
    // word boundary.
    // Use 200 as the resultLength instead since we are copying to the scratch buffer
    // anyway.
    // Practically, this only happens with curves such as NIST-P521 that are not word
    // multiples.
    resultLength = 200;
    pkaResult = PKABigNumModGetResult(scratchBuffer, resultLength, resultAddress);

    if (pkaResult != PKA_STATUS_SUCCESS) {
        return PKA_STATUS_FAILURE;
    }

    // tmp ?= tmp2
    PKABigNumCmpStart(scratchBuffer,
                      scratchBuffer2,
                      length);

    while(PKAGetOpsStatus() == PKA_STATUS_OPERATION_BUSY);

    pkaResult = PKABigNumCmpGetResult();

    if (pkaResult != PKA_STATUS_EQUAL) {
        return PKA_STATUS_POINT_NOT_ON_CURVE;
    }
    else {
        return PKA_STATUS_SUCCESS;
    }
}
