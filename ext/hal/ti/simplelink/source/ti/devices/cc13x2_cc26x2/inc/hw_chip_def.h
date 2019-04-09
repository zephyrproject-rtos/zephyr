/******************************************************************************
*  Filename:       hw_chip_def.h
*  Revised:        2017-06-26 09:33:33 +0200 (Mon, 26 Jun 2017)
*  Revision:       49227
*
*  Description:    Defines for device properties.
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

//*****************************************************************************
//
//! \addtogroup config_api
//! @{
//
//*****************************************************************************

#ifndef __HW_CHIP_DEF_H__
#define __HW_CHIP_DEF_H__

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

//*****************************************************************************
//
// Define CC_CHIP_ID code used in the following macros defined at the bottom:
// CC_GET_CHIP_FAMILY/DEVICE/PACKAGE/HWREV
//
//*****************************************************************************
/* CC2620F128 */
#if   defined(CC2620F128RGZ_R20) || defined(CC2620F128RGZ_R21)
    #define CC_CHIP_ID 0x26200720
#elif defined(CC2620F128RHB_R20) || defined(CC2620F128RHB_R21)
    #define CC_CHIP_ID 0x26200520
#elif defined(CC2620F128RSM_R20) || defined(CC2620F128RSM_R21)
    #define CC_CHIP_ID 0x26200420
#elif defined(CC2620F128_R20)    || defined(CC2620F128_R21)
    #define CC_CHIP_ID 0x26200020
#elif defined(CC2620F128RGZ_R22) || defined(CC2620F128RGZ)
    #define CC_CHIP_ID 0x26200722
#elif defined(CC2620F128RHB_R22) || defined(CC2620F128RHB)
    #define CC_CHIP_ID 0x26200522
#elif defined(CC2620F128RSM_R22) || defined(CC2620F128RSM)
    #define CC_CHIP_ID 0x26200422
#elif defined(CC2620F128_R22)    || defined(CC2620F128)
    #define CC_CHIP_ID 0x26200022
/* CC2630F128 */
#elif defined(CC2630F128RGZ_R20) || defined(CC2630F128RGZ_R21)
    #define CC_CHIP_ID 0x26300720
#elif defined(CC2630F128RHB_R20) || defined(CC2630F128RHB_R21)
    #define CC_CHIP_ID 0x26300520
#elif defined(CC2630F128RSM_R20) || defined(CC2630F128RSM_R21)
    #define CC_CHIP_ID 0x26300420
#elif defined(CC2630F128_R20)    || defined(CC2630F128_R21)
    #define CC_CHIP_ID 0x26300020
#elif defined(CC2630F128RGZ_R22) || defined(CC2630F128RGZ)
    #define CC_CHIP_ID 0x26300722
#elif defined(CC2630F128RHB_R22) || defined(CC2630F128RHB)
    #define CC_CHIP_ID 0x26300522
#elif defined(CC2630F128RSM_R22) || defined(CC2630F128RSM)
    #define CC_CHIP_ID 0x26300422
#elif defined(CC2630F128_R22)    || defined(CC2630F128)
    #define CC_CHIP_ID 0x26300022
/* CC2640F128 */
#elif defined(CC2640F128RGZ_R20) || defined(CC2640F128RGZ_R21)
    #define CC_CHIP_ID 0x26400720
#elif defined(CC2640F128RHB_R20) || defined(CC2640F128RHB_R21)
    #define CC_CHIP_ID 0x26400520
#elif defined(CC2640F128RSM_R20) || defined(CC2640F128RSM_R21)
    #define CC_CHIP_ID 0x26400420
#elif defined(CC2640F128_R20)    || defined(CC2640F128_R21)
    #define CC_CHIP_ID 0x26400020
#elif defined(CC2640F128RGZ_R22) || defined(CC2640F128RGZ)
    #define CC_CHIP_ID 0x26400722
#elif defined(CC2640F128RHB_R22) || defined(CC2640F128RHB)
    #define CC_CHIP_ID 0x26400522
#elif defined(CC2640F128RSM_R22) || defined(CC2640F128RSM)
    #define CC_CHIP_ID 0x26400422
#elif defined(CC2640F128_R22)    || defined(CC2640F128)
    #define CC_CHIP_ID 0x26400022
/* CC2650F128 */
#elif defined(CC2650F128RGZ_R20) || defined(CC2650F128RGZ_R21)
    #define CC_CHIP_ID 0x26500720
#elif defined(CC2650F128RHB_R20) || defined(CC2650F128RHB_R21)
    #define CC_CHIP_ID 0x26500520
#elif defined(CC2650F128RSM_R20) || defined(CC2650F128RSM_R21)
    #define CC_CHIP_ID 0x26500420
#elif defined(CC2650F128_R20)    || defined(CC2650F128_R21)
    #define CC_CHIP_ID 0x26500020
#elif defined(CC2650F128RGZ_R22) || defined(CC2650F128RGZ)
    #define CC_CHIP_ID 0x26500722
#elif defined(CC2650F128RHB_R22) || defined(CC2650F128RHB)
    #define CC_CHIP_ID 0x26500522
