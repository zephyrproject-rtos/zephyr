/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(osdp, CONFIG_OSDP_LOG_LEVEL);

#include "osdp_common.h"

#define OSDP_PD_POLL_TIMEOUT_MS        (1000 / CONFIG_OSDP_PD_POLL_RATE)
#define OSDP_CMD_RETRY_WAIT_MS         (CONFIG_OSDP_CMD_RETRY_WAIT_SEC * 1000)

#ifdef CONFIG_OSDP_SC_ENABLED
#define OSDP_PD_SC_RETRY_MS            (CONFIG_OSDP_SC_RETRY_WAIT_SEC * 1000)
#endif

#define CMD_POLL_LEN                   1
#define CMD_LSTAT_LEN                  1
#define CMD_ISTAT_LEN                  1
#define CMD_OSTAT_LEN                  1
#define CMD_RSTAT_LEN                  1
#define CMD_ID_LEN                     2
#define CMD_CAP_LEN                    2
#define CMD_DIAG_LEN                   2
#define CMD_OUT_LEN                    5
#define CMD_LED_LEN                    15
#define CMD_BUZ_LEN                    6
#define CMD_TEXT_LEN                   7   /* variable length command */
#define CMD_COMSET_LEN                 6
#define CMD_KEYSET_LEN                 19
#define CMD_CHLNG_LEN                  9
#define CMD_SCRYPT_LEN                 17

#define REPLY_ACK_DATA_LEN             0
#define REPLY_PDID_DATA_LEN            12
#define REPLY_PDCAP_ENTITY_LEN         3
#define REPLY_LSTATR_DATA_LEN          2
#define REPLY_RSTATR_DATA_LEN          1
#define REPLY_COM_DATA_LEN             5
#define REPLY_NAK_DATA_LEN             1
#define REPLY_CCRYPT_DATA_LEN          32
#define REPLY_RMAC_I_DATA_LEN          16
#define REPLY_KEYPPAD_DATA_LEN         2   /* variable length command */
#define REPLY_RAW_DATA_LEN             4   /* variable length command */
#define REPLY_FMT_DATA_LEN             3   /* variable length command */
#define REPLY_BUSY_DATA_LEN            0

enum osdp_cp_error_e {
	OSDP_CP_ERR_NONE = 0,
	OSDP_CP_ERR_GENERIC = -1,
	OSDP_CP_ERR_NO_DATA = -2,
	OSDP_CP_ERR_RETRY_CMD = -3,
	OSDP_CP_ERR_CAN_YIELD = -4,
	OSDP_CP_ERR_INPROG = -5,
	OSDP_CP_ERR_UNKNOWN = -6,
};


static struct osdp_cmd *cp_cmd_alloc(struct osdp_pd *pd)
{
	struct osdp_cmd *cmd = NULL;

	if (k_mem_slab_alloc(&pd->cmd.slab, (void **)&cmd, K_MSEC(100))) {
		LOG_ERR("Memory allocation time-out");
		return NULL;
	}
	return cmd;
}

static void cp_cmd_free(struct osdp_pd *pd, struct osdp_cmd *cmd)
{
	k_mem_slab_free(&pd->cmd.slab, (void *)cmd);
}

static void cp_cmd_enqueue(struct osdp_pd *pd, struct osdp_cmd *cmd)
{
	sys_slist_append(&pd->cmd.queue, &cmd->node);
}

static int cp_cmd_dequeue(struct osdp_pd *pd, struct osdp_cmd **cmd)
{
	sys_snode_t *node;

	node = sys_slist_peek_head(&pd->cmd.queue);
	if (node == NULL) {
		return -1;
	}
	sys_slist_remove(&pd->cmd.queue, NULL, node);
	*cmd = CONTAINER_OF(node, struct osdp_cmd, node);
	return 0;
}

int osdp_extract_address(int *address)
{
	int pd_offset = 0;
	unsigned long addr;
	char *tok, *s1, *s2, addr_buf[32 * CONFIG_OSDP_NUM_CONNECTED_PD];

	strncpy(addr_buf, CONFIG_OSDP_PD_ADDRESS_LIST, sizeof(addr_buf) - 1);
	addr_buf[sizeof(addr_buf) - 1] = '\0';
	tok = strtok_r(addr_buf, ", ", &s1);
	while (tok && pd_offset < CONFIG_OSDP_NUM_CONNECTED_PD) {
		addr = strtoul(tok, &s2, 10);
		if (*s2 != '\0') { /* tok must be number-ish */
			return -1;
		}
		address[pd_offset] = addr;
		pd_offset++;
		tok = strtok_r(NULL, ", ", &s1);
	}
	return (pd_offset == CONFIG_OSDP_NUM_CONNECTED_PD) ? 0 : -1;
}

static inline bool check_buf_len(int need, int have)
{
	if (need > have) {
		LOG_ERR("OOM at build command: need:%d have:%d", need, have);
		return false;
	}
	return true;
}

