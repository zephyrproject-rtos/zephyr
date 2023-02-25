/*
 * Copyright (c) 2023 Fabian Blatz
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Datasheet:
 * https://dfimg.dfrobot.com/nobody/wiki/3b1c7de28d8343b114c3ab6057f817e2.pdf
 */

#define DT_DRV_COMPAT hilink_ld2410

#include <string.h>
#include <stdbool.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include "hilink_ld2410.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LD2410, CONFIG_SENSOR_LOG_LEVEL);

/*
 *   Command Frame:
 *   +-------------+-----------------+------------+-------------------------+
 *   | Header      | Body Length (N) | Command ID | Body      | Footer      |
 *   | FD FC FB FA | 2 Bytes         | 2 Bytes    | N-2 Bytes | 04 03 02 01 |
 *   +-------------+-----------------+------------+-------------------------+
 *
 *   Command Response Frame:
 *   +-------------+-----------------+---------------------+-----------+-------------+-------------+
 *   | Header      | Body Length (N) | Command ID | 0x0100 | ACK       | Body        | Footer      |
 *   | FD FC FB FA | 2 Bytes         | 2 Bytes             | 2 Bytes   | N-4 Bytes   | 04 03 02 01 |
 *   +-------------+-----------------+---------------------+-----------+-------------+-------------+
 *
 *   Cyclic Data Frame:
 *   +-------------+-----------------+---------------------+-------------+
 *   | Header      | Body Length (N) | Body                | Footer      |
 *   | F4 F3 F2 F1 | 2 Bytes         | N Bytes             | F8 F7 F6 F5 |
 *   +-------------+-----------------+---------------------+-------------+
 */

const uint32_t DATA_FRAME_HEADER = 0xF1F2F3F4;
const uint32_t DATA_FRAME_FOOTER = 0xF5F6F7F8;
const uint32_t CMD_FRAME_HEADER = 0xFAFBFCFD;
const uint32_t CMD_FRAME_FOOTER = 0x01020304;

#define SERIAL_TIMEOUT_MS            200
#define CMD_ID_SIZE                  sizeof(uint16_t)
#define FRAME_HEADER_AND_SIZE_LENGTH (offsetof(struct ld2410_frame_data, body))
#define ACK_BODY_DATA_START          4
#define BODY_MAGIC_HEADER_BYTE       0xAA
#define CYCLIC_MAGIC_BODY_FOOTER     0x0055
#define CYCLIC_WITH_ENGIN_DATA       0x01

#define DISTANCE_GATE_PARAM_WORD 0x0000
#define MOTION_PARAM_WORD        0x0001
#define STATIONARY_PARAM_WORD    0x0002

#define MAX_MOV_GATE_PARAM_WORD     0x0000
#define MAX_STA_GATE_PARAM_WORD     0x0001
#define PRESENCE_TIMEOUT_PARAM_WORD 0x0002

enum ld2410_command {
	ENTER_CONFIG_MODE = 0x00FF,
	LEAVE_CONFIG_MODE = 0x00FE,
	SET_MAX_GATES_AND_DURATION = 0x0060,
	READ_SETTINGS = 0x0061,
	ENTER_ENGINEERING_MODE = 0x0062,
	LEAVE_ENGINEERING_MODE = 0x0063,
	SET_GATE_SENSITIVITY_CONFIG = 0x0064,
	SET_DISTANCE_RESOLUTION = 0x00AA,
	GET_DISTANCE_RESOLUTION = 0x00AB
};

