/*
 * Copyright (c) 2018-2019 PHYTEC Messtechnik GmbH
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file is based on DAP.c from CMSIS-DAP Source (Revision:    V2.0.0)
 * https://github.com/ARM-software/CMSIS_5/tree/develop/CMSIS/DAP/Firmware
 * Copyright (c) 2013-2017 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/swdp.h>
#include <stdint.h>

#include <cmsis_dap.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dap, CONFIG_DAP_LOG_LEVEL);

#define DAP_STATE_CONNECTED	0

struct dap_context {
	struct device *swdp_dev;
	atomic_t state;
	uint8_t debug_port;
	uint8_t capabilities;
	uint16_t pkt_size;
	struct {
		/* Idle cycles after transfer */
		uint8_t idle_cycles;
		/* Number of retries after WAIT response */
		uint16_t retry_count;
		/* Number of retries if read value does not match */
		uint16_t match_retry;
		/* Match Mask */
		uint32_t match_mask;
	} transfer;
};

static struct dap_context dap_ctx[1];

#define CMSIS_DAP_PACKET_MIN_SIZE 64

BUILD_ASSERT(sizeof(CONFIG_CMSIS_DAP_PROBE_VENDOR) <=
	     MIN(CMSIS_DAP_PACKET_MIN_SIZE - 2, UINT8_MAX - 2),
	     "PROBE_VENDOR string is too long.");
BUILD_ASSERT(sizeof(CONFIG_CMSIS_DAP_PROBE_NAME) <=
	     MIN(CMSIS_DAP_PACKET_MIN_SIZE - 2, UINT8_MAX - 2),
	     "PROBE_NAME string is too long.");
BUILD_ASSERT(sizeof(CONFIG_CMSIS_DAP_BOARD_VENDOR) <=
	     MIN(CMSIS_DAP_PACKET_MIN_SIZE - 2, UINT8_MAX - 2),
	     "BOARD_VENDOR string is too long.");
BUILD_ASSERT(sizeof(CONFIG_CMSIS_DAP_BOARD_NAME) <=
	     MIN(CMSIS_DAP_PACKET_MIN_SIZE - 2, UINT8_MAX - 2),
	     "BOARD_NAME string is too long.");
BUILD_ASSERT(sizeof(CONFIG_CMSIS_DAP_DEVICE_VENDOR) <=
	     MIN(CMSIS_DAP_PACKET_MIN_SIZE - 2, UINT8_MAX - 2),
	     "DEVICE_VENDOR string is too long.");
BUILD_ASSERT(sizeof(CONFIG_CMSIS_DAP_DEVICE_NAME) <=
	     MIN(CMSIS_DAP_PACKET_MIN_SIZE - 2, UINT8_MAX - 2),
	     "DEVICE_NAME string is too long.");

