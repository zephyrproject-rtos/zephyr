/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025 Siratul Islam
 */

#define DT_DRV_COMPAT adh_tech_gt5x

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/biometrics.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "biometrics_gt5x.h"

LOG_MODULE_REGISTER(gt5x, CONFIG_BIOMETRICS_LOG_LEVEL);

static inline uint16_t api_id_to_hw_id(uint16_t api_id)
{
	return api_id - 1;
}

static inline uint16_t hw_id_to_api_id(uint16_t hw_id)
{
	return hw_id + 1;
}

static int gt5x_validate_id(const struct device *dev, uint16_t api_id)
{
	const struct gt5x_config *cfg = dev->config;

	if (api_id < 1 || api_id > cfg->max_templates) {
		LOG_ERR("Invalid ID %u (valid: 1-%u)", api_id, cfg->max_templates);
		return -EINVAL;
	}

	return 0;
}

static int gt5x_nack_to_errno(const struct device *dev, uint32_t nack_param)
{
	const struct gt5x_config *cfg = dev->config;

	if (nack_param < cfg->max_templates) {
		LOG_WRN("Duplicate fingerprint at HW ID %u", nack_param);
		return -EEXIST;
	}

	switch (nack_param) {
	case GT5X_NACK_INVALID_POS:
	case GT5X_NACK_INVALID_PARAM:
		return -EINVAL;

	case GT5X_NACK_IS_NOT_USED:
	case GT5X_NACK_VERIFY_FAILED:
	case GT5X_NACK_IDENTIFY_FAILED:
	case GT5X_NACK_DB_IS_EMPTY:
		return -ENOENT;

	case GT5X_NACK_IS_ALREADY_USED:
		return -EEXIST;

	case GT5X_NACK_DB_IS_FULL:
		return -ENOSPC;

	case GT5X_NACK_BAD_FINGER:
	case GT5X_NACK_FINGER_IS_NOT_PRESSED:
		return -EAGAIN;

	case GT5X_NACK_IS_NOT_SUPPORTED:
		return -ENOSYS;

	case GT5X_NACK_TIMEOUT:
		return -ETIMEDOUT;

	case GT5X_NACK_COMM_ERR:
	case GT5X_NACK_ENROLL_FAILED:
	case GT5X_NACK_DEV_ERR:
	default:
		return -EIO;
	}
}

static void gt5x_uart_tx_handler(const struct device *uart_dev, struct gt5x_data *data)
{
	int sent;
	k_spinlock_key_t key = k_spin_lock(&data->irq_lock);

	uint16_t remaining = data->tx_pkt.len - data->tx_pkt.offset;

	if (remaining > 0) {
		sent = uart_fifo_fill(uart_dev, &data->tx_pkt.buf[data->tx_pkt.offset], remaining);
		if (sent > 0) {
			data->tx_pkt.offset += sent;
			remaining = data->tx_pkt.len - data->tx_pkt.offset;
		}
	}

	if (remaining == 0 && uart_irq_tx_complete(uart_dev)) {
		uart_irq_tx_disable(uart_dev);
		k_spin_unlock(&data->irq_lock, key);
		k_sem_give(&data->uart_tx_sem);
		return;
	}

	k_spin_unlock(&data->irq_lock, key);
}

static void gt5x_uart_rx_handler(const struct device *uart_dev, struct gt5x_data *data)
{
	uint8_t byte;
	uint16_t offset;
	uint16_t expected;
	k_spinlock_key_t key;

	while (uart_fifo_read(uart_dev, &byte, 1) > 0) {
		key = k_spin_lock(&data->irq_lock);
		offset = data->rx_pkt.len;

		if (offset >= GT5X_CMD_PACKET_SIZE) {
			data->rx_error = GT5X_RX_OVERFLOW;
			uart_irq_rx_disable(uart_dev);
			k_spin_unlock(&data->irq_lock, key);
			k_sem_give(&data->uart_rx_sem);
			return;
		}

		data->rx_pkt.buf[offset++] = byte;

		expected = data->rx_expected;
		data->rx_pkt.len = offset;

		if (offset >= expected) {
			uart_irq_rx_disable(uart_dev);
			k_spin_unlock(&data->irq_lock, key);
			k_sem_give(&data->uart_rx_sem);
			return;
		}

		k_spin_unlock(&data->irq_lock, key);
	}
}

static void gt5x_uart_callback(const struct device *uart_dev, void *user_data)
{
	struct gt5x_data *data = user_data;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (uart_irq_tx_ready(uart_dev)) {
		gt5x_uart_tx_handler(uart_dev, data);
	}

	if (uart_irq_rx_ready(uart_dev)) {
		gt5x_uart_rx_handler(uart_dev, data);
	}
}

