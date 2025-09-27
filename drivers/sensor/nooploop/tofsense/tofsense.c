/*
 * Copyright (c) 2025 Alex Fabre
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://ftp.nooploop.com/downloads/tofsense/TOFSense_Datasheet_V3.0_en.pdf
 * User manual:
 * https://ftp.nooploop.com/downloads/tofsense/TOFSense_User_Manual_V3.0_en.pdf
 *
 */

#define DT_DRV_COMPAT nooploop_tofsense

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>

#include "tofsense.h"

LOG_MODULE_REGISTER(TOFSense, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_TOFSENSE_BUS_UART

/**
 * @brief Flush the UART receive buffer.
 *
 * Reads and discards all data currently in the UART receive buffer.
 * Used to clear any unwanted or residual data before starting a new
 * communication session with the sensor.
 *
 * @param uart_dev Pointer to the UART device structure.
 */
static void tofsense_uart_clear(const struct device *uart_dev)
{
	uint8_t c;

	while (uart_fifo_read(uart_dev, &c, 1) > 0) {
		;
	}
}

/**
 * @brief Compute the checksum of a TOFSense data frame.
 *
 * This function calculates the checksum for a given data frame by summing up
 * all bytes except the last one. The checksum is used to verify the integrity
 * of the data received from the sensor.
 *
 * @param data Pointer to the data array containing the frame.
 * @param data_size Size of the data array, including the checksum byte.
 *
 * @return The computed checksum as an 8-bit unsigned integer.
 */
static uint8_t tofsense_uart_checksum(const uint8_t *data, const size_t data_size)
{
	uint8_t sum = 0;

	for (size_t i = 0; i < (data_size - 1); i++) {
		sum += data[i];
	}

	return sum;
}

int tofsense_uart_init(const struct device *dev)
{
	const struct tofsense_cfg *cfg = dev->config;
	const struct device *const uart_dev = cfg->bus_cfg.uart_dev;

	int ret;

	if (!device_is_ready(uart_dev)) {
		/* Not ready, do not use */
		LOG_ERR("UART: Device %s not ready.\n", uart_dev->name);
		return -ENODEV;
	}

	uart_irq_rx_disable(uart_dev);
	uart_irq_tx_disable(uart_dev);

	tofsense_uart_clear(uart_dev);

	LOG_INF("Initializing sensor %u in UART %s mode", cfg->id,
		(cfg->operating_mode == TOFSENSE_MODE_QUERY) ? "QUERY" : "ACTIVE");

	ret = uart_irq_callback_user_data_set(uart_dev, cfg->bus_cfg.uart_irq_cb, (void *)dev);

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

	uart_irq_rx_enable(uart_dev);

	return 0;
}

static int tofsense_uart_query_data(const struct device *dev)
{
	const struct tofsense_cfg *cfg = dev->config;

	union {
		uint8_t byte_array[UART_QUERY_FRAME_LENGTH];
		struct tofsense_uart_query_data_frame field;
	} query = {
		.field.header = UART_FRAME_HEADER_BYTE,
		.field.function_mark = UART_QUERY_OUTPUT_PROTOCOL_BYTE,
		.field.id = cfg->id,
		.field.sum_check =
			tofsense_uart_checksum(query.byte_array, UART_QUERY_FRAME_LENGTH),
	};

	for (int i = 0; i < UART_QUERY_FRAME_LENGTH; i++) {
		uart_poll_out(cfg->bus_cfg.uart_dev, query.byte_array[i]);
	}

	return 0;
}

static int tofsense_uart_read_data(const struct device *dev)
{
	const struct tofsense_cfg *cfg = dev->config;
	struct tofsense_data *data = dev->data;

	int64_t_t start = k_uptime_get();
	int64_t duration = 0;

	while (duration <= cfg->communication_timeout) {

		if (data->uart_data_frame_bytes[0] == UART_FRAME_HEADER_BYTE) {
			break;
		}

		duration = (k_uptime_get() - start);
	}

	if (duration > cfg->communication_timeout) {
		/* In active mode, the sensor autonomously sends it's values.
		 * This case handles when no (new) data frame was received in given time.
		 */
		LOG_ERR("No data received from sensor %u after %u ms", cfg->id,
			cfg->communication_timeout);
		return -ENODATA;
	}

	uint8_t checksum;

	checksum = tofsense_uart_checksum(data->uart_data_frame_bytes, UART_DATA_FRAME_LENGTH);
	if (checksum != data->uart_data_frame_bytes[UART_DATA_CHECKSUM_INDEX]) {
		LOG_ERR("Sensor %d, checksum mismatch: calculated 0x%X != data checksum 0x%X",
			cfg->id, checksum, data->uart_data_frame_bytes[UART_DATA_CHECKSUM_INDEX]);
		LOG_HEXDUMP_ERR(data->uart_data_frame_bytes, UART_DATA_FRAME_LENGTH, "Rx data");

		return -EBADMSG;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	data->system_time = data->uart_data_frame.data.system_time;
	data->distance_mm = data->uart_data_frame.data.distance.value_mm;
	data->distance_status = data->uart_data_frame.data.distance.status;
	data->signal_strength = data->uart_data_frame.data.signal_strength;

	k_mutex_unlock(&data->mutex);

	/* Once the data frame has been fetched, it gets cleared. It let us detect if no new data
	 * frame is received upon next fetch request.
	 */
	BUFFER_CLEAR(data->uart_data_frame_bytes);

	return 0;
}

static void tofsense_uart_isr(const struct device *uart_dev, void *user_data)
{
	const struct device *dev = user_data;
	struct tofsense_data *data = dev->data;

	if (uart_dev == NULL) {
		LOG_ERR("UART device is NULL");
		return;
	}

	if (!uart_irq_update(uart_dev)) {
		LOG_ERR("Unable to start processing interrupts");
		return;
	}

	if (uart_irq_rx_ready(uart_dev)) {
		data->nb_frame_bytes_received += uart_fifo_read(
			uart_dev, &data->uart_rx_buffer[data->nb_frame_bytes_received],
			UART_DATA_FRAME_LENGTH - data->nb_frame_bytes_received);

		/* The first byte should be UART_FRAME_HEADER_BYTE for a valid read.
		 * If we do not read UART_FRAME_HEADER_BYTE on what we think is the
		 * first byte, then reset the number of bytes read until we do.
		 */
		if ((data->uart_rx_buffer[0] != UART_FRAME_HEADER_BYTE) &
		    (data->nb_frame_bytes_received == 1)) {
			LOG_DBG("First byte is not a valid header (0x%2X). Resetting # of bytes "
				"read.",
				UART_FRAME_HEADER_BYTE);
			data->nb_frame_bytes_received = 0;
			BUFFER_CLEAR(data->uart_rx_buffer);
		}

		if (data->nb_frame_bytes_received == UART_DATA_FRAME_LENGTH) {

			BUFFER_COPY(data->uart_data_frame_bytes, data->uart_rx_buffer);

			LOG_HEXDUMP_DBG(data->uart_rx_buffer, UART_DATA_FRAME_LENGTH, "Rx data");

			tofsense_uart_clear(uart_dev);
			data->nb_frame_bytes_received = 0;
			BUFFER_CLEAR(data->uart_rx_buffer);
		}
	}
}

#endif /* CONFIG_TOFSENSE_BUS_UART */

#ifdef CONFIG_TOFSENSE_BUS_CAN

static int tofsense_can_query_data(const struct device *dev)
{
	const struct tofsense_cfg *cfg = dev->config;

	LOG_DBG("CAN query to sensor %d", cfg->id);

	union {
		uint8_t byte_array[CAN_QUERY_FRAME_LENGTH];
		struct tofsense_can_query_data_frame field;
	} query = {/* All <reserved_*> fields have to be set to 0xFF during a request */
		   .field.reserved_0 = 0xFFFF,
		   .field.reserved_1 = 0xFF,
		   .field.id = cfg->id,
		   .field.reserved_2 = 0xFFFFFFFF};

	struct can_frame frame = {
		.id = CAN_TOFSENSE_QUERY_ID,
		.dlc = CAN_QUERY_FRAME_LENGTH,
	};

	memcpy(&frame.data, &query, CAN_QUERY_FRAME_LENGTH);

	return can_send(cfg->bus_cfg.can_dev, &frame, K_MSEC(cfg->communication_timeout), NULL,
			NULL);
}

static int tofsense_can_read_data(const struct device *dev)
{
	struct tofsense_data *data = dev->data;

	k_mutex_lock(&data->mutex, K_FOREVER);

	data->distance_mm = data->latest_can_data_received.distance.value_mm;
	data->distance_status = data->latest_can_data_received.distance.status;
	data->signal_strength = data->latest_can_data_received.signal_strength;

	k_mutex_unlock(&data->mutex);
	return 0;
}

static void tofsense_can_isr(const struct device *can_dev, struct can_frame *frame, void *user_data)
{
	const struct device *dev = user_data;
	struct tofsense_data *data = dev->data;
	const struct tofsense_cfg *cfg = dev->config;
	struct tofsense_can_data_frame tmp_data;

	if (can_dev == NULL) {
		LOG_ERR("CAN device is NULL");
		return;
	}

	memcpy(&tmp_data, frame->data, CAN_DATA_FRAME_LENGTH);

	if (tmp_data.distance.status == DISTANCE_STATUS_OKAY) {
		memcpy(&data->latest_can_data_received, &tmp_data, CAN_DATA_FRAME_LENGTH);
	} else {
		/* Do not save the frame content as the distance value may be false
		 * Do not log it as an error as it's quite common.
		 */
		LOG_DBG("Sensor %d, distance status error: %u", cfg->id, tmp_data.distance.status);
	}

	LOG_DBG("Sensor %d, CAN Read: (0x%x %x %x %x %x %x %x %x)", cfg->id, frame->data[0],
		frame->data[1], frame->data[2], frame->data[3], frame->data[4], frame->data[5],
		frame->data[6], frame->data[7]);
}

int tofsense_can_bus_init(const struct device *dev)
{
	const struct tofsense_cfg *cfg = dev->config;
	const struct device *const can_dev = cfg->bus_cfg.can_dev;
	int ret = 0;

	LOG_INF("Initializing sensor %u in CAN %s mode", cfg->id,
		(cfg->operating_mode == TOFSENSE_MODE_QUERY) ? "QUERY" : "ACTIVE");

	const struct can_filter tofsense_filter = {
		.id = (CAN_TOFSENSE_RECEIVE_ID_BASE + cfg->id),
		.mask = CAN_STD_ID_MASK,
		.flags = 0U,
	};

	if (!device_is_ready(can_dev)) {
		LOG_ERR("CAN: Device %s not ready.\n", can_dev->name);
		return -ENODEV;
	}

	ret = can_start(can_dev);
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("Error starting CAN controller [%d]", ret);
		return ret;
	}

	ret = can_add_rx_filter(can_dev, cfg->bus_cfg.can_irq_cb, (void *)dev, &tofsense_filter);
	if (ret == -ENOSPC) {
		LOG_ERR("Error, no filter available!\n");
		return 0;
	}

	return 0;
}
#endif /* CONFIG_TOFSENSE_BUS_CAN */

static inline int tofsense_poll_data(const struct device *dev)
{
	const struct tofsense_cfg *cfg = dev->config;
	int ret = 0;

	if (cfg->operating_mode == TOFSENSE_MODE_QUERY) {
		ret = cfg->query_data(dev);
	}

	if (ret) {
		LOG_ERR("Sensor %d, query send failed", cfg->id);
		return ret;
	}

	return cfg->read_data(dev);
}

static int tofsense_channel_get(const struct device *dev, enum sensor_channel chan,
				struct sensor_value *val)
{
	struct tofsense_data *data = dev->data;

	if (chan != SENSOR_CHAN_DISTANCE) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	/* val1 is meters, val2 is microns. Both are int32_t
	 * data->distance_mm is in mm and units of uint32_t
	 */
	val->val1 = (data->distance_mm / 1000);
	val->val2 = (int32_t)((data->distance_mm % 1000) * 1000);

	k_mutex_unlock(&data->mutex);

	return 0;
}

static int tofsense_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_DISTANCE);

	if (chan == SENSOR_CHAN_DISTANCE || chan == SENSOR_CHAN_ALL) {
		return tofsense_poll_data(dev);
	}

	return -ENOTSUP;
}