/* Get DAP Information */
static uint16_t dap_info(struct dap_context *const ctx,
			 const uint8_t *const request,
			 uint8_t *const response)
{
	uint8_t *info = response + 1;
	uint8_t id = request[0];
	uint8_t length = 0U;

	switch (id) {
	case DAP_ID_VENDOR:
		LOG_DBG("ID_VENDOR");
		memcpy(info, CONFIG_CMSIS_DAP_PROBE_VENDOR,
		      sizeof(CONFIG_CMSIS_DAP_PROBE_VENDOR));
		length = sizeof(CONFIG_CMSIS_DAP_PROBE_VENDOR);
		break;
	case DAP_ID_PRODUCT:
		LOG_DBG("ID_PRODUCT");
		memcpy(info, CONFIG_CMSIS_DAP_PROBE_NAME,
		      sizeof(CONFIG_CMSIS_DAP_PROBE_NAME));
		length = sizeof(CONFIG_CMSIS_DAP_PROBE_NAME);
		break;
	case DAP_ID_SER_NUM:
		/* optional to implement */
		LOG_DBG("ID_SER_NUM unsupported");
		break;
	case DAP_ID_FW_VER:
		LOG_DBG("ID_FW_VER");
		memcpy(info, DAP_FW_VER, sizeof(DAP_FW_VER));
		length = sizeof(DAP_FW_VER);
		break;
	case DAP_ID_DEVICE_VENDOR:
		LOG_DBG("ID_DEVICE_VENDOR");
		memcpy(info, CONFIG_CMSIS_DAP_DEVICE_VENDOR,
		      sizeof(CONFIG_CMSIS_DAP_DEVICE_VENDOR));
		length = sizeof(CONFIG_CMSIS_DAP_DEVICE_VENDOR);
		break;
	case DAP_ID_DEVICE_NAME:
		LOG_DBG("ID_DEVICE_NAME");
		memcpy(info, CONFIG_CMSIS_DAP_DEVICE_NAME,
		      sizeof(CONFIG_CMSIS_DAP_DEVICE_NAME));
		length = sizeof(CONFIG_CMSIS_DAP_DEVICE_NAME);
		break;
	case DAP_ID_BOARD_VENDOR:
		LOG_DBG("ID_BOARD_VENDOR");
		memcpy(info, CONFIG_CMSIS_DAP_BOARD_VENDOR,
		      sizeof(CONFIG_CMSIS_DAP_BOARD_VENDOR));
		length = sizeof(CONFIG_CMSIS_DAP_BOARD_VENDOR);
		break;
	case DAP_ID_BOARD_NAME:
		memcpy(info, CONFIG_CMSIS_DAP_BOARD_NAME,
		      sizeof(CONFIG_CMSIS_DAP_BOARD_NAME));
		length = sizeof(CONFIG_CMSIS_DAP_BOARD_NAME);
		LOG_DBG("ID_BOARD_NAME");
		break;
	case DAP_ID_PRODUCT_FW_VER:
		/* optional to implement */
		LOG_DBG("ID_PRODUCT_FW_VER unsupported");
		break;
	case DAP_ID_CAPABILITIES:
		info[0] = ctx->capabilities;
		LOG_DBG("ID_CAPABILITIES 0x%0x", info[0]);
		length = 1U;
		break;
	case DAP_ID_TIMESTAMP_CLOCK:
		LOG_DBG("ID_TIMESTAMP_CLOCK unsupported");
		break;
	case DAP_ID_UART_RX_BUFFER_SIZE:
		LOG_DBG("ID_UART_RX_BUFFER_SIZE unsupported");
		break;
	case DAP_ID_UART_TX_BUFFER_SIZE:
		LOG_DBG("ID_UART_TX_BUFFER_SIZE unsupported");
		break;
	case DAP_ID_SWO_BUFFER_SIZE:
		LOG_DBG("ID_SWO_BUFFER_SIZE unsupported");
		break;
	case DAP_ID_PACKET_SIZE:
		LOG_DBG("ID_PACKET_SIZE");
		sys_put_le16(ctx->pkt_size, &info[0]);
		length = 2U;
		break;
	case DAP_ID_PACKET_COUNT:
		LOG_DBG("ID_PACKET_COUNT");
		info[0] = CONFIG_CMSIS_DAP_PACKET_COUNT;
		length = 1U;
		break;
	default:
		LOG_DBG("unsupported ID");
		break;
	}

	response[0] = length;

	return length + 1U;
}

/* Process Host Status command and prepare response */
static uint16_t dap_host_status(struct dap_context *const ctx,
				const uint8_t *const request,
				uint8_t *const response)
{
	switch (request[0]) {
	case DAP_DEBUGGER_CONNECTED:
		if (request[1]) {
			LOG_INF("Debugger connected");
		} else {
			LOG_INF("Debugger disconnected");
		}
		break;
	case DAP_TARGET_RUNNING:
		LOG_DBG("unsupported");
		break;
	default:
		*response = DAP_ERROR;
		return 1U;
	}

	response[0] = DAP_OK;
	return 1U;
}

/* Process Connect command and prepare response */
static uint16_t dap_connect(struct dap_context *const ctx,
			    const uint8_t *const request,
			    uint8_t *const response)
{
	const struct swdp_api *api = ctx->swdp_dev->api;
	uint8_t port;

	if (request[0] == DAP_PORT_AUTODETECT) {
		port = DAP_PORT_SWD;
	} else {
		port = request[0];
	}

	switch (port) {
	case DAP_PORT_SWD:
		LOG_INF("port swd");
		ctx->debug_port = DAP_PORT_SWD;

		if (atomic_test_and_set_bit(&ctx->state,
					    DAP_STATE_CONNECTED)) {
			LOG_ERR("DAP device is already connected");
			break;
		}

		api->swdp_port_on(ctx->swdp_dev);
		break;
	case DAP_PORT_JTAG:
		LOG_ERR("port unsupported");
		port = DAP_ERROR;
		break;
	default:
		LOG_DBG("port disabled");
		port = DAP_PORT_DISABLED;
		break;
	}

	response[0] = port;
	return 1U;
}