static int gt5x_send_command(const struct device *dev, uint16_t cmd, uint32_t param)
{
	const struct gt5x_config *cfg = dev->config;
	struct gt5x_data *data = dev->data;
	uint16_t checksum = 0;
	k_spinlock_key_t key;

	/* Build command packet with Little Endian byte order */
	data->tx_pkt.buf[0] = GT5X_CMD_START_CODE1;
	data->tx_pkt.buf[1] = GT5X_CMD_START_CODE2;
	sys_put_le16(GT5X_DEVICE_ID, &data->tx_pkt.buf[2]);
	sys_put_le32(param, &data->tx_pkt.buf[4]);
	sys_put_le16(cmd, &data->tx_pkt.buf[8]);

	for (int i = 0; i < GT5X_CMD_CHECKSUM_OFFSET; i++) {
		checksum += data->tx_pkt.buf[i];
	}
	sys_put_le16(checksum, &data->tx_pkt.buf[10]);

	key = k_spin_lock(&data->irq_lock);
	data->tx_pkt.len = GT5X_CMD_PACKET_SIZE;
	data->tx_pkt.offset = 0;
	k_spin_unlock(&data->irq_lock, key);

	LOG_HEXDUMP_DBG(data->tx_pkt.buf, GT5X_CMD_PACKET_SIZE, "CMD TX");

	uart_irq_tx_enable(cfg->uart_dev);

	if (k_sem_take(&data->uart_tx_sem, K_MSEC(GT5X_UART_TIMEOUT_MS)) != 0) {
		uart_irq_tx_disable(cfg->uart_dev);
		LOG_ERR("UART TX timeout");
		return -ETIMEDOUT;
	}

	return 0;
}

/* Receive response packet (12 bytes fixed) */
static int gt5x_recv_response(const struct device *dev, uint32_t *param_out)
{
	const struct gt5x_config *cfg = dev->config;
	struct gt5x_data *data = dev->data;
	uint16_t recv_checksum, calc_checksum = 0;
	uint16_t response;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->irq_lock);
	data->rx_pkt.len = 0;
	data->rx_expected = GT5X_RESP_PACKET_SIZE;
	data->rx_error = GT5X_RX_OK;
	k_spin_unlock(&data->irq_lock, key);

	uart_irq_rx_enable(cfg->uart_dev);

	if (k_sem_take(&data->uart_rx_sem, K_MSEC(GT5X_UART_TIMEOUT_MS)) != 0) {
		uart_irq_rx_disable(cfg->uart_dev);
		LOG_ERR("UART RX timeout");
		return -ETIMEDOUT;
	}

	if (data->rx_error == GT5X_RX_OVERFLOW) {
		LOG_ERR("RX buffer overflow");
		return -EOVERFLOW;
	}

	if (data->rx_error == GT5X_RX_INVALID) {
		LOG_ERR("Invalid RX state");
		return -EBADMSG;
	}

	LOG_HEXDUMP_DBG(data->rx_pkt.buf, data->rx_pkt.len, "RESP RX");

	/* Validate start codes */
	if (data->rx_pkt.buf[0] != GT5X_CMD_START_CODE1 ||
	    data->rx_pkt.buf[1] != GT5X_CMD_START_CODE2) {
		LOG_ERR("Invalid start codes");
		return -EBADMSG;
	}

	/* Validate device ID */
	if (sys_get_le16(&data->rx_pkt.buf[2]) != GT5X_DEVICE_ID) {
		LOG_ERR("Device ID mismatch");
		return -EBADMSG;
	}

	/* Calculate and verify checksum (sum of bytes 0-9) */
	for (int i = 0; i < 10; i++) {
		calc_checksum += data->rx_pkt.buf[i];
	}
	recv_checksum = sys_get_le16(&data->rx_pkt.buf[10]);

	if (recv_checksum != calc_checksum) {
		LOG_ERR("Checksum mismatch: recv=0x%04x calc=0x%04x", recv_checksum, calc_checksum);
		return -EBADMSG;
	}

	response = sys_get_le16(&data->rx_pkt.buf[8]);
	*param_out = sys_get_le32(&data->rx_pkt.buf[4]);

	if (response == GT5X_NACK) {
		LOG_DBG("NACK received, error=0x%08x", *param_out);
		return gt5x_nack_to_errno(dev, *param_out);
	}

	if (response != GT5X_ACK) {
		LOG_ERR("Invalid response code: 0x%04x", response);
		return -EBADMSG;
	}

	return 0;
}

