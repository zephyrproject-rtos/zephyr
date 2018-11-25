/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>

#include <device.h>
#include <init.h>
#include <fsl_clock.h>

#define SCG_LPFLL_DISABLE 0U

/*
 * Run-mode configuration for the fast internal reference clock (FIRC).
 */
static const scg_firc_config_t rv32m1_firc_config = {
	.enableMode = kSCG_FircEnable,
	.div1 = kSCG_AsyncClkDivBy1,
	.div2 = kSCG_AsyncClkDivBy1,
	.div3 = kSCG_AsyncClkDivBy1,
	.range = kSCG_FircRange48M,
	.trimConfig = NULL,
};

/*
 * FIRC-based system clock configuration.
 */
static const scg_sys_clk_config_t rv32m1_sys_clk_config_firc = {
	.divSlow = kSCG_SysClkDivBy2,
	.divBus = kSCG_SysClkDivBy1,
	.divExt = kSCG_SysClkDivBy1,
	.divCore = kSCG_SysClkDivBy1,
	.src = kSCG_SysClkSrcFirc,
};

/*
 * LPFLL configuration.
 */
static const scg_lpfll_config_t rv32m1_lpfll_cfg = {
	.enableMode = SCG_LPFLL_DISABLE,
	.div1 = kSCG_AsyncClkDivBy1,
	.div2 = kSCG_AsyncClkDisable,
	.div3 = kSCG_AsyncClkDisable,
	.range = kSCG_LpFllRange48M,
	.trimConfig = NULL,
};

void _arch_irq_enable(unsigned int irq)
{
	EVENT_UNIT->INTPTEN |= (1U << irq);
	(void)(EVENT_UNIT->INTPTEN); /* Ensures write has finished. */
}

void _arch_irq_disable(unsigned int irq)
{
	EVENT_UNIT->INTPTEN &= ~(1U << irq);
	(void)(EVENT_UNIT->INTPTEN); /* Ensures write has finished. */
}

int _arch_irq_is_enabled(unsigned int irq)
{
	return (EVENT_UNIT->INTPTEN & (1U << irq)) != 0;
}

/*
 * SoC-level interrupt initialization. Clear any pending interrupts or
 * events.
 */
void soc_interrupt_init(void)
{
	EVENT_UNIT->INTPTPENDCLEAR = 0xFFFFFFFF;
	(void)(EVENT_UNIT->INTPTPENDCLEAR); /* Ensures write has finished. */
	EVENT_UNIT->EVTPENDCLEAR   = 0xFFFFFFFF;
	(void)(EVENT_UNIT->EVTPENDCLEAR); /* Ensures write has finished. */
}

/**
 * @brief Switch system clock configuration in run mode.
 *
 * Blocks until the updated configuration takes effect.
 *
 * @param cfg New system clock configuration
 */
static void rv32m1_switch_sys_clk(const scg_sys_clk_config_t *cfg)
{
	scg_sys_clk_config_t cur_cfg;

	CLOCK_SetRunModeSysClkConfig(cfg);
	do {
		CLOCK_GetCurSysClkConfig(&cur_cfg);
	} while (cur_cfg.src != cfg->src);
}

/**
 * @brief Initializes SIRC and switches system clock source to SIRC.
 */
static void rv32m1_switch_to_sirc(void)
{
	const scg_sirc_config_t sirc_config = {
		.enableMode = kSCG_SircEnable,
		.div1 = kSCG_AsyncClkDisable,
		.div2 = kSCG_AsyncClkDivBy2,
		.range = kSCG_SircRangeHigh,
	};
	const scg_sys_clk_config_t sys_clk_config_sirc = {
		.divSlow = kSCG_SysClkDivBy4,
		.divCore = kSCG_SysClkDivBy1,
		.src = kSCG_SysClkSrcSirc,
	};

	CLOCK_InitSirc(&sirc_config);
	rv32m1_switch_sys_clk(&sys_clk_config_sirc);
}

/**
 * @brief Perform basic hardware initialization
 *
 * Initializes the base clocks and LPFLL using helpers provided by the HAL.
 *
 * @return 0
 */
static int soc_rv32m1_init(struct device *arg)
{
	unsigned int key;

	ARG_UNUSED(arg);

	key = irq_lock();

	/* Switch to SIRC so we can initialize the FIRC. */
	rv32m1_switch_to_sirc();

	/* Now that we're running off of SIRC, set up and switch to FIRC. */
	CLOCK_InitFirc(&rv32m1_firc_config);
	rv32m1_switch_sys_clk(&rv32m1_sys_clk_config_firc);

	/* Initialize LPFLL */
	CLOCK_InitLpFll(&rv32m1_lpfll_cfg);

	irq_unlock(key);

	return 0;
}

SYS_INIT(soc_rv32m1_init, PRE_KERNEL_1, 0);
