/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_hsfll_global

#include "clock_control_nrf2_common.h"
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <nrfs_gdfs.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(clock_control_nrf2, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define GLOBAL_HSFLL_CLOCK_FREQUENCIES \
	DT_INST_PROP(0, supported_clock_frequencies)

#define GLOBAL_HSFLL_CLOCK_FREQUENCIES_IDX(idx) \
	DT_INST_PROP_BY_IDX(0, supported_clock_frequencies, idx)

#define GLOBAL_HSFLL_CLOCK_FREQUENCIES_SIZE \
	DT_INST_PROP_LEN(0, supported_clock_frequencies)

#define GLOBAL_HSFLL_FREQ_REQ_TIMEOUT \
	K_MSEC(CONFIG_CLOCK_CONTROL_NRF2_GLOBAL_HSFLL_TIMEOUT_MS)

#define GLOBAL_HSFLL_INIT_LOW_REQ \
	CONFIG_CLOCK_CONTROL_NRF2_GLOBAL_HSFLL_REQ_LOW_FREQ

BUILD_ASSERT(GLOBAL_HSFLL_CLOCK_FREQUENCIES_SIZE == 4);
BUILD_ASSERT(GLOBAL_HSFLL_CLOCK_FREQUENCIES_IDX(0) == 64000000);
BUILD_ASSERT(GLOBAL_HSFLL_CLOCK_FREQUENCIES_IDX(1) == 128000000);
BUILD_ASSERT(GLOBAL_HSFLL_CLOCK_FREQUENCIES_IDX(2) == 256000000);
BUILD_ASSERT(GLOBAL_HSFLL_CLOCK_FREQUENCIES_IDX(3) == 320000000);
BUILD_ASSERT(GDFS_FREQ_COUNT == 4);
BUILD_ASSERT(GDFS_FREQ_HIGH == 0);
BUILD_ASSERT(GDFS_FREQ_MEDHIGH == 1);
BUILD_ASSERT(GDFS_FREQ_MEDLOW == 2);
BUILD_ASSERT(GDFS_FREQ_LOW == 3);

struct global_hsfll_dev_config {
	uint32_t clock_frequencies[GLOBAL_HSFLL_CLOCK_FREQUENCIES_SIZE];
};

struct global_hsfll_dev_data {
	STRUCT_CLOCK_CONFIG(global_hsfll, GLOBAL_HSFLL_CLOCK_FREQUENCIES_SIZE) clk_cfg;
	const struct device *dev;
	struct k_work evt_work;
	nrfs_gdfs_evt_type_t evt;
	struct k_work_delayable timeout_dwork;

#if GLOBAL_HSFLL_INIT_LOW_REQ
	struct k_sem evt_sem;
#endif /* GLOBAL_HSFLL_INIT_LOW_REQ */
};

static uint32_t global_hsfll_get_max_clock_frequency(const struct device *dev)
{
	const struct global_hsfll_dev_config *dev_config = dev->config;

	return dev_config->clock_frequencies[ARRAY_SIZE(dev_config->clock_frequencies) - 1];
}

static struct onoff_manager *global_hsfll_find_mgr(const struct device *dev,
						   const struct nrf_clock_spec *spec)
{
	struct global_hsfll_dev_data *dev_data = dev->data;
	const struct global_hsfll_dev_config *dev_config = dev->config;
	uint32_t frequency;

	if (!spec) {
		return &dev_data->clk_cfg.onoff[0].mgr;
	}

	if (spec->accuracy || spec->precision) {
		LOG_ERR("invalid specification of accuracy or precision");
		return NULL;
	}

	frequency = spec->frequency == NRF_CLOCK_CONTROL_FREQUENCY_MAX
		  ? global_hsfll_get_max_clock_frequency(dev)
		  : spec->frequency;

	for (uint8_t i = 0; i < ARRAY_SIZE(dev_config->clock_frequencies); i++) {
		if (dev_config->clock_frequencies[i] < frequency) {
			continue;
		}

		return &dev_data->clk_cfg.onoff[i].mgr;
	}

	LOG_ERR("invalid frequency");
	return NULL;
}

static int api_request_global_hsfll(const struct device *dev,
				    const struct nrf_clock_spec *spec,
				    struct onoff_client *cli)
{
	struct onoff_manager *mgr = global_hsfll_find_mgr(dev, spec);

	if (mgr) {
		return onoff_request(mgr, cli);
	}

	return -EINVAL;
}

static int api_release_global_hsfll(const struct device *dev,
				    const struct nrf_clock_spec *spec)
{
	struct onoff_manager *mgr = global_hsfll_find_mgr(dev, spec);

	if (mgr) {
		return onoff_release(mgr);
	}

	return -EINVAL;
}

static int api_cancel_or_release_global_hsfll(const struct device *dev,
					      const struct nrf_clock_spec *spec,
					      struct onoff_client *cli)
{
	struct onoff_manager *mgr = global_hsfll_find_mgr(dev, spec);

	if (mgr) {
		return onoff_cancel_or_release(mgr, cli);
	}

	return -EINVAL;
}

static struct nrf_clock_control_driver_api driver_api = {
	.std_api = {
		.on = api_nosys_on_off,
		.off = api_nosys_on_off,
	},
	.request = api_request_global_hsfll,
	.release = api_release_global_hsfll,
	.cancel_or_release = api_cancel_or_release_global_hsfll,
};

static enum gdfs_frequency_setting global_hsfll_freq_idx_to_nrfs_freq(const struct device *dev,
								      uint8_t freq_idx)
{
	const struct global_hsfll_dev_config *dev_config = dev->config;

	return ARRAY_SIZE(dev_config->clock_frequencies) - 1 - freq_idx;
}

static const char *global_hsfll_gdfs_freq_to_str(enum gdfs_frequency_setting freq)
{
	switch (freq) {
	case GDFS_FREQ_HIGH:
		return "GDFS_FREQ_HIGH";
	case GDFS_FREQ_MEDHIGH:
		return "GDFS_FREQ_MEDHIGH";
	case GDFS_FREQ_MEDLOW:
		return "GDFS_FREQ_MEDLOW";
	case GDFS_FREQ_LOW:
		return "GDFS_FREQ_LOW";
	default:
		break;
	}

	return "UNKNOWN";
}

static void global_hsfll_work_handler(struct k_work *work)
{
	struct global_hsfll_dev_data *dev_data =
		CONTAINER_OF(work, struct global_hsfll_dev_data, clk_cfg.work);
	const struct device *dev = dev_data->dev;
	uint8_t freq_idx;
	enum gdfs_frequency_setting target_freq;
	nrfs_err_t err;

	freq_idx = clock_config_update_begin(work);
	target_freq = global_hsfll_freq_idx_to_nrfs_freq(dev, freq_idx);

	LOG_DBG("requesting %s", global_hsfll_gdfs_freq_to_str(target_freq));
	err = nrfs_gdfs_request_freq(target_freq, dev_data);
	if (err != NRFS_SUCCESS) {
		clock_config_update_end(&dev_data->clk_cfg, -EIO);
		return;
	}

	k_work_schedule(&dev_data->timeout_dwork, GLOBAL_HSFLL_FREQ_REQ_TIMEOUT);
}

static void global_hsfll_evt_handler(struct k_work *work)
{
	struct global_hsfll_dev_data *dev_data =
		CONTAINER_OF(work, struct global_hsfll_dev_data, evt_work);
	int rc;

	k_work_cancel_delayable(&dev_data->timeout_dwork);
	rc = dev_data->evt == NRFS_GDFS_EVT_FREQ_CONFIRMED ? 0 : -EIO;
	clock_config_update_end(&dev_data->clk_cfg, rc);
}

#if GLOBAL_HSFLL_INIT_LOW_REQ
static void global_hfsll_nrfs_gdfs_init_evt_handler(nrfs_gdfs_evt_t const *p_evt, void *context)
{
	struct global_hsfll_dev_data *dev_data = context;

	dev_data->evt = p_evt->type;
	k_sem_give(&dev_data->evt_sem);
}
#endif /* GLOBAL_HSFLL_INIT_LOW_REQ */

static void global_hfsll_nrfs_gdfs_evt_handler(nrfs_gdfs_evt_t const *p_evt, void *context)
{
	struct global_hsfll_dev_data *dev_data = context;

	if (k_work_is_pending(&dev_data->evt_work)) {
		return;
	}

	dev_data->evt = p_evt->type;
	k_work_submit(&dev_data->evt_work);
}

static void global_hsfll_timeout_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct global_hsfll_dev_data *dev_data =
		CONTAINER_OF(dwork, struct global_hsfll_dev_data, timeout_dwork);

	clock_config_update_end(&dev_data->clk_cfg, -ETIMEDOUT);
}