static int find_rx_frame_start(struct ld2410_frame *rx_frame, enum ld2410_frame_type expected_type)
{
	size_t frame_start = 0;
	size_t frame_length;

	/* Ensure at least frame header info is read */
	if (rx_frame->byte_count < FRAME_HEADER_AND_SIZE_LENGTH + FRAME_FOOTER_SIZE) {
		return -EAGAIN;
	}

	/* Locate the start of the frame */
	for (;;) {
		if (expected_type == DATA_FRAME && rx_frame->data.header == DATA_FRAME_HEADER) {
			break;
		}

		if (expected_type == ACK_FRAME && rx_frame->data.header == CMD_FRAME_HEADER) {
			break;
		}

		if (frame_start + FRAME_HEADER_SIZE >= rx_frame->byte_count) {
			/* If the buffer is full shift but leave at least header bytes in buffer */
			if (frame_start + FRAME_HEADER_SIZE >= sizeof(rx_frame->raw)) {
				memmove(&rx_frame->raw[0],
					&rx_frame->raw[sizeof(rx_frame->raw) -
						       FRAME_HEADER_AND_SIZE_LENGTH - 1],
					FRAME_HEADER_AND_SIZE_LENGTH);
				rx_frame->byte_count = FRAME_HEADER_AND_SIZE_LENGTH;
			}
			LOG_DBG("Header not found in bytes read");
			return -EAGAIN;
		}
		frame_start++;
	}

	if (frame_start != 0) {
		/* Shift frame to start of the buffer */
		frame_length = rx_frame->byte_count - frame_start;
		memmove(&rx_frame->raw[0], &rx_frame->raw[frame_start], frame_length);
		rx_frame->byte_count -= frame_start;
	}

	if (rx_frame->data.body_len >= LD2410_MAX_FRAME_BODYLEN) {
		/* Length information is implausible, discard buffer */
		LOG_DBG("Implausible length information: %u", rx_frame->data.body_len);
		rx_frame->byte_count = 0;
		return -EBADMSG;
	}

	if (rx_frame->byte_count <
	    FRAME_HEADER_AND_SIZE_LENGTH + rx_frame->data.body_len + FRAME_FOOTER_SIZE) {
		return -EAGAIN;
	}

	if (expected_type == DATA_FRAME &&
	    sys_get_le32(&rx_frame->data.body[rx_frame->data.body_len]) != DATA_FRAME_FOOTER) {
		LOG_DBG("Data frame footer mismatch");
		rx_frame->byte_count = 0;
		return -EBADMSG;
	}

	if (expected_type == ACK_FRAME &&
	    sys_get_le32(&rx_frame->data.body[rx_frame->data.body_len]) != CMD_FRAME_FOOTER) {
		LOG_DBG("ACK frame footer mismatch");
		rx_frame->byte_count = 0;
		return -EBADMSG;
	}

	return expected_type;
}

static void uart_tx_cb_handler(const struct device *dev)
{
	const struct ld2410_config *config = dev->config;
	struct ld2410_data *drv_data = dev->data;
	int sent = 0;
	uint8_t retries = 3;

	if (drv_data->tx_frame.byte_count) {
		LOG_HEXDUMP_DBG(&drv_data->tx_frame.raw[0], drv_data->tx_frame.byte_count, "TX");
	}

	while (drv_data->tx_frame.byte_count > 0) {
		sent = uart_fifo_fill(config->uart_dev, &drv_data->tx_frame.raw[sent],
				      drv_data->tx_frame.byte_count);
		drv_data->tx_frame.byte_count -= sent;
	}

	while (retries--) {
		if (uart_irq_tx_complete(config->uart_dev)) {
			uart_irq_tx_disable(config->uart_dev);
			drv_data->rx_frame.byte_count = 0;
			uart_irq_rx_enable(config->uart_dev);
			k_sem_give(&drv_data->tx_sem);
			break;
		}
	}
}

static void uart_cb_handler(const struct device *uart_dev, void *user_data)
{
	const struct device *dev = user_data;
	struct ld2410_data *drv_data = dev->data;
	size_t rx_available_space;
	int found_frame;

	if (!uart_dev || uart_irq_update(uart_dev) < 0) {
		return;
	}

	if (uart_irq_tx_ready(uart_dev)) {
		uart_tx_cb_handler(dev);
	}

	rx_available_space = sizeof(drv_data->rx_frame.raw) - drv_data->rx_frame.byte_count;

	while (uart_irq_rx_ready(uart_dev) > 0 && rx_available_space) {
		drv_data->rx_frame.byte_count += uart_fifo_read(
			uart_dev, &drv_data->rx_frame.raw[drv_data->rx_frame.byte_count],
			rx_available_space);

		found_frame =
			find_rx_frame_start(&drv_data->rx_frame, drv_data->awaited_rx_frame_type);
		if (found_frame >= 0) {
			LOG_HEXDUMP_DBG(&drv_data->rx_frame.raw[0], drv_data->rx_frame.byte_count,
					"RX");
			uart_irq_rx_disable(uart_dev);
			k_sem_give(&drv_data->rx_sem);
			break;
		}

		rx_available_space = sizeof(drv_data->rx_frame.raw) - drv_data->rx_frame.byte_count;
	}
}

