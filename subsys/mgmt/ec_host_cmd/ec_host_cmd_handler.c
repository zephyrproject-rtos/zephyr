/*
 * Copyright (c) 2020 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/ec_host_cmd/ec_host_cmd.h>
#include <zephyr/mgmt/ec_host_cmd/backend.h>
#include <string.h>

LOG_MODULE_REGISTER(host_cmd_handler, CONFIG_EC_HC_LOG_LEVEL);

#define EC_HOST_CMD_CHOSEN_BACKEND_LIST                                                            \
	zephyr_host_cmd_espi_backend, zephyr_host_cmd_shi_backend, zephyr_host_cmd_uart_backend

#define EC_HOST_CMD_ADD_CHOSEN(chosen) COND_CODE_1(DT_NODE_EXISTS(DT_CHOSEN(chosen)), (1), (0))

#define NUMBER_OF_CHOSEN_BACKENDS                                                                  \
	FOR_EACH(EC_HOST_CMD_ADD_CHOSEN, (+), EC_HOST_CMD_CHOSEN_BACKEND_LIST)                     \
	+0

BUILD_ASSERT(NUMBER_OF_CHOSEN_BACKENDS < 2, "Number of chosen backends > 1");

#define RX_HEADER_SIZE (sizeof(struct ec_host_cmd_request_header))
#define TX_HEADER_SIZE (sizeof(struct ec_host_cmd_response_header))

#define EC_HOST_CMD_DEFINE(_name)                                                                  \
	COND_CODE_1(                                                                               \
		CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_DEF,                                          \
		(static uint8_t _name##_rx_buffer[CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_SIZE];),    \
		())                                                                                \
	COND_CODE_1(                                                                               \
		CONFIG_EC_HOST_CMD_HANDLER_TX_BUFFER_DEF,                                          \
		(static uint8_t _name##_tx_buffer[CONFIG_EC_HOST_CMD_HANDLER_TX_BUFFER_SIZE];),    \
		())                                                                                \
	static K_KERNEL_STACK_DEFINE(_name##stack, CONFIG_EC_HOST_CMD_HANDLER_STACK_SIZE);         \
	static struct k_thread _name##thread;                                                      \
	static struct ec_host_cmd _name = {                                                        \
		.rx_ctx =                                                                          \
			{                                                                          \
				.buf = COND_CODE_1(CONFIG_EC_HOST_CMD_HANDLER_RX_BUFFER_DEF,       \
						   (_name##_rx_buffer), (NULL)),                   \
			},                                                                         \
		.tx =                                                                              \
			{                                                                          \
				.buf = COND_CODE_1(CONFIG_EC_HOST_CMD_HANDLER_TX_BUFFER_DEF,       \
						   (_name##_tx_buffer), (NULL)),                   \
				.len_max = COND_CODE_1(                                            \
					CONFIG_EC_HOST_CMD_HANDLER_TX_BUFFER_DEF,                  \
					(CONFIG_EC_HOST_CMD_HANDLER_TX_BUFFER_SIZE), (0)),         \
			},                                                                         \
		.thread = &_name##thread,                                                          \
		.stack = _name##stack,                                                             \
	}

EC_HOST_CMD_DEFINE(ec_host_cmd);

static uint8_t cal_checksum(const uint8_t *const buffer, const uint16_t size)
{
	uint8_t checksum = 0;

	for (size_t i = 0; i < size; ++i) {
		checksum += buffer[i];
	}
	return (uint8_t)(-checksum);
}

static void send_error_response(const struct ec_host_cmd_backend *backend,
				struct ec_host_cmd_tx_buf *tx, const enum ec_host_cmd_status error)
{
	struct ec_host_cmd_response_header *const tx_header = (void *)tx->buf;

	tx_header->prtcl_ver = 3;
	tx_header->result = error;
	tx_header->data_len = 0;
	tx_header->reserved = 0;
	tx_header->checksum = 0;
	tx_header->checksum = cal_checksum((uint8_t *)tx_header, TX_HEADER_SIZE);

	tx->len = TX_HEADER_SIZE;

	backend->api->send(backend);
}

static enum ec_host_cmd_status verify_rx(struct ec_host_cmd_rx_ctx *rx)
{
	/* rx buf and len now have valid incoming data */
	if (rx->len < RX_HEADER_SIZE) {
		return EC_HOST_CMD_REQUEST_TRUNCATED;
	}

	const struct ec_host_cmd_request_header *rx_header =
		(struct ec_host_cmd_request_header *)rx->buf;

	/* Only support version 3 */
	if (rx_header->prtcl_ver != 3) {
		return EC_HOST_CMD_INVALID_HEADER;
	}

	const uint16_t rx_valid_data_size = rx_header->data_len + RX_HEADER_SIZE;
	/*
	 * Ensure we received at least as much data as is expected.
	 * It is okay to receive more since some hardware interfaces
	 * add on extra padding bytes at the end.
	 */
	if (rx->len < rx_valid_data_size) {
		return EC_HOST_CMD_REQUEST_TRUNCATED;
	}

	/* Validate checksum */
	if (cal_checksum((uint8_t *)rx_header, rx_valid_data_size) != 0) {
		return EC_HOST_CMD_INVALID_CHECKSUM;
	}

	return EC_HOST_CMD_SUCCESS;
}

