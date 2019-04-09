/******************************************************************************
*  Filename:       rf_patch_cpe_bt5.h
*  Revised:        $Date: 2019-02-27 16:13:01 +0100 (on, 27 feb 2019) $
*  Revision:       $Revision: 18889 $
*
*  Description: RF core patch for Bluetooth 5 support ("BLE" and "BLE5" API command sets) in CC13x2 and CC26x2
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
#ifndef _RF_PATCH_CPE_BT5_H
#define _RF_PATCH_CPE_BT5_H

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <string.h>

#ifndef CPE_PATCH_TYPE
#define CPE_PATCH_TYPE static const uint32_t
#endif

#ifndef SYS_PATCH_TYPE
#define SYS_PATCH_TYPE static const uint32_t
#endif

#ifndef PATCH_FUN_SPEC
#define PATCH_FUN_SPEC static inline
#endif

#ifndef _APPLY_PATCH_TAB
#define _APPLY_PATCH_TAB
#endif


CPE_PATCH_TYPE patchImageBt5[] = {
   0x21004059,
   0x210040a5,
   0x21004085,
   0x79654c07,
   0xf809f000,
   0x40697961,
   0xd5030749,
   0x4a042101,
   0x60110389,
   0xb570bd70,
   0x47084902,
   0x21000380,
   0x40041108,
   0x0000592d,
   0x21014805,
   0x438a6802,
   0x6b836002,
   0x6383438b,
   0x6002430a,
   0x47004801,
   0x40046000,
   0x00005b3f,
   0x490cb510,
   0x4a0c4788,
   0x5e512106,
   0xd0072900,
   0xd0052902,
   0xd0032909,
   0xd0012910,
   0xd1072911,
   0x43c92177,
   0xdd014288,
   0xdd012800,
   0x43c0207f,
   0x0000bd10,
   0x000065a9,
   0x21000380,
};
#define _NWORD_PATCHIMAGE_BT5 37

#define _NWORD_PATCHCPEHD_BT5 0

#define _NWORD_PATCHSYS_BT5 0



#ifndef _BT5_SYSRAM_START
#define _BT5_SYSRAM_START 0x20000000
#endif

#ifndef _BT5_CPERAM_START
#define _BT5_CPERAM_START 0x21000000
#endif

#define _BT5_SYS_PATCH_FIXED_ADDR 0x20000000

#define _BT5_PATCH_VEC_ADDR_OFFSET 0x03D0
#define _BT5_PATCH_TAB_OFFSET 0x03D4
#define _BT5_IRQPATCH_OFFSET 0x0480
#define _BT5_PATCH_VEC_OFFSET 0x404C

#define _BT5_PATCH_CPEHD_OFFSET 0x04E0

#ifndef _BT5_NO_PROG_STATE_VAR
static uint8_t bBt5PatchEntered = 0;
#endif

PATCH_FUN_SPEC void enterBt5CpePatch(void)
{
#if (_NWORD_PATCHIMAGE_BT5 > 0)
   uint32_t *pPatchVec = (uint32_t *) (_BT5_CPERAM_START + _BT5_PATCH_VEC_OFFSET);

   memcpy(pPatchVec, patchImageBt5, sizeof(patchImageBt5));
#endif
}

PATCH_FUN_SPEC void enterBt5CpeHdPatch(void)
{
#if (_NWORD_PATCHCPEHD_BT5 > 0)
   uint32_t *pPatchCpeHd = (uint32_t *) (_BT5_CPERAM_START + _BT5_PATCH_CPEHD_OFFSET);

   memcpy(pPatchCpeHd, patchCpeHd, sizeof(patchCpeHd));
#endif
}

PATCH_FUN_SPEC void enterBt5SysPatch(void)
{
}

PATCH_FUN_SPEC void configureBt5Patch(void)
{
   uint8_t *pPatchTab = (uint8_t *) (_BT5_CPERAM_START + _BT5_PATCH_TAB_OFFSET);


   pPatchTab[76] = 0;
   pPatchTab[91] = 1;
   pPatchTab[79] = 2;
}

PATCH_FUN_SPEC void applyBt5Patch(void)
{
#ifdef _BT5_NO_PROG_STATE_VAR
   enterBt5SysPatch();
   enterBt5CpePatch();
#else
   if (!bBt5PatchEntered)
   {
      enterBt5SysPatch();
      enterBt5CpePatch();
      bBt5PatchEntered = 1;
   }
#endif
   enterBt5CpeHdPatch();
   configureBt5Patch();
}

PATCH_FUN_SPEC void refreshBt5Patch(void)
{
   enterBt5CpeHdPatch();
   configureBt5Patch();
}

#ifndef _BT5_NO_PROG_STATE_VAR
PATCH_FUN_SPEC void cleanBt5Patch(void)
{
   bBt5PatchEntered = 0;
}
#endif

PATCH_FUN_SPEC void rf_patch_cpe_bt5(void)
{
   applyBt5Patch();
}


//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  _RF_PATCH_CPE_BT5_H

