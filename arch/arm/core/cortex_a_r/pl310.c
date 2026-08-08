/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * ARM L2C-310 (PL310) outer (L2) cache controller.
 *
 * Generic ARM IP shared across many platforms (Zynq-7000, i.MX6, Ux500,
 * RZA2M, ...). This driver programs the controller and provides the arch
 * outer-cache hooks (z_arm_outer_cache_*) so that, on a Cortex-A/R core, every
 * L1 maintenance op is paired with the matching L2 op for DMA correctness.
 *
 * It deliberately does NOT implement the cache_data_* API and does NOT select
 * CACHE_HAS_DRIVER: this is the outer level; the CPU-integrated L1 cache keeps
 * owning cache_data_*. It exposes no public API either - the maintenance ops
 * reach callers only through the arch outer-cache hooks, and the sole entry
 * point (z_pl310_early_enable) is declared in the arch-internal header
 * <cortex_a_r/outercache.h>. Platform-specific values (base, aux straps, RAM
 * latencies, way count) come from the arm,pl310-cache devicetree node.
 */

#define DT_DRV_COMPAT arm_pl310_cache

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <cortex_a_r/outercache.h>

#define PL310_LINE_SIZE 32U

/* Register offsets */
#define PL310_CTRL              0x100
#define PL310_AUX_CTRL          0x104
#define PL310_TAG_RAM_CTRL      0x108
#define PL310_DATA_RAM_CTRL     0x10C
#define PL310_INT_CLEAR         0x220
#define PL310_CACHE_SYNC        0x730
#define PL310_INV_LINE_PA       0x770
#define PL310_INV_WAY           0x77C
#define PL310_CLEAN_LINE_PA     0x7B0
#define PL310_CLEAN_INV_LINE_PA 0x7F0
#define PL310_CLEAN_INV_WAY     0x7FC

/* AUX_CTRL bit 22: Shared Attribute Override Enable */
#define PL310_AUX_SHARED_OVERRIDE BIT(22)

/*
 * AUX_CTRL associativity strap (bit 16): 0 = 8-way, 1 = 16-way. Used to derive
 * the way count when the devicetree does not pin arm,num-ways.
 */
#define PL310_AUX_ASSOCIATIVITY BIT(16)

#define PL310_NODE DT_DRV_INST(0)

#define PL310_BASE    ((uintptr_t)DT_REG_ADDR(PL310_NODE))
#define PL310_HAS_TAG_LATENCY  DT_INST_NODE_HAS_PROP(0, tag_ram_latency)
#define PL310_HAS_DATA_LATENCY DT_INST_NODE_HAS_PROP(0, data_ram_latency)

static inline void pl310_sync(void)
{
	sys_write32(0, PL310_BASE + PL310_CACHE_SYNC);
}

static inline uint32_t pl310_num_ways(uint32_t aux)
{
#if DT_INST_NODE_HAS_PROP(0, arm_num_ways)
	ARG_UNUSED(aux);
	return DT_INST_PROP(0, arm_num_ways);
#else
	return (aux & PL310_AUX_ASSOCIATIVITY) ? 16U : 8U;
#endif
}

void z_pl310_early_enable(void)
{
	uint32_t aux;
	uint32_t way_mask;

	sys_write32(0, PL310_BASE + PL310_CTRL);

#if PL310_HAS_TAG_LATENCY
	sys_write32(DT_INST_PROP(0, tag_ram_latency),
		    PL310_BASE + PL310_TAG_RAM_CTRL);
#endif
#if PL310_HAS_DATA_LATENCY
	sys_write32(DT_INST_PROP(0, data_ram_latency),
		    PL310_BASE + PL310_DATA_RAM_CTRL);
#endif

	/*
	 * Preserve the reset value of AUX_CTRL (correct way-size and
	 * associativity straps) and only force the shared-override bit when the
	 * platform requests it. On Zynq-7000 this bit is required: without it
	 * the PL310 treats Normal Shareable Cacheable transactions as
	 * non-cacheable and the core hangs at MMU enable.
	 */
	aux = sys_read32(PL310_BASE + PL310_AUX_CTRL);
#if DT_INST_PROP(0, arm_shared_override)
	aux |= PL310_AUX_SHARED_OVERRIDE;
#endif
	sys_write32(aux, PL310_BASE + PL310_AUX_CTRL);

	way_mask = BIT(pl310_num_ways(aux)) - 1U;

	sys_write32(way_mask, PL310_BASE + PL310_INV_WAY);
	while (sys_read32(PL310_BASE + PL310_INV_WAY) & way_mask) {
	}
	pl310_sync();

	sys_write32(0x1FFU, PL310_BASE + PL310_INT_CLEAR);
	sys_write32(1U, PL310_BASE + PL310_CTRL);
	pl310_sync();

	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

static void pl310_range(uintptr_t reg, void *addr, size_t size)
{
	uintptr_t a = (uintptr_t)addr & ~(PL310_LINE_SIZE - 1U);
	uintptr_t end = (uintptr_t)addr + size;

	/*
	 * TODO: for large ranges Linux's l2x0 switches to clean/invalidate
	 * by-way past a threshold; iterating PA lines here is correct but slow
	 * over big DMA buffers.
	 */
	for (; a < end; a += PL310_LINE_SIZE) {
		sys_write32((uint32_t)a, PL310_BASE + reg);
	}
	pl310_sync();
}

void z_arm_outer_cache_flush_range(void *addr, size_t size)
{
	pl310_range(PL310_CLEAN_LINE_PA, addr, size);
}

void z_arm_outer_cache_invd_range(void *addr, size_t size)
{
	pl310_range(PL310_INV_LINE_PA, addr, size);
}

void z_arm_outer_cache_flush_and_invd_range(void *addr, size_t size)
{
	pl310_range(PL310_CLEAN_INV_LINE_PA, addr, size);
}
