/*
 * RFID ISO 14443 Type A functions.
 *
 * Copyright (c) 2023 Basalte bv
 * Copyrigth (c) 2025 Conny Marco Menebröcker
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(rfid_iso14443a, CONFIG_RFID_LOG_LEVEL);

#include <zephyr/device.h>
#include <zephyr/drivers/rfid.h>
#include <zephyr/rfid/iso14443.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#define RFID_ISO14443_EXCHANGE_MAX_RETRY 3

static const uint16_t fs_table[] = {
	16U, 24U, 32U, 40U, 48U, 64U, 96U, 128U, 256U,
};

/* Convert FSDI/FSCI to FSD/FSC */
static inline uint16_t fsi_to_fs(uint8_t fsi)
{
	return fs_table[MIN(fsi, 8U)];
}

static inline uint8_t rfid_iso14443a_bcc(const uint8_t *data, size_t data_len)
{
	uint8_t bcc = 0;

	for (size_t i = 0; i < data_len; ++i) {
		bcc ^= data[i];
	}

	return bcc;
}

static inline uint16_t rfid_iso14443a_crc(const uint8_t *data, size_t data_len)
{
	return crc16_reflect(RFID_ISO14443A_CRC16_POLY, RFID_ISO14443A_CRC16_SEED, data, data_len);
}

static inline void rfid_iso14443a_crc_append(uint8_t *data, size_t data_len)
{
	sys_put_le16(rfid_iso14443a_crc(data, data_len), &data[data_len]);
}

int rfid_iso14443a_request(const struct device *dev, uint8_t *atqa, bool sens)
{
	int ret;
	struct rfid_property props[] = {
		{
			.type = RFID_PROP_MFC_CRYPTO,
			.mfc_crypto_on = false,
		},
		{
			.type = RFID_PROP_HW_TX_CRC,
			.hw_tx_crc = false,
		},
		{
			.type = RFID_PROP_HW_RX_CRC,
			.hw_rx_crc = false,
		},
		{
			.type = RFID_PROP_TIMEOUT,
			.timeout_us = 85U + 60U,
		},
	};
	uint8_t cmd = sens ? RFID_ISO14443A_CMD_SENS_REQ : RFID_ISO14443A_CMD_ALL_REQ;
	uint16_t rx_len = RFID_ISO14443A_MAX_ATQA_LEN;

	ret = rfid_claim(dev);
	if (ret < 0) {
		return ret;
	}

	ret = rfid_set_properties(dev, props, ARRAY_SIZE(props));
	if (ret < 0) {
		goto unlock;
	}

	ret = rfid_initiator_transceive(dev, &cmd, 1, 7U, atqa, &rx_len);
	if (ret < 0) {
		goto unlock;
	}

	if (rx_len != RFID_ISO14443A_MAX_ATQA_LEN) {
		ret = -EPROTO;
		goto unlock;
	}
	/* TODO: verify valid ATQA */

unlock:

	(void)rfid_release(dev);

	return ret;
}

static int rfid_iso14443a_cascade_cmd(const struct device *dev, struct rfid_iso14443a_info *info,
				      uint8_t cascade_level)
{
	int ret;
	uint8_t tx_data[10] = {0};
	uint16_t tx_len;
	uint8_t rx_data[5];
	uint16_t rx_len = sizeof(rx_data);

	struct rfid_property hw_tx_crc_prop = {
		.type = RFID_PROP_HW_TX_CRC,
		.hw_tx_crc = false,
	};
	struct rfid_property hw_rx_crc_prop = {
		.type = RFID_PROP_HW_RX_CRC,
		.hw_rx_crc = false,
	};
	struct rfid_property timeout = {
		.type = RFID_PROP_TIMEOUT,
		.timeout_us = 85U + 60U,
	};

	switch (cascade_level) {
	case 0:
		tx_data[0] = RFID_ISO14443A_CMD_SDD_SEL_CL1;
		break;
	case 1:
		tx_data[0] = RFID_ISO14443A_CMD_SDD_SEL_CL2;
		break;
	case 2:
		tx_data[0] = RFID_ISO14443A_CMD_SDD_SEL_CL3;
		break;
	default:
		return -EINVAL;
	}

	tx_data[1] = 0x20U;
	tx_len = 2U;

	(void)rfid_set_properties(dev, &timeout, 1U);

	/* Set HW CRC properties, status field will be verified for fallback SW CRC */
	(void)rfid_set_properties(dev, &hw_tx_crc_prop, 1U);
	(void)rfid_set_properties(dev, &hw_rx_crc_prop, 1U);

	ret = rfid_initiator_transceive(dev, tx_data, tx_len, 8U, rx_data, &rx_len);
	if (ret < 0) {
		return ret;
	}

	if (rx_len != sizeof(rx_data) || rfid_iso14443a_bcc(rx_data, 5U) != 0U) {
		return -EBADMSG;
	}

	if (rx_data[0] == RFID_ISO14443A_CASCADE_TAG) {
		/* Only 3 UID bytes + BCC */
		memcpy(&info->uid[info->uid_len], &rx_data[1], 3U);
		info->uid_len += 3U;
	} else {
		/* Final 4 UID bytes */
		memcpy(&info->uid[info->uid_len], &rx_data[0], 4U);
		info->uid_len += 4U;
	}

	tx_data[1] = 0x70U;
	tx_len = 7U;
	memcpy(&tx_data[2], rx_data, 5U);

	/* Set HW CRC properties, status field will be verified for fallback SW CRC */
	hw_tx_crc_prop.hw_tx_crc = true;
	hw_rx_crc_prop.hw_rx_crc = true;
	(void)rfid_set_properties(dev, &hw_tx_crc_prop, 1U);
	(void)rfid_set_properties(dev, &hw_rx_crc_prop, 1U);
	if (hw_tx_crc_prop.status == -ENOTSUP) {
		/* Fallback SW CRC */
		rfid_iso14443a_crc_append(tx_data, tx_len);
		tx_len += 2U;
	}

	rx_len = sizeof(rx_data);
	ret = rfid_initiator_transceive(dev, tx_data, tx_len, 8U, rx_data, &rx_len);
	if (ret < 0) {
		return ret;
	}

	if (hw_rx_crc_prop.status == -ENOTSUP) {
		/* Fallback SW CRC */
		if (rx_len != 3U || rfid_iso14443a_crc(rx_data, 3U) != 0U) {
			return -EBADMSG;
		}
	} else if (rx_len != 1U) {
		return -EBADMSG;
	}

	info->sak = rx_data[0];

	return 0;
}

