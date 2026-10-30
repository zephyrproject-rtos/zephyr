/*
 * Copyright (c) 2026 Witekio
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * EMW3080 IPC layer.
 *
 * Frame: | req_id (u32 LE) | api_id (u16 LE) | params |
 * A response echoes the req_id and api_id of its command. Events carry
 * req_id 0 and have bit 15 of api_id set.
 */

#include "emw3080_ipc.h"

#include <stdbool.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(emw3080_ipc, CONFIG_WIFI_LOG_LEVEL);

#include "emw3080_spi.h"

#define EMW3080_IPC_REQ_ID_OFFSET 0
#define EMW3080_IPC_API_ID_OFFSET 4
#define EMW3080_IPC_PARAMS_OFFSET EMW3080_IPC_HDR_LEN

#define EMW3080_IPC_REQ_ID_NONE     0x00000000U
#define EMW3080_IPC_REQ_ID_RESERVED 0xFFFFFFFFU
#define EMW3080_IPC_API_EVENT_BIT   0x8000U

struct emw3080_ipc_hdr {
	uint32_t req_id;
	uint16_t api_id;
};

struct emw3080_ipc_frame {
	struct emw3080_ipc_hdr hdr;
	const void *payload;
	uint16_t payload_len;
};

/* One command outstanding at a time, held for the whole request. */
static K_MUTEX_DEFINE(req_lock);

/* Protects 'pending', shared with the SPI transport thread. */
static K_MUTEX_DEFINE(pending_lock);
static K_SEM_DEFINE(rsp_sem, 0, 1);

static struct {
	bool active;
	struct emw3080_ipc_frame req;
	void *res;
	size_t res_len;
	int result;
} pending;

static uint32_t next_req_id = 1U;
static uint8_t tx_buf[EMW3080_IPC_MAX_PKT];

static emw3080_ipc_event_cb_t event_cb;
static void *event_cb_user;

static uint32_t req_id_next(void)
{
	uint32_t id = next_req_id++;

	if (next_req_id == EMW3080_IPC_REQ_ID_NONE || next_req_id == EMW3080_IPC_REQ_ID_RESERVED) {
		next_req_id = 1U;
	}
	return id;
}

static struct emw3080_ipc_frame frame_create(uint16_t api_id, const void *params,
					     uint16_t params_len)
{
	return (struct emw3080_ipc_frame){
		.hdr.req_id = req_id_next(),
		.hdr.api_id = api_id,
		.payload = params,
		.payload_len = params_len,
	};
}

static int frame_encode(const struct emw3080_ipc_frame *frame, uint8_t *buf, size_t buf_size)
{
	size_t total = EMW3080_IPC_HDR_LEN + frame->payload_len;

	if (total > buf_size) {
		return -EMSGSIZE;
	}

	sys_put_le32(frame->hdr.req_id, &buf[EMW3080_IPC_REQ_ID_OFFSET]);
	sys_put_le16(frame->hdr.api_id, &buf[EMW3080_IPC_API_ID_OFFSET]);
	if (frame->payload_len > 0U) {
		memcpy(&buf[EMW3080_IPC_PARAMS_OFFSET], frame->payload, frame->payload_len);
	}
	return (int)total;
}

static int frame_decode(const uint8_t *buf, uint16_t len, struct emw3080_ipc_frame *frame)
{
	if (len < EMW3080_IPC_HDR_LEN) {
		return -EBADMSG;
	}

	frame->hdr.req_id = sys_get_le32(&buf[EMW3080_IPC_REQ_ID_OFFSET]);
	frame->hdr.api_id = sys_get_le16(&buf[EMW3080_IPC_API_ID_OFFSET]);
	frame->payload = &buf[EMW3080_IPC_PARAMS_OFFSET];
	frame->payload_len = len - EMW3080_IPC_HDR_LEN;
	return 0;
}

static bool frame_is_event(const struct emw3080_ipc_frame *frame)
{
	return (frame->hdr.req_id == EMW3080_IPC_REQ_ID_NONE) ||
	       ((frame->hdr.api_id & EMW3080_IPC_API_EVENT_BIT) != 0U);
}

static bool frame_is_response_to(const struct emw3080_ipc_frame *req,
				 const struct emw3080_ipc_frame *res)
{
	return req->hdr.api_id == res->hdr.api_id && req->hdr.req_id == res->hdr.req_id;
}

