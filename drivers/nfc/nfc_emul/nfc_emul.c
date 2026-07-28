/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_nfc_emul

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/nfc.h>
#include <zephyr/drivers/nfc/nfc_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(nfc_emul, CONFIG_NFC_DRIVERS_LOG_LEVEL);

/** Controller class an instance impersonates, in devicetree enum order. */
enum nfc_emul_backend {
	NFC_EMUL_BACKEND_OFFLOAD,
	NFC_EMUL_BACKEND_INITIATOR,
	NFC_EMUL_BACKEND_TARGET,
};

struct nfc_emul_config {
	enum nfc_emul_backend backend;
	nfc_proto_t protos;
	nfc_mode_t modes;
};

struct nfc_emul_data {
	struct k_mutex lock;

	const struct nfc_emul_frame *script;
	size_t script_len;
	size_t script_pos;

	nfc_target_cb_t poll_cb;
	void *poll_user_data;
	struct nfc_target target;
	bool target_staged;
	bool target_released;
	bool polling;

	nfc_target_event_cb_t target_cb;
	void *target_user_data;
	bool listening;

	uint32_t timeout_us;
	uint32_t field_cycles;
	bool rf_on;
	bool hw_tx_crc;
	bool hw_rx_crc;
};

static int nfc_emul_step(const struct device *dev, const uint8_t *tx_data, uint16_t tx_len,
			 uint8_t tx_last_bits, uint8_t *rx_data, uint16_t *rx_len)
{
	struct nfc_emul_data *data = dev->data;
	const struct nfc_emul_frame *frame;

	/* A real exchange waits on the field, so it is a scheduling point. */
	k_yield();

	if (data->script_pos >= data->script_len) {
		LOG_ERR("%s: script exhausted after %zu frame(s)", dev->name, data->script_len);
		return -ENODATA;
	}

	frame = &data->script[data->script_pos];
	data->script_pos++;

	if (frame->tx != NULL) {
		if (tx_len != frame->tx_len || memcmp(tx_data, frame->tx, tx_len) != 0) {
			LOG_ERR("%s: frame %zu payload mismatch", dev->name, data->script_pos - 1U);
			LOG_HEXDUMP_ERR(tx_data, tx_len, "actual");
			LOG_HEXDUMP_ERR(frame->tx, frame->tx_len, "expected");
			return -EIO;
		}

		/* 0 and 8 both mean a whole byte, as the hardware drivers mask it. */
		if ((tx_last_bits % 8U) != (frame->tx_last_bits % 8U)) {
			LOG_ERR("%s: frame %zu expected %u last bit(s), got %u", dev->name,
				data->script_pos - 1U, frame->tx_last_bits, tx_last_bits);
			return -EIO;
		}
	}

	if (frame->ret != 0) {
		return frame->ret;
	}

	if (rx_data == NULL || rx_len == NULL) {
		return 0;
	}

	if (*rx_len < frame->rx_len) {
		return -ENOMEM;
	}

	if (frame->rx_len != 0U) {
		memcpy(rx_data, frame->rx, frame->rx_len);
	}
	*rx_len = frame->rx_len;

	return 0;
}

static nfc_proto_t nfc_emul_supported_protocols(const struct device *dev)
{
	const struct nfc_emul_config *cfg = dev->config;

	return cfg->protos;
}

static nfc_mode_t nfc_emul_supported_modes(const struct device *dev, nfc_proto_t proto)
{
	const struct nfc_emul_config *cfg = dev->config;

	if ((proto & cfg->protos) == 0U) {
		return 0;
	}

	return cfg->modes;
}

static nfc_proto_t nfc_emul_claim(const struct device *dev)
{
	const struct nfc_emul_config *cfg = dev->config;
	struct nfc_emul_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	return cfg->protos;
}

static int nfc_emul_release(const struct device *dev)
{
	struct nfc_emul_data *data = dev->data;

	return k_mutex_unlock(&data->lock);
}

static int nfc_emul_load_protocol(const struct device *dev, nfc_proto_t proto, nfc_mode_t mode)
{
	const struct nfc_emul_config *cfg = dev->config;

	if ((proto & cfg->protos) == 0U || (mode & cfg->modes & NFC_MODE_ROLE_MASK) == 0U) {
		return -ENOTSUP;
	}

	return 0;
}

static int nfc_emul_get_properties(const struct device *dev, struct nfc_property *props,
				   size_t props_len)
{
	struct nfc_emul_data *data = dev->data;

	for (size_t i = 0; i < props_len; i++) {
		props[i].status = 0;

		switch (props[i].type) {
		case NFC_PROP_RF_FIELD:
			props[i].rf_on = data->rf_on;
			break;
		case NFC_PROP_HW_TX_CRC:
			props[i].hw_tx_crc = data->hw_tx_crc;
			break;
		case NFC_PROP_HW_RX_CRC:
			props[i].hw_rx_crc = data->hw_rx_crc;
			break;
		case NFC_PROP_TIMEOUT:
			props[i].timeout_us = data->timeout_us;
			break;
		default:
			props[i].status = -ENOTSUP;
			break;
		}
	}

	return 0;
}

