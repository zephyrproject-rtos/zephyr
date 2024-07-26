/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_FREQM_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_FREQM_INSTANCE_FIXUP_H_

/* ========== Register definition for FREQM peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_FREQM_CTRLA            (0x40002C00) /**< \brief (FREQM) Control A Register */
#define REG_FREQM_CTRLB            (0x40002C01) /**< \brief (FREQM) Control B Register */
#define REG_FREQM_CFGA             (0x40002C02) /**< \brief (FREQM) Config A register */
#define REG_FREQM_INTENCLR         (0x40002C08) /**< \brief (FREQM) Interrupt Enable Clear Register */
#define REG_FREQM_INTENSET         (0x40002C09) /**< \brief (FREQM) Interrupt Enable Set Register */
#define REG_FREQM_INTFLAG          (0x40002C0A) /**< \brief (FREQM) Interrupt Flag Register */
#define REG_FREQM_STATUS           (0x40002C0B) /**< \brief (FREQM) Status Register */
#define REG_FREQM_SYNCBUSY         (0x40002C0C) /**< \brief (FREQM) Synchronization Busy Register */
#define REG_FREQM_VALUE            (0x40002C10) /**< \brief (FREQM) Count Value Register */
#else
#define REG_FREQM_CTRLA            (*(RwReg8 *)0x40002C00UL) /**< \brief (FREQM) Control A Register */
#define REG_FREQM_CTRLB            (*(WoReg8 *)0x40002C01UL) /**< \brief (FREQM) Control B Register */
#define REG_FREQM_CFGA             (*(RwReg16*)0x40002C02UL) /**< \brief (FREQM) Config A register */
#define REG_FREQM_INTENCLR         (*(RwReg8 *)0x40002C08UL) /**< \brief (FREQM) Interrupt Enable Clear Register */
#define REG_FREQM_INTENSET         (*(RwReg8 *)0x40002C09UL) /**< \brief (FREQM) Interrupt Enable Set Register */
#define REG_FREQM_INTFLAG          (*(RwReg8 *)0x40002C0AUL) /**< \brief (FREQM) Interrupt Flag Register */
#define REG_FREQM_STATUS           (*(RwReg8 *)0x40002C0BUL) /**< \brief (FREQM) Status Register */
#define REG_FREQM_SYNCBUSY         (*(RoReg  *)0x40002C0CUL) /**< \brief (FREQM) Synchronization Busy Register */
#define REG_FREQM_VALUE            (*(RoReg  *)0x40002C10UL) /**< \brief (FREQM) Count Value Register */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_FREQM_INSTANCE_FIXUP_H_ */
