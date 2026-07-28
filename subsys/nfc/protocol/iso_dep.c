/*
 * Copyright (c) 2023 Basalte bv
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/nfc.h>
#include <zephyr/logging/log.h>
#include <zephyr/nfc/iso_dep.h>

#include "common/poller.h"
#include "protocol/iso14443a.h"

LOG_MODULE_REGISTER(nfc_iso_dep, CONFIG_NFC_LOG_LEVEL);

static const uint16_t fs_table[] = {16U, 24U, 32U, 40U, 48U, 64U, 96U, 128U, 256U};

static inline uint16_t fsi_to_fs(uint8_t fsi)
{
	return fs_table[MIN(fsi, 8U)];
}

static void iso_dep_ats_defaults(struct nfc_iso_dep_tag *tag)
{
	tag->cid_supported = false;
	tag->cid = 0U;
	tag->nad_supported = false;
	tag->fsci = 2U;
	tag->fwi = 4U;
	tag->sfgi = 0U;
	tag->modes = NFC_MODE_TX_106 | NFC_MODE_RX_106;
	tag->historical_len = 0U;
	tag->block_num = 0U;
}

static int iso_dep_ats_ta(struct nfc_iso_dep_tag *tag, uint8_t ta)
{
	if ((ta & BIT(3)) != 0U) {
		return -EBADMSG;
	}

	tag->modes |=
		((ta & BIT(0)) ? NFC_MODE_RX_212 : 0U) | ((ta & BIT(1)) ? NFC_MODE_RX_424 : 0U) |
		((ta & BIT(2)) ? NFC_MODE_RX_848 : 0U) | ((ta & BIT(4)) ? NFC_MODE_TX_212 : 0U) |
		((ta & BIT(5)) ? NFC_MODE_TX_424 : 0U) | ((ta & BIT(6)) ? NFC_MODE_TX_848 : 0U) |
		((ta & BIT(7)) ? NFC_MODE_TX_RX_SAME_RATE : 0U);

	return 0;
}

static void iso_dep_ats_tb(struct nfc_iso_dep_tag *tag, uint8_t tb)
{
	tag->sfgi = tb & 0x0FU;
	tag->fwi = (tb & 0xF0U) >> 4;
}

static int iso_dep_ats_tc(struct nfc_iso_dep_tag *tag, uint8_t tc, uint8_t cid)
{
	if ((tc & 0xFCU) != 0U) {
		return -EBADMSG;
	}

	tag->nad_supported = (tc & BIT(0)) != 0U;
	tag->cid_supported = (tc & BIT(1)) != 0U;
	if (tag->cid_supported) {
		tag->cid = cid;
	}

	return 0;
}

static int iso_dep_ats_next(const uint8_t *ats, uint16_t len, uint8_t *idx, uint8_t *out)
{
	if (*idx >= len) {
		return -EBADMSG;
	}

	*out = ats[(*idx)++];

	return 0;
}

static int iso_dep_parse_ats(struct nfc_iso_dep_tag *tag, const uint8_t *ats, uint16_t len,
			     uint8_t cid)
{
	uint8_t idx = 2U;
	uint8_t field;
	int ret;

	if (len == 0U || ats[0] != len) {
		return -EBADMSG;
	}

	iso_dep_ats_defaults(tag);

	if (len == 1U) {
		return 0;
	}

	if ((ats[1] & BIT(7)) != 0U) {
		return -EBADMSG;
	}

	tag->fsci = ats[1] & 0x0FU;

	if (ats[1] & NFC_ISO14443A_ATS_TA_PRESENT) {
		ret = iso_dep_ats_next(ats, len, &idx, &field);
		if (ret == 0) {
			ret = iso_dep_ats_ta(tag, field);
		}
		if (ret < 0) {
			return ret;
		}
	}

	if (ats[1] & NFC_ISO14443A_ATS_TB_PRESENT) {
		ret = iso_dep_ats_next(ats, len, &idx, &field);
		if (ret < 0) {
			return ret;
		}
		iso_dep_ats_tb(tag, field);
	}

	if (ats[1] & NFC_ISO14443A_ATS_TC_PRESENT) {
		ret = iso_dep_ats_next(ats, len, &idx, &field);
		if (ret == 0) {
			ret = iso_dep_ats_tc(tag, field, cid);
		}
		if (ret < 0) {
			return ret;
		}
	}

	if (idx < len) {
		tag->historical_len = MIN(len - idx, (uint16_t)sizeof(tag->historical));
		memcpy(tag->historical, &ats[idx], tag->historical_len);
	}

	LOG_DBG("ATS: FSCI %u (%u bytes), FWI %u (%u us), SFGI %u, CID %u, NAD %u", tag->fsci,
		fsi_to_fs(tag->fsci), tag->fwi, NFC_ISO14443_FGT_UNIT_US * (1U << tag->fwi),
		tag->sfgi, tag->cid_supported, tag->nad_supported);

	return 0;
}

static int iso_dep_rats(struct nfc_iso_dep_tag *tag, uint8_t cid)
{
	uint8_t tx_data[4];
	uint16_t tx_len;
	uint8_t ats[NFC_ISO14443A_MAX_ATS_LEN + 2U];
	uint16_t rx_len = sizeof(ats);
	struct nfc_property hw_tx_crc = {.type = NFC_PROP_HW_TX_CRC, .hw_tx_crc = true};
	struct nfc_property hw_rx_crc = {.type = NFC_PROP_HW_RX_CRC, .hw_rx_crc = true};
	struct nfc_property timeout = {.type = NFC_PROP_TIMEOUT, .timeout_us = 5286U + 60U};
	int ret;

	if (cid >= 0x0FU) {
		return -EINVAL;
	}

	tx_data[0] = NFC_ISO14443A_CMD_RATS;
	tx_data[1] = CONFIG_NFC_ISO14443_FSDI | cid;
	tx_len = 2U;

	(void)nfc_set_properties(tag->poller->dev, &timeout, 1U);
	(void)nfc_set_properties(tag->poller->dev, &hw_tx_crc, 1U);
	(void)nfc_set_properties(tag->poller->dev, &hw_rx_crc, 1U);
	if (hw_tx_crc.status == -ENOTSUP) {
		nfc_iso14443a_crc_append(tx_data, tx_len);
		tx_len += 2U;
	}

	ret = nfc_initiator_transceive(tag->poller->dev, tx_data, tx_len, 8U, ats, &rx_len);
	if (ret < 0) {
		return ret;
	}

	if (hw_rx_crc.status == -ENOTSUP) {
		if (rx_len < 2U || nfc_iso14443a_crc(ats, rx_len) != 0U) {
			return -EBADMSG;
		}
		rx_len -= 2U;
	}

	return iso_dep_parse_ats(tag, ats, rx_len, cid);
}

static int iso_dep_connect_offload(struct nfc_iso_dep_tag *tag, uint8_t cid)
{
	if (tag->target.a.ats_len == 0U) {
		return -ENOTSUP;
	}

	return iso_dep_parse_ats(tag, tag->target.a.ats, tag->target.a.ats_len, cid);
}

static int iso_dep_connect_rats(struct nfc_iso_dep_tag *tag, uint8_t cid)
{
	int ret;

	z_nfc_poller_lock(tag->poller);
	ret = iso_dep_rats(tag, cid);
	z_nfc_poller_unlock(tag->poller);

	if (ret < 0) {
		LOG_DBG("RATS failed (%d)", ret);
		return ret;
	}

	/* ISO/IEC 14443-4: no command until SFGT has elapsed after the ATS. */
	k_sleep(K_USEC(NFC_ISO14443_FGT_UNIT_US * (1U << tag->sfgi)));

	return 0;
}

