/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/dt-bindings/clock/nxp_mcxw_clock.h>

#include <fsl_clock.h>
#include <fsl_ccm32k.h>

#define DT_DRV_COMPAT nxp_mcxw_clock

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
LOG_MODULE_REGISTER(clock_control);

extern uint32_t SystemCoreClock;

/*
 * Mapping table from device tree clock IP mux values to HAL clock IP source values.
 * Used to translate DT-defined mux selections to the corresponding FSL SDK clock source.
 */
static clock_ip_src_t ip_clk_mux_mapping[] = {
	[MCXW_CLK_IP_MUX_FRO_6M] = kCLOCK_IpSrcFro6M,
	[MCXW_CLK_IP_MUX_FRO_192M_DIV] = kCLOCK_IpSrcFro192M,
	[MCXW_CLK_IP_MUX_SOSC] = kCLOCK_IpSrcSoscClk,
	[MCXW_CLK_IP_MUX_32K] = kCLOCK_IpSrc32kClk,
#if defined(CONFIG_SOC_MCXW70AC)
	[MCXW_CLK_IP_MUX_FRO_200M_DIV] = kCLOCK_IpSrcFro200M,
	[MCXW_CLK_IP_MUX_1M] = kCLOCK_IpSrc1M,
#endif /* CONFIG_SOC_MCXW70AC */
};

struct mcxw_clock_control_subsys_info {
	uint32_t flags: 4;
	uint32_t div: 4;
	uint32_t mux: 8;
	uint32_t offset: 16;
};

struct mcxw_clock_control_config {
	/* OSC32K mode configuration */
	uint8_t osc32k_mode;
	/* OSC32K XTAL capacitance configuration */
	ccm32k_osc_xtal_cap_t xtal_cap;
	/* OSC32K EXTAL capacitance configuration */
	ccm32k_osc_extal_cap_t extal_cap;

	/* FIRC mode configuration */
	uint8_t firc_mode;
	/* FIRC frequency range configuration */
	uint8_t firc_range;

	/* Enable SIRC in low power mode */
	bool enable_sirc_in_lp_mode;

	/* System clock source selection */
	uint8_t sys_clk_src;
	/* System clock divider for slow clock */
	uint8_t sys_clk_div_slow;
	/* System clock divider for bus clock */
	uint8_t sys_clk_div_bus;
	/* System clock divider for core clock */
	uint8_t sys_clk_div_core;

	/* System oscillator frequency in Hz */
	uint32_t sosc_freq;
	/* Enable system oscillator */
	bool enable_sosc;
};

static int nxp_mcxw_clock_control_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct mcxw_clock_control_subsys_info *subsys_info =
		(struct mcxw_clock_control_subsys_info *)&sub_system;

	if (subsys_info->offset == 0) {
		/* offset is 0 indicate do not support clock gate. */
		return 0;
	}

	clock_ip_name_t ip_name =
		(clock_ip_name_t)MAKE_MRCC_REGADDR(MRCC_BASE, subsys_info->offset);

	if ((subsys_info->flags & MCXW_CLK_MODE_ENABLED_LP_NO_STALL) != 0) {
		CLOCK_EnableClockLPMode(ip_name, (clock_ip_control_t)MRCC_CC(subsys_info->flags));
	} else {
		CLOCK_EnableClock(ip_name);
	}

	return 0;
}

static int nxp_mcxw_clock_control_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct mcxw_clock_control_subsys_info *subsys_info =
		(struct mcxw_clock_control_subsys_info *)&sub_system;

	if (subsys_info->offset == 0) {
		/* offset is 0 indicate do not support clock gate. */
		return -ENOTSUP;
	}

	clock_ip_name_t ip_name =
		(clock_ip_name_t)MAKE_MRCC_REGADDR(MRCC_BASE, subsys_info->offset);
	CLOCK_DisableClock(ip_name);

	return 0;
}

static int nxp_mcxw_clock_control_get_rate(const struct device *dev,
					   clock_control_subsys_t sub_system, uint32_t *rate)
{
	struct mcxw_clock_control_subsys_info *subsys_info =
		(struct mcxw_clock_control_subsys_info *)&sub_system;
	if (subsys_info == NULL || rate == NULL) {
		return -EINVAL;
	}

	if (subsys_info->offset == 0) {
		/* offset is 0 indicate do not support clock gate. */
		return -ENOTSUP;
	}

	clock_ip_name_t ip_name =
		(clock_ip_name_t)MAKE_MRCC_REGADDR(MRCC_BASE, subsys_info->offset);
	*rate = CLOCK_GetIpFreq(ip_name);

	if ((*rate == 0) && ((ip_name == kCLOCK_Wdog0) || (ip_name == kCLOCK_Wdog1))) {
		switch (subsys_info->mux) {
		case MCXW_CLK_IP_MUX_SLOW_CLK: {
			*rate = CLOCK_GetFreq(kCLOCK_SlowClk);
			break;
		}
		case MCXW_CLK_IP_MUX_SOSC: {
			*rate = CLOCK_GetFreq(kCLOCK_ScgSysOscClk);
			break;
		}
		case MCXW_CLK_IP_MUX_32K: {
			*rate = CLOCK_GetFreq(kCLOCK_RtcOscClk);
			break;
		}
		case MCXW_CLK_IP_MUX_FRO_6M: {
			*rate = CLOCK_GetFreq(kCLOCK_ScgSircClk);
		}
		}
	}

	return 0;
}

