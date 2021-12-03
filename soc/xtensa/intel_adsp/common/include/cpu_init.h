/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __INTEL_ADSP_CPU_INIT_H
#define __INTEL_ADSP_CPU_INIT_H

#define CxL1CCAP (*(volatile uint32_t *)0x9F080080)
#define CxL1CCFG (*(volatile uint32_t *)0x9F080084)
#define CxL1PCFG (*(volatile uint32_t *)0x9F080088)

/* "Data/Instruction Cache Memory Way Count" fields */
#define CxL1CCAP_DCMWC ((CxL1CCAP >> 16) & 7)
#define CxL1CCAP_ICMWC ((CxL1CCAP >> 20) & 7)

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
	if (IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V25)) {
		/* Already set up by the ROM on older hardware. */
		const uint8_t attribs[] = { 2, 15, 15, 15, 2, 4, 15, 15 };

		for (int region = 0; region < 8; region++) {
			reg = 0x20000000 * region;
			__asm__ volatile("wdtlb %0, %1"
					 :: "r"(attribs[region]), "r"(reg));
		}
	}

	/* Initialize ATOMCTL: Hardware defaults for S32C1I use
	 * "internal" operations, meaning they are atomic only WRT the
	 * local CPU!  We need external transactions on the shared
	 * bus.
	 */
	reg = 0x15;
	__asm__ volatile("wsr %0, ATOMCTL" :: "r"(reg));
}

#endif /* __INTEL_ADSP_CPU_INIT_H */