static int nfc_emul_set_properties(const struct device *dev, struct nfc_property *props,
				   size_t props_len)
{
	struct nfc_emul_data *data = dev->data;

	for (size_t i = 0; i < props_len; i++) {
		props[i].status = 0;

		switch (props[i].type) {
		case NFC_PROP_RF_FIELD:
			if (data->rf_on && !props[i].rf_on) {
				data->field_cycles++;
			}
			data->rf_on = props[i].rf_on;
			break;
		case NFC_PROP_HW_TX_CRC:
			data->hw_tx_crc = props[i].hw_tx_crc;
			break;
		case NFC_PROP_HW_RX_CRC:
			data->hw_rx_crc = props[i].hw_rx_crc;
			break;
		case NFC_PROP_TIMEOUT:
			data->timeout_us = props[i].timeout_us;
			break;
		default:
			props[i].status = -ENOTSUP;
			break;
		}
	}

	return 0;
}

static int nfc_emul_im_transceive(const struct device *dev, const uint8_t *tx_data, uint16_t tx_len,
				  uint8_t tx_last_bits, uint8_t *rx_data, uint16_t *rx_len)
{
	return nfc_emul_step(dev, tx_data, tx_len, tx_last_bits, rx_data, rx_len);
}

static int nfc_emul_offload_poll_start(const struct device *dev, nfc_proto_t protos,
				       const struct nfc_poll_config *cfg, nfc_target_cb_t cb,
				       void *user_data)
{
	const struct nfc_emul_config *config = dev->config;
	struct nfc_emul_data *data = dev->data;

	ARG_UNUSED(cfg);

	if ((protos & config->protos) == 0U) {
		return -ENOTSUP;
	}

	data->poll_cb = cb;
	data->poll_user_data = user_data;
	data->polling = true;

	if (data->target_staged && cb != NULL) {
		cb(dev, &data->target, user_data);
	}

	return 0;
}

static int nfc_emul_offload_poll_stop(const struct device *dev)
{
	struct nfc_emul_data *data = dev->data;

	data->polling = false;
	data->poll_cb = NULL;
	data->poll_user_data = NULL;

	return 0;
}

static int nfc_emul_offload_exchange(const struct device *dev, const struct nfc_target *target,
				     const uint8_t *tx_data, uint16_t tx_len, uint8_t *rx_data,
				     uint16_t *rx_len, uint32_t timeout_ms)
{
	ARG_UNUSED(target);
	ARG_UNUSED(timeout_ms);

	return nfc_emul_step(dev, tx_data, tx_len, 0U, rx_data, rx_len);
}

static int nfc_emul_offload_release(const struct device *dev, const struct nfc_target *target)
{
	struct nfc_emul_data *data = dev->data;

	ARG_UNUSED(target);

	k_mutex_lock(&data->lock, K_FOREVER);
	data->target_released = true;
	k_mutex_unlock(&data->lock);

	return 0;
}

static int nfc_emul_target_start(const struct device *dev, nfc_proto_t protos,
				 nfc_target_event_cb_t cb, void *user_data)
{
	const struct nfc_emul_config *cfg = dev->config;
	struct nfc_emul_data *data = dev->data;

	if ((protos & cfg->protos) == 0U) {
		return -ENOTSUP;
	}

	data->target_cb = cb;
	data->target_user_data = user_data;
	data->listening = true;

	return 0;
}

static int nfc_emul_target_stop(const struct device *dev)
{
	struct nfc_emul_data *data = dev->data;

	data->listening = false;
	data->target_cb = NULL;
	data->target_user_data = NULL;

	return 0;
}

static int nfc_emul_target_send(const struct device *dev, const uint8_t *tx_data, uint16_t tx_len,
				uint8_t tx_last_bits)
{
	struct nfc_emul_data *data = dev->data;

	if (!data->listening) {
		return -EPERM;
	}

	return nfc_emul_step(dev, tx_data, tx_len, tx_last_bits, NULL, NULL);
}

void nfc_emul_load_script(const struct device *dev, const struct nfc_emul_frame *frames,
			  size_t count)
{
	struct nfc_emul_data *data = dev->data;

	data->script = frames;
	data->script_len = frames != NULL ? count : 0U;
	data->script_pos = 0U;
}

size_t nfc_emul_script_remaining(const struct device *dev)
{
	struct nfc_emul_data *data = dev->data;

	return data->script_len - data->script_pos;
}

uint32_t nfc_emul_field_cycles(const struct device *dev)
{
	struct nfc_emul_data *data = dev->data;

	return data->field_cycles;
}

