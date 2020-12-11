/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT plantower_pms7003

/* sensor pms7003.c - Driver for plantower PMS7003 sensor
 * PMS7003 product: http://www.plantower.com/en/content/?110.html
 * PMS7003 spec: http://aqicn.org/air/view/sensor/spec/pms7003.pdf
 */

#include <errno.h>

#include <arch/cpu.h>
#include <init.h>
#include <kernel.h>
#include <drivers/sensor.h>
#include <stdlib.h>
#include <string.h>
#include <drivers/uart.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(PMS7003, CONFIG_SENSOR_LOG_LEVEL);

/* wait serial output with 1000ms timeout */
#define CFG_PMS7003_SERIAL_TIMEOUT 1000

struct pms7003_data {
	const struct device *uart_dev;
	uint16_t pm_1_0;
	uint16_t pm_2_5;
	uint16_t pm_10;
};

/**
 * @brief wait for an array data from uart device with a timeout
 *
 * @param dev the uart device
 * @param data the data array to be matched
 * @param len the data array len
 * @param timeout the timeout in milliseconds
 * @return 0 if success; -ETIME if timeout
 */
static int uart_wait_for(const struct device *dev, uint8_t *data, int len,
			 int timeout)
{
	int matched_size = 0;
	int64_t timeout_time = k_uptime_get() + K_MSEC(timeout);

	while (1) {
		uint8_t c;

		if (k_uptime_get() > timeout_time) {
			return -ETIME;
		}

		if (uart_poll_in(dev, &c) == 0) {
			if (c == data[matched_size]) {
				matched_size++;

				if (matched_size == len) {
					break;
				}
			} else if (c == data[0]) {
				matched_size = 1;
			} else {
				matched_size = 0;
			}
		}
	}
	return 0;
}

/**
 * @brief read bytes from uart
 *
 * @param data the data buffer
 * @param len the data len
 * @param timeout the timeout in milliseconds
 * @return 0 if success; -ETIME if timeout
 */
static int uart_read_bytes(const struct device *dev, uint8_t *data, int len,
			   int timeout)
{
	int read_size = 0;
	int64_t timeout_time = k_uptime_get() + K_MSEC(timeout);

	while (1) {
		uint8_t c;

		if (k_uptime_get() > timeout_time) {
			return -ETIME;
		}

		if (uart_poll_in(dev, &c) == 0) {
			data[read_size++] = c;
			if (read_size == len) {
				break;
			}
		}
	}
	return 0;
}

static int pms7003_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct pms7003_data *drv_data = dev->data;

	/* sample output */
	/* 42 4D 00 1C 00 01 00 01 00 01 00 01 00 01 00 01 01 92
	 * 00 4E 00 03 00 00 00 00 00 00 71 00 02 06
	 */

	uint8_t pms7003_start_bytes[] = {0x42, 0x4d};
	uint8_t pms7003_receive_buffer[30];

	if (uart_wait_for(drv_data->uart_dev, pms7003_start_bytes,
			  sizeof(pms7003_start_bytes),
			  CFG_PMS7003_SERIAL_TIMEOUT) < 0) {
		LOG_WRN("waiting for start bytes is timeout");
		return -ETIME;
	}

	if (uart_read_bytes(drv_data->uart_dev, pms7003_receive_buffer, 30,
			    CFG_PMS7003_SERIAL_TIMEOUT) < 0) {
		return -ETIME;
	}

	drv_data->pm_1_0 =
	    (pms7003_receive_buffer[8] << 8) + pms7003_receive_buffer[9];
	drv_data->pm_2_5 =
	    (pms7003_receive_buffer[10] << 8) + pms7003_receive_buffer[11];
	drv_data->pm_10 =
	    (pms7003_receive_buffer[12] << 8) + pms7003_receive_buffer[13];

	LOG_DBG("pm1.0 = %d", drv_data->pm_1_0);
	LOG_DBG("pm2.5 = %d", drv_data->pm_2_5);
	LOG_DBG("pm10 = %d", drv_data->pm_10);
	return 0;
}

static int pms7003_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct pms7003_data *drv_data = dev->data;

	if (chan == SENSOR_CHAN_PM_1_0) {
		val->val1 = drv_data->pm_1_0;
		val->val2 = 0;
	} else if (chan == SENSOR_CHAN_PM_2_5) {
		val->val1 = drv_data->pm_2_5;
		val->val2 = 0;
	} else if (chan == SENSOR_CHAN_PM_10) {
		val->val1 = drv_data->pm_10;
		val->val2 = 0;
	} else {
		return -EINVAL;
	}
	return 0;
}

static const struct sensor_driver_api pms7003_api = {
	.sample_fetch = &pms7003_sample_fetch,
	.channel_get = &pms7003_channel_get,
};

static int pms7003_init(const struct device *dev)
{
	struct pms7003_data *drv_data = dev->data;

	drv_data->uart_dev = device_get_binding(DT_INST_BUS_LABEL(0));

	if (!drv_data->uart_dev) {
		LOG_DBG("uart device is not found: %s",
			    DT_INST_BUS_LABEL(0));
		return -EINVAL;
	}

	return 0;
}

static struct pms7003_data pms7003_data;

DEVICE_AND_API_INIT(gts_dev, DT_INST_LABEL(0), &pms7003_init,
		    &pms7003_data, NULL, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &pms7003_api);
