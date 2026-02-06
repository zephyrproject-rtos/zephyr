/*
 * SPDX-License-Identifier: Apache-2.0
 * Copyright (c) 2025 Siratul Islam
 */

#define DT_DRV_COMPAT zhiantec_zfm_x0

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/biometrics.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include "biometrics_zfm_x0.h"

LOG_MODULE_REGISTER(zfm_x0, CONFIG_BIOMETRICS_LOG_LEVEL);

/* Convert ZFM-x0 confirmation codes to errno */
static inline int zfm_x0_err_to_errno(uint8_t code)
{
	switch (code) {
	case ZFM_X0_OK:
		return 0;
	case ZFM_X0_ERR_NO_FINGER:
		return -EAGAIN;
	case ZFM_X0_ERR_NO_MATCH:
	case ZFM_X0_ERR_NOT_FOUND:
		return -ENOENT;
	case ZFM_X0_ERR_BAD_LOCATION:
		return -EINVAL;
	case ZFM_X0_ERR_BAD_IMAGE:
	case ZFM_X0_ERR_TOO_FEW:
	case ZFM_X0_ERR_INVALID_IMAGE:
	case ZFM_X0_ERR_ENROLL_FAIL:
	case ZFM_X0_ERR_MERGE_FAIL:
	case ZFM_X0_ERR_FLASH_ERR:
	default:
		return -EIO;
	}
}

/* UART IRQ handlers */
static void zfm_x0_uart_tx_handler(const struct device *uart_dev, struct zfm_x0_data *data)
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

static void zfm_x0_uart_rx_handler(const struct device *uart_dev, struct zfm_x0_data *data)
{
	uint8_t byte;
	uint16_t offset;
	uint16_t expected;
	k_spinlock_key_t key;

	while (uart_fifo_read(uart_dev, &byte, 1) > 0) {
		key = k_spin_lock(&data->irq_lock);
		offset = data->rx_pkt.len;

		if (offset >= ZFM_X0_MAX_PACKET_SIZE) {
			data->rx_error = ZFM_X0_RX_OVERFLOW;
			uart_irq_rx_disable(uart_dev);
			k_spin_unlock(&data->irq_lock, key);
			k_sem_give(&data->uart_rx_sem);
			return;
		}

		data->rx_pkt.buf[offset++] = byte;

		if (offset == ZFM_X0_HDR_SIZE) {
			uint16_t payload_len = sys_get_be16(&data->rx_pkt.buf[7]);

			if (payload_len > ZFM_X0_MAX_DATA_LEN + ZFM_X0_CHECKSUM_SIZE) {
				data->rx_error = ZFM_X0_RX_INVALID_LEN;
				uart_irq_rx_disable(uart_dev);
				k_spin_unlock(&data->irq_lock, key);
				k_sem_give(&data->uart_rx_sem);
				return;
			}

			data->rx_expected = payload_len + ZFM_X0_HDR_SIZE;
		}

		expected = data->rx_expected;
		data->rx_pkt.len = offset;

		if (offset >= ZFM_X0_HDR_SIZE && offset >= expected) {
			uart_irq_rx_disable(uart_dev);
			k_spin_unlock(&data->irq_lock, key);
			k_sem_give(&data->uart_rx_sem);
			return;
		}

		k_spin_unlock(&data->irq_lock, key);
	}
}

static void zfm_x0_uart_callback(const struct device *uart_dev, void *user_data)
{
	struct zfm_x0_data *data = user_data;

	if (!uart_irq_update(uart_dev)) {
		return;
	}

	if (uart_irq_tx_ready(uart_dev)) {
		zfm_x0_uart_tx_handler(uart_dev, data);
	}

	if (uart_irq_rx_ready(uart_dev)) {
		zfm_x0_uart_rx_handler(uart_dev, data);
	}
}

