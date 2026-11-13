/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "canopen_sdo.h"

#define SDO_CCS_DOWNLOAD_SEGMENT  0U
#define SDO_CCS_DOWNLOAD_INITIATE 1U
#define SDO_CCS_UPLOAD_INITIATE   2U
#define SDO_CCS_UPLOAD_SEGMENT    3U
#define SDO_CS_ABORT              4U

#define SDO_SCS_UPLOAD_SEGMENT    0x00U
#define SDO_SCS_DOWNLOAD_SEGMENT  0x20U
#define SDO_SCS_UPLOAD_INITIATE   0x40U
#define SDO_SCS_DOWNLOAD_INITIATE 0x60U

#define SDO_SIZE_INDICATED BIT(0)
#define SDO_EXPEDITED      BIT(1)
#define SDO_LAST           BIT(0)
#define SDO_TOGGLE         BIT(4)

#define SDO_EXPEDITED_DATA 4U

void canopen_sdo_send(struct canopen_node *node, const uint8_t *data)
{
	struct can_frame frame = {
		.id = node->sdo_cob_id[1],
		.dlc = SDO_FRAME_LEN,
	};

	memcpy(frame.data, data, SDO_FRAME_LEN);
	(void)canopen_send(node, &frame);
}

void canopen_sdo_abort(struct canopen_node *node, uint16_t index, uint8_t sub, uint32_t code)
{
	uint8_t data[SDO_FRAME_LEN] = {SDO_ABORT};

	canopen_sdo_put_mux(data, index, sub);
	sys_put_le32(code, &data[4]);
	canopen_sdo_send(node, data);
	node->sdo.state = SDO_IDLE;
}

void canopen_sdo_start(struct canopen_sdo_server *sdo, enum sdo_state state, uint16_t index,
		       uint8_t sub, size_t size)
{
	sdo->state = state;
	sdo->index = index;
	sdo->sub = sub;
	sdo->size = size;
	sdo->pos = 0U;
	sdo->toggle = 0U;
	canopen_sdo_rearm(sdo);
}

static void sdo_next(struct canopen_sdo_server *sdo)
{
	sdo->toggle ^= SDO_TOGGLE;
	canopen_sdo_rearm(sdo);
}

static void sdo_download_initiate(struct canopen_node *node, const uint8_t *req)
{
	struct canopen_sdo_server *sdo = &node->sdo;
	uint8_t resp[SDO_FRAME_LEN] = {SDO_SCS_DOWNLOAD_INITIATE};
	uint16_t index = sys_get_le16(&req[1]);
	uint8_t sub = req[3];
	struct canopen_od_ref ref;
	size_t size = 0U;
	uint32_t sdo_abort;

	sdo->state = SDO_IDLE;
	sdo_abort = canopen_sdo_find(node, index, sub, true, &ref);

	if ((sdo_abort == CANOPEN_SDO_ABORT_NONE) && ((req[0] & SDO_EXPEDITED) != 0U)) {
		if ((req[0] & SDO_SIZE_INDICATED) != 0U) {
			size = SDO_EXPEDITED_DATA - ((req[0] >> 2) & 0x3U);
		} else {
			size = MIN(canopen_od_wire_size(&ref), SDO_EXPEDITED_DATA);
		}
		sdo_abort = canopen_od_write_wire(node, &ref, &req[4], size);
	} else if (sdo_abort == CANOPEN_SDO_ABORT_NONE) {
		if ((req[0] & SDO_SIZE_INDICATED) != 0U) {
			size = sys_get_le32(&req[4]);
		}

		if (size > canopen_od_wire_size(&ref)) {
			sdo_abort = CANOPEN_SDO_ABORT_LENGTH_TOO_HIGH;
		} else if (size > sizeof(sdo->buf)) {
			sdo_abort = CANOPEN_SDO_ABORT_OUT_OF_MEMORY;
		} else {
			canopen_sdo_start(sdo, SDO_DOWNLOAD, index, sub, size);
		}
	}

	if (sdo_abort != CANOPEN_SDO_ABORT_NONE) {
		canopen_sdo_abort(node, index, sub, sdo_abort);
		return;
	}

	canopen_sdo_put_mux(resp, index, sub);
	canopen_sdo_send(node, resp);
}

static void sdo_download_segment(struct canopen_node *node, const uint8_t *req)
{
	struct canopen_sdo_server *sdo = &node->sdo;
	uint8_t resp[SDO_FRAME_LEN] = {SDO_SCS_DOWNLOAD_SEGMENT | (req[0] & SDO_TOGGLE)};
	size_t len = SDO_SEGMENT_DATA - ((req[0] >> 1) & 0x7U);
	struct canopen_od_ref ref;
	uint32_t sdo_abort = CANOPEN_SDO_ABORT_NONE;

	if ((req[0] & SDO_TOGGLE) != sdo->toggle) {
		sdo_abort = CANOPEN_SDO_ABORT_TOGGLE;
	} else if ((sdo->pos + len) > sizeof(sdo->buf)) {
		sdo_abort = CANOPEN_SDO_ABORT_OUT_OF_MEMORY;
	} else {
		memcpy(&sdo->buf[sdo->pos], &req[1], len);
		sdo->pos += len;
	}

	if ((sdo_abort == CANOPEN_SDO_ABORT_NONE) && ((req[0] & SDO_LAST) != 0U)) {
		if ((sdo->size != 0U) && (sdo->pos != sdo->size)) {
			sdo_abort = (sdo->pos > sdo->size) ? CANOPEN_SDO_ABORT_LENGTH_TOO_HIGH
						       : CANOPEN_SDO_ABORT_LENGTH_TOO_LOW;
		} else {
			sdo_abort = canopen_od_find(node, sdo->index, sdo->sub, &ref);
		}

		if (sdo_abort == CANOPEN_SDO_ABORT_NONE) {
			sdo_abort = canopen_od_write_wire(node, &ref, sdo->buf, sdo->pos);
		}

		sdo->state = SDO_IDLE;
	} else if (sdo_abort == CANOPEN_SDO_ABORT_NONE) {
		sdo_next(sdo);
	}

	if (sdo_abort != CANOPEN_SDO_ABORT_NONE) {
		canopen_sdo_abort(node, sdo->index, sdo->sub, sdo_abort);
		return;
	}

	canopen_sdo_send(node, resp);
}

