/*
 * Copyright (c) 2023 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/ec_host_cmd/backend.h>
#include <zephyr/mgmt/ec_host_cmd/ec_host_cmd.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

LOG_MODULE_REGISTER(host_cmd_uart, CONFIG_EC_HC_LOG_LEVEL);

/* TODO: Try to use circular mode once it is supported and compare timings */

enum uart_host_command_state {
	/*
	 * UART host command handler not enabled.
	 */
	UART_HOST_CMD_STATE_DISABLED,

	/*
	 * This state represents UART layer is initialized and ready to
	 * receive host request. Once the response is sent, the current state is
	 * reset to this state to accept next packet.
	 */
	UART_HOST_CMD_READY_TO_RX,

	/*
	 * After first byte is received the current state is moved to receiving
	 * state until all the header bytes + datalen bytes are received.
	 * If host_request_timeout was called in this state, it would be
	 * because of an underrun situation.
	 */
	UART_HOST_CMD_RECEIVING,

	/*
	 * Once the process_request starts processing the rx buffer,
	 * the current state is moved to processing state. Host should not send
	 * any bytes in this state as it would be considered contiguous
	 * request.
	 */
	UART_HOST_CMD_PROCESSING,

	/*
	 * Once host task is ready with the response bytes, the current state is
	 * moved to sending state.
	 */
	UART_HOST_CMD_SENDING,

	/*
	 * If bad packet header is received, the current state is moved to rx_bad
	 * state and after timeout all the bytes are dropped.
	 */
	UART_HOST_CMD_RX_BAD,

	/*
	 * If extra bytes are received when the host command is being processed,
	 * host is sending extra bytes which indicates data overrun.
	 */
	UART_HOST_CMD_RX_OVERRUN,
};

static const char * const state_name[] = {
	[UART_HOST_CMD_STATE_DISABLED] = "DISABLED",
	[UART_HOST_CMD_READY_TO_RX] = "READY_TO_RX",
	[UART_HOST_CMD_RECEIVING] = "RECEIVING",
	[UART_HOST_CMD_PROCESSING] = "PROCESSING",
	[UART_HOST_CMD_SENDING] = "SENDING",
	[UART_HOST_CMD_RX_BAD] = "RX_BAD",
	[UART_HOST_CMD_RX_OVERRUN] = "RX_OVERRUN",
};

struct ec_host_cmd_uart_ctx {
	const struct device *uart_dev;
	struct ec_host_cmd_rx_ctx *rx_ctx;
	const size_t rx_buf_size;
	struct ec_host_cmd_tx_buf *tx_buf;
	struct k_work_delayable timeout_work;
	enum uart_host_command_state state;
};

static int request_expected_size(const struct ec_host_cmd_request_header *r)
{
	/* Check host request version */
	if (r->prtcl_ver != 3) {
		return 0;
	}

	/* Reserved byte should be 0 */
	if (r->reserved) {
		return 0;
	}

	return sizeof(*r) + r->data_len;
}

#define EC_HOST_CMD_UART_DEFINE(_name)                                                             \
	static struct ec_host_cmd_uart_ctx _name##_hc_uart = {                                     \
		.rx_buf_size = CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_SIZE,                          \
	};                                                                                         \
	static struct ec_host_cmd_backend _name = {                                                \
		.api = &ec_host_cmd_api,                                                           \
		.ctx = &_name##_hc_uart,                                                           \
	}

/*
 * Max data size for a version 3 request/response packet. This is big enough
 * to handle a request/response header, flash write offset/size and 512 bytes
 * of request payload or 224 bytes of response payload.
 */
#define UART_MAX_REQ_SIZE  0x220
#define UART_MAX_RESP_SIZE 0x100

