/*
 * Copyright (c) 2018-2019 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This file is based on DAP.c from DAPLink Interface Firmware.
 * Copyright (c) 2009-2016, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(dap, CONFIG_CMSIS_DAP_LOG_LEVEL);

#include <string.h>
#include <zephyr.h>
#include <device.h>
#include <init.h>
#include <sys/byteorder.h>
#include <drivers/debug/swd.h>

#include "cmsis_dap.h"

#define DAP_PACKET_SIZE		CONFIG_HID_INTERRUPT_EP_MPS

K_MBOX_DEFINE(dap_ctrl_mbox);

K_MEM_POOL_DEFINE(dap_mpool, DAP_PACKET_SIZE, DAP_PACKET_SIZE,
		  CONFIG_CMSIS_DAP_PACKET_COUNT, 4);

static struct k_thread dap_tdata;
static K_THREAD_STACK_DEFINE(dap_stack, CONFIG_CMSIS_DAP_STACK_SIZE);

struct dap_configuration {
	const char *swd_dev_name;
	const struct device *swd_dev;
	uint8_t debug_port;
	uint8_t capabilities;
	struct {
		/** Idle cycles after transfer */
		uint8_t idle_cycles;
		/** Number of retries after WAIT response */
		uint16_t retry_count;
		/** Number of retries if read value does not match */
		uint16_t match_retry;
		/** Match Mask */
		uint32_t match_mask;
	} transfer;
};

#define DT_DRV_COMPAT dap_sw_gpio
#define DAP_DT_GET_SW_GPIO(d) {.swd_dev_name = DT_INST_LABEL(d),},

static struct dap_configuration dap_ctx[] = {
	DT_INST_FOREACH_STATUS_OKAY(DAP_DT_GET_SW_GPIO)
};

static uint8_t dap_request_buf[DAP_PACKET_SIZE];

const char dap_fw_ver[] = DAP_FW_VER;

/*
 * Get DAP Information
 */
