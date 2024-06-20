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

/* TODO: get from Kconfig */
#define UPDATE_TIMEOUT K_MSEC(10)

#define HFXO_NODE DT_NODELABEL(hfxo)
#define LFXO_NODE DT_NODELABEL(lfxo)
#define HFXO_ACCURACY \
	NRF_CLOCK_CONTROL_ACCURACY_PPM(DT_PROP(HFXO_NODE, accuracy_ppm))
#define LFXO_ACCURACY \
	NRF_CLOCK_CONTROL_ACCURACY_PPM(DT_PROP(LFXO_NODE, accuracy_ppm))

static const struct clock_options {
	uint16_t accuracy : 15;
	uint16_t precision : 1;
	nrfs_clock_src_t src;
} clock_options[] = {
#if !DT_NODE_HAS_STATUS(LFXO_NODE, okay)
	{
		/* TODO: use real accuracy and precision values */
		.accuracy = NRF_CLOCK_CONTROL_ACCURACY_PPM(1000),
		.precision = 0,
		.src = NRFS_CLOCK_SRC_LFCLK_LFLPRC,
	},
	{
		/* TODO: use real accuracy and precision values */
		.accuracy = NRF_CLOCK_CONTROL_ACCURACY_PPM(500),
		.precision = 0,
		.src = NRFS_CLOCK_SRC_LFCLK_LFRC,
	},
	{
		.accuracy = HFXO_ACCURACY,
		.precision = 1,
		.src = NRFS_CLOCK_SRC_LFCLK_SYNTH,
	},
	#define MAX_ACCURACY HFXO_ACCURACY
#elif DT_ENUM_HAS_VALUE(LFXO_NODE, mode, Crystal)
	#if 0 /* not to be used currently */
	{
		.accuracy = /* TODO: get from devicetree (?) */,
		.precision = 0,
		.src = NRFS_CLOCK_SRC_LFCLK_XO_PIXO,
	},
	#endif
	{
		.accuracy = LFXO_ACCURACY,
		.precision = 0,
		.src = NRFS_CLOCK_SRC_LFCLK_XO_PIERCE,
	},
	{
		.accuracy = LFXO_ACCURACY,
		.precision = 1,
		.src = NRFS_CLOCK_SRC_LFCLK_XO_PIERCE_HP,
	},
	#define MAX_ACCURACY LFXO_ACCURACY
#elif DT_ENUM_HAS_VALUE(LFXO_NODE, mode, ExtSine)
	{
		.accuracy = LFXO_ACCURACY,
		.precision = 0,
		.src = NRFS_CLOCK_SRC_LFCLK_XO_EXT_SINE,
	},
	{
		.accuracy = LFXO_ACCURACY,
		.precision = 1,
		.src = NRFS_CLOCK_SRC_LFCLK_XO_EXT_SINE_HP,
	},
	#define MAX_ACCURACY LFXO_ACCURACY
#elif DT_ENUM_HAS_VALUE(LFXO_NODE, mode, ExtSquare)
	{
		.accuracy = LFXO_ACCURACY,
		.precision = 0,
		.src = NRFS_CLOCK_SRC_LFCLK_XO_EXT_SQUARE,
	},
	#define MAX_ACCURACY LFXO_ACCURACY
#else
#error "Unsupported LFXO mode."
#endif
};
static const uint16_t max_accuracy = MAX_ACCURACY;

struct lfclk_dev_data {
	STRUCT_CLOCK_CONFIG(lfclk, ARRAY_SIZE(clock_options)) clk_cfg;
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

	clock_config_update_end(&dev_data->clk_cfg, status);
}

static void lfclk_update_timeout_handler(struct k_timer *timer)
{
	struct lfclk_dev_data *dev_data =
		CONTAINER_OF(timer, struct lfclk_dev_data, timer);

	clock_config_update_end(&dev_data->clk_cfg, -ETIMEDOUT);
}

static void lfclk_work_handler(struct k_work *work)
{
	struct lfclk_dev_data *dev_data =
		CONTAINER_OF(work, struct lfclk_dev_data, clk_cfg.work);
	uint8_t to_activate_idx;
	nrfs_err_t err;

	to_activate_idx = clock_config_update_begin(work);

	err = nrfs_clock_lfclk_src_set(clock_options[to_activate_idx].src,
				       dev_data);
	if (err != NRFS_SUCCESS) {
		clock_config_update_end(&dev_data->clk_cfg, -EIO);
	} else {
		k_timer_start(&dev_data->timer, UPDATE_TIMEOUT, K_NO_WAIT);
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
		 ? max_accuracy
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

	return clock_config_init(&dev_data->clk_cfg,
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