int nfc_iso_dep_connect(struct nfc_poller *poller, const struct nfc_target *target,
			const struct nfc_iso_dep_connect_param *param,
			struct nfc_iso_dep_tag *iso_dep)
{
	uint8_t cid;

	if (target == NULL || iso_dep == NULL) {
		return -EINVAL;
	}
	if (!z_nfc_poller_ready(poller)) {
		return -EPERM;
	}
	if (target->tech != NFC_TECH_A) {
		return -ENOTSUP;
	}

	memset(iso_dep, 0, sizeof(*iso_dep));
	iso_dep->poller = poller;
	iso_dep->target = *target;
	cid = (param != NULL) ? param->cid : CONFIG_NFC_ISO_DEP_DEFAULT_CID;

	if (param != NULL && sys_timepoint_expired(sys_timepoint_calc(param->timeout))) {
		return -EAGAIN;
	}

	if (z_nfc_poller_backend(poller) == Z_NFC_BACKEND_OFFLOAD) {
		return iso_dep_connect_offload(iso_dep, cid);
	}

	return iso_dep_connect_rats(iso_dep, cid);
}

int z_nfc_iso_dep_deselect(struct nfc_iso_dep_tag *tag, k_timeout_t timeout)
{
	uint8_t tx[4] = {NFC_ISO14443_PCB_SBLOCK | NFC_ISO14443_PCB_SBLOCK_FXD};
	uint16_t tx_len = 1U;
	uint8_t rx[4];
	uint16_t rx_len = sizeof(rx);
	uint32_t timeout_us = z_nfc_timeout_us_cap(
		sys_timepoint_calc(timeout), 60U + NFC_ISO14443_FGT_UNIT_US * (1U << tag->fwi));
	int ret;

	if (tag->cid_supported && tag->cid != 0U) {
		tx[0] |= NFC_ISO14443_PCB_BLOCK_CID;
		tx[tx_len++] = tag->cid;
	}

	const struct z_nfca_xfer xfer = {.tx_crc = true, .rx_crc = true, .timeout_us = timeout_us};

	ret = z_nfca_transceive(tag->poller, &tag->target, tx, tx_len, rx, &rx_len, &xfer);
	if (ret < 0) {
		return ret;
	}

	if (rx_len == 0U) {
		return -EBADMSG;
	}

	if ((rx[0] & (NFC_ISO14443_PCB_BLOCK_MASK | NFC_ISO14443_PCB_SBLOCK_WTX)) !=
	    NFC_ISO14443_PCB_SBLOCK) {
		return -EBADMSG;
	}

	return 0;
}