static int cp_build_command(struct osdp_pd *pd, uint8_t *buf, int max_len)
{
	struct osdp_cmd *cmd = NULL;
	int len = 0;
	int data_off = osdp_phy_packet_get_data_offset(pd, buf);
#ifdef CONFIG_OSDP_SC_ENABLED
	uint8_t *smb = osdp_phy_packet_get_smb(pd, buf);
#endif

	buf += data_off;
	max_len -= data_off;
	if (max_len <= 0) {
		return OSDP_CP_ERR_GENERIC;
	}

	switch (pd->cmd_id) {
	case CMD_POLL:
		if (!check_buf_len(CMD_POLL_LEN, max_len)) {
			return OSDP_CP_ERR_GENERIC;
		}
		buf[len++] = pd->cmd_id;
		break;
	case CMD_LSTAT:
		if (!check_buf_len(CMD_LSTAT_LEN, max_len)) {
			return OSDP_CP_ERR_GENERIC;
		}
		buf[len++] = pd->cmd_id;
		break;
	case CMD_ISTAT:
		if (!check_buf_len(CMD_ISTAT_LEN, max_len)) {
			return OSDP_CP_ERR_GENERIC;
		}
		buf[len++] = pd->cmd_id;
		break;
	case CMD_OSTAT:
		if (!check_buf_len(CMD_OSTAT_LEN, max_len)) {
			return OSDP_CP_ERR_GENERIC;
		}
		buf[len++] = pd->cmd_id;
		break;
	case CMD_RSTAT:
		if (!check_buf_len(CMD_RSTAT_LEN, max_len)) {
			return OSDP_CP_ERR_GENERIC;
		}
		buf[len++] = pd->cmd_id;
		break;
	case CMD_ID:
		if (!check_buf_len(CMD_ID_LEN, max_len)) {
			return OSDP_CP_ERR_GENERIC;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = 0x00;
		break;
	case CMD_CAP:
		if (!check_buf_len(CMD_CAP_LEN, max_len)) {
			return OSDP_CP_ERR_GENERIC;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = 0x00;
		break;
	case CMD_DIAG:
		if (!check_buf_len(CMD_DIAG_LEN, max_len)) {
			return OSDP_CP_ERR_GENERIC;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = 0x00;
		break;
	case CMD_OUT:
		if (!check_buf_len(CMD_OUT_LEN, max_len)) {
			return OSDP_CP_ERR_GENERIC;
		}
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->output.output_no;
		buf[len++] = cmd->output.control_code;
		buf[len++] = BYTE_0(cmd->output.timer_count);
		buf[len++] = BYTE_1(cmd->output.timer_count);
		break;
	case CMD_LED:
		if (!check_buf_len(CMD_LED_LEN, max_len)) {
			return OSDP_CP_ERR_GENERIC;
		}
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->led.reader;
		buf[len++] = cmd->led.led_number;

		buf[len++] = cmd->led.temporary.control_code;
		buf[len++] = cmd->led.temporary.on_count;
		buf[len++] = cmd->led.temporary.off_count;
		buf[len++] = cmd->led.temporary.on_color;
		buf[len++] = cmd->led.temporary.off_color;
		buf[len++] = BYTE_0(cmd->led.temporary.timer_count);
		buf[len++] = BYTE_1(cmd->led.temporary.timer_count);

		buf[len++] = cmd->led.permanent.control_code;
		buf[len++] = cmd->led.permanent.on_count;
		buf[len++] = cmd->led.permanent.off_count;
		buf[len++] = cmd->led.permanent.on_color;
		buf[len++] = cmd->led.permanent.off_color;
		break;
	case CMD_BUZ:
		if (!check_buf_len(CMD_BUZ_LEN, max_len)) {
			return OSDP_CP_ERR_GENERIC;
		}
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->buzzer.reader;
		buf[len++] = cmd->buzzer.control_code;
		buf[len++] = cmd->buzzer.on_count;
		buf[len++] = cmd->buzzer.off_count;
		buf[len++] = cmd->buzzer.rep_count;
		break;
	case CMD_TEXT:
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		if (!check_buf_len(CMD_TEXT_LEN + cmd->text.length, max_len)) {
			return OSDP_CP_ERR_GENERIC;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->text.reader;
		buf[len++] = cmd->text.control_code;
		buf[len++] = cmd->text.temp_time;
		buf[len++] = cmd->text.offset_row;
		buf[len++] = cmd->text.offset_col;
		buf[len++] = cmd->text.length;
		memcpy(buf + len, cmd->text.data, cmd->text.length);
		len += cmd->text.length;
		break;
	case CMD_COMSET:
		if (!check_buf_len(CMD_COMSET_LEN, max_len)) {
			return OSDP_CP_ERR_GENERIC;
		}
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->comset.address;
		buf[len++] = BYTE_0(cmd->comset.baud_rate);
		buf[len++] = BYTE_1(cmd->comset.baud_rate);
		buf[len++] = BYTE_2(cmd->comset.baud_rate);
		buf[len++] = BYTE_3(cmd->comset.baud_rate);
		break;
#ifdef CONFIG_OSDP_SC_ENABLED
	case CMD_KEYSET:
		if (!sc_is_active(pd)) {
			LOG_ERR("Cannot perform KEYSET without SC!");
			return OSDP_CP_ERR_GENERIC;
		}
		if (!check_buf_len(CMD_KEYSET_LEN, max_len)) {
			return OSDP_CP_ERR_GENERIC;
		}
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		if (cmd->keyset.length != 16) {
			LOG_ERR("Invalid key length");
			return OSDP_CP_ERR_GENERIC;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = 1;  /* key type (1: SCBK) */
		buf[len++] = 16; /* key length in bytes */
		if (cmd->keyset.type == 1) { /* SCBK */
			memcpy(buf + len, cmd->keyset.data, 16);
		} else if (cmd->keyset.type == 0) {  /* master_key */
			osdp_compute_scbk(pd, cmd->keyset.data, buf + len);
		} else {
			LOG_ERR("Unknown key type (%d)", cmd->keyset.type);
			return OSDP_CP_ERR_GENERIC;
		}
		len += 16;
		break;
	case CMD_CHLNG:
		if (!check_buf_len(CMD_CHLNG_LEN, max_len)) {
			return OSDP_CP_ERR_GENERIC;
		}
		if (smb == NULL) {
			LOG_ERR("Invalid secure message block!");
			return -1;
		}
		smb[0] = 3;       /* length */
		smb[1] = SCS_11;  /* type */
		smb[2] = ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD) ? 0 : 1;
		buf[len++] = pd->cmd_id;
		memcpy(buf + len, pd->sc.cp_random, 8);
		len += 8;
		break;
	case CMD_SCRYPT:
		if (!check_buf_len(CMD_SCRYPT_LEN, max_len)) {
			return OSDP_CP_ERR_GENERIC;
		}
		if (smb == NULL) {
			LOG_ERR("Invalid secure message block!");
			return -1;
		}
		osdp_compute_cp_cryptogram(pd);
		smb[0] = 3;       /* length */
		smb[1] = SCS_13;  /* type */
		smb[2] = ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD) ? 0 : 1;
		buf[len++] = pd->cmd_id;
		memcpy(buf + len, pd->sc.cp_cryptogram, 16);
		len += 16;
		break;
#endif /* CONFIG_OSDP_SC_ENABLED */
	default:
		LOG_ERR("Unknown/Unsupported CMD(%02x)", pd->cmd_id);
		return OSDP_CP_ERR_GENERIC;
	}

#ifdef CONFIG_OSDP_SC_ENABLED
	if (smb && (smb[1] > SCS_14) && sc_is_active(pd)) {
		/**
		 * When SC active and current cmd is not a handshake (<= SCS_14)
		 * then we must set SCS type to 17 if this message has data
		 * bytes and 15 otherwise.
		 */
		smb[0] = 2;
		smb[1] = (len > 1) ? SCS_17 : SCS_15;
	}
#endif /* CONFIG_OSDP_SC_ENABLED */

	return len;
}

static int cp_decode_response(struct osdp_pd *pd, uint8_t *buf, int len)
{
	uint32_t temp32;
	struct osdp *ctx = pd_to_osdp(pd);
	int ret = OSDP_CP_ERR_GENERIC, pos = 0, t1, t2;
	struct osdp_event event;

	pd->reply_id = buf[pos++];
	len--;		/* consume reply id from the head */

	switch (pd->reply_id) {
	case REPLY_ACK:
		if (len != REPLY_ACK_DATA_LEN) {
			break;
		}
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_NAK:
		if (len != REPLY_NAK_DATA_LEN) {
			break;
		}
		LOG_WRN("PD replied with NAK(%d) for CMD(%02x)",
			buf[pos], pd->cmd_id);
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_PDID:
		if (len != REPLY_PDID_DATA_LEN) {
			break;
		}
		pd->id.vendor_code  = buf[pos++];
		pd->id.vendor_code |= buf[pos++] << 8;
		pd->id.vendor_code |= buf[pos++] << 16;

		pd->id.model = buf[pos++];
		pd->id.version = buf[pos++];

		pd->id.serial_number = buf[pos++];
		pd->id.serial_number |= buf[pos++] << 8;
		pd->id.serial_number |= buf[pos++] << 16;
		pd->id.serial_number |= buf[pos++] << 24;

		pd->id.firmware_version = buf[pos++] << 16;
		pd->id.firmware_version |= buf[pos++] << 8;
		pd->id.firmware_version |= buf[pos++];
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_PDCAP:
		if ((len % REPLY_PDCAP_ENTITY_LEN) != 0) {
			LOG_ERR("PDCAP response length is not a multiple of 3");
			return OSDP_CP_ERR_GENERIC;
		}
		while (pos < len) {
			t1 = buf[pos++]; /* func_code */
			if (t1 >= OSDP_PD_CAP_SENTINEL) {
				break;
			}
			pd->cap[t1].function_code = t1;
			pd->cap[t1].compliance_level = buf[pos++];
			pd->cap[t1].num_items = buf[pos++];
		}

		/* post-capabilities hooks */
		t2 = OSDP_PD_CAP_COMMUNICATION_SECURITY;
		if (pd->cap[t2].compliance_level & 0x01) {
			SET_FLAG(pd, PD_FLAG_SC_CAPABLE);
		} else {
			CLEAR_FLAG(pd, PD_FLAG_SC_CAPABLE);
		}
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_LSTATR:
		if (len != REPLY_LSTATR_DATA_LEN) {
			break;
		}
		if (buf[pos++]) {
			SET_FLAG(pd, PD_FLAG_TAMPER);
		} else {
			CLEAR_FLAG(pd, PD_FLAG_TAMPER);
		}
		if (buf[pos++]) {
			SET_FLAG(pd, PD_FLAG_POWER);
		} else {
			CLEAR_FLAG(pd, PD_FLAG_POWER);
		}
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_RSTATR:
		if (len != REPLY_RSTATR_DATA_LEN) {
			break;
		}
		if (buf[pos++]) {
			SET_FLAG(pd, PD_FLAG_R_TAMPER);
		} else {
			CLEAR_FLAG(pd, PD_FLAG_R_TAMPER);
		}
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_COM:
		if (len != REPLY_COM_DATA_LEN) {
			break;
		}
		t1 = buf[pos++];
		temp32 = buf[pos++];
		temp32 |= buf[pos++] << 8;
		temp32 |= buf[pos++] << 16;
		temp32 |= buf[pos++] << 24;
		LOG_WRN("COMSET responded with ID:%d Baud:%d", t1, temp32);
		pd->address = t1;
		pd->baud_rate = temp32;
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_KEYPPAD:
		if (len < REPLY_KEYPPAD_DATA_LEN || !ctx->event_callback) {
			break;
		}
		event.type = OSDP_EVENT_KEYPRESS;
		event.keypress.reader_no = buf[pos++];
		event.keypress.length = buf[pos++];
		if ((len - REPLY_KEYPPAD_DATA_LEN) != event.keypress.length) {
			break;
		}
		memcpy(event.keypress.data, buf + pos, event.keypress.length);
		ctx->event_callback(ctx->event_callback_arg, pd->idx, &event);
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_RAW:
		if (len < REPLY_RAW_DATA_LEN || !ctx->event_callback) {
			break;
		}
		event.type = OSDP_EVENT_CARDREAD;
		event.cardread.reader_no = buf[pos++];
		event.cardread.format = buf[pos++];
		event.cardread.length = buf[pos++]; /* bits LSB */
		event.cardread.length |= buf[pos++] << 8; /* bits MSB */
		event.cardread.direction = 0; /* un-specified */
		t1 = (event.cardread.length + 7) / 8; /* len: bytes */
		if (t1 != (len - REPLY_RAW_DATA_LEN)) {
			break;
		}
		memcpy(event.cardread.data, buf + pos, t1);
		ctx->event_callback(ctx->event_callback_arg, pd->idx, &event);
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_FMT:
		if (len < REPLY_FMT_DATA_LEN || !ctx->event_callback) {
			break;
		}
		event.type = OSDP_EVENT_CARDREAD;
		event.cardread.reader_no = buf[pos++];
		event.cardread.direction = buf[pos++];
		event.cardread.length = buf[pos++];
		event.cardread.format = OSDP_CARD_FMT_ASCII;
		if (event.cardread.length != (len - REPLY_FMT_DATA_LEN) ||
		    event.cardread.length > OSDP_EVENT_MAX_DATALEN) {
			break;
		}
		memcpy(event.cardread.data, buf + pos, event.cardread.length);
		ctx->event_callback(ctx->event_callback_arg, pd->idx, &event);
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_BUSY:
		/* PD busy; signal upper layer to retry command */
		if (len != REPLY_BUSY_DATA_LEN) {
			break;
		}
		ret = OSDP_CP_ERR_RETRY_CMD;
		break;
#ifdef CONFIG_OSDP_SC_ENABLED
	case REPLY_CCRYPT:
		if (sc_is_active(pd) || pd->cmd_id != CMD_CHLNG) {
			LOG_ERR("Out of order REPLY_CCRYPT; has PD gone rogue?");
			break;
		}
		if (len != REPLY_CCRYPT_DATA_LEN) {
			break;
		}
		memcpy(pd->sc.pd_client_uid, buf + pos, 8);
		memcpy(pd->sc.pd_random, buf + pos + 8, 8);
		memcpy(pd->sc.pd_cryptogram, buf + pos + 16, 16);
		pos += 32;
		osdp_compute_session_keys(pd);
		if (osdp_verify_pd_cryptogram(pd) != 0) {
			LOG_ERR("Failed to verify PD cryptogram");
			return OSDP_CP_ERR_GENERIC;
		}
		ret = OSDP_CP_ERR_NONE;
		break;
	case REPLY_RMAC_I:
		if (sc_is_active(pd) || pd->cmd_id != CMD_SCRYPT) {
			LOG_ERR("Out of order REPLY_RMAC_I; has PD gone rogue?");
			break;
		}
		if (len != REPLY_RMAC_I_DATA_LEN) {
			break;
		}
		memcpy(pd->sc.r_mac, buf + pos, 16);
		sc_activate(pd);
		ret = OSDP_CP_ERR_NONE;
		break;
#endif /* CONFIG_OSDP_SC_ENABLED */
	default:
		LOG_WRN("Unexpected REPLY(%02x) for CMD(%02x)",
			pd->cmd_id, pd->reply_id);
		return OSDP_CP_ERR_UNKNOWN;
	}

	if (ret != OSDP_CP_ERR_NONE) {
		LOG_ERR("Failed to decode response: REPLY(%02x) for CMD(%02x)",
			pd->cmd_id, pd->reply_id);
	}

	if (pd->cmd_id != CMD_POLL) {
		LOG_DBG("CMD(%02x) REPLY(%02x)", pd->cmd_id, pd->reply_id);
	}

	return ret;
}

static int cp_build_packet(struct osdp_pd *pd)
{
	int ret, len;

	/* init packet buf with header */
	len = osdp_phy_packet_init(pd, pd->rx_buf, sizeof(pd->rx_buf));
	if (len < 0) {
		return OSDP_CP_ERR_GENERIC;
	}

	/* fill command data */
	ret = cp_build_command(pd, pd->rx_buf, sizeof(pd->rx_buf));
	if (ret < 0) {
		return OSDP_CP_ERR_GENERIC;
	}
	len += ret;

	/* finalize packet */
	len = osdp_phy_packet_finalize(pd, pd->rx_buf, len, sizeof(pd->rx_buf));
	if (len < 0) {
		return OSDP_CP_ERR_GENERIC;
	}

	pd->rx_buf_len = len;

	return OSDP_CP_ERR_NONE;
}

static int cp_send_command(struct osdp_pd *pd)
{
	int ret;

	/* flush rx to remove any invalid data. */
	if (pd->channel.flush) {
		pd->channel.flush(pd->channel.data);
	}

	ret = pd->channel.send(pd->channel.data, pd->rx_buf, pd->rx_buf_len);
	if (ret != pd->rx_buf_len) {
		LOG_ERR("Channel send for %d bytes failed! ret: %d",
			pd->rx_buf_len, ret);
		return OSDP_CP_ERR_GENERIC;
	}

	if (IS_ENABLED(CONFIG_OSDP_PACKET_TRACE)) {
		if (pd->cmd_id != CMD_POLL) {
			LOG_DBG("bytes sent");
			osdp_dump(NULL, pd->rx_buf, pd->rx_buf_len);
		}
	}

	return OSDP_CP_ERR_NONE;
}

static int cp_process_reply(struct osdp_pd *pd)
{
	uint8_t *buf;
	int err, len, one_pkt_len, remaining;

	buf = pd->rx_buf + pd->rx_buf_len;
	remaining = sizeof(pd->rx_buf) - pd->rx_buf_len;

	len = pd->channel.recv(pd->channel.data, buf, remaining);
	if (len <= 0) { /* No data received */
		return OSDP_CP_ERR_NO_DATA;
	}
	pd->rx_buf_len += len;

	if (IS_ENABLED(CONFIG_OSDP_PACKET_TRACE)) {
		if (pd->cmd_id != CMD_POLL) {
			LOG_DBG("bytes received");
			osdp_dump(NULL, pd->rx_buf, pd->rx_buf_len);
		}
	}

	err = osdp_phy_check_packet(pd, pd->rx_buf, pd->rx_buf_len, &len);

	/* Translate phy error codes to CP errors */
	switch (err) {
	case OSDP_ERR_PKT_NONE:
		break;
	case OSDP_ERR_PKT_WAIT:
		return OSDP_CP_ERR_NO_DATA;
	case OSDP_ERR_PKT_BUSY:
		return OSDP_CP_ERR_RETRY_CMD;
	default:
		return OSDP_CP_ERR_GENERIC;
	}

	one_pkt_len = len;

	/* Valid OSDP packet in buffer */
	len = osdp_phy_decode_packet(pd, pd->rx_buf, len, &buf);
	if (len <= 0) {
		return OSDP_CP_ERR_GENERIC;
	}
	err = cp_decode_response(pd, buf, len);

	/* We are done with the packet (error or not). Remove processed bytes */
	len = pd->rx_buf_len - one_pkt_len;
	if (len) {
		memmove(pd->rx_buf, pd->rx_buf + one_pkt_len, len);
	}
	pd->rx_buf_len = len;

	return err;
}

static void cp_flush_command_queue(struct osdp_pd *pd)
{
	struct osdp_cmd *cmd;

	while (cp_cmd_dequeue(pd, &cmd) == 0) {
		cp_cmd_free(pd, cmd);
	}
}

static inline void cp_set_state(struct osdp_pd *pd, enum osdp_cp_state_e state)
{
	pd->state = state;
	CLEAR_FLAG(pd, PD_FLAG_AWAIT_RESP);
}

static inline void cp_set_online(struct osdp_pd *pd)
{
	cp_set_state(pd, OSDP_CP_STATE_ONLINE);
	pd->wait_ms = 0;
	pd->tstamp = 0;
}

static inline void cp_set_offline(struct osdp_pd *pd)
{
	sc_deactivate(pd);
	pd->state = OSDP_CP_STATE_OFFLINE;
	pd->tstamp = osdp_millis_now();
	if (pd->wait_ms == 0) {
		pd->wait_ms = 1000; /* retry after 1 second initially */
	} else {
		pd->wait_ms <<= 1;
		if (pd->wait_ms > OSDP_ONLINE_RETRY_WAIT_MAX_MS) {
			pd->wait_ms = OSDP_ONLINE_RETRY_WAIT_MAX_MS;
		}
	}
}

#ifdef CONFIG_OSDP_SC_ENABLED
static inline bool cp_sc_should_retry(struct osdp_pd *pd)
{
	return (sc_is_capable(pd) && !sc_is_active(pd) &&
		osdp_millis_since(pd->sc_tstamp) > OSDP_PD_SC_RETRY_MS);
}
#endif

static int cp_phy_state_update(struct osdp_pd *pd)
{
	int64_t elapsed;
	int rc, ret = OSDP_CP_ERR_CAN_YIELD;
	struct osdp_cmd *cmd = NULL;

	switch (pd->phy_state) {
	case OSDP_CP_PHY_STATE_WAIT:
		elapsed = osdp_millis_since(pd->phy_tstamp);
		if (elapsed < OSDP_CMD_RETRY_WAIT_MS) {
			break;
		}
		pd->phy_state = OSDP_CP_PHY_STATE_SEND_CMD;
		break;
	case OSDP_CP_PHY_STATE_ERR:
		ret = OSDP_CP_ERR_GENERIC;
		break;
	case OSDP_CP_PHY_STATE_IDLE:
		if (cp_cmd_dequeue(pd, &cmd)) {
			ret = OSDP_CP_ERR_NONE; /* command queue is empty */
			break;
		}
		pd->cmd_id = cmd->id;
		memcpy(pd->ephemeral_data, cmd, sizeof(struct osdp_cmd));
		cp_cmd_free(pd, cmd);
		/* fall-thru */
	case OSDP_CP_PHY_STATE_SEND_CMD:
		if (cp_build_packet(pd)) {
			LOG_ERR("Failed to build packet for CMD(%d)",
				pd->cmd_id);
			ret = OSDP_CP_ERR_GENERIC;
			break;
		}
		if (cp_send_command(pd)) {
			LOG_ERR("Failed to send CMD(%d)", pd->cmd_id);
			pd->phy_state = OSDP_CP_PHY_STATE_ERR;
			ret = OSDP_CP_ERR_GENERIC;
			break;
		}
		ret = OSDP_CP_ERR_INPROG;
		pd->phy_state = OSDP_CP_PHY_STATE_REPLY_WAIT;
		pd->rx_buf_len = 0; /* reset buf_len for next use */
		pd->phy_tstamp = osdp_millis_now();
		break;
	case OSDP_CP_PHY_STATE_REPLY_WAIT:
		rc = cp_process_reply(pd);
		if (rc == OSDP_CP_ERR_NONE) {
#ifdef CONFIG_OSDP_SC_ENABLED
			if (sc_is_active(pd)) {
				pd->sc_tstamp = osdp_millis_now();
			}
#endif
			pd->phy_state = OSDP_CP_PHY_STATE_IDLE;
			break;
		}
		if (rc == OSDP_CP_ERR_RETRY_CMD) {
			LOG_INF("PD busy; retry last command");
			pd->phy_tstamp = osdp_millis_now();
			pd->phy_state = OSDP_CP_PHY_STATE_WAIT;
			break;
		}
		if (rc == OSDP_CP_ERR_GENERIC || rc == OSDP_CP_ERR_UNKNOWN ||
		    osdp_millis_since(pd->phy_tstamp) > OSDP_RESP_TOUT_MS) {
			if (rc != OSDP_CP_ERR_GENERIC) {
				LOG_ERR("Response timeout for CMD(%02x)",
					pd->cmd_id);
			}
			pd->rx_buf_len = 0;
			if (pd->channel.flush) {
				pd->channel.flush(pd->channel.data);
			}
			cp_flush_command_queue(pd);
			pd->phy_state = OSDP_CP_PHY_STATE_ERR;
			ret = OSDP_CP_ERR_GENERIC;
			break;
		}
		ret = OSDP_CP_ERR_INPROG;
		break;
	}

	return ret;
}

static int cp_cmd_dispatcher(struct osdp_pd *pd, int cmd)
{
	struct osdp_cmd *c;

	if (ISSET_FLAG(pd, PD_FLAG_AWAIT_RESP)) {
		CLEAR_FLAG(pd, PD_FLAG_AWAIT_RESP);
		return OSDP_CP_ERR_NONE; /* nothing to be done here */
	}

	c = cp_cmd_alloc(pd);
	if (c == NULL) {
		return OSDP_CP_ERR_GENERIC;
	}

	c->id = cmd;

	if (c->id == CMD_KEYSET) {
		memcpy(&c->keyset, pd->ephemeral_data, sizeof(c->keyset));
	}

	cp_cmd_enqueue(pd, c);
	SET_FLAG(pd, PD_FLAG_AWAIT_RESP);
	return OSDP_CP_ERR_INPROG;
}

static int state_update(struct osdp_pd *pd)
{
	bool soft_fail;
	int phy_state;
#ifdef CONFIG_OSDP_SC_ENABLED
	struct osdp *ctx = pd_to_osdp(pd);
	struct osdp_cmd_keyset *keyset;
#endif

	phy_state = cp_phy_state_update(pd);
	if (phy_state == OSDP_CP_ERR_INPROG ||
	    phy_state == OSDP_CP_ERR_CAN_YIELD) {
		return phy_state;
	}

	/* Certain states can fail without causing PD offline */
	soft_fail = (pd->state == OSDP_CP_STATE_SC_CHLNG);

	/* phy state error -- cleanup */
	if (pd->state != OSDP_CP_STATE_OFFLINE &&
	    phy_state == OSDP_CP_ERR_GENERIC && !soft_fail) {
		cp_set_offline(pd);
		return OSDP_CP_ERR_CAN_YIELD;
	}

	/* command queue is empty and last command was successful */

	switch (pd->state) {
	case OSDP_CP_STATE_ONLINE:
#ifdef CONFIG_OSDP_SC_ENABLED
		if (cp_sc_should_retry(pd)) {
			LOG_INF("Retry SC after retry timeout");
			cp_set_state(pd, OSDP_CP_STATE_SC_INIT);
			break;
		}
#endif
		if (osdp_millis_since(pd->tstamp) < OSDP_PD_POLL_TIMEOUT_MS) {
			break;
		}
		if (cp_cmd_dispatcher(pd, CMD_POLL) == 0) {
			pd->tstamp = osdp_millis_now();
		}
		break;
	case OSDP_CP_STATE_OFFLINE:
		if (osdp_millis_since(pd->tstamp) > pd->wait_ms) {
			cp_set_state(pd, OSDP_CP_STATE_INIT);
			osdp_phy_state_reset(pd);
		}
		break;
	case OSDP_CP_STATE_INIT:
		if (cp_cmd_dispatcher(pd, CMD_POLL) != 0) {
			break;
		}
		cp_set_state(pd, OSDP_CP_STATE_IDREQ);
		__fallthrough;
	case OSDP_CP_STATE_IDREQ:
		if (cp_cmd_dispatcher(pd, CMD_ID) != 0) {
			break;
		}
		if (pd->reply_id != REPLY_PDID) {
			LOG_ERR("Unexpected REPLY(%02x) for cmd "
				STRINGIFY(CMD_ID), pd->reply_id);
			cp_set_offline(pd);
			break;
		}
		cp_set_state(pd, OSDP_CP_STATE_CAPDET);
		__fallthrough;
	case OSDP_CP_STATE_CAPDET:
		if (cp_cmd_dispatcher(pd, CMD_CAP) != 0) {
			break;
		}
		if (pd->reply_id != REPLY_PDCAP) {
			LOG_ERR("Unexpected REPLY(%02x) for cmd "
				STRINGIFY(CMD_CAP), pd->reply_id);
			cp_set_offline(pd);
			break;
		}
#ifdef CONFIG_OSDP_SC_ENABLED
		if (sc_is_capable(pd)) {
			CLEAR_FLAG(pd, PD_FLAG_SC_SCBKD_DONE);
			CLEAR_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
			cp_set_state(pd, OSDP_CP_STATE_SC_INIT);
			break;
		}
#endif /* CONFIG_OSDP_SC_ENABLED */
		cp_set_online(pd);
		break;
#ifdef CONFIG_OSDP_SC_ENABLED
	case OSDP_CP_STATE_SC_INIT:
		osdp_sc_setup(pd);
		cp_set_state(pd, OSDP_CP_STATE_SC_CHLNG);
		__fallthrough;
	case OSDP_CP_STATE_SC_CHLNG:
		if (cp_cmd_dispatcher(pd, CMD_CHLNG) != 0) {
			break;
		}
		if (phy_state < 0) {
			if (ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD)) {
				LOG_INF("SC Failed. Online without SC");
				pd->sc_tstamp = osdp_millis_now();
				osdp_phy_state_reset(pd);
				cp_set_online(pd);
				break;
			}
			SET_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
			cp_set_state(pd, OSDP_CP_STATE_SC_INIT);
			pd->phy_state = 0; /* soft reset phy state */
			LOG_WRN("SC Failed. Retry with SCBK-D");
			break;
		}
		if (pd->reply_id != REPLY_CCRYPT) {
			LOG_ERR("CHLNG failed. Online without SC");
			pd->sc_tstamp = osdp_millis_now();
			osdp_phy_state_reset(pd);
			cp_set_online(pd);
			break;
		}
		cp_set_state(pd, OSDP_CP_STATE_SC_SCRYPT);
		__fallthrough;
	case OSDP_CP_STATE_SC_SCRYPT:
		if (cp_cmd_dispatcher(pd, CMD_SCRYPT) != 0) {
			break;
		}
		if (pd->reply_id != REPLY_RMAC_I) {
			LOG_ERR("SCRYPT failed. Online without SC");
			osdp_phy_state_reset(pd);
			pd->sc_tstamp = osdp_millis_now();
			cp_set_online(pd);
			break;
		}
		if (ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD)) {
			LOG_WRN("SC ACtive with SCBK-D. Set SCBK");
			cp_set_state(pd, OSDP_CP_STATE_SET_SCBK);
			break;
		}
		LOG_INF("SC Active");
		pd->sc_tstamp = osdp_millis_now();
		cp_set_online(pd);
		break;
	case OSDP_CP_STATE_SET_SCBK:
		if (!ISSET_FLAG(pd, PD_FLAG_AWAIT_RESP)) {
			keyset = (struct osdp_cmd_keyset *)pd->ephemeral_data;
			if (ISSET_FLAG(pd, PD_FLAG_HAS_SCBK)) {
				memcpy(keyset->data, pd->sc.scbk, 16);
				keyset->type = 1;
			} else {
				keyset->type = 0;
				memcpy(keyset->data, ctx->sc_master_key, 16);
			}
			keyset->length = 16;
		}
		if (cp_cmd_dispatcher(pd, CMD_KEYSET) != 0) {
			break;
		}
		if (pd->reply_id == REPLY_NAK) {
			LOG_WRN("Failed to set SCBK; continue with SCBK-D");
			cp_set_online(pd);
			break;
		}
		cp_keyset_complete(pd);
		pd->seq_number = -1;
		break;
#endif /* CONFIG_OSDP_SC_ENABLED */
	default:
		break;
	}

	return OSDP_CP_ERR_CAN_YIELD;
}

#ifdef CONFIG_OSDP_SC_ENABLED
static int osdp_cp_send_command_keyset(struct osdp_cmd_keyset *p)
{
	int i, res = 0;
	struct osdp_cmd *cmd[OSDP_PD_MAX] = { 0 };
	struct osdp_pd *pd;
	struct osdp *ctx = osdp_get_ctx();

	for (i = 0; i < NUM_PD(ctx); i++) {
		pd = osdp_to_pd(ctx, i);
		if (!ISSET_FLAG(pd, PD_FLAG_SC_ACTIVE)) {
			LOG_WRN("master_key based key set can be performed only"
				" when all PDs are ONLINE, SC_ACTIVE");
			return -1;
		}
	}

	LOG_INF("master_key based key set is a global command; "
		"all connected PDs will be affected.");

	for (i = 0; i < NUM_PD(ctx); i++) {
		pd = osdp_to_pd(ctx, i);
		cmd[i] = cp_cmd_alloc(pd);
		if (cmd[i] == NULL) {
			res = -1;
			break;
		}
		cmd[i]->id = CMD_KEYSET;
		memcpy(&cmd[i]->keyset, p, sizeof(struct osdp_cmd_keyset));
	}

	for (i = 0; i < NUM_PD(ctx); i++) {
		pd = osdp_to_pd(ctx, i);
		if (res == 0) {
			cp_cmd_enqueue(pd, cmd[i]);
		} else if (cmd[i]) {
			cp_cmd_free(pd, cmd[i]);
		}
	}

	return res;
}

void cp_keyset_complete(struct osdp_pd *pd)
{
	struct osdp_cmd *c = (struct osdp_cmd *)pd->ephemeral_data;

	sc_deactivate(pd);
	CLEAR_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
	memcpy(pd->sc.scbk, c->keyset.data, 16);
	cp_set_state(pd, OSDP_CP_STATE_SC_INIT);
	LOG_INF("SCBK set; restarting SC to verify new SCBK");
}
#endif /* CONFIG_OSDP_SC_ENABLED */

void osdp_update(struct osdp *ctx)
{
	int i;

	for (i = 0; i < NUM_PD(ctx); i++) {
		SET_CURRENT_PD(ctx, i);
		state_update(GET_CURRENT_PD(ctx));
	}
}

int osdp_setup(struct osdp *ctx, uint8_t *key)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(key);

#ifdef CONFIG_OSDP_SC_ENABLED
	if (key == NULL) {
		LOG_ERR("Master key cannot be null");
		return -1;
	}
	memcpy(ctx->sc_master_key, key, 16);
#endif
	return 0;
}

