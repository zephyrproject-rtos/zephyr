/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __INTEL_ADSP_CPU_INIT_H
#define __INTEL_ADSP_CPU_INIT_H

#include <zephyr/arch/arch_inlines.h>
#include <zephyr/arch/xtensa/arch.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/corebits.h>
#include <adsp_memory.h>

#define MEMCTL_VALUE    (MEMCTL_INV_EN | MEMCTL_ICWU_MASK | MEMCTL_DCWA_MASK | \
			 MEMCTL_DCWU_MASK | MEMCTL_L0IBUF_EN)

#define ATOMCTL_BY_RCW	BIT(0) /* RCW Transaction for Bypass Memory */
#define ATOMCTL_WT_RCW	BIT(2) /* RCW Transaction for Writethrough Cacheable Memory */
#define ATOMCTL_WB_RCW	BIT(4) /* RCW Transaction for Writeback Cacheable Memory */
#define ATOMCTL_VALUE (ATOMCTL_BY_RCW | ATOMCTL_WT_RCW | ATOMCTL_WB_RCW)

#ifdef CONFIG_INTEL_ADSP_MEMORY_IS_MIRRORED

/* Utility to generate an unrolled and optimal[1] code sequence to set
 * the RPO TLB registers (contra the HAL cacheattr macros, which
 * generate larger code and can't be called from C), based on the
 * KERNEL_COHERENCE configuration in use.  Selects RPO attribute "2"
 * for regions (including MMIO registers in region zero) which want to
 * bypass L1, "4" for the cached region which wants writeback, and
 * "15" (invalid) elsewhere.
 *
 * Note that on cores that have the "translation" option set, we need
 * to put an identity mapping in the high bits.  Also per spec
 * changing the current code region (by definition cached) requires
 * that WITLB be followed by an ISYNC and that both instructions live
 * in the same cache line (two 3-byte instructions fit in an 8-byte
 * aligned region, so that's guaranteed not to cross a cache line
 * boundary).
 *
 * [1] With the sole exception of gcc's infuriating insistence on
 * emitting a precomputed literal for addr + addrincr instead of
 * computing it with a single ADD instruction from values it already
 * has in registers.  Explicitly assigning the variables to registers
 * via an attribute works, but then emits needless MOV instructions
 * instead.  I tell myself it's just 32 bytes of .text, but... Sigh.
 */
#define _REGION_ATTR(r)						\
	((r) == 0 ? 2 :						\
	 ((r) == CONFIG_INTEL_ADSP_CACHED_REGION ? 4 :		\
	  ((r) == CONFIG_INTEL_ADSP_UNCACHED_REGION ? 2 : 15)))

#define _SET_ONE_TLB(region) do {				\
	uint32_t attr = _REGION_ATTR(region);			\
	if (XCHAL_HAVE_XLT_CACHEATTR) {				\
		attr |= addr; /* RPO with translation */	\
	}							\
	if (region != CONFIG_INTEL_ADSP_CACHED_REGION) {	\
		__asm__ volatile("wdtlb %0, %1; witlb %0, %1"	\
				 :: "r"(attr), "r"(addr));	\
	} else {						\
		__asm__ volatile("wdtlb %0, %1"			\
				 :: "r"(attr), "r"(addr));	\
		__asm__ volatile("j 1f; .align 8; 1:");		\
		__asm__ volatile("witlb %0, %1; isync"		\
				 :: "r"(attr), "r"(addr));	\
	}							\
	addr += addrincr;					\
} while (0)

/**
 * @brief Setup RPO TLB registers.
 */
#define SET_RPO_TLB()							\
	do {								\
		register uint32_t addr = 0, addrincr = 0x20000000;	\
		FOR_EACH(_SET_ONE_TLB, (;), 0, 1, 2, 3, 4, 5, 6, 7);	\
	} while (0)

#endif /* CONFIG_INTEL_ADSP_MEMORY_IS_MIRRORED */

/* Low-level CPU initialization.  Call this immediately after entering
 * C code to initialize the cache, protection and synchronization
 * features.
 */
static ALWAYS_INLINE void cpu_early_init(void)
{
	uint32_t reg;

#ifdef CONFIG_ADSP_NEED_POWER_ON_CACHE
	/* First, we need to power the cache SRAM banks on!  Write a bit
	 * for each cache way in the bottom half of the L1CCFG register
	 * and poll the top half for them to turn on.
	 */
	uint32_t dmask = BIT(ADSP_CxL1CCAP_DCMWC) - 1;
	uint32_t imask = BIT(ADSP_CxL1CCAP_ICMWC) - 1;
	uint32_t waymask = (imask << 8) | dmask;

	ADSP_CxL1CCFG_REG = waymask;
	while (((ADSP_CxL1CCFG_REG >> 16) & waymask) != waymask) {
	}

	/* Prefetcher also power gates, same interface */
	ADSP_CxL1PCFG_REG = 1;
	while ((ADSP_CxL1PCFG_REG & 0x10000) == 0) {
	}
#endif

	/* Now set up the Xtensa CPU to enable the cache logic.  The
	 * details of the fields are somewhat complicated, but per the
	 * ISA ref: "Turning on caches at power-up usually consists of
	 * writing a constant with bits[31:8] all 1’s to MEMCTL.".
	 * Also set bit 0 to enable the LOOP extension instruction
	 * fetch buffer.
	 */
#if XCHAL_USE_MEMCTL
	reg = MEMCTL_VALUE;
	XTENSA_WSR("MEMCTL", reg);
	__asm__ volatile("rsync");
#endif

#if XCHAL_HAVE_THREADPTR
	reg = 0;
	XTENSA_WUR("THREADPTR", reg);
#endif

	/* Likewise enable prefetching.  Sadly these values are not
	 * architecturally defined by Xtensa (they're just documented
	 * as priority hints), so this constant is just copied from
	 * SOF for now.  If we care about prefetch priority tuning
	 * we're supposed to ask Cadence I guess.
	 */
	reg = ADSP_L1_CACHE_PREFCTL_VALUE;
	XTENSA_WSR("PREFCTL", reg);
	__asm__ volatile("rsync");

	/* Finally we need to enable the cache in the Region
	 * Protection Option "TLB" entries.  The hardware defaults
	 * have this set to RW/uncached everywhere.
	 *
	 * If we have MMU enabled, we don't need to do this right now.
	 * Let use the default configuration and properly configure the
	 * MMU when running from RAM.
	 */
#if defined(CONFIG_INTEL_ADSP_MEMORY_IS_MIRRORED) && !defined(CONFIG_MMU)
	SET_RPO_TLB();
#endif /* CONFIG_INTEL_ADSP_MEMORY_IS_MIRRORED && !CONFIG_MMU */


	/* Initialize ATOMCTL: Hardware defaults for S32C1I use
	 * "internal" operations, meaning they are atomic only WRT the
	 * local CPU!  We need external transactions on the shared
	 * bus.
	 */
	reg = ATOMCTL_VALUE;
	XTENSA_WSR("ATOMCTL", reg);

	/* Initialize interrupts to "disabled" */
	reg = 0;
	XTENSA_WSR("INTENABLE", reg);

	/* Finally VECBASE.  Note that on core 0 startup, we're still
	 * running in IMR and the vectors at this address won't be
	 * copied into HP-SRAM until later.  That's OK, as interrupts
	 * are still disabled at this stage and will remain so
	 * consistently until Zephyr switches into the main thread.
	 */
	reg = VECBASE_RESET_PADDR_SRAM;
	XTENSA_WSR("VECBASE", reg);
}

#endif /* __INTEL_ADSP_CPU_INIT_H */
