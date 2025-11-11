/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_auxpll

#include <errno.h>
#include <stdint.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>
#include <zephyr/dt-bindings/clock/nrf-auxpll.h>
#include "clock_control_nrf2_common.h"

#include <hal/nrf_auxpll.h>


/* Check dt-bindings match MDK frequency division definitions*/
BUILD_ASSERT(NRF_AUXPLL_FREQ_DIV_MIN		== NRF_AUXPLL_FREQUENCY_DIV_MIN,
	"Different AUXPLL_FREQ_DIV_MIN definition in MDK and devicetree binding");
BUILD_ASSERT(NRF_AUXPLL_FREQ_DIV_AUDIO_44K1	== NRF_AUXPLL_FREQUENCY_AUDIO_44K1,
	"Different AUXPLL_FREQ_DIV_AUDIO_44K1 definition in MDK and devicetree binding");
BUILD_ASSERT(NRF_AUXPLL_FREQ_DIV_USB24M		== NRF_AUXPLL_FREQUENCY_USB_24M,
	"Different AUXPLL_FREQ_DIV_USB24M definition in MDK and devicetree binding");
BUILD_ASSERT(NRF_AUXPLL_FREQ_DIV_AUDIO_48K	== NRF_AUXPLL_FREQUENCY_AUDIO_48K,
	"Different AUXPLL_FREQ_DIV_AUDIO_48K definition in MDK and devicetree binding");
BUILD_ASSERT(NRF_AUXPLL_FREQ_DIV_MAX		== NRF_AUXPLL_FREQUENCY_DIV_MAX,
	"Different AUXPLL_FREQ_DIV_MAX definition in MDK and devicetree binding");

/* maximum lock time in us, >10x time observed experimentally */
#define AUXPLL_LOCK_TIME_MAX_US  20000
/* lock wait step in us*/
#define AUXPLL_LOCK_WAIT_STEP_US 1000

struct dev_data_auxpll {
	struct onoff_manager mgr;
	onoff_notify_fn notify;
	const struct device *dev;
};

struct clock_control_nrf_auxpll_config {
	NRF_AUXPLL_Type *auxpll;
	uint32_t ref_clk_hz;
	uint32_t ficr_ctune;
	nrf_auxpll_config_t cfg;
	nrf_auxpll_freq_div_ratio_t frequency;
	nrf_auxpll_ctrl_outsel_t out_div;
};

static int clock_control_nrf_auxpll_on(struct dev_data_auxpll *dev_data)
{
	const struct clock_control_nrf_auxpll_config *config = dev_data->dev->config;
	bool locked;

	nrf_auxpll_task_trigger(config->auxpll, NRF_AUXPLL_TASK_START);

	NRFX_WAIT_FOR(nrf_auxpll_mode_locked_check(config->auxpll),
					AUXPLL_LOCK_TIME_MAX_US / AUXPLL_LOCK_WAIT_STEP_US,
					AUXPLL_LOCK_WAIT_STEP_US, locked);

	return locked ? 0 : -ETIMEDOUT;
}

static int clock_control_nrf_auxpll_off(struct dev_data_auxpll *dev_data)
{
	const struct clock_control_nrf_auxpll_config *config = dev_data->dev->config;

	nrf_auxpll_task_trigger(config->auxpll, NRF_AUXPLL_TASK_STOP);

	while (nrf_auxpll_running_check(config->auxpll)) {
	}

	return 0;
}

static void onoff_start_auxpll(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	struct dev_data_auxpll *dev_data =
		CONTAINER_OF(mgr, struct dev_data_auxpll, mgr);

	int ret = clock_control_nrf_auxpll_on(dev_data);

	notify(&dev_data->mgr, ret);

}

static void onoff_stop_auxpll(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	struct dev_data_auxpll *dev_data =
		CONTAINER_OF(mgr, struct dev_data_auxpll, mgr);

	clock_control_nrf_auxpll_off(dev_data);
	notify(mgr, 0);
}

