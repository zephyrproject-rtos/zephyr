/*
 * Copyright (c) 2026 Meta Platforms, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * PDFU Initiator, USB PD Firmware Update Specification, Revision 1.0,
 * sections 4 and 5.
 *
 * One request is outstanding at a time. Requests go out from a work item, so
 * the WaitTime a responder asks for can be honoured without blocking the USB-C
 * thread; responses arrive on that thread and only decide what is next.
 */

#include <zephyr/usb_c/pdfu_initiator.h>

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb_c/usbc.h>

#include "pdfu_proto.h"

LOG_MODULE_REGISTER(usbc_pdfu_initiator, CONFIG_USBC_PDFU_LOG_LEVEL);

/*
 * How long to wait for a Response. tPDFUResponseSent gives the responder
 * 27 ms to start answering; a 260-byte Data Block then takes ten chunk round
 * trips each way, so this is generous on purpose.
 */
#define PDFU_RSP_TIMEOUT_MS 1000

/* Retries while the Protocol Layer is still busy with the previous message. */
#define PDFU_SEND_RETRY_MS 5
#define PDFU_SEND_RETRIES  100

/*
 * Times a request is sent again when its Response does not come. A chunk that
 * misses tChunkSenderResponse makes the receiver drop the whole message, so on
 * a slow link an occasional request simply vanishes; the responder re-acks a
 * repeated Data Block, so asking again is safe.
 */
#define PDFU_RSP_RETRIES 3

/* Largest request: PDFU_DATA, 2 header + 2 index + 256 data. */
#define PDFU_MAX_REQ_LEN (2 + 2 + PDFU_DATA_BLOCK_LEN)

#define PDFU_PROGRESS_EVERY 64

