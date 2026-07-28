/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/nfc/t2t.h>

#include "common/poller.h"
#include "protocol/iso14443a.h"
#include "tag/tag_internal.h"

LOG_MODULE_REGISTER(nfc_t2t, CONFIG_NFC_LOG_LEVEL);

#define T2T_CMD_READ  0x30U
#define T2T_CMD_WRITE 0xA2U
#define T2T_ACK       0x0AU

#define T2T_CC_BLOCK         3U
#define T2T_CC_MAGIC         0xE1U
#define T2T_DATA_BLOCK_START 4U
#define T2T_BLOCK_SIZE       4U

#define T2T_TLV_NULL       0x00U
#define T2T_TLV_NDEF       0x03U
#define T2T_TLV_TERMINATOR 0xFEU

#define T2T_DEFAULT_TIMEOUT_US 20000U
#define T2T_TLV_HDR_LEN        2U

static int t2t_tlv_length(const uint8_t *data, uint16_t len, uint16_t *i, uint16_t *out)
{
	uint16_t l;

	if (*i >= len) {
		return -EAGAIN;
	}

	l = data[(*i)++];
	if (l == 0xFFU) {
		if (*i + 2U > len) {
			return -EAGAIN;
		}
		l = sys_get_be16(&data[*i]);
		*i += 2U;
	}

	*out = l;

	return 0;
}

int z_nfc_t2t_find_ndef(const uint8_t *data, uint16_t len, uint16_t *off, uint16_t *ndef_len)
{
	uint16_t i = 0;

	while (i < len) {
		uint8_t t = data[i++];
		uint16_t l;
		int ret;

		if (t == T2T_TLV_NULL) {
			continue;
		}
		if (t == T2T_TLV_TERMINATOR) {
			return -ENOENT;
		}

		ret = t2t_tlv_length(data, len, &i, &l);
		if (ret < 0) {
			return ret;
		}

		if (t == T2T_TLV_NDEF) {
			*off = i;
			*ndef_len = l;

			return ((uint32_t)i + l > len) ? -EAGAIN : 0;
		}

		if ((uint32_t)i + l > len) {
			return -EAGAIN;
		}
		i += l;
	}

	return -EAGAIN;
}

int nfc_t2t_read_block(struct nfc_t2t_tag *t2t, uint8_t block, uint8_t out[16], k_timeout_t timeout)
{
	uint8_t cmd[2] = {T2T_CMD_READ, block};
	uint8_t rx[18];
	uint16_t rx_len = sizeof(rx);
	uint32_t to_us = z_nfc_timeout_us_cap(sys_timepoint_calc(timeout), T2T_DEFAULT_TIMEOUT_US);
	int ret;

	if (to_us == 0U) {
		return -EAGAIN;
	}

	const struct z_nfca_xfer xfer = {.tx_crc = true, .rx_crc = true, .timeout_us = to_us};

	z_nfc_poller_lock(t2t->poller);
	ret = z_nfca_transceive(t2t->poller, &t2t->target, cmd, sizeof(cmd), rx, &rx_len, &xfer);
	z_nfc_poller_unlock(t2t->poller);

	if (ret < 0) {
		return ret;
	}
	if (rx_len != 16U) {
		LOG_DBG("READ answered %u bytes, expected 16", rx_len);
		return -EIO;
	}

	memcpy(out, rx, 16U);
	return 0;
}

static bool t2t_ack_is_visible(const struct nfc_t2t_tag *t2t)
{
	return z_nfc_poller_backend(t2t->poller) != Z_NFC_BACKEND_OFFLOAD;
}

int nfc_t2t_write_block(struct nfc_t2t_tag *t2t, uint8_t block, const uint8_t in[4],
			k_timeout_t timeout)
{
	uint8_t cmd[6] = {T2T_CMD_WRITE, block};
	uint8_t rx[4];
	uint16_t rx_len = sizeof(rx);
	uint32_t to_us = z_nfc_timeout_us_cap(sys_timepoint_calc(timeout), T2T_DEFAULT_TIMEOUT_US);
	int ret;

	if (to_us == 0U) {
		return -EAGAIN;
	}

	memcpy(&cmd[2], in, 4U);

	/* The ACK is a 4-bit frame without CRC. */
	const struct z_nfca_xfer xfer = {.tx_crc = true, .rx_crc = false, .timeout_us = to_us};

	z_nfc_poller_lock(t2t->poller);
	ret = z_nfca_transceive(t2t->poller, &t2t->target, cmd, sizeof(cmd), rx, &rx_len, &xfer);
	z_nfc_poller_unlock(t2t->poller);

	if (ret < 0) {
		return ret;
	}
	if (t2t_ack_is_visible(t2t) && (rx_len < 1U || (rx[0] & 0x0FU) != T2T_ACK)) {
		LOG_DBG("WRITE was not acknowledged");
		return -EIO;
	}

	return 0;
}

