/*
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/common/instr_mem.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#define INSTR_MEM_NODE DT_NODELABEL(instr_mem)

#ifdef CONFIG_HARVARD
#if DT_NODE_EXISTS(INSTR_MEM_NODE)
#define IS_INSTR_MEM(base_addr, alloc)                                                             \
	(((uintptr_t)(base_addr) >= DT_REG_ADDR(INSTR_MEM_NODE)) &&                                \
	 (((uintptr_t)(base_addr) + (uintptr_t)(alloc)) <=                                         \
	  DT_REG_ADDR(INSTR_MEM_NODE) + DT_REG_SIZE(INSTR_MEM_NODE)))
#else
#define IS_INSTR_MEM(base_addr, alloc) false /* Unknown if instruction memory */
#endif
#else
#define IS_INSTR_MEM(base_addr, alloc) true
#endif

bool arch_is_instr_mem(const void *addr, size_t len)
{
	return IS_INSTR_MEM(addr, len);
}