static void ld2410_uart_flush(const struct device *dev)
{
	uint8_t c;

	while (uart_fifo_read(dev, &c, 1) > 0) {
		continue;
	}
}

static int transceive_command(const struct device *dev, enum ld2410_command command, uint8_t *data,
			      uint16_t data_len)
{
	struct ld2410_data *drv_data = dev->data;
	const struct ld2410_config *drv_cfg = dev->config;
	int ret;

	if (CMD_ID_SIZE + data_len >= LD2410_MAX_FRAME_BODYLEN) {
		return -EINVAL;
	}

	/* Make sure last command has been transferred */
	ret = k_sem_take(&drv_data->tx_sem, K_MSEC(SERIAL_TIMEOUT_MS));
	if (ret) {
		return ret;
	}

	k_sem_reset(&drv_data->rx_sem);
	drv_data->awaited_rx_frame_type = ACK_FRAME;

	drv_data->tx_frame.data.header = CMD_FRAME_HEADER;
	drv_data->tx_frame.data.body_len = data_len + CMD_ID_SIZE;
	sys_put_le16(command, &drv_data->tx_frame.data.body[0]);
	memcpy(&drv_data->tx_frame.data.body[CMD_ID_SIZE], data, data_len);
	sys_put_le32(CMD_FRAME_FOOTER, &drv_data->tx_frame.data.body[CMD_ID_SIZE + data_len]);

	drv_data->tx_frame.byte_count =
		FRAME_HEADER_AND_SIZE_LENGTH + CMD_ID_SIZE + data_len + FRAME_FOOTER_SIZE;

	uart_irq_tx_enable(drv_cfg->uart_dev);

	ret = k_sem_take(&drv_data->rx_sem, K_MSEC(SERIAL_TIMEOUT_MS));
	if (ret) {
		LOG_DBG("Awaiting rx message timedout");
		uart_irq_rx_disable(drv_cfg->uart_dev);
		return ret;
	}

	/* Assert frame is a command response */
	__ASSERT(drv_data->rx_frame.data.header == CMD_FRAME_HEADER,
		 "Header does not contain magic value");

	/* Verify command id is contained */
	if (sys_get_le16(&drv_data->rx_frame.data.body[0]) != (command | 0x0100)) {
		LOG_DBG("Message did not contain expected command|0x0100");
		return -EIO;
	}

	/* Check return value */
	if (sys_get_le16(&drv_data->rx_frame.data.body[CMD_ID_SIZE])) {
		LOG_DBG("Non zero ack state");
		return -EIO;
	}

	return 0;
}

static inline int set_config_mode(const struct device *dev, bool enabled)
{
	uint8_t payload[2] = {0};

	if (enabled) {
		payload[0] = 0x01;
		return transceive_command(dev, ENTER_CONFIG_MODE, payload, sizeof(payload));
	} else {
		return transceive_command(dev, LEAVE_CONFIG_MODE, payload, sizeof(payload));
	}
}

static int transceive_in_cfg_mode(const struct device *dev, enum ld2410_command command,
				  uint8_t *data, uint16_t len, struct ld2410_frame *rx_frame)
{
	int ret;
	struct ld2410_data *drv_data = dev->data;

	k_mutex_lock(&drv_data->lock, K_FOREVER);

	ret = set_config_mode(dev, true);
	if (ret < 0) {
		goto unlock;
	}
	ret = transceive_command(dev, command, data, len);
	if (rx_frame != NULL && ret == 0) {
		memcpy(rx_frame, &drv_data->rx_frame, sizeof(struct ld2410_frame));
	}
	set_config_mode(dev, false);

unlock:
	k_mutex_unlock(&drv_data->lock);
	return ret;
}

static inline int set_engineering_mode(const struct device *dev, bool enabled)
{
	if (enabled) {
		return transceive_in_cfg_mode(dev, ENTER_ENGINEERING_MODE, NULL, 0, NULL);
	} else {
		return transceive_in_cfg_mode(dev, LEAVE_ENGINEERING_MODE, NULL, 0, NULL);
	}
}

static inline int set_distance_resolution(const struct device *dev,
					  enum ld2410_gate_resolution resolution)
{
	uint8_t payload[2];

	if (resolution != LD2410_GATE_RESOLUTION_20 && resolution != LD2410_GATE_RESOLUTION_75) {
		return -EINVAL;
	}

