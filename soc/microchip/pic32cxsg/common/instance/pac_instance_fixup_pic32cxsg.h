/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_PAC_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_PAC_INSTANCE_FIXUP_H_

/* ========== Register definition for PAC peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_PAC_WRCTRL             (0x40000000) /**< \brief (PAC) Write control */
#define REG_PAC_EVCTRL             (0x40000004) /**< \brief (PAC) Event control */
#define REG_PAC_INTENCLR           (0x40000008) /**< \brief (PAC) Interrupt enable clear */
#define REG_PAC_INTENSET           (0x40000009) /**< \brief (PAC) Interrupt enable set */
#define REG_PAC_INTFLAGAHB         (0x40000010) /**< \brief (PAC) Bridge interrupt flag status */
#define REG_PAC_INTFLAGA           (0x40000014) /**< \brief (PAC) Peripheral interrupt flag status - Bridge A */
#define REG_PAC_INTFLAGB           (0x40000018) /**< \brief (PAC) Peripheral interrupt flag status - Bridge B */
#define REG_PAC_INTFLAGC           (0x4000001C) /**< \brief (PAC) Peripheral interrupt flag status - Bridge C */
#define REG_PAC_INTFLAGD           (0x40000020) /**< \brief (PAC) Peripheral interrupt flag status - Bridge D */
#define REG_PAC_STATUSA            (0x40000034) /**< \brief (PAC) Peripheral write protection status - Bridge A */
#define REG_PAC_STATUSB            (0x40000038) /**< \brief (PAC) Peripheral write protection status - Bridge B */
#define REG_PAC_STATUSC            (0x4000003C) /**< \brief (PAC) Peripheral write protection status - Bridge C */
#define REG_PAC_STATUSD            (0x40000040) /**< \brief (PAC) Peripheral write protection status - Bridge D */
#else
#define REG_PAC_WRCTRL             (*(RwReg  *)0x40000000UL) /**< \brief (PAC) Write control */
#define REG_PAC_EVCTRL             (*(RwReg8 *)0x40000004UL) /**< \brief (PAC) Event control */
#define REG_PAC_INTENCLR           (*(RwReg8 *)0x40000008UL) /**< \brief (PAC) Interrupt enable clear */
#define REG_PAC_INTENSET           (*(RwReg8 *)0x40000009UL) /**< \brief (PAC) Interrupt enable set */
#define REG_PAC_INTFLAGAHB         (*(RwReg  *)0x40000010UL) /**< \brief (PAC) Bridge interrupt flag status */
#define REG_PAC_INTFLAGA           (*(RwReg  *)0x40000014UL) /**< \brief (PAC) Peripheral interrupt flag status - Bridge A */
#define REG_PAC_INTFLAGB           (*(RwReg  *)0x40000018UL) /**< \brief (PAC) Peripheral interrupt flag status - Bridge B */
#define REG_PAC_INTFLAGC           (*(RwReg  *)0x4000001CUL) /**< \brief (PAC) Peripheral interrupt flag status - Bridge C */
#define REG_PAC_INTFLAGD           (*(RwReg  *)0x40000020UL) /**< \brief (PAC) Peripheral interrupt flag status - Bridge D */
#define REG_PAC_STATUSA            (*(RoReg  *)0x40000034UL) /**< \brief (PAC) Peripheral write protection status - Bridge A */
#define REG_PAC_STATUSB            (*(RoReg  *)0x40000038UL) /**< \brief (PAC) Peripheral write protection status - Bridge B */
#define REG_PAC_STATUSC            (*(RoReg  *)0x4000003CUL) /**< \brief (PAC) Peripheral write protection status - Bridge C */
#define REG_PAC_STATUSD            (*(RoReg  *)0x40000040UL) /**< \brief (PAC) Peripheral write protection status - Bridge D */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_PAC_INSTANCE_FIXUP_H_ */
