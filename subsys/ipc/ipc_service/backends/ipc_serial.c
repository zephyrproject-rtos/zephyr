/*
 * SPDX-FileCopyrightText: 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/ipc/ipc_service_backend.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/data/cobs.h>

#ifdef CONFIG_IPC_SERVICE_BACKEND_SERIAL_CRC
#include <zephyr/sys/crc.h>
#endif

#define DT_DRV_COMPAT zephyr_ipc_serial

LOG_MODULE_REGISTER(ipc_serial, CONFIG_IPC_SERVICE_LOG_LEVEL);

#define NUM_EP         CONFIG_IPC_SERVICE_BACKEND_SERIAL_NUM_EP
#define FRAME_BUF_SIZE CONFIG_IPC_SERVICE_BACKEND_SERIAL_FRAME_BUF_SIZE

#define SERIAL_PROTOCOL_VERSION 1

/*
 * Wire protocol:
 *
 * Each frame is COBS-encoded and delimited by 0x00 bytes.
 * Every decoded frame starts with a 1-byte command ID:
 *
 *   [cmd_id:u8][data...]
 *
 * Commands:
 *
 *   SERIAL_CMD_RESET      [version:u8][max_rx_frame:u16_be]
 *     - Sent by a side when it (re)opens the instance.  Carries the sender's
 *       protocol version and maximum decoded frame size.  Upon receipt the
 *       remote stores these capabilities, replies with SERIAL_CMD_RESET_ACK,
 *       then marks all currently-bound endpoints as unbound and fires their
 *       unbound callbacks, so applications can detect a peer reset and wait
 *       for endpoints to re-bind.
 *
 *   SERIAL_CMD_RESET_ACK  [version:u8][max_rx_frame:u16_be]
 *     - Sent in reply to SERIAL_CMD_RESET.  Carries the same capability
 *       fields so the initiating side learns the remote's parameters without
 *       waiting for the remote to (re)open.  Does not trigger endpoint
 *       unbinding.
 *
 *   SERIAL_CMD_DISC_REQ   [local_id:u8][name_bytes...]
 *     - Endpoint discovery request. Sent when registering an endpoint.
 *       The receiver looks up a matching local endpoint by name, records
 *       the remote ID mapping, marks the endpoint bound, and replies with
 *       SERIAL_CMD_DISC_RSP.
 *
 *   SERIAL_CMD_DISC_RSP   [local_id:u8][name_bytes...]
 *     - Endpoint discovery response. Sent in reply to SERIAL_CMD_DISC_REQ.
 *       The receiver records the remote ID mapping and marks the endpoint
 *       bound. No further reply is sent.
 *
 *   SERIAL_CMD_DEREGISTER [local_id:u8]
 *     - Endpoint deregistration. Signals that the sender is removing the
 *       endpoint, triggering the unbound callback on the receiver.
 *
 *   SERIAL_CMD_DATA       [ept_id:u8][payload...]
 *     - Data frame addressed to a bound endpoint.
 */

enum serial_cmd {
	SERIAL_CMD_RESET      = 1,
	SERIAL_CMD_RESET_ACK  = 2,
	SERIAL_CMD_DISC_REQ   = 3,
	SERIAL_CMD_DISC_RSP   = 4,
	SERIAL_CMD_DEREGISTER = 5,
	SERIAL_CMD_DATA       = 6,
};

struct serial_ept {
	const struct ipc_ept_cfg *cfg;
	uint8_t local_id;  /* 1-based local endpoint ID */
	uint8_t remote_id; /* Remote peer's endpoint ID (0 = not yet bound) */
	bool bound;
};

struct serial_config {
	const struct device *uart_dev;
	uint32_t timeout_ms;
};

struct serial_data {
	const struct device *self;

	/* Endpoint table */
	struct k_mutex ept_mutex;
	struct serial_ept epts[NUM_EP];

	/* UART RX */
	struct ring_buf rx_ringbuf;
	uint8_t rx_ringbuf_data[CONFIG_IPC_SERVICE_BACKEND_SERIAL_RX_BUF_SIZE];
	struct k_work rx_work;

	/* COBS decoder for incoming frames */
	uint8_t decode_buf[FRAME_BUF_SIZE];
	size_t decode_len;
	struct cobs_decoder decoder;

	/* TX serialization */
	struct k_mutex tx_mutex;
	struct cobs_encoder encoder;

	/* Endpoint binding sync */
	struct k_event bind_event;