static enum ec_host_cmd_status validate_handler(const struct ec_host_cmd_handler *handler,
						const struct ec_host_cmd_handler_args *args)
{
	if (handler->min_rqt_size > args->input_buf_size) {
		return EC_HOST_CMD_REQUEST_TRUNCATED;
	}

	if (handler->min_rsp_size > args->output_buf_max) {
		return EC_HOST_CMD_INVALID_RESPONSE;
	}

	if (args->version > sizeof(handler->version_mask) ||
	    !(handler->version_mask & BIT(args->version))) {
		return EC_HOST_CMD_INVALID_VERSION;
	}

	return EC_HOST_CMD_SUCCESS;
}

static enum ec_host_cmd_status prepare_response(struct ec_host_cmd_tx_buf *tx, uint16_t len)
{
	struct ec_host_cmd_response_header *const tx_header = (void *)tx->buf;

	tx_header->prtcl_ver = 3;
	tx_header->result = EC_HOST_CMD_SUCCESS;
	tx_header->data_len = len;
	tx_header->reserved = 0;

	const uint16_t tx_valid_data_size = tx_header->data_len + TX_HEADER_SIZE;

	if (tx_valid_data_size > tx->len_max) {
		return EC_HOST_CMD_INVALID_RESPONSE;
	}

	/* Calculate checksum */
	tx_header->checksum = 0;
	tx_header->checksum = cal_checksum(tx->buf, tx_valid_data_size);

	tx->len = tx_valid_data_size;

	return EC_HOST_CMD_SUCCESS;
}

static void ec_host_cmd_thread(void *hc_handle, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	enum ec_host_cmd_status status;
	struct ec_host_cmd *hc = (struct ec_host_cmd *)hc_handle;
	struct ec_host_cmd_rx_ctx *rx = &hc->rx_ctx;
	struct ec_host_cmd_tx_buf *tx = &hc->tx;
	const struct ec_host_cmd_handler *found_handler;
	const struct ec_host_cmd_request_header *const rx_header = (void *)rx->buf;
	/* The pointer to rx buffer is constant during communication */
	struct ec_host_cmd_handler_args args = {
		.output_buf = (uint8_t *)tx->buf + TX_HEADER_SIZE,
		.output_buf_max = tx->len_max - TX_HEADER_SIZE,
		.input_buf = rx->buf + RX_HEADER_SIZE,
		.reserved = NULL,
	};

	while (1) {
		/* Wait until RX messages is received on host interface */
		k_sem_take(&rx->handler_owns, K_FOREVER);

		status = verify_rx(rx);
		if (status != EC_HOST_CMD_SUCCESS) {
			send_error_response(hc->backend, tx, status);
			continue;
		}

		found_handler = NULL;
		STRUCT_SECTION_FOREACH(ec_host_cmd_handler, handler)
		{
			if (handler->id == rx_header->cmd_id) {
				found_handler = handler;
				break;
			}
		}

		/* No handler in this image for requested command */
		if (found_handler == NULL) {
			send_error_response(hc->backend, tx, EC_HOST_CMD_INVALID_COMMAND);
			continue;
		}

		args.command = rx_header->cmd_id;
		args.version = rx_header->cmd_ver;
		args.input_buf_size = rx_header->data_len;
		args.output_buf_size = 0;

		status = validate_handler(found_handler, &args);
		if (status != EC_HOST_CMD_SUCCESS) {
			send_error_response(hc->backend, tx, status);
			continue;
		}

		status = found_handler->handler(&args);
		if (status != EC_HOST_CMD_SUCCESS) {
			send_error_response(hc->backend, tx, status);
			continue;
		}

		status = prepare_response(tx, args.output_buf_size);
		if (status != EC_HOST_CMD_SUCCESS) {
			send_error_response(hc->backend, tx, status);
			continue;
		}

		hc->backend->api->send(hc->backend);
	}
}

int ec_host_cmd_init(struct ec_host_cmd_backend *backend)
{
	struct ec_host_cmd *hc = &ec_host_cmd;
	int ret;
	uint8_t *handler_tx_buf, *handler_rx_buf;

	hc->backend = backend;

	/* Allow writing to rx buff at startup */
	k_sem_init(&hc->rx_ctx.handler_owns, 0, 1);

	handler_tx_buf = hc->tx.buf;
	handler_rx_buf = hc->rx_ctx.buf;

	ret = backend->api->init(backend, &hc->rx_ctx, &hc->tx);

	if (ret != 0) {
		return ret;
	}

	if (!hc->tx.buf | !hc->rx_ctx.buf) {
		LOG_ERR("No buffer for Host Command communication");
		return -EIO;
	}

	if ((handler_tx_buf && (handler_tx_buf != hc->tx.buf)) ||
	    (handler_rx_buf && (handler_rx_buf != hc->rx_ctx.buf))) {
		LOG_WRN("Host Command handler provided unused buffer");
	}

	k_thread_create(hc->thread, hc->stack, CONFIG_EC_HOST_CMD_HANDLER_STACK_SIZE,
			ec_host_cmd_thread, (void *)hc, NULL, NULL, CONFIG_EC_HOST_CMD_HANDLER_PRIO,
			0, K_NO_WAIT);
	k_thread_name_set(hc->thread, "ec_host_cmd");

	return 0;
}