static void sdo_upload_initiate(struct canopen_node *node, const uint8_t *req)
{
	struct canopen_sdo_server *sdo = &node->sdo;
	uint8_t resp[SDO_FRAME_LEN] = {0};
	uint16_t index = sys_get_le16(&req[1]);
	uint8_t sub = req[3];
	struct canopen_od_ref ref;
	size_t len = 0U;
	uint32_t sdo_abort;

	sdo->state = SDO_IDLE;
	sdo_abort = canopen_sdo_find(node, index, sub, false, &ref);
	if (sdo_abort == CANOPEN_SDO_ABORT_NONE) {
		sdo_abort = canopen_od_read_wire(&ref, sdo->buf, sizeof(sdo->buf), &len);
	}

	if (sdo_abort != CANOPEN_SDO_ABORT_NONE) {
		canopen_sdo_abort(node, index, sub, sdo_abort);
		return;
	}

	canopen_sdo_put_mux(resp, index, sub);

	if ((len > 0U) && (len <= SDO_EXPEDITED_DATA)) {
		resp[0] = SDO_SCS_UPLOAD_INITIATE | ((SDO_EXPEDITED_DATA - len) << 2) |
			  SDO_EXPEDITED | SDO_SIZE_INDICATED;
		memcpy(&resp[4], sdo->buf, len);
	} else {
		resp[0] = SDO_SCS_UPLOAD_INITIATE | SDO_SIZE_INDICATED;
		sys_put_le32(len, &resp[4]);
		canopen_sdo_start(sdo, SDO_UPLOAD, index, sub, len);
	}

	canopen_sdo_send(node, resp);
}

static void sdo_upload_segment(struct canopen_node *node, const uint8_t *req)
{
	struct canopen_sdo_server *sdo = &node->sdo;
	uint8_t resp[SDO_FRAME_LEN] = {0};
	size_t len = MIN(sdo->size - sdo->pos, SDO_SEGMENT_DATA);

	if ((req[0] & SDO_TOGGLE) != sdo->toggle) {
		canopen_sdo_abort(node, sdo->index, sdo->sub, CANOPEN_SDO_ABORT_TOGGLE);
		return;
	}

	resp[0] = SDO_SCS_UPLOAD_SEGMENT | sdo->toggle | ((SDO_SEGMENT_DATA - len) << 1);
	memcpy(&resp[1], &sdo->buf[sdo->pos], len);
	sdo->pos += len;

	if (sdo->pos == sdo->size) {
		resp[0] |= SDO_LAST;
		sdo->state = SDO_IDLE;
	} else {
		sdo_next(sdo);
	}

	canopen_sdo_send(node, resp);
}

void canopen_sdo_server_rx(struct canopen_node *node, const struct can_frame *frame)
{
	const uint8_t *req = frame->data;

	if (frame->dlc != SDO_FRAME_LEN) {
		return;
	}

	switch (req[0] >> 5) {
	case SDO_CCS_DOWNLOAD_INITIATE:
		sdo_download_initiate(node, req);
		break;
	case SDO_CCS_DOWNLOAD_SEGMENT:
		if (node->sdo.state == SDO_DOWNLOAD) {
			sdo_download_segment(node, req);
		} else {
			canopen_sdo_abort(node, node->sdo.index, node->sdo.sub,
					  CANOPEN_SDO_ABORT_COMMAND);
		}
		break;
	case SDO_CCS_UPLOAD_INITIATE:
		sdo_upload_initiate(node, req);
		break;
	case SDO_CCS_UPLOAD_SEGMENT:
		if (node->sdo.state == SDO_UPLOAD) {
			sdo_upload_segment(node, req);
		} else {
			canopen_sdo_abort(node, node->sdo.index, node->sdo.sub,
					  CANOPEN_SDO_ABORT_COMMAND);
		}
		break;
	case SDO_CS_ABORT:
		node->sdo.state = SDO_IDLE;
		break;
	default:
		canopen_sdo_abort(node, sys_get_le16(&req[1]), req[3], CANOPEN_SDO_ABORT_COMMAND);
		break;
	}
}

void canopen_sdo_server_reset(struct canopen_node *node)
{
	node->sdo.state = SDO_IDLE;
}

void canopen_sdo_server_process(struct canopen_node *node)
{
	if ((node->sdo.state != SDO_IDLE) && sys_timepoint_expired(node->sdo.deadline)) {
		canopen_sdo_abort(node, node->sdo.index, node->sdo.sub, CANOPEN_SDO_ABORT_TIMEOUT);
	}
}

k_timepoint_t canopen_sdo_server_deadline(struct canopen_node *node)
{
	return (node->sdo.state == SDO_IDLE) ? sys_timepoint_calc(K_FOREVER) : node->sdo.deadline;
}
