/*
 * Copyright (c) 2025 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "k3_ctrl_partition.h"
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mm/system_mm.h>
#include <zephyr/arch/arm/cortex_a_r/sys_io.h>

#define KICK_LOCK_VAL    (0x00000000U)
#define KICK0_UNLOCK_VAL (0x68EF3490U)
#define KICK1_UNLOCK_VAL (0xD172BC5AU)

static uint32_t translate_addr(uint32_t pa)
{
	uint32_t va;

#ifdef CONFIG_MM_TI_RAT
	sys_mm_drv_page_phys_get((void *)pa, (uintptr_t *)&va);
#else
	va = pa;
#endif

	return va;
}

#define KICK_ADDRESS(node_id)                                                                      \
	translate_addr(DT_PROP_BY_IDX(node_id, reg, 0) + DT_PROP(node_id, lock_offset))

#define K3_UNLOCK_CTRL_PARTITION(node_id)                                                          \
	{                                                                                          \
		const uint32_t kick_addr = KICK_ADDRESS(node_id);                                  \
		sys_write32(KICK0_UNLOCK_VAL, kick_addr);                                          \
		sys_write32(KICK1_UNLOCK_VAL, kick_addr + sizeof(uint32_t *));                     \
	}

#define K3_LOCK_CTRL_PARTITION(node_id)                                                            \
	{                                                                                          \
		const uint32_t kick_addr = KICK_ADDRESS(node_id);                                  \
		sys_write32(KICK_LOCK_VAL, kick_addr);                                             \
		sys_write32(KICK_LOCK_VAL, kick_addr + sizeof(uint32_t *));                        \
	}

void k3_unlock_all_ctrl_partitions()
{
	DT_FOREACH_STATUS_OKAY(ti_k3_ctrl_partition, K3_UNLOCK_CTRL_PARTITION);
}

void k3_lock_all_ctrl_partitions()
{
	DT_FOREACH_STATUS_OKAY(ti_k3_ctrl_partition, K3_LOCK_CTRL_PARTITION);
}