/* Process Disconnect command and prepare response */
static uint16_t dap_disconnect(struct dap_context *const ctx,
			       uint8_t *const response)
{
	const struct swdp_api *api = ctx->swdp_dev->api;

	LOG_DBG("");

	ctx->debug_port = DAP_PORT_DISABLED;

	if (atomic_test_bit(&ctx->state, DAP_STATE_CONNECTED)) {
		api->swdp_port_off(ctx->swdp_dev);
	} else {
		LOG_WRN("DAP device is not connected");
	}

	response[0] = DAP_OK;
	atomic_clear_bit(&ctx->state, DAP_STATE_CONNECTED);

	return 1;
}

/* Process Delay command and prepare response */
static uint16_t dap_delay(struct dap_context *const ctx,
			  const uint8_t *const request,
			  uint8_t *const response)
{
	uint16_t delay = sys_get_le16(&request[0]);

	LOG_DBG("dap delay %u ms", delay);

	k_busy_wait(delay * USEC_PER_MSEC);
	response[0] = DAP_OK;

	return 1U;
}

/* Process Reset Target command and prepare response */
static uint16_t dap_reset_target(struct dap_context *const ctx,
				 uint8_t *const response)
{
	response[0] = DAP_OK;
	response[1] = 0U;
	LOG_WRN("unsupported");

	return 2;
}

/* Process SWJ Pins command and prepare response */
static uint16_t dap_swj_pins(struct dap_context *const ctx,
			     const uint8_t *const request,
			     uint8_t *const response)
{
	const struct swdp_api *api = ctx->swdp_dev->api;
	uint8_t value = request[0];
	uint8_t select = request[1];
	uint32_t wait = sys_get_le32(&request[2]);
	k_timepoint_t end = sys_timepoint_calc(K_USEC(wait));
	uint8_t state;

	if (!atomic_test_bit(&ctx->state, DAP_STATE_CONNECTED)) {
		LOG_ERR("DAP device is not connected");
		response[0] = DAP_ERROR;
		return 1U;
	}

	/* Skip if nothing selected. */
	if (select) {
		api->swdp_set_pins(ctx->swdp_dev, select, value);
	}

	do {
		api->swdp_get_pins(ctx->swdp_dev, &state);
		LOG_INF("select 0x%02x, value 0x%02x, wait %u, state 0x%02x",
			select, value, wait, state);
		if ((value & select) == (state & select)) {
			LOG_DBG("swdp_get_pins succeeded before timeout");
			break;
		}
	} while (!sys_timepoint_expired(end));

	response[0] = state;

	return sizeof(state);
}

/* Process SWJ Clock command and prepare response */
static uint16_t dap_swj_clock(struct dap_context *const ctx,
			      const uint8_t *const request,
			      uint8_t *const response)
{
	const struct swdp_api *api = ctx->swdp_dev->api;
	uint32_t clk = sys_get_le32(&request[0]);

	LOG_DBG("clock %d", clk);

	if (atomic_test_bit(&ctx->state, DAP_STATE_CONNECTED)) {
		if (clk) {
			api->swdp_set_clock(ctx->swdp_dev, clk);
			response[0] = DAP_OK;
		} else {
			response[0] = DAP_ERROR;
		}
	} else {
		LOG_WRN("DAP device is not connected");
		response[0] = DAP_OK;
	}

	return 1U;
}

/* Process SWJ Sequence command and prepare response */
static uint16_t dap_swj_sequence(struct dap_context *const ctx,
				 const uint8_t *const request,
				 uint8_t *const response)
{
	const struct swdp_api *api = ctx->swdp_dev->api;
	uint16_t count = request[0];

	LOG_DBG("count %u", count);

	if (count == 0U) {
		count = 256U;
	}

	if (!atomic_test_bit(&ctx->state, DAP_STATE_CONNECTED)) {
		LOG_ERR("DAP device is not connected");
		response[0] = DAP_ERROR;
		return 1U;
	}

	api->swdp_output_sequence(ctx->swdp_dev, count, &request[1]);
	response[0] = DAP_OK;

	return 1U;
}

