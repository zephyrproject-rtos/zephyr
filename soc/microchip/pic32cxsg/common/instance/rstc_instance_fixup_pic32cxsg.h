/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_RSTC_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_RSTC_INSTANCE_FIXUP_H_

/* ========== Register definition for RSTC peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_RSTC_RCAUSE            (0x40000C00) /**< \brief (RSTC) Reset Cause */
#define REG_RSTC_BKUPEXIT          (0x40000C02) /**< \brief (RSTC) Backup Exit Source */
#else
#define REG_RSTC_RCAUSE            (*(RoReg8 *)0x40000C00UL) /**< \brief (RSTC) Reset Cause */
#define REG_RSTC_BKUPEXIT          (*(RoReg8 *)0x40000C02UL) /**< \brief (RSTC) Backup Exit Source */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_RSTC_INSTANCE_FIXUP_H_ */