static int gt5x_transceive(const struct device *dev, uint16_t cmd, uint32_t param,
			   uint32_t *resp_param)
{
	struct gt5x_data *data = dev->data;
	uint32_t response_param;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = gt5x_send_command(dev, cmd, param);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = gt5x_recv_response(dev, &response_param);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	if (resp_param) {
		*resp_param = response_param;
	}

	k_mutex_unlock(&data->lock);
	return 0;
}

static int gt5x_send_data_packet(const struct device *dev, const uint8_t *data, size_t len)
{
	const struct gt5x_config *cfg = dev->config;
	struct gt5x_data *drv_data = dev->data;
	size_t packet_size = GT5X_DATA_HDR_SIZE + len + GT5X_CHECKSUM_SIZE;
	uint16_t checksum = 0;
	uint8_t *packet;
	k_spinlock_key_t key;
	int ret = 0;

	packet = k_malloc(packet_size);
	if (!packet) {
		return -ENOMEM;
	}

	packet[0] = GT5X_DATA_START_CODE1;
	packet[1] = GT5X_DATA_START_CODE2;
	sys_put_le16(GT5X_DEVICE_ID, &packet[2]);
	memcpy(&packet[4], data, len);

	for (size_t i = 0; i < packet_size - GT5X_CHECKSUM_SIZE; i++) {
		checksum += packet[i];
	}
	sys_put_le16(checksum, &packet[packet_size - GT5X_CHECKSUM_SIZE]);

	LOG_HEXDUMP_DBG(packet, MIN(packet_size, 64), "DATA TX (partial)");

	size_t offset = 0;

	while (offset < packet_size) {
		size_t chunk_size = MIN(packet_size - offset, GT5X_CMD_PACKET_SIZE);

		key = k_spin_lock(&drv_data->irq_lock);
		memcpy(drv_data->tx_pkt.buf, &packet[offset], chunk_size);
		drv_data->tx_pkt.len = chunk_size;
		drv_data->tx_pkt.offset = 0;
		k_spin_unlock(&drv_data->irq_lock, key);

		uart_irq_tx_enable(cfg->uart_dev);

		if (k_sem_take(&drv_data->uart_tx_sem, K_MSEC(GT5X_UART_TIMEOUT_MS)) != 0) {
			uart_irq_tx_disable(cfg->uart_dev);
			LOG_ERR("Data packet TX timeout at offset %zu", offset);
			ret = -ETIMEDOUT;
			goto cleanup;
		}

		offset += chunk_size;
	}

cleanup:
	k_free(packet);
	return ret;
}

static int gt5x_recv_data_packet(const struct device *dev, uint8_t *data, size_t expected_len)
{
	const struct gt5x_config *cfg = dev->config;
	struct gt5x_data *drv_data = dev->data;
	size_t packet_size = GT5X_DATA_HDR_SIZE + expected_len + GT5X_CHECKSUM_SIZE;
	uint16_t recv_checksum, calc_checksum = 0;
	uint8_t *packet;
	size_t offset = 0;
	int ret = 0;
	k_spinlock_key_t key;

	packet = k_malloc(packet_size);
	if (!packet) {
		return -ENOMEM;
	}

	while (offset < packet_size) {
		size_t chunk_size = MIN(packet_size - offset, GT5X_CMD_PACKET_SIZE);

		key = k_spin_lock(&drv_data->irq_lock);
		drv_data->rx_pkt.len = 0;
		drv_data->rx_expected = chunk_size;
		drv_data->rx_error = GT5X_RX_OK;
		k_spin_unlock(&drv_data->irq_lock, key);

		uart_irq_rx_enable(cfg->uart_dev);

		if (k_sem_take(&drv_data->uart_rx_sem, K_MSEC(GT5X_UART_TIMEOUT_MS)) != 0) {
			uart_irq_rx_disable(cfg->uart_dev);
			LOG_ERR("Data packet RX timeout at offset %zu", offset);
			ret = -ETIMEDOUT;
			goto cleanup;
		}

		if (drv_data->rx_error != GT5X_RX_OK) {
			LOG_ERR("RX error during data packet reception");
			ret = -EIO;
			goto cleanup;
		}

		memcpy(&packet[offset], drv_data->rx_pkt.buf, chunk_size);
		offset += chunk_size;
	}

	LOG_HEXDUMP_DBG(packet, MIN(packet_size, 64), "DATA RX (partial)");

	/* Validate start codes */
	if (packet[0] != GT5X_DATA_START_CODE1 || packet[1] != GT5X_DATA_START_CODE2) {
		LOG_ERR("Invalid data packet start codes");
		ret = -EBADMSG;
		goto cleanup;
	}

	/* Validate device ID */
	if (sys_get_le16(&packet[2]) != GT5X_DEVICE_ID) {
		LOG_ERR("Data packet device ID mismatch");
		ret = -EBADMSG;
		goto cleanup;
	}

	/* Calculate and verify checksum */
	for (size_t i = 0; i < packet_size - GT5X_CHECKSUM_SIZE; i++) {
		calc_checksum += packet[i];
	}
	recv_checksum = sys_get_le16(&packet[packet_size - GT5X_CHECKSUM_SIZE]);

	if (recv_checksum != calc_checksum) {
		LOG_ERR("Data packet checksum mismatch");
		ret = -EBADMSG;
		goto cleanup;
	}

	/* Extract data payload */
	memcpy(data, &packet[4], expected_len);

cleanup:
	k_free(packet);
	return ret;
}