static void rx_timeout(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ec_host_cmd_uart_ctx *hc_uart =
		CONTAINER_OF(dwork, struct ec_host_cmd_uart_ctx, timeout_work);

	LOG_ERR("Request error in state: %s", state_name[hc_uart->state]);

	uart_rx_disable(hc_uart->uart_dev);
	uart_rx_enable(hc_uart->uart_dev, hc_uart->rx_ctx->buf, hc_uart->rx_buf_size, 0);

	hc_uart->state = UART_HOST_CMD_READY_TO_RX;
}

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	struct ec_host_cmd_uart_ctx *hc_uart = user_data;
	size_t new_len;

	switch (evt->type) {
	case UART_RX_RDY:
		if (hc_uart->state == UART_HOST_CMD_READY_TO_RX) {
			hc_uart->rx_ctx->len = 0;
			hc_uart->state = UART_HOST_CMD_RECEIVING;
			k_work_reschedule(&hc_uart->timeout_work,
					  K_MSEC(CONFIG_EC_HOST_CMD_BACKEND_UART_TIMEOUT));
		} else if (hc_uart->state == UART_HOST_CMD_PROCESSING ||
			   hc_uart->state == UART_HOST_CMD_SENDING) {
			LOG_ERR("Received data while in state: %s", state_name[hc_uart->state]);
			return;
		} else if (hc_uart->state == UART_HOST_CMD_RX_BAD ||
			   hc_uart->state == UART_HOST_CMD_RX_OVERRUN) {
			/* Wait for timeout if an error has been detected */
			return;
		}

		__ASSERT(hc_uart->state == UART_HOST_CMD_RECEIVING,
			 "UART Host Command state mishandled, state: %d", hc_uart->state);

		new_len = hc_uart->rx_ctx->len + evt->data.rx.len;

		if (new_len > hc_uart->rx_buf_size) {
			/* Bad data error, set the state and wait for timeout */
			hc_uart->state = UART_HOST_CMD_RX_BAD;
			return;
		}

		hc_uart->rx_ctx->len = new_len;

		if (hc_uart->rx_ctx->len >= sizeof(struct ec_host_cmd_request_header)) {
			/* Buffer has request header. Check header and get data_len */
			size_t expected_len = request_expected_size(
				(struct ec_host_cmd_request_header *)hc_uart->rx_ctx->buf);

			if (expected_len == 0 || expected_len > hc_uart->rx_buf_size) {
				/* Invalid expected size, set the state and wait for timeout */
				hc_uart->state = UART_HOST_CMD_RX_BAD;
			} else if (hc_uart->rx_ctx->len == expected_len) {
				/* Don't wait for overrun, because it is already done
				 * in a UART driver.
				 */
				(void)k_work_cancel_delayable(&hc_uart->timeout_work);

				/* Disable receiving to prevent overwriting the rx buffer while
				 * processing. Enabling receiving to a temporary buffer to detect
				 * unexpected transfer while processing increases average handling
				 * time ~40% so don't do that.
				 */
				uart_rx_disable(hc_uart->uart_dev);

				/* If no data more in request, packet is complete. Start processing
				 */
				hc_uart->state = UART_HOST_CMD_PROCESSING;

				ec_host_cmd_rx_notify();
			} else if (hc_uart->rx_ctx->len > expected_len) {
				/* Overrun error, set the state and wait for timeout */
				hc_uart->state = UART_HOST_CMD_RX_OVERRUN;
			}
		}
		break;
	case UART_RX_BUF_REQUEST:
		/* Do not provide the second buffer, because we reload DMA after every packet. */
		break;
	case UART_TX_DONE:
		if (hc_uart->state != UART_HOST_CMD_SENDING) {
			LOG_ERR("Unexpected end of sending");
		}
		/* Receiving is already enabled in the send function. */
		hc_uart->state = UART_HOST_CMD_READY_TO_RX;
		break;
	case UART_RX_STOPPED:
		LOG_ERR("Receiving data stopped");
		break;
	default:
		break;
	}
}