/* Packet transmission/reception */
static int zfm_x0_send_packet(const struct device *dev, uint8_t pid, const uint8_t *payload,
			      uint16_t payload_len)
{
	const struct zfm_x0_config *cfg = dev->config;
	struct zfm_x0_data *data = dev->data;
	uint16_t checksum = pid + payload_len + ZFM_X0_CHECKSUM_SIZE;
	uint16_t total_len = ZFM_X0_HDR_SIZE + payload_len + ZFM_X0_CHECKSUM_SIZE;
	k_spinlock_key_t key;

	if (payload_len > ZFM_X0_MAX_DATA_LEN) {
		return -ENOMEM;
	}

	if (total_len > ZFM_X0_MAX_PACKET_SIZE) {
		return -ENOMEM;
	}

	sys_put_be16(ZFM_X0_START_CODE, &data->tx_pkt.buf[0]);
	sys_put_be32(cfg->comm_addr, &data->tx_pkt.buf[2]);
	data->tx_pkt.buf[6] = pid;
	sys_put_be16(payload_len + ZFM_X0_CHECKSUM_SIZE, &data->tx_pkt.buf[7]);

	if (payload_len > 0) {
		memcpy(&data->tx_pkt.buf[ZFM_X0_HDR_SIZE], payload, payload_len);
		for (uint16_t i = 0; i < payload_len; i++) {
			checksum += payload[i];
		}
	}

	sys_put_be16(checksum, &data->tx_pkt.buf[ZFM_X0_HDR_SIZE + payload_len]);

	key = k_spin_lock(&data->irq_lock);
	data->tx_pkt.len = total_len;
	data->tx_pkt.offset = 0;
	k_spin_unlock(&data->irq_lock, key);

	LOG_HEXDUMP_DBG(data->tx_pkt.buf, total_len, "TX");

	uart_irq_tx_enable(cfg->uart_dev);

	if (k_sem_take(&data->uart_tx_sem, K_MSEC(ZFM_X0_UART_TIMEOUT_MS)) != 0) {
		uart_irq_tx_disable(cfg->uart_dev);
		LOG_ERR("UART TX timeout");
		return -ETIMEDOUT;
	}

	return 0;
}

static int zfm_x0_recv_packet(const struct device *dev)
{
	struct zfm_x0_data *data = dev->data;
	const struct zfm_x0_config *cfg = dev->config;
	uint16_t recv_checksum, calc_checksum;
	uint16_t data_len;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->irq_lock);
	data->rx_pkt.len = 0;
	data->rx_expected = ZFM_X0_HDR_SIZE;
	data->rx_error = ZFM_X0_RX_OK;
	k_spin_unlock(&data->irq_lock, key);

	uart_irq_rx_enable(cfg->uart_dev);

	if (k_sem_take(&data->uart_rx_sem, K_MSEC(ZFM_X0_UART_TIMEOUT_MS)) != 0) {
		uart_irq_rx_disable(cfg->uart_dev);
		LOG_ERR("UART RX timeout");
		return -ETIMEDOUT;
	}

	/* Check for RX errors detected in IRQ handler */
	if (data->rx_error == ZFM_X0_RX_OVERFLOW) {
		LOG_ERR("RX buffer overflow");
		return -EOVERFLOW;
	}

	if (data->rx_error == ZFM_X0_RX_INVALID_LEN) {
		LOG_ERR("Invalid packet length");
		return -EBADMSG;
	}

	LOG_HEXDUMP_DBG(data->rx_pkt.buf, data->rx_pkt.len, "RX");

	if (sys_get_be16(&data->rx_pkt.buf[0]) != ZFM_X0_START_CODE) {
		LOG_ERR("Invalid start code");
		return -EBADMSG;
	}

	if (sys_get_be32(&data->rx_pkt.buf[2]) != cfg->comm_addr) {
		LOG_ERR("Address mismatch");
		return -EBADMSG;
	}

	data_len = sys_get_be16(&data->rx_pkt.buf[7]);
	if (data_len < ZFM_X0_CHECKSUM_SIZE) {
		return -EBADMSG;
	}

	recv_checksum =
		sys_get_be16(&data->rx_pkt.buf[ZFM_X0_HDR_SIZE + data_len - ZFM_X0_CHECKSUM_SIZE]);
	calc_checksum = data->rx_pkt.buf[6] + (data_len >> 8) + (data_len & 0xFF);

	for (uint16_t i = 0; i < data_len - ZFM_X0_CHECKSUM_SIZE; i++) {
		calc_checksum += data->rx_pkt.buf[ZFM_X0_HDR_SIZE + i];
	}

	if (recv_checksum != calc_checksum) {
		LOG_ERR("Checksum mismatch: recv=%04x calc=%04x", recv_checksum, calc_checksum);
		return -EBADMSG;
	}

	return 0;
}

