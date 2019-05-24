/*
 * Copyright (c) 2016 ARM Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <sensor.h>
#include <clock_control.h>
#include <logging/log.h>

#define LOG_LEVEL CONFIG_SENSOR_LOG_LEVEL
LOG_MODULE_REGISTER(TEMPNRF5);

#include "nrf.h"


/* The nRF5 temperature device returns measurements in 0.25C
 * increments.  Scale to mDegrees C.
 */
#define TEMP_NRF5_TEMP_SCALE (1000000 / 4)

struct temp_nrf5_data {
	struct k_sem device_sync_sem;
	s32_t sample;
	struct device *clk_m16_dev;
};


static int temp_nrf5_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct temp_nrf5_data *data = dev->driver_data;
	int r;

	LOG_DBG("");

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	/* The clock driver for nrf51 currently overloads the
	 * subsystem parameter with a flag to indicate whether or not
	 * it should block.
	 */
	r = clock_control_on(data->clk_m16_dev, (void *)1);
	__ASSERT_NO_MSG(!r);

	NRF_TEMP->TASKS_START = 1;
	k_sem_take(&data->device_sync_sem, K_FOREVER);

	r = clock_control_off(data->clk_m16_dev, (void *)1);
	__ASSERT_NO_MSG(!r || r == -EBUSY);

	data->sample = NRF_TEMP->TEMP;

	NRF_TEMP->TASKS_STOP = 1;

	return 0;
}

static int temp_nrf5_channel_get(struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct temp_nrf5_data *data = dev->driver_data;
	s32_t uval;

	LOG_DBG("");

	if (chan != SENSOR_CHAN_DIE_TEMP) {
		return -ENOTSUP;
	}

	uval = data->sample * TEMP_NRF5_TEMP_SCALE;
	val->val1 = uval / 1000000;
	val->val2 = uval % 1000000;

	return 0;
}

static void temp_nrf5_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct temp_nrf5_data *data = dev->driver_data;

	NRF_TEMP->EVENTS_DATARDY = 0;
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

	data->clk_m16_dev =
		device_get_binding(DT_NORDIC_NRF_CLOCK_0_LABEL "_16M");
	__ASSERT_NO_MSG(data->clk_m16_dev);

	k_sem_init(&data->device_sync_sem, 0, UINT_MAX);
	IRQ_CONNECT(
		DT_NORDIC_NRF_TEMP_0_IRQ_0,
		DT_NORDIC_NRF_TEMP_0_IRQ_0_PRIORITY,
		temp_nrf5_isr,
		DEVICE_GET(temp_nrf5),
		0);
	irq_enable(DT_NORDIC_NRF_TEMP_0_IRQ_0);

	NRF_TEMP->INTENSET = TEMP_INTENSET_DATARDY_Set;

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
