/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __INTEL_ADSP_CPU_INIT_H
#define __INTEL_ADSP_CPU_INIT_H

#include <xtensa/config/core-isa.h>

#define CxL1CCAP (*(volatile uint32_t *)0x9F080080)
#define CxL1CCFG (*(volatile uint32_t *)0x9F080084)
#define CxL1PCFG (*(volatile uint32_t *)0x9F080088)

/* "Data/Instruction Cache Memory Way Count" fields */
#define CxL1CCAP_DCMWC ((CxL1CCAP >> 16) & 7)
#define CxL1CCAP_ICMWC ((CxL1CCAP >> 20) & 7)

/* Utilities to generate an unwrapped code sequence to set the RPO TLB
 * registers.  Pass the 8 region attributes as arguments, e.g.:
 *
 *     SET_RPO_TLB(2, 15, 15, 15, 2, 4, 15, 15);
 *
 * Note that cAVS 1.5 has the "translation" option that we don't use,
 * but still need to put an identity mapping in the high bits.  Also
 * per spec changing the current code region requires that WITLB be
 * followed by an ISYNC and that both instructions live in the same
 * cache line (two 3-byte instructions fit in an 8-byte aligned
 * region, so that's guaranteed not to cross a caceh line boundary).
 */
#define SET_ONE_TLB(region, att) do {					\
	uint32_t addr = region * 0x20000000U, attr = att;		\
	if (XCHAL_HAVE_XLT_CACHEATTR) {					\
		attr |= addr; /* RPO with translation */		\
	}								\
	if (region != (L2_SRAM_BASE >> 29)) {				\
		__asm__ volatile("wdtlb %0, %1; witlb %0, %1"		\
				 :: "r"(attr), "r"(addr));		\
	} else {							\
		__asm__ volatile("wdtlb %0, %1"				\
				 :: "r"(attr), "r"(addr));		\
		__asm__ volatile("j 1f; .align 8; 1:");			\
		__asm__ volatile("witlb %0, %1; isync"			\
				 :: "r"(attr), "r"(addr));		\
	}								\
} while (0)

#define SET_RPO_TLB(...) do {				\
	FOR_EACH_IDX(SET_ONE_TLB, (;), __VA_ARGS__);	\
} while (0)

/* Low-level CPU initialization.  Call this immediately after entering
 * C code to initialize the cache, protection and synchronization
 * features.
 */
static ALWAYS_INLINE void cpu_early_init(void)
{
	uint32_t reg;

	if (IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V25)) {
		/* First, on cAVS 2.5 we need to power the cache SRAM banks
		 * on!  Write a bit for each cache way in the bottom half of
		 * the L1CCFG register and poll the top half for them to turn
		 * on.
		 */
		uint32_t dmask = BIT(CxL1CCAP_DCMWC) - 1;
		uint32_t imask = BIT(CxL1CCAP_ICMWC) - 1;
		uint32_t waymask = (imask << 8) | dmask;

		CxL1CCFG = waymask;
		while (((CxL1CCFG >> 16) & waymask) != waymask) {
		}

		/* Prefetcher also power gates, same interface */
		CxL1PCFG = 1;
		while ((CxL1PCFG & 0x10000) == 0) {
		}
	}

	/* Now set up the Xtensa CPU to enable the cache logic.  The
	 * details of the fields are somewhat complicated, but per the
	 * ISA ref: "Turning on caches at power-up usually consists of
	 * writing a constant with bits[31:8] all 1’s to MEMCTL.".
	 * Also set bit 0 to enable the LOOP extension instruction
	 * fetch buffer.
	 */
#ifdef XCHAL_HAVE_ICACHE_DYN_ENABLE
	reg = 0xffffff01;
	__asm__ volatile("wsr %0, MEMCTL; rsync" :: "r"(reg));
#endif

	/* Likewise enable prefetching.  Sadly these values are not
	 * architecturally defined by Xtensa (they're just documented
	 * as priority hints), so this constant is just copied from
	 * SOF for now.  If we care about prefetch priority tuning
	 * we're supposed to ask Cadence I guess.
	 */
	reg = IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V25) ? 0x1038 : 0;
	__asm__ volatile("wsr %0, PREFCTL; rsync" :: "r"(reg));

	/* Finally we need to enable the cache in the Region
	 * Protection Option "TLB" entries.  The hardware defaults
	 * have this set to RW/uncached (2) everywhere.  We want
	 * writeback caching (4) in the sixth mapping (the second of
	 * two RAM mappings) and to mark all unused regions
	 * inaccessible (15) for safety.  Note that there is a HAL
	 * routine that does this (by emulating the older "cacheattr"
	 * hardware register), but it generates significantly larger
	 * code.
	 */
	SET_RPO_TLB(2, 15, 15, 15, 2, 4, 15, 15);

	/* Initialize ATOMCTL: Hardware defaults for S32C1I use
	 * "internal" operations, meaning they are atomic only WRT the
	 * local CPU!  We need external transactions on the shared
	 * bus.
	 */
	reg = CONFIG_MP_NUM_CPUS == 1 ? 0 : 0x15;
	__asm__ volatile("wsr %0, ATOMCTL" :: "r"(reg));

	/* Initialize interrupts to "disabled" */
	reg = 0;
	__asm__ volatile("wsr %0, INTENABLE" :: "r"(reg));

	/* Finally VECBASE.  Note that on core 0 startup, we're still
	 * running in IMR and the vectors at this address won't be
	 * copied into HP-SRAM until later.  That's OK, as interrupts
	 * are still disabled at this stage and will remain so
	 * consistently until Zephyr switches into the main thread.
	 */
	reg = XCHAL_VECBASE_RESET_PADDR_SRAM;
	__asm__ volatile("wsr %0, VECBASE" :: "r"(reg));
}

#endif /* __INTEL_ADSP_CPU_INIT_H */