int rfid_iso14443a_sdd(const struct device *dev, struct rfid_iso14443a_info *info)
{
	int ret;

	ret = rfid_claim(dev);
	if (ret < 0) {
		return ret;
	}

	for (uint8_t lvl = 0U; lvl < 3U; ++lvl) {
		ret = rfid_iso14443a_cascade_cmd(dev, info, lvl);
		if (ret < 0 || (info->sak & RFID_ISO14443A_SAK_CASCADE) == 0U) {
			break;
		}
	}

	(void)rfid_release(dev);

	return ret;
}

int rfid_iso14443a_rats(const struct device *dev, struct rfid_iso14443a_info *info, uint8_t cid)
{
	int ret;
	uint8_t tx_data[4];
	uint16_t tx_len;
	uint8_t ats[RFID_ISO14443A_MAX_ATS_LEN + 2U];
	uint16_t rx_len;
	uint8_t ats_index;

	struct rfid_property hw_tx_crc_prop = {
		.type = RFID_PROP_HW_TX_CRC,
		.hw_tx_crc = true,
	};
	struct rfid_property hw_rx_crc_prop = {
		.type = RFID_PROP_HW_RX_CRC,
		.hw_rx_crc = true,
	};
	struct rfid_property timeout = {
		.type = RFID_PROP_TIMEOUT,
		.timeout_us = 5286U + 60U,
	};

	if (cid >= 0x0FU) {
		return -EINVAL;
	}

	ret = rfid_claim(dev);
	if (ret < 0) {
		return ret;
	}

	tx_data[0] = RFID_ISO14443A_CMD_RATS;
	tx_data[1] = CONFIG_RFID_ISO14443_FSDI | cid;
	tx_len = 2U;

	(void)rfid_set_properties(dev, &timeout, 1U);
	(void)rfid_set_properties(dev, &hw_tx_crc_prop, 1U);
	(void)rfid_set_properties(dev, &hw_rx_crc_prop, 1U);
	if (hw_tx_crc_prop.status == -ENOTSUP) {
		/* Fallback SW CRC */
		rfid_iso14443a_crc_append(tx_data, tx_len);
		tx_len += 2U;
	}

	rx_len = sizeof(ats);
	ret = rfid_initiator_transceive(dev, tx_data, tx_len, 8U, ats, &rx_len);
	if (ret < 0) {
		goto unlock;
	}

	if (hw_rx_crc_prop.status == -ENOTSUP) {
		/* Fallback SW CRC */
		if (rx_len < 2U || rfid_iso14443a_crc(ats, rx_len) != 0U) {
			ret = -EBADMSG;
			goto unlock;
		}

		rx_len -= 2U;
	}

	if (rx_len == 0 || ats[0] != rx_len) {
		ret = -EBADMSG;
		goto unlock;
	}

	/* Fill in defaults */
	info->cid_supported = false;
	info->cid = 0U;
	info->nad_supported = false;
	info->fsci = 2U;
	info->fwi = 4U;
	info->sfgi = 0U;
	info->modes = RFID_MODE_TX_106 | RFID_MODE_RX_106;
	info->history_len = 0U;
	info->block_num = 0U;

	/* Empty ATS */
	if (rx_len == 1U) {
		goto unlock;
	}

	/* Default to a bad message result */
	ret = -EBADMSG;

	/* T0 b8 should be zero */
	if ((ats[1] & BIT(7)) != 0U) {
		goto unlock;
	}

	info->fsci = (ats[1] & 0xF0U) >> 4;

	ats_index = 2U;

	if (ats[1] & RFID_ISO14443A_ATS_TA_PRESENT) {
		uint8_t ta = ats[ats_index];

		if (ats_index >= rx_len) {
			goto unlock;
		}

		/* TA(1) b4 should be zero */
		if ((ta & BIT(3)) != 0U) {
			goto unlock;
		}
		info->modes |= (ta & BIT(0))        ? RFID_MODE_TX_212
			       : 0U | (ta & BIT(1)) ? RFID_MODE_TX_424
			       : 0U | (ta & BIT(2)) ? RFID_MODE_TX_848
			       : 0U | (ta & BIT(4)) ? RFID_MODE_RX_212
			       : 0U | (ta & BIT(5)) ? RFID_MODE_RX_424
			       : 0U | (ta & BIT(6)) ? RFID_MODE_RX_848
			       : 0U | (ta & BIT(7)) ? RFID_MODE_TX_RX_SAME_RATE
						    : 0U;

		ats_index++;
	}

	if (ats[1] & RFID_ISO14443A_ATS_TB_PRESENT) {
		uint8_t tb = ats[ats_index];

		if (ats_index >= rx_len) {
			goto unlock;
		}

		info->sfgi = tb & 0x0FU;
		info->fwi = (tb & 0xF0U) >> 4;

		ats_index++;
	}

	if (ats[1] & RFID_ISO14443A_ATS_TC_PRESENT) {
		uint8_t tc = ats[ats_index];

		if (ats_index >= rx_len) {
			goto unlock;
		}

		/* TC(1) b3-b8 should be zero */
		if ((tc & 0xFC) != 0U) {
			goto unlock;
		}

		/* Nad support is indicated by TC(1) b1 */
		info->nad_supported = (tc & BIT(0)) != 0U;

		/* CID support is indicated by TC(1) b2 */
		info->cid_supported = (tc & BIT(1)) != 0U;
		if (info->cid_supported) {
			info->cid = cid;
		}

		ats_index++;
	}

	/* Remainder for history bytes */
	if (ats_index < rx_len) {
		info->history_len = rx_len - ats_index;
		memcpy(info->history, &ats[ats_index], info->history_len);
	}

	/* All OK */
	ret = 0;

unlock:

	(void)rfid_release(dev);

	return ret;
}

