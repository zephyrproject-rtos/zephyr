/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_RAMECC_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_RAMECC_INSTANCE_FIXUP_H_

/* ========== Register definition for RAMECC peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_RAMECC_INTENCLR        (0x41020000) /**< \brief (RAMECC) Interrupt Enable Clear */
#define REG_RAMECC_INTENSET        (0x41020001) /**< \brief (RAMECC) Interrupt Enable Set */
#define REG_RAMECC_INTFLAG         (0x41020002) /**< \brief (RAMECC) Interrupt Flag */
#define REG_RAMECC_STATUS          (0x41020003) /**< \brief (RAMECC) Status */
#define REG_RAMECC_ERRADDR         (0x41020004) /**< \brief (RAMECC) Error Address */
#define REG_RAMECC_DBGCTRL         (0x4102000F) /**< \brief (RAMECC) Debug Control */
#else
#define REG_RAMECC_INTENCLR        (*(RwReg8 *)0x41020000UL) /**< \brief (RAMECC) Interrupt Enable Clear */
#define REG_RAMECC_INTENSET        (*(RwReg8 *)0x41020001UL) /**< \brief (RAMECC) Interrupt Enable Set */
#define REG_RAMECC_INTFLAG         (*(RwReg8 *)0x41020002UL) /**< \brief (RAMECC) Interrupt Flag */
#define REG_RAMECC_STATUS          (*(RoReg8 *)0x41020003UL) /**< \brief (RAMECC) Status */
#define REG_RAMECC_ERRADDR         (*(RoReg  *)0x41020004UL) /**< \brief (RAMECC) Error Address */
#define REG_RAMECC_DBGCTRL         (*(RwReg8 *)0x4102000FUL) /**< \brief (RAMECC) Debug Control */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_RAMECC_INSTANCE_FIXUP_H_ */
