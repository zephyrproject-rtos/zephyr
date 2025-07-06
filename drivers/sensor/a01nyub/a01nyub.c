/*
 * Copyright (c) 2023 SteadConnect
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://wiki.dfrobot.com/A01NYUB%20Waterproof%20Ultrasonic%20Sensor%20SKU:%20SEN0313
 *
 */

#define DT_DRV_COMPAT dfrobot_a01nyub

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(a01nyub_sensor, CONFIG_SENSOR_LOG_LEVEL);

#define A01NYUB_BUF_LEN 4
#define A01NYUB_CHECKSUM_IDX 3
#define A01NYUB_HEADER 0xff

const struct uart_config uart_cfg_a01nyub = {
	.baudrate = 9600,
	.parity = UART_CFG_PARITY_NONE,
	.stop_bits = UART_CFG_STOP_BITS_1,
	.data_bits = UART_CFG_DATA_BITS_8,
	.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
};

struct a01nyub_data {
	/* Max data length is 16 bits */
	uint16_t data;
	uint8_t xfer_bytes;
	uint8_t rd_data[A01NYUB_BUF_LEN];
};

struct a01nyub_cfg {
	const struct device *uart_dev;
	uart_irq_callback_user_data_t cb;
};

static void a01nyub_uart_flush(const struct device *uart_dev)
{
	uint8_t c;

	while (uart_fifo_read(uart_dev, &c, 1) > 0) {
		continue;
	}
}

static uint8_t a01nyub_checksum(const uint8_t *data)
{
	uint16_t cs = 0;

	for (uint8_t i = 0; i < A01NYUB_BUF_LEN - 1; i++) {
		cs += data[i];
	}

	return (uint8_t) (cs & 0x00FF);
}

static inline int a01nyub_poll_data(const struct device *dev)
{
	struct a01nyub_data *data = dev->data;
	uint8_t checksum;

	checksum = a01nyub_checksum(data->rd_data);
	if (checksum != data->rd_data[A01NYUB_CHECKSUM_IDX]) {
		LOG_DBG("Checksum mismatch: calculated 0x%x != data checksum 0x%x",
			checksum,
			data->rd_data[A01NYUB_CHECKSUM_IDX]);
		LOG_DBG("Data bytes: (%x,%x,%x,%x)",
						data->rd_data[0],
						data->rd_data[1],
						data->rd_data[2],
						data->rd_data[3]);

		return -EBADMSG;
	}

	data->data = (data->rd_data[1]<<8) + data->rd_data[2];

	return 0;
}

static int a01nyub_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct a01nyub_data *data = dev->data;

	if (chan != SENSOR_CHAN_DISTANCE) {
		return -ENOTSUP;
	}
	/* val1 is meters, val2 is microns. Both are int32_t
	 * data->data is in mm and units of uint16_t
	 */
	val->val1 = (uint32_t) (data->data / (uint16_t) 1000);
	val->val2 = (uint32_t) ((data->data % 1000) * 1000);
	return 0;
}

static int a01nyub_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	if (chan == SENSOR_CHAN_DISTANCE || chan == SENSOR_CHAN_ALL) {
		return a01nyub_poll_data(dev);
	}

	return -ENOTSUP;
}

static DEVICE_API(sensor, a01nyub_api_funcs) = {
	.sample_fetch = a01nyub_sample_fetch,
	.channel_get = a01nyub_channel_get,
};

static void a01nyub_uart_isr(const struct device *uart_dev, void *user_data)
{
	const struct device *dev = user_data;
	struct a01nyub_data *data = dev->data;

	if (uart_dev == NULL) {
		LOG_DBG("UART device is NULL");
		return;
	}

	if (!uart_irq_update(uart_dev)) {
		LOG_DBG("Unable to start processing interrupts");
		return;
	}

	if (uart_irq_rx_ready(uart_dev)) {
		data->xfer_bytes += uart_fifo_read(uart_dev, &data->rd_data[data->xfer_bytes],
						   A01NYUB_BUF_LEN - data->xfer_bytes);

		/* The first byte should be A01NYUB_HEADER for a valid read.
		 * If we do not read A01NYUB_HEADER on what we think is the
		 * first byte, then reset the number of bytes read until we do
		 */
		if ((data->rd_data[0] != A01NYUB_HEADER) & (data->xfer_bytes == 1)) {
			LOG_DBG("First byte not header! Resetting # of bytes read.");
			data->xfer_bytes = 0;
		}

		if (data->xfer_bytes == A01NYUB_BUF_LEN) {
			LOG_DBG("Read (0x%x,0x%x,0x%x,0x%x)",
				data->rd_data[0],
				data->rd_data[1],
				data->rd_data[2],
				data->rd_data[3]);
			a01nyub_uart_flush(uart_dev);
			data->xfer_bytes = 0;
		}
	}
}

static int a01nyub_init(const struct device *dev)
{
	const struct a01nyub_cfg *cfg = dev->config;
	int ret = 0;

	uart_irq_rx_disable(cfg->uart_dev);
	uart_irq_tx_disable(cfg->uart_dev);

	a01nyub_uart_flush(cfg->uart_dev);

	LOG_DBG("Initializing A01NYUB driver");

	ret = uart_configure(cfg->uart_dev, &uart_cfg_a01nyub);
	if (ret == -ENOSYS) {
		LOG_ERR("Unable to configure UART port");
		return -ENOSYS;
	}

	ret = uart_irq_callback_user_data_set(cfg->uart_dev, cfg->cb, (void *)dev);

	if (ret < 0) {
		if (ret == -ENOTSUP) {
			LOG_ERR("Interrupt-driven UART API support not enabled");
		} else if (ret == -ENOSYS) {
			LOG_ERR("UART device does not support interrupt-driven API");
		} else {
			LOG_ERR("Error setting UART callback: %d", ret);
		}
		return ret;
	}

	uart_irq_rx_enable(cfg->uart_dev);

	return ret;
}

#define A01NYUB_INIT(inst)							\
										\
	static struct a01nyub_data a01nyub_data_##inst;				\
										\
	static const struct a01nyub_cfg a01nyub_cfg_##inst = {			\
		.uart_dev = DEVICE_DT_GET(DT_INST_BUS(inst)),			\
		.cb = a01nyub_uart_isr,						\
	};									\
										\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, a01nyub_init, NULL,			\
		&a01nyub_data_##inst, &a01nyub_cfg_##inst,			\
		POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &a01nyub_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(A01NYUB_INIT)
