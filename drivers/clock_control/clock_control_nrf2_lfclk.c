/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_lfclk

#include "clock_control_nrf2_common.h"
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(clock_control_nrf2, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#include <nrfs_clock.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "multiple instances not supported");

#define NRF2_LFCLK_HAS_LFXO									\
	(DT_INST_NODE_HAS_PROP(0, lfxo_clock) &&						\
	 DT_NODE_HAS_STATUS(DT_INST_PROP(0, lfxo_clock), okay))

#if NRF2_LFCLK_HAS_LFXO
#define NRF2_LFCLK_MAX_ACCURACY DT_INST_PROP(0, lfxo_accuracy_ppm)
#else
#define NRF2_LFCLK_MAX_ACCURACY DT_INST_PROP(0, synth_accuracy_ppm)
#endif

#define NRFS_CLOCK_TIMEOUT K_MSEC(CONFIG_CLOCK_CONTROL_NRF2_NRFS_CLOCK_TIMEOUT_MS)

static const struct clock_options {
	uint16_t accuracy : 15;
	uint16_t precision : 1;
	nrfs_clock_src_t src;
} clock_options[] = {
	{
		.accuracy = DT_INST_PROP(0, lflprc_accuracy_ppm),
		.precision = 0,
		.src = NRFS_CLOCK_SRC_LFCLK_LFLPRC,
	},
	{
		.accuracy = DT_INST_PROP(0, lfrc_accuracy_ppm),
		.precision = 0,
		.src = NRFS_CLOCK_SRC_LFCLK_LFRC,
	},
	{
		.accuracy = DT_INST_PROP(0, synth_accuracy_ppm),
		.precision = 1,
		.src = NRFS_CLOCK_SRC_LFCLK_SYNTH,
	},
#if NRF2_LFCLK_HAS_LFXO
#if DT_ENUM_HAS_VALUE(DT_INST_PROP(0, lfxo_clock), mode, crystal)
	{
		.accuracy = DT_INST_PROP(0, lfxo_accuracy_ppm),
		.src = NRFS_CLOCK_SRC_LFCLK_XO_PIERCE,
	},
	{
		.accuracy = DT_INST_PROP(0, lfxo_accuracy_ppm),
		.precision = 1,
		.src = NRFS_CLOCK_SRC_LFCLK_XO_PIERCE_HP,
	},
#elif DT_ENUM_HAS_VALUE(DT_INST_PROP(0, lfxo_clock), mode, external_sine)
	{
		.accuracy = DT_INST_PROP(0, lfxo_accuracy_ppm),
		.precision = 0,
		.src = NRFS_CLOCK_SRC_LFCLK_XO_EXT_SINE,
	},
	{
		.accuracy = DT_INST_PROP(0, lfxo_accuracy_ppm),
		.precision = 1,
		.src = NRFS_CLOCK_SRC_LFCLK_XO_EXT_SINE_HP,
	},
#elif DT_ENUM_HAS_VALUE(DT_INST_PROP(0, lfxo_clock), mode, external_square)
	{
		.accuracy = DT_INST_PROP(0, lfxo_accuracy_ppm),
		.precision = 0,
		.src = NRFS_CLOCK_SRC_LFCLK_XO_EXT_SQUARE,
	},
#else
#error "unsupported LFXO mode"
#endif
#endif
};

struct lfclk_dev_data {
	NRF2_STRUCT_CLOCK_CONFIG(lfclk, ARRAY_SIZE(clock_options)) clk_cfg;
	struct k_timer timer;
};

struct lfclk_dev_config {
	uint32_t fixed_frequency;
};

static void clock_evt_handler(nrfs_clock_evt_t const *p_evt, void *context)
{
	struct lfclk_dev_data *dev_data = context;
	int status = 0;

	k_timer_stop(&dev_data->timer);

	if (p_evt->type == NRFS_CLOCK_EVT_REJECT) {
		status = -ENXIO;
	}

	nrf2_clock_config_update_end(&dev_data->clk_cfg, status);
}

static void lfclk_update_timeout_handler(struct k_timer *timer)
{
	struct lfclk_dev_data *dev_data =
		CONTAINER_OF(timer, struct lfclk_dev_data, timer);

	nrf2_clock_config_update_end(&dev_data->clk_cfg, -ETIMEDOUT);
}

static void lfclk_work_handler(struct k_work *work)
{
	struct lfclk_dev_data *dev_data =
		CONTAINER_OF(work, struct lfclk_dev_data, clk_cfg.work);
	uint8_t to_activate_idx;
	nrfs_err_t err;

	to_activate_idx = nrf2_clock_config_update_begin(work);

	err = nrfs_clock_lfclk_src_set(clock_options[to_activate_idx].src,
				       dev_data);
	if (err != NRFS_SUCCESS) {
		nrf2_clock_config_update_end(&dev_data->clk_cfg, -EIO);
	} else {
		k_timer_start(&dev_data->timer, NRFS_CLOCK_TIMEOUT, K_NO_WAIT);
	}
}

static struct onoff_manager *lfclk_find_mgr(const struct device *dev,
					    const struct nrf_clock_spec *spec)
{
	struct lfclk_dev_data *dev_data = dev->data;
	const struct lfclk_dev_config *dev_config = dev->config;
	uint16_t accuracy;

	if (!spec) {
		return &dev_data->clk_cfg.onoff[0].mgr;
	}

	if (spec->frequency > dev_config->fixed_frequency) {
		LOG_ERR("invalid frequency");
		return NULL;
	}

	accuracy = spec->accuracy == NRF_CLOCK_CONTROL_ACCURACY_MAX
		 ? NRF2_LFCLK_MAX_ACCURACY
		 : spec->accuracy;

	for (int i = 0; i < ARRAY_SIZE(clock_options); ++i) {
		if ((accuracy &&
		     accuracy < clock_options[i].accuracy) ||
		    spec->precision > clock_options[i].precision) {
			continue;
		}

		return &dev_data->clk_cfg.onoff[i].mgr;
	}

	LOG_ERR("invalid accuracy or precision");
	return NULL;
}

static int api_request_lfclk(const struct device *dev,
			     const struct nrf_clock_spec *spec,
			     struct onoff_client *cli)
{
	struct onoff_manager *mgr = lfclk_find_mgr(dev, spec);

	if (mgr) {
		return onoff_request(mgr, cli);
	}

	return -EINVAL;
}

static int api_release_lfclk(const struct device *dev,
			     const struct nrf_clock_spec *spec)
{
	struct onoff_manager *mgr = lfclk_find_mgr(dev, spec);

	if (mgr) {
		return onoff_release(mgr);
	}

	return -EINVAL;
}

static int api_cancel_or_release_lfclk(const struct device *dev,
				       const struct nrf_clock_spec *spec,
				       struct onoff_client *cli)
{
	struct onoff_manager *mgr = lfclk_find_mgr(dev, spec);

	if (mgr) {
		return onoff_cancel_or_release(mgr, cli);
	}

	return -EINVAL;
}

static int api_get_rate_lfclk(const struct device *dev,
			      clock_control_subsys_t sys,
			      uint32_t *rate)
{
	ARG_UNUSED(sys);

	const struct lfclk_dev_config *dev_config = dev->config;

	*rate = dev_config->fixed_frequency;

	return 0;
}

static int lfclk_init(const struct device *dev)
{
	struct lfclk_dev_data *dev_data = dev->data;
	nrfs_err_t res;

	res = nrfs_clock_init(clock_evt_handler);
	if (res != NRFS_SUCCESS) {
		return -EIO;
	}

	k_timer_init(&dev_data->timer, lfclk_update_timeout_handler, NULL);

	return nrf2_clock_config_init(&dev_data->clk_cfg,
				      ARRAY_SIZE(dev_data->clk_cfg.onoff),
				      lfclk_work_handler);
}

static struct nrf_clock_control_driver_api lfclk_drv_api = {
	.std_api = {
		.on = api_nosys_on_off,
		.off = api_nosys_on_off,
		.get_rate = api_get_rate_lfclk,
	},
	.request = api_request_lfclk,
	.release = api_release_lfclk,
	.cancel_or_release = api_cancel_or_release_lfclk,
};

static struct lfclk_dev_data lfclk_data;

static const struct lfclk_dev_config lfclk_config = {
	.fixed_frequency = DT_INST_PROP(0, clock_frequency),
};

DEVICE_DT_INST_DEFINE(0, lfclk_init, NULL,
		      &lfclk_data, &lfclk_config,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &lfclk_drv_api);