int rfid_iso14443a_pps(const struct device *dev, struct rfid_iso14443a_info *info,
		       rfid_mode_t modes)
{
	int ret;
	uint8_t tx_data[5];
	uint16_t tx_len;
	uint8_t rx_data[3];
	uint16_t rx_len;

	struct rfid_property hw_tx_crc_prop = {
		.type = RFID_PROP_HW_TX_CRC,
		.hw_tx_crc = true,
	};
	struct rfid_property hw_rx_crc_prop = {
		.type = RFID_PROP_HW_RX_CRC,
		.hw_rx_crc = true,
	};
	struct rfid_property timeout = {
		.type = RFID_PROP_TIMEOUT,
		.timeout_us = 5286U + 60U,
	};

	ret = rfid_claim(dev);
	if (ret < 0) {
		return ret;
	}

	tx_data[0] = RFID_ISO14443A_CMD_PPSS | info->cid;
	/* Set PPS0 b5 to indicate PPS1 is present */
	tx_data[1] = RFID_ISO14443A_PPS_PPS0 | BIT(4);
	tx_data[2] = RFID_ISO14443A_PPS_PPS1;
	tx_len = 3U;

	if ((info->modes & RFID_MODE_TX_RX_SAME_RATE) != 0U) {
		/* Verify same TX/RX data rates */
		switch (modes & (RFID_MODE_TX_MASK | RFID_MODE_RX_MASK)) {
		case RFID_MODE_TX_106 | RFID_MODE_RX_106:
		case RFID_MODE_TX_212 | RFID_MODE_RX_212:
		case RFID_MODE_TX_424 | RFID_MODE_RX_424:
		case RFID_MODE_TX_848 | RFID_MODE_RX_848:
			break;
		default:
			ret = -EINVAL;
			goto unlock;
		}
	}

	/* Encode data rate from PCD to PICC */
	switch (modes & info->modes & RFID_MODE_TX_MASK) {
	case RFID_MODE_TX_106:
		/* NOP */
		break;
	case RFID_MODE_TX_212:
		tx_data[2] |= 0x01U;
		break;
	case RFID_MODE_TX_424:
		tx_data[2] |= 0x02U;
		break;
	case RFID_MODE_TX_848:
		tx_data[2] |= 0x03U;
		break;
	default:
		ret = -EINVAL;
		goto unlock;
	}

	/* Encode data rate from PICC to PDC */
	switch (modes & info->modes & RFID_MODE_RX_MASK) {
	case RFID_MODE_RX_106:
		/* NOP */
		break;
	case RFID_MODE_RX_212:
		tx_data[2] |= 0x04U;
		break;
	case RFID_MODE_RX_424:
		tx_data[2] |= 0x08U;
		break;
	case RFID_MODE_RX_848:
		tx_data[2] |= 0x0CU;
		break;
	default:
		ret = -EINVAL;
		goto unlock;
	}

	(void)rfid_set_properties(dev, &timeout, 1U);
	(void)rfid_set_properties(dev, &hw_tx_crc_prop, 1U);
	(void)rfid_set_properties(dev, &hw_rx_crc_prop, 1U);
	if (hw_tx_crc_prop.status == -ENOTSUP) {
		/* Fallback SW CRC */
		rfid_iso14443a_crc_append(tx_data, tx_len);
		tx_len += 2U;
	}

	rx_len = sizeof(rx_data);
	ret = rfid_initiator_transceive(dev, tx_data, tx_len, 8U, rx_data, &rx_len);
	if (ret < 0) {
		goto unlock;
	}

	if (hw_rx_crc_prop.status == -ENOTSUP) {
		/* Fallback SW CRC */
		if (rx_len < 2U || rfid_iso14443a_crc(rx_data, rx_len) != 0U) {
			ret = -EBADMSG;
			goto unlock;
		}

		rx_len -= 2U;
	}

	if (rx_len != 1U || rx_data[0] != RFID_ISO14443A_CMD_PPSS) {
		ret = -EBADMSG;
		goto unlock;
	}

unlock:
	(void)rfid_release(dev);

	return ret;
}

