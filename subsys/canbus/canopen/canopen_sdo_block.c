/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>

#include "canopen_sdo.h"

#define SDO_CCS_BLOCK_UPLOAD   5U
#define SDO_CCS_BLOCK_DOWNLOAD 6U

#define SDO_SCS_BLOCK_DOWNLOAD 0xA0U
#define SDO_SCS_BLOCK_UPLOAD   0xC0U

#define SDO_BLOCK_CS_MASK  0x03U
#define SDO_BLOCK_INITIATE 0U
#define SDO_BLOCK_END      1U
#define SDO_BLOCK_ACK      2U
#define SDO_BLOCK_START    3U

#define SDO_BLOCK_CRC      BIT(2)
#define SDO_BLOCK_SIZE_IND BIT(1)
#define SDO_BLOCK_LAST     BIT(7)
#define SDO_BLOCK_SEQNO    0x7FU
#define SDO_BLKSIZE_MAX    127U

static bool sdo_block_domain(const struct canopen_sdo_server *sdo)
{
	return IS_ENABLED(CONFIG_CANOPEN_DOMAIN) && (sdo->domain != NULL);
}

static bool sdo_blksize_valid(uint8_t blksize)
{
	return (blksize > 0U) && (blksize <= SDO_BLKSIZE_MAX);
}

static void sdo_block_begin(struct canopen_sdo_server *sdo, uint8_t cmd, uint8_t blksize)
{
	sdo->block_pos = 0U;
	sdo->crc_pos = 0U;
	sdo->crc = 0U;
	sdo->blksize = blksize;
	sdo->seqno = 0U;
	sdo->crc_on = (cmd & SDO_BLOCK_CRC) != 0U;
}

static void sdo_block_crc(struct canopen_sdo_server *sdo, const uint8_t *data, size_t len)
{
	if (sdo->crc_on) {
		sdo->crc = crc16_itu_t(sdo->crc, data, len);
	}
}

static void sdo_block_download_initiate(struct canopen_node *node, const uint8_t *req)
{
	struct canopen_sdo_server *sdo = &node->sdo;
	uint8_t resp[SDO_FRAME_LEN] = {SDO_SCS_BLOCK_DOWNLOAD | SDO_BLOCK_CRC};
	uint16_t index = sys_get_le16(&req[1]);
	uint8_t sub = req[3];
	const struct canopen_od_entry *domain = NULL;
	struct canopen_od_ref ref;
	size_t size = 0U;
	uint32_t abort;

	sdo->state = SDO_IDLE;
	if ((req[0] & SDO_BLOCK_SIZE_IND) != 0U) {
		size = sys_get_le32(&req[4]);
	}

	abort = canopen_sdo_find(node, index, sub, true, &ref);
	if ((abort == CANOPEN_SDO_ABORT_NONE) && IS_ENABLED(CONFIG_CANOPEN_DOMAIN) &&
	    (ref.type == CANOPEN_OD_TYPE_DOMAIN)) {
		domain = ref.entry;
		abort = sdo_domain(domain)->write(domain, 0U, &req[4], 0U, size, false,
						  domain->user_data);
	} else if (abort == CANOPEN_SDO_ABORT_NONE) {
		if (size > canopen_od_wire_size(&ref)) {
			abort = CANOPEN_SDO_ABORT_LENGTH_TOO_HIGH;
		} else if (size > sizeof(sdo->buf)) {
			abort = CANOPEN_SDO_ABORT_OUT_OF_MEMORY;
		}
	}

	if (abort != CANOPEN_SDO_ABORT_NONE) {
		canopen_sdo_abort(node, index, sub, abort);
		return;
	}

	canopen_sdo_start(sdo, SDO_BLOCK_DOWNLOAD, index, sub, size);
	sdo->domain = domain;
	sdo_block_begin(sdo, req[0], CONFIG_CANOPEN_SDO_BLOCK_SIZE);

	canopen_sdo_put_mux(resp, index, sub);
	resp[4] = sdo->blksize;
	canopen_sdo_send(node, resp);
}