	sys_put_le16((uint16_t)resolution, payload);
	return transceive_in_cfg_mode(dev, SET_DISTANCE_RESOLUTION, payload, sizeof(payload), NULL);
}

static inline int get_distance_resolution(const struct device *dev,
					  enum ld2410_gate_resolution *resolution)
{
	int ret;
	struct ld2410_frame rx_frame = {0};

	ret = transceive_in_cfg_mode(dev, GET_DISTANCE_RESOLUTION, NULL, 0, &rx_frame);
	if (ret == 0) {
		*resolution = sys_get_le16(&rx_frame.data.body[ACK_BODY_DATA_START]);
	}

	return ret;
}

static int read_settings(const struct device *dev)
{
	int ret;
	struct ld2410_data *drv_data = dev->data;
	struct ld2410_frame rx_frame = {0};

	ret = transceive_in_cfg_mode(dev, READ_SETTINGS, NULL, 0, &rx_frame);

	if (ret == 0) {
		/* Check for header byte */
		if (rx_frame.data.body[ACK_BODY_DATA_START] != BODY_MAGIC_HEADER_BYTE) {
			LOG_ERR("Setting read response non matching header byte");
			return -EBADMSG;
		}
		memcpy(&drv_data->settings, &rx_frame.data.body[ACK_BODY_DATA_START + 1],
		       sizeof(struct ld2410_settings));
	}

	return ret;
}

static int set_gate_sensitivities(const struct device *dev, const uint8_t *motion_sensitivity,
				  const uint8_t *stationary_sensitivity)
{
	int ret;
	int single_rc;
	struct ld2410_data *drv_data = dev->data;
	uint8_t payload[18];

	k_mutex_lock(&drv_data->lock, K_FOREVER);

	ret = set_config_mode(dev, true);
	if (ret < 0) {
		goto unlock;
	}

	sys_put_le16(DISTANCE_GATE_PARAM_WORD, &payload[0]);
	sys_put_le16(MOTION_PARAM_WORD, &payload[6]);
	sys_put_le16(STATIONARY_PARAM_WORD, &payload[12]);

	for (uint8_t i = 0; i < LD2410_GATE_COUNT; i++) {
		sys_put_le32(i, &payload[2]);
		sys_put_le32(motion_sensitivity[i], &payload[8]);
		sys_put_le32(stationary_sensitivity[i], &payload[14]);

		single_rc = transceive_command(dev, SET_GATE_SENSITIVITY_CONFIG, payload,
					       sizeof(payload));

		if (single_rc < 0) {
			LOG_ERR("Could not set config for gate: %u", i);
		}

		ret |= single_rc;
	}

	set_config_mode(dev, false);

unlock:
	k_mutex_unlock(&drv_data->lock);
	return ret;
}

static int set_max_gates_and_duration(const struct device *dev, uint8_t max_motion_gate,
				      uint8_t max_stationary_gate, uint16_t presence_timeout)
{
	uint8_t payload[18];

	sys_put_le16(MAX_MOV_GATE_PARAM_WORD, &payload[0]);
	sys_put_le32(max_motion_gate, &payload[2]);

	sys_put_le16(MAX_STA_GATE_PARAM_WORD, &payload[6]);
	sys_put_le32(max_stationary_gate, &payload[8]);

	sys_put_le16(PRESENCE_TIMEOUT_PARAM_WORD, &payload[12]);
	sys_put_le32(presence_timeout, &payload[14]);

	return transceive_in_cfg_mode(dev, SET_MAX_GATES_AND_DURATION, payload, sizeof(payload),
				      NULL);
}