int rfid_iso14443a_halt(const struct device *dev)
{
	int ret;
	uint8_t tx_data[4];
	uint16_t tx_len;
	uint8_t rx_data[3];
	uint16_t rx_len = 0;
	struct rfid_property hw_tx_crc_prop = {
		.type = RFID_PROP_HW_TX_CRC,
		.hw_tx_crc = true,
	};
	struct rfid_property timeout = {
		.type = RFID_PROP_TIMEOUT,
		.timeout_us = 1100U + 60U,
	};

	tx_data[0] = RFID_ISO14443A_CMD_HALT;
	tx_data[1] = 0U;
	tx_len = 2U;

	ret = rfid_claim(dev);
	if (ret < 0) {
		return ret;
	}

	(void)rfid_set_properties(dev, &hw_tx_crc_prop, 1U);
	if (hw_tx_crc_prop.status == -ENOTSUP) {
		rfid_iso14443a_crc_append(tx_data, tx_len);
		tx_len += 2U;
	}

	(void)rfid_set_properties(dev, &timeout, 1U);
	ret = rfid_initiator_transceive(dev, tx_data, tx_len, 8U, rx_data, &rx_len);

	(void)rfid_release(dev);

	return ret;
}

int rfid_iso14443a_exchange(const struct device *dev, struct rfid_iso14443a_info *info,
			    const uint8_t *tx_data, uint16_t tx_data_len, uint8_t *rx_data,
			    uint16_t *rx_data_len, uint8_t nad)
{
	int ret;
	uint8_t tx_frame[CONFIG_RFID_ISO14443_FSD_MAX];
	uint8_t rx_frame[CONFIG_RFID_ISO14443_FSD_MAX];
	uint8_t wtx_frame[4U];
	uint16_t rx_len;
	/* The maximum frame size for both PCD and PICC */
	uint16_t tx_data_size = MIN(fsi_to_fs(info->fsci), CONFIG_RFID_ISO14443_FSD_MAX);
	uint16_t hdr_len = 1U;
	uint16_t block, num_blocks, block_size, retry_cnt;
	uint8_t wtx;
	struct rfid_property hw_tx_crc_prop = {
		.type = RFID_PROP_HW_TX_CRC,
		.hw_tx_crc = true,
	};
	struct rfid_property hw_rx_crc_prop = {
		.type = RFID_PROP_HW_RX_CRC,
		.hw_rx_crc = true,
	};
	struct rfid_property timeout = {
		.type = RFID_PROP_TIMEOUT,
	};

	ret = rfid_claim(dev);
	if (ret < 0) {
		return ret;
	}

	(void)rfid_set_properties(dev, &hw_tx_crc_prop, 1);
	(void)rfid_set_properties(dev, &hw_rx_crc_prop, 1);

	/* Prepare prologue */
	tx_frame[0] = RFID_ISO14443_PCB_IBLOCK | RFID_ISO14443_PCB_IBLOCK_FXD |
		      RFID_ISO14443_PCB_IBLOCK_CHAINING;
	if (info->cid_supported && info->cid != 0U) {
		tx_frame[0] |= RFID_ISO14443_PCB_BLOCK_CID;
		tx_frame[hdr_len++] = info->cid;
	}
	if (info->nad_supported && nad != 0U) {
		tx_frame[0] |= RFID_ISO14443_PCB_BLOCK_NAD;
		tx_frame[hdr_len++] = nad;
	}

	tx_data_size -= hdr_len + 2U;
	num_blocks = DIV_ROUND_UP(tx_data_len, tx_data_size);

	/* TX */
	block = 0U;
	retry_cnt = 0U;
	wtx = 0U;
	while (true) {
		if (wtx == 0U) {
			/* Set current block num bit */
			if ((info->block_num & RFID_ISO14443_PCB_BLOCK_NUM) == 0U) {
				tx_frame[0] &= ~RFID_ISO14443_PCB_BLOCK_NUM;
			} else {
				tx_frame[0] |= RFID_ISO14443_PCB_BLOCK_NUM;
			}

			/* Check if last block is being sent */
			if (block + 1 == num_blocks) {
				tx_frame[0] &= ~RFID_ISO14443_PCB_IBLOCK_CHAINING;
			}

			if (tx_data_len < block * tx_data_size) {
				LOG_ERR("Invalid block size, (%d) (%d)", tx_data_len,
					(block * tx_data_size));
				ret = -ENOMEM;
				goto unlock;
			}

			block_size =
				hdr_len + MIN(tx_data_size, tx_data_len - (block * tx_data_size));

			memcpy(&tx_frame[hdr_len], &tx_data[block * tx_data_size], block_size);

			timeout.timeout_us = 60U + 302U * (1U << info->fwi);
		} else {
			wtx_frame[0] = RFID_ISO14443_PCB_SBLOCK | RFID_ISO14443_PCB_SBLOCK_FXD |
				       RFID_ISO14443_PCB_SBLOCK_WTX;
			wtx_frame[1] = wtx & 0x3FU;
			block_size = 2U;

			timeout.timeout_us = 60U + 302U * (1U << info->fwi) * wtx;
		}

		if (hw_tx_crc_prop.status == -ENOTSUP) {
			rfid_iso14443a_crc_append(wtx > 0U ? wtx_frame : tx_frame, block_size);
			block_size += 2U;
		}

		(void)rfid_set_properties(dev, &timeout, 1U);

		rx_len = sizeof(rx_frame);
		ret = rfid_initiator_transceive(dev, wtx > 0U ? wtx_frame : tx_frame, block_size,
						8U, rx_frame, &rx_len);
		if (ret < 0) {
			goto unlock;
		}

		if (rx_len == 0U) {
			ret = -EBADMSG;
			goto unlock;
		}

		if ((rx_frame[0] & RFID_ISO14443_PCB_BLOCK_MASK) == RFID_ISO14443_PCB_IBLOCK) {
			/* TX done, start RX */
			if (block + 1 >= num_blocks) {
				/* Toggle our NUM bit */
				info->block_num ^= RFID_ISO14443_PCB_BLOCK_NUM;
				break;
			}

			ret = -EINVAL;
			goto unlock;
		}

		if (hw_rx_crc_prop.status == -ENOTSUP) {
			/* Fallback SW CRC */
			if (rx_len < 2U || rfid_iso14443a_crc(rx_frame, rx_len) != 0U) {
				ret = -EBADMSG;
				goto unlock;
			}

			rx_len -= 2U;
		}

		if ((rx_frame[0] & RFID_ISO14443_PCB_BLOCK_MASK) == RFID_ISO14443_PCB_SBLOCK) {
			if ((rx_frame[0] & RFID_ISO14443_PCB_SBLOCK_WTX) != 0U) {
				if (rx_len != 2U) {
					ret = -EBADMSG;
					goto unlock;
				}

				wtx = rx_frame[1];
				continue;
			} else {
				ret = -ECONNRESET;
				goto unlock;
			}
		}

		/* Reset WTX */
		wtx = 0U;

		/* Validate ACK */
		if ((rx_frame[0] & RFID_ISO14443_PCB_BLOCK_MASK) != RFID_ISO14443_PCB_RBLOCK) {
			ret = -EBADMSG;
			goto unlock;
		}

		if ((rx_frame[0] & RFID_ISO14443_PCB_BLOCK_CID) != 0U &&
		    (rx_len < 2U || rx_frame[1] != info->cid)) {
			/* FIXME: what to do here */
			ret = -EBADMSG;
			goto unlock;
		}

		if ((rx_frame[0] & RFID_ISO14443_PCB_RBLOCK_NAK) == 0U &&
		    ((tx_frame[0] ^ rx_frame[0]) & RFID_ISO14443_PCB_BLOCK_NUM) == 0U &&
		    (tx_frame[0] & RFID_ISO14443_PCB_IBLOCK_CHAINING) != 0U) {
			block++;
			retry_cnt = 0U;

			/* Toggle our NUM bit */
			info->block_num ^= RFID_ISO14443_PCB_BLOCK_NUM;
		} else if (retry_cnt++ > RFID_ISO14443_EXCHANGE_MAX_RETRY) {
			LOG_WRN("Exceeded %d retry attempts", retry_cnt);

			ret = -EAGAIN;
			goto unlock;
		}
	}

	block = 0U;
	/* RX */
	while (true) {
		if (hw_rx_crc_prop.status == -ENOTSUP) {
			/* Fallback SW CRC */
			if (rx_len < 2U || rfid_iso14443a_crc(rx_frame, rx_len) != 0U) {
				ret = -EBADMSG;
				goto unlock;
			}

			rx_len -= 2U;
		}

		/* Sanity check */
		if (rx_len == 0U) {
			ret = -EIO;
			goto unlock;
		}

		hdr_len = 1U;

		if (rx_frame[0] & RFID_ISO14443_PCB_BLOCK_CID) {
			/* TODO: Check CID */
			hdr_len++;
		}
		if (rx_frame[0] & RFID_ISO14443_PCB_BLOCK_NAD) {
			/* TODO: Check NAD */
			hdr_len++;
		}

		/* TODO: check num block for duplicate */

		if (rx_len < hdr_len) {
			ret = -EBADMSG;
			goto unlock;
		}

		/* Validate memory bounds */
		block_size = rx_len - hdr_len;
		if (block + block_size > *rx_data_len) {
			ret = -ENOSPC;
			goto unlock;
		}

		/* Copy data and add block offset */
		memcpy(&rx_data[block], &rx_frame[hdr_len], block_size);
		block += block_size;

		if ((rx_frame[0] & RFID_ISO14443_PCB_IBLOCK_CHAINING) == 0U) {
			break;
		}

		/* Send ACK */
		hdr_len = 1U;
		tx_frame[0] = RFID_ISO14443_PCB_RBLOCK | RFID_ISO14443_PCB_RBLOCK_FXD;
		if ((rx_frame[0] & RFID_ISO14443_PCB_BLOCK_NUM) != 0U) {
			tx_frame[0] |= RFID_ISO14443_PCB_BLOCK_NUM;
		}
		if (info->cid_supported && info->cid != 0U) {
			tx_frame[0] |= RFID_ISO14443_PCB_BLOCK_CID;
			tx_frame[hdr_len++] = info->cid;
		}
		if (hw_tx_crc_prop.status == -ENOTSUP) {
			rfid_iso14443a_crc_append(tx_frame, hdr_len);
			hdr_len += 2U;
		}

		rx_len = sizeof(rx_frame);
		ret = rfid_initiator_transceive(dev, tx_frame, hdr_len, 8U, rx_frame, &rx_len);
		if (ret < 0) {
			goto unlock;
		}
	}

	/* Override received data length */
	*rx_data_len = block;

unlock:
	if (ret < 0) {
		LOG_ERR("Failed to exchange data (%d)", ret);
	}

	(void)rfid_release(dev);

	return ret;
}

