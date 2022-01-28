/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <device.h>
#include <xtensa/xtruntime.h>
#include <irq_nextlevel.h>
#include <xtensa/hal.h>
#include <init.h>


#include <arch/xtensa/cache.h>
#include <cavs-shim.h>
#include <cavs-mem.h>
#include <cpu_init.h>
#include "manifest.h"
#include <ace_v1x-regs.h>
#include "soc.h"

extern void soc_mp_init(void);
extern void win_setup(void);
extern void lp_sram_init(void);
extern void hp_sram_init(uint32_t memory_size);
extern void parse_manifest(void);

#define DSP_INIT_LPGPDMA(x)		(0x71A60 + (2*x))
#define LPGPDMA_CTLOSEL_FLAG	BIT(15)
#define LPGPDMA_CHOSEL_FLAG		0xFF

__imr void boot_core0(void)
{
	/* This is a workaround for simulator, which
	 * doesn't support direct control over the secondary core boot
	 * vectors.  It always boots into the ROM, and ends up here.
	 * This emulates the hardware design and jumps to the pointer
	 * found in the BADDR register (which is wired to the Xtensa
	 * LX core's alternate boot vector input).  Should be removed
	 * at some point, but it's tiny and harmless otherwise.
	 */
	int prid;

	prid = arch_proc_id();
	if (prid != 0) {
		((void(*)(void))MTL_PWRBOOT.bootctl[prid].baddr)();
	}

	cpu_early_init();

#ifdef PLATFORM_DISABLE_L2CACHE_AT_BOOT
	/* FIXME: L2 cache control PCFG register */
	*(uint32_t *)0x1508 = 0;
#endif

	hp_sram_init(L2_SRAM_SIZE);
	win_setup();
	lp_sram_init();
	parse_manifest();
	soc_trace_init();
	z_xtensa_cache_flush_all();

	/* Zephyr! */
	extern FUNC_NORETURN void z_cstart(void);
	z_cstart();
}

static __imr void power_init_mtl(void)
{
	/* Disable idle power gating */
	MTL_PWRBOOT.bootctl[0].bctl |= MTL_PWRBOOT_BCTL_WAITIPCG | MTL_PWRBOOT_BCTL_WAITIPPG;

#if CONFIG_DMA_ACE_GPDMA
	sys_write32(LPGPDMA_CHOSEL_FLAG | LPGPDMA_CTLOSEL_FLAG, DSP_INIT_LPGPDMA(0));
	sys_write32(LPGPDMA_CHOSEL_FLAG | LPGPDMA_CTLOSEL_FLAG, DSP_INIT_LPGPDMA(1));
#endif
}

static __imr int soc_init(const struct device *dev)
{
	power_init_mtl();

#if CONFIG_MP_NUM_CPUS > 1
	soc_mp_init();
#endif

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, 99);
