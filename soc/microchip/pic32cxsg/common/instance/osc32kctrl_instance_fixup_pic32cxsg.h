/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_OSC32KCTRL_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_OSC32KCTRL_INSTANCE_FIXUP_H_

/* ========== Register definition for OSC32KCTRL peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_OSC32KCTRL_INTENCLR    (0x40001400) /**< \brief (OSC32KCTRL) Interrupt Enable Clear */
#define REG_OSC32KCTRL_INTENSET    (0x40001404) /**< \brief (OSC32KCTRL) Interrupt Enable Set */
#define REG_OSC32KCTRL_INTFLAG     (0x40001408) /**< \brief (OSC32KCTRL) Interrupt Flag Status and Clear */
#define REG_OSC32KCTRL_STATUS      (0x4000140C) /**< \brief (OSC32KCTRL) Power and Clocks Status */
#define REG_OSC32KCTRL_RTCCTRL     (0x40001410) /**< \brief (OSC32KCTRL) RTC Clock Selection */
#define REG_OSC32KCTRL_XOSC32K     (0x40001414) /**< \brief (OSC32KCTRL) 32kHz External Crystal Oscillator (XOSC32K) Control */
#define REG_OSC32KCTRL_CFDCTRL     (0x40001416) /**< \brief (OSC32KCTRL) Clock Failure Detector Control */
#define REG_OSC32KCTRL_EVCTRL      (0x40001417) /**< \brief (OSC32KCTRL) Event Control */
#define REG_OSC32KCTRL_OSCULP32K   (0x4000141C) /**< \brief (OSC32KCTRL) 32kHz Ultra Low Power Internal Oscillator (OSCULP32K) Control */
#else
#define REG_OSC32KCTRL_INTENCLR    (*(RwReg  *)0x40001400UL) /**< \brief (OSC32KCTRL) Interrupt Enable Clear */
#define REG_OSC32KCTRL_INTENSET    (*(RwReg  *)0x40001404UL) /**< \brief (OSC32KCTRL) Interrupt Enable Set */
#define REG_OSC32KCTRL_INTFLAG     (*(RwReg  *)0x40001408UL) /**< \brief (OSC32KCTRL) Interrupt Flag Status and Clear */
#define REG_OSC32KCTRL_STATUS      (*(RoReg  *)0x4000140CUL) /**< \brief (OSC32KCTRL) Power and Clocks Status */
#define REG_OSC32KCTRL_RTCCTRL     (*(RwReg8 *)0x40001410UL) /**< \brief (OSC32KCTRL) RTC Clock Selection */
#define REG_OSC32KCTRL_XOSC32K     (*(RwReg16*)0x40001414UL) /**< \brief (OSC32KCTRL) 32kHz External Crystal Oscillator (XOSC32K) Control */
#define REG_OSC32KCTRL_CFDCTRL     (*(RwReg8 *)0x40001416UL) /**< \brief (OSC32KCTRL) Clock Failure Detector Control */
#define REG_OSC32KCTRL_EVCTRL      (*(RwReg8 *)0x40001417UL) /**< \brief (OSC32KCTRL) Event Control */
#define REG_OSC32KCTRL_OSCULP32K   (*(RwReg  *)0x4000141CUL) /**< \brief (OSC32KCTRL) 32kHz Ultra Low Power Internal Oscillator (OSCULP32K) Control */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_OSC32KCTRL_INSTANCE_FIXUP_H_ */
