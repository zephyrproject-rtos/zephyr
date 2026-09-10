/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/common/instr_mem.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#ifdef CONFIG_HARVARD
#define IN_NODE(inst, compat, base_addr, alloc)                                                    \
	(((uintptr_t)(base_addr) >= DT_REG_ADDR(DT_INST(inst, compat)) &&                          \
	  ((uintptr_t)(base_addr) + (uintptr_t)(alloc)) <=                                         \
		  DT_REG_ADDR(DT_INST(inst, compat)) + DT_REG_SIZE(DT_INST(inst, compat)))) ||
#define IS_INSTR_MEM(base_addr, alloc)                                                             \
	(DT_COMPAT_FOREACH_STATUS_OKAY_VARGS(arc_iccm, IN_NODE, base_addr, alloc) false)
#else
#define IS_INSTR_MEM(base_addr, alloc) true
#endif

bool arch_is_instr_mem(const void *addr, size_t len)
{
	return IS_INSTR_MEM(addr, len);
}