#elif defined(CC2650F128RSM_R22) || defined(CC2650F128RSM)
    #define CC_CHIP_ID 0x26500422
#elif defined(CC2650F128_R22)    || defined(CC2650F128)
    #define CC_CHIP_ID 0x26500022
/* CC2650L128 (OTP) */
#elif defined(CC2650L128)
    #define CC_CHIP_ID 0x26501710
/* CC1310F128 */
#elif defined(CC1310F128RGZ_R20) || defined(CC1310F128RGZ)
    #define CC_CHIP_ID 0x13100720
#elif defined(CC1310F128RHB_R20) || defined(CC1310F128RHB)
    #define CC_CHIP_ID 0x13100520
#elif defined(CC1310F128RSM_R20) || defined(CC1310F128RSM)
    #define CC_CHIP_ID 0x13100420
#elif defined(CC1310F128_R20)    || defined(CC1310F128)
    #define CC_CHIP_ID 0x13100020
/* CC1350F128 */
#elif defined(CC1350F128RGZ_R20) || defined(CC1350F128RGZ)
    #define CC_CHIP_ID 0x13500720
#elif defined(CC1350F128RHB_R20) || defined(CC1350F128RHB)
    #define CC_CHIP_ID 0x13500520
#elif defined(CC1350F128RSM_R20) || defined(CC1350F128RSM)
    #define CC_CHIP_ID 0x13500420
#elif defined(CC1350F128_R20)    || defined(CC1350F128)
    #define CC_CHIP_ID 0x13500020
/* CC2640R2F */
#elif defined(CC2640R2FRGZ_R25) || defined(CC2640R2FRGZ)
    #define CC_CHIP_ID 0x26401710
#elif defined(CC2640R2FRHB_R25) || defined(CC2640R2FRHB)
    #define CC_CHIP_ID 0x26401510
#elif defined(CC2640R2FRSM_R25) || defined(CC2640R2FRSM)
    #define CC_CHIP_ID 0x26401410
#elif defined(CC2640R2F_R25)    || defined(CC2640R2F)
    #define CC_CHIP_ID 0x26401010
/* CC2652R1F */
#elif defined(CC2652R1FRGZ_R10) || defined(CC2652R1FRGZ)
    #define CC_CHIP_ID 0x26523710
#elif defined(CC2652R1F_R10)    || defined(CC2652R1F)
    #define CC_CHIP_ID 0x26523010
/* CC2644R1F */
#elif defined(CC2644R1FRGZ_R10) || defined(CC2644R1FRGZ)
    #define CC_CHIP_ID 0x26443710
#elif defined(CC2644R1F_R10)    || defined(CC2644R1F)
    #define CC_CHIP_ID 0x26443010
/* CC2642R1F */
#elif defined(CC2642R1FRGZ_R10) || defined(CC2642R1FRGZ)
    #define CC_CHIP_ID 0x26423710
#elif defined(CC2642R1F_R10)    || defined(CC2642R1F)
    #define CC_CHIP_ID 0x26423010
/* CC1354R1F */
#elif defined(CC1354R1FRGZ_R10) || defined(CC1354R1FRGZ)
    #define CC_CHIP_ID 0x13543710
#elif defined(CC1354R1F_R10)    || defined(CC1354R1F)
    #define CC_CHIP_ID 0x13543010
/* CC1352R1F */
#elif defined(CC1352R1FRGZ_R10) || defined(CC1352R1FRGZ)
    #define CC_CHIP_ID 0x13523710
#elif defined(CC1352R1F_R10)    || defined(CC1352R1F)
    #define CC_CHIP_ID 0x13523010
/* CC1312R1F */
#elif defined(CC1312R1FRGZ_R10) || defined(CC1312R1FRGZ)
    #define CC_CHIP_ID 0x13123710
#elif defined(CC1312R1F_R10)    || defined(CC1312R1F)
    #define CC_CHIP_ID 0x13123010
#endif

#define CC_GET_CHIP_FAMILY 0x26
#define CC_GET_CHIP_OPTION 0x3
#define CC_GET_CHIP_HWREV 0x20

#ifdef CC_CHIP_ID
    /* Define chip package only if specified */
    #if (CC_CHIP_ID & 0x00000F00) != 0
        #define CC_GET_CHIP_PACKAGE (((CC_CHIP_ID) & 0x00000F00) >> 8)
    #endif

    /* Define chip device */
    #define CC_GET_CHIP_DEVICE (((CC_CHIP_ID) & 0xFFFF0000) >> 16)

    /* The chip family, option and package shall match the DriverLib release */
    #if (CC_GET_CHIP_OPTION != ((CC_CHIP_ID & 0x0000F000) >> 12))
        #error "Specified chip option does not match DriverLib release"
    #endif
    #if (CC_GET_CHIP_HWREV  != ((CC_CHIP_ID & 0x000000FF) >> 0))
        #error "Specified chip hardware revision does not match DriverLib release"
    #endif
#endif

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __HW_CHIP_DEF_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//
//*****************************************************************************