	/* Remote capabilities */
	uint8_t remote_version; /* 0 = unknown */
	uint16_t remote_max_rx; /* 0 = unknown */

	bool opened;
};

static void rx_work_handler(struct k_work *work);
static int decoder_output_cb(const uint8_t *buf, size_t len, void *user_data);

static void uart_isr_callback(const struct device *uart_dev, void *user_data)
{
	struct serial_data *data = user_data;
	uint8_t *buf;
	uint32_t claimed;
	int read;

	while (uart_irq_update(uart_dev) && uart_irq_is_pending(uart_dev)) {
		if (!uart_irq_rx_ready(uart_dev)) {
			continue;
		}

		claimed = ring_buf_put_claim(&data->rx_ringbuf, &buf, UINT32_MAX);
		if (claimed == 0) {
			uint8_t dummy;

			while (uart_fifo_read(uart_dev, &dummy, 1) == 1) {
			}
			LOG_WRN("RX ring buffer full, dropping bytes");
			k_work_submit(&data->rx_work);
			continue;
		}

		read = uart_fifo_read(uart_dev, buf, claimed);
		ring_buf_put_finish(&data->rx_ringbuf, read > 0 ? read : 0);

		if (read > 0) {
			k_work_submit(&data->rx_work);
		}
	}
}

static struct serial_ept *find_ept_by_name(struct serial_data *data, const char *name,
					   size_t name_len)
{
	for (int i = 0; i < NUM_EP; i++) {
		if (data->epts[i].cfg != NULL && strlen(data->epts[i].cfg->name) == name_len &&
		    memcmp(data->epts[i].cfg->name, name, name_len) == 0) {
			return &data->epts[i];
		}
	}
	return NULL;
}

static struct serial_ept *find_ept_by_remote_id(struct serial_data *data, uint8_t remote_id)
{
	for (int i = 0; i < NUM_EP; i++) {
		if (data->epts[i].cfg != NULL && data->epts[i].remote_id == remote_id) {
			return &data->epts[i];
		}
	}
	return NULL;
}

static int encoder_uart_cb(const uint8_t *buf, size_t len, void *user_data)
{
	const struct device *uart_dev = user_data;

	for (size_t i = 0; i < len; i++) {
		uart_poll_out(uart_dev, buf[i]);
	}

	return 0;
}

static int send_frame(struct serial_data *data, const uint8_t *hdr, size_t hdr_len,
		      const uint8_t *payload, size_t payload_len)
{
	const struct serial_config *cfg = data->self->config;
	int ret;

	ret = cobs_encoder_init(&data->encoder, encoder_uart_cb, (void *)cfg->uart_dev,
				COBS_FLAG_TRAILING_DELIMITER);
	if (ret < 0) {
		return ret;
	}

	ret = cobs_encoder_write(&data->encoder, hdr, hdr_len);
	if (ret < 0) {
		return ret;
	}

	ret = cobs_encoder_write(&data->encoder, payload, payload_len);
	if (ret < 0) {
		return ret;
	}

#ifdef CONFIG_IPC_SERVICE_BACKEND_SERIAL_CRC
	uint8_t crc_buf[2];

	sys_put_be16(crc16_itu_t(crc16_itu_t(0, hdr, hdr_len), payload, payload_len), crc_buf);

	ret = cobs_encoder_write(&data->encoder, crc_buf, sizeof(crc_buf));
	if (ret < 0) {
		return ret;
	}
#endif

	return cobs_encoder_close(&data->encoder);
}

static int send_string(struct serial_data *data, enum serial_cmd cmd, uint8_t local_id,
		       const char *txt)
{
	const uint8_t hdr[] = {
		cmd,
		local_id,
	};
	size_t len = txt == NULL ? 0 : strlen(txt);
	int ret;

	k_mutex_lock(&data->tx_mutex, K_FOREVER);
	ret = send_frame(data, hdr, sizeof(hdr), txt, len);
	k_mutex_unlock(&data->tx_mutex);

	return ret;
}

static int send_capabilities(struct serial_data *data, enum serial_cmd cmd)
{
	uint8_t hdr[4];
	int ret;

	hdr[0] = (uint8_t)cmd;
	hdr[1] = SERIAL_PROTOCOL_VERSION;
	sys_put_be16(FRAME_BUF_SIZE, &hdr[2]);

	k_mutex_lock(&data->tx_mutex, K_FOREVER);
	ret = send_frame(data, hdr, sizeof(hdr), NULL, 0);
	k_mutex_unlock(&data->tx_mutex);

	return ret;
}

