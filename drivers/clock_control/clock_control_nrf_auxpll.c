/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_auxpll

#include <errno.h>
#include <stdint.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include <hal/nrf_auxpll.h>

/* maximum lock time in ms, >10x time observed experimentally */
#define AUXPLL_LOCK_TIME_MAX_MS  20
/* lock wait step in ms*/
#define AUXPLL_LOCK_WAIT_STEP_MS 1

struct clock_control_nrf_auxpll_config {
	NRF_AUXPLL_Type *auxpll;
	uint32_t ref_clk_hz;
	uint32_t ficr_ctune;
	nrf_auxpll_config_t cfg;
	uint16_t frequency;
	nrf_auxpll_ctrl_outsel_t out_div;
};

static int clock_control_nrf_auxpll_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_nrf_auxpll_config *config = dev->config;
	bool locked;
	unsigned int wait = 0U;

	ARG_UNUSED(sys);

	nrf_auxpll_task_trigger(config->auxpll, NRF_AUXPLL_TASK_START);

	do {
		locked = nrf_auxpll_mode_locked_check(config->auxpll);
		if (!locked) {
			k_msleep(AUXPLL_LOCK_WAIT_STEP_MS);
			wait += AUXPLL_LOCK_WAIT_STEP_MS;
		}
	} while (wait < AUXPLL_LOCK_TIME_MAX_MS && !locked);

	return locked ? 0 : -ETIMEDOUT;
}

static int clock_control_nrf_auxpll_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_control_nrf_auxpll_config *config = dev->config;

	ARG_UNUSED(sys);

	nrf_auxpll_task_trigger(config->auxpll, NRF_AUXPLL_TASK_STOP);

	while (nrf_auxpll_running_check(config->auxpll)) {
	}

	return 0;
}

static int clock_control_nrf_auxpll_get_rate(const struct device *dev, clock_control_subsys_t sys,
					     uint32_t *rate)
{
	const struct clock_control_nrf_auxpll_config *config = dev->config;
	uint8_t ratio;

	ARG_UNUSED(sys);

	ratio = nrf_auxpll_static_ratio_get(config->auxpll);

	*rate = (ratio * config->ref_clk_hz +
		 (config->ref_clk_hz * (uint64_t)config->frequency) /
			 (AUXPLL_AUXPLLCTRL_FREQUENCY_FREQUENCY_MaximumDiv + 1U)) /
		config->out_div;

	return 0;
}

static enum clock_control_status clock_control_nrf_auxpll_get_status(const struct device *dev,
								     clock_control_subsys_t sys)
{
	const struct clock_control_nrf_auxpll_config *config = dev->config;

	ARG_UNUSED(sys);

	if (nrf_auxpll_mode_locked_check(config->auxpll)) {
		return CLOCK_CONTROL_STATUS_ON;
	}

	return CLOCK_CONTROL_STATUS_OFF;
}

static const struct clock_control_driver_api clock_control_nrf_auxpll_api = {
	.on = clock_control_nrf_auxpll_on,
	.off = clock_control_nrf_auxpll_off,
	.get_rate = clock_control_nrf_auxpll_get_rate,
	.get_status = clock_control_nrf_auxpll_get_status,
};

static int clock_control_nrf_auxpll_init(const struct device *dev)
{
	const struct clock_control_nrf_auxpll_config *config = dev->config;

	nrf_auxpll_ctrl_frequency_set(config->auxpll, config->frequency);

	nrf_auxpll_lock(config->auxpll);
	nrf_auxpll_trim_ctune_set(config->auxpll, sys_read8(config->ficr_ctune));
	nrf_auxpll_config_set(config->auxpll, &config->cfg);
	nrf_auxpll_ctrl_outsel_set(config->auxpll, config->out_div);
	nrf_auxpll_unlock(config->auxpll);

	nrf_auxpll_ctrl_mode_set(config->auxpll, NRF_AUXPLL_CTRL_MODE_LOCKED);

	return 0;
}

#define CLOCK_CONTROL_NRF_AUXPLL_DEFINE(n)                                                         \
	static const struct clock_control_nrf_auxpll_config config##n = {                          \
		.auxpll = (NRF_AUXPLL_Type *)DT_INST_REG_ADDR(n),                                  \
		.ref_clk_hz = DT_PROP(DT_INST_CLOCKS_CTLR(n), clock_frequency),                    \
		.ficr_ctune = DT_REG_ADDR(DT_INST_PHANDLE(n, nordic_ficrs)) +                      \
			      DT_INST_PHA(n, nordic_ficrs, offset),                                \
		.cfg =                                                                             \
			{                                                                          \
				.outdrive = DT_INST_PROP(n, nordic_out_drive),                     \
				.current_tune = DT_INST_PROP(n, nordic_current_tune),              \
				.sdm_off = DT_INST_PROP(n, nordic_sdm_disable),                    \
				.dither_off = DT_INST_PROP(n, nordic_dither_disable),              \
				.range = DT_INST_ENUM_IDX(n, nordic_range),                        \
			},                                                                         \
		.frequency = DT_INST_PROP(n, nordic_frequency),                                    \
		.out_div = DT_INST_PROP(n, nordic_out_div),                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, clock_control_nrf_auxpll_init, NULL, NULL, &config##n,            \
			      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,                    \
			      &clock_control_nrf_auxpll_api);

DT_INST_FOREACH_STATUS_OKAY(CLOCK_CONTROL_NRF_AUXPLL_DEFINE)