static int rfid_iso14443a_handle_rats(const struct device *dev, struct rfid_iso14443a_info *info,
				      uint8_t param)
{
	uint8_t ats[CONFIG_RFID_ISO14443_FSC_MAX + 2U];
	uint16_t tx_len;
	struct rfid_property hw_tx_crc_prop = {
		.type = RFID_PROP_HW_TX_CRC,
		.hw_tx_crc = true,
	};
	rfid_mode_t hw_caps;

	if ((5U + info->history_len) > CONFIG_RFID_ISO14443_FSC_MAX) {
		return -ENOSPC;
	}

	hw_caps = rfid_supported_modes(dev, RFID_PROTO_ISO14443A);

	/* Store PCC info */
	info->cid = info->cid_supported ? (param & 0x0FU) : 0U;
	info->fsdi = (param & 0xF0U) >> 4U;

	/* The PICC block number shall be initialized to 1 at activation. */
	info->block_num = 1U;

	/* TODO: check tx_len vs fsi_to_fs(info->fsdi) */

	/* TL */
	tx_len = ats[0] = 5U + info->history_len;

	/* T0 */
	ats[1] = RFID_ISO14443A_ATS_TA_PRESENT | RFID_ISO14443A_ATS_TB_PRESENT |
		 RFID_ISO14443A_ATS_TC_PRESENT | CONFIG_RFID_ISO14443_FSCI;

	/* TA(1) */
	ats[2] = ((hw_caps & RFID_MODE_RX_212) ? BIT(0) : 0U) |
		 ((hw_caps & RFID_MODE_RX_424) ? BIT(1) : 0U) |
		 ((hw_caps & RFID_MODE_RX_848) ? BIT(2) : 0U) |
		 ((hw_caps & RFID_MODE_TX_212) ? BIT(4) : 0U) |
		 ((hw_caps & RFID_MODE_TX_424) ? BIT(5) : 0U) |
		 ((hw_caps & RFID_MODE_TX_848) ? BIT(6) : 0U) |
		 ((hw_caps & RFID_MODE_TX_RX_SAME_RATE) ? BIT(7) : 0U);

	/* TB(1) */
	ats[3] = (info->sfgi & 0x0FU) | (info->fwi & 0x0FU) << 4U;

	/* TC(1) */
	ats[4] = (info->nad_supported ? BIT(0) : 0U) | (info->cid_supported ? BIT(1) : 0U);

	/* Historical bytes */
	if (info->history_len > 0U) {
		memcpy(&ats[5], info->history, info->history_len);
	}

	(void)rfid_set_properties(dev, &hw_tx_crc_prop, 1U);
	if (hw_tx_crc_prop.status == -ENOTSUP) {
		rfid_iso14443a_crc_append(ats, tx_len);
		tx_len += 2U;
	}

	return rfid_target_transmit(dev, ats, tx_len, 8U);
}

