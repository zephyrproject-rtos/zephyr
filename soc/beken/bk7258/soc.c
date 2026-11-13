/*
 * Copyright 2020-2021 Beken
 * Copyright (c) 2026 Embracecactus
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#define BK7258_AON_WDT_CTRL   0x44000600U
#define BK7258_APB_WDT_GLOBAL 0x44800008U
#define BK7258_APB_WDT_CTRL   0x44800010U

#define BK7258_SCB_CCR     0xe000ed14U
#define BK7258_SCB_CLIDR   0xe000ed78U
#define BK7258_SCB_ICIALLU 0xe000ef50U

#define BK7258_SCB_CCR_ICACHE   BIT(17)
#define BK7258_SCB_CLIDR_ICACHE BIT(0)

/*
 * The flash image carries the BootROM boot marker at offset 0x100, which is
 * where the vectors of IRQ 48 and 49 are located. These interrupts can not
 * be used.
 */
#define BK7258_BOOT_MARKER_IRQ_FIRST 48
#define BK7258_BOOT_MARKER_IRQ_LAST  49

#define BK7258_IRQ_IDX_USES_MARKER(node_id, idx)                                                   \
	COND_CODE_1(DT_IRQ_HAS_IDX(node_id, idx),                                                  \
		    ((DT_IRQN_BY_IDX(node_id, idx) >= BK7258_BOOT_MARKER_IRQ_FIRST &&              \
		      DT_IRQN_BY_IDX(node_id, idx) <= BK7258_BOOT_MARKER_IRQ_LAST)),               \
		    (false))

#define BK7258_NODE_USES_MARKER_IRQ(node_id)                                                       \
	BK7258_IRQ_IDX_USES_MARKER(node_id, 0) || BK7258_IRQ_IDX_USES_MARKER(node_id, 1) ||

BUILD_ASSERT(!(DT_FOREACH_STATUS_OKAY_NODE(BK7258_NODE_USES_MARKER_IRQ) false),
	     "IRQ 48 and 49 are reserved for the BootROM boot marker");

#define BK7258_WDT_KEY1 (0x5aU << 16)
#define BK7258_WDT_KEY2 (0xa5U << 16)

void soc_early_init_hook(void)
{
	/* BootROM leaves the STAR-MC1 instruction cache extension disabled. */
	if ((sys_read32(BK7258_SCB_CLIDR) & BK7258_SCB_CLIDR_ICACHE) != 0U &&
	    (sys_read32(BK7258_SCB_CCR) & BK7258_SCB_CCR_ICACHE) == 0U) {
		barrier_dsync_fence_full();
		barrier_isync_fence_full();
		sys_write32(0U, BK7258_SCB_ICIALLU);
		barrier_dsync_fence_full();
		sys_set_bits(BK7258_SCB_CCR, BK7258_SCB_CCR_ICACHE);
		barrier_dsync_fence_full();
		barrier_isync_fence_full();
	}
}

void soc_reset_hook(void)
{
	sys_write32(BK7258_WDT_KEY1, BK7258_AON_WDT_CTRL);
	sys_write32(BK7258_WDT_KEY2, BK7258_AON_WDT_CTRL);

	sys_set_bits(BK7258_APB_WDT_GLOBAL, BIT(1));
	sys_write32(BK7258_WDT_KEY1, BK7258_APB_WDT_CTRL);
	sys_write32(BK7258_WDT_KEY2, BK7258_APB_WDT_CTRL);

	barrier_dsync_fence_full();
}