static int ec_host_cmd_uart_init(const struct ec_host_cmd_backend *backend,
				 struct ec_host_cmd_rx_ctx *rx_ctx, struct ec_host_cmd_tx_buf *tx)
{
	int ret;
	struct ec_host_cmd_uart_ctx *hc_uart = backend->ctx;

	hc_uart->state = UART_HOST_CMD_STATE_DISABLED;

	if (!device_is_ready(hc_uart->uart_dev)) {
		return -ENODEV;
	}

	/* UART backend needs rx and tx buffers provided by the handler */
	if (!rx_ctx->buf || !tx->buf) {
		return -EIO;
	}

	hc_uart->rx_ctx = rx_ctx;
	hc_uart->tx_buf = tx;

	/* Limit the requset/response max sizes */
	if (hc_uart->rx_ctx->len_max > UART_MAX_REQ_SIZE) {
		hc_uart->rx_ctx->len_max = UART_MAX_REQ_SIZE;
	}
	if (hc_uart->tx_buf->len_max > UART_MAX_RESP_SIZE) {
		hc_uart->tx_buf->len_max = UART_MAX_RESP_SIZE;
	}

	k_work_init_delayable(&hc_uart->timeout_work, rx_timeout);
	uart_callback_set(hc_uart->uart_dev, uart_callback, hc_uart);
	ret = uart_rx_enable(hc_uart->uart_dev, hc_uart->rx_ctx->buf, hc_uart->rx_buf_size, 0);

	hc_uart->state = UART_HOST_CMD_READY_TO_RX;

	return ret;
}

static int ec_host_cmd_uart_send(const struct ec_host_cmd_backend *backend)
{
	struct ec_host_cmd_uart_ctx *hc_uart = backend->ctx;
	int ret;

	if (hc_uart->state != UART_HOST_CMD_PROCESSING) {
		LOG_ERR("Unexpected state while sending");
	}

	/* The state is changed to UART_HOST_CMD_READY_TO_RX in the UART_TX_DONE event */
	hc_uart->state = UART_HOST_CMD_SENDING;

	/* The rx buffer is no longer in use by command handler.
	 * Enable receiving to be ready to get a new command right after sending the response.
	 */
	uart_rx_enable(hc_uart->uart_dev, hc_uart->rx_ctx->buf, hc_uart->rx_buf_size, 0);

	/* uart_tx is non-blocking asynchronous function.
	 * The state is changed to UART_HOST_CMD_READY_TO_RX in the UART_TX_DONE event.
	 */
	ret = uart_tx(hc_uart->uart_dev, hc_uart->tx_buf->buf, hc_uart->tx_buf->len,
		      SYS_FOREVER_US);

	/* If sending fails, reset the state */
	if (ret) {
		hc_uart->state = UART_HOST_CMD_READY_TO_RX;
		LOG_ERR("Sending failed");
	}

	return ret;
}

static const struct ec_host_cmd_backend_api ec_host_cmd_api = {
	.init = ec_host_cmd_uart_init,
	.send = ec_host_cmd_uart_send,
};

EC_HOST_CMD_UART_DEFINE(ec_host_cmd_uart);
struct ec_host_cmd_backend *ec_host_cmd_backend_get_uart(const struct device *dev)
{
	struct ec_host_cmd_uart_ctx *hc_uart = ec_host_cmd_uart.ctx;

	hc_uart->uart_dev = dev;
	return &ec_host_cmd_uart;
}

#if DT_NODE_EXISTS(DT_CHOSEN(zephyr_host_cmd_uart_backend)) &&                                     \
	defined(CONFIG_EC_HOST_CMD_INITIALIZE_AT_BOOT)
static int host_cmd_init(void)
{
	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_host_cmd_uart_backend));

	ec_host_cmd_init(ec_host_cmd_backend_get_uart(dev));
	return 0;
}
SYS_INIT(host_cmd_init, POST_KERNEL, CONFIG_EC_HOST_CMD_INIT_PRIORITY);
#endif