static int iso_dep_transceive_offload(struct nfc_iso_dep_tag *tag, const uint8_t *tx_data,
				      uint16_t tx_len, uint8_t *rx_data, uint16_t *rx_len,
				      uint8_t nad, k_timeout_t timeout)
{
	uint32_t fwt_us = NFC_ISO14443_FGT_UNIT_US * (1U << tag->fwi);
	uint32_t left_us = z_nfc_timeout_us_cap(sys_timepoint_calc(timeout), fwt_us);
	int ret;

	if (nad != 0U) {
		return -ENOTSUP;
	}
	if (left_us == 0U) {
		return -EAGAIN;
	}

	z_nfc_poller_lock(tag->poller);
	ret = nfc_offload_exchange(tag->poller->dev, &tag->target, tx_data, tx_len, rx_data, rx_len,
				   DIV_ROUND_UP(left_us, USEC_PER_MSEC));
	z_nfc_poller_unlock(tag->poller);

	return ret;
}

struct iso_dep_xfer {
	struct nfc_iso_dep_tag *tag;
	uint8_t *tx;
	uint8_t *rx;
	uint16_t rx_len;
	bool sw_tx_crc;
	bool sw_rx_crc;
	k_timepoint_t deadline;
};

static void iso_dep_set_fwt(struct iso_dep_xfer *x, uint8_t wtx)
{
	struct nfc_property to = {
		.type = NFC_PROP_TIMEOUT,
		.timeout_us = 60U + NFC_ISO14443_FGT_UNIT_US * (1U << x->tag->fwi) * MAX(wtx, 1U),
	};

	(void)nfc_set_properties(x->tag->poller->dev, &to, 1U);
}

