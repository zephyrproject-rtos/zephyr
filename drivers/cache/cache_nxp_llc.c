/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Two-level cache backend: a Cortex-M L1 plus an NXP CodaCache last-level
 * cache (LLC).
 *
 * Zephyr's cache API has no level parameter and CACHE_TYPE selects one backend
 * for the whole system. Selecting EXTERNAL_CACHE to get an LLC backend removes
 * arch/arm/core/cortex_m/cache.c (the L1 backend, gated on ARCH_CACHE) from the
 * build, so this single file must serve BOTH levels: the L1 through the CMSIS
 * SCB helpers, and the CodaCache LLC through direct maintenance-register
 * programming. drivers/cache/cache_andes.c is the upstream precedent for one
 * backend driving two levels.
 *
 * Direction order (cache_andes.c pattern):
 *   - flush (push out): inner first  -> clean L1, then flush LLC
 *   - invalidate (pull in): outer first -> LLC, then invalidate L1
 *
 * The LLC geometry and the memory windows it fronts come from the devicetree
 * (see dts/bindings/cache/nxp,llc.yaml); the set and way counts come from the
 * SoC feature header.
 *
 * LLC facts established on a MIMXRT2660-EVK (do not re-derive from the RM, which
 * contradicts itself on the by-address encoding):
 *   - By-address maintenance (MNTOP=6 entry, MNTOP=7 range) takes the flat
 *     cache-line address in CCUCMLR0, with CCUCMLR1 = 0. The RM's
 *     CCUCMLR0[MNTWORD] field decomposition applies to the set/way ops, not
 *     these; using LLC_CCUCMLR0_MNTWORD() shifts by 21 and the op matches
 *     nothing (a silent no-op).
 *   - The address must be a cached alias; the direct windows bypass the LLC and
 *     are not in its address map, so a direct-window address is translated up
 *     to its alias before a maintenance op is issued.
 *   - MNTOP=8 "flush set way range" walks way-major and is useless for a byte
 *     range; range flush is batched MNTOP=7 instead.
 *   - Whole-cache flush (MNTOP=4) needs the way-valid mask derived from the live
 *     scratchpad partition (llc_way_valid_mask()), not the CCUCMWVR 0xFF reset
 *     value, because the boot ROM partitions most ways as scratchpad SRAM.
 *   - There is no address-granular invalidate that skips the write-back:
 *     invd_range is a clean+invalidate here. See cache_data_invd_range().
 *
 * DMA buffers must not live in the cached aliases, and this driver cannot make
 * them safe there:
 *   - masters such as eDMA reach the memory behind the LLC, not through it, so
 *     they neither hit nor allocate in it;
 *   - the LLC line is wider than CONFIG_DCACHE_LINE_SIZE, which reports the CPU
 *     L1 line, so a buffer that is only L1-line aligned shares an LLC line with
 *     its neighbours and a flush of that neighbour overwrites what the master
 *     deposited;
 *   - invalidate-before-read writes the line back first (see above), over the
 *     master's data.
 * Keep DMA buffers in on-chip SRAM, TCM, or the direct windows. __nocache marks
 * memory non-cacheable to the CPU only; over a cached alias it does not reach
 * the external memory at all and must not be used there. That is a
 * board/application placement rule rather than something the API can express.
 */

#define DT_DRV_COMPAT nxp_llc

#include <zephyr/cache.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <cmsis_core.h>
#include <soc.h>

#include <fsl_common.h>

/*
 * CodaCache maintenance opcodes (CCUCMCR[MNTOP]). These mirror the SDK
 * fsl_llc.h llc_maintenance_operation_t values, redefined here because this
 * driver deliberately does not link fsl_llc.c (its by-address encoding is wrong
 * on this silicon, and it has a non-static function body that multiply-defines
 * if its header is pulled into a second translation unit).
 */
#define LLC_MNTOP_FLUSH_VALID_ENTRIES 4U
#define LLC_MNTOP_FLUSH_ENTRY_ADDRESS 6U
#define LLC_MNTOP_FLUSH_ADDRESS_RANGE 7U

/* Controller register block. */
#define LLC_REGS ((LLC_Type *)DT_INST_REG_ADDR(0))

/*
 * CodaCache geometry. The line size is a per-instantiation parameter and is
 * independent of the CPU L1 line size, so it comes from the devicetree; the set
 * and way counts come from the SoC feature header.
 */
#define LLC_LINE_BYTES  ((uint32_t)DT_INST_PROP(0, cache_line_size))
#define LLC_LINE_SHIFT  LOG2(LLC_LINE_BYTES)
#define LLC_SET_COUNT   ((uint32_t)FSL_FEATURE_LLC_SET_COUNT)
#define LLC_WAY_COUNT   ((uint32_t)FSL_FEATURE_LLC_WAY_COUNT)
#define LLC_TOTAL_LINES (LLC_SET_COUNT * LLC_WAY_COUNT)

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "only one NXP LLC instance is supported");
BUILD_ASSERT((LLC_LINE_BYTES > 0U) && IS_POWER_OF_TWO(LLC_LINE_BYTES),
	     "cache-line-size must be a non-zero power of two");