static DEVICE_API(sensor, tofsense_api_funcs) = {
	.sample_fetch = tofsense_sample_fetch,
	.channel_get = tofsense_channel_get,
};

static int tofsense_init(const struct device *dev)
{
	struct tofsense_data *data = dev->data;
	const struct tofsense_cfg *cfg = dev->config;

	k_mutex_init(&data->mutex);

	return cfg->bus_init(dev);
}

/*
 * Device configuration macro, shared by TOFSENSE_CONFIG_UART() and
 * TOFSENSE_CONFIG_CAN().
 */

#define TOFSENSE_CONFIG_COMMON(inst)                                                               \
	.id = DT_INST_PROP(inst, id), .operating_mode = DT_INST_PROP(inst, operating_mode),        \
	.communication_timeout = ((1000 / DT_INST_PROP_OR(inst, active_mode_frequency, 30)) + 1)

/*
 * Device creation macro, shared by TOFSENSE_DEFINE_UART() and
 * TOFSENSE_DEFINE_CAN().
 */

#define TOFSENSE_DEVICE_INIT(inst)                                                                 \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, tofsense_init, NULL, &tofsense_data_##inst,             \
				     &tofsense_cfg_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &tofsense_api_funcs);

/*
 * Instantiation macros used when a device is on an UART bus.
 */