static int nxp_mcxw_clock_control_configure(const struct device *dev,
					    clock_control_subsys_t sub_system, void *data)
{
	struct mcxw_clock_control_subsys_info *subsys_info =
		(struct mcxw_clock_control_subsys_info *)&sub_system;

	if (subsys_info->offset == 0) {
		/* offset is 0 indicate do not support clock configure. */
		LOG_INF("Clock configure not supported for this subsystem, using default settings");
		return 0;
	}
	uint8_t mux = subsys_info->mux;

	if (mux != MCXW_CLK_IP_MUX_NONE) {
		clock_ip_name_t ip_name =
			(clock_ip_name_t)MAKE_MRCC_REGADDR(MRCC_BASE, subsys_info->offset);
		CLOCK_SetIpSrc(ip_name, ip_clk_mux_mapping[mux]);
		CLOCK_SetIpSrcDiv(ip_name, subsys_info->div);
	}

	return 0;
}

static enum clock_control_status
nxp_mcxw_clock_control_get_status(const struct device *dev, clock_control_subsys_t sub_system)
{
	struct mcxw_clock_control_subsys_info *subsys_info =
		(struct mcxw_clock_control_subsys_info *)&sub_system;

	if (subsys_info == NULL) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if (subsys_info->offset == 0) {
		/* offset is 0 indicate do not support clock gate. */
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}
	clock_ip_name_t ip_name =
		(clock_ip_name_t)MAKE_MRCC_REGADDR(MRCC_BASE, subsys_info->offset);

	uint32_t reg = CLOCK_REG(ip_name);

	if ((reg & MRCC_CC_MASK) == 0) {
		return CLOCK_CONTROL_STATUS_OFF;
	} else {
		return CLOCK_CONTROL_STATUS_ON;
	}
}

static int nxp_mcxw_clock_control_pm(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);
	return 0;
}