/*
 * Lines per MNTOP=7 operation. llc_flush_range() holds the lock across the
 * entire range, so batching bounds each operation's duration, but not the total
 * time spent with interrupts disabled. The batch size was chosen from its
 * measured worst-case operation time.
 */
#define LLC_RANGE_BATCH_LINES 64U

/*
 * Completion-poll budget. A whole-cache operation needs far more polls than the
 * SDK's default 1000-iteration budget, which returns a false timeout while the
 * op is still in flight; size the budget for the whole cache with margin.
 */
#define LLC_MAINTENANCE_POLL_LIMIT (LLC_TOTAL_LINES * 64U)

struct llc_window {
	uint32_t direct_base;
	uint32_t cached_base;
	uint32_t size;
};

/*
 * The memory windows this LLC fronts, one entry per nxp,cached-memories node.
 * Index 1 of such a node's reg is its LLC-bypassing direct window; the cached
 * alias the maintenance registers accept is that base plus the alias offset.
 */
#define LLC_ALIAS_OFFSET ((uint32_t)DT_INST_PROP(0, nxp_cached_alias_offset))

#define LLC_WINDOW_DIRECT_BASE(node_id, prop, idx)                                                 \
	((uint32_t)DT_REG_ADDR_BY_IDX(DT_PHANDLE_BY_IDX(node_id, prop, idx), 1))
#define LLC_WINDOW_SIZE(node_id, prop, idx)                                                        \
	((uint32_t)DT_REG_SIZE_BY_IDX(DT_PHANDLE_BY_IDX(node_id, prop, idx), 1))

#define LLC_WINDOW_ENTRY(node_id, prop, idx)                                                       \
	{                                                                                          \
		.direct_base = LLC_WINDOW_DIRECT_BASE(node_id, prop, idx),                         \
		.cached_base = LLC_WINDOW_DIRECT_BASE(node_id, prop, idx) + LLC_ALIAS_OFFSET,      \
		.size = LLC_WINDOW_SIZE(node_id, prop, idx),                                       \
	},

static const struct llc_window llc_windows[] = {
	DT_INST_FOREACH_PROP_ELEM(0, nxp_cached_memories, LLC_WINDOW_ENTRY)};

static struct k_spinlock llc_lock;

/*
 * Map a CPU address to the cached-alias line address the LLC maintenance
 * registers expect. Returns true and writes *cached_addr when the address is in
 * an LLC window (either the cached alias or the direct window, which is
 * translated up). Returns false for an off-window address, which needs no LLC
 * maintenance.
 */
static bool llc_to_cached_alias(uintptr_t addr, uint32_t *cached_addr)
{
	for (size_t i = 0U; i < ARRAY_SIZE(llc_windows); i++) {
		const struct llc_window *w = &llc_windows[i];

		if ((addr >= w->cached_base) && (addr < (w->cached_base + w->size))) {
			*cached_addr = (uint32_t)addr;
			return true;
		}
		if ((addr >= w->direct_base) && (addr < (w->direct_base + w->size))) {
			*cached_addr = (uint32_t)addr + (uint32_t)LLC_ALIAS_OFFSET;
			return true;
		}
	}

	return false;
}

/* Wait for the current maintenance operation to finish. Bounded so a wedged
 * controller returns an error instead of hanging the caller forever.
 */
static int llc_wait_idle(LLC_Type *llc)
{
	uint32_t spins = LLC_MAINTENANCE_POLL_LIMIT;

	while ((llc->CCUCMAR & LLC_CCUCMAR_MNTOPACTV_MASK) != 0U) {
		if (--spins == 0U) {
			return -EIO;
		}
	}

	return 0;
}

/*
 * Issue one by-address maintenance op on a cached-alias line address. mntop is
 * kLLC_MaintenanceFlushEntryAddress (single line) or
 * kLLC_MaintenanceFlushAddressRange (lines > 1). Must be called with llc_lock
 * held.
 */