static uint16_t dap_info(const uint8_t *request, uint8_t *response)
{
	uint8_t id = request[0];
	uint8_t length = 0U;
	uint8_t *info = response + 1;

	switch (id) {
	case DAP_ID_VENDOR:
		LOG_DBG("ID_VENDOR unsupported");
		break;
	case DAP_ID_PRODUCT:
		LOG_DBG("ID_PRODUCT unsupported");
		break;
	case DAP_ID_SER_NUM:
		LOG_DBG("ID_SER_NUM unsupported");
		break;
	case DAP_ID_FW_VER:
		LOG_DBG("ID_FW_VER");
		memcpy(info, dap_fw_ver, sizeof(dap_fw_ver));
		length = (uint8_t)sizeof(dap_fw_ver);
		break;
	case DAP_ID_DEVICE_VENDOR:
		LOG_DBG("ID_DEVICE_VENDOR unsupported");
		break;
	case DAP_ID_DEVICE_NAME:
		LOG_DBG("ID_DEVICE_NAME unsupported");
		break;
	case DAP_ID_CAPABILITIES:
		info[0] = dap_ctx[0].capabilities;
		LOG_DBG("ID_CAPABILITIES 0x%0x", info[0]);
		length = 1U;
		break;
	case DAP_ID_SWO_BUFFER_SIZE:
		LOG_DBG("ID_SWO_BUFFER_SIZE unsupported");
		break;
	case DAP_ID_PACKET_SIZE:
		LOG_DBG("ID_PACKET_SIZE");
		sys_put_le16(DAP_PACKET_SIZE, &info[0]);
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

	response[0] = (uint8_t)length;

	return length + 1U;
}

/*
 * Process Host Status command and prepare response
 */
static uint16_t dap_host_status(const uint8_t *request, uint8_t *response)
{
	LOG_DBG("");

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

/*
 * Process Connect command and prepare response
 */
static uint16_t dap_connect(const uint8_t *request, uint8_t *response)
{
	const struct swd_api *api = dap_ctx[0].swd_dev->api;
	uint8_t port;

	if (request[0] == DAP_PORT_AUTODETECT) {
		port = DAP_PORT_SWD;
	} else {
		port = request[0];
	}

	switch (port) {
	case DAP_PORT_SWD:
		LOG_DBG("port swd");
		dap_ctx[0].debug_port = DAP_PORT_SWD;
		api->sw_port_on(dap_ctx[0].swd_dev);
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

/*
 * Process Disconnect command and prepare response
 */
static uint16_t dap_disconnect(uint8_t *response)
{
	const struct swd_api *api = dap_ctx[0].swd_dev->api;

	LOG_DBG("");

	dap_ctx[0].debug_port = DAP_PORT_DISABLED;
	api->sw_port_off(dap_ctx[0].swd_dev);
	response[0] = DAP_OK;

	return 1;
}

/*
 * Process Delay command and prepare response
 */
static uint16_t dap_delay(const uint8_t *request, uint8_t *response)
{
	uint16_t delay = sys_get_le16(&request[0]);

	LOG_DBG("dap delay %u ms", delay);

	k_busy_wait(delay * USEC_PER_MSEC);
	response[0] = DAP_OK;

	return 1U;
}

/*
 * Process Reset Target command and prepare response
 */
static uint16_t dap_reset_target(uint8_t *response)
{
	LOG_DBG("");

	response[0] = DAP_OK;
	response[1] = 0U;
	LOG_WRN("unsupported");

	return 2;
}

/*
 * Process SWJ Pins command and prepare response
 */
static uint16_t dap_swj_pins(const uint8_t *request, uint8_t *response)
{
	const struct swd_api *api = dap_ctx[0].swd_dev->api;
	uint8_t value = request[0];
	uint8_t select = request[1];
	uint32_t wait = sys_get_le32(&request[2]);
	uint8_t state;

	/* Skip if nothing selected. */
	if (select) {
		api->sw_set_pins(dap_ctx[0].swd_dev, select, value);
	}

	/* TODO: implement wait */
	api->sw_get_pins(dap_ctx[0].swd_dev, &state);
	LOG_ERR("select 0x%02x, value 0x%02x, wait %u, state 0x%02x",
		select, value, wait, state);

	response[0] = state;

	return sizeof(state);
}

/*
 * Process SWJ Clock command and prepare response
 */
static uint16_t dap_swj_clock(const uint8_t *request, uint8_t *response)
{
	const struct swd_api *api = dap_ctx[0].swd_dev->api;
	uint32_t clock = sys_get_le32(&request[0]);

	LOG_DBG("clock %d", clock);

	if (clock) {
		api->sw_set_clock(dap_ctx[0].swd_dev, clock);
		response[0] = DAP_OK;
	} else {
		response[0] = DAP_ERROR;
	}

	return 1U;
}

/*
 * Process SWJ Sequence command and prepare response
 */
static uint16_t dap_swj_sequence(const uint8_t *request, uint8_t *response)
{
	const struct swd_api *api = dap_ctx[0].swd_dev->api;
	uint16_t count = request[0];

	LOG_DBG("count %u", count);

	if (count == 0U) {
		count = 256U;
	}

	api->sw_sequence(dap_ctx[0].swd_dev, count, &request[1]);
	response[0] = DAP_OK;

	return 1U;
}

/*
 * Process SWD Configure command and prepare response
 */
static uint16_t dap_swd_configure(const uint8_t *request, uint8_t *response)
{
	const struct swd_api *api = dap_ctx[0].swd_dev->api;
	uint8_t turnaround = (request[0] & 0x03U) + 1U;
	bool data_phase = (request[0] & 0x04U) ? true : false;

	api->sw_configure(dap_ctx[0].swd_dev, turnaround, data_phase);
	response[0] = DAP_OK;

	return 1U;
}

/*
 * Process Transfer Configure command and prepare response
 */
static uint16_t dap_transfer_cfg(const uint8_t *request, uint8_t *response)
{

	dap_ctx[0].transfer.idle_cycles = request[0];
	dap_ctx[0].transfer.retry_count = sys_get_le16(&request[1]);
	dap_ctx[0].transfer.match_retry = sys_get_le16(&request[3]);
	LOG_DBG("idle_cycles %d, retry_count %d, match_retry %d",
		dap_ctx[0].transfer.idle_cycles,
		dap_ctx[0].transfer.retry_count,
		dap_ctx[0].transfer.match_retry);

	response[0] = DAP_OK;
	return 1U;
}

static inline uint8_t do_swd_transfer(const uint8_t req_val, uint32_t *data)
{
	const struct swd_api *api = dap_ctx[0].swd_dev->api;
	uint32_t retry = dap_ctx[0].transfer.retry_count;
	uint8_t rspns_val;

	do {
		api->sw_transfer(dap_ctx[0].swd_dev,
				 req_val,
				 data,
				 dap_ctx[0].transfer.idle_cycles,
				 &rspns_val);
	} while ((rspns_val == SWD_ACK_WAIT) && retry--);

	return rspns_val;
}

static uint8_t swd_transfer_match(const uint8_t req_val, uint32_t match_val)
{
	uint32_t match_retry = dap_ctx[0].transfer.match_retry;
	uint32_t data;
	uint8_t rspns_val;

	if (req_val & SWD_REQUEST_APnDP) {
		/* Post AP read, result will be returned on the next transfer */
		rspns_val = do_swd_transfer(req_val, NULL);
		if (rspns_val != SWD_ACK_OK) {
			return rspns_val;
		}
	}

	do {
		/*
		 * Read register until its value matches
		 * or retry counter expires
		 */
		rspns_val = do_swd_transfer(req_val, &data);
		if (rspns_val != SWD_ACK_OK) {
			return rspns_val;
		}

	} while (((data & dap_ctx[0].transfer.match_mask) != match_val) &&
		 match_retry--);

	if ((data & dap_ctx[0].transfer.match_mask) != match_val) {
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
static uint16_t dap_swd_transfer(const uint8_t *request, uint8_t *response)
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
		if (req_val & SWD_REQUEST_RnW) {
			/* Read register */
			if (post_read) {
				/*
				 * Read was posted before, read previous AP
				 * data or post next AP read.
				 */
				if ((req_val & (SWD_REQUEST_APnDP |
						DAP_TRANSFER_MATCH_VALUE)) !=
				    SWD_REQUEST_APnDP) {
					req_val = DP_RDBUFF | SWD_REQUEST_RnW;
					post_read = false;
				}

				rspns_val = do_swd_transfer(req_val, &data);
				if (rspns_val != SWD_ACK_OK) {
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

				rspns_val = swd_transfer_match(req_val,
							       match_val);
				if (rspns_val != SWD_ACK_OK) {
					break;
				}

			} else if (req_val & SWD_REQUEST_APnDP) {
				/* Normal read */
				if (!post_read) {
					/* Post AP read */
					rspns_val = do_swd_transfer(req_val,
								    NULL);
					if (rspns_val != SWD_ACK_OK) {
						break;
					}
					post_read = true;
				}
			} else {
				/* Read DP register */
				rspns_val = do_swd_transfer(req_val,
							    &data);
				if (rspns_val != SWD_ACK_OK) {
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
				rspns_val = do_swd_transfer(DP_RDBUFF |
							    SWD_REQUEST_RnW,
							    &data);
				if (rspns_val != SWD_ACK_OK) {
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
				dap_ctx[0].transfer.match_mask = data;
				rspns_val = SWD_ACK_OK;
			} else {
				/* Write DP/AP register */
				rspns_val = do_swd_transfer(req_val, &data);
				if (rspns_val != SWD_ACK_OK) {
					break;
				}

				check_write = 1U;
			}
		}
		rspns_cnt++;
	}

	if (rspns_val == SWD_ACK_OK) {
		if (post_read) {
			/* Read previous data */
			rspns_val = do_swd_transfer(DP_RDBUFF |
						    SWD_REQUEST_RnW,
						    &data);
			if (rspns_val != SWD_ACK_OK) {
				goto end;
			}

			/* Store previous data */
			sys_put_le32(data, rspns_buf);
			rspns_buf += sizeof(data);
		} else if (check_write) {
			/* Check last write */
			rspns_val = do_swd_transfer(DP_RDBUFF |
						    SWD_REQUEST_RnW,
						    NULL);
		}
	}

end:
	response[0] = rspns_cnt;
	response[1] = rspns_val;

	return (rspns_buf - response);
}

/*
 * Delegate DAP Transfer command
 */
static uint16_t dap_transfer(const uint8_t *request, uint8_t *response)
{
	uint16_t retval;

	switch (dap_ctx[0].debug_port) {
	case DAP_PORT_SWD:
		retval = dap_swd_transfer(request, response);
		break;
	case DAP_PORT_JTAG:
	default:
		LOG_ERR("port unsupported");
		response[0] = DAP_ERROR;
		retval = 1U;
	}

	return retval;
}

/*
 * Process SWD DAP_TransferBlock command and prepare response.
 * pyOCD counterpart is _encode_transfer_block_data.
 * Packet format: one byte DAP_index (ignored)
 *                two bytes transfer_count
 *                one byte block_request (register)
 *                data[transfer_count * sizeof(uint32_t)]
 */
static uint16_t dap_swd_transferblock(const uint8_t *request, uint8_t *response)
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

	if (req_val & SWD_REQUEST_RnW) {
		/* Read register block */
		if (req_val & SWD_REQUEST_APnDP) {
			/* Post AP read */
			rspns_val = do_swd_transfer(req_val, NULL);
			if (rspns_val != SWD_ACK_OK) {
				goto end;
			}
		}
		while (req_cnt--) {
			/* Read DP/AP register */
			if ((req_cnt == 0U) &&
			    (req_val & SWD_REQUEST_APnDP)) {
				/* Last AP read */
				req_val = DP_RDBUFF | SWD_REQUEST_RnW;
			}

			rspns_val = do_swd_transfer(req_val, &data);
			if (rspns_val != SWD_ACK_OK) {
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
			rspns_val = do_swd_transfer(req_val, &data);
			if (rspns_val != SWD_ACK_OK) {
				goto end;
			}

			rspns_cnt++;
		}
		/* Check last write */
		rspns_val = do_swd_transfer(DP_RDBUFF | SWD_REQUEST_RnW, NULL);
	}

end:
	sys_put_le16(rspns_cnt, &response[0]);
	response[2] = rspns_val;

	if (CONFIG_CMSIS_DAP_LOG_LEVEL == LOG_LEVEL_DBG) {
		LOG_DBG("Received %u, to transmit %u, response count %u",
			req_buf - request,
			rspns_buf - response,
			rspns_cnt * 4);
	}

	return (rspns_buf - response);
}

/*
 * Delegate Transfer Block command
 */
static uint16_t dap_transferblock(const uint8_t *request, uint8_t *response)
{
	uint16_t retval;

	switch (dap_ctx[0].debug_port) {
	case DAP_PORT_SWD:
		retval = dap_swd_transferblock(request, response);
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

/*
 * Process SWD Write ABORT command and prepare response
 */
static uint16_t dap_swd_writeabort(const uint8_t *request, uint8_t *response)
{
	const struct swd_api *api = dap_ctx[0].swd_dev->api;
	/* Load data (Ignore DAP index in request[0]) */
	uint32_t data = sys_get_le32(&request[1]);

	/* Write Abort register */
	api->sw_transfer(dap_ctx[0].swd_dev, DP_ABORT, &data,
			 dap_ctx[0].transfer.idle_cycles, NULL);

	response[0] = DAP_OK;
	return 1U;
}

/*
 * Delegate DAP Write ABORT command
 */
static uint16_t dap_writeabort(const uint8_t *request, uint8_t *response)
{
	uint16_t retval;

	LOG_DBG("");

	switch (dap_ctx[0].debug_port) {
	case DAP_PORT_SWD:
		retval = dap_swd_writeabort(request, response);
		break;
	case DAP_PORT_JTAG:
	default:
		LOG_ERR("port unsupported");
		response[0] = DAP_ERROR;
		retval = 1U;
	}
	return retval;
}

/*
 * Process DAP Vendor command request
 */
static uint16_t dap_process_vendor_cmd(const uint8_t *request,
				       uint8_t *response)
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
static uint16_t dap_process_cmd(const uint8_t *request, uint8_t *response)
{
	uint16_t retval;

	if ((*request >= ID_DAP_VENDOR0) && (*request <= ID_DAP_VENDOR31)) {
		return dap_process_vendor_cmd(request, response);
	}

	*response++ = *request;
	LOG_DBG("request 0x%02x", *request);

	switch (*request++) {
	case ID_DAP_INFO:
		retval = dap_info(request, response);
		break;
	case ID_DAP_HOST_STATUS:
		retval = dap_host_status(request, response);
		break;
	case ID_DAP_CONNECT:
		retval = dap_connect(request, response);
		break;
	case ID_DAP_DISCONNECT:
		retval = dap_disconnect(response);
		break;
	case ID_DAP_DELAY:
		retval = dap_delay(request, response);
		break;
	case ID_DAP_RESET_TARGET:
		retval = dap_reset_target(response);
		break;
	case ID_DAP_SWJ_PINS:
		retval = dap_swj_pins(request, response);
		break;
	case ID_DAP_SWJ_CLOCK:
		retval = dap_swj_clock(request, response);
		break;
	case ID_DAP_SWJ_SEQUENCE:
		retval = dap_swj_sequence(request, response);
		break;
	case ID_DAP_SWD_CONFIGURE:
		retval = dap_swd_configure(request, response);
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
		retval = dap_transfer_cfg(request, response);
		break;
	case ID_DAP_TRANSFER:
		retval = dap_transfer(request, response);
		break;
	case ID_DAP_TRANSFER_BLOCK:
		retval = dap_transferblock(request, response);
		break;
	case ID_DAP_WRITE_ABORT:
		retval = dap_writeabort(request, response);
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
static uint32_t dap_execute_cmd(const uint8_t *request, uint8_t *response)
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
			n = dap_process_cmd(request, response);
			retval += n;
			request += n;
			response += n;
		}
		return retval;
	}

	return dap_process_cmd(request, response);
}

static void dap_thead(void *p1, void *p2, void *p3)
{
	struct k_mbox *mbox = &dap_ctrl_mbox;
	k_tid_t iface_tid = K_ANY;
	struct k_mbox_msg req_msg;
	struct k_mbox_msg rspns_msg;

	while (1) {
		req_msg.size = DAP_PACKET_SIZE;
		req_msg.rx_source_thread = K_ANY;

		k_mbox_get(mbox, &req_msg, dap_request_buf, K_FOREVER);
		LOG_DBG("message source thread %p size %u",
			req_msg.rx_source_thread, req_msg.size);

		if (req_msg.info == DAP_MBMSG_REGISTER_IFACE && !iface_tid) {
			iface_tid = req_msg.rx_source_thread;
			LOG_INF("register HID interface thread ID %p",
				iface_tid);
			continue;
		}

		if (req_msg.info != DAP_MBMSG_FROM_IFACE || !iface_tid) {
			LOG_INF("Invalid message or interface");
			continue;
		}

		k_mem_pool_alloc(&dap_mpool, &rspns_msg.tx_block,
				 DAP_PACKET_SIZE, K_FOREVER);

		memcpy(rspns_msg.tx_block.data, dap_request_buf, req_msg.size);
		rspns_msg.size = dap_execute_cmd(dap_request_buf,
					rspns_msg.tx_block.data);

		rspns_msg.info = DAP_MBMSG_FROM_CONTROLLER;
		rspns_msg.tx_data = NULL;
		rspns_msg.tx_target_thread = iface_tid;
		k_mbox_async_put(mbox, &rspns_msg, NULL);
	}
}

int dap_setup(struct k_mbox **mbox, k_tid_t *dap_tid)
{
	dap_ctx[0].swd_dev = device_get_binding(dap_ctx[0].swd_dev_name);

	if (dap_ctx[0].swd_dev == NULL) {
		LOG_ERR("Cannot get SWD interface driver");
		return -1;
	}

	*dap_tid = k_thread_create(&dap_tdata, dap_stack,
				   CONFIG_CMSIS_DAP_STACK_SIZE,
				   dap_thead, NULL, NULL, NULL,
				   K_PRIO_PREEMPT(0), 0, K_NO_WAIT);

	if (!*dap_tid) {
		*mbox = NULL;
		LOG_ERR("Failed to initialize DAP controller thread");
		return -1;
	}

	*mbox = &dap_ctrl_mbox;

	/* Default settings */
	dap_ctx[0].debug_port = 0U;
	dap_ctx[0].transfer.idle_cycles = 0U;
	dap_ctx[0].transfer.retry_count = 100U;
	dap_ctx[0].transfer.match_retry = 0U;
	dap_ctx[0].transfer.match_mask = 0U;
	dap_ctx[0].capabilities = DAP_SUPPORTS_ATOMIC_COMMANDS |
			       DAP_DP_SUPPOTS_SWD;

	return 0;
}
