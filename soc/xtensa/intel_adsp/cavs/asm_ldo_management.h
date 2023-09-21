/* Copyright (c) 2023 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_CAVS_LIB_ASM_LDO_MANAGEMENT_H__
#define __ZEPHYR_CAVS_LIB_ASM_LDO_MANAGEMENT_H__

#ifdef _ASMLANGUAGE

#define SHIM_BASE		0x00071F00
#define SHIM_LDOCTL		0xA4
#define SHIM_LDOCTL_HPSRAM_MASK		(3 << 0 | 3 << 16)
#define SHIM_LDOCTL_LPSRAM_MASK		(3 << 2)
#define SHIM_LDOCTL_HPSRAM_LDO_ON	(3 << 0 | 3 << 16)
#define SHIM_LDOCTL_LPSRAM_LDO_ON	(3 << 2)
#define SHIM_LDOCTL_HPSRAM_LDO_OFF	(0 << 0)
#define SHIM_LDOCTL_LPSRAM_LDO_OFF	(0 << 2)
#define SHIM_LDOCTL_HPSRAM_LDO_BYPASS	(BIT(0) | BIT(16))
#define SHIM_LDOCTL_LPSRAM_LDO_BYPASS	BIT(2)

.macro m_cavs_set_ldo_state state, ax
movi \ax, (SHIM_BASE + SHIM_LDOCTL)
s32i \state, \ax, 0
memw
/* wait loop > 300ns (min 100ns required) */
movi \ax, 128
1 :
addi \ax, \ax, -1
nop
bnez \ax, 1b
.endm

.macro m_cavs_set_hpldo_state state, ax, ay
movi \ax, (SHIM_BASE + SHIM_LDOCTL)
l32i \ay, \ax, 0

movi \ax, ~(SHIM_LDOCTL_HPSRAM_MASK)
and \ay, \ax, \ay
or \state, \ay, \state

m_cavs_set_ldo_state \state, \ax
.endm

.macro m_cavs_set_lpldo_state state, ax, ay
movi \ax, (SHIM_BASE + SHIM_LDOCTL)
l32i \ay, \ax, 0
/* LP SRAM mask */
movi \ax, ~(SHIM_LDOCTL_LPSRAM_MASK)
and \ay, \ax, \ay
or \state, \ay, \state

m_cavs_set_ldo_state \state, \ax
.endm

.macro m_cavs_set_ldo_on_state ax, ay, az
movi \ay, (SHIM_BASE + SHIM_LDOCTL)
l32i \az, \ay, 0

movi \ax, ~(SHIM_LDOCTL_HPSRAM_MASK | SHIM_LDOCTL_LPSRAM_MASK)
and \az, \ax, \az
movi \ax, (SHIM_LDOCTL_HPSRAM_LDO_ON | SHIM_LDOCTL_LPSRAM_LDO_ON)
or \ax, \az, \ax

m_cavs_set_ldo_state \ax, \ay
.endm

.macro m_cavs_set_ldo_off_state ax, ay, az
/* wait loop > 300ns (min 100ns required) */
movi \ax, 128
1 :
		addi \ax, \ax, -1
		nop
		bnez \ax, 1b
movi \ay, (SHIM_BASE + SHIM_LDOCTL)
l32i \az, \ay, 0

movi \ax, ~(SHIM_LDOCTL_HPSRAM_MASK | SHIM_LDOCTL_LPSRAM_MASK)
and \az, \az, \ax

movi \ax, (SHIM_LDOCTL_HPSRAM_LDO_OFF | SHIM_LDOCTL_LPSRAM_LDO_OFF)
or \ax, \ax, \az

s32i \ax, \ay, 0
l32i \ax, \ay, 0
.endm

.macro m_cavs_set_ldo_bypass_state ax, ay, az
/* wait loop > 300ns (min 100ns required) */
movi \ax, 128
1 :
		addi \ax, \ax, -1
		nop
		bnez \ax, 1b
movi \ay, (SHIM_BASE + SHIM_LDOCTL)
l32i \az, \ay, 0

movi \ax, ~(SHIM_LDOCTL_HPSRAM_MASK | SHIM_LDOCTL_LPSRAM_MASK)
and \az, \az, \ax

movi \ax, (SHIM_LDOCTL_HPSRAM_LDO_BYPASS | SHIM_LDOCTL_LPSRAM_LDO_BYPASS)
or \ax, \ax, \az

s32i \ax, \ay, 0
l32i \ax, \ay, 0
.endm

#endif
#endif /* __ZEPHYR_CAVS_LIB_ASM_LDO_MANAGEMENT_H__ */
