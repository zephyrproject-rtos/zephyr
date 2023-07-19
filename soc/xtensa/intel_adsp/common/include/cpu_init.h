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
	 */
	ARCH_XTENSA_SET_RPO_TLB();

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