static int api_request_auxpll(const struct device *dev,
			      const struct nrf_clock_spec *spec,
			      struct onoff_client *cli)
{
	struct dev_data_auxpll *dev_data = dev->data;

	ARG_UNUSED(spec);

	return onoff_request(&dev_data->mgr, cli);
}

static int api_release_auxpll(const struct device *dev,
			      const struct nrf_clock_spec *spec)
{
	struct dev_data_auxpll *dev_data = dev->data;

	ARG_UNUSED(spec);

	return onoff_release(&dev_data->mgr);
}

static int api_cancel_or_release_auxpll(const struct device *dev,
					const struct nrf_clock_spec *spec,
					struct onoff_client *cli)
{
	struct dev_data_auxpll *dev_data = dev->data;

	ARG_UNUSED(spec);

	return onoff_cancel_or_release(&dev_data->mgr, cli);
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

static const struct onoff_transitions transitions = {
	.start = onoff_start_auxpll,
	.stop = onoff_stop_auxpll
};

static int clock_control_nrf_auxpll_init(const struct device *dev)
{
	struct dev_data_auxpll *dev_data = dev->data;
	const struct clock_control_nrf_auxpll_config *config = dev->config;
	int rc;

	rc = onoff_manager_init(&dev_data->mgr, &transitions);
	if (rc < 0) {
		return rc;
	}

	nrf_auxpll_ctrl_frequency_set(config->auxpll, config->frequency);

	nrf_auxpll_lock(config->auxpll);
	nrf_auxpll_trim_ctune_set(config->auxpll, sys_read8(config->ficr_ctune));
	nrf_auxpll_config_set(config->auxpll, &config->cfg);
	nrf_auxpll_ctrl_outsel_set(config->auxpll, config->out_div);
	nrf_auxpll_unlock(config->auxpll);

	nrf_auxpll_ctrl_mode_set(config->auxpll, NRF_AUXPLL_CTRL_MODE_LOCKED);

	return 0;
}

static DEVICE_API(nrf_clock_control, drv_api_auxpll) = {
	.std_api = {
		.on = api_nosys_on_off,
		.off = api_nosys_on_off,
		.get_rate = clock_control_nrf_auxpll_get_rate,
		.get_status = clock_control_nrf_auxpll_get_status,
	},
	.request = api_request_auxpll,
	.release = api_release_auxpll,
	.cancel_or_release = api_cancel_or_release_auxpll,
};

#define CLOCK_CONTROL_NRF_AUXPLL_DEFINE(n)                                                         \
	BUILD_ASSERT(                                                                              \
		DT_INST_PROP(n, nordic_frequency) == NRF_AUXPLL_FREQUENCY_DIV_MIN     ||           \
		DT_INST_PROP(n, nordic_frequency) == NRF_AUXPLL_FREQUENCY_AUDIO_44K1  ||           \
		DT_INST_PROP(n, nordic_frequency) == NRF_AUXPLL_FREQUENCY_USB_24M     ||           \
		DT_INST_PROP(n, nordic_frequency) == NRF_AUXPLL_FREQUENCY_AUDIO_48K   ||           \
		DT_INST_PROP(n, nordic_frequency) == NRF_AUXPLL_FREQUENCY_DIV_MAX,                 \
		"Invalid nordic,frequency value in DeviceTree for AUXPLL instance " #n);           \
	BUILD_ASSERT(DT_INST_PROP(n, nordic_out_div) > 0,                                          \
		"nordic,out_div must be greater than 0 for AUXPLL instance " #n);                  \
	static struct dev_data_auxpll data_auxpll##n    = {                                        \
		.dev = DEVICE_DT_INST_GET(n),                                                      \
	};                                                                                         \
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
	DEVICE_DT_INST_DEFINE(n, clock_control_nrf_auxpll_init, NULL, &data_auxpll##n, &config##n, \
			      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,                    \
			      &drv_api_auxpll);

DT_INST_FOREACH_STATUS_OKAY(CLOCK_CONTROL_NRF_AUXPLL_DEFINE)
