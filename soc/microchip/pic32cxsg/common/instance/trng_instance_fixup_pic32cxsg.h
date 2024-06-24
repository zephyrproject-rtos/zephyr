/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_TRNG_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_TRNG_INSTANCE_FIXUP_H_

/* ========== Register definition for TRNG peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_TRNG_CTRLA             (0x42002800) /**< \brief (TRNG) Control A */
#define REG_TRNG_EVCTRL            (0x42002804) /**< \brief (TRNG) Event Control */
#define REG_TRNG_INTENCLR          (0x42002808) /**< \brief (TRNG) Interrupt Enable Clear */
#define REG_TRNG_INTENSET          (0x42002809) /**< \brief (TRNG) Interrupt Enable Set */
#define REG_TRNG_INTFLAG           (0x4200280A) /**< \brief (TRNG) Interrupt Flag Status and Clear */
#define REG_TRNG_DATA              (0x42002820) /**< \brief (TRNG) Output Data */
#else
#define REG_TRNG_CTRLA             (*(RwReg8 *)0x42002800UL) /**< \brief (TRNG) Control A */
#define REG_TRNG_EVCTRL            (*(RwReg8 *)0x42002804UL) /**< \brief (TRNG) Event Control */
#define REG_TRNG_INTENCLR          (*(RwReg8 *)0x42002808UL) /**< \brief (TRNG) Interrupt Enable Clear */
#define REG_TRNG_INTENSET          (*(RwReg8 *)0x42002809UL) /**< \brief (TRNG) Interrupt Enable Set */
#define REG_TRNG_INTFLAG           (*(RwReg8 *)0x4200280AUL) /**< \brief (TRNG) Interrupt Flag Status and Clear */
#define REG_TRNG_DATA              (*(RoReg  *)0x42002820UL) /**< \brief (TRNG) Output Data */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_TRNG_INSTANCE_FIXUP_H_ */