/* Process SWD Configure command and prepare response */
static uint16_t dap_swdp_configure(struct dap_context *const ctx,
				  const uint8_t *const request,
				  uint8_t *const response)
{
	const struct swdp_api *api = ctx->swdp_dev->api;
	uint8_t turnaround = (request[0] & 0x03U) + 1U;
	bool data_phase = (request[0] & 0x04U) ? true : false;

	if (!atomic_test_bit(&ctx->state, DAP_STATE_CONNECTED)) {
		LOG_ERR("DAP device is not connected");
		response[0] = DAP_ERROR;
		return 1U;
	}

	api->swdp_configure(ctx->swdp_dev, turnaround, data_phase);
	response[0] = DAP_OK;

	return 1U;
}

/* Process Transfer Configure command and prepare response */
static uint16_t dap_transfer_cfg(struct dap_context *const ctx,
				 const uint8_t *const request,
				 uint8_t *const response)
{

	ctx->transfer.idle_cycles = request[0];
	ctx->transfer.retry_count = sys_get_le16(&request[1]);
	ctx->transfer.match_retry = sys_get_le16(&request[3]);
	LOG_DBG("idle_cycles %d, retry_count %d, match_retry %d",
		ctx->transfer.idle_cycles,
		ctx->transfer.retry_count,
		ctx->transfer.match_retry);

	response[0] = DAP_OK;
	return 1U;
}

static inline uint8_t do_swdp_transfer(struct dap_context *const ctx,
				      const uint8_t req_val,
				      uint32_t *data)
{
	const struct swdp_api *api = ctx->swdp_dev->api;
	uint32_t retry = ctx->transfer.retry_count;
	uint8_t rspns_val;

	do {
		api->swdp_transfer(ctx->swdp_dev,
				   req_val,
				   data,
				   ctx->transfer.idle_cycles,
				   &rspns_val);
	} while ((rspns_val == SWDP_ACK_WAIT) && retry--);

	return rspns_val;
}

static uint8_t swdp_transfer_match(struct dap_context *const ctx,
				  const uint8_t req_val,
				  const uint32_t match_val)
{
	uint32_t match_retry = ctx->transfer.match_retry;
	uint32_t data;
	uint8_t rspns_val;

	if (req_val & SWDP_REQUEST_APnDP) {
		/* Post AP read, result will be returned on the next transfer */
		rspns_val = do_swdp_transfer(ctx, req_val, NULL);
		if (rspns_val != SWDP_ACK_OK) {
			return rspns_val;
		}
	}

	do {
		/*
		 * Read register until its value matches
		 * or retry counter expires
		 */
		rspns_val = do_swdp_transfer(ctx, req_val, &data);
		if (rspns_val != SWDP_ACK_OK) {
			return rspns_val;
		}

	} while (((data & ctx->transfer.match_mask) != match_val) &&
		 match_retry--);

	if ((data & ctx->transfer.match_mask) != match_val) {
		rspns_val |= DAP_TRANSFER_MISMATCH;
	}

	return rspns_val;
}

/*
 * Process SWD Transfer command and prepare response
 * pyOCD counterpart is _encode_transfer_data.
 * Packet format: one byte DAP_index (ignored)
 *                one bytes transfer_count
 *                following by number transfer_count pairs of
 *                    one byte request (register)
 *                    four byte data (for write request only)
 */
