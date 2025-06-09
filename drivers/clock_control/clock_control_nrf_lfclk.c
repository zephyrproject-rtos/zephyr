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

#define LFCLK_LFRC_ACCURACY DT_INST_PROP(0, lfrc_accuracy_ppm)
#define LFCLK_HFXO_ACCURACY DT_PROP(LFCLK_HFXO_NODE, accuracy_ppm)
#define LFCLK_LFLPRC_STARTUP_TIME_US DT_INST_PROP(0, lflprc_startup_time_us)
#define LFCLK_LFRC_STARTUP_TIME_US DT_INST_PROP(0, lfrc_startup_time_us)

#define LFCLK_MAX_OPTS 4
#define LFCLK_DEF_OPTS 2

#define NRFS_CLOCK_TIMEOUT K_MSEC(CONFIG_CLOCK_CONTROL_NRF_LFCLK_CLOCK_TIMEOUT_MS)

#define BICR (NRF_BICR_Type *)DT_REG_ADDR(DT_NODELABEL(bicr))

/* Clock options sorted from highest to lowest power consumption.
 * - Clock synthesized from a high frequency clock
 * - Internal RC oscillator
 * - External clock. These are inserted into the list at driver initialization.
 *   Set to one of the following:
 *   - XTAL. Low or High precision
 *   - External sine or square wave
 */
static struct clock_options {
	uint16_t accuracy : 15;
	uint16_t precision : 1;
	nrfs_clock_src_t src;
} clock_options[LFCLK_MAX_OPTS] = {
	{
		/* NRFS will request FLL16M use HFXO in bypass mode if SYNTH src is used */
		.accuracy = LFCLK_HFXO_ACCURACY,
		.precision = 1,
		.src = NRFS_CLOCK_SRC_LFCLK_SYNTH,
	},
	{
		.accuracy = LFCLK_LFRC_ACCURACY,
		.precision = 0,
		.src = NRFS_CLOCK_SRC_LFCLK_LFRC,
	},
	/* Remaining options are populated on lfclk_init */
};

struct lfclk_dev_data {
	STRUCT_CLOCK_CONFIG(lfclk, ARRAY_SIZE(clock_options)) clk_cfg;
	struct k_timer timer;
	uint16_t max_accuracy;
	uint8_t clock_options_cnt;
	uint32_t hfxo_startup_time_us;
	uint32_t lfxo_startup_time_us;
};

struct lfclk_dev_config {
	uint32_t fixed_frequency;
};

