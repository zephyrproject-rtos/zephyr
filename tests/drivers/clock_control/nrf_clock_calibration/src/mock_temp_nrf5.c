/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

/* Mock of internal temperature sensor. */
#ifdef CONFIG_TEMP_NRF5
#error "Cannot be enabled because it is being mocked"
#endif

static struct sensor_value value;

void mock_temp_nrf5_value_set(struct sensor_value *val)
{
	value = *val;
}

static int mock_temp_nrf5_init(const struct device *dev)
{
	return 0;
}

static int mock_temp_nrf5_sample_fetch(const struct device *dev,
					enum sensor_channel chan)
{
	k_sleep(K_MSEC(1));
	return 0;
}

static int mock_temp_nrf5_channel_get(const struct device *dev,
				      enum sensor_channel chan,
				      struct sensor_value *val)
{
	*val = value;
	return 0;
}

static const struct sensor_driver_api mock_temp_nrf5_driver_api = {
	.sample_fetch = mock_temp_nrf5_sample_fetch,
	.channel_get = mock_temp_nrf5_channel_get,
};

DEVICE_DT_DEFINE(DT_INST(0, nordic_nrf_temp),
		    mock_temp_nrf5_init,
		    NULL,
		    NULL,
		    NULL,
		    POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY,
		    &mock_temp_nrf5_driver_api);