static uint16_t dap_swdp_transfer(struct dap_context *const ctx,
				 const uint8_t *const request,
				 uint8_t *const response)
{
	uint8_t *rspns_buf;
	const uint8_t *req_buf;
	uint8_t rspns_cnt = 0;
	uint8_t rspns_val = 0;
	bool post_read = false;
	uint32_t check_write = 0;
	uint8_t req_cnt;
	uint8_t req_val;
	uint32_t match_val;
	uint32_t data;

	/* Ignore DAP index request[0] */
	req_cnt = request[1];
	req_buf = request + sizeof(req_cnt) + 1;
	rspns_buf = response + (sizeof(rspns_cnt) + sizeof(rspns_val));

	for (; req_cnt; req_cnt--) {
		req_val = *req_buf++;
		if (req_val & SWDP_REQUEST_RnW) {
			/* Read register */
			if (post_read) {
				/*
				 * Read was posted before, read previous AP
				 * data or post next AP read.
				 */
				if ((req_val & (SWDP_REQUEST_APnDP |
						DAP_TRANSFER_MATCH_VALUE)) !=
				    SWDP_REQUEST_APnDP) {
					req_val = DP_RDBUFF | SWDP_REQUEST_RnW;
					post_read = false;
				}

				rspns_val = do_swdp_transfer(ctx, req_val, &data);
				if (rspns_val != SWDP_ACK_OK) {
					break;
				}

				/* Store previous AP data */
				sys_put_le32(data, rspns_buf);
				rspns_buf += sizeof(data);
			}
			if (req_val & DAP_TRANSFER_MATCH_VALUE) {
				LOG_INF("match value read");
				/* Read with value match */
				match_val = sys_get_le32(req_buf);
				req_buf += sizeof(match_val);

				rspns_val = swdp_transfer_match(ctx, req_val, match_val);
				if (rspns_val != SWDP_ACK_OK) {
					break;
				}

			} else if (req_val & SWDP_REQUEST_APnDP) {
				/* Normal read */
				if (!post_read) {
					/* Post AP read */
					rspns_val = do_swdp_transfer(ctx, req_val, NULL);
					if (rspns_val != SWDP_ACK_OK) {
						break;
					}
					post_read = true;
				}
			} else {
				/* Read DP register */
				rspns_val = do_swdp_transfer(ctx, req_val, &data);
				if (rspns_val != SWDP_ACK_OK) {
					break;
				}
				/* Store data */
				sys_put_le32(data, rspns_buf);
				rspns_buf += sizeof(data);
			}
			check_write = 0U;
		} else {
			/* Write register */
			if (post_read) {
				/* Read previous data */
				rspns_val = do_swdp_transfer(ctx,
							     DP_RDBUFF | SWDP_REQUEST_RnW,
							     &data);
				if (rspns_val != SWDP_ACK_OK) {
					break;
				}

				/* Store previous data */
				sys_put_le32(data, rspns_buf);
				rspns_buf += sizeof(data);
				post_read = false;
			}
			/* Load data */
			data = sys_get_le32(req_buf);
			req_buf += sizeof(data);
			if (req_val & DAP_TRANSFER_MATCH_MASK) {
				/* Write match mask */
				ctx->transfer.match_mask = data;
				rspns_val = SWDP_ACK_OK;
			} else {
				/* Write DP/AP register */
				rspns_val = do_swdp_transfer(ctx, req_val, &data);
				if (rspns_val != SWDP_ACK_OK) {
					break;
				}

				check_write = 1U;
			}
		}
		rspns_cnt++;
	}

	if (rspns_val == SWDP_ACK_OK) {
		if (post_read) {
			/* Read previous data */
			rspns_val = do_swdp_transfer(ctx,
						     DP_RDBUFF | SWDP_REQUEST_RnW,
						     &data);
			if (rspns_val != SWDP_ACK_OK) {
				goto end;
			}

			/* Store previous data */
			sys_put_le32(data, rspns_buf);
			rspns_buf += sizeof(data);
		} else if (check_write) {
			/* Check last write */
			rspns_val = do_swdp_transfer(ctx,
						     DP_RDBUFF | SWDP_REQUEST_RnW,
						     NULL);
		}
	}

end:
	response[0] = rspns_cnt;
	response[1] = rspns_val;

	return (rspns_buf - response);
}

/* Delegate DAP Transfer command */
static uint16_t dap_transfer(struct dap_context *const ctx,
			     const uint8_t *const request,
			     uint8_t *const response)
{
	uint16_t retval;

	if (!atomic_test_bit(&ctx->state, DAP_STATE_CONNECTED)) {
		LOG_ERR("DAP device is not connected");
		response[0] = DAP_ERROR;
		return 1U;
	}

	switch (ctx->debug_port) {
	case DAP_PORT_SWD:
		retval = dap_swdp_transfer(ctx, request, response);
		break;
	case DAP_PORT_JTAG:
	default:
		LOG_ERR("port unsupported");
		response[0] = DAP_ERROR;
		retval = 1U;
	}

	return retval;
}