int z_nfc_t2t_connect(struct nfc_poller *poller, const struct nfc_target *target,
		      struct nfc_t2t_tag *t2t, k_timeout_t timeout)
{
	uint8_t blocks[16];
	int ret;

	if (target == NULL || t2t == NULL) {
		return -EINVAL;
	}
	if (target->tech != NFC_TECH_A) {
		return -ENOTSUP;
	}

	memset(t2t, 0, sizeof(*t2t));
	t2t->poller = poller;
	t2t->target = *target;

	ret = nfc_t2t_read_block(t2t, T2T_CC_BLOCK, blocks, timeout);
	if (ret < 0) {
		return ret;
	}

	/* blocks[] holds blocks 3..6; the CC is the first 4 bytes. */
	if (blocks[0] != T2T_CC_MAGIC) {
		return -EBADMSG;
	}
	t2t->data_size = (uint16_t)blocks[2] * 8U;
	t2t->writable = (blocks[3] & 0x0FU) == 0x00U;

	return 0;
}

static int t2t_find_ndef_incrementally(struct nfc_t2t_tag *t2t, uint8_t *buf, uint16_t cap,
				       uint16_t *off, uint16_t *ndef_len, k_timepoint_t deadline)
{
	uint16_t limit = MIN(t2t->data_size, cap);
	uint8_t block = T2T_DATA_BLOCK_START;
	uint16_t filled = 0U;

	*off = 0U;
	*ndef_len = 0U;

	while (filled < limit) {
		uint8_t chunk[16];
		uint16_t copy;
		int ret;

		ret = nfc_t2t_read_block(t2t, block, chunk, sys_timepoint_timeout(deadline));
		if (ret < 0) {
			return ret;
		}

		copy = MIN((uint16_t)sizeof(chunk), limit - filled);
		memcpy(&buf[filled], chunk, copy);
		filled += copy;

		ret = z_nfc_t2t_find_ndef(buf, filled, off, ndef_len);
		if (ret != -EAGAIN) {
			return ret;
		}
		if (*ndef_len > cap) {
			return -ENOMEM;
		}

		block += 4U;
	}

	return -ENOENT;
}

int z_nfc_t2t_read_ndef(struct nfc_t2t_tag *t2t, uint8_t *buf, uint16_t *len, k_timeout_t timeout)
{
	k_timepoint_t deadline = sys_timepoint_calc(timeout);
	uint16_t cap = *len;
	uint16_t off, ndef_len;
	int ret;

	if (buf == NULL || len == NULL) {
		return -EINVAL;
	}

	ret = t2t_find_ndef_incrementally(t2t, buf, cap, &off, &ndef_len, deadline);
	if (ret < 0) {
		return ret;
	}
	if (ndef_len == 0U) {
		return -ENOENT;
	}

	memmove(buf, &buf[off], ndef_len);
	*len = ndef_len;

	return 0;
}

static void t2t_fill_block(uint8_t *block, uint16_t pos, const uint8_t *buf, uint16_t len)
{
	uint8_t hdr[T2T_TLV_HDR_LEN] = {T2T_TLV_NDEF, (uint8_t)len};

	for (uint8_t i = 0; i < T2T_BLOCK_SIZE; i++) {
		uint16_t idx = pos + i;

		if (idx < T2T_TLV_HDR_LEN) {
			block[i] = hdr[idx];
		} else if (idx < T2T_TLV_HDR_LEN + len) {
			block[i] = buf[idx - T2T_TLV_HDR_LEN];
		} else if (idx == T2T_TLV_HDR_LEN + len) {
			block[i] = T2T_TLV_TERMINATOR;
		} else {
			block[i] = 0x00U;
		}
	}
}

int z_nfc_t2t_write_ndef(struct nfc_t2t_tag *t2t, const uint8_t *buf, uint16_t len,
			 k_timeout_t timeout)
{
	k_timepoint_t deadline = sys_timepoint_calc(timeout);
	uint8_t block[T2T_BLOCK_SIZE];
	uint16_t pos = 0;
	uint8_t blk = T2T_DATA_BLOCK_START;
	int ret;

	if (buf == NULL || len > 0xFEU) {
		/* Only the one-byte TLV length form is supported. */
		return len > 0xFEU ? -ENOTSUP : -EINVAL;
	}
	if (!t2t->writable) {
		return -EACCES;
	}

	while (pos < (uint16_t)(T2T_TLV_HDR_LEN + len + 1U)) {
		t2t_fill_block(block, pos, buf, len);

		ret = nfc_t2t_write_block(t2t, blk, block, sys_timepoint_timeout(deadline));
		if (ret < 0) {
			return ret;
		}
		pos += T2T_BLOCK_SIZE;
		blk++;
	}

	return 0;
}
