/**
 * \file
 *
 * \brief Instance description for SYSCTRL
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

#ifndef _SAMD20_SYSCTRL_INSTANCE_
#define _SAMD20_SYSCTRL_INSTANCE_

/* ========== Register definition for SYSCTRL peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_SYSCTRL_INTENCLR       (0x40000800) /**< \brief (SYSCTRL) Interrupt Enable Clear */
#define REG_SYSCTRL_INTENSET       (0x40000804) /**< \brief (SYSCTRL) Interrupt Enable Set */
#define REG_SYSCTRL_INTFLAG        (0x40000808) /**< \brief (SYSCTRL) Interrupt Flag Status and Clear */
#define REG_SYSCTRL_PCLKSR         (0x4000080C) /**< \brief (SYSCTRL) Power and Clocks Status */
#define REG_SYSCTRL_XOSC           (0x40000810) /**< \brief (SYSCTRL) XOSC Control */
#define REG_SYSCTRL_XOSC32K        (0x40000814) /**< \brief (SYSCTRL) XOSC32K Control */
#define REG_SYSCTRL_OSC32K         (0x40000818) /**< \brief (SYSCTRL) OSC32K Control */
#define REG_SYSCTRL_OSCULP32K      (0x4000081C) /**< \brief (SYSCTRL) OSCULP32K Control */
#define REG_SYSCTRL_OSC8M          (0x40000820) /**< \brief (SYSCTRL) OSC8M Control A */
#define REG_SYSCTRL_DFLLCTRL       (0x40000824) /**< \brief (SYSCTRL) DFLL Config */
#define REG_SYSCTRL_DFLLVAL        (0x40000828) /**< \brief (SYSCTRL) DFLL Calibration Value */
#define REG_SYSCTRL_DFLLMUL        (0x4000082C) /**< \brief (SYSCTRL) DFLL Multiplier */
#define REG_SYSCTRL_DFLLSYNC       (0x40000830) /**< \brief (SYSCTRL) DFLL Synchronization */
#define REG_SYSCTRL_BOD33          (0x40000834) /**< \brief (SYSCTRL) 3.3V Brown-Out Detector (BOD33) Control */
#define REG_SYSCTRL_VREG           (0x4000083C) /**< \brief (SYSCTRL) VREG Control */
#define REG_SYSCTRL_VREF           (0x40000840) /**< \brief (SYSCTRL) VREF Control A */
#else
#define REG_SYSCTRL_INTENCLR       (*(RwReg  *)0x40000800UL) /**< \brief (SYSCTRL) Interrupt Enable Clear */
#define REG_SYSCTRL_INTENSET       (*(RwReg  *)0x40000804UL) /**< \brief (SYSCTRL) Interrupt Enable Set */
#define REG_SYSCTRL_INTFLAG        (*(RwReg  *)0x40000808UL) /**< \brief (SYSCTRL) Interrupt Flag Status and Clear */
#define REG_SYSCTRL_PCLKSR         (*(RoReg  *)0x4000080CUL) /**< \brief (SYSCTRL) Power and Clocks Status */
#define REG_SYSCTRL_XOSC           (*(RwReg16*)0x40000810UL) /**< \brief (SYSCTRL) XOSC Control */
#define REG_SYSCTRL_XOSC32K        (*(RwReg16*)0x40000814UL) /**< \brief (SYSCTRL) XOSC32K Control */
#define REG_SYSCTRL_OSC32K         (*(RwReg  *)0x40000818UL) /**< \brief (SYSCTRL) OSC32K Control */
#define REG_SYSCTRL_OSCULP32K      (*(RwReg8 *)0x4000081CUL) /**< \brief (SYSCTRL) OSCULP32K Control */
#define REG_SYSCTRL_OSC8M          (*(RwReg  *)0x40000820UL) /**< \brief (SYSCTRL) OSC8M Control A */
#define REG_SYSCTRL_DFLLCTRL       (*(RwReg16*)0x40000824UL) /**< \brief (SYSCTRL) DFLL Config */
#define REG_SYSCTRL_DFLLVAL        (*(RwReg  *)0x40000828UL) /**< \brief (SYSCTRL) DFLL Calibration Value */
#define REG_SYSCTRL_DFLLMUL        (*(RwReg  *)0x4000082CUL) /**< \brief (SYSCTRL) DFLL Multiplier */
#define REG_SYSCTRL_DFLLSYNC       (*(RwReg8 *)0x40000830UL) /**< \brief (SYSCTRL) DFLL Synchronization */
#define REG_SYSCTRL_BOD33          (*(RwReg  *)0x40000834UL) /**< \brief (SYSCTRL) 3.3V Brown-Out Detector (BOD33) Control */
#define REG_SYSCTRL_VREG           (*(RwReg16*)0x4000083CUL) /**< \brief (SYSCTRL) VREG Control */
#define REG_SYSCTRL_VREF           (*(RwReg  *)0x40000840UL) /**< \brief (SYSCTRL) VREF Control A */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

/* ========== Instance parameters for SYSCTRL peripheral ========== */
#define SYSCTRL_BGAP_CALIB_MSB      11      
#define SYSCTRL_BOD33_CALIB_MSB     5       
#define SYSCTRL_DFLL48M_COARSE_MSB  4       
#define SYSCTRL_DFLL48M_FINE_MSB    7       
#define SYSCTRL_GCLK_ID_DFLL48      0       
#define SYSCTRL_OSC32K_COARSE_CALIB_MSB 6       
#define SYSCTRL_POR33_ENTEST_MSB    1       
#define SYSCTRL_ULPVREF_DIVLEV_MSB  3       
#define SYSCTRL_ULPVREG_FORCEGAIN_MSB 1       
#define SYSCTRL_ULPVREG_RAMREFSEL_MSB 2       
#define SYSCTRL_VREF_CONTROL_MSB    48      
#define SYSCTRL_VREF_STATUS_MSB     7       
#define SYSCTRL_VREG_LEVEL_MSB      2       
#define SYSCTRL_BOD12_VERSION       0x111   
#define SYSCTRL_BOD33_VERSION       0x111   
#define SYSCTRL_DFLL48M_VERSION     0x211   
#define SYSCTRL_GCLK_VERSION        0x210   
#define SYSCTRL_OSCULP32K_VERSION   0x111   
#define SYSCTRL_OSC8M_VERSION       0x120   
#define SYSCTRL_OSC32K_VERSION      0x1101  
#define SYSCTRL_VREF_VERSION        0x200   
#define SYSCTRL_VREG_VERSION        0x201   
#define SYSCTRL_XOSC_VERSION        0x1101  
#define SYSCTRL_XOSC32K_VERSION     0x1101  

#endif /* _SAMD20_SYSCTRL_INSTANCE_ */