static int lfosc_get_accuracy(uint16_t *accuracy)
{
	switch (nrf_bicr_lfosc_accuracy_get(BICR)) {
	case NRF_BICR_LFOSC_ACCURACY_500PPM:
		*accuracy = 500U;
		break;
	case NRF_BICR_LFOSC_ACCURACY_250PPM:
		*accuracy = 250U;
		break;
	case NRF_BICR_LFOSC_ACCURACY_150PPM:
		*accuracy = 150U;
		break;
	case NRF_BICR_LFOSC_ACCURACY_100PPM:
		*accuracy = 100U;
		break;
	case NRF_BICR_LFOSC_ACCURACY_75PPM:
		*accuracy = 75U;
		break;
	case NRF_BICR_LFOSC_ACCURACY_50PPM:
		*accuracy = 50U;
		break;
	case NRF_BICR_LFOSC_ACCURACY_30PPM:
		*accuracy = 30U;
		break;
	case NRF_BICR_LFOSC_ACCURACY_20PPM:
		*accuracy = 20U;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

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

static int lfclk_resolve_spec_to_idx(const struct device *dev,
				     const struct nrf_clock_spec *req_spec)
{
	struct lfclk_dev_data *dev_data = dev->data;
	const struct lfclk_dev_config *dev_config = dev->config;
	uint16_t req_accuracy;

	if (req_spec->frequency > dev_config->fixed_frequency) {
		LOG_ERR("invalid frequency");
		return -EINVAL;
	}

	req_accuracy = req_spec->accuracy == NRF_CLOCK_CONTROL_ACCURACY_MAX
		     ? dev_data->max_accuracy
		     : req_spec->accuracy;

	for (int i = dev_data->clock_options_cnt - 1; i >= 0; --i) {
		/* Iterate to a more power hungry and accurate clock source
		 * If the requested accuracy is higher (lower ppm) than what
		 * the clock source can provide.
		 *
		 * In case of an accuracy of 0 (don't care), do not check accuracy.
		 */
		if ((req_accuracy != 0 && req_accuracy < clock_options[i].accuracy) ||
		    (req_spec->precision > clock_options[i].precision)) {
			continue;
		}

		return i;
	}

	LOG_ERR("invalid accuracy or precision");
	return -EINVAL;
}

static void lfclk_get_spec_by_idx(const struct device *dev,
				  uint8_t idx,
				  struct nrf_clock_spec *spec)
{
	const struct lfclk_dev_config *dev_config = dev->config;

	spec->frequency = dev_config->fixed_frequency;
	spec->accuracy = clock_options[idx].accuracy;
	spec->precision = clock_options[idx].precision;
}

static struct onoff_manager *lfclk_get_mgr_by_idx(const struct device *dev, uint8_t idx)
{
	struct lfclk_dev_data *dev_data = dev->data;

	return &dev_data->clk_cfg.onoff[idx].mgr;
}

static int lfclk_get_startup_time_by_idx(const struct device *dev,
					 uint8_t idx,
					 uint32_t *startup_time_us)
{
	struct lfclk_dev_data *dev_data = dev->data;
	nrfs_clock_src_t src = clock_options[idx].src;

	switch (src) {
	case NRFS_CLOCK_SRC_LFCLK_LFLPRC:
		*startup_time_us = LFCLK_LFLPRC_STARTUP_TIME_US;
		return 0;

	case NRFS_CLOCK_SRC_LFCLK_LFRC:
		*startup_time_us = LFCLK_LFRC_STARTUP_TIME_US;
		return 0;

	case NRFS_CLOCK_SRC_LFCLK_XO_PIXO:
	case NRFS_CLOCK_SRC_LFCLK_XO_PIERCE:
	case NRFS_CLOCK_SRC_LFCLK_XO_EXT_SINE:
	case NRFS_CLOCK_SRC_LFCLK_XO_EXT_SQUARE:
	case NRFS_CLOCK_SRC_LFCLK_XO_PIERCE_HP:
	case NRFS_CLOCK_SRC_LFCLK_XO_EXT_SINE_HP:
		*startup_time_us = dev_data->lfxo_startup_time_us;
		return 0;

	case NRFS_CLOCK_SRC_LFCLK_SYNTH:
		*startup_time_us = dev_data->hfxo_startup_time_us;
		return 0;

	default:
		break;
	}

	return -EINVAL;
}

static struct onoff_manager *lfclk_find_mgr_by_spec(const struct device *dev,
						    const struct nrf_clock_spec *spec)
{
	int idx;

	if (!spec) {
		return lfclk_get_mgr_by_idx(dev, 0);
	}

	idx = lfclk_resolve_spec_to_idx(dev, spec);
	return idx < 0 ? NULL : lfclk_get_mgr_by_idx(dev, idx);
}

static int api_request_lfclk(const struct device *dev,
			     const struct nrf_clock_spec *spec,
			     struct onoff_client *cli)
{
	struct onoff_manager *mgr = lfclk_find_mgr_by_spec(dev, spec);

	if (mgr) {
		return clock_config_request(mgr, cli);
	}

	return -EINVAL;
}

static int api_release_lfclk(const struct device *dev,
			     const struct nrf_clock_spec *spec)
{
	struct onoff_manager *mgr = lfclk_find_mgr_by_spec(dev, spec);

	if (mgr) {
		return onoff_release(mgr);
	}

	return -EINVAL;
}

static int api_cancel_or_release_lfclk(const struct device *dev,
				       const struct nrf_clock_spec *spec,
				       struct onoff_client *cli)
{
	struct onoff_manager *mgr = lfclk_find_mgr_by_spec(dev, spec);

	if (mgr) {
		return onoff_cancel_or_release(mgr, cli);
	}

	return -EINVAL;
}


static int api_resolve(const struct device *dev,
		       const struct nrf_clock_spec *req_spec,
		       struct nrf_clock_spec *res_spec)
{
	int idx;

	idx = lfclk_resolve_spec_to_idx(dev, req_spec);
	if (idx < 0) {
		return -EINVAL;
	}

	lfclk_get_spec_by_idx(dev, idx, res_spec);
	return 0;
}

static int api_get_startup_time(const struct device *dev,
				const struct nrf_clock_spec *spec,
				uint32_t *startup_time_us)
{
	int idx;

	idx = lfclk_resolve_spec_to_idx(dev, spec);
	if (idx < 0) {
		return -EINVAL;
	}

	return lfclk_get_startup_time_by_idx(dev, idx, startup_time_us);
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
			clock_options[LFCLK_MAX_OPTS - 1].accuracy = dev_data->max_accuracy;
			clock_options[LFCLK_MAX_OPTS - 1].precision = 0;
			clock_options[LFCLK_MAX_OPTS - 1].src = NRFS_CLOCK_SRC_LFCLK_XO_PIERCE;

			clock_options[LFCLK_MAX_OPTS - 2].accuracy = dev_data->max_accuracy;
			clock_options[LFCLK_MAX_OPTS - 2].precision = 1;
			clock_options[LFCLK_MAX_OPTS - 2].src = NRFS_CLOCK_SRC_LFCLK_XO_PIERCE_HP;

			dev_data->clock_options_cnt += 2;
			break;
		case NRF_BICR_LFOSC_MODE_EXTSINE:
			clock_options[LFCLK_MAX_OPTS - 1].accuracy = dev_data->max_accuracy;
			clock_options[LFCLK_MAX_OPTS - 1].precision = 0;
			clock_options[LFCLK_MAX_OPTS - 1].src = NRFS_CLOCK_SRC_LFCLK_XO_EXT_SINE;

			clock_options[LFCLK_MAX_OPTS - 2].accuracy = dev_data->max_accuracy;
			clock_options[LFCLK_MAX_OPTS - 2].precision = 1;
			clock_options[LFCLK_MAX_OPTS - 2].src = NRFS_CLOCK_SRC_LFCLK_XO_EXT_SINE_HP;

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

		dev_data->lfxo_startup_time_us = nrf_bicr_lfosc_startup_time_ms_get(BICR)
					       * USEC_PER_MSEC;
		if (dev_data->lfxo_startup_time_us == NRF_BICR_LFOSC_STARTUP_TIME_UNCONFIGURED) {
			LOG_ERR("BICR LFXO startup time invalid");
			return -ENODEV;
		}
	}

	dev_data->hfxo_startup_time_us = nrf_bicr_hfxo_startup_time_us_get(BICR);
	if (dev_data->hfxo_startup_time_us == NRF_BICR_HFXO_STARTUP_TIME_UNCONFIGURED) {
		LOG_ERR("BICR HFXO startup time invalid");
		return -ENODEV;
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
	.resolve = api_resolve,
	.get_startup_time = api_get_startup_time,
};

static struct lfclk_dev_data lfclk_data;

static const struct lfclk_dev_config lfclk_config = {
	.fixed_frequency = DT_INST_PROP(0, clock_frequency),
};

DEVICE_DT_INST_DEFINE(0, lfclk_init, NULL,
		      &lfclk_data, &lfclk_config,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
		      &lfclk_drv_api);