static int rfid_iso14443a_handle_cmd(const struct device *dev, struct rfid_iso14443a_info *info,
				     const uint8_t *rx_data, uint16_t rx_len)
{
	if (rx_len == 0U) {
		return -EINVAL;
	}

	switch (rx_data[0]) {
	case RFID_ISO14443A_CMD_ALL_REQ:
	case RFID_ISO14443A_CMD_SENS_REQ:
	case RFID_ISO14443A_CMD_SDD_SEL_CL1:
	case RFID_ISO14443A_CMD_SDD_SEL_CL2:
	case RFID_ISO14443A_CMD_SDD_SEL_CL3:
	case RFID_ISO14443A_CMD_PPSS:
		LOG_ERR("Command not implemented %02x", rx_data[0]);
		/* TODO: Add compliant layer 3/4 handling if frontend IC does not */
		/* Return positive value */
		return -ENOSYS;
	case RFID_ISO14443A_CMD_HALT:
		break;
	case RFID_ISO14443A_CMD_RATS:
		if (rx_len != 2U) {
			return -EINVAL;
		}

		return rfid_iso14443a_handle_rats(dev, info, rx_data[1]);
	default:
		break;
	}

	return -EINVAL;
}

int rfid_iso14443a_listen(const struct device *dev, struct rfid_iso14443a_info *info)
{
	uint8_t rx_data[CONFIG_RFID_ISO14443_FSC_MAX];
	uint16_t rx_len;
	struct rfid_property props[] = {
		{
			.type = RFID_PROP_SENS_RES,
		},
		{
			.type = RFID_PROP_RANDOM_UID,
			.random_uid = info->uid_len == 0U,
		},
		{
			.type = RFID_PROP_SEL_RES,
			.sel_res = info->sak,
		},
		{
			.type = RFID_PROP_TIMEOUT,
			.timeout_us = 5000U,
		},
	};
	int ret;

	memcpy(props[0].sens_res, info->atqa, sizeof(props[0].sens_res));

	ret = rfid_claim(dev);
	if (ret < 0) {
		return ret;
	}

	ret = rfid_set_properties(dev, props, ARRAY_SIZE(props));
	if (ret < 0) {
		LOG_ERR("Failed to set properties (%d)", ret);
		goto unlock;
	}

	/* Initiate blocking listen */
	ret = rfid_listen(dev, RFID_PROTO_ISO14443A, rx_data, &rx_len);
	if (ret < 0) {
		LOG_ERR("Failed to listen (%d)", ret);
		goto unlock;
	}

	while (true) {
		ret = rfid_iso14443a_handle_cmd(dev, info, rx_data, rx_len);

		if (ret > 0) {
			ret = rfid_target_receive(dev, rx_data, &rx_len);
			if (ret < 0) {
				LOG_ERR("Failed to receive data (%d)", ret);
				goto unlock;
			}
			continue;
		}

		if (ret < 0) {
			LOG_ERR("Failed to handle CMD (%d)", ret);
		}
		break;
	}

unlock:
	(void)rfid_release(dev);

	return ret;
}