static void store_remote_capabilities(struct serial_data *data, const uint8_t *payload,
				      size_t payload_len)
{
	if (payload_len >= 1) {
		data->remote_version = payload[0];
		if (data->remote_version != SERIAL_PROTOCOL_VERSION) {
			LOG_ERR("%s: Protocol version mismatch: local=%d remote=%d",
				data->self->name, SERIAL_PROTOCOL_VERSION,
				data->remote_version);
		}
	} else {
		LOG_WRN("%s: remote did not send protocol version", data->self->name);
		data->remote_version = 0;
	}

	if (payload_len >= 3) {
		data->remote_max_rx = sys_get_be16(&payload[1]);
		LOG_DBG("%s: remote max RX frame: %u bytes", data->self->name,
			data->remote_max_rx);
	} else {
		data->remote_max_rx = 0;
	}
}

static void handle_cmd_reset(struct serial_data *data, const uint8_t *payload,
			     size_t payload_len)
{
	int ret;

	store_remote_capabilities(data, payload, payload_len);
	ret = send_capabilities(data, SERIAL_CMD_RESET_ACK);
	if (ret < 0) {
		LOG_ERR("%s: error on send reset ACK: %d", data->self->name, ret);
	}

	LOG_DBG("%s: Remote reset", data->self->name);

	for (int i = 0; i < NUM_EP; i++) {
		struct serial_ept *ept = &data->epts[i];
		const struct ipc_ept_cfg *cfg;

		k_mutex_lock(&data->ept_mutex, K_FOREVER);
		if (ept->cfg == NULL || !ept->bound) {
			k_mutex_unlock(&data->ept_mutex);
			continue;
		}
		cfg = ept->cfg;
		ept->remote_id = 0;
		ept->bound = false;
		k_mutex_unlock(&data->ept_mutex);

		if (cfg->cb.unbound != NULL) {
			cfg->cb.unbound(cfg->priv);
		}
	}
}

static void handle_cmd_reset_ack(struct serial_data *data, const uint8_t *payload,
				 size_t payload_len)
{
	store_remote_capabilities(data, payload, payload_len);
	LOG_DBG("%s: Received RESET_ACK from remote", data->self->name);
}

static void handle_cmd_deregister(struct serial_data *data, const uint8_t *payload,
				  size_t payload_len)
{
	if (payload_len < 1) {
		LOG_WRN("Deregister message too short");
		return;
	}

	uint8_t remote_local_id = payload[0];

	LOG_DBG("Deregister: remote_id=%d", remote_local_id);

	k_mutex_lock(&data->ept_mutex, K_FOREVER);

	struct serial_ept *ept = find_ept_by_remote_id(data, remote_local_id);

	if (ept == NULL || !ept->bound) {
		k_mutex_unlock(&data->ept_mutex);
		return;
	}

	const struct ipc_ept_cfg *cfg = ept->cfg;

	ept->remote_id = 0;
	ept->bound = false;
	k_mutex_unlock(&data->ept_mutex);

	if (cfg != NULL && cfg->cb.unbound != NULL) {
		cfg->cb.unbound(cfg->priv);
	}
}

static void handle_cmd_disc_req(struct serial_data *data, const uint8_t *payload,
				size_t payload_len)
{
	if (payload_len < 2) {
		LOG_WRN("Discovery request too short");
		return;
	}

	uint8_t remote_local_id = payload[0];
	const char *name = (const char *)&payload[1];
	size_t name_len = payload_len - 1;
	int ret;

	LOG_DBG("Disc req: remote_id=%d name=%.*s", remote_local_id, (int)name_len, name);

	k_mutex_lock(&data->ept_mutex, K_FOREVER);

	struct serial_ept *ept = find_ept_by_name(data, name, name_len);

	if (ept == NULL) {
		LOG_DBG("No local endpoint matching '%.*s'", (int)name_len, name);
		k_mutex_unlock(&data->ept_mutex);
		return;
	}

	bool was_bound = ept->bound;
	const struct ipc_ept_cfg *cfg = ept->cfg;

	ept->remote_id = remote_local_id;
	ept->bound = true;
	k_mutex_unlock(&data->ept_mutex);

	ret = send_string(data, SERIAL_CMD_DISC_RSP, ept->local_id, cfg->name);
	if (ret < 0) {
		LOG_ERR("Failed to send discovery response for endpoint '%s': %d", cfg->name, ret);
		return;
	}

	if (was_bound) {
		/*
		 * Remote re-registered without sending SERIAL_CMD_RESET first
		 * (e.g. crash or power loss).  Treat the old binding as gone
		 * before signalling the new one.
		 */
		LOG_WRN("%s: endpoint '%s' re-bound without reset", data->self->name, cfg->name);
		if (cfg->cb.unbound != NULL) {
			cfg->cb.unbound(cfg->priv);
		}
	}

	k_event_post(&data->bind_event, BIT(ept->local_id - 1));

	if (cfg->cb.bound != NULL) {
		cfg->cb.bound(cfg->priv);
	}
}