int ld2410_attr_set(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		    const struct sensor_value *val)
{
	struct ld2410_data *drv_data = dev->data;
	uint8_t tmp_sensitivity[LD2410_GATE_COUNT] = {0};

	switch ((enum sensor_attribute_ld2410)attr) {
	case SENSOR_ATTR_LD2410_ENGINEERING_MODE:
		return set_engineering_mode(dev, val->val1);
	case SENSOR_ATTR_LD2410_DISTANCE_RESOLUTION:
		return set_distance_resolution(dev, val->val1);
	case SENSOR_ATTR_LD2410_MOTION_SENSITIVITY_PER_GATE:
		for (int i = 0; i < LD2410_GATE_COUNT; i++) {
			tmp_sensitivity[i] = val->val1;
		}
		return set_gate_sensitivities(dev, &tmp_sensitivity[0],
					      &drv_data->settings.stationary_gate_sensitivity[0]);
	case SENSOR_ATTR_LD2410_STATIONARY_SENSITIVITY_PER_GATE:
		for (int i = 0; i < LD2410_GATE_COUNT; i++) {
			tmp_sensitivity[i] = val->val1;
		}
		return set_gate_sensitivities(dev, &drv_data->settings.motion_gate_sensitivity[0],
					      &tmp_sensitivity[0]);
	case SENSOR_ATTR_LD2410_MAX_MOTION_GATE:
		return set_max_gates_and_duration(dev, val->val1,
						  drv_data->settings.max_stationary_gate,
						  drv_data->settings.presence_timeout);
	case SENSOR_ATTR_LD2410_MAX_STATIONARY_GATE:
		return set_max_gates_and_duration(dev, drv_data->settings.max_motion_gate,
						  val->val1, drv_data->settings.presence_timeout);
	case SENSOR_ATTR_LD2410_PRESENCE_TIMEOUT:
		return set_max_gates_and_duration(dev, drv_data->settings.max_motion_gate,
						  drv_data->settings.max_stationary_gate,
						  val->val1);
	default:
		return -ENOTSUP;
	}

	return 0;
}

int ld2410_attr_get(const struct device *dev, enum sensor_channel chan, enum sensor_attribute attr,
		    struct sensor_value *val)
{
	struct ld2410_data *drv_data = dev->data;
	int ret;

	switch ((enum sensor_attribute_ld2410)attr) {
	case SENSOR_ATTR_LD2410_ENGINEERING_MODE:
		val->val1 = (drv_data->cyclic_data.data_type == CYCLIC_WITH_ENGIN_DATA);
		ret = 0;
		break;
	case SENSOR_ATTR_LD2410_DISTANCE_RESOLUTION:
		ret = get_distance_resolution(dev, (enum ld2410_gate_resolution *)&val->val1);
		break;
	case SENSOR_ATTR_LD2410_MOTION_SENSITIVITY_PER_GATE:
		ret = read_settings(dev);
		if (ret == 0) {
			for (int i = 0; i < LD2410_GATE_COUNT; i++) {
				val[i].val1 = drv_data->settings.motion_gate_sensitivity[i];
			}
		}
		break;
	case SENSOR_ATTR_LD2410_STATIONARY_SENSITIVITY_PER_GATE:
		ret = read_settings(dev);
		if (ret == 0) {
			for (int i = 0; i < LD2410_GATE_COUNT; i++) {
				val[i].val1 = drv_data->settings.stationary_gate_sensitivity[i];
			}
		}
		break;
	case SENSOR_ATTR_LD2410_MAX_MOTION_GATE:
		ret = read_settings(dev);
		if (ret == 0) {
			val->val1 = drv_data->settings.max_motion_gate;
		}
		break;
	case SENSOR_ATTR_LD2410_MAX_STATIONARY_GATE:
		ret = read_settings(dev);
		if (ret == 0) {
			val->val1 = drv_data->settings.max_stationary_gate;
		}
		break;
	case SENSOR_ATTR_LD2410_PRESENCE_TIMEOUT:
		ret = read_settings(dev);
		if (ret == 0) {
			val->val1 = drv_data->settings.presence_timeout;
		}
		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

static int ld2410_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ld2410_data *drv_data = dev->data;
	const struct ld2410_config *drv_cfg = dev->config;
	int ret;
	bool in_engineering_mode = false;
	size_t data_end = sizeof(struct ld2410_cyclic_data);

	ARG_UNUSED(chan);

	k_mutex_lock(&drv_data->lock, K_FOREVER);

	drv_data->rx_frame.byte_count = 0;
	drv_data->awaited_rx_frame_type = DATA_FRAME;
	k_sem_reset(&drv_data->rx_sem);
	uart_irq_rx_enable(drv_cfg->uart_dev);

	ret = k_sem_take(&drv_data->rx_sem, K_MSEC(SERIAL_TIMEOUT_MS));
	if (ret < 0) {
		uart_irq_rx_disable(drv_cfg->uart_dev);
		goto unlock;
	}

	if (drv_data->rx_frame.data.body_len < sizeof(struct ld2410_cyclic_data)) {
		LOG_DBG("Unexpected size field");
		ret = -EBADMSG;
		goto unlock;
	}
	memcpy(&drv_data->cyclic_data, &drv_data->rx_frame.data.body[0],
	       sizeof(struct ld2410_cyclic_data));
	in_engineering_mode = drv_data->cyclic_data.data_type == CYCLIC_WITH_ENGIN_DATA;

	if (drv_data->cyclic_data.header_byte != BODY_MAGIC_HEADER_BYTE) {
		LOG_DBG("No magic header byte found");
		ret = -EBADMSG;
		goto unlock;
	}

	if (in_engineering_mode) {
		data_end += sizeof(struct ld2410_engineering_data);
	}

	if (sys_get_le16(&drv_data->rx_frame.data.body[data_end]) != CYCLIC_MAGIC_BODY_FOOTER) {
		LOG_DBG("No magic cyclic body footer found");
		ret = -EBADMSG;
		goto unlock;
	}

	if (in_engineering_mode) {
		memcpy(&drv_data->engineering_data,
		       &drv_data->rx_frame.data.body[sizeof(struct ld2410_cyclic_data)],
		       sizeof(struct ld2410_engineering_data));
	}

unlock:
	k_mutex_unlock(&drv_data->lock);
	return ret;
}

static int ld2410_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct ld2410_data *drv_data = dev->data;

