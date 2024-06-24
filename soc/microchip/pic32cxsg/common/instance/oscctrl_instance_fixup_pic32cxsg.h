/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_OSCCTRL_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_OSCCTRL_INSTANCE_FIXUP_H_

/* ========== Register definition for OSCCTRL peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_OSCCTRL_EVCTRL         (0x40001000) /**< \brief (OSCCTRL) Event Control */
#define REG_OSCCTRL_INTENCLR       (0x40001004) /**< \brief (OSCCTRL) Interrupt Enable Clear */
#define REG_OSCCTRL_INTENSET       (0x40001008) /**< \brief (OSCCTRL) Interrupt Enable Set */
#define REG_OSCCTRL_INTFLAG        (0x4000100C) /**< \brief (OSCCTRL) Interrupt Flag Status and Clear */
#define REG_OSCCTRL_STATUS         (0x40001010) /**< \brief (OSCCTRL) Status */
#define REG_OSCCTRL_XOSCCTRL0      (0x40001014) /**< \brief (OSCCTRL) External Multipurpose Crystal Oscillator Control 0 */
#define REG_OSCCTRL_XOSCCTRL1      (0x40001018) /**< \brief (OSCCTRL) External Multipurpose Crystal Oscillator Control 1 */
#define REG_OSCCTRL_DFLLCTRLA      (0x4000101C) /**< \brief (OSCCTRL) DFLL48M Control A */
#define REG_OSCCTRL_DFLLCTRLB      (0x40001020) /**< \brief (OSCCTRL) DFLL48M Control B */
#define REG_OSCCTRL_DFLLVAL        (0x40001024) /**< \brief (OSCCTRL) DFLL48M Value */
#define REG_OSCCTRL_DFLLMUL        (0x40001028) /**< \brief (OSCCTRL) DFLL48M Multiplier */
#define REG_OSCCTRL_DFLLSYNC       (0x4000102C) /**< \brief (OSCCTRL) DFLL48M Synchronization */
#define REG_OSCCTRL_DPLLCTRLA0     (0x40001030) /**< \brief (OSCCTRL) DPLL Control A 0 */
#define REG_OSCCTRL_DPLLRATIO0     (0x40001034) /**< \brief (OSCCTRL) DPLL Ratio Control 0 */
#define REG_OSCCTRL_DPLLCTRLB0     (0x40001038) /**< \brief (OSCCTRL) DPLL Control B 0 */
#define REG_OSCCTRL_DPLLSYNCBUSY0  (0x4000103C) /**< \brief (OSCCTRL) DPLL Synchronization Busy 0 */
#define REG_OSCCTRL_DPLLSTATUS0    (0x40001040) /**< \brief (OSCCTRL) DPLL Status 0 */
#define REG_OSCCTRL_DPLLCTRLA1     (0x40001044) /**< \brief (OSCCTRL) DPLL Control A 1 */
#define REG_OSCCTRL_DPLLRATIO1     (0x40001048) /**< \brief (OSCCTRL) DPLL Ratio Control 1 */
#define REG_OSCCTRL_DPLLCTRLB1     (0x4000104C) /**< \brief (OSCCTRL) DPLL Control B 1 */
#define REG_OSCCTRL_DPLLSYNCBUSY1  (0x40001050) /**< \brief (OSCCTRL) DPLL Synchronization Busy 1 */
#define REG_OSCCTRL_DPLLSTATUS1    (0x40001054) /**< \brief (OSCCTRL) DPLL Status 1 */
#else
#define REG_OSCCTRL_EVCTRL         (*(RwReg8 *)0x40001000UL) /**< \brief (OSCCTRL) Event Control */
#define REG_OSCCTRL_INTENCLR       (*(RwReg  *)0x40001004UL) /**< \brief (OSCCTRL) Interrupt Enable Clear */
#define REG_OSCCTRL_INTENSET       (*(RwReg  *)0x40001008UL) /**< \brief (OSCCTRL) Interrupt Enable Set */
#define REG_OSCCTRL_INTFLAG        (*(RwReg  *)0x4000100CUL) /**< \brief (OSCCTRL) Interrupt Flag Status and Clear */
#define REG_OSCCTRL_STATUS         (*(RoReg  *)0x40001010UL) /**< \brief (OSCCTRL) Status */
#define REG_OSCCTRL_XOSCCTRL0      (*(RwReg  *)0x40001014UL) /**< \brief (OSCCTRL) External Multipurpose Crystal Oscillator Control 0 */
#define REG_OSCCTRL_XOSCCTRL1      (*(RwReg  *)0x40001018UL) /**< \brief (OSCCTRL) External Multipurpose Crystal Oscillator Control 1 */
#define REG_OSCCTRL_DFLLCTRLA      (*(RwReg8 *)0x4000101CUL) /**< \brief (OSCCTRL) DFLL48M Control A */
#define REG_OSCCTRL_DFLLCTRLB      (*(RwReg8 *)0x40001020UL) /**< \brief (OSCCTRL) DFLL48M Control B */
#define REG_OSCCTRL_DFLLVAL        (*(RwReg  *)0x40001024UL) /**< \brief (OSCCTRL) DFLL48M Value */
#define REG_OSCCTRL_DFLLMUL        (*(RwReg  *)0x40001028UL) /**< \brief (OSCCTRL) DFLL48M Multiplier */
#define REG_OSCCTRL_DFLLSYNC       (*(RwReg8 *)0x4000102CUL) /**< \brief (OSCCTRL) DFLL48M Synchronization */
#define REG_OSCCTRL_DPLLCTRLA0     (*(RwReg8 *)0x40001030UL) /**< \brief (OSCCTRL) DPLL Control A 0 */
#define REG_OSCCTRL_DPLLRATIO0     (*(RwReg  *)0x40001034UL) /**< \brief (OSCCTRL) DPLL Ratio Control 0 */
#define REG_OSCCTRL_DPLLCTRLB0     (*(RwReg  *)0x40001038UL) /**< \brief (OSCCTRL) DPLL Control B 0 */
#define REG_OSCCTRL_DPLLSYNCBUSY0  (*(RoReg  *)0x4000103CUL) /**< \brief (OSCCTRL) DPLL Synchronization Busy 0 */
#define REG_OSCCTRL_DPLLSTATUS0    (*(RoReg  *)0x40001040UL) /**< \brief (OSCCTRL) DPLL Status 0 */
#define REG_OSCCTRL_DPLLCTRLA1     (*(RwReg8 *)0x40001044UL) /**< \brief (OSCCTRL) DPLL Control A 1 */
#define REG_OSCCTRL_DPLLRATIO1     (*(RwReg  *)0x40001048UL) /**< \brief (OSCCTRL) DPLL Ratio Control 1 */
#define REG_OSCCTRL_DPLLCTRLB1     (*(RwReg  *)0x4000104CUL) /**< \brief (OSCCTRL) DPLL Control B 1 */
#define REG_OSCCTRL_DPLLSYNCBUSY1  (*(RoReg  *)0x40001050UL) /**< \brief (OSCCTRL) DPLL Synchronization Busy 1 */
#define REG_OSCCTRL_DPLLSTATUS1    (*(RoReg  *)0x40001054UL) /**< \brief (OSCCTRL) DPLL Status 1 */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_OSCCTRL_INSTANCE_FIXUP_H_ */
