/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <logging/log.h>
LOG_MODULE_DECLARE(osdp, CONFIG_OSDP_LOG_LEVEL);

#include "osdp_common.h"

#define TAG "CP: "

#define OSDP_PD_POLL_TIMEOUT_MS        (1000 / CONFIG_OSDP_PD_POLL_RATE)
#define OSDP_CMD_RETRY_WAIT_MS         (CONFIG_OSDP_CMD_RETRY_WAIT_SEC * 1000)

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

#define REPLY_ACK_DATA_LEN             0
#define REPLY_PDID_DATA_LEN            12
#define REPLY_PDCAP_ENTITY_LEN         3
#define REPLY_LSTATR_DATA_LEN          2
#define REPLY_RSTATR_DATA_LEN          1
#define REPLY_COM_DATA_LEN             5
#define REPLY_NAK_DATA_LEN             1
#define REPLY_KEYPPAD_DATA_LEN         2   /* variable length command */
#define REPLY_RAW_DATA_LEN             4   /* variable length command */
#define REPLY_FMT_DATA_LEN             3   /* variable length command */
#define REPLY_BUSY_DATA_LEN            0

#define OSDP_CP_ERR_GENERIC           -1
#define OSDP_CP_ERR_NO_DATA            1
#define OSDP_CP_ERR_RETRY_CMD          2
#define OSDP_CP_ERR_CAN_YIELD          3
#define OSDP_CP_ERR_INPROG             4

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

/**
 * Returns:
 * +ve: length of command
 * -ve: error
 */