	switch ((enum sensor_channel_ld2410)chan) {
	case SENSOR_CHAN_LD2410_MOTION_TARGET_DISTANCE:
		val->val1 = drv_data->cyclic_data.motion_target_distance;
		break;
	case SENSOR_CHAN_LD2410_MOTION_TARGET_ENERGY:
		val->val1 = drv_data->cyclic_data.motion_target_energy;
		break;
	case SENSOR_CHAN_LD2410_STATIONARY_TARGET_DISTANCE:
		val->val1 = drv_data->cyclic_data.stationary_target_distance;
		break;
	case SENSOR_CHAN_LD2410_STATIONARY_TARGET_ENERGY:
		val->val1 = drv_data->cyclic_data.stationary_target_energy;
		break;
	case SENSOR_CHAN_LD2410_TARGET_TYPE:
		val->val1 = drv_data->cyclic_data.target_type;
		break;
	case SENSOR_CHAN_LD2410_MOTION_ENERGY_PER_GATE:
		if (drv_data->cyclic_data.data_type != CYCLIC_WITH_ENGIN_DATA) {
			return -ENODATA;
		}
		for (int i = 0; i < LD2410_GATE_COUNT; i++) {
			val[i].val1 = drv_data->engineering_data.motion_energy_per_gate[i];
		}
		break;
	case SENSOR_CHAN_LD2410_STATIONARY_ENERGY_PER_GATE:
		if (drv_data->cyclic_data.data_type != CYCLIC_WITH_ENGIN_DATA) {
			return -ENODATA;
		}
		for (int i = 0; i < LD2410_GATE_COUNT; i++) {
			val[i].val1 = drv_data->engineering_data.stationary_energy_per_gate[i];
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api ld2410_api = {
	.sample_fetch = &ld2410_sample_fetch,
	.channel_get = &ld2410_channel_get,
	.attr_get = &ld2410_attr_get,
	.attr_set = &ld2410_attr_set,
#ifdef CONFIG_LD2410_TRIGGER
	.trigger_set = &ld2410_trigger_set,
#endif
};

static int ld2410_init(const struct device *dev)
{
	const struct ld2410_config *drv_cfg = dev->config;
	struct ld2410_data *drv_data = dev->data;
	int ret;

	if (!device_is_ready(drv_cfg->uart_dev)) {
		LOG_ERR("Bus device is not ready");
		return -ENODEV;
	}

	uart_irq_rx_disable(drv_cfg->uart_dev);
	uart_irq_tx_disable(drv_cfg->uart_dev);

	ld2410_uart_flush(drv_cfg->uart_dev);

	k_sem_init(&drv_data->rx_sem, 0, 1);
	k_sem_init(&drv_data->tx_sem, 1, 1);
	k_mutex_init(&drv_data->lock);

	uart_irq_callback_user_data_set(drv_cfg->uart_dev, uart_cb_handler, (void *)dev);

#ifdef CONFIG_LD2410_TRIGGER
	ret = ld2410_init_interrupt(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize interrupt!");
		return ret;
	}
#endif

	ret = set_engineering_mode(dev, drv_cfg->engineering_mode);
	if (ret < 0) {
		LOG_ERR("Error setting engineering mode: %d", ret);
		return ret;
	}

	ret = set_distance_resolution(dev, drv_cfg->distance_resolution);
	if (ret < 0) {
		LOG_ERR("Error setting distance resolution: %d", ret);
		return ret;
	}

	ret = set_gate_sensitivities(dev, &drv_data->settings.motion_gate_sensitivity[0],
				     &drv_data->settings.stationary_gate_sensitivity[0]);
	if (ret < 0) {
		LOG_ERR("Error setting per gate sensitivity: %d", ret);
		return ret;
	}

	ret = set_max_gates_and_duration(dev, drv_data->settings.max_motion_gate,
					 drv_data->settings.max_stationary_gate,
					 drv_data->settings.presence_timeout);
	if (ret < 0) {
		LOG_ERR("Error setting max gates and presence timeout: %d", ret);
		return ret;
	}

	return 0;
}

#define INST_MAX_MOV_GATE(inst)                                                                    \
	DT_INST_PROP_OR(inst, max_motion_gate, LD2410_DEFAULT_MAX_MOTION_GATE)
#define INST_MAX_STA_GATE(inst)                                                                    \
	DT_INST_PROP_OR(inst, max_stationary_gate, LD2410_DEFAULT_MAX_STATIONARY_GATE)
#define INST_PRESEN_TIMEOUT(inst)                                                                  \
	DT_INST_PROP_OR(inst, presence_timeout, LD2410_DEFAULT_PRESENCE_TIMEOUT)

#define LD2410_DEFINE(inst)                                                                        \
	BUILD_ASSERT(DT_INST_PROP_LEN_OR(inst, motion_sensitivity, LD2410_GATE_COUNT) ==           \
			     LD2410_GATE_COUNT,                                                    \
		     "ld2410: Error motion-sensitivity len is different of 9");                    \
	BUILD_ASSERT(DT_INST_PROP_LEN_OR(inst, stationary_sensitivity, LD2410_GATE_COUNT) ==       \
			     LD2410_GATE_COUNT,                                                    \
		     "ld2410: Error stationary-sensitivity len is different of 9");                \
	BUILD_ASSERT(INST_MAX_MOV_GATE(inst) >= 2 && INST_MAX_MOV_GATE(inst) < LD2410_GATE_COUNT,  \
		     "ld2410: Error max-motion-gate needs to be in range 2-8");                    \
	BUILD_ASSERT(INST_MAX_STA_GATE(inst) >= 2 && INST_MAX_STA_GATE(inst) < LD2410_GATE_COUNT,  \
		     "ld2410: Error max-stationary-gate needs to be in range 2-8");                \
	BUILD_ASSERT(INST_PRESEN_TIMEOUT(inst) > 0 && INST_PRESEN_TIMEOUT(inst) <= 65535,          \
		     "ld2410: Error presence-timeout needs to be in range 0-65535");               \
                                                                                                   \
	static struct ld2410_data ld2410_data_##inst = {                                           \
		.settings =                                                                        \
			{                                                                          \
				.motion_gate_sensitivity = DT_INST_PROP(inst, motion_sensitivity), \
				.stationary_gate_sensitivity =                                     \
					DT_INST_PROP(inst, stationary_sensitivity),                \
				.max_motion_gate = INST_MAX_MOV_GATE(inst),                        \
				.max_stationary_gate = INST_MAX_STA_GATE(inst),                    \
				.presence_timeout = INST_PRESEN_TIMEOUT(inst),                     \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static const struct ld2410_config ld2410_config_##inst = {                                 \
		.uart_dev = DEVICE_DT_GET(DT_INST_BUS(inst)),                                      \
		IF_ENABLED(CONFIG_LD2410_TRIGGER,                                                  \
			   (.int_gpios = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {}),))         \
			.engineering_mode = DT_INST_PROP(inst, engineering_mode),                  \
		.distance_resolution = DT_INST_PROP(inst, distance_resolution),                    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &ld2410_init, NULL, &ld2410_data_##inst,                       \
			      &ld2410_config_##inst, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,     \
			      &ld2410_api);

DT_INST_FOREACH_STATUS_OKAY(LD2410_DEFINE)