static int llc_maintenance_range_op(LLC_Type *llc, uint32_t cached_addr, uint32_t lines)
{
	uint32_t mntop =
		(lines > 1U) ? LLC_MNTOP_FLUSH_ADDRESS_RANGE : LLC_MNTOP_FLUSH_ENTRY_ADDRESS;
	int ret = llc_wait_idle(llc);

	if (ret != 0) {
		return ret;
	}

	/* Flat cache-line address; NOT LLC_CCUCMLR0_MNTWORD() (see file header). */
	llc->CCUCMLR0 = cached_addr >> LLC_LINE_SHIFT;
	llc->CCUCMLR1 = 0U;

	if (mntop == LLC_MNTOP_FLUSH_ADDRESS_RANGE) {
		llc->CCUCMLR2 = (llc->CCUCMLR2 & ~LLC_CCUCMLR2_MNTRANGE_MASK) |
				LLC_CCUCMLR2_MNTRANGE(lines - 1U);
	}

	llc->CCUCMCR = (llc->CCUCMCR & ~(LLC_CCUCMCR_ARRAYID_MASK | LLC_CCUCMCR_MNTOP_MASK)) |
		       LLC_CCUCMCR_ARRAYID(0U) | LLC_CCUCMCR_MNTOP(mntop);

	return llc_wait_idle(llc);
}

/*
 * Way-valid mask for whole-cache maintenance. The boot stage partitions some
 * ways as scratchpad SRAM (CCUSPCR0[NUMSCPADWAYS]); only the remaining ways are
 * cache. A whole-cache operation must target exactly those cache ways -- the
 * CCUCMWVR 0xFF reset value would operate on scratchpad SRAM instead. The mask
 * is derived from the live partition, so it tracks whatever the boot stage
 * configured. Returns a WAYVALID bitmask (bit i set => way i is cache).
 */
static uint32_t llc_way_valid_mask(const LLC_Type *llc)
{
	uint32_t scpad_ways = 0U;

	/*
	 * The scratchpad occupies the low-numbered ways when enabled; the cache
	 * ways are the remaining high-numbered ones. NUMSCPADWAYS is zero-based
	 * (0 => 1 way), so the scratchpad way count is NUMSCPADWAYS + 1 when
	 * SCPADEN is set. Any way not claimed by the scratchpad is cache.
	 */
	if ((llc->CCUSPCR0 & LLC_CCUSPCR0_SCPADEN_MASK) != 0U) {
		scpad_ways = ((llc->CCUSPCR0 & LLC_CCUSPCR0_NUMSCPADWAYS_MASK) >>
			      LLC_CCUSPCR0_NUMSCPADWAYS_SHIFT) +
			     1U;
	}

	if (scpad_ways >= LLC_WAY_COUNT) {
		return 0U;
	}

	return GENMASK(LLC_WAY_COUNT - 1U, scpad_ways) & LLC_CCUCMWVR_WAYVALID_MASK;
}

/*
 * Issue the whole-cache flush (MNTOP=4) on the ways the scratchpad partition
 * leaves as cache. Must be called with llc_lock held; returns after the
 * operation completes.
 */
static int llc_whole_cache_flush(LLC_Type *llc)
{
	uint32_t cmcr;
	int ret = llc_wait_idle(llc);

	if (ret != 0) {
		return ret;
	}

	cmcr = llc->CCUCMCR & ~(LLC_CCUCMCR_ARRAYID_MASK | LLC_CCUCMCR_MNTOP_MASK);

	llc->CCUCMWVR = llc_way_valid_mask(llc);
	llc->CCUCMCR =
		cmcr | LLC_CCUCMCR_ARRAYID(0U) | LLC_CCUCMCR_MNTOP(LLC_MNTOP_FLUSH_VALID_ENTRIES);

	return llc_wait_idle(llc);
}

/*
 * Flush an LLC address range. addr/size are rounded to 128-byte lines. A range
 * at or beyond whole-cache capacity falls back to the whole-cache flush. An
 * off-window range is a successful no-op.
 */
static int llc_flush_range(uintptr_t addr, size_t size)
{
	uint32_t cached_start;
	uintptr_t start;
	uintptr_t end;
	uint32_t total_lines;

	if (size == 0U) {
		return 0;
	}

	if (!llc_to_cached_alias(addr, &cached_start)) {
		return 0;
	}

	/* Round start down and end up to whole cache lines. */
	start = (uintptr_t)cached_start & ~((uintptr_t)LLC_LINE_BYTES - 1U);
	end = ((uintptr_t)cached_start + size + LLC_LINE_BYTES - 1U) &
	      ~((uintptr_t)LLC_LINE_BYTES - 1U);
	total_lines = (uint32_t)((end - start) >> LLC_LINE_SHIFT);

	LLC_Type *const llc = LLC_REGS;
	k_spinlock_key_t key = k_spin_lock(&llc_lock);
	int ret = 0;

	if (total_lines >= LLC_TOTAL_LINES) {
		/* Whole cache is cheaper than walking the range. */
		ret = llc_whole_cache_flush(llc);
	} else {
		uint32_t line_addr = (uint32_t)start;
		uint32_t remaining = total_lines;

		while ((remaining != 0U) && (ret == 0)) {
			uint32_t batch = MIN(remaining, LLC_RANGE_BATCH_LINES);

			ret = llc_maintenance_range_op(llc, line_addr, batch);
			line_addr += batch * LLC_LINE_BYTES;
			remaining -= batch;
		}
	}

	__DSB();
	k_spin_unlock(&llc_lock, key);

	return ret;
}

