/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Two-level cache backend for i.MX RT266x (Cortex-M85 L1 + CodaCache LLC).
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
 * LLC facts established on a MIMXRT2660-EVK (do not re-derive from the RM, which
 * contradicts itself on the by-address encoding):
 *   - By-address maintenance (MNTOP=6 entry, MNTOP=7 range) takes the flat
 *     cache-line address in CCUCMLR0 (address >> 7), with CCUCMLR1 = 0. The RM's
 *     CCUCMLR0[MNTWORD] field decomposition applies to the set/way ops, not
 *     these; using LLC_CCUCMLR0_MNTWORD() shifts by 21 and the op matches
 *     nothing (a silent no-op).
 *   - The address must be a cached alias (0x68000000 XSPI0 / 0x88000000 XSPI1);
 *     the direct windows are not in the LLC address map.
 *   - MNTOP=8 "flush set way range" walks way-major and is useless for a byte
 *     range; range flush is batched MNTOP=7 instead.
 *   - Whole-cache flush (MNTOP=4) needs the boot-derived way-valid mask
 *     (soc_llc_way_valid_mask()), not the 0xFF reset value, because the boot ROM
 *     partitions most ways as scratchpad SRAM.
 *   - There is no address-granular invalidate that skips the write-back:
 *     invd_range is a clean+invalidate here. See cache_data_invd_range().
 *
 * DMA buffers must not live in the cached aliases (eDMA bypasses the LLC, the
 * LLC line is 128 B while CONFIG_DCACHE_LINE_SIZE is the M85 L1's 32 B, and
 * __nocache does not reach the XSPI through a cached alias). That is a
 * board/application placement rule, documented in Kconfig.nxp_llc; this driver
 * only maintains coherency for buffers that are addressable through the alias.
 */

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

/* CodaCache geometry (SoC feature header ground truth). */
#define LLC_LINE_BYTES    128U

#define LLC_LINE_SHIFT    7U /* log2(128) */
#define LLC_SET_COUNT     ((uint32_t)FSL_FEATURE_LLC_SET_COUNT)
#define LLC_WAY_COUNT     ((uint32_t)FSL_FEATURE_LLC_WAY_COUNT)
#define LLC_TOTAL_LINES   (LLC_SET_COUNT * LLC_WAY_COUNT)

/*
 * Lines per locked MNTOP=7 operation. A range flush holds a spinlock for the
 * whole op, so the operation duration is the interrupt-off duration. Batching
 * bounds a single locked op instead of letting it scale with the whole range.
 * The cost probe (scratch/rt2660_llc_cost) measured the interrupt-off time for
 * this batch size; keep this in sync with DRIVER_BATCH_LINES there.
 */
#define LLC_RANGE_BATCH_LINES 64U

/*
 * Completion-poll budget. A whole-cache operation needs far more polls than the
 * SDK's default 1000-iteration budget, which returns a false timeout while the
 * op is still in flight; size the budget for the whole cache with margin.
 */
#define LLC_MAINTENANCE_POLL_LIMIT (LLC_TOTAL_LINES * 64U)

/*
 * The cached aliases the LLC is addressable through, and the offset from each
 * cached alias to its LLC-bypassing direct window. A direct-window address is
 * translated up to its cached alias for maintenance; an address in neither is
 * off-window and needs no LLC maintenance.
 */
#define LLC_ALIAS_OFFSET 0x08000000UL

struct llc_window {
	uint32_t direct_base;
	uint32_t cached_base;
	uint32_t size;
};

/* XSPI0: direct 0x60000000 / cached 0x68000000. XSPI1: 0x80000000 / 0x88000000.
 * Each XSPI AHB window is 128 MB.
 */
#define LLC_WINDOW_SIZE 0x08000000UL

static const struct llc_window llc_windows[] = {
	{0x60000000UL, 0x68000000UL, LLC_WINDOW_SIZE},
	{0x80000000UL, 0x88000000UL, LLC_WINDOW_SIZE},
};

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
	uint32_t mntop = (lines > 1U) ? LLC_MNTOP_FLUSH_ADDRESS_RANGE
				      : LLC_MNTOP_FLUSH_ENTRY_ADDRESS;
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
 * Issue the whole-cache flush (MNTOP=4) on the boot-derived way-valid mask.
 * Must be called with llc_lock held and the controller idle-waited by the
 * caller path; returns after the operation completes.
 */
static int llc_whole_cache_flush(LLC_Type *llc)
{
	uint32_t cmcr = llc->CCUCMCR & ~(LLC_CCUCMCR_ARRAYID_MASK | LLC_CCUCMCR_MNTOP_MASK);

	llc->CCUCMWVR = soc_llc_way_valid_mask();
	llc->CCUCMCR = cmcr | LLC_CCUCMCR_ARRAYID(0U) |
		       LLC_CCUCMCR_MNTOP(LLC_MNTOP_FLUSH_VALID_ENTRIES);

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

	LLC_Type *const llc = CMPT__LLC;
	k_spinlock_key_t key = k_spin_lock(&llc_lock);
	int ret = 0;

	if (total_lines >= LLC_TOTAL_LINES) {
		/* Whole cache is cheaper than walking the range. */
		ret = llc_wait_idle(llc);
		if (ret == 0) {
			ret = llc_whole_cache_flush(llc);
		}
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

/* Whole-cache LLC flush using the boot-derived way-valid mask. */
static int llc_flush_all(void)
{
	LLC_Type *const llc = CMPT__LLC;
	k_spinlock_key_t key = k_spin_lock(&llc_lock);
	int ret = llc_wait_idle(llc);

	if (ret == 0) {
		ret = llc_whole_cache_flush(llc);
	}

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