static int gt5x_led_control_internal(const struct device *dev, bool on)
{
	struct gt5x_data *data = dev->data;
	int ret;

	if (data->led_on == on) {
		return 0;
	}

	ret = gt5x_transceive(dev, GT5X_CMD_CMOS_LED, on ? 1 : 0, NULL);
	if (ret == 0) {
		data->led_on = on;
		LOG_DBG("LED %s", on ? "ON" : "OFF");
	}

	return ret;
}

static int gt5x_capture_finger_internal(const struct device *dev, bool best_quality,
					k_timeout_t timeout)
{
	uint32_t timeout_ms;
	int ret;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		timeout_ms = GT5X_MAX_TIMEOUT_MS;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		timeout_ms = 0;
	} else {
		int64_t ms = k_ticks_to_ms_ceil64(timeout.ticks);

		if (ms > GT5X_MAX_TIMEOUT_MS) {
			return -EINVAL;
		}
		timeout_ms = (uint32_t)ms;
	}

	ret = gt5x_led_control_internal(dev, true);
	if (ret < 0) {
		LOG_ERR("Failed to enable LED before capture");
		return ret;
	}

	int64_t start = k_uptime_get();
	uint32_t finger_status;

	/* Poll until finger is detected on sensor */
	do {
		ret = gt5x_transceive(dev, GT5X_CMD_IS_PRESS_FINGER, 0, &finger_status);
		if (ret < 0) {
			return ret;
		}

		if (finger_status == 0) {
			/* Finger detected, proceed with capture */
			break;
		}

		if ((k_uptime_get() - start) >= timeout_ms) {
			LOG_DBG("Timeout waiting for finger");
			return -ETIMEDOUT;
		}

		k_msleep(GT5X_FINGER_POLL_MS);
	} while (true);

	/* Now capture the finger image */
	ret = gt5x_transceive(dev, GT5X_CMD_CAPTURE_FINGER, best_quality ? 1 : 0, NULL);
	if (ret < 0) {
		return ret;
	}

	LOG_DBG("Finger captured (quality: %s)", best_quality ? "best" : "fast");
	return 0;
}

static int gt5x_wait_finger_removal(const struct device *dev, k_timeout_t timeout)
{
	uint32_t finger_status;
	int64_t start = k_uptime_get();
	uint32_t timeout_ms;
	int ret;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		timeout_ms = GT5X_MAX_TIMEOUT_MS;
	} else {
		int64_t ms = k_ticks_to_ms_ceil64(timeout.ticks);

		timeout_ms = (uint32_t)MIN(ms, GT5X_MAX_TIMEOUT_MS);
	}

	do {
		ret = gt5x_transceive(dev, GT5X_CMD_IS_PRESS_FINGER, 0, &finger_status);
		if (ret < 0) {
			return ret;
		}

		if (finger_status != 0) {
			LOG_DBG("Finger removed");
			return 0;
		}

		if ((k_uptime_get() - start) >= timeout_ms) {
			LOG_WRN("Timeout waiting for finger removal");
			return -ETIMEDOUT;
		}

		k_msleep(GT5X_FINGER_POLL_MS);
	} while (true);
}

/* Biometric API implementations */

static int gt5x_get_capabilities(const struct device *dev, struct biometric_capabilities *caps)
{
	const struct gt5x_config *cfg = dev->config;

	caps->type = BIOMETRIC_TYPE_FINGERPRINT;
	caps->max_templates = cfg->max_templates;
	caps->template_size = cfg->template_size;
	caps->storage_modes = BIOMETRIC_STORAGE_DEVICE;
	caps->enrollment_samples_required = 3;

	return 0;
}

