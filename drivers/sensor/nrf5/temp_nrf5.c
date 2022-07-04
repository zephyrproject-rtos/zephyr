/*
 * Copyright (c) 2016 ARM Ltd.
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_temp

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_temp.h>

LOG_MODULE_REGISTER(temp_nrf5, CONFIG_SENSOR_LOG_LEVEL);


/* The nRF5 temperature device returns measurements in 0.25C
 * increments.  Scale to mDegrees C.
 */
#define TEMP_NRF5_TEMP_SCALE (1000000 / 4)

struct temp_nrf5_data {
	struct k_sem device_sync_sem;
	struct k_mutex mutex;
	int32_t sample;
	struct onoff_manager *clk_mgr;
};

static void hfclk_on_callback(struct onoff_manager *mgr,
			      struct onoff_client *cli,
			      uint32_t state,
			      int res)
{
	nrf_temp_task_trigger(NRF_TEMP, NRF_TEMP_TASK_START);
}

static int temp_nrf5_sample_fetch(const struct device *dev,
				  enum sensor_channel chan)
{
	struct temp_nrf5_data *data = dev->data;
	struct onoff_client cli;
	int r;

	/* Error if before sensor initialized */
	if (data->clk_mgr == NULL) {
		return -EAGAIN;
	}

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	sys_notify_init_callback(&cli.notify, hfclk_on_callback);
	r = onoff_request(data->clk_mgr, &cli);
	__ASSERT_NO_MSG(r >= 0);

	k_sem_take(&data->device_sync_sem, K_FOREVER);

	r = onoff_release(data->clk_mgr);
	__ASSERT_NO_MSG(r >= 0);

	data->sample = nrf_temp_result_get(NRF_TEMP);
	LOG_DBG("sample: %d", data->sample);
	nrf_temp_task_trigger(NRF_TEMP, NRF_TEMP_TASK_STOP);

	k_mutex_unlock(&data->mutex);

	return 0;
}

static int temp_nrf5_channel_get(const struct device *dev,
				 enum sensor_channel chan,
				 struct sensor_value *val)
{
	struct temp_nrf5_data *data = dev->data;
	int32_t uval;


	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	uval = data->sample * TEMP_NRF5_TEMP_SCALE;
	val->val1 = uval / 1000000;
	val->val2 = uval % 1000000;

	LOG_DBG("Temperature:%d,%d", val->val1, val->val2);

	return 0;
}

static void temp_nrf5_isr(void *arg)
{
	const struct device *dev = (const struct device *)arg;
	struct temp_nrf5_data *data = dev->data;

	nrf_temp_event_clear(NRF_TEMP, NRF_TEMP_EVENT_DATARDY);
	k_sem_give(&data->device_sync_sem);
}

static const struct sensor_driver_api temp_nrf5_driver_api = {
	.sample_fetch = temp_nrf5_sample_fetch,
	.channel_get = temp_nrf5_channel_get,
};

static int temp_nrf5_init(const struct device *dev)
{
	struct temp_nrf5_data *data = dev->data;

	LOG_DBG("");

	/* A null clk_mgr indicates sensor has not been initialized */
	data->clk_mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	__ASSERT_NO_MSG(data->clk_mgr);

	k_sem_init(&data->device_sync_sem, 0, K_SEM_MAX_LIMIT);
	k_mutex_init(&data->mutex);

	IRQ_CONNECT(
		DT_INST_IRQN(0),
		DT_INST_IRQ(0, priority),
		temp_nrf5_isr,
		DEVICE_DT_INST_GET(0),
		0);
	irq_enable(DT_INST_IRQN(0));

	nrf_temp_int_enable(NRF_TEMP, NRF_TEMP_INT_DATARDY_MASK);

	return 0;
}

#define NRF_TEMP_DEFINE(inst)								\
	static struct temp_nrf5_data temp_nrf5_data_##inst;				\
											\
	DEVICE_DT_INST_DEFINE(inst, temp_nrf5_init, NULL,				\
			      &temp_nrf5_data_##inst, NULL, POST_KERNEL,		\
			      CONFIG_SENSOR_INIT_PRIORITY, &temp_nrf5_driver_api);	\

DT_INST_FOREACH_STATUS_OKAY(NRF_TEMP_DEFINE)