static uint32_t sdo_block_store(struct canopen_sdo_server *sdo, const uint8_t *data)
{
	const struct canopen_od_entry *entry = sdo->domain;
	uint32_t abort = CANOPEN_SDO_ABORT_NONE;

	if ((sdo->size != 0U) && ((sdo->pos + SDO_SEGMENT_DATA) > sdo->size)) {
		abort = CANOPEN_SDO_ABORT_LENGTH_TOO_HIGH;
	} else if (sdo_block_domain(sdo)) {
		abort = sdo_domain(entry)->write(entry, sdo->pos, data, SDO_SEGMENT_DATA, sdo->size,
						 false, entry->user_data);
	} else if ((sdo->pos + SDO_SEGMENT_DATA) > sizeof(sdo->buf)) {
		abort = CANOPEN_SDO_ABORT_OUT_OF_MEMORY;
	} else {
		memcpy(&sdo->buf[sdo->pos], data, SDO_SEGMENT_DATA);
	}

	if (abort == CANOPEN_SDO_ABORT_NONE) {
		sdo_block_crc(sdo, data, SDO_SEGMENT_DATA);
		sdo->pos += SDO_SEGMENT_DATA;
	}

	return abort;
}

static void sdo_block_hold(struct canopen_sdo_server *sdo, const uint8_t *data)
{
	size_t room = sizeof(sdo->buf) - sdo->pos;

	if (sdo_block_domain(sdo)) {
		memcpy(sdo->buf, data, SDO_SEGMENT_DATA);
	} else {
		memcpy(&sdo->buf[sdo->pos], data, MIN(SDO_SEGMENT_DATA, room));
	}

	sdo->held = true;
}

static void sdo_block_download_segment(struct canopen_node *node, const uint8_t *req)
{
	struct canopen_sdo_server *sdo = &node->sdo;
	uint8_t resp[SDO_FRAME_LEN] = {SDO_SCS_BLOCK_DOWNLOAD | SDO_BLOCK_ACK};
	uint8_t seqno = req[0] & SDO_BLOCK_SEQNO;
	bool last = (req[0] & SDO_BLOCK_LAST) != 0U;
	uint32_t abort = CANOPEN_SDO_ABORT_NONE;

	if (req[0] == SDO_ABORT) {
		sdo->state = SDO_IDLE;
		return;
	}

	if ((seqno == 0U) || (seqno > sdo->blksize)) {
		abort = CANOPEN_SDO_ABORT_SEQUENCE;
	} else if (seqno == (sdo->seqno + 1U)) {
		sdo->seqno = seqno;
		if (last) {
			sdo_block_hold(sdo, &req[1]);
		} else {
			abort = sdo_block_store(sdo, &req[1]);
		}
	}

	if (abort != CANOPEN_SDO_ABORT_NONE) {
		canopen_sdo_abort(node, sdo->index, sdo->sub, abort);
		return;
	}

	canopen_sdo_rearm(sdo);
	if (!last && (seqno < sdo->blksize)) {
		return;
	}

	resp[1] = sdo->seqno;
	resp[2] = sdo->blksize;
	sdo->seqno = 0U;
	if (sdo->held) {
		sdo->state = SDO_BLOCK_DOWNLOAD_END;
	}

	canopen_sdo_send(node, resp);
}

