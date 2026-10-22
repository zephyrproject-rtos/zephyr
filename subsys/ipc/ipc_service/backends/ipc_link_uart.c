/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * IPC link transport over a UART: each frame is COBS-encoded and ends with a 0x00 delimiter, so
 * the receiver finds the next frame after any corruption. Frames go each way on their own.
 */

#define DT_DRV_COMPAT zephyr_ipc_uart

#include <errno.h>
#include <string.h>

#include <zephyr/data/cobs.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/ipc/ipc_link.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "ipc_link_priv.h"

LOG_MODULE_REGISTER(ipc_link_uart, CONFIG_IPC_SERVICE_BACKEND_LINK_LOG_LEVEL);

/* The asynchronous API receives into two buffers, handed over when full or after an idle time */
#define IPC_LINK_UART_RX_CHUNKS          2
#define IPC_LINK_UART_RX_CHUNK_SIZE      64
#define IPC_LINK_UART_RX_IDLE_TIMEOUT_US 1000

/* COBS adds a byte per 254 and the delimiter: cobs_max_encoded_len(), checked at initialization */
#define IPC_LINK_UART_ENCODED_MAX(mtu) ((mtu) + (mtu) / 254 + 2)

/* Receive buffers of the asynchronous API */
struct ipc_link_uart_rx_bufs {
	uint8_t chunks[IPC_LINK_UART_RX_CHUNKS][IPC_LINK_UART_RX_CHUNK_SIZE];
};

struct ipc_link_uart_config {
	struct ipc_link_config link;
	const struct device *uart;
};

struct ipc_link_uart_data {
	const struct device *dev;
	struct k_work tx_work;
	struct cobs_encoder encoder;
	struct cobs_decoder decoder;

	/* The encoded frame being sent */
	uint8_t *tx_buf;
	size_t tx_size;
	size_t tx_len;
	size_t tx_sent;
	atomic_t tx_busy;
	/* The interrupt handler sends the encoded frame: not while it is being encoded */
	bool tx_sending;

	/* The frame being received, decoded into the link's receive buffer */
	uint8_t *rx_frame;
	size_t rx_len;
	bool rx_overflow;

	/* The asynchronous API is used, rather than the interrupt-driven one */
	bool async;
	/* Receive buffers of the asynchronous API, NULL without it */
	struct ipc_link_uart_rx_bufs *rx_bufs;
	uint8_t rx_chunk_used;
};

static int ipc_link_uart_encoded(const uint8_t *buf, size_t len, void *user_data)
{
	struct ipc_link_uart_data *data = user_data;

	if (data->tx_len + len > data->tx_size) {
		return -ENOMEM;
	}

	memcpy(&data->tx_buf[data->tx_len], buf, len);
	data->tx_len += len;

	return 0;
}

static int ipc_link_uart_decoded(const uint8_t *buf, size_t len, void *user_data)
{
	struct ipc_link_uart_data *data = user_data;
	const struct ipc_link_uart_config *cfg = data->dev->config;

	if (buf == NULL) {
		/* End of frame */
		if (data->rx_overflow) {
			(void)ipc_link_rx(data->dev, 0, false);
		} else if (data->rx_len > 0) {
			(void)ipc_link_rx(data->dev, data->rx_len, true);
		} else {
			/* A delimiter that ends nothing */
		}

		data->rx_frame = NULL;
		data->rx_len = 0;
		data->rx_overflow = false;

		return 0;
	}

	if (data->rx_overflow) {
		return 0;
	}

	if (data->rx_frame == NULL) {
		data->rx_frame = ipc_link_rx_buf(data->dev);
	}

	if (data->rx_len + len > cfg->link.mtu) {
		data->rx_overflow = true;
		return 0;
	}

	memcpy(&data->rx_frame[data->rx_len], buf, len);
	data->rx_len += len;

	return 0;
}

/* One delimited frame at a time: a corrupted one is dropped, the ones after it are decoded */
static void ipc_link_uart_feed(struct ipc_link_uart_data *data, const uint8_t *buf, size_t len)
{
	while (len > 0) {
		const uint8_t *end = memchr(buf, 0x00, len);
		size_t seg = end != NULL ? (size_t)(end - buf) + 1U : len;

		/* The decoder only fails at the delimiter, and resets itself */
		if (cobs_decoder_write(&data->decoder, buf, seg) < 0) {
			data->rx_frame = NULL;
			data->rx_len = 0;
			data->rx_overflow = false;
			(void)ipc_link_rx(data->dev, 0, false);
		}

		buf += seg;
		len -= seg;
	}
}

static void ipc_link_uart_tx_done(struct ipc_link_uart_data *data)
{
	atomic_clear(&data->tx_busy);
	k_work_submit(&data->tx_work);
}

static uint8_t *ipc_link_uart_rx_chunk(struct ipc_link_uart_data *data)
{
	for (int i = 0; i < IPC_LINK_UART_RX_CHUNKS; i++) {
		if ((data->rx_chunk_used & BIT(i)) == 0U) {
			data->rx_chunk_used |= BIT(i);
			return data->rx_bufs->chunks[i];
		}
	}

	return NULL;
}