static int zfm_x0_transceive(const struct device *dev, uint8_t cmd, const uint8_t *params,
			     uint16_t params_len, uint8_t *response, uint16_t *response_len)
{
	struct zfm_x0_data *data = dev->data;
	uint8_t *payload;
	uint16_t payload_len;
	int ret;

	if (params_len + 1 > ZFM_X0_MAX_DATA_LEN) {
		return -ENOMEM;
	}

	payload = &data->tx_pkt.buf[ZFM_X0_HDR_SIZE];
	payload[0] = cmd;
	if (params_len > 0) {
		memcpy(&payload[1], params, params_len);
	}

	ret = zfm_x0_send_packet(dev, ZFM_X0_PID_COMMAND, payload, params_len + 1);
	if (ret < 0) {
		return ret;
	}

	ret = zfm_x0_recv_packet(dev);
	if (ret < 0) {
		return ret;
	}

	if (data->rx_pkt.buf[6] != ZFM_X0_PID_ACK) {
		LOG_ERR("Expected ACK, got PID %02x", data->rx_pkt.buf[6]);
		return -EBADMSG;
	}

	payload_len = sys_get_be16(&data->rx_pkt.buf[7]) - ZFM_X0_CHECKSUM_SIZE;

	if (response && response_len) {
		*response_len = MIN(payload_len, *response_len);
		memcpy(response, &data->rx_pkt.buf[ZFM_X0_HDR_SIZE], *response_len);
	}

	return data->rx_pkt.buf[ZFM_X0_HDR_SIZE];
}

/* Timeout conversion with overflow check */
static int zfm_x0_poll_finger(const struct device *dev, uint32_t timeout_ms)
{
	const int64_t start = k_uptime_get();
	int ret;

	while ((k_uptime_get() - start) < timeout_ms) {
		ret = zfm_x0_transceive(dev, ZFM_X0_CMD_GET_IMAGE, NULL, 0, NULL, NULL);
		if (ret == ZFM_X0_OK) {
			return 0;
		}
		if (ret != ZFM_X0_ERR_NO_FINGER) {
			return zfm_x0_err_to_errno(ret);
		}
		k_msleep(ZFM_X0_FINGER_POLL_MS);
	}
	return -ETIMEDOUT;
}

static int zfm_x0_enroll_capture_blocking(const struct device *dev, uint8_t buffer_id,
					  uint32_t timeout_ms)
{
	struct zfm_x0_data *data = dev->data;
	int ret;

	ret = zfm_x0_poll_finger(dev, timeout_ms);
	if (ret < 0) {
		return ret;
	}

	ret = zfm_x0_transceive(dev, ZFM_X0_CMD_IMG_2_TZ, &buffer_id, 1, NULL, NULL);
	if (ret != ZFM_X0_OK) {
		LOG_ERR("Image conversion failed: %d", ret);
		return zfm_x0_err_to_errno(ret);
	}

	data->image_quality = 0;

	return 0;
}

static int zfm_x0_match_blocking(const struct device *dev, enum biometric_match_mode mode,
				 uint16_t template_id, uint32_t timeout_ms)
{
	struct zfm_x0_data *data = dev->data;
	uint8_t params[5];
	uint8_t response[ZFM_X0_MATCH_RESPONSE_SIZE];
	uint16_t response_len = sizeof(response);
	const uint8_t buffer = ZFM_X0_BUFFER_1;
	int ret;

	ret = zfm_x0_poll_finger(dev, timeout_ms);
	if (ret < 0) {
		return ret;
	}

	ret = zfm_x0_transceive(dev, ZFM_X0_CMD_IMG_2_TZ, &buffer, 1, NULL, NULL);
	if (ret != ZFM_X0_OK) {
		return zfm_x0_err_to_errno(ret);
	}

	data->image_quality = 0;

	if (mode == BIOMETRIC_MATCH_VERIFY) {
		params[0] = ZFM_X0_BUFFER_2;
		sys_put_be16(template_id - 1, &params[1]);

		ret = zfm_x0_transceive(dev, ZFM_X0_CMD_LOAD, params, 3, NULL, NULL);
		if (ret != ZFM_X0_OK) {
			return zfm_x0_err_to_errno(ret);
		}

		ret = zfm_x0_transceive(dev, ZFM_X0_CMD_MATCH, NULL, 0, response, &response_len);
		if (ret != ZFM_X0_OK) {
			return zfm_x0_err_to_errno(ret);
		}

		if (response_len < 3) {
			return -EBADMSG;
		}
		return sys_get_be16(&response[1]);
	}

	params[0] = ZFM_X0_BUFFER_1;
	sys_put_be16(0, &params[1]);
	sys_put_be16(data->max_templates, &params[3]);

	ret = zfm_x0_transceive(dev, ZFM_X0_CMD_SEARCH, params, 5, response, &response_len);
	if (ret != ZFM_X0_OK) {
		return zfm_x0_err_to_errno(ret);
	}

	if (response_len < 5) {
		return -EBADMSG;
	}

	data->last_match_id = sys_get_be16(&response[1]) + 1;
	return sys_get_be16(&response[3]);
}

