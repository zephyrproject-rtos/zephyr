/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(osdp, CONFIG_OSDP_LOG_LEVEL);

#include <string.h>
#include "osdp_common.h"

#define OSDP_PKT_MARK                  0xFF
#define OSDP_PKT_SOM                   0x53
#define PKT_CONTROL_SQN                0x03
#define PKT_CONTROL_CRC                0x04
#define PKT_CONTROL_SCB                0x08

struct osdp_packet_header {
	uint8_t som;
	uint8_t pd_address;
	uint8_t len_lsb;
	uint8_t len_msb;
	uint8_t control;
	uint8_t data[];
} __packed;

static inline bool packet_has_mark(struct osdp_pd *pd)
{
	return ISSET_FLAG(pd, PD_FLAG_PKT_HAS_MARK);
}

static inline void packet_set_mark(struct osdp_pd *pd, bool mark)
{
	if (mark) {
		SET_FLAG(pd, PD_FLAG_PKT_HAS_MARK);
	} else {
		CLEAR_FLAG(pd, PD_FLAG_PKT_HAS_MARK);
	}
}

uint8_t osdp_compute_checksum(uint8_t *msg, int length)
{
	uint8_t checksum = 0;
	int i, whole_checksum;

	whole_checksum = 0;
	for (i = 0; i < length; i++) {
		whole_checksum += msg[i];
		checksum = ~(0xff & whole_checksum) + 1;
	}
	return checksum;
}

static int osdp_phy_get_seq_number(struct osdp_pd *pd, int do_inc)
{
	/* pd->seq_num is set to -1 to reset phy cmd state */
	if (do_inc) {
		pd->seq_number += 1;
		if (pd->seq_number > 3) {
			pd->seq_number = 1;
		}
	}
	return pd->seq_number & PKT_CONTROL_SQN;
}

int osdp_phy_packet_get_data_offset(struct osdp_pd *pd, const uint8_t *buf)
{
	int sb_len = 0, mark_byte_len = 0;
	struct osdp_packet_header *pkt;

	ARG_UNUSED(pd);
	if (packet_has_mark(pd)) {
		mark_byte_len = 1;
		buf += 1;
	}
	pkt = (struct osdp_packet_header *)buf;
	if (pkt->control & PKT_CONTROL_SCB) {
		sb_len = pkt->data[0];
	}
	return mark_byte_len + sizeof(struct osdp_packet_header) + sb_len;
}

uint8_t *osdp_phy_packet_get_smb(struct osdp_pd *pd, const uint8_t *buf)
{
	struct osdp_packet_header *pkt;

	ARG_UNUSED(pd);
	if (packet_has_mark(pd)) {
		buf += 1;
	}
	pkt = (struct osdp_packet_header *)buf;
	if (pkt->control & PKT_CONTROL_SCB) {
		return pkt->data;
	}
	return NULL;
}

int osdp_phy_in_sc_handshake(int is_reply, int id)
{
	if (is_reply) {
		return (id == REPLY_CCRYPT || id == REPLY_RMAC_I);
	} else {
		return (id == CMD_CHLNG || id == CMD_SCRYPT);
	}
}

int osdp_phy_packet_init(struct osdp_pd *pd, uint8_t *buf, int max_len)
{
	int exp_len, id, scb_len = 0, mark_byte_len = 0;
	struct osdp_packet_header *pkt;

	exp_len = sizeof(struct osdp_packet_header) + 64; /* 64 is estimated */
	if (max_len < exp_len) {
		LOG_ERR("packet_init: out of space! CMD: %02x", pd->cmd_id);
		return OSDP_ERR_PKT_FMT;
	}

	/**
	 * In PD mode just follow what we received from CP. In CP mode, as we
	 * initiate the transaction, choose based on CONFIG_OSDP_SKIP_MARK_BYTE.
	 */
	if ((is_pd_mode(pd) && packet_has_mark(pd)) ||
	    (is_cp_mode(pd) && !ISSET_FLAG(pd, PD_FLAG_PKT_SKIP_MARK))) {
		buf[0] = OSDP_PKT_MARK;
		buf++;
		mark_byte_len = 1;
		packet_set_mark(pd, true);
	}

	/* Fill packet header */
	pkt = (struct osdp_packet_header *)buf;
	pkt->som = OSDP_PKT_SOM;
	pkt->pd_address = pd->address & 0x7F;	/* Use only the lower 7 bits */
	if (is_pd_mode(pd)) {
		/* PD must reply with MSB of it's address set */
		pkt->pd_address |= 0x80;
		id = pd->reply_id;
	} else {
		id = pd->cmd_id;
	}
	pkt->control = osdp_phy_get_seq_number(pd, is_cp_mode(pd));
	pkt->control |= PKT_CONTROL_CRC;

	if (sc_is_active(pd)) {
		pkt->control |= PKT_CONTROL_SCB;
		pkt->data[0] = scb_len = 2;
		pkt->data[1] = SCS_15;
	} else if (osdp_phy_in_sc_handshake(is_pd_mode(pd), id)) {
		pkt->control |= PKT_CONTROL_SCB;
		pkt->data[0] = scb_len = 3;
		pkt->data[1] = SCS_11;
	}

	return mark_byte_len + sizeof(struct osdp_packet_header) + scb_len;
}