static void ipc_link_uart_rx_chunk_free(struct ipc_link_uart_data *data, const uint8_t *buf)
{
	for (int i = 0; i < IPC_LINK_UART_RX_CHUNKS; i++) {
		if (buf == data->rx_bufs->chunks[i]) {
			data->rx_chunk_used &= ~BIT(i);
		}
	}
}

static int ipc_link_uart_rx_start(struct ipc_link_uart_data *data)
{
	const struct ipc_link_uart_config *cfg = data->dev->config;
	uint8_t *chunk = ipc_link_uart_rx_chunk(data);
	int ret;

	if (chunk == NULL) {
		return -ENOMEM;
	}

	ret = uart_rx_enable(cfg->uart, chunk, IPC_LINK_UART_RX_CHUNK_SIZE,
			     IPC_LINK_UART_RX_IDLE_TIMEOUT_US);
	if (ret < 0) {
		ipc_link_uart_rx_chunk_free(data, chunk);
	}

	return ret;
}

static void ipc_link_uart_async_cb(const struct device *uart, struct uart_event *evt,
				   void *user_data)
{
	struct ipc_link_uart_data *data = user_data;
	uint8_t *chunk;

	switch (evt->type) {
	case UART_TX_DONE:
	case UART_TX_ABORTED:
		ipc_link_uart_tx_done(data);
		break;
	case UART_RX_RDY:
		ipc_link_uart_feed(data, &evt->data.rx.buf[evt->data.rx.offset], evt->data.rx.len);
		break;
	case UART_RX_BUF_REQUEST:
		chunk = ipc_link_uart_rx_chunk(data);
		if (chunk != NULL) {
			if (uart_rx_buf_rsp(uart, chunk, IPC_LINK_UART_RX_CHUNK_SIZE) < 0) {
				ipc_link_uart_rx_chunk_free(data, chunk);
			}
		}
		break;
	case UART_RX_BUF_RELEASED:
		ipc_link_uart_rx_chunk_free(data, evt->data.rx_buf.buf);
		break;
	case UART_RX_DISABLED:
		if (ipc_link_uart_rx_start(data) < 0) {
			LOG_ERR("Failed to restart reception");
		}
		break;
	case UART_RX_STOPPED:
		LOG_DBG("Reception stopped (%d)", evt->data.rx_stop.reason);
		break;
	default:
		break;
	}
}

static void ipc_link_uart_isr(const struct device *uart, void *user_data)
{
	struct ipc_link_uart_data *data = user_data;
	uint8_t buf[32];
	int len;

	uart_irq_update(uart);

	while (uart_irq_rx_ready(uart) > 0) {
		len = uart_fifo_read(uart, buf, sizeof(buf));
		if (len <= 0) {
			break;
		}

		ipc_link_uart_feed(data, buf, len);
	}

	if (data->tx_sending && uart_irq_tx_ready(uart) > 0) {
		if (data->tx_sent < data->tx_len) {
			len = uart_fifo_fill(uart, &data->tx_buf[data->tx_sent],
					     data->tx_len - data->tx_sent);
			if (len > 0) {
				data->tx_sent += len;
			}
		}

		/* Copied into the FIFO, the encoded frame is done with: not every driver reports
		 * the completion of the transmission itself
		 */
		if (data->tx_sent >= data->tx_len) {
			data->tx_sending = false;
			uart_irq_tx_disable(uart);
			ipc_link_uart_tx_done(data);
		}
	}
}

static int ipc_link_uart_tx_start(struct ipc_link_uart_data *data)
{
	const struct ipc_link_uart_config *cfg = data->dev->config;

	if (data->async) {
		/* With flow control, a peer that stops taking bytes stops the transmission: it is
		 * given up after the retransmission timeout, by when the link sends again anyway,
		 * so that the retransmissions go on and a silent peer still ends the session
		 */
		return uart_tx(cfg->uart, data->tx_buf, data->tx_len,
			       cfg->link.retransmit_timeout_ms * USEC_PER_MSEC);
	}

	data->tx_sent = 0;
	data->tx_sending = true;
	uart_irq_tx_enable(cfg->uart);

	return 0;
}

/* The asynchronous API if the UART has it, the interrupt-driven API otherwise */
static int ipc_link_uart_io_open(struct ipc_link_uart_data *data)
{
	const struct ipc_link_uart_config *cfg = data->dev->config;
	int ret = -ENOTSUP;

	if (IS_ENABLED(CONFIG_UART_ASYNC_API)) {
		ret = uart_callback_set(cfg->uart, ipc_link_uart_async_cb, data);
		if (ret == 0) {
			data->async = true;
			data->rx_chunk_used = 0U;

			return ipc_link_uart_rx_start(data);
		}
	}

	if (IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN) && (ret == -ENOSYS || ret == -ENOTSUP)) {
		ret = uart_irq_callback_user_data_set(cfg->uart, ipc_link_uart_isr, data);
		if (ret == 0) {
			data->async = false;
			uart_irq_rx_enable(cfg->uart);
		}
	}

	return ret;
}

