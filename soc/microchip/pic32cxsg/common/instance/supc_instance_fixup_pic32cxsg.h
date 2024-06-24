/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_SUPC_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_SUPC_INSTANCE_FIXUP_H_

/* ========== Register definition for SUPC peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_SUPC_INTENCLR          (0x40001800) /**< \brief (SUPC) Interrupt Enable Clear */
#define REG_SUPC_INTENSET          (0x40001804) /**< \brief (SUPC) Interrupt Enable Set */
#define REG_SUPC_INTFLAG           (0x40001808) /**< \brief (SUPC) Interrupt Flag Status and Clear */
#define REG_SUPC_STATUS            (0x4000180C) /**< \brief (SUPC) Power and Clocks Status */
#define REG_SUPC_BOD33             (0x40001810) /**< \brief (SUPC) BOD33 Control */
#define REG_SUPC_VREG              (0x40001818) /**< \brief (SUPC) VREG Control */
#define REG_SUPC_VREF              (0x4000181C) /**< \brief (SUPC) VREF Control */
#define REG_SUPC_BBPS              (0x40001820) /**< \brief (SUPC) Battery Backup Power Switch */
#define REG_SUPC_BKOUT             (0x40001824) /**< \brief (SUPC) Backup Output Control */
#define REG_SUPC_BKIN              (0x40001828) /**< \brief (SUPC) Backup Input Control */
#else
#define REG_SUPC_INTENCLR          (*(RwReg  *)0x40001800UL) /**< \brief (SUPC) Interrupt Enable Clear */
#define REG_SUPC_INTENSET          (*(RwReg  *)0x40001804UL) /**< \brief (SUPC) Interrupt Enable Set */
#define REG_SUPC_INTFLAG           (*(RwReg  *)0x40001808UL) /**< \brief (SUPC) Interrupt Flag Status and Clear */
#define REG_SUPC_STATUS            (*(RoReg  *)0x4000180CUL) /**< \brief (SUPC) Power and Clocks Status */
#define REG_SUPC_BOD33             (*(RwReg  *)0x40001810UL) /**< \brief (SUPC) BOD33 Control */
#define REG_SUPC_VREG              (*(RwReg  *)0x40001818UL) /**< \brief (SUPC) VREG Control */
#define REG_SUPC_VREF              (*(RwReg  *)0x4000181CUL) /**< \brief (SUPC) VREF Control */
#define REG_SUPC_BBPS              (*(RwReg  *)0x40001820UL) /**< \brief (SUPC) Battery Backup Power Switch */
#define REG_SUPC_BKOUT             (*(RwReg  *)0x40001824UL) /**< \brief (SUPC) Backup Output Control */
#define REG_SUPC_BKIN              (*(RoReg  *)0x40001828UL) /**< \brief (SUPC) Backup Input Control */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_SUPC_INSTANCE_FIXUP_H_ */