int rfid_iso14443a_receive(const struct device *dev, struct rfid_iso14443a_info *info,
			   uint8_t *rx_data, uint16_t *rx_len, uint8_t *nad)
{
	uint8_t rx_block[CONFIG_RFID_ISO14443_FSC_MAX];
	uint8_t tx_block[4U];
	uint16_t rx_block_len, rx_block_index, tx_block_len;
	uint16_t rx_offset = 0;
	int ret;

	ret = rfid_claim(dev);
	if (ret < 0) {
		return ret;
	}

	while (true) {
		rx_block_index = 1U;
		rx_block_len = sizeof(rx_block);
		ret = rfid_target_receive(dev, rx_block, &rx_block_len);
		if (ret < 0) {
			LOG_ERR("Failed to receive data (%d)", ret);
			goto unlock;
		}
		/* TODO: fallback SW CRC */

		if (rx_block_len == 0) {
			ret = -EBADMSG;
			goto unlock;
		}

		/* Verify CID if provided */
		if ((rx_block[0] & RFID_ISO14443_PCB_BLOCK_CID) != 0U) {
			if (rx_block_len < 2U) {
				ret = -EBADMSG;
				goto unlock;
			}
			if (!info->cid_supported && rx_block[1] != 0U) {
				/* Ignore CID blocks */
				break;
			}
			if (rx_block[rx_block_index] != info->cid) {
				/* Ignore unknown CID */
				break;
			}
			rx_block_index++;
		}

		if ((rx_block[0] & RFID_ISO14443_PCB_BLOCK_MASK) == RFID_ISO14443_PCB_SBLOCK) {
			/* Verify fixed bit(s) */
			if ((rx_block[0] & RFID_ISO14443_PCB_SBLOCK_FXD) !=
			    RFID_ISO14443_PCB_SBLOCK_FXD) {
				ret = -EBADMSG;
				goto unlock;
			}

			if ((rx_block[0] & RFID_ISO14443_PCB_SBLOCK_MASK) ==
			    RFID_ISO14443_PCB_SBLOCK_DESELECT) {
				tx_block_len = 1U;
				tx_block[0] = RFID_ISO14443_PCB_SBLOCK |
					      RFID_ISO14443_PCB_SBLOCK_FXD |
					      RFID_ISO14443_PCB_SBLOCK_DESELECT;
				if ((rx_block[0] & RFID_ISO14443_PCB_BLOCK_CID) != 0U) {
					tx_block[0] |= RFID_ISO14443_PCB_BLOCK_CID;
					tx_block[1] = info->cid;
					tx_block_len++;
				}

				ret = rfid_target_transmit(dev, tx_block, tx_block_len, 8U);
				if (ret < 0) {
					LOG_ERR("Failed to transmit DESELECT (%d)", ret);
					goto unlock;
				}

				ret = -ECONNRESET;
				goto unlock;
			}

			continue;
		}

		if ((rx_block[0] & RFID_ISO14443_PCB_BLOCK_MASK) != RFID_ISO14443_PCB_IBLOCK) {
			/* If we get here, we couldn't parse the block */
			ret = -EBADMSG;
			goto unlock;
		}

		/* Toggle our block bit */
		info->block_num ^= RFID_ISO14443_PCB_BLOCK_NUM;

		/* Verify fixed bit(s) */
		if ((rx_block[0] & RFID_ISO14443_PCB_IBLOCK_FXD) != RFID_ISO14443_PCB_IBLOCK_FXD) {
			ret = -EBADMSG;
			goto unlock;
		}

		if ((rx_block[0] & RFID_ISO14443_PCB_BLOCK_NAD) != 0U) {
			if (!info->nad_supported) {
				ret = -EBADMSG;
				goto unlock;
			}

			if (nad != NULL) {
				*nad = rx_block[rx_block_index];
			} else {
				LOG_WRN("NAD ignored (0x%02x)", rx_block[rx_block_index]);
			}
			rx_block_index++;
		}

		if (rx_block_index > rx_block_len) {
			ret = -EBADMSG;
			goto unlock;
		}

		/* Copy the data */
		if (((rx_block_len - rx_block_index) + rx_offset) > *rx_len) {
			ret = -ENOSPC;
			goto unlock;
		}

		memcpy(&rx_data[rx_offset], &rx_block[rx_block_index],
		       rx_block_len - rx_block_index);

		rx_offset += rx_block_len - rx_block_index;

		if ((rx_block[0] & RFID_ISO14443_PCB_IBLOCK_CHAINING) != 0U) {
			tx_block_len = 1U;
			tx_block[0] = RFID_ISO14443_PCB_RBLOCK | RFID_ISO14443_PCB_RBLOCK_FXD |
				      (rx_block[0] & RFID_ISO14443_PCB_BLOCK_NUM);

			/* Add CID to R(ACK) */
			if ((rx_block[0] & RFID_ISO14443_PCB_BLOCK_CID) != 0U) {
				tx_block[0] |= RFID_ISO14443_PCB_BLOCK_CID;
				tx_block[1] = info->cid;
				tx_block_len++;
			}

			/* TODO: SW CRC fallback */
			ret = rfid_target_transmit(dev, tx_block, tx_block_len, 8U);
			if (ret < 0) {
				LOG_ERR("Failed to send R(ACK) (%d)", ret);
				goto unlock;
			}
			continue;
		}

		break;
	}

	/* Pass received length to caller */
	*rx_len = rx_offset;

unlock:
	if (ret < 0) {
		LOG_ERR("Failed to receive data (%d)", ret);
	}
	(void)rfid_release(dev);

	return ret;
}

