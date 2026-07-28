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
 * <cortex_a_r/outer_cache.h>. Platform-specific values (base, aux straps, RAM
 * latencies, way count) come from the arm,pl310-cache devicetree node.
 */

#define DT_DRV_COMPAT arm_pl310_cache

#include <stdbool.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include <kernel_arch_interface.h>

#include <zephyr/arch/arm/cortex_a_r/cpu.h>
#include <cortex_a_r/outer_cache.h>

LOG_MODULE_REGISTER(pl310, CONFIG_LOG_DEFAULT_LEVEL);

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
#define PL310_CLEAN_WAY         0x7BC
#define PL310_CLEAN_INV_LINE_PA 0x7F0
#define PL310_CLEAN_INV_WAY     0x7FC

/* CTRL bit 0: L2 cache enable */
#define PL310_CTRL_ENABLE BIT(0)

/* All nine PL310 interrupt sources (bits 8:0), write-1-to-clear */
#define PL310_INT_ALL GENMASK(8, 0)

/* AUX_CTRL bit 22: Shared Attribute Override Enable */
#define PL310_AUX_SHARED_OVERRIDE BIT(22)

/*
 * AUX_CTRL associativity strap (bit 16): 0 = 8-way, 1 = 16-way. Used to derive
 * the way count when the devicetree does not pin arm,num-ways.
 */
#define PL310_AUX_ASSOCIATIVITY BIT(16)

#define PL310_NODE DT_DRV_INST(0)

#define PL310_BASE             ((uintptr_t)DT_REG_ADDR(PL310_NODE))
#define PL310_HAS_TAG_LATENCY  DT_INST_NODE_HAS_PROP(0, arm_tag_latency)
#define PL310_HAS_DATA_LATENCY DT_INST_NODE_HAS_PROP(0, arm_data_latency)

/*
 * Latency Control register field packing (Tag and Data RAM Latency Control
 * registers share the same layout): SETUP[2:0], RD[6:4], WR[10:8], each
 * holding (cycles - 1). Matches Linux's cache-l2x0.c L310_LATENCY_CTRL_*().
 */
#define PL310_LATENCY_SETUP_SHIFT 0
#define PL310_LATENCY_RD_SHIFT    4
#define PL310_LATENCY_WR_SHIFT    8

#define PL310_LATENCY_REG(rd, wr, setup)                                                           \
	((((rd) - 1U) << PL310_LATENCY_RD_SHIFT) | (((wr) - 1U) << PL310_LATENCY_WR_SHIFT) |       \
	 (((setup) - 1U) << PL310_LATENCY_SETUP_SHIFT))

/*
 * Latched state of PL310_CTRL.enable as seen on entry to
 * z_pl310_early_enable(), reported later by pl310_check_firmware_handoff().
 *
 * __noinit is load-bearing, not stylistic: z_pl310_early_enable() is called
 * from soc_reset_hook() (reset.S), which runs before z_prep_c() reaches
 * arch_bss_zero(). A plain static would live in .bss and be zeroed after
 * being written, so the POST_KERNEL check would always see false.
 */
static bool pl310_was_enabled_at_entry __noinit;

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

static inline uint32_t pl310_way_mask(void)
{
	uint32_t aux = sys_read32(PL310_BASE + PL310_AUX_CTRL);

	return BIT(pl310_num_ways(aux)) - 1U;
}

/* Common tail for a by-way maintenance op: issue, poll to completion, sync. */
static void pl310_by_way(uintptr_t reg)
{
	uint32_t way_mask = pl310_way_mask();

	sys_write32(way_mask, PL310_BASE + reg);
	while (sys_read32(PL310_BASE + reg) & way_mask) {
	}
	pl310_sync();
}

/*
 * Firmware-handoff contract: the boot firmware (u-boot / JTAG loader) is
 * assumed to hand off with the PL310 either disabled, or enabled with no
 * dirty lines belonging to memory Zephyr will reuse (e.g. it quiesced DMA
 * and cleaned its own working set before jumping to Zephyr). This function
 * disables and invalidates the controller unconditionally, which discards
 * any dirty data present at entry - if that assumption does not hold for a
 * given boot path, data belonging to the previous stage is silently lost.
 *
 * A violation cannot be fully prevented here, only reported: the PL310
 * exposes no way to tell dirty lines from clean ones, so "was enabled at
 * entry" is the only observable proxy, and it is not by itself an error
 * (firmware that cleaned up before handoff is compliant). Refusing to boot
 * on it would break those compliant paths. What is done instead: the entry
 * state is latched below and reported by pl310_check_firmware_handoff() once
 * logging is up, so a violating boot path is visible rather than silent.
 *
 * SMP: this reset/enable sequence touches shared PL310 state and races if
 * run from more than one core, or from a secondary core after the primary
 * has already started caching through it. It must run exactly once, from
 * the boot core, before any secondary core is released - callers do not
 * enforce this by construction (soc_reset_hook() runs on every core), so the
 * guard below drops out on any core whose affinity level 0 is non-zero. That
 * makes "the boot core is affinity 0" a requirement on any platform
 * instantiating this node, not a property of the IP.
 */