static int global_hfsll_init(const struct device *dev)
{
	struct global_hsfll_dev_data *dev_data = dev->data;
	nrfs_err_t err;
	int rc;

	k_work_init_delayable(&dev_data->timeout_dwork, global_hsfll_timeout_handler);
	k_work_init(&dev_data->evt_work, global_hsfll_evt_handler);

#if GLOBAL_HSFLL_INIT_LOW_REQ
	k_sem_init(&dev_data->evt_sem, 0, 1);

	err = nrfs_gdfs_init(global_hfsll_nrfs_gdfs_init_evt_handler);
	if (err != NRFS_SUCCESS) {
		return -EIO;
	}

	LOG_DBG("initial request %s", global_hsfll_gdfs_freq_to_str(GDFS_FREQ_LOW));
	err = nrfs_gdfs_request_freq(GDFS_FREQ_LOW, dev_data);
	if (err != NRFS_SUCCESS) {
		return -EIO;
	}

	rc = k_sem_take(&dev_data->evt_sem, GLOBAL_HSFLL_FREQ_REQ_TIMEOUT);
	if (rc) {
		return -EIO;
	}

	if (dev_data->evt != NRFS_GDFS_EVT_FREQ_CONFIRMED) {
		return -EIO;
	}

	nrfs_gdfs_uninit();
#endif /* GLOBAL_HSFLL_INIT_LOW_REQ */

	rc = clock_config_init(&dev_data->clk_cfg,
			       ARRAY_SIZE(dev_data->clk_cfg.onoff),
			       global_hsfll_work_handler);
	if (rc < 0) {
		return rc;
	}

	err = nrfs_gdfs_init(global_hfsll_nrfs_gdfs_evt_handler);
	if (err != NRFS_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static struct global_hsfll_dev_data driver_data = {
	.dev = DEVICE_DT_INST_GET(0),
};

static const struct global_hsfll_dev_config driver_config = {
	GLOBAL_HSFLL_CLOCK_FREQUENCIES
};

DEVICE_DT_INST_DEFINE(
	0,
	global_hfsll_init,
	NULL,
	&driver_data,
	&driver_config,
	POST_KERNEL,
	CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
	&driver_api
);
