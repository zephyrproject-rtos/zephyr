/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MICROCHIP_PIC32CXSG_CCL_INSTANCE_FIXUP_H_
#define _MICROCHIP_PIC32CXSG_CCL_INSTANCE_FIXUP_H_

/* ========== Register definition for CCL peripheral ========== */
#if (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__))
#define REG_CCL_CTRL               (0x42003800) /**< \brief (CCL) Control */
#define REG_CCL_SEQCTRL0           (0x42003804) /**< \brief (CCL) SEQ Control x 0 */
#define REG_CCL_SEQCTRL1           (0x42003805) /**< \brief (CCL) SEQ Control x 1 */
#define REG_CCL_LUTCTRL0           (0x42003808) /**< \brief (CCL) LUT Control x 0 */
#define REG_CCL_LUTCTRL1           (0x4200380C) /**< \brief (CCL) LUT Control x 1 */
#define REG_CCL_LUTCTRL2           (0x42003810) /**< \brief (CCL) LUT Control x 2 */
#define REG_CCL_LUTCTRL3           (0x42003814) /**< \brief (CCL) LUT Control x 3 */
#else
#define REG_CCL_CTRL               (*(RwReg8 *)0x42003800UL) /**< \brief (CCL) Control */
#define REG_CCL_SEQCTRL0           (*(RwReg8 *)0x42003804UL) /**< \brief (CCL) SEQ Control x 0 */
#define REG_CCL_SEQCTRL1           (*(RwReg8 *)0x42003805UL) /**< \brief (CCL) SEQ Control x 1 */
#define REG_CCL_LUTCTRL0           (*(RwReg  *)0x42003808UL) /**< \brief (CCL) LUT Control x 0 */
#define REG_CCL_LUTCTRL1           (*(RwReg  *)0x4200380CUL) /**< \brief (CCL) LUT Control x 1 */
#define REG_CCL_LUTCTRL2           (*(RwReg  *)0x42003810UL) /**< \brief (CCL) LUT Control x 2 */
#define REG_CCL_LUTCTRL3           (*(RwReg  *)0x42003814UL) /**< \brief (CCL) LUT Control x 3 */
#endif /* (defined(__ASSEMBLY__) || defined(__IAR_SYSTEMS_ASM__)) */

#endif /* _MICROCHIP_PIC32CXSG_CCL_INSTANCE_FIXUP_H_ */
