/*
 * Copyright (c) 2025 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>

#include <reg/pmu.h>
#include <reg/gcfg.h>
#include <reg/xbi.h>
#include <reg/vcc0.h>

#define GCFG_BASE DT_REG_ADDR_BY_NAME(DT_NODELABEL(gcfg), gcfg)
#define PMU_BASE  DT_REG_ADDR_BY_NAME(DT_NODELABEL(gcfg), pmu)
#define VCC0_BASE DT_REG_ADDR_BY_NAME(DT_NODELABEL(gcfg), vcc0)
#define XBI_BASE  DT_REG_ADDR(DT_NODELABEL(flash_controller))

static void pmu_init(void)
{
	struct pmu_regs *pmu = ((struct pmu_regs *)PMU_BASE);

	/* Interrupt Event Wakeup from IDLE mode Enable */
	pmu->PMUIDLE |= PMU_IDLE_WU_ENABLE;
	/* GPTD wake up from STOP mode enable. */
	pmu->PMUSTOP |= PMU_STOP_WU_GPTD;
	/* SWD EDI32 wake up from STOP mode enable */
	pmu->PMUSTOP |= (PMU_STOP_WU_EDI32 | PMU_STOP_WU_SWD);

	/* ADCO auto-disable in STOP mode */
	pmu->PMUSTOPC |= 0x0001;
}

static int clock_init(void)
{
	struct gcfg_regs *gcfg = (struct gcfg_regs *)GCFG_BASE;
	int rc = 0;

	switch (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) {
	case 48000000:
		/* AHB:48MHz APB:24MHz*/
		gcfg->CLKCFG = (gcfg->CLKCFG & ~0x10) | 0x10;
		break;
	case 24000000:
		/* AHB:24MHz APB:12MHz*/
		gcfg->CLKCFG = (gcfg->CLKCFG & ~0x10) | 0x00;
		break;
	default:
		rc = -EINVAL;
		break;
	}
	return rc;
}

static uint32_t read_trim(uint32_t addr)
{
	struct xbi_regs *xbi = (struct xbi_regs *)XBI_BASE;
	uint32_t busy_wait = 1000;

	xbi->EFCFG |= BIT(0);
	xbi->EFADDR = addr;
	xbi->EFCMD = CMD_NVR_READ32_SINGLE;
	while (xbi->EFSTA & 0x01) {
		busy_wait--;
	}
	xbi->EFCFG &= ~BIT(0);
	return xbi->EFDAT;
}

static void trim_clock(void)
{
	struct gcfg_regs *gcfg = (struct gcfg_regs *)GCFG_BASE;
	struct xbi_regs *xbi = (struct xbi_regs *)XBI_BASE;
	struct vcc0_regs *vcc0 = (struct vcc0_regs *)VCC0_BASE;
	uint32_t data;
	uint32_t flag;

	flag = read_trim(ENE_TRIM_NVR0_ADDRESS);
	/* Trim data exist. */
	if ((flag == ENE_TRIM_VALID_FLAG) || (flag == LAB_TRIM_VALID_FLAG)) {
		data = read_trim(0x04);
		gcfg->ADCOTR = data;

		data = read_trim(0x08);
		gcfg->IDSR = data;

		data = read_trim(0x0C);
		vcc0->IOSCCFG_T = (unsigned char)(data >> 8);

		data = read_trim(0x10);
		if (flag == ENE_TRIM_VALID_FLAG) {
			gcfg->LDO15TRIM = (gcfg->LDO15TRIM & (~0x07UL)) | (data & 0x07);
			xbi->EFCFG = (xbi->EFCFG & (~0x000000F0)) | (0 << 4);
		} else {
			gcfg->LDO15TRIM = data;
			xbi->EFCFG = (xbi->EFCFG & (~0x000000F0)) | (2 << 4);
		}
	}
}

void soc_early_init_hook(void)
{
	struct gcfg_regs *gcfg = ((struct gcfg_regs *)GCFG_BASE);
	/* Boot Timer Disable */
	gcfg->GCFGMISC |= BIT(13);

	trim_clock();
	clock_init();
	pmu_init();
}