static uint16_t dap_swdp_sequence(struct dap_context *const ctx,
				  const uint8_t *const request,
				  uint8_t *const response)
{
	const struct swdp_api *api = ctx->swdp_dev->api;
	const uint8_t *request_data = request + 1;
	uint8_t *response_data = response + 1;
	uint8_t count = request[0];
	uint8_t num_cycles;
	uint32_t num_bytes;
	bool input;

	switch (ctx->debug_port) {
	case DAP_PORT_SWD:
		response[0] = DAP_OK;
		break;
	case DAP_PORT_JTAG:
	default:
		LOG_ERR("port unsupported");
		response[0] = DAP_ERROR;
		return 1U;
	}

	for (size_t i = 0; i < count; ++i) {
		input = *request_data & BIT(7);
		num_cycles = *request_data & BIT_MASK(7);
		num_bytes = (num_cycles + 7) >> 3; /* rounded up to full bytes */

		if (num_cycles == 0) {
			num_cycles = 64;
		}

		request_data += 1;

		if (input) {
			api->swdp_input_sequence(ctx->swdp_dev, num_cycles, response_data);
			response_data += num_bytes;
		} else {
			api->swdp_output_sequence(ctx->swdp_dev, num_cycles, request_data);
			request_data += num_bytes;
		}
	}

	return response_data - response;
}

/*
 * Process SWD DAP_TransferBlock command and prepare response.
 * pyOCD counterpart is _encode_transfer_block_data.
 * Packet format: one byte DAP_index (ignored)
 *                two bytes transfer_count
 *                one byte block_request (register)
 *                data[transfer_count * sizeof(uint32_t)]
 */
static uint16_t dap_swdp_transferblock(struct dap_context *const ctx,
				      const uint8_t *const request,
				      uint8_t *const response)
{
	uint32_t data;
	uint8_t *rspns_buf;
	const uint8_t *req_buf;
	uint16_t rspns_cnt = 0;
	uint16_t req_cnt;
	uint8_t rspns_val = 0;
	uint8_t req_val;

	req_cnt = sys_get_le16(&request[1]);
	req_val = request[3];
	req_buf = request + (sizeof(req_cnt) + sizeof(req_val) + 1);
	rspns_buf = response + (sizeof(rspns_cnt) + sizeof(rspns_val));

	if (req_cnt == 0U) {
		goto end;
	}

	if (req_val & SWDP_REQUEST_RnW) {
		/* Read register block */
		if (req_val & SWDP_REQUEST_APnDP) {
			/* Post AP read */
			rspns_val = do_swdp_transfer(ctx, req_val, NULL);
			if (rspns_val != SWDP_ACK_OK) {
				goto end;
			}
		}

		while (req_cnt--) {
			/* Read DP/AP register */
			if ((req_cnt == 0U) &&
			    (req_val & SWDP_REQUEST_APnDP)) {
				/* Last AP read */
				req_val = DP_RDBUFF | SWDP_REQUEST_RnW;
			}

			rspns_val = do_swdp_transfer(ctx, req_val, &data);
			if (rspns_val != SWDP_ACK_OK) {
				goto end;
			}

			/* Store data */
			sys_put_le32(data, rspns_buf);
			rspns_buf += sizeof(data);
			rspns_cnt++;
		}
	} else {
		/* Write register block */
		while (req_cnt--) {
			/* Load data */
			data = sys_get_le32(req_buf);
			req_buf += sizeof(data);
			/* Write DP/AP register */
			rspns_val = do_swdp_transfer(ctx, req_val, &data);
			if (rspns_val != SWDP_ACK_OK) {
				goto end;
			}

			rspns_cnt++;
		}
		/* Check last write */
		rspns_val = do_swdp_transfer(ctx, DP_RDBUFF | SWDP_REQUEST_RnW, NULL);
	}

end:
	sys_put_le16(rspns_cnt, &response[0]);
	response[2] = rspns_val;

	LOG_DBG("Received %u, to transmit %u, response count %u",
		req_buf - request,
		rspns_buf - response,
		rspns_cnt * 4);

	return (rspns_buf - response);
}

/* Delegate Transfer Block command */
static uint16_t dap_transferblock(struct dap_context *const ctx,
				  const uint8_t *const request,
				  uint8_t *const response)
{
	uint16_t retval;

	if (!atomic_test_bit(&ctx->state, DAP_STATE_CONNECTED)) {
		LOG_ERR("DAP device is not connected");
		/* Clear response count */
		sys_put_le16(0U, &response[0]);
		/* Clear DAP response (ACK) value */
		response[2] = 0U;
		return 3U;
	}

	switch (ctx->debug_port) {
	case DAP_PORT_SWD:
		retval = dap_swdp_transferblock(ctx, request, response);
		break;
	case DAP_PORT_JTAG:
	default:
		LOG_ERR("port unsupported");
		/* Clear response count */
		sys_put_le16(0U, &response[0]);
		/* Clear DAP response (ACK) value */
		response[2] = 0U;
		retval = 3U;
	}

	return retval;
}