void z_pl310_early_enable(void)
{
	uint32_t aux;

#ifdef CONFIG_SMP
	if (MPIDR_TO_CORE(GET_MPIDR()) != 0U) {
		return;
	}
#endif

	pl310_was_enabled_at_entry =
		(sys_read32(PL310_BASE + PL310_CTRL) & PL310_CTRL_ENABLE) != 0U;

	sys_write32(0, PL310_BASE + PL310_CTRL);

#if PL310_HAS_TAG_LATENCY
	sys_write32(PL310_LATENCY_REG(DT_INST_PROP_BY_IDX(0, arm_tag_latency, 0),
				      DT_INST_PROP_BY_IDX(0, arm_tag_latency, 1),
				      DT_INST_PROP_BY_IDX(0, arm_tag_latency, 2)),
		    PL310_BASE + PL310_TAG_RAM_CTRL);
#endif
#if PL310_HAS_DATA_LATENCY
	sys_write32(PL310_LATENCY_REG(DT_INST_PROP_BY_IDX(0, arm_data_latency, 0),
				      DT_INST_PROP_BY_IDX(0, arm_data_latency, 1),
				      DT_INST_PROP_BY_IDX(0, arm_data_latency, 2)),
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

	pl310_by_way(PL310_INV_WAY);

	sys_write32(PL310_INT_ALL, PL310_BASE + PL310_INT_CLEAR);
	sys_write32(PL310_CTRL_ENABLE, PL310_BASE + PL310_CTRL);
	pl310_sync();

	barrier_dsync_fence_full();
	barrier_isync_fence_full();
}

/*
 * End of the window over which a single translation stays valid, clamped to
 * the end of the caller's range: one MMU page under an MMU, the whole range
 * without one (virt == phys by construction, so one translation covers it).
 */
static inline uintptr_t pl310_chunk_end(uintptr_t va, uintptr_t end)
{
#ifdef CONFIG_ARM_AARCH32_MMU
	/* va + 1 so that a page-aligned va advances a full page, not zero. */
	return MIN(ROUND_UP(va + 1U, CONFIG_MMU_PAGE_SIZE), end);
#else
	ARG_UNUSED(va);
	return end;
#endif
}

/*
 * PL310_*_LINE_PA registers take a physical address, so each line's VA has to
 * be translated. arch_page_phys_get() walks the page tables under
 * arch_irq_lock(), which makes it far too expensive to call per line: at a
 * 32-byte line and a 4 kB page that would be 128 table walks and 128 irq-lock
 * round trips for every page of a DMA buffer. Translate once per page instead -
 * within a page VA and PA advance together, so the per-line PA is the page's PA
 * plus the same offset.
 */
static inline bool pl310_chunk_pa(uintptr_t va, uintptr_t *pa)
{
#ifdef CONFIG_ARM_AARCH32_MMU
	int rc = arch_page_phys_get((void *)va, pa);

	/*
	 * A buffer handed to cache maintenance is expected to be mapped. If it
	 * is not, there is no correct action left: writing the untranslated VA
	 * would maintain an unrelated physical line, and skipping drops the
	 * line from a flush, leaving dirty data stranded in L2. Skip as the
	 * least-destructive fallback, but assert - this is a caller bug.
	 */
	__ASSERT(rc == 0, "PL310: unmapped VA %p in cache maintenance range", (void *)va);

	return rc == 0;
#else
	*pa = va;
	return true;
#endif
}

static void pl310_range(uintptr_t reg, void *addr, size_t size)
{
	uintptr_t va = (uintptr_t)addr & ~(PL310_LINE_SIZE - 1U);
	uintptr_t end = (uintptr_t)addr + size;

	/*
	 * TODO: for large ranges Linux's l2x0 switches to clean/invalidate
	 * by-way past a threshold; iterating PA lines here is correct but slow
	 * over big DMA buffers.
	 */
	while (va < end) {
		uintptr_t chunk_end = pl310_chunk_end(va, end);
		uintptr_t pa;

		if (!pl310_chunk_pa(va, &pa)) {
			va = chunk_end;
			continue;
		}

		for (; va < chunk_end; va += PL310_LINE_SIZE, pa += PL310_LINE_SIZE) {
			sys_write32((uint32_t)pa, PL310_BASE + reg);
		}
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

void z_arm_outer_cache_flush_all(void)
{
	pl310_by_way(PL310_CLEAN_WAY);
}

void z_arm_outer_cache_invd_all(void)
{
	pl310_by_way(PL310_INV_WAY);
}

void z_arm_outer_cache_flush_and_invd_all(void)
{
	pl310_by_way(PL310_CLEAN_INV_WAY);
}

/*
 * Report a possible firmware-handoff contract violation. z_pl310_early_enable()
 * runs long before the console exists, so the entry state is latched there and
 * only reported here, at POST_KERNEL, where logging is initialised.
 *
 * Being enabled at entry is not proof of data loss - firmware that cleaned its
 * working set before handoff is compliant and will still trip this. It is the
 * only thing the hardware makes observable (there is no cheap per-line dirty
 * status), so this warns rather than failing the boot.
 */
static int pl310_check_firmware_handoff(void)
{
	if (pl310_was_enabled_at_entry) {
		LOG_WRN("PL310 was already enabled at handoff; the reset sequence "
			"invalidated it, discarding any dirty lines. Boot firmware must "
			"clean the L2 before jumping to Zephyr.");
	}

	return 0;
}

SYS_INIT(pl310_check_firmware_handoff, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