int nfc_emul_set_target(const struct device *dev, const struct nfc_target *target)
{
	const struct nfc_emul_config *cfg = dev->config;
	struct nfc_emul_data *data = dev->data;

	if (cfg->backend != NFC_EMUL_BACKEND_OFFLOAD) {
		return -ENOTSUP;
	}

	if (target == NULL) {
		data->target_staged = false;
		return 0;
	}

	data->target = *target;
	data->target_staged = true;
	data->target_released = false;

	if (data->polling && data->poll_cb != NULL) {
		data->poll_cb(dev, &data->target, data->poll_user_data);
	}

	return 0;
}

bool nfc_emul_target_released(const struct device *dev)
{
	struct nfc_emul_data *data = dev->data;
	bool released;

	k_mutex_lock(&data->lock, K_FOREVER);
	released = data->target_released;
	k_mutex_unlock(&data->lock);

	return released;
}

int nfc_emul_raise_target_event(const struct device *dev, enum nfc_target_event event,
				const uint8_t *data_buf, uint16_t len)
{
	const struct nfc_emul_config *cfg = dev->config;
	struct nfc_emul_data *data = dev->data;

	if (cfg->backend != NFC_EMUL_BACKEND_TARGET) {
		return -ENOTSUP;
	}

	if (!data->listening || data->target_cb == NULL) {
		return -EPERM;
	}

	data->target_cb(dev, event, data_buf, len, data->target_user_data);

	return 0;
}

static int nfc_emul_init(const struct device *dev)
{
	struct nfc_emul_data *data = dev->data;

	data->rf_on = true;

	return k_mutex_init(&data->lock);
}

DEVICE_API(nfc, nfc_emul_api_offload) = {
	.supported_protocols = nfc_emul_supported_protocols,
	.supported_modes = nfc_emul_supported_modes,
	.offload_poll_start = nfc_emul_offload_poll_start,
	.offload_poll_stop = nfc_emul_offload_poll_stop,
	.offload_exchange = nfc_emul_offload_exchange,
	.offload_release = nfc_emul_offload_release,
};

DEVICE_API(nfc, nfc_emul_api_initiator) = {
	.claim = nfc_emul_claim,
	.release = nfc_emul_release,
	.load_protocol = nfc_emul_load_protocol,
	.get_properties = nfc_emul_get_properties,
	.set_properties = nfc_emul_set_properties,
	.im_transceive = nfc_emul_im_transceive,
	.supported_protocols = nfc_emul_supported_protocols,
	.supported_modes = nfc_emul_supported_modes,
};

DEVICE_API(nfc, nfc_emul_api_target) = {
	.target_start = nfc_emul_target_start,
	.target_stop = nfc_emul_target_stop,
	.target_send = nfc_emul_target_send,
	.supported_protocols = nfc_emul_supported_protocols,
	.supported_modes = nfc_emul_supported_modes,
};

#define NFC_EMUL_API_OFFLOAD   (&nfc_emul_api_offload)
#define NFC_EMUL_API_INITIATOR (&nfc_emul_api_initiator)
#define NFC_EMUL_API_TARGET    (&nfc_emul_api_target)

#define NFC_EMUL_PROTOS_OFFLOAD   (NFC_PROTO_ISO14443A | NFC_PROTO_ISO14443B)
#define NFC_EMUL_PROTOS_INITIATOR NFC_PROTO_ISO14443A
#define NFC_EMUL_PROTOS_TARGET    NFC_PROTO_ISO14443A

#define NFC_EMUL_MODES_OFFLOAD   NFC_MODE_INITIATOR
#define NFC_EMUL_MODES_INITIATOR (NFC_MODE_INITIATOR | NFC_MODE_TX_106 | NFC_MODE_RX_106)
#define NFC_EMUL_MODES_TARGET    (NFC_MODE_TARGET | NFC_MODE_TX_106 | NFC_MODE_RX_106)

#define NFC_EMUL_DEFINE(inst)                                                                      \
	static struct nfc_emul_data nfc_emul_data_##inst;                                          \
                                                                                                   \
	static const struct nfc_emul_config nfc_emul_config_##inst = {                             \
		.backend = (enum nfc_emul_backend)DT_INST_ENUM_IDX(inst, backend),                 \
		.protos = UTIL_CAT(NFC_EMUL_PROTOS_, DT_INST_STRING_UPPER_TOKEN(inst, backend)),   \
		.modes = UTIL_CAT(NFC_EMUL_MODES_, DT_INST_STRING_UPPER_TOKEN(inst, backend)),     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, nfc_emul_init, NULL, &nfc_emul_data_##inst,                    \
			      &nfc_emul_config_##inst, POST_KERNEL, CONFIG_NFC_INIT_PRIORITY,      \
			      UTIL_CAT(NFC_EMUL_API_, DT_INST_STRING_UPPER_TOKEN(inst, backend)));

DT_INST_FOREACH_STATUS_OKAY(NFC_EMUL_DEFINE)