static void handle_cmd_disc_rsp(struct serial_data *data, const uint8_t *payload,
				size_t payload_len)
{
	if (payload_len < 2) {
		LOG_WRN("Discovery response too short");
		return;
	}

	uint8_t remote_local_id = payload[0];
	const char *name = (const char *)&payload[1];
	size_t name_len = payload_len - 1;

	LOG_DBG("Disc rsp: remote_id=%d name=%.*s", remote_local_id, (int)name_len, name);

	k_mutex_lock(&data->ept_mutex, K_FOREVER);

	struct serial_ept *ept = find_ept_by_name(data, name, name_len);

	if (ept == NULL) {
		LOG_DBG("No local endpoint matching '%.*s'", (int)name_len, name);
		k_mutex_unlock(&data->ept_mutex);
		return;
	}

	ept->remote_id = remote_local_id;

	if (!ept->bound) {
		const struct ipc_ept_cfg *cfg = ept->cfg;
		uint8_t local_id = ept->local_id;

		ept->bound = true;
		k_mutex_unlock(&data->ept_mutex);

		k_event_post(&data->bind_event, BIT(local_id - 1));

		if (cfg != NULL && cfg->cb.bound != NULL) {
			cfg->cb.bound(cfg->priv);
		}
	} else {
		k_mutex_unlock(&data->ept_mutex);
	}
}

static void process_frame(struct serial_data *data, const uint8_t *frame, size_t frame_len)
{
#ifdef CONFIG_IPC_SERVICE_BACKEND_SERIAL_CRC
	uint16_t crc = crc16_itu_t(0, frame, frame_len);

	if (crc != 0U || frame_len < 3) {
		LOG_WRN("Drop frame (crc: 0x%04x, len: %zu)", crc, frame_len);
		return;
	}

	frame_len -= 2;

#else
	if (frame_len < 1) {
		return;
	}
#endif

	uint8_t cmd_id = frame[0];
	const uint8_t *payload = &frame[1];
	size_t payload_len = frame_len - 1;

	switch (cmd_id) {
	case SERIAL_CMD_RESET:
		handle_cmd_reset(data, payload, payload_len);
		break;
	case SERIAL_CMD_RESET_ACK:
		handle_cmd_reset_ack(data, payload, payload_len);
		break;
	case SERIAL_CMD_DISC_REQ:
		handle_cmd_disc_req(data, payload, payload_len);
		break;
	case SERIAL_CMD_DISC_RSP:
		handle_cmd_disc_rsp(data, payload, payload_len);
		break;
	case SERIAL_CMD_DEREGISTER:
		handle_cmd_deregister(data, payload, payload_len);
		break;
	case SERIAL_CMD_DATA: {
		if (payload_len < 1) {
			break;
		}

		uint8_t ept_id = payload[0];
		struct serial_ept *ept;

		k_mutex_lock(&data->ept_mutex, K_FOREVER);
		ept = find_ept_by_remote_id(data, ept_id);
		k_mutex_unlock(&data->ept_mutex);

		if (ept == NULL || ept->cfg == NULL) {
			LOG_WRN("Received data for unknown endpoint ID %d", ept_id);
			break;
		}

		if (ept->cfg->cb.received != NULL) {
			ept->cfg->cb.received(&payload[1], payload_len - 1, ept->cfg->priv);
		}
		break;
	}
	default:
		LOG_WRN("Unknown command: %d", cmd_id);
		break;
	}
}

/*
 * COBS streaming decoder callback.
 * Called with decoded data chunks, or with NULL to indicate frame end.
 */
static int decoder_output_cb(const uint8_t *buf, size_t len, void *user_data)
{
	struct serial_data *data = user_data;

	if (buf == NULL && len == 0) {
		/* Frame delimiter received - process complete frame */
		if (data->decode_len > 0) {
			process_frame(data, data->decode_buf, data->decode_len);
			data->decode_len = 0;
		}
		return 0;
	}

	/* Accumulate decoded data */
	if (data->decode_len + len > FRAME_BUF_SIZE) {
		LOG_WRN("Decoded frame exceeds buffer, discarding");
		data->decode_len = 0;
		return -ENOMEM;
	}

	memcpy(&data->decode_buf[data->decode_len], buf, len);
	data->decode_len += len;

	return 0;
}