static void pdfu_send_work_cb(struct k_work *work);
static void pdfu_timeout_cb(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(pdfu_send_work, pdfu_send_work_cb);
static K_WORK_DELAYABLE_DEFINE(pdfu_timeout_work, pdfu_timeout_cb);
static K_MUTEX_DEFINE(pdfu_lock);

static struct {
	const struct device *port;
	const struct pdfu_image_src *src;
	struct pdfu_initiator_faults faults;
	struct pdfu_initiator_status st;
	uint16_t version[4];
	/** Block index to send next */
	uint16_t next_block;
	/** True once the block with fewer than 256 bytes has been sent */
	bool last_sent;
	int send_retries;
	int rsp_retries;
	int64_t started_ms;
} init;

const char *pdfu_initiator_state_name(enum pdfu_initiator_state state)
{
	switch (state) {
	case PDFU_INIT_IDLE:
		return "idle";
	case PDFU_INIT_GET_FW_ID:
		return "get_fw_id";
	case PDFU_INIT_INITIATE:
		return "initiate";
	case PDFU_INIT_DATA:
		return "data";
	case PDFU_INIT_VALIDATE:
		return "validate";
	case PDFU_INIT_DONE:
		return "done";
	case PDFU_INIT_ABORTED:
		return "aborted";
	case PDFU_INIT_FAILED:
		return "failed";
	default:
		return "?";
	}
}

static bool pdfu_running(void)
{
	return init.st.state >= PDFU_INIT_GET_FW_ID && init.st.state <= PDFU_INIT_VALIDATE;
}

static void pdfu_send_abort(void)
{
	uint8_t req[2] = {PDFU_PROTOCOL_V1_0, PDFU_REQ_ABORT};

	/* Never answered, so nothing to wait for. */
	(void)usbc_send_ext_msg(init.port, PD_EXT_FIRMWARE_UPDATE_REQUEST, req, sizeof(req));
}

static void pdfu_finish(enum pdfu_initiator_state state, const char *error)
{
	k_work_cancel_delayable(&pdfu_timeout_work);
	k_work_cancel_delayable(&pdfu_send_work);

	/*
	 * A responder left mid-transfer refuses everything but PDFU_INITIATE
	 * and PDFU_ABORT, which made one failure fail every push after it.
	 */
	if (state == PDFU_INIT_FAILED || state == PDFU_INIT_ABORTED) {
		pdfu_send_abort();
	}

	init.st.state = state;
	init.st.error = error;
	init.st.elapsed_ms = (uint32_t)(k_uptime_get() - init.started_ms);

	if (state == PDFU_INIT_DONE) {
		LOG_INF("done: %u bytes in %u blocks, %u ms", init.st.bytes_sent,
			init.st.blocks_sent, init.st.elapsed_ms);
	} else if (error != NULL) {
		LOG_ERR("%s after %u bytes: %s", pdfu_initiator_state_name(state),
			init.st.bytes_sent, error);
	} else {
		LOG_WRN("%s after %u bytes", pdfu_initiator_state_name(state), init.st.bytes_sent);
	}
}

/* Queue the request for the current state, after @p wait_ms. */
static void pdfu_next(uint32_t wait_ms)
{
	init.send_retries = 0;
	init.rsp_retries = 0;
	k_work_reschedule(&pdfu_send_work, K_MSEC(wait_ms));
}

static int pdfu_build(uint8_t *buf, uint32_t *len)
{
	buf[0] = PDFU_PROTOCOL_V1_0;

	switch (init.st.state) {
	case PDFU_INIT_GET_FW_ID:
		buf[1] = PDFU_REQ_GET_FW_ID;
		*len = 2;
		return 0;

	case PDFU_INIT_INITIATE:
		buf[1] = PDFU_REQ_INITIATE;
		for (int i = 0; i < 4; i++) {
			sys_put_le16(init.version[i], &buf[2 + i * 2]);
		}
		*len = 10;
		return 0;

	case PDFU_INIT_DATA: {
		uint32_t offset = (uint32_t)init.next_block * PDFU_DATA_BLOCK_LEN;
		uint32_t n = MIN(init.st.size - MIN(offset, init.st.size), PDFU_DATA_BLOCK_LEN);
		int err;

		buf[1] = PDFU_REQ_DATA;
		sys_put_le16(init.next_block, &buf[2]);

		/*
		 * An image that is a whole number of blocks still needs a short
		 * block to end the transfer, so the last one is empty.
		 */
		if (n > 0) {
			err = init.src->read(offset, &buf[4], n);
			if (err) {
				return err;
			}
		}

		if (init.faults.corrupt_block != 0 &&
		    init.next_block + 1 == init.faults.corrupt_block && n > 0) {
			LOG_WRN("corrupting block %u", init.next_block);
			buf[4] ^= 0xff;
		}

		init.last_sent = n < PDFU_DATA_BLOCK_LEN;
		*len = 4 + n;
		return 0;
	}

	case PDFU_INIT_VALIDATE:
		buf[1] = PDFU_REQ_VALIDATE;
		*len = 2;
		return 0;

	default:
		return -EINVAL;
	}
}

static void pdfu_send_work_cb(struct k_work *work)
{
	uint8_t buf[PDFU_MAX_REQ_LEN];
	uint32_t len = 0;
	int err;

	ARG_UNUSED(work);

	k_mutex_lock(&pdfu_lock, K_FOREVER);

	if (!pdfu_running()) {
		k_mutex_unlock(&pdfu_lock);
		return;
	}

	err = pdfu_build(buf, &len);
	if (err) {
		pdfu_finish(PDFU_INIT_FAILED, "image read failed");
		k_mutex_unlock(&pdfu_lock);
		return;
	}

	err = usbc_send_ext_msg(init.port, PD_EXT_FIRMWARE_UPDATE_REQUEST, buf, len);
	if (err == -EBUSY && ++init.send_retries < PDFU_SEND_RETRIES) {
		k_work_reschedule(&pdfu_send_work, K_MSEC(PDFU_SEND_RETRY_MS));
		k_mutex_unlock(&pdfu_lock);
		return;
	}
	if (err) {
		LOG_ERR("send failed (%d)", err);
		pdfu_finish(PDFU_INIT_FAILED, "could not send a request");
		k_mutex_unlock(&pdfu_lock);
		return;
	}

	k_work_reschedule(&pdfu_timeout_work, K_MSEC(PDFU_RSP_TIMEOUT_MS));
	k_mutex_unlock(&pdfu_lock);
}

static void pdfu_timeout_cb(struct k_work *work)
{
	ARG_UNUSED(work);

	k_mutex_lock(&pdfu_lock, K_FOREVER);
	if (pdfu_running()) {
		if (init.rsp_retries++ < PDFU_RSP_RETRIES) {
			LOG_WRN("no response in %s, asking again",
				pdfu_initiator_state_name(init.st.state));
			init.send_retries = 0;
			k_work_reschedule(&pdfu_send_work, K_NO_WAIT);
		} else {
			pdfu_finish(PDFU_INIT_FAILED, "no response");
		}
	}
	k_mutex_unlock(&pdfu_lock);
}

static void pdfu_on_get_fw_id(const uint8_t *p, uint32_t len)
{
	if (len < 20 || p[0] != PDFU_OK) {
		init.st.last_status = len > 0 ? p[0] : PDFU_ERR_UNKNOWN;
		pdfu_finish(PDFU_INIT_FAILED, "GET_FW_ID refused");
		return;
	}

	for (int i = 0; i < 4; i++) {
		init.st.responder_version[i] = sys_get_le16(&p[7 + i * 2]);
	}

	LOG_INF("responder runs v%u.%u.%u.%u, offering v%u.%u.%u.%u, %u bytes",
		init.st.responder_version[0], init.st.responder_version[1],
		init.st.responder_version[2], init.st.responder_version[3], init.version[0],
		init.version[1], init.version[2], init.version[3], init.st.size);

	init.st.state = PDFU_INIT_INITIATE;
	pdfu_next(0);
}

static void pdfu_on_initiate(const uint8_t *p, uint32_t len)
{
	uint32_t max_size;

	if (len < 2 || p[0] != PDFU_OK) {
		init.st.last_status = len > 0 ? p[0] : PDFU_ERR_UNKNOWN;
		pdfu_finish(PDFU_INIT_FAILED, "PDFU_INITIATE refused");
		return;
	}

	if (p[1] == PDFU_WAIT_STOP) {
		pdfu_finish(PDFU_INIT_FAILED, "responder cannot continue");
		return;
	}

	if (len >= 5) {
		max_size = p[2] | (p[3] << 8) | ((p[4] & 0x0f) << 16);
		if (init.st.size > max_size) {
			pdfu_finish(PDFU_INIT_FAILED, "image larger than the responder takes");
			return;
		}
	}

	init.next_block = 0;
	init.last_sent = false;
	init.st.state = PDFU_INIT_DATA;
	pdfu_next(p[1]);
}

static void pdfu_on_data(const uint8_t *p, uint32_t len)
{
	uint16_t next;

	if (len < 5 || p[0] != PDFU_OK) {
		init.st.last_status = len > 0 ? p[0] : PDFU_ERR_UNKNOWN;
		pdfu_finish(PDFU_INIT_FAILED, "data block refused");
		return;
	}

	next = sys_get_le16(&p[3]);
	init.st.bytes_sent = MIN((uint32_t)next * PDFU_DATA_BLOCK_LEN, init.st.size);
	init.st.blocks_sent = next;

	if ((next % PDFU_PROGRESS_EVERY) == 0) {
		LOG_INF("%u of %u bytes", init.st.bytes_sent, init.st.size);
	}

	if (init.faults.abort_after != 0 && next >= init.faults.abort_after) {
		pdfu_finish(PDFU_INIT_ABORTED, NULL);
		return;
	}

	if (init.last_sent) {
		init.st.bytes_sent = init.st.size;
		init.st.state = PDFU_INIT_VALIDATE;
		pdfu_next(p[1]);
		return;
	}

	if (next != init.next_block + 1) {
		pdfu_finish(PDFU_INIT_FAILED, "responder skipped a block");
		return;
	}

	init.next_block = next;
	pdfu_next(p[1]);
}

static void pdfu_on_validate(const uint8_t *p, uint32_t len)
{
	if (len < 2) {
		pdfu_finish(PDFU_INIT_FAILED, "short validate response");
		return;
	}

	init.st.last_status = p[0];

	/* Still checking: ask again once the WaitTime has passed. */
	if (p[0] == PDFU_ERR_NOTDONE && p[1] != PDFU_WAIT_STOP) {
		pdfu_next(p[1]);
		return;
	}

	if (p[0] != PDFU_OK) {
		pdfu_finish(PDFU_INIT_FAILED, "image did not validate");
		return;
	}

	pdfu_finish(PDFU_INIT_DONE, NULL);
}

bool pdfu_initiator_msg(const struct device *port, enum pd_ext_msg_type type, const uint8_t *data,
			uint32_t len)
{
	uint8_t rsp;

	if (init.src == NULL || port != init.port || type != PD_EXT_FIRMWARE_UPDATE_RESPONSE) {
		return false;
	}

	k_mutex_lock(&pdfu_lock, K_FOREVER);

	if (!pdfu_running()) {
		k_mutex_unlock(&pdfu_lock);
		return true;
	}

	if (len < 2 || data[0] != PDFU_PROTOCOL_V1_0) {
		pdfu_finish(PDFU_INIT_FAILED, "malformed response");
		k_mutex_unlock(&pdfu_lock);
		return true;
	}

	k_work_cancel_delayable(&pdfu_timeout_work);

	rsp = data[1];
	data += 2;
	len -= 2;

	switch (init.st.state) {
	case PDFU_INIT_GET_FW_ID:
		if (rsp == PDFU_RSP_GET_FW_ID) {
			pdfu_on_get_fw_id(data, len);
		}
		break;
	case PDFU_INIT_INITIATE:
		if (rsp == PDFU_RSP_INITIATE) {
			pdfu_on_initiate(data, len);
		}
		break;
	case PDFU_INIT_DATA:
		if (rsp == PDFU_RSP_DATA) {
			pdfu_on_data(data, len);
		}
		break;
	case PDFU_INIT_VALIDATE:
		if (rsp == PDFU_RSP_VALIDATE) {
			pdfu_on_validate(data, len);
		}
		break;
	default:
		break;
	}

	k_mutex_unlock(&pdfu_lock);

	return true;
}

int pdfu_initiator_start(const struct pdfu_initiator_faults *faults)
{
	int err;

	if (init.src == NULL) {
		return -ENODEV;
	}

	k_mutex_lock(&pdfu_lock, K_FOREVER);

	if (pdfu_running()) {
		k_mutex_unlock(&pdfu_lock);
		return -EBUSY;
	}

	memset(&init.st, 0, sizeof(init.st));
	init.faults = faults ? *faults : (struct pdfu_initiator_faults){0};

	err = init.src->open(&init.st.size, init.version);
	if (err || init.st.size == 0) {
		k_mutex_unlock(&pdfu_lock);
		return -ENOENT;
	}

	init.started_ms = k_uptime_get();
	init.st.state = PDFU_INIT_GET_FW_ID;
	pdfu_next(0);

	k_mutex_unlock(&pdfu_lock);

	LOG_INF("pushing %u bytes, v%u.%u.%u.%u", init.st.size, init.version[0], init.version[1],
		init.version[2], init.version[3]);

	return 0;
}

void pdfu_initiator_abort(void)
{
	k_mutex_lock(&pdfu_lock, K_FOREVER);
	if (pdfu_running()) {
		pdfu_finish(PDFU_INIT_ABORTED, NULL);
	}
	k_mutex_unlock(&pdfu_lock);
}

void pdfu_initiator_status(struct pdfu_initiator_status *out)
{
	k_mutex_lock(&pdfu_lock, K_FOREVER);
	*out = init.st;
	if (pdfu_running()) {
		out->elapsed_ms = (uint32_t)(k_uptime_get() - init.started_ms);
	}
	k_mutex_unlock(&pdfu_lock);
}

int pdfu_initiator_init(const struct device *port, const struct pdfu_image_src *src)
{
	if (port == NULL || src == NULL || src->open == NULL || src->read == NULL) {
		return -EINVAL;
	}

	init.port = port;
	init.src = src;
	init.st.state = PDFU_INIT_IDLE;

	LOG_INF("PDFU initiator ready on %s", port->name);

	return 0;
}