static void sdo_block_download_end(struct canopen_node *node, const uint8_t *req)
{
	struct canopen_sdo_server *sdo = &node->sdo;
	const struct canopen_od_entry *entry = sdo->domain;
	uint8_t resp[SDO_FRAME_LEN] = {SDO_SCS_BLOCK_DOWNLOAD | SDO_BLOCK_END};
	bool domain = sdo_block_domain(sdo);
	uint8_t *data = domain ? sdo->buf : &sdo->buf[sdo->pos];
	size_t len = SDO_SEGMENT_DATA - ((req[0] >> 2) & 0x7U);
	size_t total = sdo->pos + len;
	struct canopen_od_ref ref;
	uint32_t abort = CANOPEN_SDO_ABORT_NONE;

	if ((sdo->size != 0U) && (total != sdo->size)) {
		abort = (total > sdo->size) ? CANOPEN_SDO_ABORT_LENGTH_TOO_HIGH
					    : CANOPEN_SDO_ABORT_LENGTH_TOO_LOW;
	} else if (!domain && (total > sizeof(sdo->buf))) {
		abort = CANOPEN_SDO_ABORT_OUT_OF_MEMORY;
	} else {
		sdo_block_crc(sdo, data, len);
		if (sdo->crc_on && (sdo->crc != sys_get_le16(&req[1]))) {
			abort = CANOPEN_SDO_ABORT_CRC;
		}
	}

	if ((abort == CANOPEN_SDO_ABORT_NONE) && domain) {
		abort = sdo_domain(entry)->write(entry, sdo->pos, data, len, sdo->size, true,
						 entry->user_data);
	} else if (abort == CANOPEN_SDO_ABORT_NONE) {
		abort = canopen_od_find(node, sdo->index, sdo->sub, &ref);
		if (abort == CANOPEN_SDO_ABORT_NONE) {
			abort = canopen_od_write_wire(node, &ref, sdo->buf, total);
		}
	}

	if (abort != CANOPEN_SDO_ABORT_NONE) {
		canopen_sdo_abort(node, sdo->index, sdo->sub, abort);
		return;
	}

	sdo->state = SDO_IDLE;
	canopen_sdo_send(node, resp);
}

static void sdo_block_upload_initiate(struct canopen_node *node, const uint8_t *req)
{
	struct canopen_sdo_server *sdo = &node->sdo;
	uint8_t resp[SDO_FRAME_LEN] = {SDO_SCS_BLOCK_UPLOAD | SDO_BLOCK_CRC};
	uint16_t index = sys_get_le16(&req[1]);
	uint8_t sub = req[3];
	const struct canopen_od_entry *domain = NULL;
	struct canopen_od_ref ref;
	size_t len = 0U;
	uint32_t abort = CANOPEN_SDO_ABORT_BLOCK_SIZE;

	sdo->state = SDO_IDLE;
	if (sdo_blksize_valid(req[4])) {
		abort = canopen_sdo_find(node, index, sub, false, &ref);
	}

	if ((abort == CANOPEN_SDO_ABORT_NONE) && IS_ENABLED(CONFIG_CANOPEN_DOMAIN) &&
	    (ref.type == CANOPEN_OD_TYPE_DOMAIN)) {
		domain = ref.entry;
	} else if (abort == CANOPEN_SDO_ABORT_NONE) {
		abort = canopen_od_read_wire(&ref, sdo->buf, sizeof(sdo->buf), &len);
		resp[0] |= SDO_BLOCK_SIZE_IND;
		sys_put_le32(len, &resp[4]);
	}

	if (abort != CANOPEN_SDO_ABORT_NONE) {
		canopen_sdo_abort(node, index, sub, abort);
		return;
	}

	canopen_sdo_start(sdo, SDO_BLOCK_UPLOAD_START, index, sub, len);
	sdo->domain = domain;
	sdo_block_begin(sdo, req[0], req[4]);

	canopen_sdo_put_mux(resp, index, sub);
	canopen_sdo_send(node, resp);
}

static void sdo_block_upload_fill(struct canopen_node *node)
{
	struct canopen_sdo_server *sdo = &node->sdo;
	const struct canopen_od_entry *entry = sdo->domain;

	while ((sdo->state == SDO_BLOCK_UPLOAD) && !sdo->held && (sdo->seqno < sdo->blksize) &&
	       (k_msgq_num_used_get(&node->txq) == 0U)) {
		uint8_t resp[SDO_FRAME_LEN] = {0};
		size_t len = SDO_SEGMENT_DATA;
		bool last;

		if (sdo_block_domain(sdo)) {
			uint32_t abort = sdo_domain(entry)->read(entry, sdo->pos, &resp[1], &len,
								 entry->user_data);

			if (abort != CANOPEN_SDO_ABORT_NONE) {
				canopen_sdo_abort(node, sdo->index, sdo->sub, abort);
				return;
			}

			last = len < SDO_SEGMENT_DATA;
		} else {
			len = MIN(sdo->size - sdo->pos, SDO_SEGMENT_DATA);
			memcpy(&resp[1], &sdo->buf[sdo->pos], len);
			last = (sdo->pos + len) == sdo->size;
		}

		if (sdo->pos == sdo->crc_pos) {
			sdo_block_crc(sdo, &resp[1], len);
			sdo->crc_pos += len;
		}

		sdo->pos += len;
		sdo->seqno++;
		resp[0] = sdo->seqno;
		if (last) {
			resp[0] |= SDO_BLOCK_LAST;
			sdo->held = true;
			sdo->last_len = (uint8_t)len;
		}

		canopen_sdo_send(node, resp);
	}
}