static int cp_build_command(struct osdp_pd *pd, uint8_t *buf, int max_len)
{
	struct osdp_cmd *cmd = NULL;
	int data_off, i, ret = -1, len = 0;

	data_off = osdp_phy_packet_get_data_offset(pd, buf);

	buf += data_off;
	max_len -= data_off;
	if (max_len <= 0) {
		return OSDP_CP_ERR_GENERIC;
	}

	switch (pd->cmd_id) {
	case CMD_POLL:
		if (max_len < CMD_POLL_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		ret = 0;
		break;
	case CMD_LSTAT:
		if (max_len < CMD_LSTAT_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		ret = 0;
		break;
	case CMD_ISTAT:
		if (max_len < CMD_ISTAT_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		ret = 0;
		break;
	case CMD_OSTAT:
		if (max_len < CMD_OSTAT_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		ret = 0;
		break;
	case CMD_RSTAT:
		if (max_len < CMD_RSTAT_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		ret = 0;
		break;
	case CMD_ID:
		if (max_len < CMD_ID_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = 0x00;
		ret = 0;
		break;
	case CMD_CAP:
		if (max_len < CMD_CAP_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = 0x00;
		ret = 0;
		break;
	case CMD_DIAG:
		if (max_len < CMD_DIAG_LEN) {
			break;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = 0x00;
		ret = 0;
		break;
	case CMD_OUT:
		if (max_len < CMD_OUT_LEN) {
			break;
		}
		cmd = (struct osdp_cmd *)pd->cmd_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->output.output_no;
		buf[len++] = cmd->output.control_code;
		buf[len++] = BYTE_0(cmd->output.timer_count);
		buf[len++] = BYTE_1(cmd->output.timer_count);
		ret = 0;
		break;
	case CMD_LED:
		if (max_len < CMD_LED_LEN) {
			break;
		}
		cmd = (struct osdp_cmd *)pd->cmd_data;
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
		ret = 0;
		break;
	case CMD_BUZ:
		if (max_len < CMD_BUZ_LEN) {
			break;
		}
		cmd = (struct osdp_cmd *)pd->cmd_data;
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->buzzer.reader;
		buf[len++] = cmd->buzzer.control_code;
		buf[len++] = cmd->buzzer.on_count;
		buf[len++] = cmd->buzzer.off_count;
		buf[len++] = cmd->buzzer.rep_count;
		ret = 0;
		break;
	case CMD_TEXT:
		cmd = (struct osdp_cmd *)pd->cmd_data;
		if (max_len < (CMD_TEXT_LEN + cmd->text.length)) {
			break;
		}
		buf[len++] = pd->cmd_id;
		buf[len++] = cmd->text.reader;
		buf[len++] = cmd->text.control_code;
		buf[len++] = cmd->text.temp_time;
		buf[len++] = cmd->text.offset_row;
		buf[len++] = cmd->text.offset_col;
		buf[len++] = cmd->text.length;
		for (i = 0; i < cmd->text.length; i++) {
			buf[len++] = cmd->text.data[i];
		}
		ret = 0;
		break;
	default:
		LOG_ERR(TAG "Unknown/Unsupported command %02x", pd->cmd_id);
		return OSDP_CP_ERR_GENERIC;
	}

	if (ret < 0) {
		LOG_ERR(TAG "Unable to build command %02x", pd->cmd_id);
		return OSDP_CP_ERR_GENERIC;
	}

	return len;
}

static int cp_decode_response(struct osdp_pd *pd, uint8_t *buf, int len)
{
	uint32_t temp32;
	struct osdp_cp *cp = TO_CTX(pd)->cp;
	int i, ret = OSDP_CP_ERR_GENERIC, pos = 0, t1, t2;

	if (len < 1) {
		LOG_ERR("response must have at least one byte");
		return OSDP_CP_ERR_GENERIC;
	}

	pd->reply_id = buf[pos++];
	len--;		/* consume reply id from the head */

	switch (pd->reply_id) {
	case REPLY_ACK:
		if (len != REPLY_ACK_DATA_LEN) {
			break;
		}
		ret = 0;
		break;
	case REPLY_NAK:
		if (len != REPLY_NAK_DATA_LEN) {
			break;
		}
		LOG_ERR(TAG "PD replied with NAK code %d", buf[pos]);
		ret = 0;
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

		pd->id.serial_number  = buf[pos++];
		pd->id.serial_number |= buf[pos++] << 8;
		pd->id.serial_number |= buf[pos++] << 16;
		pd->id.serial_number |= buf[pos++] << 24;

		pd->id.firmware_version  = buf[pos++] << 16;
		pd->id.firmware_version |= buf[pos++] << 8;
		pd->id.firmware_version |= buf[pos++];
		ret = 0;
		break;
	case REPLY_PDCAP:
		if ((len % REPLY_PDCAP_ENTITY_LEN) != 0) {
			break;
		}
		while (pos < len) {
			t1 = buf[pos++]; /* func_code */
			if (t1 > OSDP_PD_CAP_SENTINEL) {
				break;
			}
			pd->cap[t1].function_code    = t1;
			pd->cap[t1].compliance_level = buf[pos++];
			pd->cap[t1].num_items        = buf[pos++];
		}
		/* post-capabilities hooks */
		t2 = OSDP_PD_CAP_COMMUNICATION_SECURITY;
		if (pd->cap[t2].compliance_level & 0x01)
			SET_FLAG(pd, PD_FLAG_SC_CAPABLE);
		else
			CLEAR_FLAG(pd, PD_FLAG_SC_CAPABLE);
		ret = 0;
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
		ret = 0;
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
		ret = 0;
		break;
	case REPLY_COM:
		if (len != REPLY_COM_DATA_LEN) {
			break;
		}
		t1 = buf[pos++];
		temp32  = buf[pos++];
		temp32 |= buf[pos++] << 8;
		temp32 |= buf[pos++] << 16;
		temp32 |= buf[pos++] << 24;
		LOG_WRN(TAG "COMSET responded with ID:%d baud:%d", t1, temp32);
		pd->address = t1;
		pd->baud_rate = temp32;
		ret = 0;
		break;
	case REPLY_KEYPPAD:
		if (len < REPLY_KEYPPAD_DATA_LEN) {
			break;
		}
		pos++;	         /* reader number; skip */
		t1 = buf[pos++]; /* key length */
		if ((len - REPLY_KEYPPAD_DATA_LEN) != t1) {
			break;
		}
		if (cp->notifier.keypress) {
			for (i = 0; i < t1; i++) {
				t2 = buf[pos + i]; /* key data */
				cp->notifier.keypress(pd->offset, t2);
			}
		}
		ret = 0;
		break;
	case REPLY_RAW:
		if (len < REPLY_RAW_DATA_LEN) {
			break;
		}
		pos++;	                /* reader number; skip */
		t1 = buf[pos++];        /* format */
		t2 = buf[pos++];        /* length LSB */
		t2 |= buf[pos++] << 8; /* length MSB */
		if ((len - REPLY_RAW_DATA_LEN) != t2) {
			break;
		}
		if (cp->notifier.cardread) {
			cp->notifier.cardread(pd->offset, t1, buf + pos, t2);
		}
		ret = 0;
		break;
	case REPLY_FMT:
		if (len < REPLY_FMT_DATA_LEN) {
			break;
		}
		pos++;	/* reader number; skip */
		pos++;	/* skip one byte -- TODO: handle reader direction */
		t1 = buf[pos++]; /* Key length */
		if ((len - REPLY_FMT_DATA_LEN) != t1) {
			break;
		}
		if (cp->notifier.cardread) {
			cp->notifier.cardread(pd->offset, OSDP_CARD_FMT_ASCII,
					      buf + pos, t1);
		}
		ret = 0;
		break;
	case REPLY_BUSY:
		/* PD busy; signal upper layer to retry command */
		if (len != REPLY_BUSY_DATA_LEN) {
			break;
		}
		ret = OSDP_CP_ERR_RETRY_CMD;
		break;
	default:
		LOG_DBG(TAG "unexpected reply: 0x%02x", pd->reply_id);
		return OSDP_CP_ERR_GENERIC;
	}

	if (ret == OSDP_CP_ERR_GENERIC) {
		LOG_ERR(TAG "REPLY %02x for CMD %02x format error!",
			pd->cmd_id, pd->reply_id);
		return OSDP_CP_ERR_GENERIC;
	}

	if (pd->cmd_id != CMD_POLL) {
		LOG_DBG(TAG "CMD: %02x REPLY: %02x", pd->cmd_id, pd->reply_id);
	}

	return ret;
}

static int cp_send_command(struct osdp_pd *pd)
{
	int ret, len;

	/* init packet buf with header */
	len = osdp_phy_packet_init(pd, pd->rx_buf, sizeof(pd->rx_buf));
	if (len < 0) {
		return -1;
	}

	/* fill command data */
	ret = cp_build_command(pd, pd->rx_buf, sizeof(pd->rx_buf));
	if (ret < 0) {
		return -1;
	}
	len += ret;

	/* finalize packet */
	len = osdp_phy_packet_finalize(pd, pd->rx_buf, len, sizeof(pd->rx_buf));
	if (len < 0) {
		return -1;
	}

	ret = pd->channel.send(pd->channel.data, pd->rx_buf, len);

	if (IS_ENABLED(CONFIG_OSDP_PACKET_TRACE)) {
		if (pd->cmd_id != CMD_POLL) {
			LOG_DBG(TAG "bytes sent");
			osdp_dump(NULL, pd->rx_buf, len);
		}
	}

	return (ret == len) ? 0 : -1;
}

static int cp_process_reply(struct osdp_pd *pd)
{
	uint8_t *buf;
	int rec_bytes, ret, max_len;

	buf = pd->rx_buf + pd->rx_buf_len;
	max_len = sizeof(pd->rx_buf) - pd->rx_buf_len;

	rec_bytes = pd->channel.recv(pd->channel.data, buf, max_len);
	if (rec_bytes <= 0) {	/* No data received */
		return OSDP_CP_ERR_NO_DATA;
	}
	pd->rx_buf_len += rec_bytes;

	if (IS_ENABLED(CONFIG_OSDP_PACKET_TRACE)) {
		if (pd->cmd_id != CMD_POLL) {
			LOG_DBG(TAG "bytes received");
			osdp_dump(NULL, pd->rx_buf, pd->rx_buf_len);
		}
	}

	/* Valid OSDP packet in buffer */
	ret = osdp_phy_decode_packet(pd, pd->rx_buf, pd->rx_buf_len);
	if (ret == OSDP_ERR_PKT_FMT) {
		return -1; /* fatal errors */
	} else if (ret == OSDP_ERR_PKT_WAIT) {
		/* rx_buf_len != pkt->len; wait for more data */
		return OSDP_CP_ERR_NO_DATA;
	} else if (ret == OSDP_ERR_PKT_SKIP) {
		/* soft fail - discard this message */
		pd->rx_buf_len = 0;
		if (pd->channel.flush) {
			pd->channel.flush(pd->channel.data);
		}
		return OSDP_CP_ERR_NO_DATA;
	}
	pd->rx_buf_len = ret;

	return cp_decode_response(pd, pd->rx_buf, pd->rx_buf_len);
}

static void cp_flush_command_queue(struct osdp_pd *pd)
{
	struct osdp_cmd *cmd;

	while (osdp_cmd_dequeue(pd, &cmd) == 0) {
		osdp_cmd_free(pd, cmd);
	}
}

static inline void cp_set_offline(struct osdp_pd *pd)
{
	pd->state = OSDP_CP_STATE_OFFLINE;
	pd->tstamp = osdp_millis_now();
}

static inline void cp_reset_state(struct osdp_pd *pd)
{
	pd->state = OSDP_CP_STATE_INIT;
	osdp_phy_state_reset(pd);
	pd->flags = 0;
}

static inline void cp_set_state(struct osdp_pd *pd, enum osdp_cp_state_e state)
{
	pd->state = state;
	CLEAR_FLAG(pd, PD_FLAG_AWAIT_RESP);
}

/**
 * Note: This method must not dequeue cmd unless it reaches an invalid state.
 */
static int cp_phy_state_update(struct osdp_pd *pd)
{
	int ret = OSDP_CP_ERR_INPROG, tmp;
	struct osdp_cmd *cmd = NULL;

	switch (pd->phy_state) {
	case OSDP_CP_PHY_STATE_ERR_WAIT:
		ret = OSDP_CP_ERR_GENERIC;
		break;
	case OSDP_CP_PHY_STATE_IDLE:
		if (osdp_cmd_dequeue(pd, &cmd)) {
			ret = 0;
			break;
		}
		pd->cmd_id = cmd->id;
		memcpy(pd->cmd_data, cmd, sizeof(struct osdp_cmd));
		osdp_cmd_free(pd, cmd);
		/* fall-thru */
	case OSDP_CP_PHY_STATE_SEND_CMD:
		if ((cp_send_command(pd)) < 0) {
			LOG_ERR(TAG "send command error");
			pd->phy_state = OSDP_CP_PHY_STATE_ERR;
			ret = OSDP_CP_ERR_GENERIC;
			break;
		}
		pd->phy_state = OSDP_CP_PHY_STATE_REPLY_WAIT;
		pd->rx_buf_len = 0; /* reset buf_len for next use */
		pd->phy_tstamp = osdp_millis_now();
		break;
	case OSDP_CP_PHY_STATE_REPLY_WAIT:
		tmp = cp_process_reply(pd);
		if (tmp == 0) { /* success */
			pd->phy_state = OSDP_CP_PHY_STATE_CLEANUP;
			break;
		}
		if (tmp == OSDP_CP_ERR_RETRY_CMD) {
			LOG_INF(TAG "PD busy; retry last command");
			pd->phy_tstamp = osdp_millis_now();
			pd->phy_state = OSDP_CP_PHY_STATE_WAIT;
			ret = 2;
			break;
		}
		if (tmp == OSDP_CP_ERR_GENERIC) {
			pd->phy_state = OSDP_CP_PHY_STATE_ERR;
			break;
		}
		if (osdp_millis_since(pd->phy_tstamp) > OSDP_RESP_TOUT_MS) {
			LOG_ERR(TAG "CMD: %02x - response timeout", pd->cmd_id);
			pd->phy_state = OSDP_CP_PHY_STATE_ERR;
		}
		break;
	case OSDP_CP_PHY_STATE_WAIT:
		if (osdp_millis_since(pd->phy_tstamp) < OSDP_CMD_RETRY_WAIT_MS) {
			break;
		}
		pd->phy_state = OSDP_CP_PHY_STATE_IDLE;
		break;
	case OSDP_CP_PHY_STATE_ERR:
		pd->rx_buf_len = 0;
		if (pd->channel.flush) {
			pd->channel.flush(pd->channel.data);
		}
		cp_flush_command_queue(pd);
		pd->phy_state = OSDP_CP_PHY_STATE_ERR_WAIT;
		ret = OSDP_CP_ERR_GENERIC;
		break;
	case OSDP_CP_PHY_STATE_CLEANUP:
		pd->phy_state = OSDP_CP_PHY_STATE_IDLE;
		ret = OSDP_CP_ERR_CAN_YIELD; /* in between commands */
		break;
	}

	return ret;
}

/**
 * Returns:
 *   0: nothing done
 *   1: dispatched
 *  -1: error
 */
static int cp_cmd_dispatcher(struct osdp_pd *pd, int cmd)
{
	struct osdp_cmd *c;

	if (ISSET_FLAG(pd, PD_FLAG_AWAIT_RESP)) {
		CLEAR_FLAG(pd, PD_FLAG_AWAIT_RESP);
		return 0;
	}

	c = osdp_cmd_alloc(pd);
	if (c == NULL) {
		return OSDP_CP_ERR_GENERIC;
	}

	c->id = cmd;
	osdp_cmd_enqueue(pd, c);
	SET_FLAG(pd, PD_FLAG_AWAIT_RESP);
	return OSDP_CP_ERR_INPROG;
}

static int state_update(struct osdp_pd *pd)
{
	int phy_state, soft_fail;

	phy_state = cp_phy_state_update(pd);
	if (phy_state == OSDP_CP_ERR_INPROG ||
	    phy_state == OSDP_CP_ERR_CAN_YIELD) {
		return OSDP_CP_ERR_GENERIC;
	}

	/* Certain states can fail without causing PD offline */
	soft_fail = (pd->state == OSDP_CP_STATE_SC_CHLNG);

	/* phy state error -- cleanup */
	if (pd->state != OSDP_CP_STATE_OFFLINE &&
	    phy_state == OSDP_CP_ERR_GENERIC && soft_fail == 0) {
		cp_set_offline(pd);
	}

	/* command queue is empty and last command was successful */

	switch (pd->state) {
	case OSDP_CP_STATE_ONLINE:
		if (osdp_millis_since(pd->tstamp) < OSDP_PD_POLL_TIMEOUT_MS) {
			break;
		}
		if (cp_cmd_dispatcher(pd, CMD_POLL) == 0) {
			pd->tstamp = osdp_millis_now();
		}
		break;
	case OSDP_CP_STATE_OFFLINE:
		if (osdp_millis_since(pd->tstamp) > OSDP_CMD_RETRY_WAIT_MS) {
			cp_reset_state(pd);
		}
		break;
	case OSDP_CP_STATE_INIT:
		cp_set_state(pd, OSDP_CP_STATE_IDREQ);
		/* FALLTHRU */
	case OSDP_CP_STATE_IDREQ:
		if (cp_cmd_dispatcher(pd, CMD_ID) != 0) {
			break;
		}
		if (pd->reply_id != REPLY_PDID) {
			cp_set_offline(pd);
		}
		cp_set_state(pd, OSDP_CP_STATE_CAPDET);
		/* FALLTHRU */
	case OSDP_CP_STATE_CAPDET:
		if (cp_cmd_dispatcher(pd, CMD_CAP) != 0) {
			break;
		}
		if (pd->reply_id != REPLY_PDCAP) {
			cp_set_offline(pd);
		}
		cp_set_state(pd, OSDP_CP_STATE_ONLINE);
		break;
	default:
		break;
	}

	return 0;
}

void osdp_update(struct osdp *ctx)
{
	int i;

	for (i = 0; i < NUM_PD(ctx); i++) {
		SET_CURRENT_PD(ctx, i);
		state_update(GET_CURRENT_PD(ctx));
	}
}

int osdp_setup(struct osdp *ctx)
{
	ARG_UNUSED(ctx);
	return 0;
}

int osdp_cp_set_callback_key_press(int (*cb)(int address, uint8_t key))
{
	struct osdp *ctx = osdp_get_ctx();

	ctx->cp->notifier.keypress = cb;

	return 0;
}

int osdp_cp_set_callback_card_read(
	int (*cb)(int address, int format, uint8_t *data, int len))
{
	struct osdp *ctx = osdp_get_ctx();

	TO_CP(ctx)->notifier.cardread = cb;

	return 0;
}

int osdp_cp_send_cmd_output(int pd, struct osdp_cmd_output *p)
{
	struct osdp *ctx = osdp_get_ctx();
	struct osdp_cmd *cmd;

	if (TO_PD(ctx, pd)->state != OSDP_CP_STATE_ONLINE) {
		LOG_WRN(TAG "PD not online");
		return -1;
	}
	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_ERR(TAG "Invalid PD number");
		return -1;
	}

	cmd = osdp_cmd_alloc(TO_PD(ctx, pd));
	if (cmd == NULL) {
		return -1;
	}

	cmd->id = CMD_OUT;
	memcpy(&cmd->output, p, sizeof(struct osdp_cmd_output));
	osdp_cmd_enqueue(TO_PD(ctx, pd), cmd);
	return 0;
}

int osdp_cp_send_cmd_led(int pd, struct osdp_cmd_led *p)
{
	struct osdp *ctx = osdp_get_ctx();
	struct osdp_cmd *cmd;

	if (TO_PD(ctx, pd)->state != OSDP_CP_STATE_ONLINE) {
		LOG_WRN(TAG "PD not online");
		return -1;
	}
	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_ERR(TAG "Invalid PD number");
		return -1;
	}

	cmd = osdp_cmd_alloc(TO_PD(ctx, pd));
	if (cmd == NULL) {
		return -1;
	}

	cmd->id = CMD_LED;
	memcpy(&cmd->led, p, sizeof(struct osdp_cmd_led));
	osdp_cmd_enqueue(TO_PD(ctx, pd), cmd);
	return 0;
}

int osdp_cp_send_cmd_buzzer(int pd, struct osdp_cmd_buzzer *p)
{
	struct osdp *ctx = osdp_get_ctx();
	struct osdp_cmd *cmd;

	if (TO_PD(ctx, pd)->state != OSDP_CP_STATE_ONLINE) {
		LOG_WRN(TAG "PD not online");
		return -1;
	}
	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_ERR(TAG "Invalid PD number");
		return -1;
	}

	cmd = osdp_cmd_alloc(TO_PD(ctx, pd));
	if (cmd == NULL) {
		return -1;
	}

	cmd->id = CMD_BUZ;
	memcpy(&cmd->buzzer, p, sizeof(struct osdp_cmd_buzzer));
	osdp_cmd_enqueue(TO_PD(ctx, pd), cmd);
	return 0;
}

int osdp_cp_send_cmd_text(int pd, struct osdp_cmd_text *p)
{
	struct osdp *ctx = osdp_get_ctx();
	struct osdp_cmd *cmd;

	if (TO_PD(ctx, pd)->state != OSDP_CP_STATE_ONLINE) {
		LOG_WRN(TAG "PD not online");
		return -1;
	}
	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_ERR(TAG "Invalid PD number");
		return -1;
	}

	cmd = osdp_cmd_alloc(TO_PD(ctx, pd));
	if (cmd == NULL) {
		return -1;
	}

	cmd->id = CMD_TEXT;
	memcpy(&cmd->text, p, sizeof(struct osdp_cmd_text));
	osdp_cmd_enqueue(TO_PD(ctx, pd), cmd);
	return 0;
}

int osdp_cp_send_cmd_comset(int pd, struct osdp_cmd_comset *p)
{
	struct osdp *ctx = osdp_get_ctx();
	struct osdp_cmd *cmd;

	if (TO_PD(ctx, pd)->state != OSDP_CP_STATE_ONLINE) {
		LOG_WRN(TAG "PD not online");
		return -1;
	}
	if (pd < 0 || pd >= NUM_PD(ctx)) {
		LOG_ERR(TAG "Invalid PD number");
		return -1;
	}

	cmd = osdp_cmd_alloc(TO_PD(ctx, pd));
	if (cmd == NULL) {
		return -1;
	}

	cmd->id = CMD_COMSET;
	memcpy(&cmd->text, p, sizeof(struct osdp_cmd_comset));
	osdp_cmd_enqueue(TO_PD(ctx, pd), cmd);
	return 0;
}