int osdp_phy_packet_finalize(struct osdp_pd *pd, uint8_t *buf,
			     int len, int max_len)
{
	uint16_t crc16;
	struct osdp_packet_header *pkt;

	/* Do a sanity check only; we expect header to be pre-filled */
	if ((unsigned long)len <= sizeof(struct osdp_packet_header)) {
		LOG_ERR("PKT_F: Invalid header");
		return OSDP_ERR_PKT_FMT;
	}

	if (packet_has_mark(pd)) {
		if (buf[0] != OSDP_PKT_MARK) {
			LOG_ERR("PKT_F: MARK validation failed! ID: 0x%02x",
				is_cp_mode(pd) ? pd->cmd_id : pd->reply_id);
			return OSDP_ERR_PKT_FMT;
		}
		/* temporarily get rid of mark byte */
		buf += 1;
		len -= 1;
		max_len -= 1;
	}
	pkt = (struct osdp_packet_header *)buf;
	if (pkt->som != OSDP_PKT_SOM) {
		LOG_ERR("PKT_F: header SOM validation failed! ID: 0x%02x",
			is_cp_mode(pd) ? pd->cmd_id : pd->reply_id);
		return OSDP_ERR_PKT_FMT;
	}

	/* len: with 2 byte CRC */
	pkt->len_lsb = BYTE_0(len + 2);
	pkt->len_msb = BYTE_1(len + 2);

#ifdef CONFIG_OSDP_SC_ENABLED
	uint8_t *data;
	int i, data_len;

	if (sc_is_active(pd) &&
	    (pkt->control & PKT_CONTROL_SCB) && pkt->data[1] >= SCS_15)
		if (pkt->data[1] == SCS_17 || pkt->data[1] == SCS_18) {
			/**
			 * Only the data portion of message (after id byte)
			 * is encrypted. While (en/de)crypting, we must skip
			 * header, security block, and cmd/reply ID byte.
			 *
			 * Note: if cmd/reply has no data, we must set type to
			 * SCS_15/SCS_16 and send them.
			 */
			data = pkt->data + pkt->data[0] + 1;
			data_len = len - (sizeof(struct osdp_packet_header) +
					  pkt->data[0] + 1);
			len -= data_len;
			/**
			 * check if the passed buffer can hold the encrypted
			 * data where length may be rounded up to the nearest
			 * 16 byte block boundary.
			 */
			if (AES_PAD_LEN(data_len + 1) > max_len) {
				/* data_len + 1 for OSDP_SC_EOM_MARKER */
				goto out_of_space_error;
			}
			len += osdp_encrypt_data(pd, is_cp_mode(pd), data, data_len);
		}
		/* len: with 4bytes MAC; with 2 byte CRC; without 1 byte mark */
		if (len + 4 > max_len) {
			goto out_of_space_error;
		}

		/* len: with 2 byte CRC; with 4 byte MAC */
		pkt->len_lsb = BYTE_0(len + 2 + 4);
		pkt->len_msb = BYTE_1(len + 2 + 4);

		/* compute and extend the buf with 4 MAC bytes */
		osdp_compute_mac(pd, is_cp_mode(pd), buf, len);
		data = is_cp_mode(pd) ? pd->sc.c_mac : pd->sc.r_mac;
		memcpy(buf + len, data, 4);
		len += 4;
	}