static void sdo_block_upload_ack(struct canopen_node *node, const uint8_t *req)
{
	struct canopen_sdo_server *sdo = &node->sdo;
	uint8_t resp[SDO_FRAME_LEN] = {0};
	uint8_t ackseq = req[1];

	if (ackseq > sdo->seqno) {
		canopen_sdo_abort(node, sdo->index, sdo->sub, CANOPEN_SDO_ABORT_SEQUENCE);
		return;
	}

	if (!sdo_blksize_valid(req[2])) {
		canopen_sdo_abort(node, sdo->index, sdo->sub, CANOPEN_SDO_ABORT_BLOCK_SIZE);
		return;
	}

	canopen_sdo_rearm(sdo);
	sdo->blksize = req[2];

	if ((ackseq == sdo->seqno) && sdo->held) {
		resp[0] = SDO_SCS_BLOCK_UPLOAD | ((SDO_SEGMENT_DATA - sdo->last_len) << 2) |
			  SDO_BLOCK_END;
		sys_put_le16(sdo->crc_on ? sdo->crc : 0U, &resp[1]);
		sdo->state = SDO_BLOCK_UPLOAD_END;
		canopen_sdo_send(node, resp);
		return;
	}

	sdo->pos = sdo->block_pos + ((size_t)ackseq * SDO_SEGMENT_DATA);
	sdo->block_pos = sdo->pos;
	sdo->seqno = 0U;
	sdo->held = false;
	sdo_block_upload_fill(node);
}

static void sdo_block_command(struct canopen_node *node, const uint8_t *req)
{
	struct canopen_sdo_server *sdo = &node->sdo;
	uint8_t ccs = req[0] >> 5;
	uint8_t cs = req[0] & SDO_BLOCK_CS_MASK;

	if (ccs == SDO_CCS_BLOCK_DOWNLOAD) {
		if ((req[0] & SDO_BLOCK_END) == 0U) {
			sdo_block_download_initiate(node, req);
		} else if (sdo->state == SDO_BLOCK_DOWNLOAD_END) {
			sdo_block_download_end(node, req);
		} else {
			canopen_sdo_abort(node, sdo->index, sdo->sub, CANOPEN_SDO_ABORT_COMMAND);
		}
	} else if (cs == SDO_BLOCK_INITIATE) {
		sdo_block_upload_initiate(node, req);
	} else if ((cs == SDO_BLOCK_START) && (sdo->state == SDO_BLOCK_UPLOAD_START)) {
		sdo->state = SDO_BLOCK_UPLOAD;
		canopen_sdo_rearm(sdo);
		sdo_block_upload_fill(node);
	} else if ((cs == SDO_BLOCK_ACK) && (sdo->state == SDO_BLOCK_UPLOAD)) {
		sdo_block_upload_ack(node, req);
	} else if ((cs == SDO_BLOCK_END) && (sdo->state == SDO_BLOCK_UPLOAD_END)) {
		sdo->state = SDO_IDLE;
	} else {
		canopen_sdo_abort(node, sdo->index, sdo->sub, CANOPEN_SDO_ABORT_COMMAND);
	}
}

bool canopen_sdo_block_rx(struct canopen_node *node, const uint8_t *req)
{
	uint8_t ccs = req[0] >> 5;

	if (node->sdo.state == SDO_BLOCK_DOWNLOAD) {
		sdo_block_download_segment(node, req);
		return true;
	}

	if ((ccs != SDO_CCS_BLOCK_UPLOAD) && (ccs != SDO_CCS_BLOCK_DOWNLOAD)) {
		return false;
	}

	sdo_block_command(node, req);

	return true;
}

void canopen_sdo_block_process(struct canopen_node *node)
{
	sdo_block_upload_fill(node);
}