/* --- Exported Methods --- */

void osdp_cp_set_event_callback(cp_event_callback_t cb, void *arg)
{
	struct osdp *ctx = osdp_get_ctx();

	ctx->event_callback = cb;
	ctx->event_callback_arg = arg;
}

int osdp_cp_send_command(int pd, struct osdp_cmd *cmd)
{
	struct osdp *ctx = osdp_get_ctx();
	struct osdp_cmd *p;
	int cmd_id;

	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_ERR("Invalid PD number");
		return -1;
	}
	if (osdp_to_pd(ctx, pd)->state != OSDP_CP_STATE_ONLINE) {
		LOG_WRN("PD not online");
		return -1;
	}

	switch (cmd->id) {
	case OSDP_CMD_OUTPUT:
		cmd_id = CMD_OUT;
		break;
	case OSDP_CMD_LED:
		cmd_id = CMD_LED;
		break;
	case OSDP_CMD_BUZZER:
		cmd_id = CMD_BUZ;
		break;
	case OSDP_CMD_TEXT:
		cmd_id = CMD_TEXT;
		break;
	case OSDP_CMD_COMSET:
		cmd_id = CMD_COMSET;
		break;
#ifdef CONFIG_OSDP_SC_ENABLED
	case OSDP_CMD_KEYSET:
		return osdp_cp_send_command_keyset(&cmd->keyset);
#endif
	default:
		LOG_ERR("Invalid CMD_ID:%d", cmd->id);
		return -1;
	}

	p = cp_cmd_alloc(osdp_to_pd(ctx, pd));
	if (p == NULL) {
		return -1;
	}
	memcpy(p, cmd, sizeof(struct osdp_cmd));
	p->id = cmd_id; /* translate to internal */
	cp_cmd_enqueue(osdp_to_pd(ctx, pd), p);
	return 0;
}