static int gt5x_attr_set(const struct device *dev, enum biometric_attribute attr, int32_t val)
{
	struct gt5x_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (attr) {
	case BIOMETRIC_ATTR_MATCH_THRESHOLD:
		data->match_threshold = val;
		ret = 0;
		break;

	case BIOMETRIC_ATTR_ENROLLMENT_QUALITY:
		data->enroll_quality = val;
		ret = 0;
		break;

	case BIOMETRIC_ATTR_SECURITY_LEVEL:
		if (val < 1 || val > 10) {
			ret = -EINVAL;
			break;
		}
		data->security_level = val;
		ret = 0;
		break;

	case BIOMETRIC_ATTR_TIMEOUT_MS:
		if (val > GT5X_MAX_TIMEOUT_MS) {
			ret = -EINVAL;
		} else {
			data->timeout_ms = val;
			ret = 0;
		}
		break;

	case BIOMETRIC_ATTR_IMAGE_QUALITY:
		ret = -EACCES;
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int gt5x_attr_get(const struct device *dev, enum biometric_attribute attr, int32_t *val)
{
	struct gt5x_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	switch (attr) {
	case BIOMETRIC_ATTR_MATCH_THRESHOLD:
		*val = data->match_threshold;
		break;
	case BIOMETRIC_ATTR_ENROLLMENT_QUALITY:
		*val = data->enroll_quality;
		break;
	case BIOMETRIC_ATTR_SECURITY_LEVEL:
		*val = data->security_level;
		break;
	case BIOMETRIC_ATTR_TIMEOUT_MS:
		*val = data->timeout_ms;
		break;
	case BIOMETRIC_ATTR_IMAGE_QUALITY:
		ret = -ENOTSUP;
		break;
	case BIOMETRIC_ATTR_PRIV_START:
		*val = data->last_match_id;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int gt5x_enroll_start(const struct device *dev, uint16_t template_id)
{
	struct gt5x_data *data = dev->data;
	uint16_t hw_id;
	int ret;

	ret = gt5x_validate_id(dev, template_id);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->enroll_state != GT5X_ENROLL_IDLE) {
		k_mutex_unlock(&data->lock);
		return -EBUSY;
	}

	hw_id = api_id_to_hw_id(template_id);

	ret = gt5x_transceive(dev, GT5X_CMD_ENROLL_START, hw_id, NULL);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	data->enroll_state = GT5X_ENROLL_WAIT_SAMPLE_1;
	data->enroll_id = template_id;

	k_mutex_unlock(&data->lock);

	LOG_INF("Enrollment started for ID %u", template_id);
	return 0;
}

static int gt5x_enroll_capture(const struct device *dev, k_timeout_t timeout,
			       struct biometric_capture_result *result)
{
	struct gt5x_data *data = dev->data;
	enum gt5x_enroll_state current_state;
	uint16_t cmd;
	uint8_t pass;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	current_state = data->enroll_state;

	if (current_state == GT5X_ENROLL_IDLE) {
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	if (current_state == GT5X_ENROLL_READY) {
		k_mutex_unlock(&data->lock);
		return -EALREADY;
	}

	/* Determine enrollment stage */
	switch (current_state) {
	case GT5X_ENROLL_WAIT_SAMPLE_1:
		pass = 1;
		cmd = GT5X_CMD_ENROLL_1;
		break;
	case GT5X_ENROLL_WAIT_SAMPLE_2:
		pass = 2;
		cmd = GT5X_CMD_ENROLL_2;
		break;
	case GT5X_ENROLL_WAIT_SAMPLE_3:
		pass = 3;
		cmd = GT5X_CMD_ENROLL_3;
		break;
	default:
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	LOG_INF("Enrollment capture %u/3 for ID %u", pass, data->enroll_id);

	ret = gt5x_capture_finger_internal(dev, true, timeout);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	ret = gt5x_transceive(dev, cmd, 0, NULL);
	if (ret < 0) {
		LOG_ERR("Enroll%u failed: %d", pass, ret);
		k_mutex_unlock(&data->lock);
		return ret;
	}

	if (pass < 3) {
		data->enroll_state++;
		LOG_INF("Sample %u captured, waiting for finger removal", pass);
	} else {
		data->enroll_state = GT5X_ENROLL_READY;
		LOG_INF("All 3 samples captured, ready to finalize");
	}

	if (result) {
		result->samples_captured = pass;
		result->samples_required = 3;
		result->quality = 0;
	}

	k_mutex_unlock(&data->lock);

	if (pass < 3) {
		ret = gt5x_wait_finger_removal(dev, K_SECONDS(5));
		if (ret < 0) {
			LOG_WRN("Finger removal check failed: %d", ret);
		}
	}

	return 0;
}

static int gt5x_enroll_finalize(const struct device *dev)
{
	struct gt5x_data *data = dev->data;
	uint16_t enrolled_id;
	uint32_t resp_param;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->enroll_state != GT5X_ENROLL_READY) {
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	enrolled_id = data->enroll_id;
	data->enroll_state = GT5X_ENROLL_IDLE;

	ret = gt5x_transceive(dev, GT5X_CMD_GET_ENROLL_COUNT, 0, &resp_param);
	if (ret == 0) {
		data->enrolled_count = (uint16_t)resp_param;
	}

	k_mutex_unlock(&data->lock);

	LOG_INF("Enrollment completed for ID %u", enrolled_id);
	return 0;
}

static int gt5x_enroll_abort(const struct device *dev)
{
	struct gt5x_data *data = dev->data;
	bool was_idle;

	k_mutex_lock(&data->lock, K_FOREVER);
	was_idle = (data->enroll_state == GT5X_ENROLL_IDLE);
	data->enroll_state = GT5X_ENROLL_IDLE;
	k_mutex_unlock(&data->lock);

	if (was_idle) {
		return -EALREADY;
	}

	LOG_INF("Enrollment aborted");
	return 0;
}

static int gt5x_template_store(const struct device *dev, uint16_t id, const uint8_t *data,
			       size_t size)
{
	const struct gt5x_config *cfg = dev->config;
	struct gt5x_data *drv_data = dev->data;
	uint16_t hw_id;
	uint32_t resp_param;
	int ret;

	/*
	 * LIMITATION: GT5X SET_TEMPLATE command uploads template data but does not
	 * mark it as "enrolled" in the device's internal database. Subsequent VERIFY
	 * or IDENTIFY commands will return NACK_DB_IS_EMPTY (0x100A).
	 */

	ret = gt5x_validate_id(dev, id);
	if (ret < 0) {
		return ret;
	}

	if (size != cfg->template_size) {
		LOG_ERR("Template size mismatch: %zu != %u", size, cfg->template_size);
		return -EINVAL;
	}

	hw_id = api_id_to_hw_id(id);

	k_mutex_lock(&drv_data->lock, K_FOREVER);

	ret = gt5x_transceive(dev, GT5X_CMD_SET_TEMPLATE, hw_id, &resp_param);
	if (ret < 0) {
		k_mutex_unlock(&drv_data->lock);
		return ret;
	}

	ret = gt5x_send_data_packet(dev, data, size);
	if (ret < 0) {
		k_mutex_unlock(&drv_data->lock);
		return ret;
	}

	ret = gt5x_recv_response(dev, &resp_param);
	if (ret < 0) {
		k_mutex_unlock(&drv_data->lock);
		return ret;
	}

	ret = gt5x_transceive(dev, GT5X_CMD_GET_ENROLL_COUNT, 0, &resp_param);
	if (ret == 0) {
		drv_data->enrolled_count = (uint16_t)resp_param;
	}

	k_mutex_unlock(&drv_data->lock);

	LOG_INF("Template stored at ID %u", id);
	return 0;
}

static int gt5x_template_read(const struct device *dev, uint16_t id, uint8_t *data, size_t size)
{
	const struct gt5x_config *cfg = dev->config;
	struct gt5x_data *drv_data = dev->data;
	uint16_t hw_id;
	int ret;

	ret = gt5x_validate_id(dev, id);
	if (ret < 0) {
		return ret;
	}

	if (size < cfg->template_size) {
		LOG_ERR("Buffer too small: %zu < %u", size, cfg->template_size);
		return -EINVAL;
	}

	hw_id = api_id_to_hw_id(id);

	k_mutex_lock(&drv_data->lock, K_FOREVER);

	ret = gt5x_transceive(dev, GT5X_CMD_GET_TEMPLATE, hw_id, NULL);
	if (ret < 0) {
		k_mutex_unlock(&drv_data->lock);
		return ret;
	}

	ret = gt5x_recv_data_packet(dev, data, cfg->template_size);
	if (ret < 0) {
		k_mutex_unlock(&drv_data->lock);
		return ret;
	}

	k_mutex_unlock(&drv_data->lock);

	LOG_INF("Template read from ID %u", id);
	return cfg->template_size;
}

static int gt5x_template_delete(const struct device *dev, uint16_t id)
{
	struct gt5x_data *data = dev->data;
	uint16_t hw_id;
	uint32_t resp_param;
	int ret;

	ret = gt5x_validate_id(dev, id);
	if (ret < 0) {
		return ret;
	}

	hw_id = api_id_to_hw_id(id);

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = gt5x_transceive(dev, GT5X_CMD_DELETE_ID, hw_id, NULL);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	/* Update enrolled count */
	ret = gt5x_transceive(dev, GT5X_CMD_GET_ENROLL_COUNT, 0, &resp_param);
	if (ret == 0) {
		data->enrolled_count = (uint16_t)resp_param;
	}

	k_mutex_unlock(&data->lock);

	LOG_INF("Template deleted at ID %u", id);
	return 0;
}

static int gt5x_template_delete_all(const struct device *dev)
{
	struct gt5x_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = gt5x_transceive(dev, GT5X_CMD_DELETE_ALL, 0, NULL);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	data->enrolled_count = 0;

	k_mutex_unlock(&data->lock);

	LOG_INF("All templates deleted");
	return 0;
}

static int gt5x_template_list(const struct device *dev, uint16_t *ids, size_t max_count,
			      size_t *actual_count)
{
	const struct gt5x_config *cfg = dev->config;
	struct gt5x_data *data = dev->data;
	size_t count = 0;
	uint32_t resp_param;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Iterate through all possible IDs and check enrollment status */
	for (uint16_t hw_id = 0; hw_id < cfg->max_templates && count < max_count; hw_id++) {
		ret = gt5x_transceive(dev, GT5X_CMD_CHECK_ENROLLED, hw_id, &resp_param);
		if (ret == 0) {
			ids[count++] = hw_id_to_api_id(hw_id);
		} else if (ret != -ENOENT) {
			LOG_WRN("Check enrolled failed for HW ID %u: %d", hw_id, ret);
		}

		k_yield();
	}

	*actual_count = count;

	k_mutex_unlock(&data->lock);

	return 0;
}

static int gt5x_match(const struct device *dev, enum biometric_match_mode mode,
		      uint16_t template_id, k_timeout_t timeout,
		      struct biometric_match_result *result)
{
	struct gt5x_data *data = dev->data;
	uint32_t resp_param;
	uint16_t hw_id;
	int ret;

	if (mode == BIOMETRIC_MATCH_VERIFY) {
		ret = gt5x_validate_id(dev, template_id);
		if (ret < 0) {
			return ret;
		}
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = gt5x_capture_finger_internal(dev, false, timeout);
	if (ret < 0) {
		k_mutex_unlock(&data->lock);
		return ret;
	}

	if (mode == BIOMETRIC_MATCH_VERIFY) {
		hw_id = api_id_to_hw_id(template_id);
		ret = gt5x_transceive(dev, GT5X_CMD_VERIFY, hw_id, &resp_param);

		if (ret == 0) {
			if (result) {
				result->confidence = 0;
				result->template_id = template_id;
				result->image_quality = 0;
			}
			LOG_INF("Verification successful for ID %u", template_id);
		} else {
			LOG_DBG("Verification failed: %d", ret);
		}
	} else {
		ret = gt5x_transceive(dev, GT5X_CMD_IDENTIFY, 0, &resp_param);

		if (ret == 0) {
			uint16_t matched_hw_id = (uint16_t)resp_param;
			uint16_t matched_id = hw_id_to_api_id(matched_hw_id);

			data->last_match_id = matched_id;

			if (result) {
				result->confidence = 0;
				result->template_id = matched_id;
				result->image_quality = 0;
			}
			LOG_INF("Identification successful, matched ID %u", matched_id);
		} else {
			LOG_DBG("Identification failed: %d", ret);
		}
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int gt5x_led_control(const struct device *dev, enum biometric_led_state state)
{
	struct gt5x_data *data = dev->data;
	bool led_on;
	int ret;

	/* GT5X only supports simple ON/OFF, no blink or breathe modes */
	switch (state) {
	case BIOMETRIC_LED_OFF:
		led_on = false;
		break;
	case BIOMETRIC_LED_ON:
		led_on = true;
		break;
	case BIOMETRIC_LED_BLINK:
	case BIOMETRIC_LED_BREATHE:
		LOG_WRN("LED mode not supported by GT5X, using ON instead");
		led_on = true;
		break;
	default:
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = gt5x_led_control_internal(dev, led_on);
	k_mutex_unlock(&data->lock);

	return ret;
}

static DEVICE_API(biometric, biometrics_gt5x_api) = {
	.get_capabilities = gt5x_get_capabilities,
	.attr_set = gt5x_attr_set,
	.attr_get = gt5x_attr_get,
	.enroll_start = gt5x_enroll_start,
	.enroll_capture = gt5x_enroll_capture,
	.enroll_finalize = gt5x_enroll_finalize,
	.enroll_abort = gt5x_enroll_abort,
	.template_store = gt5x_template_store,
	.template_read = gt5x_template_read,
	.template_delete = gt5x_template_delete,
	.template_delete_all = gt5x_template_delete_all,
	.template_list = gt5x_template_list,
	.match = gt5x_match,
	.led_control = gt5x_led_control,
};

static int gt5x_init(const struct device *dev)
{
	const struct gt5x_config *cfg = dev->config;
	struct gt5x_data *data = dev->data;
	uint32_t resp_param;
	int ret;
	bool sn_valid;

	if (!device_is_ready(cfg->uart_dev)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	data->dev = dev;
	data->enroll_state = GT5X_ENROLL_IDLE;
	data->timeout_ms = CONFIG_GT5X_TIMEOUT_MS;
	data->security_level = 5;
	data->match_threshold = 100;
	data->enroll_quality = 100;
	data->led_on = false;
	data->rx_error = GT5X_RX_OK;
	data->last_match_id = 0;
	data->enrolled_count = 0;

	k_mutex_init(&data->lock);
	k_sem_init(&data->uart_tx_sem, 0, 1);
	k_sem_init(&data->uart_rx_sem, 0, 1);

	/* Allocate template buffer based on model */
	data->template_buf = k_malloc(cfg->template_size);
	if (!data->template_buf) {
		LOG_ERR("Failed to allocate template buffer (%u bytes)", cfg->template_size);
		return -ENOMEM;
	}

	uart_irq_callback_user_data_set(cfg->uart_dev, gt5x_uart_callback, data);
	uart_irq_rx_disable(cfg->uart_dev);
	uart_irq_tx_disable(cfg->uart_dev);

	LOG_INF("Initializing GT5X (max:%u, template:%u bytes)", cfg->max_templates,
		cfg->template_size);

	/* Try Open(1) to get device info */
	ret = gt5x_transceive(dev, GT5X_CMD_OPEN, 1, &resp_param);
	if (ret < 0) {
		LOG_WRN("Initial Open failed (%d), attempting recovery", ret);

		/* Recovery: Close -> wait -> Open */
		gt5x_transceive(dev, GT5X_CMD_CLOSE, 0, NULL);
		k_msleep(100);

		ret = gt5x_transceive(dev, GT5X_CMD_OPEN, 1, &resp_param);
		if (ret < 0) {
			LOG_ERR("Open failed after recovery: %d", ret);
			k_free(data->template_buf);
			return ret;
		}

		LOG_INF("Device recovered successfully");
	}

	/* Receive DeviceInfo data packet (24 bytes) */
	ret = gt5x_recv_data_packet(dev, (uint8_t *)&data->devinfo,
				    sizeof(struct gt5x_device_info));
	if (ret < 0) {
		LOG_ERR("Failed to receive DeviceInfo: %d", ret);
		k_free(data->template_buf);
		return ret;
	}

	/* Check if serial number is all zeros (possible counterfeit) */
	sn_valid = false;
	for (int i = 0; i < 16; i++) {
		if (data->devinfo.serial_number[i] != 0) {
			sn_valid = true;
			break;
		}
	}

	if (!sn_valid) {
		LOG_WRN("Device serial number is all zeros");
		LOG_WRN("Device may be counterfeit or defective");
	}

	LOG_INF("Firmware version: 0x%08x", sys_le32_to_cpu(data->devinfo.firmware_version));
	LOG_HEXDUMP_INF(data->devinfo.serial_number, 16, "Serial Number");

	/* Get enrolled count */
	ret = gt5x_transceive(dev, GT5X_CMD_GET_ENROLL_COUNT, 0, &resp_param);
	if (ret == 0) {
		data->enrolled_count = (uint16_t)resp_param;
		LOG_INF("Enrolled templates: %u/%u", data->enrolled_count, cfg->max_templates);
	}

	/* Turn on LED for user feedback */
	ret = gt5x_led_control_internal(dev, true);
	if (ret < 0) {
		LOG_WRN("Failed to enable LED: %d", ret);
	}

	LOG_INF("GT5X initialization complete");
	return 0;
}

#define GT5X_DEFINE(inst)                                                                          \
	static struct gt5x_data gt5x_data_##inst;                                                  \
                                                                                                   \
	static const struct gt5x_config gt5x_config_##inst = {                                     \
		.uart_dev = DEVICE_DT_GET(DT_INST_BUS(inst)),                                      \
		.max_templates = DT_INST_PROP(inst, max_templates),                                \
		.template_size = DT_INST_PROP(inst, template_size),                                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, gt5x_init, NULL, &gt5x_data_##inst, &gt5x_config_##inst,       \
			      POST_KERNEL, CONFIG_BIOMETRICS_INIT_PRIORITY, &biometrics_gt5x_api);

DT_INST_FOREACH_STATUS_OKAY(GT5X_DEFINE)