static void rx_work_handler(struct k_work *work)
{
	struct serial_data *data = CONTAINER_OF(work, struct serial_data, rx_work);
	uint8_t byte;

	while (ring_buf_get(&data->rx_ringbuf, &byte, 1) == 1) {
		int ret = cobs_decoder_write(&data->decoder, &byte, 1);

		if (ret < 0) {
			LOG_WRN("COBS decode error: %d, resetting decoder", ret);
			data->decode_len = 0;
			cobs_decoder_init(&data->decoder, decoder_output_cb, data,
					  COBS_FLAG_TRAILING_DELIMITER);
		}
	}
}

static int serial_open(const struct device *instance)
{
	const struct serial_config *cfg = instance->config;
	struct serial_data *data = instance->data;
	int ret;

	if (data->opened) {
		return -EALREADY;
	}

	if (!device_is_ready(cfg->uart_dev)) {
		LOG_ERR("UART device not ready");
		return -ENODEV;
	}

	/* Initialize ring buffer */
	ring_buf_init(&data->rx_ringbuf, sizeof(data->rx_ringbuf_data), data->rx_ringbuf_data);

	/* Initialize COBS streaming decoder */
	cobs_decoder_init(&data->decoder, decoder_output_cb, data, COBS_FLAG_TRAILING_DELIMITER);
	data->decode_len = 0;

	/* Set up UART interrupt-driven RX/TX */
	ret = uart_irq_callback_user_data_set(cfg->uart_dev, uart_isr_callback, data);
	if (ret < 0) {
		LOG_ERR("Failed to set UART callback (%d)", ret);
		return ret;
	}
	uart_irq_rx_enable(cfg->uart_dev);

	data->remote_version = 0;
	data->remote_max_rx = 0;
	data->opened = true;
	LOG_DBG("Serial IPC backend opened on %s", cfg->uart_dev->name);

	/*
	 * Announce (re)start to the remote so it can unbound any stale
	 * endpoints from a previous session and wait for re-binding.
	 * The remote will reply with RESET_ACK carrying its capabilities.
	 */
	ret = send_capabilities(data, SERIAL_CMD_RESET);
	if (ret < 0) {
		LOG_ERR("Failed to send RESET (%d)", ret);
		return ret;
	}

	return 0;
}

static int serial_close(const struct device *instance)
{
	const struct serial_config *cfg = instance->config;
	struct serial_data *data = instance->data;

	if (!data->opened) {
		return -EALREADY;
	}

	uart_irq_rx_disable(cfg->uart_dev);

	for (int i = 0; i < NUM_EP; i++) {
		struct serial_ept *ept = &data->epts[i];

		k_mutex_lock(&data->ept_mutex, K_FOREVER);
		if (ept->bound && ept->cfg != NULL) {
			const struct ipc_ept_cfg *ept_cfg = ept->cfg;

			ept->remote_id = 0;
			ept->bound = false;
			k_mutex_unlock(&data->ept_mutex);

			if (ept_cfg->cb.unbound != NULL) {
				ept_cfg->cb.unbound(ept_cfg->priv);
			}
		} else {
			k_mutex_unlock(&data->ept_mutex);
		}
	}

	data->opened = false;

	return 0;
}

