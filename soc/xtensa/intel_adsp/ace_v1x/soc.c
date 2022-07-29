/*
 * Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/arch/xtensa/cache.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/irq_nextlevel.h>

#include <ace_v1x-regs.h>
#include <adsp_memory.h>
#include <adsp_shim.h>
#include <cpu_init.h>
#include <soc.h>
#include <xtensa/hal.h>
#include <xtensa/xtruntime.h>

extern void soc_mp_init(void);
extern void win_setup(void);
extern void lp_sram_init(void);
extern void hp_sram_init(void);
extern void parse_manifest(void);

#define DSP_INIT_LPGPDMA(x)  (0x71A60 + (2 * x))
#define LPGPDMA_CTLOSEL_FLAG BIT(15)
#define LPGPDMA_CHOSEL_FLAG  0xFF

#define LPSRAM_MASK(x) 0x00000003
#define SRAM_BANK_SIZE (64 * 1024)
#define HOST_PAGE_SIZE 4096

#define MANIFEST_SEGMENT_COUNT 3


static __imr void power_init(void)
{
	/* Disable idle power gating */
	DFDSPBRCP.bootctl[0].bctl |= DFDSPBRCP_BCTL_WAITIPCG | DFDSPBRCP_BCTL_WAITIPPG;

#if CONFIG_DMA_INTEL_ADSP_GPDMA
	sys_write32(LPGPDMA_CHOSEL_FLAG | LPGPDMA_CTLOSEL_FLAG, DSP_INIT_LPGPDMA(0));
	sys_write32(LPGPDMA_CHOSEL_FLAG | LPGPDMA_CTLOSEL_FLAG, DSP_INIT_LPGPDMA(1));
#endif
}

static __imr int soc_init(const struct device *dev)
{
	power_init();

#if CONFIG_MP_NUM_CPUS > 1
	soc_mp_init();
#endif

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, 99);