/* Process SWD Write ABORT command and prepare response */
static uint16_t dap_swdp_writeabort(struct dap_context *const ctx,
				   const uint8_t *const request,
				   uint8_t *const response)
{
	const struct swdp_api *api = ctx->swdp_dev->api;
	/* Load data (Ignore DAP index in request[0]) */
	uint32_t data = sys_get_le32(&request[1]);

	/* Write Abort register */
	api->swdp_transfer(ctx->swdp_dev, DP_ABORT, &data,
			  ctx->transfer.idle_cycles, NULL);

	response[0] = DAP_OK;
	return 1U;
}

/* Delegate DAP Write ABORT command */
static uint16_t dap_writeabort(struct dap_context *const ctx,
			       const uint8_t *const request,
			       uint8_t *const response)
{
	uint16_t retval;

	if (!atomic_test_bit(&ctx->state, DAP_STATE_CONNECTED)) {
		LOG_ERR("DAP device is not connected");
		response[0] = DAP_ERROR;
		return 1U;
	}

	switch (ctx->debug_port) {
	case DAP_PORT_SWD:
		retval = dap_swdp_writeabort(ctx, request, response);
		break;
	case DAP_PORT_JTAG:
	default:
		LOG_ERR("port unsupported");
		response[0] = DAP_ERROR;
		retval = 1U;
	}
	return retval;
}

/* Process DAP Vendor command request */
static uint16_t dap_process_vendor_cmd(struct dap_context *const ctx,
				       const uint8_t *const request,
				       uint8_t *const response)
{
	response[0] = ID_DAP_INVALID;
	return 1U;
}

/*
 * Process DAP command request and prepare response
 *   request:  pointer to request data
 *   response: pointer to response data
 *   return:   number of bytes in response
 *
 *   All the subsequent command functions have the same parameter
 *   and return value structure.
 */