static int serial_register_ept(const struct device *instance, void **token,
			       const struct ipc_ept_cfg *cfg)
{
	const struct serial_config *serial_cfg = instance->config;
	struct serial_data *data = instance->data;
	int ret;

	if (cfg == NULL || cfg->name == NULL || strlen(cfg->name) == 0) {
		return -EINVAL;
	}

	k_mutex_lock(&data->ept_mutex, K_FOREVER);

	/* Find a free slot (cfg == NULL means the slot was freed by deregister) */
	struct serial_ept *ept = NULL;

	for (int i = 0; i < NUM_EP; i++) {
		if (data->epts[i].cfg == NULL) {
			ept = &data->epts[i];
			break;
		}
	}

	if (ept == NULL) {
		k_mutex_unlock(&data->ept_mutex);
		LOG_ERR("No free endpoint slots");
		return -ENOMEM;
	}

	ept->cfg = cfg;
	ept->remote_id = 0;
	ept->bound = false;

	*token = ept;

	k_mutex_unlock(&data->ept_mutex);

	/* Announce endpoint to remote */
	ret = send_string(data, SERIAL_CMD_DISC_REQ, ept->local_id, cfg->name);
	if (ret < 0) {
		k_mutex_lock(&data->ept_mutex, K_FOREVER);
		ept->cfg = NULL;
		*token = NULL;
		k_mutex_unlock(&data->ept_mutex);

		LOG_ERR("Failed to send discovery for endpoint '%s': %d", cfg->name, ret);
		return ret;
	}

	if (serial_cfg->timeout_ms > 0) {
		/* Wait for remote to respond (bound callback will fire asynchronously) */
		uint32_t events = k_event_wait(&data->bind_event, BIT(ept->local_id - 1), true,
					       K_MSEC(serial_cfg->timeout_ms));

		if (events == 0) {
			LOG_WRN("Endpoint '%s' bind timed out after %u ms", cfg->name,
				serial_cfg->timeout_ms);
			/* Don't fail - the remote might come up later and bind will happen then.
			 * The endpoint remains registered.
			 */
		}
	}

	return 0;
}

static int serial_deregister_ept(const struct device *instance, void *token)
{
	struct serial_data *data = instance->data;
	struct serial_ept *ept = token;
	const struct ipc_ept_cfg *cfg;
	bool was_bound;
	uint8_t local_id;

	k_mutex_lock(&data->ept_mutex, K_FOREVER);
	cfg = ept->cfg;
	was_bound = ept->bound;
	local_id = ept->local_id;
	ept->cfg = NULL;
	ept->remote_id = 0;
	ept->bound = false;
	k_mutex_unlock(&data->ept_mutex);

	if (was_bound && cfg != NULL) {
		int ret;

		ret = send_string(data, SERIAL_CMD_DEREGISTER, local_id, NULL);
		if (ret < 0) {
			LOG_ERR("Error on send de-register (%d)", ret);
		}
		if (cfg->cb.unbound != NULL) {
			cfg->cb.unbound(cfg->priv);
		}
	}

	return 0;
}

static int serial_send(const struct device *instance, void *token, const void *msg, size_t len)
{
	struct serial_data *data = instance->data;
	struct serial_ept *ept = token;
	int ret;

	if (!ept->bound || ept->remote_id == 0) {
		return -ENOTCONN;
	}

	/* DATA frame overhead: cmd_id (1) + ept_id (1) = 2 bytes */
	if (data->remote_max_rx > 0 && len + 2 > data->remote_max_rx) {
		LOG_WRN("Payload (%zu bytes + 2) exceeds remote RX capacity (%u bytes)", len,
			data->remote_max_rx);
		return -EMSGSIZE;
	}

	const uint8_t hdr[] = {SERIAL_CMD_DATA, ept->local_id};

	k_mutex_lock(&data->tx_mutex, K_FOREVER);
	ret = send_frame(data, hdr, sizeof(hdr), msg, len);
	k_mutex_unlock(&data->tx_mutex);

	if (ret < 0) {
		return ret;
	}

	return len;
}

static const struct ipc_service_backend serial_backend_ops = {
	.open_instance = serial_open,
	.close_instance = serial_close,
	.register_endpoint = serial_register_ept,
	.deregister_endpoint = serial_deregister_ept,
	.send = serial_send,
};

static int serial_backend_init(const struct device *instance)
{
	struct serial_data *data = instance->data;

	data->self = instance;
	k_mutex_init(&data->ept_mutex);
	k_mutex_init(&data->tx_mutex);
	k_work_init(&data->rx_work, rx_work_handler);
	k_event_init(&data->bind_event);
	data->opened = false;

	/* Fixed 1-based local IDs */
	for (int i = 0; i < NUM_EP; i++) {
		data->epts[i].local_id = i + 1;
	}

	return 0;
}

#define DEFINE_SERIAL_BACKEND(inst)                                                                \
	static const struct serial_config serial_config_##inst = {                                 \
		.uart_dev = DEVICE_DT_GET(DT_INST_BUS(inst)),                                      \
		.timeout_ms = DT_INST_PROP(inst, timeout_ms),                                      \
	};                                                                                         \
                                                                                                   \
	static struct serial_data serial_data_##inst;                                              \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &serial_backend_init, NULL, &serial_data_##inst,               \
			      &serial_config_##inst, POST_KERNEL,                                  \
			      CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY, &serial_backend_ops);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_SERIAL_BACKEND)
