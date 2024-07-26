/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_MCLK_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_MCLK_INSTANCE_FIXUP_H_

/* ========== Register definition for MCLK peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_MCLK_INTENCLR          (0x40000801) /**< \brief (MCLK) Interrupt Enable Clear */
#define REG_MCLK_INTENSET          (0x40000802) /**< \brief (MCLK) Interrupt Enable Set */
#define REG_MCLK_INTFLAG           (0x40000803) /**< \brief (MCLK) Interrupt Flag Status and Clear */
#define REG_MCLK_HSDIV             (0x40000804) /**< \brief (MCLK) HS Clock Division */
#define REG_MCLK_CPUDIV            (0x40000805) /**< \brief (MCLK) CPU Clock Division */
#define REG_MCLK_AHBMASK           (0x40000810) /**< \brief (MCLK) AHB Mask */
#define REG_MCLK_APBAMASK          (0x40000814) /**< \brief (MCLK) APBA Mask */
#define REG_MCLK_APBBMASK          (0x40000818) /**< \brief (MCLK) APBB Mask */
#define REG_MCLK_APBCMASK          (0x4000081C) /**< \brief (MCLK) APBC Mask */
#define REG_MCLK_APBDMASK          (0x40000820) /**< \brief (MCLK) APBD Mask */
#else
#define REG_MCLK_INTENCLR          (*(RwReg8 *)0x40000801UL) /**< \brief (MCLK) Interrupt Enable Clear */
#define REG_MCLK_INTENSET          (*(RwReg8 *)0x40000802UL) /**< \brief (MCLK) Interrupt Enable Set */
#define REG_MCLK_INTFLAG           (*(RwReg8 *)0x40000803UL) /**< \brief (MCLK) Interrupt Flag Status and Clear */
#define REG_MCLK_HSDIV             (*(RoReg8 *)0x40000804UL) /**< \brief (MCLK) HS Clock Division */
#define REG_MCLK_CPUDIV            (*(RwReg8 *)0x40000805UL) /**< \brief (MCLK) CPU Clock Division */
#define REG_MCLK_AHBMASK           (*(RwReg  *)0x40000810UL) /**< \brief (MCLK) AHB Mask */
#define REG_MCLK_APBAMASK          (*(RwReg  *)0x40000814UL) /**< \brief (MCLK) APBA Mask */
#define REG_MCLK_APBBMASK          (*(RwReg  *)0x40000818UL) /**< \brief (MCLK) APBB Mask */
#define REG_MCLK_APBCMASK          (*(RwReg  *)0x4000081CUL) /**< \brief (MCLK) APBC Mask */
#define REG_MCLK_APBDMASK          (*(RwReg  *)0x40000820UL) /**< \brief (MCLK) APBD Mask */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_MCLK_INSTANCE_FIXUP_H_ */