int rfid_iso14443a_transmit(const struct device *dev, struct rfid_iso14443a_info *info,
			    const uint8_t *tx_data, uint16_t tx_len, uint8_t nad)
{
	uint8_t tx_block[CONFIG_RFID_ISO14443_FSC_MAX];
	uint8_t rx_block[4];
	uint16_t tx_block_len, tx_block_index, rx_block_len, tx_block_max_len;
	uint16_t tx_offset = 0;
	int ret;

	ret = rfid_claim(dev);
	if (ret < 0) {
		return ret;
	}

	tx_block_max_len = MIN(fsi_to_fs(info->fsdi), CONFIG_RFID_ISO14443_FSC_MAX);

	while (tx_offset < tx_len) {
		/* Wait for ACK from previous block */
		if (tx_offset > 0) {
			rx_block_len = sizeof(rx_block);
			ret = rfid_target_receive(dev, rx_block, &rx_block_len);
			if (ret < 0 || rx_block_len == 0U) {
				LOG_ERR("Failed to receive R(ACK) (%d)", ret);
				goto unlock;
			}
			/* TODO: SW CRC fallback */

			if ((rx_block[0] & RFID_ISO14443_PCB_BLOCK_MASK) !=
			    RFID_ISO14443_PCB_RBLOCK) {
				ret = -EBADMSG;
				goto unlock;
			}
			/* TODO retry */

			/* Toggle block bit if RX bit is different than current */
			info->block_num ^= (rx_block[0] ^ RFID_ISO14443_PCB_BLOCK_NUM);
		}

		tx_block[0] =
			RFID_ISO14443_PCB_IBLOCK | RFID_ISO14443_PCB_IBLOCK_FXD | info->block_num;
		tx_block_index = 1U;
		if (info->cid_supported && info->cid != 0U) {
			tx_block[0] |= RFID_ISO14443_PCB_BLOCK_CID;
			tx_block[tx_block_index] = info->cid;
			tx_block_index++;
		}
		if (info->nad_supported && nad != 0U) {
			tx_block[0] |= RFID_ISO14443_PCB_BLOCK_NAD;
			tx_block[tx_block_index] = nad;
			tx_block_index++;
		}

		tx_block_len = tx_len - tx_offset;

		if (tx_block_len > tx_block_max_len - tx_block_index) {
			tx_block[0] |= RFID_ISO14443_PCB_IBLOCK_CHAINING;
			tx_block_len = tx_block_max_len - tx_block_index;
		}

		memcpy(&tx_block[tx_block_index], &tx_data[tx_offset], tx_block_len);
		tx_offset += tx_block_len;

		ret = rfid_target_transmit(dev, tx_block, tx_block_index + tx_block_len, 8U);
		if (ret < 0) {
			LOG_ERR("Failed to transmit data (%d)", ret);
			goto unlock;
		}
	}

unlock:
	if (ret < 0) {
		LOG_ERR("Failed to transmit data (%d)", ret);
	}
	(void)rfid_release(dev);

	return ret;
}
