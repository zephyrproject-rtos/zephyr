/*
 * Copyright (c) 2026 Meta Platforms, Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * PDFU Responder. Message numbering, payload layouts and the phase matrix
 * are from the USB PD Firmware Update Specification, Revision 1.0, sections
 * 5.2, 5.3 and Table 5-32.
 */

#include <zephyr/usb_c/pdfu.h>

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb_c/usbc.h>

#include "pdfu_proto.h"

LOG_MODULE_REGISTER(usbc_pdfu, CONFIG_USBC_PDFU_LOG_LEVEL);

/* Largest response we build: GET_FW_ID, 2 header + 20 payload. */
#define PDFU_MAX_RSP_LEN 22

/*
 * Between the VALIDATE Response and manifesting. Manifesting usually reboots,
 * and doing it inline would reset the device with the Response still queued,
 * so the initiator would see no answer to a validation that had succeeded.
 */
#define PDFU_MANIFEST_DELAY_MS 100

static void pdfu_manifest_work_cb(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(pdfu_manifest_work, pdfu_manifest_work_cb);

static struct {
	const struct device *port;
	const struct pdfu_ops *ops;
	enum pdfu_phase phase;
	uint32_t bytes;
	uint32_t max_image_size;
	/** Next Data Block index we expect */
	uint16_t next_block;
	/** Whether any PDFU_DATA has arrived, needed for the Transfer rules */
	bool data_started;
} pdfu;

const char *pdfu_phase_name(enum pdfu_phase phase)
{
	switch (phase) {
	case PDFU_PHASE_ENUMERATION:
		return "enumeration";
	case PDFU_PHASE_RECONFIGURATION:
		return "reconfiguration";
	case PDFU_PHASE_TRANSFER:
		return "transfer";
	case PDFU_PHASE_VALIDATION:
		return "validation";
	case PDFU_PHASE_MANIFESTATION:
		return "manifestation";
	default:
		return "?";
	}
}

enum pdfu_phase pdfu_responder_phase(void)
{
	return pdfu.phase;
}

uint32_t pdfu_responder_bytes(void)
{
	return pdfu.bytes;
}

static void pdfu_reset(const char *why)
{
	if (pdfu.phase != PDFU_PHASE_ENUMERATION) {
		LOG_INF("reset from %s: %s", pdfu_phase_name(pdfu.phase), why);
		if (pdfu.ops->abort != NULL) {
			pdfu.ops->abort();
		}
	}

	pdfu.phase = PDFU_PHASE_ENUMERATION;
	pdfu.bytes = 0;
	pdfu.next_block = 0;
	pdfu.data_started = false;
}

static void pdfu_send(uint8_t msg_type, const uint8_t *payload, uint32_t payload_len)
{
	uint8_t buf[PDFU_MAX_RSP_LEN];
	int err;

	if (payload_len + 2 > sizeof(buf)) {
		LOG_ERR("response type %02x too large (%u)", msg_type, payload_len);
		return;
	}

	buf[0] = PDFU_PROTOCOL_V1_0;
	buf[1] = msg_type;
	if (payload_len > 0) {
		memcpy(&buf[2], payload, payload_len);
	}

	err = usbc_send_ext_msg(pdfu.port, PD_EXT_FIRMWARE_UPDATE_RESPONSE, buf, payload_len + 2);
	if (err) {
		LOG_ERR("send of response %02x failed (%d)", msg_type, err);
	}
}

/**
 * Refuse a Request that is not valid in the current phase.
 *
 * Table 5-32 distinguishes "notify as unexpected" from "ignore". Both leave
 * the phase alone; only the former puts a Response on the wire.
 */
static void pdfu_unexpected(uint8_t rsp_type)
{
	uint8_t status = PDFU_ERR_UNEXPECTED_REQUEST;

	LOG_WRN("unexpected request in %s phase", pdfu_phase_name(pdfu.phase));
	pdfu_send(rsp_type, &status, 1);
}

static void pdfu_handle_get_fw_id(void)
{
	struct pdfu_fw_id id = {0};
	uint8_t p[20] = {0};

	pdfu.ops->get_fw_id(&id);

	p[0] = PDFU_OK;
	sys_put_le16(id.vid, &p[1]);
	sys_put_le16(id.pid, &p[3]);
	p[5] = id.hw_version;
	p[6] = id.si_version;
	sys_put_le16(id.fw_version[0], &p[7]);
	sys_put_le16(id.fw_version[1], &p[9]);
	sys_put_le16(id.fw_version[2], &p[11]);
	sys_put_le16(id.fw_version[3], &p[13]);
	p[15] = id.image_bank;
	memcpy(&p[16], id.flags, sizeof(id.flags));

	LOG_INF("GET_FW_ID: v%u.%u.%u.%u, image bank %u", id.fw_version[0], id.fw_version[1],
		id.fw_version[2], id.fw_version[3], id.image_bank);

	pdfu_send(PDFU_RSP_GET_FW_ID, p, sizeof(p));
}

static void pdfu_handle_initiate(const uint8_t *data, uint32_t len)
{
	uint16_t offered[4] = {0};
	uint8_t p[5] = {0};
	uint32_t max_size = 0;
	int status;

	if (len < 8) {
		LOG_WRN("PDFU_INITIATE payload short (%u)", len);
		p[0] = PDFU_ERR_UNKNOWN;
		p[1] = PDFU_WAIT_STOP;
		pdfu_send(PDFU_RSP_INITIATE, p, sizeof(p));
		return;
	}

	for (int i = 0; i < 4; i++) {
		offered[i] = sys_get_le16(&data[i * 2]);
	}

	status = pdfu.ops->begin(offered, &max_size);
	if (status != 0) {
		LOG_WRN("PDFU_INITIATE refused, status %02x", status);
		p[0] = status;
		p[1] = PDFU_WAIT_STOP;
		pdfu_send(PDFU_RSP_INITIATE, p, sizeof(p));
		return;
	}

	pdfu.max_image_size = max_size;
	pdfu.bytes = 0;
	pdfu.next_block = 0;
	pdfu.data_started = false;
	pdfu.phase = PDFU_PHASE_RECONFIGURATION;

	p[0] = PDFU_OK;
	/* Ready immediately; nothing to reconfigure before receiving data. */
	p[1] = 0;
	p[2] = max_size & 0xff;
	p[3] = (max_size >> 8) & 0xff;
	p[4] = (max_size >> 16) & 0x0f;

	LOG_INF("PDFU_INITIATE accepted: v%u.%u.%u.%u, max image %u bytes", offered[0], offered[1],
		offered[2], offered[3], max_size);

	pdfu_send(PDFU_RSP_INITIATE, p, sizeof(p));
}

/**
 * @param respond false for PDFU_DATA_NR, which the initiator does not expect
 *		  a Response to.
 */
static void pdfu_handle_data(const uint8_t *data, uint32_t len, bool respond)
{
	uint8_t p[5] = {0};
	uint16_t index;
	uint32_t block_len;
	int status = PDFU_OK;

	if (len < 2) {
		LOG_WRN("PDFU_DATA payload short (%u)", len);
		status = PDFU_ERR_UNKNOWN;
		goto reply;
	}

	index = sys_get_le16(data);
	block_len = len - 2;

	/*
	 * The block just written, again: its Response was lost and the
	 * initiator is asking a second time. Acknowledge without writing it
	 * twice, so a dropped message costs a retry rather than the update.
	 */
	if (pdfu.data_started && index + 1 == pdfu.next_block &&
	    (pdfu.phase == PDFU_PHASE_TRANSFER || pdfu.phase == PDFU_PHASE_VALIDATION)) {
		LOG_WRN("data block %u repeated, acknowledging again", index);
		goto reply;
	}

	/* Past the short block, only a repeat of it or an empty block is legal. */
	if (pdfu.phase == PDFU_PHASE_VALIDATION && block_len > 0) {
		status = PDFU_ERR_UNEXPECTED_REQUEST;
		goto reply;
	}

	/*
	 * PDFU_DATA and PDFU_DATA_NR are not retried when GoodCRC is missed,
	 * so the index has to be checked on every block rather than trusted
	 * to arrive in order (section 4.4).
	 */
	if (index != pdfu.next_block) {
		LOG_WRN("data block %u arrived, expected %u", index, pdfu.next_block);
		status = PDFU_ERR_ADDRESS;
		goto reply;
	}

	if (block_len > PDFU_DATA_BLOCK_LEN || pdfu.bytes + block_len > pdfu.max_image_size) {
		LOG_ERR("data block %u would overrun the image", index);
		status = PDFU_ERR_ADDRESS;
		goto reply;
	}

	pdfu.data_started = true;
	pdfu.phase = PDFU_PHASE_TRANSFER;

	if (block_len > 0) {
		status = pdfu.ops->write(pdfu.bytes, &data[2], block_len);
		if (status != 0) {
			LOG_ERR("write of block %u failed, status %02x", index, status);
			goto reply;
		}

		pdfu.bytes += block_len;
		pdfu.next_block = index + 1;
	}

	/*
	 * A short block ends the transfer; a zero length block means the
	 * initiator thinks it is done. Either way the next legal request is
	 * PDFU_VALIDATE, so move on.
	 */
	if (block_len < PDFU_DATA_BLOCK_LEN) {
		if (pdfu.bytes == 0) {
			status = PDFU_ERR_NOTDONE;
			goto reply;
		}

		LOG_INF("transfer complete, %u bytes in %u blocks", pdfu.bytes, pdfu.next_block);
		pdfu.phase = PDFU_PHASE_VALIDATION;
	}

reply:
	if (!respond) {
		return;
	}

	p[0] = status;
	if (status != PDFU_OK) {
		p[1] = PDFU_WAIT_STOP;
		p[2] = 0;
		sys_put_le16(0, &p[3]);
	} else {
		/*
		 * Ready immediately. The write already happened inline, inside
		 * the 27 ms tPDFUResponseSent budget, which is why struct
		 * pdfu_ops asks for writes that return promptly.
		 */
		p[1] = 0;
		/* No PDFU_DATA_NR: every block is acknowledged for now. */
		p[2] = 0;
		sys_put_le16(pdfu.next_block, &p[3]);
	}

	pdfu_send(PDFU_RSP_DATA, p, sizeof(p));
}

static void pdfu_handle_validate(void)
{
	uint8_t p[2] = {0};
	int status;

	status = pdfu.ops->validate(pdfu.bytes);

	p[0] = (status == 0) ? PDFU_OK : status;
	/*
	 * WaitTime here is how long the initiator should wait before assuming
	 * the update completed. We reboot, so tell it not to wait for us.
	 */
	p[1] = (status == 0) ? PDFU_WAIT_STOP : PDFU_WAIT_STOP;

	pdfu_send(PDFU_RSP_VALIDATE, p, sizeof(p));

	if (status != 0) {
		LOG_ERR("validation failed, status %02x", status);
		pdfu_reset("validation failed");
		return;
	}

	LOG_INF("validated %u bytes, entering manifestation", pdfu.bytes);
	pdfu.phase = PDFU_PHASE_MANIFESTATION;

	k_work_schedule(&pdfu_manifest_work, K_MSEC(PDFU_MANIFEST_DELAY_MS));
}

static void pdfu_manifest_work_cb(struct k_work *work)
{
	ARG_UNUSED(work);

	if (pdfu.phase == PDFU_PHASE_MANIFESTATION && pdfu.ops->manifest != NULL) {
		pdfu.ops->manifest();
	}
}

bool pdfu_responder_msg(const struct device *port, enum pd_ext_msg_type type, const uint8_t *data,
			uint32_t len)
{
	uint8_t msg_type;

	if (pdfu.ops == NULL || port != pdfu.port) {
		return false;
	}

	if (type != PD_EXT_FIRMWARE_UPDATE_REQUEST) {
		return false;
	}

	if (len < 2) {
		LOG_WRN("runt PDFU message (%u bytes)", len);
		return true;
	}

	if (data[0] != PDFU_PROTOCOL_V1_0) {
		LOG_WRN("unsupported PDFU protocol version %02x", data[0]);
		return true;
	}

	msg_type = data[1];
	data += 2;
	len -= 2;

	/* Table 5-32, by phase. */
	switch (msg_type) {
	case PDFU_REQ_GET_FW_ID:
		if (pdfu.phase == PDFU_PHASE_ENUMERATION) {
			pdfu_handle_get_fw_id();
		} else {
			pdfu_unexpected(PDFU_RSP_GET_FW_ID);
		}
		break;

	case PDFU_REQ_INITIATE:
		/*
		 * Valid in Enumeration, and in Transfer as long as no data has
		 * arrived yet, which lets an initiator restart cleanly.
		 */
		if (pdfu.phase == PDFU_PHASE_ENUMERATION ||
		    (pdfu.phase == PDFU_PHASE_TRANSFER && !pdfu.data_started) ||
		    pdfu.phase == PDFU_PHASE_RECONFIGURATION) {
			pdfu_handle_initiate(data, len);
		} else {
			pdfu_unexpected(PDFU_RSP_INITIATE);
		}
		break;

	case PDFU_REQ_DATA:
		/*
		 * Reconfiguration is instantaneous for us, so the first
		 * PDFU_DATA is treated as the start of the Transfer Phase.
		 */
		if (pdfu.phase == PDFU_PHASE_RECONFIGURATION || pdfu.phase == PDFU_PHASE_TRANSFER ||
		    pdfu.phase == PDFU_PHASE_VALIDATION) {
			pdfu_handle_data(data, len, true);
		} else {
			pdfu_unexpected(PDFU_RSP_DATA);
		}
		break;

	case PDFU_REQ_DATA_NR:
		/* Ignored outside Transfer, never answered. */
		if (pdfu.phase == PDFU_PHASE_TRANSFER) {
			pdfu_handle_data(data, len, false);
		}
		break;

	case PDFU_REQ_VALIDATE:
		/*
		 * An image that is a whole number of blocks has no short block
		 * to end it, and initiators go straight from the last full
		 * block to PDFU_VALIDATE. The initiator knows
		 * where the image ends, so take its word for it.
		 */
		if (pdfu.phase == PDFU_PHASE_TRANSFER && pdfu.data_started) {
			LOG_INF("transfer complete, %u bytes in %u blocks", pdfu.bytes,
				pdfu.next_block);
			pdfu.phase = PDFU_PHASE_VALIDATION;
		}

		if (pdfu.phase == PDFU_PHASE_VALIDATION) {
			pdfu_handle_validate();
		} else {
			pdfu_unexpected(PDFU_RSP_VALIDATE);
		}
		break;

	case PDFU_REQ_ABORT:
		/* Expected in every phase and never answered. */
		pdfu_reset("initiator aborted");
		break;

	case PDFU_REQ_DATA_PAUSE: {
		uint8_t p[2];

		if (pdfu.phase != PDFU_PHASE_TRANSFER) {
			/* Ignored outside Transfer. */
			break;
		}

		/*
		 * Pausing is refused. Honouring it would mean holding a
		 * half-written image open across an unbounded gap, and the
		 * initiator can simply abort and restart instead.
		 */
		p[0] = PDFU_ERR_REJECT_PAUSE;
		p[1] = PDFU_WAIT_STOP;
		pdfu_send(PDFU_RSP_DATA_PAUSE, p, sizeof(p));
		break;
	}

	case PDFU_REQ_VENDOR:
		pdfu_unexpected(PDFU_RSP_VENDOR);
		break;

	default:
		LOG_WRN("unknown PDFU request %02x", msg_type);
		pdfu_unexpected(PDFU_RSP_GET_FW_ID);
		break;
	}

	return true;
}

int pdfu_responder_init(const struct device *port, const struct pdfu_ops *ops)
{
	if (port == NULL || ops == NULL || ops->get_fw_id == NULL || ops->begin == NULL ||
	    ops->write == NULL || ops->validate == NULL) {
		return -EINVAL;
	}

	pdfu.port = port;
	pdfu.ops = ops;
	pdfu.phase = PDFU_PHASE_ENUMERATION;
	pdfu.bytes = 0;
	pdfu.next_block = 0;
	pdfu.data_started = false;

	LOG_INF("PDFU responder ready on %s", port->name);

	return 0;
}