static int iso_dep_send(struct iso_dep_xfer *x, uint8_t *frame, uint16_t len)
{
	if (x->sw_tx_crc) {
		nfc_iso14443a_crc_append(frame, len);
		len += 2U;
	}

	x->rx_len = CONFIG_NFC_ISO14443_FSD_MAX;

	return nfc_initiator_transceive(x->tag->poller->dev, frame, len, 8U, x->rx, &x->rx_len);
}

static int iso_dep_strip_crc(struct iso_dep_xfer *x)
{
	if (!x->sw_rx_crc) {
		return 0;
	}

	if (x->rx_len < 2U || nfc_iso14443a_crc(x->rx, x->rx_len) != 0U) {
		return -EBADMSG;
	}
	x->rx_len -= 2U;

	return 0;
}

static uint16_t iso_dep_iblock_header(struct iso_dep_xfer *x, uint8_t nad)
{
	struct nfc_iso_dep_tag *tag = x->tag;
	uint16_t len = 1U;

	x->tx[0] = NFC_ISO14443_PCB_IBLOCK | NFC_ISO14443_PCB_IBLOCK_FXD |
		   NFC_ISO14443_PCB_IBLOCK_CHAINING;
	if (tag->cid_supported && tag->cid != 0U) {
		x->tx[0] |= NFC_ISO14443_PCB_BLOCK_CID;
		x->tx[len++] = tag->cid;
	}
	if (tag->nad_supported && nad != 0U) {
		x->tx[0] |= NFC_ISO14443_PCB_BLOCK_NAD;
		x->tx[len++] = nad;
	}

	return len;
}

struct iso_dep_tx {
	const uint8_t *data;
	uint16_t len;
	uint16_t hdr_len;
	uint16_t chunk;
	uint16_t num_blocks;
	uint16_t block;
	uint16_t retry_cnt;
};

static uint16_t iso_dep_build_iblock(struct iso_dep_xfer *x, const struct iso_dep_tx *tx)
{
	uint16_t off = tx->block * tx->chunk;
	uint16_t len = tx->hdr_len + MIN(tx->chunk, tx->len - off);

	if ((x->tag->block_num & NFC_ISO14443_PCB_BLOCK_NUM) == 0U) {
		x->tx[0] &= ~NFC_ISO14443_PCB_BLOCK_NUM;
	} else {
		x->tx[0] |= NFC_ISO14443_PCB_BLOCK_NUM;
	}
	if (tx->block + 1U == tx->num_blocks) {
		x->tx[0] &= ~NFC_ISO14443_PCB_IBLOCK_CHAINING;
	}

	memcpy(&x->tx[tx->hdr_len], &tx->data[off], len - tx->hdr_len);

	return len;
}

static uint16_t iso_dep_build_wtx(uint8_t *frame, uint8_t wtx)
{
	frame[0] =
		NFC_ISO14443_PCB_SBLOCK | NFC_ISO14443_PCB_SBLOCK_FXD | NFC_ISO14443_PCB_SBLOCK_WTX;
	frame[1] = wtx & 0x3FU;

	return 2U;
}

static int iso_dep_tx_done(struct iso_dep_xfer *x, const struct iso_dep_tx *tx)
{
	if (tx->block + 1U < tx->num_blocks) {
		return -EINVAL;
	}

	x->tag->block_num ^= NFC_ISO14443_PCB_BLOCK_NUM;

	return 0;
}

