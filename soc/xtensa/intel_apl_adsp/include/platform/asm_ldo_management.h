/* SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright(c) 2018 Intel Corporation. All rights reserved.
 *
 * Author: Lech Betlej <lech.betlej@linux.intel.com>
 */

/**
 * \file platform/apollolake/include/platform/asm_ldo_management.h
 * \brief Macros for controlling LDO state specific for cAVS 1.5
 * \author Lech Betlej <lech.betlej@linux.intel.com>
 */
#ifndef ASM_LDO_MANAGEMENT_H
#define ASM_LDO_MANAGEMENT_H

#ifndef ASSEMBLY
#warning "Header can only be used by assembly sources."
#endif

#include <platform/shim.h>

.macro m_cavs_set_ldo_state state, ax
movi \ax, (SHIM_BASE + SHIM_LDOCTL)
s32i \state, \ax, 0
memw
// wait loop > 300ns (min 100ns required)
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
// LP SRAM mask
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
// wait loop > 300ns (min 100ns required)
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
// wait loop > 300ns (min 100ns required)
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

#endif /* ASM_LDO_MANAGEMENT_H */
