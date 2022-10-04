/*
 * Copyright (c) 2020 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/ec_host_cmd_periph.h>
#include <zephyr/mgmt/ec_host_cmd.h>
#include <zephyr/devicetree.h>
#include <string.h>

#if !DT_HAS_CHOSEN(zephyr_ec_host_interface)
#error Must chose zephyr,ec-host-interface in device tree
#endif

#define DT_HOST_CMD_DEV DT_CHOSEN(zephyr_ec_host_interface)

#define RX_HEADER_SIZE (sizeof(struct ec_host_cmd_request_header))
#define TX_HEADER_SIZE (sizeof(struct ec_host_cmd_response_header))

/** Used by host command handlers for their response before going over wire */
uint8_t tx_buffer[CONFIG_EC_HOST_CMD_HANDLER_TX_BUFFER];

static uint8_t cal_checksum(const uint8_t *const buffer, const uint16_t size)
{
	uint8_t checksum = 0;

	for (size_t i = 0; i < size; ++i) {
		checksum += buffer[i];
	}
	return (uint8_t)(-checksum);
}

static void send_error_response(const struct device *const ec_host_cmd_dev,
				const enum ec_host_cmd_status error)
{
	struct ec_host_cmd_response_header *const tx_header = (void *)tx_buffer;

	tx_header->prtcl_ver = 3;
	tx_header->result = error;
	tx_header->data_len = 0;
	tx_header->reserved = 0;
	tx_header->checksum = 0;
	tx_header->checksum = cal_checksum(tx_buffer, TX_HEADER_SIZE);

	const struct ec_host_cmd_periph_tx_buf tx = {
		.buf = tx_buffer,
		.len = TX_HEADER_SIZE,
	};
	ec_host_cmd_periph_send(ec_host_cmd_dev, &tx);
}

static void handle_host_cmds_entry(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	const struct device *ec_host_cmd_dev = DEVICE_DT_GET(DT_HOST_CMD_DEV);
	struct ec_host_cmd_periph_rx_ctx rx;

	if (!device_is_ready(ec_host_cmd_dev)) {
		return;
	}

	ec_host_cmd_periph_init(ec_host_cmd_dev, &rx);

	while (1) {
		/* We have finished reading from RX buffer, so allow another
		 * incoming msg.
		 */
		k_sem_give(rx.dev_owns);

		/* Wait until and RX messages is received on host interface */
		if (k_sem_take(rx.handler_owns, K_FOREVER) < 0) {
			/* This code path should never occur due to the nature of
			 * k_sem_take with K_FOREVER
			 */
			send_error_response(ec_host_cmd_dev,
					    EC_HOST_CMD_ERROR);
		}
		/* rx buf and len now have valid incoming data */

		if (*rx.len < RX_HEADER_SIZE) {
			send_error_response(ec_host_cmd_dev,
					    EC_HOST_CMD_REQUEST_TRUNCATED);
			continue;
		}

		const struct ec_host_cmd_request_header *const rx_header =
			(void *)rx.buf;

		/* Only support version 3 */
		if (rx_header->prtcl_ver != 3) {
			send_error_response(ec_host_cmd_dev,
					    EC_HOST_CMD_INVALID_HEADER);
			continue;
		}

		const uint16_t rx_valid_data_size =
			rx_header->data_len + RX_HEADER_SIZE;
		/*
		 * Ensure we received at least as much data as is expected.
		 * It is okay to receive more since some hardware interfaces
		 * add on extra padding bytes at the end.
		 */
		if (*rx.len < rx_valid_data_size) {
			send_error_response(ec_host_cmd_dev,
					    EC_HOST_CMD_REQUEST_TRUNCATED);
			continue;
		}

		/* Validate checksum */
		if (cal_checksum(rx.buf, rx_valid_data_size) != 0) {
			send_error_response(ec_host_cmd_dev,
					    EC_HOST_CMD_INVALID_CHECKSUM);
			continue;
		}

		const struct ec_host_cmd_handler *found_handler = NULL;

		STRUCT_SECTION_FOREACH(ec_host_cmd_handler, handler)
		{
			if (handler->id == rx_header->cmd_id) {
				found_handler = handler;
				break;
			}
		}

		/* No handler in this image for requested command */
		if (found_handler == NULL) {
			send_error_response(ec_host_cmd_dev,
					    EC_HOST_CMD_INVALID_COMMAND);
			continue;
		}

		/*
		 * Ensure that RX/TX buffers are cleared between each host
		 * command to ensure subsequent host command handlers cannot
		 * read data from previous host command runs.
		 */
		memset(&rx.buf[rx_valid_data_size], 0,
		       *rx.len - rx_valid_data_size);
		memset(tx_buffer, 0, sizeof(tx_buffer));

		struct ec_host_cmd_handler_args args = {
			.input_buf = rx.buf + RX_HEADER_SIZE,
			.input_buf_size = rx_header->data_len,
			.output_buf = tx_buffer + TX_HEADER_SIZE,
			.output_buf_size = sizeof(tx_buffer) - TX_HEADER_SIZE,
			.version = rx_header->cmd_ver,
		};

		if (found_handler->min_rqt_size > args.input_buf_size) {
			send_error_response(ec_host_cmd_dev,
					    EC_HOST_CMD_REQUEST_TRUNCATED);
			continue;
		}

		if (found_handler->min_rsp_size > args.output_buf_size) {
			send_error_response(ec_host_cmd_dev,
					    EC_HOST_CMD_INVALID_RESPONSE);
			continue;
		}

		if (args.version > sizeof(found_handler->version_mask) ||
		    !(found_handler->version_mask & BIT(args.version))) {
			send_error_response(ec_host_cmd_dev,
					    EC_HOST_CMD_INVALID_VERSION);
			continue;
		}

		const enum ec_host_cmd_status handler_rv =
			found_handler->handler(&args);

		if (handler_rv != EC_HOST_CMD_SUCCESS) {
			send_error_response(ec_host_cmd_dev, handler_rv);
			continue;
		}

		struct ec_host_cmd_response_header *const tx_header =
			(void *)tx_buffer;

		tx_header->prtcl_ver = 3;
		tx_header->result = EC_HOST_CMD_SUCCESS;
		tx_header->data_len = args.output_buf_size;

		const uint16_t tx_valid_data_size =
			tx_header->data_len + TX_HEADER_SIZE;
		if (tx_valid_data_size > sizeof(tx_buffer)) {
			send_error_response(ec_host_cmd_dev,
					    EC_HOST_CMD_INVALID_RESPONSE);
			continue;
		}

		/* Calculate checksum */
		tx_header->checksum =
			cal_checksum(tx_buffer, tx_valid_data_size);

		const struct ec_host_cmd_periph_tx_buf tx = {
			.buf = tx_buffer,
			.len = tx_valid_data_size,
		};
		ec_host_cmd_periph_send(ec_host_cmd_dev, &tx);
	}
}

K_THREAD_DEFINE(ec_host_cmd_handler_tid, CONFIG_EC_HOST_CMD_HANDLER_STACK_SIZE,
		handle_host_cmds_entry, NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO, 0, 0);
