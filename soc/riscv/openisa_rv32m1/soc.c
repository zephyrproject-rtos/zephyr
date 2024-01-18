/*
 * Copyright (c) 2018 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <fsl_clock.h>
#include <zephyr/sys/util.h>

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS)
#include <errno.h>
#include <zephyr/irq_nextlevel.h>
#endif

#include <soc.h>

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(soc);

#define SCG_LPFLL_DISABLE 0U

static const struct device *dev_intmux;

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

void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	EVENT_UNIT->SLPCTRL |= EVENT_SLPCTRL_SYSRSTREQST_MASK;
}

void arch_irq_enable(unsigned int irq)
{
	if (IS_ENABLED(CONFIG_MULTI_LEVEL_INTERRUPTS)) {
		unsigned int level = rv32m1_irq_level(irq);

		if (level == 1U) {
			EVENT_UNIT->INTPTEN |= BIT(rv32m1_level1_irq(irq));
			/* Ensures write has finished: */
			(void)(EVENT_UNIT->INTPTEN);
		} else {
			irq_enable_next_level(dev_intmux, irq);
		}
	} else {
		EVENT_UNIT->INTPTEN |= BIT(rv32m1_level1_irq(irq));
		(void)(EVENT_UNIT->INTPTEN);
	}
}

void arch_irq_disable(unsigned int irq)
{
	if (IS_ENABLED(CONFIG_MULTI_LEVEL_INTERRUPTS)) {
		unsigned int level = rv32m1_irq_level(irq);

		if (level == 1U) {
			EVENT_UNIT->INTPTEN &= ~BIT(rv32m1_level1_irq(irq));
			/* Ensures write has finished: */
			(void)(EVENT_UNIT->INTPTEN);
		} else {
			irq_disable_next_level(dev_intmux, irq);
		}
	} else {
		EVENT_UNIT->INTPTEN &= ~BIT(rv32m1_level1_irq(irq));
		(void)(EVENT_UNIT->INTPTEN);
	}
}

int arch_irq_is_enabled(unsigned int irq)
{
	if (IS_ENABLED(CONFIG_MULTI_LEVEL_INTERRUPTS)) {
		unsigned int level = rv32m1_irq_level(irq);

		if (level == 1U) {
			return (EVENT_UNIT->INTPTEN &
				BIT(rv32m1_level1_irq(irq))) != 0;
		} else {
			uint32_t channel, line, ier;

			/*
			 * Here we break the abstraction and look
			 * directly at the INTMUX registers. We can't
			 * use the irq_nextlevel.h API, as that only
			 * tells us whether some IRQ at the next level
			 * is enabled or not.
			 */
			channel = rv32m1_intmux_channel(irq);
			line = rv32m1_intmux_line(irq);
			ier = INTMUX->CHANNEL[channel].CHn_IER_31_0 & BIT(line);

			return ier != 0U;
		}
	} else {
		return (EVENT_UNIT->INTPTEN & BIT(rv32m1_level1_irq(irq))) != 0;
	}
}

/*
 * SoC-level interrupt initialization. Clear any pending interrupts or
 * events, and find the INTMUX device if necessary.
 *
 * This gets called as almost the first thing z_cstart() does, so it
 * will happen before any calls to the _arch_irq_xxx() routines above.
 */
void soc_interrupt_init(void)
{
	EVENT_UNIT->INTPTPENDCLEAR = 0xFFFFFFFF;
	(void)(EVENT_UNIT->INTPTPENDCLEAR); /* Ensures write has finished. */
	EVENT_UNIT->EVTPENDCLEAR   = 0xFFFFFFFF;
	(void)(EVENT_UNIT->EVTPENDCLEAR); /* Ensures write has finished. */

	if (IS_ENABLED(CONFIG_MULTI_LEVEL_INTERRUPTS)) {
		dev_intmux = DEVICE_DT_GET(DT_INST(0, openisa_rv32m1_intmux));
	}
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
 * @brief Setup peripheral clocks
 *
 * Setup the peripheral clock sources.
 */
static void rv32m1_setup_peripheral_clocks(void)
{
#if DT_NODE_HAS_STATUS(DT_NODELABEL(tpm0), okay)
	CLOCK_SetIpSrc(kCLOCK_Tpm0, kCLOCK_IpSrcFircAsync);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(tpm1), okay)
	CLOCK_SetIpSrc(kCLOCK_Tpm1, kCLOCK_IpSrcFircAsync);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(tpm2), okay)
	CLOCK_SetIpSrc(kCLOCK_Tpm2, kCLOCK_IpSrcFircAsync);
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(tpm3), okay)
	CLOCK_SetIpSrc(kCLOCK_Tpm3, kCLOCK_IpSrcFircAsync);
#endif
}

/**
 * @brief Perform basic hardware initialization
 *
 * Initializes the base clocks and LPFLL using helpers provided by the HAL.
 *
 * @return 0
 */
static int soc_rv32m1_init(void)
{
	unsigned int key;


	key = irq_lock();

	/* Switch to SIRC so we can initialize the FIRC. */
	rv32m1_switch_to_sirc();

	/* Now that we're running off of SIRC, set up and switch to FIRC. */
	CLOCK_InitFirc(&rv32m1_firc_config);
	rv32m1_switch_sys_clk(&rv32m1_sys_clk_config_firc);

	/* Initialize LPFLL */
	CLOCK_InitLpFll(&rv32m1_lpfll_cfg);

	/* Initialize peripheral clocks */
	rv32m1_setup_peripheral_clocks();

	irq_unlock(key);

	return 0;
}

SYS_INIT(soc_rv32m1_init, PRE_KERNEL_1, 0);