/* Whole-cache LLC flush over the ways the scratchpad partition leaves as cache. */
static int llc_flush_all(void)
{
	LLC_Type *const llc = LLC_REGS;
	k_spinlock_key_t key = k_spin_lock(&llc_lock);
	int ret = llc_whole_cache_flush(llc);

	__DSB();
	k_spin_unlock(&llc_lock, key);

	return ret;
}

/* ---- Data cache: enable / disable ---- */

void cache_data_enable(void)
{
	SCB_EnableDCache();
}

void cache_data_disable(void)
{
	SCB_DisableDCache();
	(void)llc_flush_all();
}

/* ---- Data cache: whole-cache ops ---- */

int cache_data_flush_all(void)
{
	/* Inner first: clean L1 out to the LLC, then flush the LLC to memory. */
	SCB_CleanDCache();
	__DSB();

	return llc_flush_all();
}

int cache_data_invd_all(void)
{
	/*
	 * No discard-only whole-cache invalidate exists on the LLC either; a
	 * clean+invalidate is the honourable whole-cache invalidate here. Outer
	 * first, then invalidate L1.
	 */
	int ret = llc_flush_all();

	SCB_InvalidateDCache();
	__DSB();

	return ret;
}

int cache_data_flush_and_invd_all(void)
{
	/* Clean+invalidate L1, then flush the LLC (also clean+invalidate). */
	SCB_CleanInvalidateDCache();
	__DSB();

	return llc_flush_all();
}

/* ---- Data cache: range ops ---- */

int cache_data_flush_range(void *addr, size_t size)
{
	if (size == 0U) {
		return 0;
	}

	/* Inner first: clean L1, then flush the LLC. */
	SCB_CleanDCache_by_Addr(addr, (int32_t)size);
	__DSB();

	return llc_flush_range((uintptr_t)addr, size);
}

int cache_data_invd_range(void *addr, size_t size)
{
	/*
	 * WEAKER GUARANTEE ON THIS IP: the CodaCache maintenance set has no
	 * address-granular invalidate that skips the write-back, so this is a
	 * clean+invalidate. A dirty LLC line inside the range is written BACK to
	 * memory rather than discarded. Callers needing discard-before-read for a
	 * DMA buffer must keep that buffer out of the cached aliases (see
	 * Kconfig.nxp_llc); for such off-window buffers this is a no-op at the LLC
	 * and a plain L1 invalidate, which is correct.
	 */
	int ret;

	if (size == 0U) {
		return 0;
	}

	/* Outer first: LLC (clean+invalidate), then invalidate L1. */
	ret = llc_flush_range((uintptr_t)addr, size);
	SCB_InvalidateDCache_by_Addr(addr, (int32_t)size);
	__DSB();

	return ret;
}

int cache_data_flush_and_invd_range(void *addr, size_t size)
{
	if (size == 0U) {
		return 0;
	}

	/* Clean+invalidate L1, then flush the LLC (also clean+invalidate). */
	SCB_CleanInvalidateDCache_by_Addr(addr, (int32_t)size);
	__DSB();

	return llc_flush_range((uintptr_t)addr, size);
}

/* ---- Instruction cache ---- */

void cache_instr_enable(void)
{
	SCB_EnableICache();
}

void cache_instr_disable(void)
{
	SCB_DisableICache();
}

int cache_instr_flush_all(void)
{
	return -ENOTSUP;
}

int cache_instr_invd_all(void)
{
	/*
	 * The LLC is unified but instruction fetches from the XSPI windows go
	 * through it; a whole-cache flush publishes any code written through the
	 * data path, then the L1 I-cache is invalidated so the CPU refetches.
	 */
	int ret = llc_flush_all();

	SCB_InvalidateICache();
	__DSB();
	__ISB();

	return ret;
}

int cache_instr_flush_and_invd_all(void)
{
	return cache_instr_invd_all();
}

int cache_instr_flush_range(void *addr, size_t size)
{
	ARG_UNUSED(addr);
	ARG_UNUSED(size);

	return -ENOTSUP;
}

int cache_instr_invd_range(void *addr, size_t size)
{
	int ret;

	if (size == 0U) {
		return 0;
	}

	/* Publish through the LLC, then drop the whole L1 I-cache (CMSIS has no
	 * by-address I-cache invalidate).
	 */
	ret = llc_flush_range((uintptr_t)addr, size);
	SCB_InvalidateICache();
	__DSB();
	__ISB();

	return ret;
}

int cache_instr_flush_and_invd_range(void *addr, size_t size)
{
	return cache_instr_invd_range(addr, size);
}