static uint16_t iso_dep_rblock_ack(struct iso_dep_xfer *x)
{
	struct nfc_iso_dep_tag *tag = x->tag;
	uint16_t len = 1U;

	x->tx[0] = NFC_ISO14443_PCB_RBLOCK | NFC_ISO14443_PCB_RBLOCK_FXD;
	if ((x->rx[0] & NFC_ISO14443_PCB_BLOCK_NUM) != 0U) {
		x->tx[0] |= NFC_ISO14443_PCB_BLOCK_NUM;
	}
	if (tag->cid_supported && tag->cid != 0U) {
		x->tx[0] |= NFC_ISO14443_PCB_BLOCK_CID;
		x->tx[len++] = tag->cid;
	}

	return len;
}

static int iso_dep_take_wtx(struct iso_dep_xfer *x, uint8_t *wtx)
{
	if ((x->rx[0] & NFC_ISO14443_PCB_SBLOCK_WTX) == 0U) {
		return -ECONNRESET;
	}
	if (x->rx_len != 2U) {
		return -EBADMSG;
	}

	*wtx = x->rx[1];
	LOG_DBG("target asked for WTX %u", *wtx);

	return 0;
}

static int iso_dep_ack_progress(struct iso_dep_xfer *x, struct iso_dep_tx *tx)
{
	struct nfc_iso_dep_tag *tag = x->tag;

	if ((x->rx[0] & NFC_ISO14443_PCB_BLOCK_MASK) != NFC_ISO14443_PCB_RBLOCK) {
		return -EBADMSG;
	}
	if ((x->rx[0] & NFC_ISO14443_PCB_BLOCK_CID) != 0U &&
	    (x->rx_len < 2U || x->rx[1] != tag->cid)) {
		return -EBADMSG;
	}

	if ((x->rx[0] & NFC_ISO14443_PCB_RBLOCK_NAK) == 0U &&
	    ((x->tx[0] ^ x->rx[0]) & NFC_ISO14443_PCB_BLOCK_NUM) == 0U) {
		tx->block++;
		tx->retry_cnt = 0U;
		tag->block_num ^= NFC_ISO14443_PCB_BLOCK_NUM;

		return 0;
	}

	if (tx->retry_cnt++ > NFC_ISO14443_EXCHANGE_MAX_RETRY) {
		LOG_DBG("no progress after %u retries", tx->retry_cnt);
		return -EAGAIN;
	}

	return 0;
}

static int iso_dep_send_chained(struct iso_dep_xfer *x, const uint8_t *data, uint16_t data_len,
				uint8_t nad)
{
	uint8_t wtx_frame[4U];
	struct iso_dep_tx tx = {
		.data = data,
		.len = data_len,
		.hdr_len = iso_dep_iblock_header(x, nad),
	};
	uint8_t wtx = 0U;
	int ret;

	tx.chunk = MIN(fsi_to_fs(x->tag->fsci), CONFIG_NFC_ISO14443_FSD_MAX) - tx.hdr_len - 2U;
	tx.num_blocks = DIV_ROUND_UP(data_len, tx.chunk);

	while (true) {
		uint8_t *frame = (wtx > 0U) ? wtx_frame : x->tx;
		uint16_t len;

		if (sys_timepoint_expired(x->deadline)) {
			return -EAGAIN;
		}

		len = (wtx > 0U) ? iso_dep_build_wtx(wtx_frame, wtx) : iso_dep_build_iblock(x, &tx);
		iso_dep_set_fwt(x, wtx);

		ret = iso_dep_send(x, frame, len);
		if (ret < 0) {
			return ret;
		}
		if (x->rx_len == 0U) {
			return -EBADMSG;
		}

		if ((x->rx[0] & NFC_ISO14443_PCB_BLOCK_MASK) == NFC_ISO14443_PCB_IBLOCK) {
			return iso_dep_tx_done(x, &tx);
		}

		ret = iso_dep_strip_crc(x);
		if (ret < 0) {
			return ret;
		}

		if ((x->rx[0] & NFC_ISO14443_PCB_BLOCK_MASK) == NFC_ISO14443_PCB_SBLOCK) {
			ret = iso_dep_take_wtx(x, &wtx);
			if (ret < 0) {
				return ret;
			}
			continue;
		}

		wtx = 0U;

		ret = iso_dep_ack_progress(x, &tx);
		if (ret < 0) {
			return ret;
		}
	}
}

