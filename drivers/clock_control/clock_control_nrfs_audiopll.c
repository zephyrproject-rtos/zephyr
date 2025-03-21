/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrfs_audiopll

#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#include <nrfs_audiopll.h>
#include <nrfs_backend_ipc_service.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nrfs_audiopll, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

BUILD_ASSERT(
	DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	"multiple instances not supported"
);

BUILD_ASSERT((int)AUDIOPLL_DIV_DISABLED == (int)NRF_CLOCK_CONTROL_AUDIOPLL_DIV_DISABLED);
BUILD_ASSERT((int)AUDIOPLL_DIV_1 == (int)NRF_CLOCK_CONTROL_AUDIOPLL_DIV_1);
BUILD_ASSERT((int)AUDIOPLL_DIV_2 == (int)NRF_CLOCK_CONTROL_AUDIOPLL_DIV_2);
BUILD_ASSERT((int)AUDIOPLL_DIV_3 == (int)NRF_CLOCK_CONTROL_AUDIOPLL_DIV_3);
BUILD_ASSERT((int)AUDIOPLL_DIV_4 == (int)NRF_CLOCK_CONTROL_AUDIOPLL_DIV_4);
BUILD_ASSERT((int)AUDIOPLL_DIV_6 == (int)NRF_CLOCK_CONTROL_AUDIOPLL_DIV_6);
BUILD_ASSERT((int)AUDIOPLL_DIV_8 == (int)NRF_CLOCK_CONTROL_AUDIOPLL_DIV_8);
BUILD_ASSERT((int)AUDIOPLL_DIV_12 == (int)NRF_CLOCK_CONTROL_AUDIOPLL_DIV_12);
BUILD_ASSERT((int)AUDIOPLL_DIV_16 == (int)NRF_CLOCK_CONTROL_AUDIOPLL_DIV_16);

struct shim_data {
	const struct device *dev;
	struct k_sem evt_sem;
	nrfs_audiopll_evt_type_t evt;
};

struct shim_config {
	uint16_t freq_fraction;
	enum nrf_clock_control_audiopll_prescaler_div div;
};

static void shim_nrfs_audiopll_evt_handler(nrfs_audiopll_evt_t const *evt, void *context)
{
	struct shim_data *dev_data = context;

	dev_data->evt = evt->type;
	k_sem_give(&dev_data->evt_sem);
}

static int shim_nrfs_request_enable(const struct device *dev)
{
	struct shim_data *dev_data = dev->data;
	nrfs_err_t err;

	LOG_DBG("send enable request");

	err = nrfs_audiopll_enable_request(dev_data);
	if (err != NRFS_SUCCESS) {
		return -EAGAIN;
	}

	k_sem_take(&dev_data->evt_sem, K_FOREVER);
	return dev_data->evt == NRFS_AUDIOPLL_EVT_ENABLED ? 0 : -EAGAIN;
}

static int shim_nrfs_request_disable(const struct device *dev)
{
	struct shim_data *dev_data = dev->data;
	nrfs_err_t err;

	LOG_DBG("send disable request");

	err = nrfs_audiopll_disable_request(dev_data);
	if (err != NRFS_SUCCESS) {
		return -EAGAIN;
	}

	k_sem_take(&dev_data->evt_sem, K_FOREVER);
	return dev_data->evt == NRFS_AUDIOPLL_EVT_DISABLED ? 0 : -EAGAIN;
}

static int shim_clock_control_api_on(const struct device *dev,
				     clock_control_subsys_t sub_system)
{
	ARG_UNUSED(sub_system);

	return shim_nrfs_request_enable(dev);
}

static int shim_clock_control_api_off(const struct device *dev,
				      clock_control_subsys_t sub_system)
{
	ARG_UNUSED(sub_system);

	return shim_nrfs_request_disable(dev);
}

static DEVICE_API(clock_control, shim_clock_control_api) = {
	.on = shim_clock_control_api_on,
	.off = shim_clock_control_api_off,
};

int nrf_clock_control_audiopll_set_freq(const struct device *dev, uint16_t freq_fraction)
{
	struct shim_data *dev_data = dev->data;
	nrfs_err_t err;

	LOG_DBG("send freq request");

	err = nrfs_audiopll_request_freq(freq_fraction, dev_data);
	if (err != NRFS_SUCCESS) {
		return -EAGAIN;
	}

	k_sem_take(&dev_data->evt_sem, K_FOREVER);
	return dev_data->evt == NRFS_AUDIOPLL_EVT_FREQ_CONFIRMED ? 0 : -EAGAIN;
}

int nrf_clock_control_audiopll_set_prescaler(const struct device *dev,
					     enum nrf_clock_control_audiopll_prescaler_div div)
{
	struct shim_data *dev_data = dev->data;
	nrfs_err_t err;

	LOG_DBG("send prescaler request");

	err = nrfs_audiopll_request_prescaler((enum audiopll_prescaler_div)div, dev_data);
	if (err != NRFS_SUCCESS) {
		return -EAGAIN;
	}

	k_sem_take(&dev_data->evt_sem, K_FOREVER);
	return dev_data->evt == NRFS_AUDIOPLL_EVT_PRESCALER_CONFIRMED ? 0 : -EAGAIN;
}

int nrf_clock_control_audiopll_set_freq_inc(const struct device *dev,
					    int8_t inc_val,
					    uint16_t inc_period_ms)
{
	struct shim_data *dev_data = dev->data;
	nrfs_err_t err;

	LOG_DBG("send freq inc request");

	err = nrfs_audiopll_request_freq_inc(inc_val, inc_period_ms, dev_data);
	if (err != NRFS_SUCCESS) {
		return -EAGAIN;
	}

	k_sem_take(&dev_data->evt_sem, K_FOREVER);
	return dev_data->evt == NRFS_AUDIOPLL_EVT_FREQ_INC_CONFIRMED ? 0 : -EAGAIN;
}

static int shim_init(const struct device *dev)
{
	struct shim_data *dev_data = dev->data;
	const struct shim_config *dev_config = dev->data;
	nrfs_err_t err;
	int ret;

	LOG_DBG("waiting for nrfs backend connected");
	err = nrfs_backend_wait_for_connection(K_FOREVER);
	if (err != NRFS_SUCCESS) {
		LOG_ERR("nrfs backend not connected");
		return -ENODEV;
	}

	err = nrfs_audiopll_init(shim_nrfs_audiopll_evt_handler);
	if (err != NRFS_SUCCESS) {
		LOG_ERR("failed to init audiopll service");
		return -ENODEV;
	}

	k_sem_init(&dev_data->evt_sem, 0, 1);

	ret = nrf_clock_control_audiopll_set_freq(dev, dev_config->freq_fraction);
	if (ret) {
		LOG_ERR("failed to set initial frequency fraction");
		return ret;
	}

	ret = nrf_clock_control_audiopll_set_prescaler(dev, dev_config->div);
	if (ret) {
		LOG_ERR("failed to set initial prescaler divider");
		return ret;
	}

	return 0;
}

static struct shim_data shim_data = {
	.dev = DEVICE_DT_INST_GET(0),
};

static struct shim_config shim_config = {
	.freq_fraction = DT_INST_PROP(0, freq_fraction),
	.div = DT_INST_ENUM_IDX(0, prescaler_div),
};

DEVICE_DT_INST_DEFINE(
	0,
	shim_init,
	NULL,
	&shim_data,
	&shim_config,
	POST_KERNEL,
	UTIL_INC(CONFIG_NRFS_BACKEND_IPC_SERVICE_INIT_PRIO),
	&shim_clock_control_api,
);
