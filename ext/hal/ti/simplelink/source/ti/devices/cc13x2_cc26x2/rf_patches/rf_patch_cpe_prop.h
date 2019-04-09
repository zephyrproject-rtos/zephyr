/******************************************************************************
*  Filename:       rf_patch_cpe_prop.h
*  Revised:        $Date: 2019-02-27 16:13:01 +0100 (on, 27 feb 2019) $
*  Revision:       $Revision: 18889 $
*
*  Description: RF core patch for proprietary radio support ("PROP" API command set) in CC13x2 and CC26x2
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
#ifndef _RF_PATCH_CPE_PROP_H
#define _RF_PATCH_CPE_PROP_H

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


CPE_PATCH_TYPE patchImageProp[] = {
   0x21004059,
   0x210040c3,
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
   0xf819f000,
   0x296cb2e1,
   0x2804d00b,
   0x2806d001,
   0x490ed107,
   0x07c97809,
   0x7821d103,
   0xd4000709,
   0x490b2002,
   0x210c780a,
   0xd0024211,
   0x22804909,
   0xb003600a,
   0xb5f0bdf0,
   0x4907b083,
   0x48044708,
   0x22407801,
   0x70014391,
   0x47004804,
   0x210000c8,
   0x21000133,
   0xe000e200,
   0x00031641,
   0x00031b23,
};
#define _NWORD_PATCHIMAGE_PROP 38

#define _NWORD_PATCHCPEHD_PROP 0

#define _NWORD_PATCHSYS_PROP 0



#ifndef _PROP_SYSRAM_START
#define _PROP_SYSRAM_START 0x20000000
#endif

#ifndef _PROP_CPERAM_START
#define _PROP_CPERAM_START 0x21000000
#endif

#define _PROP_SYS_PATCH_FIXED_ADDR 0x20000000

#define _PROP_PATCH_VEC_ADDR_OFFSET 0x03D0
#define _PROP_PATCH_TAB_OFFSET 0x03D4
#define _PROP_IRQPATCH_OFFSET 0x0480
#define _PROP_PATCH_VEC_OFFSET 0x404C

#define _PROP_PATCH_CPEHD_OFFSET 0x04E0

#ifndef _PROP_NO_PROG_STATE_VAR
static uint8_t bPropPatchEntered = 0;
#endif

PATCH_FUN_SPEC void enterPropCpePatch(void)
{
#if (_NWORD_PATCHIMAGE_PROP > 0)
   uint32_t *pPatchVec = (uint32_t *) (_PROP_CPERAM_START + _PROP_PATCH_VEC_OFFSET);

   memcpy(pPatchVec, patchImageProp, sizeof(patchImageProp));
#endif
}

PATCH_FUN_SPEC void enterPropCpeHdPatch(void)
{
#if (_NWORD_PATCHCPEHD_PROP > 0)
   uint32_t *pPatchCpeHd = (uint32_t *) (_PROP_CPERAM_START + _PROP_PATCH_CPEHD_OFFSET);

   memcpy(pPatchCpeHd, patchCpeHd, sizeof(patchCpeHd));
#endif
}

PATCH_FUN_SPEC void enterPropSysPatch(void)
{
}

PATCH_FUN_SPEC void configurePropPatch(void)
{
   uint8_t *pPatchTab = (uint8_t *) (_PROP_CPERAM_START + _PROP_PATCH_TAB_OFFSET);


   pPatchTab[76] = 0;
   pPatchTab[62] = 1;
   pPatchTab[64] = 2;
}

PATCH_FUN_SPEC void applyPropPatch(void)
{
#ifdef _PROP_NO_PROG_STATE_VAR
   enterPropSysPatch();
   enterPropCpePatch();
#else
   if (!bPropPatchEntered)
   {
      enterPropSysPatch();
      enterPropCpePatch();
      bPropPatchEntered = 1;
   }
#endif
   enterPropCpeHdPatch();
   configurePropPatch();
}

PATCH_FUN_SPEC void refreshPropPatch(void)
{
   enterPropCpeHdPatch();
   configurePropPatch();
}

#ifndef _PROP_NO_PROG_STATE_VAR
PATCH_FUN_SPEC void cleanPropPatch(void)
{
   bPropPatchEntered = 0;
}
#endif

PATCH_FUN_SPEC void rf_patch_cpe_prop(void)
{
   applyPropPatch();
}


//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif //  _RF_PATCH_CPE_PROP_H