static int nxp_mcxw_clock_control_init(const struct device *dev)
{
	const struct mcxw_clock_control_config *config = dev->config;

	/* Unlock Reference Clock Status Registers to allow writes */
	CLOCK_UnlockFircControlStatusReg();
	CLOCK_UnlockSircControlStatusReg();
	CLOCK_UnlockRoscControlStatusReg();
	CLOCK_UnlockSysOscControlStatusReg();

	CLOCK_SetXtal32Freq(32768U);

	/* Init OSC32K */
	CLOCK_SetRoscMonitorMode(kSCG_RoscMonitorDisable);
	ccm32k_osc_config_t ccm32k_osc_config = {
		.coarseAdjustment = kCCM32K_OscCoarseAdjustmentRange0,
		.enableInternalCapBank = true,
		.xtalCap = config->xtal_cap,
		.extalCap = config->extal_cap,
	};
	CCM32K_Set32kOscConfig(CCM32K, (ccm32k_osc_mode_t)config->osc32k_mode, &ccm32k_osc_config);

	/* Configuration to set FIRC */
	scg_firc_config_t scg_firc_config = {
		.enableMode = (config->firc_mode == MCXW_CLK_FIRC_DISABLE)
				      ? 0
				      : ((config->firc_mode == MCXW_CLK_FIRC_ENABLE)
						 ? kSCG_FircEnable
						 : kSCG_FircEnableInSleep),
		.range = (scg_firc_range_t)config->firc_range,
		.trimConfig = NULL,
	};

	/* Switch to safe clock source (SIRC) before reconfiguring FIRC */
	scg_sys_clk_config_t sys_clk_safe_config_source = {
		.divSlow = (uint32_t)kSCG_SysClkDivBy4,
		.divCore = (uint32_t)kSCG_SysClkDivBy1,
		.src = (uint32_t)kSCG_SysClkSrcSirc,
	};

	CLOCK_SetRunModeSysClkConfig(&sys_clk_safe_config_source);

	scg_sys_clk_config_t cur_config;
	/* Wait for clock source switch to finish */
	do {
		CLOCK_GetCurSysClkConfig(&cur_config);
	} while (cur_config.src != sys_clk_safe_config_source.src);

	/* Initialize FIRC */
	(void)CLOCK_InitFirc(&scg_firc_config);

	if (config->enable_sirc_in_lp_mode) {
		scg_sirc_config_t scg_sirc_config = {
			.enableMode = kSCG_SircEnableInSleep,
		};
		(void)CLOCK_InitSirc(&scg_sirc_config);
	}

	if (config->enable_sosc) {
		scg_sosc_config_t scg_sosc_config = {
			.freq = config->sosc_freq,
			.enableMode = kSCG_SoscEnable,
			.monitorMode = kSCG_SysOscMonitorDisable,
		};
		(void)CLOCK_InitSysOsc(&scg_sosc_config);
		CLOCK_SetXtal0Freq(config->sosc_freq);
	}

	/* Configure system clock with user-defined settings */
	scg_sys_clk_config_t sys_clk_config = {
		.divSlow = (config->sys_clk_div_slow - 1),
		.divBus = (config->sys_clk_div_bus - 1),
		.divCore = (config->sys_clk_div_core - 1),
		.src = config->sys_clk_src,
	};

	CLOCK_SetRunModeSysClkConfig(&sys_clk_config);

	/* Wait for clock source switch to finish */
	do {
		CLOCK_GetCurSysClkConfig(&cur_config);
	} while (cur_config.src != sys_clk_config.src);

	switch (config->sys_clk_src) {
	case MCXW_CLK_SYSTEM_CLK_SRC_SOSC: {
		if (!config->enable_sosc) {
			LOG_ERR("System OSC selected but not enabled in config");
			return -EINVAL;
		}
		SystemCoreClock = config->sosc_freq;
		break;
	}
	case MCXW_CLK_SYSTEM_CLK_SRC_SIRC: {
		SystemCoreClock = 6000000U; /* SIRC frequency is 6 MHz */
		break;
	}
	case MCXW_CLK_SYSTEM_CLK_SRC_FIRC: {
		if (config->firc_mode == MCXW_CLK_FIRC_DISABLE) {
			LOG_ERR("FIRC selected but disabled in config");
			return -EINVAL;
		}
		SystemCoreClock = CLOCK_GetFircFreq();
		break;
	}
	case MCXW_CLK_SYSTEM_CLK_SRC_ROSC: {
		SystemCoreClock = 32768U; /* ROSC frequency is 32.768 kHz */
		break;
	}
	}

	if (config->osc32k_mode != kCCM32K_Disable32kHzCrystalOsc) {
		while ((CCM32K_GetStatusFlag(CCM32K) & kCCM32K_32kOscReadyStatusFlag) == 0UL) {
		}
		CCM32K_SelectClockSource(CCM32K, kCCM32K_ClockSourceSelectOsc32k);
		while ((CCM32K_GetStatusFlag(CCM32K) & kCCM32K_32kOscActiveStatusFlag) == 0UL) {
		}
		/* Wait for RTC Oscillator to be Valid */
		while (!CLOCK_IsRoscValid()) {
		}
		CLOCK_SetRoscMonitorMode(kSCG_RoscMonitorInt);
		/* Disable the FRO32K to save power */
		CCM32K_Enable32kFro(CCM32K, false);
#if FSL_FEATURE_CCM32K_HAS_CGC32K
		CCM32K_SelectClockSource(CCM32K, kCCM32K_ClockSourceSelectOsc32k);
#endif
	}

#if FSL_FEATURE_CCM32K_HAS_CGC32K
	/* Enable 32kHz clock output to all peripherals.  */
	CCM32K_EnableCLKOutToPeripherals(CCM32K, 0xFF);
#endif

	return pm_device_driver_init(dev, nxp_mcxw_clock_control_pm);
}

static DEVICE_API(clock_control, nxp_mcxw_clock_control_api) = {
	.on = nxp_mcxw_clock_control_on,
	.off = nxp_mcxw_clock_control_off,
	.get_rate = nxp_mcxw_clock_control_get_rate,
	.get_status = nxp_mcxw_clock_control_get_status,
	.configure = nxp_mcxw_clock_control_configure,
};

static struct mcxw_clock_control_config config = {
	.osc32k_mode = DT_INST_PROP(0, osc32k_mode),
	.xtal_cap = DT_INST_PROP(0, osc32k_xtal_cap),
	.extal_cap = DT_INST_PROP(0, osc32k_extal_cap),
	.firc_mode = DT_INST_PROP(0, firc_mode),
	.firc_range = DT_INST_PROP(0, firc_range),
	.enable_sirc_in_lp_mode = DT_INST_PROP(0, enable_sirc_in_lp_mode),
	.sys_clk_src = DT_INST_PROP(0, sys_clk_src),
	.sys_clk_div_bus = DT_INST_PROP(0, sys_clk_div_bus),
	.sys_clk_div_slow = DT_INST_PROP(0, sys_clk_div_slow),
	.sys_clk_div_core = DT_INST_PROP(0, sys_clk_div_core),
	.enable_sosc = DT_INST_PROP(0, enable_sosc),
	.sosc_freq = DT_INST_PROP(0, sosc_freq),
};

DEVICE_DT_INST_DEFINE(0, nxp_mcxw_clock_control_init, NULL, NULL, &config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &nxp_mcxw_clock_control_api);