#endif /* CONFIG_OSDP_SC_ENABLED */

	/* fill crc16 */
	if (len + 2 > max_len) {
		goto out_of_space_error;
	}
	crc16 = osdp_compute_crc16(buf, len);
	buf[len + 0] = BYTE_0(crc16);
	buf[len + 1] = BYTE_1(crc16);
	len += 2;

	if (packet_has_mark(pd)) {
		len += 1; /* put back mark byte */
	}

	return len;

out_of_space_error:
	LOG_ERR("PKT_F: Out of buffer space! CMD(%02x)", pd->cmd_id);
	return OSDP_ERR_PKT_FMT;
}

int osdp_phy_decode_packet(struct osdp_pd *pd, uint8_t *buf, int len)
{
	uint8_t *data;
	uint16_t comp, cur;
	int pkt_len, pd_addr, mac_offset;
	struct osdp_packet_header *pkt;

	/* wait till we have the header */
	if ((unsigned long)len < sizeof(struct osdp_packet_header)) {
		/* incomplete data */
		return OSDP_ERR_PKT_WAIT;
	}

	packet_set_mark(pd, false);
	if (buf[0] == OSDP_PKT_MARK) {
		buf += 1;
		len -= 1;
		packet_set_mark(pd, true);
	}

	pkt = (struct osdp_packet_header *)buf;

	/* validate packet header */
	if (pkt->som != OSDP_PKT_SOM) {
		LOG_ERR("Invalid SOM 0x%02x", pkt->som);
		return OSDP_ERR_PKT_FMT;
	}

	if (is_cp_mode(pd) && !(pkt->pd_address & 0x80)) {
		LOG_ERR("Reply without address MSB set!");
		return OSDP_ERR_PKT_FMT;
	}

	/* validate packet length */
	pkt_len = (pkt->len_msb << 8) | pkt->len_lsb;
	if (len < pkt_len) {
		/* wait for more data? */
		return OSDP_ERR_PKT_WAIT;
	}

	/* validate PD address */
	pd_addr = pkt->pd_address & 0x7F;
	if (pd_addr != pd->address && pd_addr != 0x7F) {
		/* not addressed to us and was not broadcasted */
		if (is_cp_mode(pd)) {
			LOG_ERR("Invalid pd address %d", pd_addr);
			return OSDP_ERR_PKT_FMT;
		}
		LOG_DBG("cmd for PD[%d] discarded", pd_addr);
		return OSDP_ERR_PKT_SKIP;
	}

	/* validate sequence number */
	cur = pkt->control & PKT_CONTROL_SQN;
	if (is_pd_mode(pd) && cur == 0) {
		/**
		 * CP is trying to restart communication by sending a 0. The
		 * current PD implementation does not hold any state between
		 * commands so we can just set seq_number to -1 (so it gets
		 * incremented to 0 with a call to phy_get_seq_number()) and
		 * invalidate any established secure channels.
		 */
		pd->seq_number = -1;
		CLEAR_FLAG(pd, PD_FLAG_SC_ACTIVE);
	}
	if (is_pd_mode(pd) && cur == pd->seq_number) {
		/**
		 * TODO: PD must resend the last response if CP send the same
		 * sequence number again.
		 */
		LOG_ERR("seq-repeat/reply-resend feature not supported!");
		pd->reply_id = REPLY_NAK;
		pd->ephemeral_data[0] = OSDP_PD_NAK_SEQ_NUM;
		return OSDP_ERR_PKT_FMT;
	}
	comp = osdp_phy_get_seq_number(pd, is_pd_mode(pd));
	if (comp != cur && !ISSET_FLAG(pd, PD_FLAG_SKIP_SEQ_CHECK)) {
		LOG_ERR("packet seq mismatch %d/%d", comp, cur);
		pd->reply_id = REPLY_NAK;
		pd->ephemeral_data[0] = OSDP_PD_NAK_SEQ_NUM;
		return OSDP_ERR_PKT_FMT;
	}
	len -= sizeof(struct osdp_packet_header); /* consume header */

	/* validate CRC/checksum */
	if (pkt->control & PKT_CONTROL_CRC) {
		cur = (buf[pkt_len] << 8) | buf[pkt_len - 1];
		comp = osdp_compute_crc16(buf + 1, pkt_len - 2);
		if (comp != cur) {
			LOG_ERR("invalid crc 0x%04x/0x%04x", comp, cur);
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_MSG_CHK;
			return OSDP_ERR_PKT_FMT;
		}
		mac_offset = pkt_len - 4 - 2;
		len -= 2; /* consume CRC */
	} else {
		comp = osdp_compute_checksum(buf + 1, pkt_len - 1);
		if (comp != buf[len - 1]) {
			LOG_ERR("invalid checksum %02x/%02x", comp, cur);
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_MSG_CHK;
			return OSDP_ERR_PKT_FMT;
		}
		mac_offset = pkt_len - 4 - 1;
		len -= 1; /* consume checksum */
	}

	data = pkt->data;

#ifdef CONFIG_OSDP_SC_ENABLED
	uint8_t *mac;
	int is_cmd;

	if (pkt->control & PKT_CONTROL_SCB) {
		if (is_pd_mode(pd) && !sc_is_capable(pd)) {
			LOG_ERR("PD is not SC capable");
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_UNSUP;
			return OSDP_ERR_PKT_FMT;
		}
		if (pkt->data[1] < SCS_11 || pkt->data[1] > SCS_18) {
			LOG_ERR("Invalid SB Type");
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_COND;
			return OSDP_ERR_PKT_FMT;
		}
		if (!sc_is_active(pd) && pkt->data[1] > SCS_14) {
			LOG_ERR("Received invalid secure message!");
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_COND;
			return OSDP_ERR_PKT_FMT;
		}
		if (pkt->data[1] == SCS_11 || pkt->data[1] == SCS_13) {
			/**
			 * CP signals PD to use SCBKD by setting SB data byte
			 * to 0. In CP, PD_FLAG_SC_USE_SCBKD comes from FSM; on
			 * PD we extract it from the command itself. But this
			 * usage of SCBKD is allowed only when the PD is in
			 * install mode (indicated by PD_FLAG_INSTALL_MODE).
			 */
			if (ISSET_FLAG(pd, PD_FLAG_INSTALL_MODE) &&
			    pkt->data[2] == 0) {
				SET_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
			}
		}
		data = pkt->data + pkt->data[0];
		len -= pkt->data[0]; /* consume security block */
	} else {
		if (sc_is_active(pd)) {
			LOG_ERR("Received plain-text message in SC");
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_COND;
			return OSDP_ERR_PKT_FMT;
		}
	}

	if (sc_is_active(pd) &&
	    pkt->control & PKT_CONTROL_SCB && pkt->data[1] >= SCS_15) {
		/* validate MAC */
		is_cmd = is_pd_mode(pd);
		osdp_compute_mac(pd, is_cmd, buf, mac_offset);
		mac = is_cmd ? pd->sc.c_mac : pd->sc.r_mac;
		if (memcmp(buf + mac_offset, mac, 4) != 0) {
			LOG_ERR("Invalid MAC; discarding SC");
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_COND;
			return OSDP_ERR_PKT_FMT;
		}
		len -= 4; /* consume MAC */

		/* decrypt data block */
		if (pkt->data[1] == SCS_17 || pkt->data[1] == SCS_18) {
			/**
			 * Only the data portion of message (after id byte)
			 * is encrypted. While (en/de)crypting, we must skip
			 * header (6), security block (2) and cmd/reply id (1)
			 * bytes if cmd/reply has no data, use SCS_15/SCS_16.
			 *
			 * At this point, the header and security block is
			 * already consumed. So we can just skip the cmd/reply
			 * ID (data[0])  when calling osdp_decrypt_data().
			 */
			len = osdp_decrypt_data(pd, is_cmd, data + 1, len - 1);
			if (len <= 0) {
				LOG_ERR("Failed at decrypt; discarding SC");
				pd->reply_id = REPLY_NAK;
				pd->ephemeral_data[0] = OSDP_PD_NAK_SC_COND;
				return OSDP_ERR_PKT_FMT;
			}
			len += 1; /* put back cmd/reply ID */
		}
	}
#endif /* CONFIG_OSDP_SC_ENABLED */

	memmove(buf, data, len);
	return len;
}

void osdp_phy_state_reset(struct osdp_pd *pd)
{
#ifdef CONFIG_OSDP_MODE_CP
	pd->phy_state = 0;
#endif
	pd->seq_number = -1;
	pd->rx_buf_len = 0;
}