static uint16_t iso_dep_rx_header_len(const struct iso_dep_xfer *x)
{
	uint16_t len = 1U;

	if (x->rx[0] & NFC_ISO14443_PCB_BLOCK_CID) {
		len++;
	}
	if (x->rx[0] & NFC_ISO14443_PCB_BLOCK_NAD) {
		len++;
	}

	return len;
}

static int iso_dep_recv_chained(struct iso_dep_xfer *x, uint8_t *data, uint16_t *data_len)
{
	uint16_t got = 0U;
	int ret;

	while (true) {
		uint16_t hdr_len;
		uint16_t chunk;
		uint16_t ack_len;

		if (sys_timepoint_expired(x->deadline)) {
			return -EAGAIN;
		}

		ret = iso_dep_strip_crc(x);
		if (ret < 0) {
			return ret;
		}
		if (x->rx_len == 0U) {
			return -EIO;
		}

		hdr_len = iso_dep_rx_header_len(x);
		if (x->rx_len < hdr_len) {
			return -EBADMSG;
		}

		chunk = x->rx_len - hdr_len;
		if (got + chunk > *data_len) {
			return -ENOSPC;
		}
		memcpy(&data[got], &x->rx[hdr_len], chunk);
		got += chunk;

		if ((x->rx[0] & NFC_ISO14443_PCB_IBLOCK_CHAINING) == 0U) {
			break;
		}

		ack_len = iso_dep_rblock_ack(x);
		ret = iso_dep_send(x, x->tx, ack_len);
		if (ret < 0) {
			return ret;
		}
	}

	*data_len = got;

	return 0;
}

int nfc_iso_dep_transceive(struct nfc_iso_dep_tag *tag, const uint8_t *tx_data,
			   uint16_t tx_data_len, uint8_t *rx_data, uint16_t *rx_data_len,
			   uint8_t nad, k_timeout_t timeout)
{
	uint8_t tx_frame[CONFIG_NFC_ISO14443_FSD_MAX];
	uint8_t rx_frame[CONFIG_NFC_ISO14443_FSD_MAX];
	struct nfc_property hw_tx_crc = {.type = NFC_PROP_HW_TX_CRC, .hw_tx_crc = true};
	struct nfc_property hw_rx_crc = {.type = NFC_PROP_HW_RX_CRC, .hw_rx_crc = true};
	struct iso_dep_xfer xfer = {
		.tag = tag,
		.tx = tx_frame,
		.rx = rx_frame,
		.deadline = sys_timepoint_calc(timeout),
	};
	int ret;

	if (!z_nfc_poller_ready(tag->poller)) {
		return -EPERM;
	}

	if (z_nfc_poller_backend(tag->poller) == Z_NFC_BACKEND_OFFLOAD) {
		return iso_dep_transceive_offload(tag, tx_data, tx_data_len, rx_data, rx_data_len,
						  nad, timeout);
	}

	z_nfc_poller_lock(tag->poller);

	(void)nfc_set_properties(tag->poller->dev, &hw_tx_crc, 1U);
	(void)nfc_set_properties(tag->poller->dev, &hw_rx_crc, 1U);
	xfer.sw_tx_crc = (hw_tx_crc.status == -ENOTSUP);
	xfer.sw_rx_crc = (hw_rx_crc.status == -ENOTSUP);

	ret = iso_dep_send_chained(&xfer, tx_data, tx_data_len, nad);
	if (ret == 0) {
		ret = iso_dep_recv_chained(&xfer, rx_data, rx_data_len);
	}

	if (ret < 0) {
		LOG_DBG("exchange failed (%d)", ret);
	}
	z_nfc_poller_unlock(tag->poller);

	return ret;
}
