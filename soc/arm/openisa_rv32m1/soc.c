/*
 * Copyright (c) 2018 Foundries.io
 * Copyright (c) 2020 Karsten Koenig
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>

#include <device.h>
#include <init.h>
#include <fsl_clock.h>
#include <fsl_wdog32.h>
#include <sys/util.h>



#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(soc);

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

	/* Initialize peripheral clocks */
	rv32m1_setup_peripheral_clocks();

	irq_unlock(key);

	return 0;
}

void z_arm_watchdog_init(void)
{
	WDOG32_Deinit(WDOG0);
}

SYS_INIT(soc_rv32m1_init, PRE_KERNEL_1, 0);
