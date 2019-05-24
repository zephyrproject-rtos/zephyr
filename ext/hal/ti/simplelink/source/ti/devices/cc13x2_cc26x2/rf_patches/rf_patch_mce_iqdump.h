/******************************************************************************
*  Filename:       rf_patch_mce_iqdump.h
*  Revised:        $Date: 2019-01-31 15:04:25 +0100 (to, 31 jan 2019) $
*  Revision:       $Revision: 18842 $
*
*  Description: RF core patch for IQ-dump support in CC13x2 PG2.1 and CC26x2 PG2.1
*
*  Copyright (c) 2015-2019, Texas Instruments Incorporated
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

#ifndef _RF_PATCH_MCE_IQDUMP_H
#define _RF_PATCH_MCE_IQDUMP_H

#include <stdint.h>
#include "../inc/hw_types.h"

#ifndef MCE_PATCH_TYPE
#define MCE_PATCH_TYPE static const uint32_t
#endif

#ifndef PATCH_FUN_SPEC
#define PATCH_FUN_SPEC static inline
#endif

#ifndef RFC_MCERAM_BASE
#define RFC_MCERAM_BASE 0x21008000
#endif

#ifndef MCE_PATCH_MODE
#define MCE_PATCH_MODE 0
#endif

MCE_PATCH_TYPE patchIqdumpMce[337] = { 
   0x2fcf6030,
   0x00013f9d,
   0xff00003f,
   0x07ff0fff,
   0x0300f800,
   0x00068080,
   0x00170003,
   0x00003d1f,
   0x08000000,
   0x0000000f,
   0x00000387,
   0x00434074,
   0x00828000,
   0x06f00080,
   0x091e0000,
   0x00540510,
   0x00000007,
   0x00505014,
   0xc02f0000,
   0x017f0c30,
   0x00000000,
   0x00000000,
   0x00000000,
   0x0000aa00,
   0x66957223,
   0xa4e5a35d,
   0x73057303,
   0x73047203,
   0x72047306,
   0x72917391,
   0xffc0b008,
   0xa0089010,
   0x720e720d,
   0x7210720f,
   0x7100b0d0,
   0xa0d0b110,
   0x8162721b,
   0x39521020,
   0x00200670,
   0x11011630,
   0x6c011401,
   0x60816080,
   0x610b60fd,
   0x60806080,
   0x60806080,
   0x60816080,
   0x61af60fd,
   0x60806080,
   0x60806080,
   0x60816080,
   0x611b60fd,
   0x60806080,
   0x60806080,
   0x60816080,
   0x61cb60fd,
   0x60806080,
   0x60806080,
   0x60816080,
   0x615360fd,
   0x72231210,
   0x73127311,
   0x81b17313,
   0x91b00010,
   0x6044b070,
   0xc0306076,
   0xc0c1669b,
   0xc4e0c2b2,
   0x6f131820,
   0x16116e23,
   0x68871612,
   0x99c07830,
   0x948078a0,
   0xc4f29490,
   0x1820c750,
   0x12034099,
   0x16126e23,
   0x78b06896,
   0x72639990,
   0x6076b63c,
   0x96408190,
   0x39808170,
   0x10012a70,
   0x84a21611,
   0xc0f384b4,
   0xc200c0f5,
   0x40c21c01,
   0x1c10c100,
   0x4cba40b8,
   0x18031013,
   0x1a131830,
   0x39121a10,
   0x60c268b5,
   0x60c213f3,
   0x101513f3,
   0x1850c100,
   0x1a101a15,
   0x68c03914,
   0x7100b0e8,
   0xa0e8b128,
   0xb910b230,
   0x99308990,
   0xb0d1b111,
   0xb0027100,
   0xb111b012,
   0x7291a0d1,
   0xb003b630,
   0x722cb013,
   0x7100b0e0,
   0x8170b120,
   0x710092c0,
   0x8170b120,
   0x44db22f0,
   0x1c0313f0,
   0x92c340e7,
   0x71009642,
   0x92c5b120,
   0x71009644,
   0xb0e0b120,
   0x7000a630,
   0xc030a0e1,
   0xc0409910,
   0xb1119930,
   0x7100b0d1,
   0xa0d1b111,
   0xa0037291,
   0xa230a002,
   0x73117000,
   0xc0407312,
   0xc100669b,
   0x649e91f0,
   0xb113b633,
   0x7100b0d3,
   0x64eea0d3,
   0xa0d26076,
   0xa0f3a0f0,
   0x73127311,
   0xc050660f,
   0xb0d2669b,
   0x7100c035,
   0xba389b75,
   0xb112b074,
   0xa0d26115,
   0xa0f3a0f0,
   0x73127311,
   0xc18b660f,
   0x91e0c000,
   0x1218120c,
   0x787d786a,
   0x10a9788e,
   0xb0d2b074,
   0xb112c020,
   0x692d7100,
   0x669bc060,
   0xb112c035,
   0x9b757100,
   0x65a48bf0,
   0x22018ca1,
   0x10804140,
   0x453f1ca8,
   0x16181208,
   0x8c00659b,
   0x8ca165a4,
   0x414b2201,
   0x1a191090,
   0x454b1e09,
   0x659b10a9,
   0x1e048184,
   0x14bc4133,
   0x4e7e1c4c,
   0xa0d26133,
   0xa0f3a0f0,
   0x73127311,
   0x721e660f,
   0x1205120c,
   0xb0d2b074,
   0xb112c020,
   0x695f7100,
   0x669bc070,
   0x89ce789d,
   0x7100b112,
   0x22008c90,
   0x8230416f,
   0x456f2210,
   0x9a3db231,
   0x31828ab2,
   0x8af03d82,
   0x3d803180,
   0x063e1802,
   0x41911e0e,
   0x41831e2e,
   0x418a1e3e,
   0x14261056,
   0x10653d16,
   0x10566192,
   0x18563126,
   0x3d261426,
   0x61921065,
   0x31361056,
   0x14261856,
   0x10653d36,
   0x10266192,
   0x91c63976,
   0x1e048184,
   0x161c4166,
   0x4e7e1c4c,
   0x10016166,
   0x91c1c0b0,
   0x10003911,
   0x10001000,
   0x7000699d,
   0x3d303130,
   0x4dab1cd0,
   0x49ad1ce0,
   0x10d07000,
   0x10e07000,
   0xc0807000,
   0xa0d2669b,
   0xa0f3a0f0,
   0x73127311,
   0xb130660f,
   0x7100b0f0,
   0x220080b0,
   0x61b945be,
   0xc090b231,
   0xb130669b,
   0xb0d2a0f0,
   0x7100c035,
   0xba389b75,
   0xb112b074,
   0xc0a061c5,
   0xa0d2669b,
   0xa0f3a0f0,
   0x73127311,
   0xc18b660f,
   0x91e0c000,
   0x1218120c,
   0x787d786a,
   0x10a9788e,
   0xb0f0b130,
   0x80b07100,
   0x45e32200,
   0xb07461de,
   0xc0b0b231,
   0xb130669b,
   0xb0d2a0f0,
   0xb112c020,
   0x69eb7100,
   0xb112c035,
   0x9b757100,
   0x65a48bf0,
   0x22018ca1,
   0x108041fc,
   0x45fb1ca8,
   0x16181208,
   0x8c00659b,
   0x8ca165a4,
   0x42072201,
   0x1a191090,
   0x46071e09,
   0x659b10a9,
   0x1e048184,
   0x14bc41ef,
   0x4e7e1c4c,
   0x824061ef,
   0x46172230,
   0x7100b0d5,
   0xa0d5b115,
   0xc0c0620f,
   0xb118669b,
   0xb016b006,
   0xb014b004,
   0xb012b002,
   0x78428440,
   0x81730420,
   0x2a733983,
   0xc1f294e3,
   0x31621832,
   0x31511021,
   0x00200012,
   0x10309440,
   0x39301610,
   0x42352210,
   0x31501220,
   0x31801003,
   0x93801630,
   0x12041202,
   0x42472273,
   0x997084a0,
   0x1a828982,
   0x997084c0,
   0x1a848984,
   0x22636249,
   0x84b04254,
   0x89809970,
   0x14021a80,
   0x997084d0,
   0x1a808980,
   0x62601404,
   0x785184b0,
   0x99700410,
   0x1a428982,
   0x785184d0,
   0x99700410,
   0x1a448984,
   0x31543152,
   0x06333963,
   0x38321613,
   0x31823834,
   0x31843982,
   0x97220042,
   0x959084a0,
   0x95a084b0,
   0x95b084c0,
   0x95c084d0,
   0x90307810,
   0x78209050,
   0x90609040,
   0xcd90b235,
   0x70009170,
   0xb112a235,
   0xa0d27100,
   0xba3cb112,
   0x8b5481b0,
   0x31843924,
   0x91b40004,
   0x669bc0d0,
   0x72917391,
   0x72066695,
   0x72047202,
   0x73067305,
   0x86306076,
   0x3151c801,
   0x96300410,
   0x9a007000,
   0x220089f0,
   0xb9e0469c,
   0x00007000
};

PATCH_FUN_SPEC void rf_patch_mce_iqdump(void)
{
#ifdef __PATCH_NO_UNROLLING
   uint32_t i;
   for (i = 0; i < 337; i++) {
      HWREG(RFC_MCERAM_BASE + 4 * i) = patchIqdumpMce[i];
   }
#else
   const uint32_t *pS = patchIqdumpMce;
   volatile unsigned long *pD = &HWREG(RFC_MCERAM_BASE);
   uint32_t t1, t2, t3, t4, t5, t6, t7, t8;
   uint32_t nIterations = 42;

   do {
      t1 = *pS++;
      t2 = *pS++;
      t3 = *pS++;
      t4 = *pS++;
      t5 = *pS++;
      t6 = *pS++;
      t7 = *pS++;
      t8 = *pS++;
      *pD++ = t1;
      *pD++ = t2;
      *pD++ = t3;
      *pD++ = t4;
      *pD++ = t5;
      *pD++ = t6;
      *pD++ = t7;
      *pD++ = t8;
   } while (--nIterations);

   t1 = *pS++;
   *pD++ = t1;
#endif
}

#endif
