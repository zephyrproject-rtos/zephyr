/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

#define DT_DRV_COMPAT iclegend_s3km1110

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/s3km1110.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

LOG_MODULE_REGISTER(s3km1110, CONFIG_SENSOR_LOG_LEVEL);

#define S3KM1110_FRAME_DELIMITER_SIZE    4U
#define S3KM1110_LEN_SIZE                2U
#define S3KM1110_REPORT_HEAD             0xAA
#define S3KM1110_REPORT_TAIL             0x55
#define S3KM1110_REPORT_CALIBRATION      0x00
#define S3KM1110_BASE_REPORT_PAYLOAD_LEN 13U
#define S3KM1110_RADAR_MODE_BASIC        0x02

static const uint8_t s3km1110_frame_start[S3KM1110_FRAME_DELIMITER_SIZE] = {0xF4, 0xF3, 0xF2, 0xF1};
static const uint8_t s3km1110_frame_end[S3KM1110_FRAME_DELIMITER_SIZE] = {0xF8, 0xF7, 0xF6, 0xF5};

enum s3km1110_rx_state {
	S3KM1110_RX_WAIT_START,
	S3KM1110_RX_READ_LEN,
	S3KM1110_RX_READ_PAYLOAD,
	S3KM1110_RX_READ_END,
};

struct s3km1110_config {
	const struct device *uart_dev;
	uart_irq_callback_user_data_t cb;
};

struct s3km1110_data {
	/* Sensor data */
	uint8_t target_status;
	uint16_t moving_distance_cm;
	uint8_t moving_energy;
	uint16_t static_distance_cm;
	uint8_t static_energy;
	int16_t detection_distance_cm;

	/* UART RX state */
	struct k_sem lock;
	struct k_sem rx_sem;
	uint8_t rx_buf[S3KM1110_BASE_REPORT_PAYLOAD_LEN];
	uint16_t rx_pos;
	uint16_t frame_len;
	uint8_t start_matched;
	enum s3km1110_rx_state rx_state;
};

static void s3km1110_uart_flush(const struct device *uart_dev)
{
	uint8_t c;

	while (uart_fifo_read(uart_dev, &c, 1) > 0) {
		continue;
	}
}

static void s3km1110_rx_reset(struct s3km1110_data *data)
{
	data->rx_state = S3KM1110_RX_WAIT_START;
	data->rx_pos = 0;
	data->frame_len = 0;
	data->start_matched = 0;
}

static void s3km1110_uart_isr(const struct device *uart_dev, void *user_data)
{
	struct s3km1110_data *data = user_data;
	uint8_t byte;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (!uart_irq_rx_ready(uart_dev)) {
		return;
	}

	while (uart_fifo_read(uart_dev, &byte, 1) == 1) {
		switch (data->rx_state) {
		case S3KM1110_RX_WAIT_START:
			if (byte == s3km1110_frame_start[data->start_matched]) {
				data->start_matched++;
				if (data->start_matched == S3KM1110_FRAME_DELIMITER_SIZE) {
					data->rx_state = S3KM1110_RX_READ_LEN;
					data->rx_pos = 0;
				}
			} else {
				data->start_matched = (byte == s3km1110_frame_start[0]) ? 1U : 0U;
			}
			break;

		case S3KM1110_RX_READ_LEN:
			data->rx_buf[data->rx_pos++] = byte;
			if (data->rx_pos == S3KM1110_LEN_SIZE) {
				data->frame_len = sys_get_le16(data->rx_buf);
				if (data->frame_len == 0 ||
				    data->frame_len > S3KM1110_BASE_REPORT_PAYLOAD_LEN) {
					s3km1110_rx_reset(data);
					break;
				}
				data->rx_pos = 0;
				data->rx_state = S3KM1110_RX_READ_PAYLOAD;
			}
			break;

		case S3KM1110_RX_READ_PAYLOAD:
			data->rx_buf[data->rx_pos++] = byte;
			if (data->rx_pos == data->frame_len) {
				data->rx_pos = 0;
				data->rx_state = S3KM1110_RX_READ_END;
			}
			break;

		case S3KM1110_RX_READ_END:
			if (byte != s3km1110_frame_end[data->rx_pos]) {
				s3km1110_rx_reset(data);
				break;
			}
			data->rx_pos++;
			if (data->rx_pos == S3KM1110_FRAME_DELIMITER_SIZE) {
				uart_irq_rx_disable(uart_dev);
				k_sem_give(&data->rx_sem);
				return;
			}
			break;
		default:
			/* Should never happen */
			break;
		}
	}
}

static inline bool s3km1110_is_report_valid(const uint8_t *payload, size_t payload_len)
{
	if (payload[1] != S3KM1110_REPORT_HEAD ||
	    payload[payload_len - 2] != S3KM1110_REPORT_TAIL ||
	    payload[payload_len - 1] != S3KM1110_REPORT_CALIBRATION) {
		return false;
	}
	return true;
}

static int s3km1110_parse_report(const uint8_t *payload, size_t payload_len,
				 struct s3km1110_data *data)
{
	if (payload_len == 0 || payload_len < S3KM1110_BASE_REPORT_PAYLOAD_LEN) {
		return -EMSGSIZE;
	}

	if (payload[0] != S3KM1110_RADAR_MODE_BASIC) {
		return -ENOTSUP;
	}