static uint16_t dap_process_cmd(struct dap_context *const ctx,
				const uint8_t *request,
				uint8_t *response)
{
	uint16_t retval;

	LOG_HEXDUMP_DBG(request, 8, "req");

	if ((*request >= ID_DAP_VENDOR0) && (*request <= ID_DAP_VENDOR31)) {
		return dap_process_vendor_cmd(ctx, request, response);
	}

	*response++ = *request;
	LOG_DBG("request 0x%02x", *request);

	switch (*request++) {
	case ID_DAP_INFO:
		retval = dap_info(ctx, request, response);
		break;
	case ID_DAP_HOST_STATUS:
		retval = dap_host_status(ctx, request, response);
		break;
	case ID_DAP_CONNECT:
		retval = dap_connect(ctx, request, response);
		break;
	case ID_DAP_DISCONNECT:
		retval = dap_disconnect(ctx, response);
		break;
	case ID_DAP_DELAY:
		retval = dap_delay(ctx, request, response);
		break;
	case ID_DAP_RESET_TARGET:
		retval = dap_reset_target(ctx, response);
		break;
	case ID_DAP_SWJ_PINS:
		retval = dap_swj_pins(ctx, request, response);
		break;
	case ID_DAP_SWJ_CLOCK:
		retval = dap_swj_clock(ctx, request, response);
		break;
	case ID_DAP_SWJ_SEQUENCE:
		retval = dap_swj_sequence(ctx, request, response);
		break;
	case ID_DAP_SWDP_CONFIGURE:
		retval = dap_swdp_configure(ctx, request, response);
		break;
	case ID_DAP_SWDP_SEQUENCE:
		retval = dap_swdp_sequence(ctx, request, response);
		break;
	case ID_DAP_JTAG_SEQUENCE:
		LOG_ERR("JTAG sequence unsupported");
		retval = 1;
		*response = DAP_ERROR;
		break;
	case ID_DAP_JTAG_CONFIGURE:
		LOG_ERR("JTAG configure unsupported");
		retval = 1;
		*response = DAP_ERROR;
		break;
	case ID_DAP_JTAG_IDCODE:
		LOG_ERR("JTAG IDCODE unsupported");
		retval = 1;
		*response = DAP_ERROR;
		break;
	case ID_DAP_TRANSFER_CONFIGURE:
		retval = dap_transfer_cfg(ctx, request, response);
		break;
	case ID_DAP_TRANSFER:
		retval = dap_transfer(ctx, request, response);
		break;
	case ID_DAP_TRANSFER_BLOCK:
		retval = dap_transferblock(ctx, request, response);
		break;
	case ID_DAP_WRITE_ABORT:
		retval = dap_writeabort(ctx, request, response);
		break;
	case ID_DAP_SWO_TRANSPORT:
		LOG_ERR("SWO Transport unsupported");
		retval = 1;
		*response = DAP_ERROR;
		break;
	case ID_DAP_SWO_MODE:
		LOG_ERR("SWO Mode unsupported");
		retval = 1;
		*response = DAP_ERROR;
		break;
	case ID_DAP_SWO_BAUDRATE:
		LOG_ERR("SWO Baudrate unsupported");
		retval = 1;
		*response = DAP_ERROR;
		break;
	case ID_DAP_SWO_CONTROL:
		LOG_ERR("SWO Control unsupported");
		retval = 1;
		*response = DAP_ERROR;
		break;
	case ID_DAP_SWO_STATUS:
		LOG_ERR("SWO Status unsupported");
		retval = 1;
		*response = DAP_ERROR;
		break;
	case ID_DAP_SWO_DATA:
		LOG_ERR("SWO Data unsupported");
		retval = 1;
		*response = DAP_ERROR;
		break;
	case ID_DAP_UART_TRANSPORT:
		LOG_ERR("UART Transport unsupported");
		retval = 1;
		*response = DAP_ERROR;
		break;
	case ID_DAP_UART_CONFIGURE:
		LOG_ERR("UART Configure unsupported");
		retval = 1;
		*response = DAP_ERROR;
		break;
	case ID_DAP_UART_CONTROL:
		LOG_ERR("UART Control unsupported");
		retval = 1;
		*response = DAP_ERROR;
		break;
	case ID_DAP_UART_STATUS:
		LOG_ERR("UART Status unsupported");
		retval = 1;
		*response = DAP_ERROR;
		break;
	case ID_DAP_UART_TRANSFER:
		LOG_ERR("UART Transfer unsupported");
		retval = 1;
		*response = DAP_ERROR;
		break;

	default:
		*(response - 1) = ID_DAP_INVALID;
		return 1U;
	}

	return (1U + retval);
}

/*
 * Execute DAP command (process request and prepare response)
 *   request:  pointer to request data
 *   response: pointer to response data
 *   return:   number of bytes in response
 */
uint32_t dap_execute_cmd(const uint8_t *request,
			 uint8_t *response)
{
	uint32_t retval;
	uint16_t n;
	uint8_t count;

	if (request[0] == ID_DAP_EXECUTE_COMMANDS) {
		/* copy command and increment */
		*response++ = *request++;
		count = request[0];
		request += sizeof(count);
		response[0] = count;
		response += sizeof(count);
		retval = sizeof(count) + 1U;
		LOG_WRN("(untested) ID DAP EXECUTE_COMMANDS count %u", count);
		while (count--) {
			n = dap_process_cmd(&dap_ctx[0], request, response);
			retval += n;
			request += n;
			response += n;
		}
		return retval;
	}

	return dap_process_cmd(&dap_ctx[0], request, response);
}

void dap_update_pkt_size(const uint16_t pkt_size)
{
	dap_ctx[0].pkt_size = pkt_size;
	LOG_INF("New packet size %u", dap_ctx[0].pkt_size);
}

int dap_setup(const struct device *const dev)
{
	dap_ctx[0].swdp_dev = (void *)dev;

	if (!device_is_ready(dap_ctx[0].swdp_dev)) {
		LOG_ERR("SWD driver not ready");
		return -ENODEV;
	}

	/* Default settings */
	dap_ctx[0].pkt_size = CMSIS_DAP_PACKET_MIN_SIZE;
	dap_ctx[0].debug_port = 0U;
	dap_ctx[0].transfer.idle_cycles = 0U;
	dap_ctx[0].transfer.retry_count = 100U;
	dap_ctx[0].transfer.match_retry = 0U;
	dap_ctx[0].transfer.match_mask = 0U;
	dap_ctx[0].capabilities = DAP_SUPPORTS_ATOMIC_COMMANDS |
				  DAP_DP_SUPPORTS_SWD;

	return 0;
}
