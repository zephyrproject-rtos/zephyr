/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_PCC_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_PCC_INSTANCE_FIXUP_H_

/* ========== Register definition for PCC peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_PCC_MR                 (0x43002C00) /**< \brief (PCC) Mode Register */
#define REG_PCC_IER                (0x43002C04) /**< \brief (PCC) Interrupt Enable Register */
#define REG_PCC_IDR                (0x43002C08) /**< \brief (PCC) Interrupt Disable Register */
#define REG_PCC_IMR                (0x43002C0C) /**< \brief (PCC) Interrupt Mask Register */
#define REG_PCC_ISR                (0x43002C10) /**< \brief (PCC) Interrupt Status Register */
#define REG_PCC_RHR                (0x43002C14) /**< \brief (PCC) Reception Holding Register */
#define REG_PCC_WPMR               (0x43002CE0) /**< \brief (PCC) Write Protection Mode Register */
#define REG_PCC_WPSR               (0x43002CE4) /**< \brief (PCC) Write Protection Status Register */
#else
#define REG_PCC_MR                 (*(RwReg  *)0x43002C00UL) /**< \brief (PCC) Mode Register */
#define REG_PCC_IER                (*(WoReg  *)0x43002C04UL) /**< \brief (PCC) Interrupt Enable Register */
#define REG_PCC_IDR                (*(WoReg  *)0x43002C08UL) /**< \brief (PCC) Interrupt Disable Register */
#define REG_PCC_IMR                (*(RoReg  *)0x43002C0CUL) /**< \brief (PCC) Interrupt Mask Register */
#define REG_PCC_ISR                (*(RoReg  *)0x43002C10UL) /**< \brief (PCC) Interrupt Status Register */
#define REG_PCC_RHR                (*(RoReg  *)0x43002C14UL) /**< \brief (PCC) Reception Holding Register */
#define REG_PCC_WPMR               (*(RwReg  *)0x43002CE0UL) /**< \brief (PCC) Write Protection Mode Register */
#define REG_PCC_WPSR               (*(RoReg  *)0x43002CE4UL) /**< \brief (PCC) Write Protection Status Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_PCC_INSTANCE_FIXUP_H_ */