#define TOFSENSE_CONFIG_UART(inst)                                                                 \
	{                                                                                          \
		.bus_cfg.uart_dev = DEVICE_DT_GET(DT_INST_BUS(inst)),                              \
		.bus_cfg.uart_irq_cb = tofsense_uart_isr,                                          \
		.bus_init = tofsense_uart_init,                                                    \
		.query_data = tofsense_uart_query_data,                                            \
		.read_data = tofsense_uart_read_data,                                              \
		TOFSENSE_CONFIG_COMMON(inst),                                                      \
	}

#define TOFSENSE_DEFINE_UART(inst)                                                                 \
	static struct tofsense_data tofsense_data_##inst;                                          \
	static const struct tofsense_cfg tofsense_cfg_##inst = TOFSENSE_CONFIG_UART(inst);         \
	TOFSENSE_DEVICE_INIT(inst)

/*
 * Instantiation macros used when a device is on an CAN bus.
 */

#define TOFSENSE_CONFIG_CAN(inst)                                                                  \
	{                                                                                          \
		.bus_cfg.can_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                            \
		.bus_cfg.can_irq_cb = tofsense_can_isr,                                            \
		.bus_init = tofsense_can_bus_init,                                                 \
		.query_data = tofsense_can_query_data,                                             \
		.read_data = tofsense_can_read_data,                                               \
		TOFSENSE_CONFIG_COMMON(inst),                                                      \
	}

#define TOFSENSE_DEFINE_CAN(inst)                                                                  \
	static struct tofsense_data tofsense_data_##inst;                                          \
	static const struct tofsense_cfg tofsense_cfg_##inst = TOFSENSE_CONFIG_CAN(inst);          \
	TOFSENSE_DEVICE_INIT(inst)

#define TOFSENSE_DEFINE(inst)                                                                      \
	COND_CODE_1(DT_INST_ON_BUS(inst, uart), (TOFSENSE_DEFINE_UART(inst)),                      \
		    (TOFSENSE_DEFINE_CAN(inst)))

DT_INST_FOREACH_STATUS_OKAY(TOFSENSE_DEFINE)
