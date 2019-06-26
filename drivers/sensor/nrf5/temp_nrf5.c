/*
 * Copyright (c) 2016 ARM Ltd.
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/sensor.h>
#include <drivers/clock_control.h>
#include <logging/log.h>
#include <hal/nrf_temp.h>

LOG_MODULE_REGISTER(temp_nrf5, CONFIG_SENSOR_LOG_LEVEL);


/* The nRF5 temperature device returns measurements in 0.25C
 * increments.  Scale to mDegrees C.
 */
#define TEMP_NRF5_TEMP_SCALE (1000000 / 4)

struct temp_nrf5_data {
	struct k_sem device_sync_sem;
	s32_t sample;
	struct device *hfclk_dev;
};

static void hfclk_on_callback(struct device *dev, void *user_data)
{
	nrf_temp_task_trigger(NRF_TEMP, NRF_TEMP_TASK_START);
}

static int temp_nrf5_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct temp_nrf5_data *data = dev->driver_data;
	struct clock_control_async_data clk_data = {
		.cb = hfclk_on_callback
	};

	int r;


	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	r = clock_control_async_on(data->hfclk_dev, NULL, &clk_data);
	__ASSERT_NO_MSG(!r);

	k_sem_take(&data->device_sync_sem, K_FOREVER);

	r = clock_control_off(data->hfclk_dev, 0);
	__ASSERT_NO_MSG(!r);

	data->sample = nrf_temp_result_get(NRF_TEMP);
	LOG_DBG("sample: %d", data->sample);
	nrf_temp_task_trigger(NRF_TEMP, NRF_TEMP_TASK_STOP);

	return 0;
}

static int temp_nrf5_channel_get(struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct temp_nrf5_data *data = dev->driver_data;
	s32_t uval;


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
	struct device *dev = (struct device *)arg;
	struct temp_nrf5_data *data = dev->driver_data;

	nrf_temp_event_clear(NRF_TEMP, NRF_TEMP_EVENT_DATARDY);
	k_sem_give(&data->device_sync_sem);
}

static const struct sensor_driver_api temp_nrf5_driver_api = {
	.sample_fetch = temp_nrf5_sample_fetch,
	.channel_get = temp_nrf5_channel_get,
};

DEVICE_DECLARE(temp_nrf5);

static int temp_nrf5_init(struct device *dev)
{
	struct temp_nrf5_data *data = dev->driver_data;

	LOG_DBG("");

	data->hfclk_dev =
		device_get_binding(DT_INST_0_NORDIC_NRF_CLOCK_LABEL "_16M");
	__ASSERT_NO_MSG(data->hfclk_dev);

	k_sem_init(&data->device_sync_sem, 0, UINT_MAX);
	IRQ_CONNECT(
		DT_INST_0_NORDIC_NRF_TEMP_IRQ_0,
		DT_INST_0_NORDIC_NRF_TEMP_IRQ_0_PRIORITY,
		temp_nrf5_isr,
		DEVICE_GET(temp_nrf5),
		0);
	irq_enable(DT_INST_0_NORDIC_NRF_TEMP_IRQ_0);

	nrf_temp_int_enable(NRF_TEMP, NRF_TEMP_INT_DATARDY_MASK);

	return 0;
}

static struct temp_nrf5_data temp_nrf5_driver;

DEVICE_AND_API_INIT(temp_nrf5,
		    CONFIG_TEMP_NRF5_NAME,
		    temp_nrf5_init,
		    &temp_nrf5_driver,
		    NULL,
		    POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY,
		    &temp_nrf5_driver_api);