static void ipc_link_uart_tx_work(struct k_work *work)
{
	struct ipc_link_uart_data *data = CONTAINER_OF(work, struct ipc_link_uart_data, tx_work);
	const uint8_t *frame;
	size_t len;
	int ret;

	if (!ipc_link_pending(data->dev)) {
		return;
	}

	if (atomic_set(&data->tx_busy, 1) != 0) {
		/* The transmission under way submits this work again when done */
		return;
	}

	frame = ipc_link_tx_prepare(data->dev, &len);

	data->tx_len = 0;
	ret = cobs_encoder_write(&data->encoder, frame, len);
	if (ret >= 0) {
		ret = cobs_encoder_close(&data->encoder);
	}

	if (ret >= 0) {
		ret = ipc_link_uart_tx_start(data);
	}

	if (ret < 0) {
		LOG_ERR("Failed to send a frame (%d)", ret);
		ipc_link_uart_tx_done(data);
	}
}

static int ipc_link_uart_open(const struct device *dev)
{
	struct ipc_link_uart_data *data = dev->data;
	int ret;

	data->rx_frame = NULL;
	data->rx_len = 0;
	data->rx_overflow = false;
	(void)cobs_decoder_init(&data->decoder, ipc_link_uart_decoded, data,
				COBS_FLAG_TRAILING_DELIMITER);

	ret = ipc_link_uart_io_open(data);
	if (ret < 0) {
		LOG_ERR("Failed to open the UART (%d)", ret);
	}

	return ret;
}

static void ipc_link_uart_kick(const struct device *dev)
{
	struct ipc_link_uart_data *data = dev->data;

	k_work_submit(&data->tx_work);
}

static const struct ipc_link_transport_api ipc_link_uart_api = {
	.open = ipc_link_uart_open,
	.kick = ipc_link_uart_kick,
};

static int ipc_link_uart_init(const struct device *dev)
{
	const struct ipc_link_uart_config *cfg = dev->config;
	struct ipc_link_uart_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->uart)) {
		LOG_ERR("UART not ready");
		return -ENODEV;
	}

	if (cobs_max_encoded_len(cfg->link.mtu, COBS_FLAG_TRAILING_DELIMITER) > data->tx_size) {
		LOG_ERR("Encoded frame buffer too small");
		return -EINVAL;
	}

	data->dev = dev;
	k_work_init(&data->tx_work, ipc_link_uart_tx_work);

	ret = cobs_encoder_init(&data->encoder, ipc_link_uart_encoded, data,
				COBS_FLAG_TRAILING_DELIMITER);
	if (ret < 0) {
		return ret;
	}

	return ipc_link_init(dev);
}

/* Receive buffers of an instance, for the asynchronous API only */
#define IPC_LINK_UART_RX_BUFS_DEFINE(inst)                                                         \
	IF_ENABLED(CONFIG_UART_ASYNC_API,                                                          \
		   (static struct ipc_link_uart_rx_bufs ipc_link_uart_##inst##_rx_bufs;))

#define IPC_LINK_UART_RX_BUFS(inst)                                                                \
	COND_CODE_1(CONFIG_UART_ASYNC_API, (&ipc_link_uart_##inst##_rx_bufs), (NULL))

#define IPC_LINK_UART_DEFINE(inst)                                                                 \
	IPC_LINK_DEFINE(ipc_link_uart_##inst##_link, DT_INST_PROP(inst, mtu));                     \
                                                                                                   \
	static uint8_t                                                                             \
		ipc_link_uart_##inst##_tx_buf[IPC_LINK_UART_ENCODED_MAX(DT_INST_PROP(inst, mtu))]; \
	IPC_LINK_UART_RX_BUFS_DEFINE(inst)                                                         \
                                                                                                   \
	static struct ipc_link_uart_data ipc_link_uart_##inst##_data = {                           \
		.tx_buf = ipc_link_uart_##inst##_tx_buf,                                           \
		.tx_size = sizeof(ipc_link_uart_##inst##_tx_buf),                                  \
		.rx_bufs = IPC_LINK_UART_RX_BUFS(inst),                                            \
	};                                                                                         \
                                                                                                   \
	static const struct ipc_link_uart_config ipc_link_uart_##inst##_config = {                 \
		.link =                                                                            \
			{                                                                          \
				.api = &ipc_link_uart_api,                                         \
				IPC_LINK_CONFIG(ipc_link_uart_##inst##_link),                      \
				.mtu = DT_INST_PROP(inst, mtu),                                    \
				.retransmit_timeout_ms =                                           \
					DT_INST_PROP(inst, retransmit_timeout_ms),                 \
			},                                                                         \
		.uart = DEVICE_DT_GET(DT_INST_BUS(inst)),                                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, ipc_link_uart_init, NULL, &ipc_link_uart_##inst##_data,        \
			      &ipc_link_uart_##inst##_config, POST_KERNEL,                         \
			      CONFIG_IPC_SERVICE_BACKEND_LINK_INIT_PRIORITY,                       \
			      &ipc_link_backend_ops);

DT_INST_FOREACH_STATUS_OKAY(IPC_LINK_UART_DEFINE)
