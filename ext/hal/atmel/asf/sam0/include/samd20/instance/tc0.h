/**
 * \file
 *
 * \brief Instance description for TC0
 *
 * Copyright (c) 2017 Microchip Technology Inc.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

#ifndef _SAMD20_TC0_INSTANCE_
#define _SAMD20_TC0_INSTANCE_

/* ========== Register definition for TC0 peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_TC0_CTRLA              (0x42002000) /**< \brief (TC0) Control A */
#define REG_TC0_READREQ            (0x42002002) /**< \brief (TC0) Read Request */
#define REG_TC0_CTRLBCLR           (0x42002004) /**< \brief (TC0) Control B Clear */
#define REG_TC0_CTRLBSET           (0x42002005) /**< \brief (TC0) Control B Set */
#define REG_TC0_CTRLC              (0x42002006) /**< \brief (TC0) Control C */
#define REG_TC0_DBGCTRL            (0x42002008) /**< \brief (TC0) Debug Control */
#define REG_TC0_EVCTRL             (0x4200200A) /**< \brief (TC0) Event Control */
#define REG_TC0_INTENCLR           (0x4200200C) /**< \brief (TC0) Interrupt Enable Clear */
#define REG_TC0_INTENSET           (0x4200200D) /**< \brief (TC0) Interrupt Enable Set */
#define REG_TC0_INTFLAG            (0x4200200E) /**< \brief (TC0) Interrupt Flag Status and Clear */
#define REG_TC0_STATUS             (0x4200200F) /**< \brief (TC0) Status */
#define REG_TC0_COUNT16_COUNT      (0x42002010) /**< \brief (TC0) COUNT16 Counter Value */
#define REG_TC0_COUNT16_CC0        (0x42002018) /**< \brief (TC0) COUNT16 Compare/Capture 0 */
#define REG_TC0_COUNT16_CC1        (0x4200201A) /**< \brief (TC0) COUNT16 Compare/Capture 1 */
#define REG_TC0_COUNT32_COUNT      (0x42002010) /**< \brief (TC0) COUNT32 Counter Value */
#define REG_TC0_COUNT32_CC0        (0x42002018) /**< \brief (TC0) COUNT32 Compare/Capture 0 */
#define REG_TC0_COUNT32_CC1        (0x4200201C) /**< \brief (TC0) COUNT32 Compare/Capture 1 */
#define REG_TC0_COUNT8_COUNT       (0x42002010) /**< \brief (TC0) COUNT8 Counter Value */
#define REG_TC0_COUNT8_PER         (0x42002014) /**< \brief (TC0) COUNT8 Period Value */
#define REG_TC0_COUNT8_CC0         (0x42002018) /**< \brief (TC0) COUNT8 Compare/Capture 0 */
#define REG_TC0_COUNT8_CC1         (0x42002019) /**< \brief (TC0) COUNT8 Compare/Capture 1 */
#else
#define REG_TC0_CTRLA              (*(RwReg16*)0x42002000UL) /**< \brief (TC0) Control A */
#define REG_TC0_READREQ            (*(RwReg16*)0x42002002UL) /**< \brief (TC0) Read Request */
#define REG_TC0_CTRLBCLR           (*(RwReg8 *)0x42002004UL) /**< \brief (TC0) Control B Clear */
#define REG_TC0_CTRLBSET           (*(RwReg8 *)0x42002005UL) /**< \brief (TC0) Control B Set */
#define REG_TC0_CTRLC              (*(RwReg8 *)0x42002006UL) /**< \brief (TC0) Control C */
#define REG_TC0_DBGCTRL            (*(RwReg8 *)0x42002008UL) /**< \brief (TC0) Debug Control */
#define REG_TC0_EVCTRL             (*(RwReg16*)0x4200200AUL) /**< \brief (TC0) Event Control */
#define REG_TC0_INTENCLR           (*(RwReg8 *)0x4200200CUL) /**< \brief (TC0) Interrupt Enable Clear */
#define REG_TC0_INTENSET           (*(RwReg8 *)0x4200200DUL) /**< \brief (TC0) Interrupt Enable Set */
#define REG_TC0_INTFLAG            (*(RwReg8 *)0x4200200EUL) /**< \brief (TC0) Interrupt Flag Status and Clear */
#define REG_TC0_STATUS             (*(RoReg8 *)0x4200200FUL) /**< \brief (TC0) Status */
#define REG_TC0_COUNT16_COUNT      (*(RwReg16*)0x42002010UL) /**< \brief (TC0) COUNT16 Counter Value */
#define REG_TC0_COUNT16_CC0        (*(RwReg16*)0x42002018UL) /**< \brief (TC0) COUNT16 Compare/Capture 0 */
#define REG_TC0_COUNT16_CC1        (*(RwReg16*)0x4200201AUL) /**< \brief (TC0) COUNT16 Compare/Capture 1 */
#define REG_TC0_COUNT32_COUNT      (*(RwReg  *)0x42002010UL) /**< \brief (TC0) COUNT32 Counter Value */
#define REG_TC0_COUNT32_CC0        (*(RwReg  *)0x42002018UL) /**< \brief (TC0) COUNT32 Compare/Capture 0 */
#define REG_TC0_COUNT32_CC1        (*(RwReg  *)0x4200201CUL) /**< \brief (TC0) COUNT32 Compare/Capture 1 */
#define REG_TC0_COUNT8_COUNT       (*(RwReg8 *)0x42002010UL) /**< \brief (TC0) COUNT8 Counter Value */
#define REG_TC0_COUNT8_PER         (*(RwReg8 *)0x42002014UL) /**< \brief (TC0) COUNT8 Period Value */
#define REG_TC0_COUNT8_CC0         (*(RwReg8 *)0x42002018UL) /**< \brief (TC0) COUNT8 Compare/Capture 0 */
#define REG_TC0_COUNT8_CC1         (*(RwReg8 *)0x42002019UL) /**< \brief (TC0) COUNT8 Compare/Capture 1 */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* ========== Instance parameters for TC0 peripheral ========== */
#define TC0_CC8_NUM                 2       
#define TC0_CC16_NUM                2       
#define TC0_CC32_NUM                2       
#define TC0_DITHERING_EXT           0       
#define TC0_GCLK_ID                 19      
#define TC0_MASTER                  1       
#define TC0_OW_NUM                  2       
#define TC0_PERIOD_EXT              0       
#define TC0_SHADOW_EXT              0       

#endif /* _SAMD20_TC0_INSTANCE_ */