	if (!s3km1110_is_report_valid(payload, payload_len)) {
		return -EBADMSG;
	}

	data->target_status =
		(payload[2] <= S3KM1110_TARGET_BOTH) ? payload[2] : S3KM1110_TARGET_ERROR;
	data->moving_distance_cm = sys_get_le16(&payload[3]);
	data->moving_energy = payload[5];
	data->static_distance_cm = sys_get_le16(&payload[6]);
	data->static_energy = payload[8];

	if (data->target_status == S3KM1110_TARGET_ERROR) {
		data->detection_distance_cm = -1;
	} else {
		data->detection_distance_cm = (int16_t)sys_get_le16(&payload[9]);
	}

	return 0;
}

static int s3km1110_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct s3km1110_config *config = dev->config;
	struct s3km1110_data *data = dev->data;
	int rc;

	if (chan != SENSOR_CHAN_ALL) {
		LOG_ERR("Unsupported sensor channel");
		return -ENOTSUP;
	}

	k_sem_take(&data->lock, K_FOREVER);

	s3km1110_rx_reset(data);
	k_sem_reset(&data->rx_sem);
	uart_irq_rx_enable(config->uart_dev);

	rc = k_sem_take(&data->rx_sem, K_MSEC(CONFIG_S3KM1110_UART_TIMEOUT_MS));

	if (rc != 0) {
		uart_irq_rx_disable(config->uart_dev);

		if (rc == -EAGAIN) {
			LOG_DBG("Did not receive a complete frame after %d ms",
				CONFIG_S3KM1110_UART_TIMEOUT_MS);
			rc = -ETIMEDOUT;
		} else {
			LOG_DBG("k_sem_take failed with error: %d", rc);
		}

		k_sem_give(&data->lock);
		return rc;
	}

	rc = s3km1110_parse_report(data->rx_buf, data->frame_len, data);
	if (rc != 0) {
		LOG_DBG("Failed to parse report: %d", rc);
	}

	k_sem_give(&data->lock);

	return rc;
}

static int s3km1110_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	const struct s3km1110_data *data = dev->data;

	switch ((uint32_t)chan) {
	case SENSOR_CHAN_PROX:
		val->val1 = (data->target_status != S3KM1110_TARGET_NO_TARGET &&
			     data->target_status != S3KM1110_TARGET_ERROR)
				    ? 1
				    : 0;
		val->val2 = 0;
		return 0;
	case SENSOR_CHAN_DISTANCE:
		if (data->detection_distance_cm < 0) {
			return -ENODATA;
		}

		return sensor_value_from_milli(val, (int64_t)data->detection_distance_cm * 10);
	case SENSOR_CHAN_S3KM1110_TARGET_STATUS:
		val->val1 = data->target_status;
		val->val2 = 0;
		return 0;
	case SENSOR_CHAN_S3KM1110_MOVING_DISTANCE:
		return sensor_value_from_milli(val, (int64_t)data->moving_distance_cm * 10);
	case SENSOR_CHAN_S3KM1110_STATIC_DISTANCE:
		return sensor_value_from_milli(val, (int64_t)data->static_distance_cm * 10);
	case SENSOR_CHAN_S3KM1110_MOVING_ENERGY:
		val->val1 = data->moving_energy;
		val->val2 = 0;
		return 0;
	case SENSOR_CHAN_S3KM1110_STATIC_ENERGY:
		val->val1 = data->static_energy;
		val->val2 = 0;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(sensor, s3km1110_driver_api) = {
	.sample_fetch = s3km1110_sample_fetch,
	.channel_get = s3km1110_channel_get,
};

static int s3km1110_init(const struct device *dev)
{
	const struct s3km1110_config *config = dev->config;
	struct s3km1110_data *data = dev->data;
	int rc;

	if (!device_is_ready(config->uart_dev)) {
		LOG_ERR("%s is not ready", config->uart_dev->name);
		return -ENODEV;
	}

	data->target_status = S3KM1110_TARGET_NO_TARGET;
	data->detection_distance_cm = -1;

	k_sem_init(&data->lock, 1, 1);
	k_sem_init(&data->rx_sem, 0, 1);
	s3km1110_rx_reset(data);

	uart_irq_rx_disable(config->uart_dev);
	uart_irq_tx_disable(config->uart_dev);

	s3km1110_uart_flush(config->uart_dev);

	rc = uart_irq_callback_user_data_set(config->uart_dev, config->cb, data);
	if (rc != 0) {
		LOG_ERR("UART IRQ setup failed: %d", rc);
		return rc;
	}

	return 0;
}

#define S3KM1110_DEFINE(inst)                                                                      \
	static struct s3km1110_data s3km1110_data_##inst;                                          \
                                                                                                   \
	static const struct s3km1110_config s3km1110_config_##inst = {                             \
		.uart_dev = DEVICE_DT_GET(DT_INST_BUS(inst)),                                      \
		.cb = s3km1110_uart_isr,                                                           \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, s3km1110_init, NULL, &s3km1110_data_##inst,             \
				     &s3km1110_config_##inst, POST_KERNEL,                         \
				     CONFIG_SENSOR_INIT_PRIORITY, &s3km1110_driver_api);

DT_INST_FOREACH_STATUS_OKAY(S3KM1110_DEFINE)
