/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrfs_audiopll

#include "clock_control_nrf2_common.h"
#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/clock/nrfs-audiopll.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <nrfs_audiopll.h>
#include <nrfs_backend_ipc_service.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(clock_control_nrf2, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define SHIM_DEFAULT_PRESCALER AUDIOPLL_DIV_12

BUILD_ASSERT(
	DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	"multiple instances not supported"
);

BUILD_ASSERT(DT_INST_PROP(0, frequency) >= NRFS_AUDIOPLL_FREQ_MIN);
BUILD_ASSERT(DT_INST_PROP(0, frequency) <= NRFS_AUDIOPLL_FREQ_MAX);

struct shim_data {
	struct onoff_manager mgr;
	onoff_notify_fn mgr_notify;
	const struct device *dev;
	struct k_sem evt_sem;
	nrfs_audiopll_evt_type_t evt;
};

static int shim_nrfs_request_enable(const struct device *dev)
{
	struct shim_data *dev_data = dev->data;
	nrfs_err_t err;

	LOG_DBG("send enable request");

	dev_data->evt = NRFS_AUDIOPLL_EVT_ENABLED;
	err = nrfs_audiopll_enable_request(dev_data);
	if (err != NRFS_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static int shim_nrfs_request_disable(const struct device *dev)
{
	struct shim_data *dev_data = dev->data;
	nrfs_err_t err;

	LOG_DBG("send disable request");

	dev_data->evt = NRFS_AUDIOPLL_EVT_DISABLED;
	err = nrfs_audiopll_disable_request(dev_data);
	if (err != NRFS_SUCCESS) {
		return -EIO;
	}

	return 0;
}

static void onoff_start_option(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	struct shim_data *dev_data = CONTAINER_OF(mgr, struct shim_data, mgr);
	const struct device *dev = dev_data->dev;
	int ret;

	dev_data->mgr_notify = notify;

	ret = shim_nrfs_request_enable(dev);
	if (ret) {
		dev_data->mgr_notify = NULL;
		notify(mgr, -EIO);
	}
}

static void onoff_stop_option(struct onoff_manager *mgr, onoff_notify_fn notify)
{
	struct shim_data *dev_data = CONTAINER_OF(mgr, struct shim_data, mgr);
	const struct device *dev = dev_data->dev;
	int ret;

	dev_data->mgr_notify = notify;

	ret = shim_nrfs_request_disable(dev);
	if (ret) {
		dev_data->mgr_notify = NULL;
		notify(mgr, -EIO);
	}
}

static const struct onoff_transitions shim_mgr_transitions = {
	.start = onoff_start_option,
	.stop  = onoff_stop_option
};

/*
 * Formula:
 *
 *   frequency = ((4 + (freq_fraction * 2^-16)) * 32000000) / 12
 *
 * Simplified linear approximation:
 *
 *   frequency = 10666666 + (((13333292 - 10666666) / 65535) * freq_fraction)
 *   frequency = 10666666 + ((2666626 / 65535) * freq_fraction)
 *   frequency = ((10666666 * 65535) + (2666626 * freq_fraction)) / 65535
 *   frequency = (699039956310 + (2666626 * freq_fraction)) / 65535
 *
 * Isolate freq_fraction:
 *
 *   frequency = (699039956310 + (2666626 * freq_fraction)) / 65535
 *   frequency * 65535 = 699039956310 + (2666626 * freq_fraction)
 *   (frequency * 65535) - 699039956310 = 2666626 * freq_fraction
 *   freq_fraction = ((frequency * 65535) - 699039956310) / 2666626
 */
static uint16_t shim_frequency_to_freq_fraction(uint32_t frequency)
{
	uint64_t freq_fraction;

	freq_fraction = frequency;
	freq_fraction *= 65535;
	freq_fraction -= 699039956310;
	freq_fraction = DIV_ROUND_CLOSEST(freq_fraction, 2666626);

	return (uint16_t)freq_fraction;
}

static int shim_nrfs_request_freq_sync(const struct device *dev, uint16_t freq_fraction)
{
	struct shim_data *dev_data = dev->data;
	nrfs_err_t err;

	LOG_DBG("send freq request");

	err = nrfs_audiopll_request_freq(freq_fraction, dev_data);
	if (err != NRFS_SUCCESS) {
		return -EIO;
	}

	k_sem_take(&dev_data->evt_sem, K_FOREVER);
	return dev_data->evt == NRFS_AUDIOPLL_EVT_FREQ_CONFIRMED ? 0 : -EIO;
}

static int shim_nrfs_request_prescaler_sync(const struct device *dev,
					    enum audiopll_prescaler_div div)
{
	struct shim_data *dev_data = dev->data;
	nrfs_err_t err;

	LOG_DBG("send prescaler request");

	err = nrfs_audiopll_request_prescaler(div, dev_data);
	if (err != NRFS_SUCCESS) {
		return -EIO;
	}

	k_sem_take(&dev_data->evt_sem, K_FOREVER);
	return dev_data->evt == NRFS_AUDIOPLL_EVT_PRESCALER_CONFIRMED ? 0 : -EIO;
}

static void shim_nrfs_audiopll_init_evt_handler(nrfs_audiopll_evt_t const *evt, void *context)
{
	struct shim_data *dev_data = context;

	LOG_DBG("init resp evt %u", (uint32_t)evt->type);

	dev_data->evt = evt->type;
	k_sem_give(&dev_data->evt_sem);
}

static void shim_nrfs_audiopll_evt_handler(nrfs_audiopll_evt_t const *evt, void *context)
{
	struct shim_data *dev_data = context;
	int ret;

	LOG_DBG("resp evt %u", (uint32_t)evt->type);

	if (dev_data->mgr_notify == NULL) {
		return;
	}

	ret = evt->type == dev_data->evt ? 0 : -EIO;
	dev_data->mgr_notify(&dev_data->mgr, ret);
}

static int api_request_audiopll(const struct device *dev,
				const struct nrf_clock_spec *spec,
				struct onoff_client *cli)
{
	struct shim_data *dev_data = dev->data;
	struct onoff_manager *mgr = &dev_data->mgr;

	ARG_UNUSED(spec);

	return onoff_request(mgr, cli);
}

static int api_release_audiopll(const struct device *dev,
				    const struct nrf_clock_spec *spec)
{
	struct shim_data *dev_data = dev->data;
	struct onoff_manager *mgr = &dev_data->mgr;

	ARG_UNUSED(spec);

	return onoff_release(mgr);
}

static int api_cancel_or_release_audiopll(const struct device *dev,
					  const struct nrf_clock_spec *spec,
					  struct onoff_client *cli)
{
	struct shim_data *dev_data = dev->data;
	struct onoff_manager *mgr = &dev_data->mgr;

	ARG_UNUSED(spec);

	return onoff_cancel_or_release(mgr, cli);
}

static DEVICE_API(nrf_clock_control, shim_driver_api) = {
	.std_api = {
		.on = api_nosys_on_off,
		.off = api_nosys_on_off,
	},
	.request = api_request_audiopll,
	.release = api_release_audiopll,
	.cancel_or_release = api_cancel_or_release_audiopll,
};

static int shim_init(const struct device *dev)
{
	struct shim_data *dev_data = dev->data;
	nrfs_err_t err;
	int ret;
	uint16_t freq_fraction;

	LOG_DBG("waiting for nrfs backend connected");
	err = nrfs_backend_wait_for_connection(K_FOREVER);
	if (err != NRFS_SUCCESS) {
		LOG_ERR("nrfs backend not connected");
		return -ENODEV;
	}

	k_sem_init(&dev_data->evt_sem, 0, 1);

	err = nrfs_audiopll_init(shim_nrfs_audiopll_init_evt_handler);
	if (err != NRFS_SUCCESS) {
		LOG_ERR("failed to init audiopll service");
		return -ENODEV;
	}

	ret = shim_nrfs_request_prescaler_sync(dev, SHIM_DEFAULT_PRESCALER);
	if (ret) {
		LOG_ERR("failed to set prescaler divider");
		return ret;
	}

	freq_fraction = shim_frequency_to_freq_fraction(DT_INST_PROP(0, frequency));

	LOG_DBG("requesting freq_fraction %u for frequency %uHz",
		freq_fraction,
		DT_INST_PROP(0, frequency));

	ret = shim_nrfs_request_freq_sync(dev, freq_fraction);
	if (ret) {
		LOG_ERR("failed to set freq_fraction");
		return ret;
	}

	nrfs_audiopll_uninit();

	ret = onoff_manager_init(&dev_data->mgr, &shim_mgr_transitions);
	if (ret < 0) {
		LOG_ERR("failed to init onoff manager");
		return ret;
	}

	err = nrfs_audiopll_init(shim_nrfs_audiopll_evt_handler);
	if (err != NRFS_SUCCESS) {
		LOG_ERR("failed to init audiopll service");
		return -ENODEV;
	}

	return 0;
}

static struct shim_data shim_data = {
	.dev = DEVICE_DT_INST_GET(0),
};

DEVICE_DT_INST_DEFINE(
	0,
	shim_init,
	NULL,
	&shim_data,
	NULL,
	POST_KERNEL,
	UTIL_INC(CONFIG_NRFS_BACKEND_IPC_SERVICE_INIT_PRIO),
	&shim_driver_api,
);