static int zfm_x0_get_capabilities(const struct device *dev, struct biometric_capabilities *caps)
{
	struct zfm_x0_data *data = dev->data;

	caps->type = BIOMETRIC_TYPE_FINGERPRINT;
	caps->max_templates = data->max_templates;
	caps->template_size = ZFM_X0_TEMPLATE_SIZE;
	caps->storage_modes = BIOMETRIC_STORAGE_DEVICE;
	caps->enrollment_samples_required = 2;

	return 0;
}

static int zfm_x0_attr_set(const struct device *dev, enum biometric_attribute attr, int32_t val)
{
	struct zfm_x0_data *data = dev->data;
	uint8_t params[2];
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
		params[0] = ZFM_X0_PARAM_SECURITY;
		params[1] = CLAMP((val + 1) / 2, 1, 5);

		ret = zfm_x0_transceive(dev, ZFM_X0_CMD_SET_PARAM, params, 2, NULL, NULL);
		if (ret == ZFM_X0_OK) {
			data->security_level = val;
			ret = 0;
		} else {
			LOG_ERR("Failed to set security level: %d", ret);
			ret = zfm_x0_err_to_errno(ret);
		}
		break;

	case BIOMETRIC_ATTR_TIMEOUT_MS:
		if (val > ZFM_X0_MAX_TIMEOUT_MS) {
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

static int zfm_x0_attr_get(const struct device *dev, enum biometric_attribute attr, int32_t *val)
{
	struct zfm_x0_data *data = dev->data;
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
		*val = data->image_quality;
		break;
	case BIOMETRIC_ATTR_PRIV_START: /* Last matched template ID */
		*val = data->last_match_id;
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int zfm_x0_enroll_start(const struct device *dev, uint16_t template_id)
{
	struct zfm_x0_data *data = dev->data;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->enroll_state != ZFM_X0_ENROLL_IDLE) {
		ret = -EBUSY;
		goto unlock;
	}

	if (template_id == 0 || template_id > data->max_templates) {
		ret = -EINVAL;
		goto unlock;
	}

	data->enroll_state = ZFM_X0_ENROLL_WAIT_SAMPLE_1;
	data->enroll_id = template_id;

	LOG_INF("Enrollment started for ID %d", template_id);

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}

static int zfm_x0_enroll_capture(const struct device *dev, k_timeout_t timeout,
				 struct biometric_capture_result *result)
{
	struct zfm_x0_data *data = dev->data;
	enum zfm_x0_enroll_state current_state;
	uint32_t timeout_ms;
	uint8_t buffer_id;
	int ret;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		timeout_ms = data->timeout_ms;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		timeout_ms = 0;
	} else {
		int64_t ms = k_ticks_to_ms_ceil64(timeout.ticks);

		if (ms > ZFM_X0_MAX_TIMEOUT_MS) {
			return -EINVAL;
		}
		timeout_ms = (uint32_t)ms;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	current_state = data->enroll_state;

	if (current_state == ZFM_X0_ENROLL_IDLE) {
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	if (current_state == ZFM_X0_ENROLL_READY) {
		k_mutex_unlock(&data->lock);
		return -EALREADY;
	}

	buffer_id =
		(current_state == ZFM_X0_ENROLL_WAIT_SAMPLE_1) ? ZFM_X0_BUFFER_1 : ZFM_X0_BUFFER_2;

	ret = zfm_x0_enroll_capture_blocking(dev, buffer_id, timeout_ms);
	if (ret < 0) {
		data->enroll_state = ZFM_X0_ENROLL_IDLE;
		k_mutex_unlock(&data->lock);
		return ret;
	}

	if (current_state == ZFM_X0_ENROLL_WAIT_SAMPLE_1) {
		data->enroll_state = ZFM_X0_ENROLL_WAIT_SAMPLE_2;
	} else {
		data->enroll_state = ZFM_X0_ENROLL_READY;
	}

	if (result != NULL) {
		result->samples_captured =
			(data->enroll_state == ZFM_X0_ENROLL_WAIT_SAMPLE_2) ? 1 : 2;
		result->samples_required = 2;
		result->quality = (uint8_t)CLAMP(data->image_quality, 0, 100);
	}

	k_mutex_unlock(&data->lock);

	LOG_INF("Enrollment capture completed (sample %d/2)",
		(current_state == ZFM_X0_ENROLL_WAIT_SAMPLE_1) ? 1 : 2);

	return 0;
}

static int zfm_x0_enroll_finalize(const struct device *dev)
{
	struct zfm_x0_data *data = dev->data;
	uint8_t params[3];
	uint16_t enrolled_id;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (data->enroll_state != ZFM_X0_ENROLL_READY) {
		k_mutex_unlock(&data->lock);
		return -EINVAL;
	}

	ret = zfm_x0_transceive(dev, ZFM_X0_CMD_REG_MODEL, NULL, 0, NULL, NULL);
	if (ret != ZFM_X0_OK) {
		data->enroll_state = ZFM_X0_ENROLL_IDLE;
		k_mutex_unlock(&data->lock);
		LOG_ERR("Template creation failed: %d", ret);
		return zfm_x0_err_to_errno(ret);
	}

	params[0] = ZFM_X0_BUFFER_1;
	sys_put_be16(data->enroll_id - 1, &params[1]);

	ret = zfm_x0_transceive(dev, ZFM_X0_CMD_STORE, params, 3, NULL, NULL);
	if (ret != ZFM_X0_OK) {
		data->enroll_state = ZFM_X0_ENROLL_IDLE;
		k_mutex_unlock(&data->lock);
		LOG_ERR("Template storage failed: %d", ret);
		return zfm_x0_err_to_errno(ret);
	}

	enrolled_id = data->enroll_id;
	data->enroll_state = ZFM_X0_ENROLL_IDLE;
	data->template_count++;

	k_mutex_unlock(&data->lock);

	LOG_INF("Enrollment completed for ID %d", enrolled_id);

	return 0;
}

static int zfm_x0_enroll_abort(const struct device *dev)
{
	struct zfm_x0_data *data = dev->data;
	bool was_idle;

	k_mutex_lock(&data->lock, K_FOREVER);
	was_idle = (data->enroll_state == ZFM_X0_ENROLL_IDLE);
	data->enroll_state = ZFM_X0_ENROLL_IDLE;
	k_mutex_unlock(&data->lock);

	if (was_idle) {
		return -EALREADY;
	}

	LOG_INF("Enrollment aborted");
	return 0;
}

static int zfm_x0_template_delete(const struct device *dev, uint16_t id)
{
	struct zfm_x0_data *data = dev->data;
	uint8_t params[4];
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	sys_put_be16(id - 1, &params[0]);
	sys_put_be16(1, &params[2]);

	ret = zfm_x0_transceive(dev, ZFM_X0_CMD_DELETE, params, 4, NULL, NULL);

	if (ret == ZFM_X0_OK) {
		if (data->template_count > 0) {
			data->template_count--;
		}
		ret = 0;
	} else {
		ret = zfm_x0_err_to_errno(ret);
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int zfm_x0_template_delete_all(const struct device *dev)
{
	struct zfm_x0_data *data = dev->data;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = zfm_x0_transceive(dev, ZFM_X0_CMD_EMPTY, NULL, 0, NULL, NULL);

	if (ret == ZFM_X0_OK) {
		data->template_count = 0;
		ret = 0;
	} else {
		ret = zfm_x0_err_to_errno(ret);
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static int zfm_x0_template_list(const struct device *dev, uint16_t *ids, size_t max_count,
				size_t *actual_count)
{
	struct zfm_x0_data *data = dev->data;
	uint8_t response[ZFM_X0_INDEX_TABLE_SIZE];
	uint16_t response_len = sizeof(response);
	uint8_t page = 0;
	size_t count = 0;
	int ret;

	k_mutex_lock(&data->lock, K_FOREVER);

	ret = zfm_x0_transceive(dev, ZFM_X0_CMD_READ_INDEX, &page, 1, response, &response_len);

	if (ret != ZFM_X0_OK) {
		LOG_ERR("Failed to read template index: %d", ret);
		k_mutex_unlock(&data->lock);
		return zfm_x0_err_to_errno(ret);
	}

	for (size_t byte_idx = 0; byte_idx < response_len - 1; byte_idx++) {
		uint8_t byte = response[byte_idx + 1];

		for (uint8_t bit = 0; bit < 8; bit++) {
			if (count >= max_count) {
				break;
			}
			if (byte & (1 << bit)) {
				/* Convert 0-based hardware index to 1-based API template ID */
				ids[count++] = (byte_idx * 8 + bit) + 1;
			}
		}
		if (count >= max_count) {
			break;
		}
	}

	*actual_count = count;

	k_mutex_unlock(&data->lock);

	return 0;
}

static int zfm_x0_match(const struct device *dev, enum biometric_match_mode mode,
			uint16_t template_id, k_timeout_t timeout,
			struct biometric_match_result *result)
{
	struct zfm_x0_data *data = dev->data;
	uint32_t timeout_ms;
	int ret;
	int confidence;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		timeout_ms = data->timeout_ms;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		timeout_ms = 0;
	} else {
		int64_t ms = k_ticks_to_ms_ceil64(timeout.ticks);

		if (ms > ZFM_X0_MAX_TIMEOUT_MS) {
			return -EINVAL;
		}
		timeout_ms = (uint32_t)ms;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	/* Blocking match with timeout. Returns confidence score or negative error */
	confidence = zfm_x0_match_blocking(dev, mode, template_id, timeout_ms);

	if (confidence >= 0) {
		if (result != NULL) {
			result->confidence = confidence;
			result->template_id = (mode == BIOMETRIC_MATCH_IDENTIFY)
						      ? data->last_match_id
						      : template_id;
			result->image_quality = (uint8_t)CLAMP(data->image_quality, 0, 100);
		}
		LOG_INF("Match completed (mode=%d, score=%d)", mode, confidence);
		ret = 0;
	} else {
		LOG_DBG("Match failed: %d", confidence);
		ret = confidence;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int zfm_x0_led_control(const struct device *dev, enum biometric_led_state state)
{
	struct zfm_x0_data *data = dev->data;
	uint8_t params[4];
	int ret;

	switch (state) {
	case BIOMETRIC_LED_OFF:
		params[0] = ZFM_X0_LED_CTRL_OFF;
		params[1] = 0;
		params[2] = ZFM_X0_LED_COLOR_RED;
		params[3] = 0;
		break;

	case BIOMETRIC_LED_ON:
		params[0] = ZFM_X0_LED_CTRL_ON;
		params[1] = 0;
		params[2] = ZFM_X0_LED_COLOR_BLUE;
		params[3] = 0;
		break;

	case BIOMETRIC_LED_BLINK:
		params[0] = ZFM_X0_LED_CTRL_FLASHING;
		params[1] = ZFM_X0_LED_SPEED_MEDIUM;
		params[2] = ZFM_X0_LED_COLOR_PURPLE;
		params[3] = 0;
		break;

	case BIOMETRIC_LED_BREATHE:
		params[0] = ZFM_X0_LED_CTRL_BREATHING;
		params[1] = ZFM_X0_LED_SPEED_SLOW;
		params[2] = ZFM_X0_LED_COLOR_BLUE;
		params[3] = 0;
		break;

	default:
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	ret = zfm_x0_transceive(dev, ZFM_X0_CMD_LED_CONFIG, params, 4, NULL, NULL);

	if (ret == ZFM_X0_OK) {
		data->led_state = state;
		ret = 0;
	} else {
		ret = zfm_x0_err_to_errno(ret);
	}

	k_mutex_unlock(&data->lock);
	return ret;
}

static DEVICE_API(biometric, biometrics_zfm_x0_api) = {
	.get_capabilities = zfm_x0_get_capabilities,
	.attr_set = zfm_x0_attr_set,
	.attr_get = zfm_x0_attr_get,
	.enroll_start = zfm_x0_enroll_start,
	.enroll_capture = zfm_x0_enroll_capture,
	.enroll_finalize = zfm_x0_enroll_finalize,
	.enroll_abort = zfm_x0_enroll_abort,
	.template_store = NULL,
	.template_read = NULL,
	.template_delete = zfm_x0_template_delete,
	.template_delete_all = zfm_x0_template_delete_all,
	.template_list = zfm_x0_template_list,
	.match = zfm_x0_match,
	.led_control = zfm_x0_led_control,
};

static int zfm_x0_init(const struct device *dev)
{
	const struct zfm_x0_config *cfg = dev->config;
	struct zfm_x0_data *data = dev->data;
	uint8_t params[4];
	uint8_t response[ZFM_X0_SYS_PARAMS_SIZE];
	uint16_t response_len;
	int ret;

	if (!device_is_ready(cfg->uart_dev)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	data->dev = dev;
	data->comm_addr = cfg->comm_addr;
	data->enroll_state = ZFM_X0_ENROLL_IDLE;
	data->timeout_ms = CONFIG_ZFM_X0_TIMEOUT_MS;
	data->security_level = 6;
	data->match_threshold = 100;
	data->enroll_quality = 100;
	data->image_quality = 0;
	data->led_state = BIOMETRIC_LED_OFF;
	data->rx_error = ZFM_X0_RX_OK;
	data->last_match_id = 0;

	k_mutex_init(&data->lock);
	k_sem_init(&data->uart_tx_sem, 0, 1);
	k_sem_init(&data->uart_rx_sem, 0, 1);

	uart_irq_callback_user_data_set(cfg->uart_dev, zfm_x0_uart_callback, data);
	uart_irq_rx_disable(cfg->uart_dev);
	uart_irq_tx_disable(cfg->uart_dev);

	sys_put_be32(cfg->password, params);
	ret = zfm_x0_transceive(dev, ZFM_X0_CMD_VERIFY_PWD, params, 4, NULL, NULL);
	if (ret != ZFM_X0_OK) {
		LOG_ERR("Password verification failed: %d", ret);
		return -EACCES;
	}

	response_len = sizeof(response);
	ret = zfm_x0_transceive(dev, ZFM_X0_CMD_READ_PARAM, NULL, 0, response, &response_len);
	if (ret != ZFM_X0_OK) {
		LOG_ERR("Failed to read system parameters");
		return -EIO;
	}

	/* ZFM-x0 system parameters response:
	 * Byte 0: Confirmation code
	 * Byte 1-2: Status register
	 * Byte 3-4: System ID
	 * Byte 5-6: Library size (max templates)
	 * Byte 7-8: Security level
	 * Byte 9-12: Device address
	 * Byte 13-14: Data packet size
	 * Byte 15-16: Baud rate
	 */
	data->max_templates = sys_get_be16(&response[5]);

	response_len = sizeof(response);
	ret = zfm_x0_transceive(dev, ZFM_X0_CMD_TEMPLATE_COUNT, NULL, 0, response, &response_len);
	if (ret == ZFM_X0_OK) {
		data->template_count = sys_get_be16(&response[1]);
	}

	LOG_INF("ZFM-x0 initialized: %d/%d templates", data->template_count, data->max_templates);

	return 0;
}

#define ZFM_X0_DEFINE(inst)                                                                        \
	static struct zfm_x0_data zfm_x0_data_##inst;                                              \
                                                                                                   \
	static const struct zfm_x0_config zfm_x0_config_##inst = {                                 \
		.uart_dev = DEVICE_DT_GET(DT_INST_BUS(inst)),                                      \
		.comm_addr = DT_INST_PROP_OR(inst, comm_addr, ZFM_X0_DEFAULT_ADDRESS),             \
		.password = DT_INST_PROP_OR(inst, password, ZFM_X0_DEFAULT_PASSWORD),              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, zfm_x0_init, NULL, &zfm_x0_data_##inst, &zfm_x0_config_##inst, \
			      POST_KERNEL, CONFIG_BIOMETRICS_INIT_PRIORITY,                        \
			      &biometrics_zfm_x0_api);

DT_INST_FOREACH_STATUS_OKAY(ZFM_X0_DEFINE)