static void ipc_on_frame(const uint8_t *buf, uint16_t len, void *user)
{
	ARG_UNUSED(user);

	struct emw3080_ipc_frame frame;

	if (frame_decode(buf, len, &frame) != 0) {
		LOG_WRN("runt frame (%u B) dropped", len);
		return;
	}

	if (frame_is_event(&frame)) {
		LOG_DBG("event api_id=0x%04x len=%u", frame.hdr.api_id, frame.payload_len);
		if (event_cb != NULL) {
			event_cb(frame.hdr.api_id, frame.payload, frame.payload_len, event_cb_user);
		}
		return;
	}

	k_mutex_lock(&pending_lock, K_FOREVER);

	if (pending.active && frame_is_response_to(&pending.req, &frame)) {
		if (frame.payload_len > pending.res_len) {
			LOG_ERR("api_id=0x%04x: response %u B > buffer %zu B", frame.hdr.api_id,
				frame.payload_len, pending.res_len);
			pending.result = -EMSGSIZE;
		} else {
			if (frame.payload_len > 0U) {
				memcpy(pending.res, frame.payload, frame.payload_len);
			}
			pending.result = (int)frame.payload_len;
		}
		pending.active = false;
		k_sem_give(&rsp_sem);
	} else {
		LOG_WRN("unexpected response req_id=%u api_id=0x%04x dropped", frame.hdr.req_id,
			frame.hdr.api_id);
	}

	k_mutex_unlock(&pending_lock);
}

int emw3080_ipc_init(emw3080_ipc_event_cb_t cb, void *user)
{
	event_cb = cb;
	event_cb_user = user;

	return emw3080_spi_init(ipc_on_frame, NULL);
}

static int ipc_transact(uint16_t api_id, const void *req, uint16_t req_len, void *res,
			uint16_t res_len, uint32_t timeout_ms)
{
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(timeout_ms));
	struct emw3080_ipc_frame frame;
	int len;
	int ret;

	if ((req == NULL && req_len > 0U) || (res == NULL && res_len > 0U)) {
		return -EINVAL;
	}
	if (req_len > EMW3080_IPC_MAX_PARAMS) {
		return -EMSGSIZE;
	}

	if (k_mutex_lock(&req_lock, sys_timepoint_timeout(end)) != 0) {
		LOG_ERR("api_id=0x%04x: IPC busy", api_id);
		return -ETIMEDOUT;
	}

	frame = frame_create(api_id, req, req_len);

	len = frame_encode(&frame, tx_buf, sizeof(tx_buf));
	if (len < 0) {
		k_mutex_unlock(&req_lock);
		return len;
	}

	/* Armed before sending: the response may come in the same transaction. */
	k_mutex_lock(&pending_lock, K_FOREVER);
	pending.req = frame;
	pending.res = res;
	pending.res_len = res_len;
	pending.result = 0;
	pending.active = true;
	k_sem_reset(&rsp_sem);
	k_mutex_unlock(&pending_lock);

	LOG_DBG("req_id=%u api_id=0x%04x len=%d", frame.hdr.req_id, api_id, len);

	ret = emw3080_spi_send(tx_buf, (uint16_t)len, sys_timepoint_timeout(end));
	if (ret == 0) {
		ret = (k_sem_take(&rsp_sem, sys_timepoint_timeout(end)) == 0) ? pending.result
									      : -ETIMEDOUT;
	}

	/* Disarmed: the transport thread no longer writes to 'res'. */
	k_mutex_lock(&pending_lock, K_FOREVER);
	if (pending.active) {
		pending.active = false;
	} else if (ret == -ETIMEDOUT) {
		ret = pending.result;
	}
	k_mutex_unlock(&pending_lock);

	if (ret < 0) {
		LOG_ERR("api_id=0x%04x failed (%d)", api_id, ret);
	}

	k_mutex_unlock(&req_lock);
	return ret;
}

int emw3080_ipc_request(uint16_t api_id, const void *req, uint16_t req_len, void *res,
			uint16_t res_size, uint16_t *res_len, uint32_t timeout_ms)
{
	int ret = ipc_transact(api_id, req, req_len, res, res_size, timeout_ms);

	if (ret < 0) {
		return ret;
	}
	if (res_len != NULL) {
		*res_len = (uint16_t)ret;
	} else if (ret != res_size) {
		LOG_ERR("api_id=0x%04x: response %d B, expected %u B", api_id, ret, res_size);
		return -EBADMSG;
	}
	return EMW3080_IPC_OK;
}
