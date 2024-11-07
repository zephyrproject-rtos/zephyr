/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_lfclk

#include "clock_control_nrf2_common.h"
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <hal/nrf_bicr.h>
#include <nrfs_clock.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(clock_control_nrf2, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "multiple instances not supported");

#define LFCLK_HFXO_NODE DT_INST_PHANDLE_BY_NAME(0, clocks, hfxo)

#define LFCLK_LFLPRC_ACCURACY DT_INST_PROP(0, lflprc_accuracy_ppm)
#define LFCLK_LFRC_ACCURACY DT_INST_PROP(0, lfrc_accuracy_ppm)
#define LFCLK_HFXO_ACCURACY DT_PROP(LFCLK_HFXO_NODE, accuracy_ppm)

#define LFCLK_MAX_OPTS 5
#define LFCLK_DEF_OPTS 3

#define NRFS_CLOCK_TIMEOUT K_MSEC(CONFIG_CLOCK_CONTROL_NRF2_NRFS_CLOCK_TIMEOUT_MS)

#define BICR (NRF_BICR_Type *)DT_REG_ADDR(DT_NODELABEL(bicr))

/* Clock options sorted from lowest to highest accuracy/precision */
static struct clock_options {
	uint16_t accuracy : 15;
	uint16_t precision : 1;
	nrfs_clock_src_t src;
} clock_options[LFCLK_MAX_OPTS] = {
	{
		.accuracy = LFCLK_LFLPRC_ACCURACY,
		.precision = 0,
		.src = NRFS_CLOCK_SRC_LFCLK_LFLPRC,
	},
	{
		.accuracy = LFCLK_LFRC_ACCURACY,
		.precision = 0,
		.src = NRFS_CLOCK_SRC_LFCLK_LFRC,
	},
	{
		/* NRFS will request FLL16M use HFXO in bypass mode if SYNTH src is used */
		.accuracy = LFCLK_HFXO_ACCURACY,
		.precision = 1,
		.src = NRFS_CLOCK_SRC_LFCLK_SYNTH,
	},
};

struct lfclk_dev_data {
	STRUCT_CLOCK_CONFIG(lfclk, ARRAY_SIZE(clock_options)) clk_cfg;
	struct k_timer timer;
	uint16_t max_accuracy;
	uint8_t clock_options_cnt;
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
		 ? dev_data->max_accuracy
		 : spec->accuracy;

	for (int i = 0; i < dev_data->clock_options_cnt; ++i) {
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
	nrf_bicr_lfosc_mode_t lfosc_mode;
	nrfs_err_t res;

	res = nrfs_clock_init(clock_evt_handler);
	if (res != NRFS_SUCCESS) {
		return -EIO;
	}

	dev_data->clock_options_cnt = LFCLK_DEF_OPTS;

	lfosc_mode = nrf_bicr_lfosc_mode_get(BICR);

	if (lfosc_mode == NRF_BICR_LFOSC_MODE_UNCONFIGURED ||
	    lfosc_mode == NRF_BICR_LFOSC_MODE_DISABLED) {
		dev_data->max_accuracy = LFCLK_HFXO_ACCURACY;
	} else {
		int ret;

		ret = lfosc_get_accuracy(&dev_data->max_accuracy);
		if (ret < 0) {
			LOG_ERR("LFOSC enabled with invalid accuracy");
			return ret;
		}

		switch (lfosc_mode) {
		case NRF_BICR_LFOSC_MODE_CRYSTAL:
			clock_options[LFCLK_MAX_OPTS - 2].accuracy = dev_data->max_accuracy;
			clock_options[LFCLK_MAX_OPTS - 2].precision = 0;
			clock_options[LFCLK_MAX_OPTS - 2].src = NRFS_CLOCK_SRC_LFCLK_XO_PIERCE;

			clock_options[LFCLK_MAX_OPTS - 1].accuracy = dev_data->max_accuracy;
			clock_options[LFCLK_MAX_OPTS - 1].precision = 1;
			clock_options[LFCLK_MAX_OPTS - 1].src = NRFS_CLOCK_SRC_LFCLK_XO_PIERCE_HP;

			dev_data->clock_options_cnt += 2;
			break;
		case NRF_BICR_LFOSC_MODE_EXTSINE:
			clock_options[LFCLK_MAX_OPTS - 2].accuracy = dev_data->max_accuracy;
			clock_options[LFCLK_MAX_OPTS - 2].precision = 0;
			clock_options[LFCLK_MAX_OPTS - 2].src = NRFS_CLOCK_SRC_LFCLK_XO_EXT_SINE;

			clock_options[LFCLK_MAX_OPTS - 1].accuracy = dev_data->max_accuracy;
			clock_options[LFCLK_MAX_OPTS - 1].precision = 1;
			clock_options[LFCLK_MAX_OPTS - 1].src = NRFS_CLOCK_SRC_LFCLK_XO_EXT_SINE_HP;

			dev_data->clock_options_cnt += 2;
			break;
		case NRF_BICR_LFOSC_MODE_EXTSQUARE:
			clock_options[LFCLK_MAX_OPTS - 2].accuracy = dev_data->max_accuracy;
			clock_options[LFCLK_MAX_OPTS - 2].precision = 0;
			clock_options[LFCLK_MAX_OPTS - 2].src = NRFS_CLOCK_SRC_LFCLK_XO_EXT_SQUARE;

			dev_data->clock_options_cnt += 1;
			break;
		default:
			LOG_ERR("Unexpected LFOSC mode");
			return -EINVAL;
		}
	}

	k_timer_init(&dev_data->timer, lfclk_update_timeout_handler, NULL);

	return clock_config_init(&dev_data->clk_cfg,
				 ARRAY_SIZE(dev_data->clk_cfg.onoff),
				 lfclk_work_handler);
}

static DEVICE_API(nrf_clock_control, lfclk_drv_api) = {
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
