/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(osdp, CONFIG_OSDP_LOG_LEVEL);

#include <stdlib.h>
#include <string.h>

#include "osdp_common.h"

#define CMD_POLL_DATA_LEN              0
#define CMD_LSTAT_DATA_LEN             0
#define CMD_ISTAT_DATA_LEN             0
#define CMD_OSTAT_DATA_LEN             0
#define CMD_RSTAT_DATA_LEN             0
#define CMD_ID_DATA_LEN                1
#define CMD_CAP_DATA_LEN               1
#define CMD_OUT_DATA_LEN               4
#define CMD_LED_DATA_LEN               14
#define CMD_BUZ_DATA_LEN               5
#define CMD_TEXT_DATA_LEN              6   /* variable length command */
#define CMD_COMSET_DATA_LEN            5
#define CMD_KEYSET_DATA_LEN            18
#define CMD_CHLNG_DATA_LEN             8
#define CMD_SCRYPT_DATA_LEN            16

#define REPLY_ACK_LEN                  1
#define REPLY_PDID_LEN                 13
#define REPLY_PDCAP_LEN                1   /* variable length command */
#define REPLY_PDCAP_ENTITY_LEN         3
#define REPLY_LSTATR_LEN               3
#define REPLY_RSTATR_LEN               2
#define REPLY_COM_LEN                  6
#define REPLY_NAK_LEN                  2
#define REPLY_CCRYPT_LEN               33
#define REPLY_RMAC_I_LEN               17
#define REPLY_KEYPAD_LEN               2
#define REPLY_RAW_LEN                  4
#define REPLY_FMT_LEN                  3

enum osdp_pd_error_e {
	OSDP_PD_ERR_NONE = 0,
	OSDP_PD_ERR_NO_DATA = -1,
	OSDP_PD_ERR_GENERIC = -2,
	OSDP_PD_ERR_REPLY = -3,
	OSDP_PD_ERR_IGNORE = -4,
};

static struct osdp_pd_id osdp_pd_id = {
	.version = CONFIG_OSDP_PD_ID_VERSION,
	.model = CONFIG_OSDP_PD_ID_MODEL,
	.vendor_code = CONFIG_OSDP_PD_ID_VENDOR_CODE,
	.serial_number = CONFIG_OSDP_PD_ID_SERIAL_NUMBER,
	.firmware_version = CONFIG_OSDP_PD_ID_FIRMWARE_VERSION,
};

static struct osdp_pd_cap osdp_pd_cap[] = {
	/* Driver Implicit capabilities */
	{
		OSDP_PD_CAP_CHECK_CHARACTER_SUPPORT,
		1, /* The PD supports the 16-bit CRC-16 mode */
		0, /* N/A */
	},
	{
		OSDP_PD_CAP_COMMUNICATION_SECURITY,
#ifdef CONFIG_OSDP_SC_ENABLED
		1, /* (Bit-0) AES128 support */
		1, /* (Bit-0) default AES128 key */
#else
		0, /* SC not supported */
		0, /* SC not supported */
#endif
	},
	/* Configured from Kconfig */
	{
		OSDP_PD_CAP_CONTACT_STATUS_MONITORING,
		CONFIG_OSDP_PD_CAP_CONTACT_STATUS_MONITORING_COMP_LEVEL,
		CONFIG_OSDP_PD_CAP_CONTACT_STATUS_MONITORING_NUM_ITEMS,
	},
	{
		OSDP_PD_CAP_OUTPUT_CONTROL,
		CONFIG_OSDP_PD_CAP_OUTPUT_CONTROL_COMP_LEVEL,
		CONFIG_OSDP_PD_CAP_OUTPUT_CONTROL_NUM_ITEMS,
	},
	{
		OSDP_PD_CAP_READER_LED_CONTROL,
		CONFIG_OSDP_PD_CAP_READER_LED_CONTROL_COMP_LEVEL,
		CONFIG_OSDP_PD_CAP_READER_LED_CONTROL_NUM_ITEMS,
	},
	{
		OSDP_PD_CAP_READER_AUDIBLE_OUTPUT,
		CONFIG_OSDP_PD_CAP_READER_AUDIBLE_OUTPUT_COMP_LEVEL,
		CONFIG_OSDP_PD_CAP_READER_AUDIBLE_OUTPUT_NUM_ITEMS,
	},
	{
		OSDP_PD_CAP_READER_TEXT_OUTPUT,
		CONFIG_OSDP_PD_CAP_READER_TEXT_OUTPUT_COMP_LEVEL,
		CONFIG_OSDP_PD_CAP_READER_TEXT_OUTPUT_NUM_ITEMS,
	},
	{
		OSDP_PD_CAP_CARD_DATA_FORMAT,
		CONFIG_OSDP_PD_CAP_CARD_DATA_FORMAT_COMP_LEVEL,
		0, /* N/A set to 0 */
	},
	{
		OSDP_PD_CAP_TIME_KEEPING,
		CONFIG_OSDP_PD_CAP_TIME_KEEPING_COMP_LEVEL,
		0, /* N/A set to 0 */
	},
	{ -1, 0, 0 } /* Sentinel */
};

static struct osdp_event *pd_event_alloc(struct osdp_pd *pd)
{
	struct osdp_event *event = NULL;

	if (k_mem_slab_alloc(&pd->event.slab, (void **)&event, K_MSEC(100))) {
		LOG_ERR("Memory allocation time-out");
		return NULL;
	}
	return event;
}

static void pd_event_free(struct osdp_pd *pd, struct osdp_event *event)
{
	k_mem_slab_free(&pd->event.slab, (void *)event);
}

static void pd_event_enqueue(struct osdp_pd *pd, struct osdp_event *event)
{
	sys_slist_append(&pd->event.queue, &event->node);
}

static int pd_event_dequeue(struct osdp_pd *pd, struct osdp_event **event)
{
	sys_snode_t *node;

	node = sys_slist_peek_head(&pd->event.queue);
	if (node == NULL) {
		return -1;
	}
	sys_slist_remove(&pd->event.queue, NULL, node);
	*event = CONTAINER_OF(node, struct osdp_event, node);
	return 0;
}

static int pd_translate_event(struct osdp_pd *pd, struct osdp_event *event)
{
	int reply_code = 0;

	switch (event->type) {
	case OSDP_EVENT_CARDREAD:
		if (event->cardread.format == OSDP_CARD_FMT_RAW_UNSPECIFIED ||
		    event->cardread.format == OSDP_CARD_FMT_RAW_WIEGAND) {
			reply_code = REPLY_RAW;
		} else if (event->cardread.format == OSDP_CARD_FMT_ASCII) {
			reply_code = REPLY_FMT;
		} else {
			LOG_ERR("Event: cardread; Error: unknown format");
			break;
		}
		break;
	case OSDP_EVENT_KEYPRESS:
		reply_code = REPLY_KEYPPAD;
		break;
	default:
		LOG_ERR("Unknown event type %d", event->type);
		break;
	}
	if (reply_code == 0) {
		/* POLL command cannot fail even when there are errors here */
		return REPLY_ACK;
	}
	memcpy(pd->ephemeral_data, event, sizeof(struct osdp_event));
	return reply_code;
}

static bool do_command_callback(struct osdp_pd *pd, struct osdp_cmd *cmd)
{
	int ret;

	ret = pd->command_callback(pd->command_callback_arg, cmd);
	if (ret != 0) {
		pd->reply_id = REPLY_NAK;
		pd->ephemeral_data[0] = OSDP_PD_NAK_RECORD;
		return false;
	}
	return true;
}

static int pd_cmd_cap_ok(struct osdp_pd *pd, struct osdp_cmd *cmd)
{
	struct osdp_pd_cap *cap = NULL;

	/* Validate the cmd_id against a PD capabilities where applicable */
	switch (pd->cmd_id) {
	case CMD_ISTAT:
		cap = &pd->cap[OSDP_PD_CAP_CONTACT_STATUS_MONITORING];
		if (cap->num_items == 0 || cap->compliance_level == 0) {
			break;
		}
		return 0; /* Remove this when REPLY_ISTATR is supported */
	case CMD_OSTAT:
		cap = &pd->cap[OSDP_PD_CAP_OUTPUT_CONTROL];
		if (cap->num_items == 0 || cap->compliance_level == 0) {
			break;
		}
		return 0; /* Remove this when REPLY_OSTATR is supported */
	case CMD_OUT:
		cap = &pd->cap[OSDP_PD_CAP_OUTPUT_CONTROL];
		if (!cmd || cap->compliance_level == 0 ||
		    cmd->output.output_no + 1 > cap->num_items) {
			break;
		}
		return 1;
	case CMD_LED:
		cap = &pd->cap[OSDP_PD_CAP_READER_LED_CONTROL];
		if (!cmd || cap->compliance_level == 0 ||
		    cmd->led.led_number + 1 > cap->num_items) {
			break;
		}
		return 1;
	case CMD_BUZ:
		cap = &pd->cap[OSDP_PD_CAP_READER_AUDIBLE_OUTPUT];
		if (cap->num_items == 0 || cap->compliance_level == 0) {
			break;
		}
		return 1;
	case CMD_TEXT:
		cap = &pd->cap[OSDP_PD_CAP_READER_TEXT_OUTPUT];
		if (cap->num_items == 0 || cap->compliance_level == 0) {
			break;
		}
		return 1;
	case CMD_CHLNG:
	case CMD_SCRYPT:
	case CMD_KEYSET:
		cap = &pd->cap[OSDP_PD_CAP_COMMUNICATION_SECURITY];
		if (cap->compliance_level == 0) {
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_UNSUP;
			return 0;
		}
		return 1;
	}

	pd->reply_id = REPLY_NAK;
	pd->ephemeral_data[0] = OSDP_PD_NAK_CMD_UNKNOWN;
	LOG_ERR("PD is not capable of handling CMD(%02x); "
		"Reply with NAK_CMD_UNKNOWN", pd->cmd_id);
	return 0;
}

static int pd_decode_command(struct osdp_pd *pd, uint8_t *buf, int len)
{
	int ret = OSDP_PD_ERR_GENERIC;
	int pos = 0;
	struct osdp_cmd cmd;
	struct osdp_event *event;

	pd->reply_id = 0;
	pd->cmd_id = cmd.id = buf[pos++];
	len--;

	switch (pd->cmd_id) {
	case CMD_POLL:
		if (len != CMD_POLL_DATA_LEN) {
			break;
		}
		/* Check if we have external events in the queue */
		if (pd_event_dequeue(pd, &event) == 0) {
			ret = pd_translate_event(pd, event);
			pd->reply_id = ret;
			pd_event_free(pd, event);
		} else {
			pd->reply_id = REPLY_ACK;
		}
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_LSTAT:
		if (len != CMD_LSTAT_DATA_LEN) {
			break;
		}
		pd->reply_id = REPLY_LSTATR;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_ISTAT:
		if (len != CMD_ISTAT_DATA_LEN) {
			break;
		}
		if (!pd_cmd_cap_ok(pd, NULL)) {
			ret = OSDP_PD_ERR_REPLY;
			break;
		}
		pd->reply_id = REPLY_ISTATR;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_OSTAT:
		if (len != CMD_OSTAT_DATA_LEN) {
			break;
		}
		if (!pd_cmd_cap_ok(pd, NULL)) {
			ret = OSDP_PD_ERR_REPLY;
			break;
		}
		pd->reply_id = REPLY_OSTATR;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_RSTAT:
		if (len != CMD_RSTAT_DATA_LEN) {
			break;
		}
		pd->reply_id = REPLY_RSTATR;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_ID:
		if (len != CMD_ID_DATA_LEN) {
			break;
		}
		pos++;		/* Skip reply type info. */
		pd->reply_id = REPLY_PDID;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_CAP:
		if (len != CMD_CAP_DATA_LEN) {
			break;
		}
		pos++;		/* Skip reply type info. */
		pd->reply_id = REPLY_PDCAP;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_OUT:
		if (len != CMD_OUT_DATA_LEN || !pd->command_callback) {
			break;
		}
		cmd.id = OSDP_CMD_OUTPUT;
		cmd.output.output_no = buf[pos++];
		cmd.output.control_code = buf[pos++];
		cmd.output.timer_count = buf[pos++];
		cmd.output.timer_count |= buf[pos++] << 8;
		ret = OSDP_PD_ERR_REPLY;
		if (!pd_cmd_cap_ok(pd, &cmd)) {
			break;
		}
		if (!do_command_callback(pd, &cmd)) {
			break;
		}
		pd->reply_id = REPLY_ACK;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_LED:
		if (len != CMD_LED_DATA_LEN || !pd->command_callback) {
			break;
		}
		cmd.id = OSDP_CMD_LED;
		cmd.led.reader = buf[pos++];
		cmd.led.led_number = buf[pos++];

		cmd.led.temporary.control_code = buf[pos++];
		cmd.led.temporary.on_count = buf[pos++];
		cmd.led.temporary.off_count = buf[pos++];
		cmd.led.temporary.on_color = buf[pos++];
		cmd.led.temporary.off_color = buf[pos++];
		cmd.led.temporary.timer_count = buf[pos++];
		cmd.led.temporary.timer_count |= buf[pos++] << 8;

		cmd.led.permanent.control_code = buf[pos++];
		cmd.led.permanent.on_count = buf[pos++];
		cmd.led.permanent.off_count = buf[pos++];
		cmd.led.permanent.on_color = buf[pos++];
		cmd.led.permanent.off_color = buf[pos++];
		ret = OSDP_PD_ERR_REPLY;
		if (!pd_cmd_cap_ok(pd, &cmd)) {
			break;
		}
		if (!do_command_callback(pd, &cmd)) {
			break;
		}
		pd->reply_id = REPLY_ACK;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_BUZ:
		if (len != CMD_BUZ_DATA_LEN || !pd->command_callback) {
			break;
		}
		cmd.id = OSDP_CMD_BUZZER;
		cmd.buzzer.reader = buf[pos++];
		cmd.buzzer.control_code = buf[pos++];
		cmd.buzzer.on_count = buf[pos++];
		cmd.buzzer.off_count = buf[pos++];
		cmd.buzzer.rep_count = buf[pos++];
		ret = OSDP_PD_ERR_REPLY;
		if (!pd_cmd_cap_ok(pd, &cmd)) {
			break;
		}
		if (!do_command_callback(pd, &cmd)) {
			break;
		}
		pd->reply_id = REPLY_ACK;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_TEXT:
		if (len < CMD_TEXT_DATA_LEN || !pd->command_callback) {
			break;
		}
		cmd.id = OSDP_CMD_TEXT;
		cmd.text.reader = buf[pos++];
		cmd.text.control_code = buf[pos++];
		cmd.text.temp_time = buf[pos++];
		cmd.text.offset_row = buf[pos++];
		cmd.text.offset_col = buf[pos++];
		cmd.text.length = buf[pos++];
		if (cmd.text.length > OSDP_CMD_TEXT_MAX_LEN ||
		    ((len - CMD_TEXT_DATA_LEN) < cmd.text.length) ||
		    cmd.text.length > OSDP_CMD_TEXT_MAX_LEN) {
			break;
		}
		memcpy(cmd.text.data, buf + pos, cmd.text.length);
		ret = OSDP_PD_ERR_REPLY;
		if (!pd_cmd_cap_ok(pd, &cmd)) {
			break;
		}
		if (!do_command_callback(pd, &cmd)) {
			break;
		}
		pd->reply_id = REPLY_ACK;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_COMSET:
		if (len != CMD_COMSET_DATA_LEN || !pd->command_callback) {
			break;
		}
		cmd.id = OSDP_CMD_COMSET;
		cmd.comset.address = buf[pos++];
		cmd.comset.baud_rate = buf[pos++];
		cmd.comset.baud_rate |= buf[pos++] << 8;
		cmd.comset.baud_rate |= buf[pos++] << 16;
		cmd.comset.baud_rate |= buf[pos++] << 24;
		if (cmd.comset.address >= 0x7F ||
		    (cmd.comset.baud_rate != 9600 &&
		     cmd.comset.baud_rate != 19200 &&
		     cmd.comset.baud_rate != 38400 &&
		     cmd.comset.baud_rate != 115200 &&
		     cmd.comset.baud_rate != 230400)) {
			LOG_ERR("COMSET Failed! command discarded");
			cmd.comset.address = pd->address;
			cmd.comset.baud_rate = pd->baud_rate;
			break;
		}
		if (!do_command_callback(pd, &cmd)) {
			ret = OSDP_PD_ERR_REPLY;
			break;
		}
		memcpy(pd->ephemeral_data, &cmd, sizeof(struct osdp_cmd));
		pd->reply_id = REPLY_COM;
		ret = OSDP_PD_ERR_NONE;
		break;
#ifdef CONFIG_OSDP_SC_ENABLED
	case CMD_KEYSET:
		if (len != CMD_KEYSET_DATA_LEN) {
			break;
		}
		/* only key_type == 1 (SCBK) and key_len == 16 is supported */
		if (buf[pos] != 1 || buf[pos + 1] != 16) {
			LOG_ERR("Keyset invalid len/type: %d/%d",
				buf[pos], buf[pos + 1]);
			break;
		}
		ret = OSDP_PD_ERR_REPLY;
		if (!pd_cmd_cap_ok(pd, NULL)) {
			break;
		}
		/**
		 * For CMD_KEYSET to be accepted, PD must be
		 * ONLINE and SC_ACTIVE.
		 */
		if (!sc_is_active(pd)) {
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_COND;
			LOG_ERR("Keyset with SC inactive");
			break;
		}
		cmd.id = OSDP_CMD_KEYSET;
		cmd.keyset.type = buf[pos++];
		cmd.keyset.length = buf[pos++];
		memcpy(cmd.keyset.data, buf + pos, 16);
		if (!pd->command_callback) {
			LOG_ERR("Keyset without a command callback! The SC new "
				"SCBK will be lost when the PD reboots.");
		} else if (!do_command_callback(pd, &cmd)) {
			break;
		}
		memcpy(pd->sc.scbk, cmd.keyset.data, 16);
		CLEAR_FLAG(pd, PD_FLAG_SC_USE_SCBKD);
		CLEAR_FLAG(pd, PD_FLAG_INSTALL_MODE);
		sc_deactivate(pd);
		pd->reply_id = REPLY_ACK;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_CHLNG:
		if (len != CMD_CHLNG_DATA_LEN) {
			break;
		}
		ret = OSDP_PD_ERR_REPLY;
		if (!pd_cmd_cap_ok(pd, NULL)) {
			break;
		}
		sc_deactivate(pd);
		osdp_sc_setup(pd);
		memcpy(pd->sc.cp_random, buf + pos, 8);
		pd->reply_id = REPLY_CCRYPT;
		ret = OSDP_PD_ERR_NONE;
		break;
	case CMD_SCRYPT:
		if (len != CMD_SCRYPT_DATA_LEN) {
			break;
		}
		ret = OSDP_PD_ERR_REPLY;
		if (!pd_cmd_cap_ok(pd, NULL)) {
			break;
		}
		if (sc_is_active(pd)) {
			pd->reply_id = REPLY_NAK;
			pd->ephemeral_data[0] = OSDP_PD_NAK_SC_COND;
			LOG_ERR("Out of order CMD_SCRYPT; has CP gone rogue?");
			break;
		}
		memcpy(pd->sc.cp_cryptogram, buf + pos, CMD_SCRYPT_DATA_LEN);
		pd->reply_id = REPLY_RMAC_I;
		ret = OSDP_PD_ERR_NONE;
		break;
#endif /* CONFIG_OSDP_SC_ENABLED */
	default:
		LOG_ERR("Unknown CMD(%02x)", pd->cmd_id);
		pd->reply_id = REPLY_NAK;
		pd->ephemeral_data[0] = OSDP_PD_NAK_CMD_UNKNOWN;
		return OSDP_PD_ERR_REPLY;
	}

	if (ret == OSDP_PD_ERR_GENERIC) {
		LOG_ERR("Failed to decode command: CMD(%02x) Len:%d ret:%d",
			pd->cmd_id, len, ret);
		pd->reply_id = REPLY_NAK;
		pd->ephemeral_data[0] = OSDP_PD_NAK_CMD_LEN;
		ret = OSDP_PD_ERR_REPLY;
	}

	if (pd->cmd_id != CMD_POLL) {
		LOG_DBG("CMD: %02x REPLY: %02x", pd->cmd_id, pd->reply_id);
	}

	return ret;
}

static inline bool check_buf_len(int need, int have)
{
	if (need > have) {
		LOG_ERR("OOM at build reply: need:%d have:%d", need, have);
		return false;
	}
	return true;
}

/**
 * Returns:
 * +ve: length of command
 * -ve: error
 */
static int pd_build_reply(struct osdp_pd *pd, uint8_t *buf, int max_len)
{
	int ret = OSDP_PD_ERR_GENERIC;
	int i, len = 0;
	struct osdp_cmd *cmd;
	struct osdp_event *event;
	int data_off = osdp_phy_packet_get_data_offset(pd, buf);
#ifdef CONFIG_OSDP_SC_ENABLED
	uint8_t *smb = osdp_phy_packet_get_smb(pd, buf);
#endif
	buf += data_off;
	max_len -= data_off;

	switch (pd->reply_id) {
	case REPLY_ACK:
		if (!check_buf_len(REPLY_ACK_LEN, max_len)) {
			return OSDP_PD_ERR_GENERIC;
		}
		buf[len++] = pd->reply_id;
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_PDID:
		if (!check_buf_len(REPLY_PDID_LEN, max_len)) {
			return OSDP_PD_ERR_GENERIC;
		}
		buf[len++] = pd->reply_id;

		buf[len++] = BYTE_0(pd->id.vendor_code);
		buf[len++] = BYTE_1(pd->id.vendor_code);
		buf[len++] = BYTE_2(pd->id.vendor_code);

		buf[len++] = pd->id.model;
		buf[len++] = pd->id.version;

		buf[len++] = BYTE_0(pd->id.serial_number);
		buf[len++] = BYTE_1(pd->id.serial_number);
		buf[len++] = BYTE_2(pd->id.serial_number);
		buf[len++] = BYTE_3(pd->id.serial_number);

		buf[len++] = BYTE_3(pd->id.firmware_version);
		buf[len++] = BYTE_2(pd->id.firmware_version);
		buf[len++] = BYTE_1(pd->id.firmware_version);
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_PDCAP:
		if (!check_buf_len(REPLY_PDCAP_LEN, max_len)) {
			return OSDP_PD_ERR_GENERIC;
		}
		buf[len++] = pd->reply_id;
		for (i = 1; i < OSDP_PD_CAP_SENTINEL; i++) {
			if (pd->cap[i].function_code != i) {
				continue;
			}
			if (max_len < REPLY_PDCAP_ENTITY_LEN) {
				LOG_ERR("Out of buffer space!");
				break;
			}
			buf[len++] = i;
			buf[len++] = pd->cap[i].compliance_level;
			buf[len++] = pd->cap[i].num_items;
			max_len -= REPLY_PDCAP_ENTITY_LEN;
		}
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_LSTATR:
		if (!check_buf_len(REPLY_LSTATR_LEN, max_len)) {
			return OSDP_PD_ERR_GENERIC;
		}
		buf[len++] = pd->reply_id;
		buf[len++] = ISSET_FLAG(pd, PD_FLAG_TAMPER);
		buf[len++] = ISSET_FLAG(pd, PD_FLAG_POWER);
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_RSTATR:
		if (!check_buf_len(REPLY_RSTATR_LEN, max_len)) {
			return OSDP_PD_ERR_GENERIC;
		}
		buf[len++] = pd->reply_id;
		buf[len++] = ISSET_FLAG(pd, PD_FLAG_R_TAMPER);
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_KEYPPAD:
		event = (struct osdp_event *)pd->ephemeral_data;
		if (!check_buf_len(REPLY_KEYPAD_LEN + event->keypress.length, max_len)) {
			return OSDP_PD_ERR_GENERIC;
		}
		buf[len++] = pd->reply_id;
		buf[len++] = (uint8_t)event->keypress.reader_no;
		buf[len++] = (uint8_t)event->keypress.length;
		memcpy(buf + len, event->keypress.data, event->keypress.length);
		len += event->keypress.length;
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_RAW: {
		int len_bytes;

		event = (struct osdp_event *)pd->ephemeral_data;
		len_bytes = (event->cardread.length + 7) / 8;
		if (!check_buf_len(REPLY_RAW_LEN + len_bytes, max_len)) {
			return OSDP_PD_ERR_GENERIC;
		}
		buf[len++] = pd->reply_id;
		buf[len++] = (uint8_t)event->cardread.reader_no;
		buf[len++] = (uint8_t)event->cardread.format;
		buf[len++] = BYTE_0(event->cardread.length);
		buf[len++] = BYTE_1(event->cardread.length);
		memcpy(buf + len, event->cardread.data, len_bytes);
		len += len_bytes;
		ret = OSDP_PD_ERR_NONE;
		break;
	}
	case REPLY_FMT:
		event = (struct osdp_event *)pd->ephemeral_data;
		if (!check_buf_len(REPLY_FMT_LEN + event->cardread.length, max_len)) {
			return OSDP_PD_ERR_GENERIC;
		}
		buf[len++] = pd->reply_id;
		buf[len++] = (uint8_t)event->cardread.reader_no;
		buf[len++] = (uint8_t)event->cardread.direction;
		buf[len++] = (uint8_t)event->cardread.length;
		memcpy(buf + len, event->cardread.data, event->cardread.length);
		len += event->cardread.length;
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_COM:
		if (!check_buf_len(REPLY_COM_LEN, max_len)) {
			return OSDP_PD_ERR_GENERIC;
		}
		/**
		 * If COMSET succeeds, the PD must reply with the old params and
		 * then switch to the new params from then then on. We have the
		 * new params in the commands struct that we just enqueued so
		 * we can peek at tail of command queue and set that to
		 * pd->addr/pd->baud_rate.
		 *
		 * TODO: Persist pd->address and pd->baud_rate via
		 * subsys/settings
		 */
		cmd = (struct osdp_cmd *)pd->ephemeral_data;
		buf[len++] = pd->reply_id;
		buf[len++] = cmd->comset.address;
		buf[len++] = BYTE_0(cmd->comset.baud_rate);
		buf[len++] = BYTE_1(cmd->comset.baud_rate);
		buf[len++] = BYTE_2(cmd->comset.baud_rate);
		buf[len++] = BYTE_3(cmd->comset.baud_rate);

		pd->address = (int)cmd->comset.address;
		pd->baud_rate = (int)cmd->comset.baud_rate;
		LOG_INF("COMSET Succeeded! New PD-Addr: %d; Baud: %d",
			pd->address, pd->baud_rate);
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_NAK:
		if (!check_buf_len(REPLY_NAK_LEN, max_len)) {
			return OSDP_PD_ERR_GENERIC;
		}
		buf[len++] = pd->reply_id;
		buf[len++] = pd->ephemeral_data[0];
		ret = OSDP_PD_ERR_NONE;
		break;
#ifdef CONFIG_OSDP_SC_ENABLED
	case REPLY_CCRYPT:
		if (smb == NULL) {
			break;
		}
		if (!check_buf_len(REPLY_CCRYPT_LEN, max_len)) {
			return OSDP_PD_ERR_GENERIC;
		}
		osdp_fill_random(pd->sc.pd_random, 8);
		osdp_compute_session_keys(pd);
		osdp_compute_pd_cryptogram(pd);
		buf[len++] = pd->reply_id;
		memcpy(buf + len, pd->sc.pd_client_uid, 8);
		memcpy(buf + len + 8, pd->sc.pd_random, 8);
		memcpy(buf + len + 16, pd->sc.pd_cryptogram, 16);
		len += 32;
		smb[0] = 3;      /* length */
		smb[1] = SCS_12; /* type */
		smb[2] = ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD) ? 0 : 1;
		ret = OSDP_PD_ERR_NONE;
		break;
	case REPLY_RMAC_I:
		if (smb == NULL) {
			break;
		}
		if (!check_buf_len(REPLY_RMAC_I_LEN, max_len)) {
			return OSDP_PD_ERR_GENERIC;
		}
		osdp_compute_rmac_i(pd);
		buf[len++] = pd->reply_id;
		memcpy(buf + len, pd->sc.r_mac, 16);
		len += 16;
		smb[0] = 3;       /* length */
		smb[1] = SCS_14;  /* type */
		if (osdp_verify_cp_cryptogram(pd) == 0) {
			smb[2] = 1;  /* CP auth succeeded */
			sc_activate(pd);
			pd->sc_tstamp = osdp_millis_now();
			if (ISSET_FLAG(pd, PD_FLAG_SC_USE_SCBKD)) {
				LOG_WRN("SC Active with SCBK-D");
			} else {
				LOG_INF("SC Active");
			}
		} else {
			smb[2] = 0;  /* CP auth failed */
			LOG_WRN("failed to verify CP_crypt");
		}
		ret = OSDP_PD_ERR_NONE;
		break;
#endif /* CONFIG_OSDP_SC_ENABLED */
	}

#ifdef CONFIG_OSDP_SC_ENABLED
	if (smb && (smb[1] > SCS_14) && sc_is_active(pd)) {
		smb[0] = 2; /* length */
		smb[1] = (len > 1) ? SCS_18 : SCS_16;
	}
#endif /* CONFIG_OSDP_SC_ENABLED */

	if (ret != 0) {
		/* catch all errors and report it as a RECORD error to CP */
		LOG_ERR("Failed to build REPLY(%02x); Sending NAK instead!",
			pd->reply_id);
		if (!check_buf_len(REPLY_NAK_LEN, max_len)) {
			return OSDP_PD_ERR_GENERIC;
		}
		buf[0] = REPLY_NAK;
		buf[1] = OSDP_PD_NAK_RECORD;
		len = 2;
	}

	return len;
}

static int pd_send_reply(struct osdp_pd *pd)
{
	int ret, len;

	/* init packet buf with header */
	len = osdp_phy_packet_init(pd, pd->rx_buf, sizeof(pd->rx_buf));
	if (len < 0) {
		return OSDP_PD_ERR_GENERIC;
	}

	/* fill reply data */
	ret = pd_build_reply(pd, pd->rx_buf, sizeof(pd->rx_buf));
	if (ret <= 0) {
		return OSDP_PD_ERR_GENERIC;
	}
	len += ret;

	/* finalize packet */
	len = osdp_phy_packet_finalize(pd, pd->rx_buf, len, sizeof(pd->rx_buf));
	if (len < 0) {
		return OSDP_PD_ERR_GENERIC;
	}

	/* flush rx to remove any invalid data. */
	if (pd->channel.flush) {
		pd->channel.flush(pd->channel.data);
	}

	ret = pd->channel.send(pd->channel.data, pd->rx_buf, len);
	if (ret != len) {
		LOG_ERR("Channel send for %d bytes failed! ret: %d", len, ret);
		return OSDP_PD_ERR_GENERIC;
	}

	if (IS_ENABLED(CONFIG_OSDP_PACKET_TRACE)) {
		if (pd->cmd_id != CMD_POLL) {
			osdp_dump("PD sent", pd->rx_buf, len);
		}
	}

	return OSDP_PD_ERR_NONE;
}

static int pd_decode_packet(struct osdp_pd *pd, int *one_pkt_len)
{
	int err, len;
	uint8_t *buf;

	err = osdp_phy_check_packet(pd, pd->rx_buf, pd->rx_buf_len, one_pkt_len);

	/* Translate phy error codes to PD errors */
	switch (err) {
	case OSDP_ERR_PKT_NONE:
		break;
	case OSDP_ERR_PKT_NACK:
		return OSDP_PD_ERR_REPLY;
	case OSDP_ERR_PKT_WAIT:
		return OSDP_PD_ERR_NO_DATA;
	case OSDP_ERR_PKT_SKIP:
		return OSDP_PD_ERR_IGNORE;
	case OSDP_ERR_PKT_FMT:
		return OSDP_PD_ERR_GENERIC;
	default:
		return err; /* propagate other errors as-is */
	}

	len = osdp_phy_decode_packet(pd, pd->rx_buf, *one_pkt_len, &buf);
	if (len <= 0) {
		if (len == OSDP_ERR_PKT_NACK) {
			return OSDP_PD_ERR_REPLY; /* Send a NAK */
		}
		return OSDP_PD_ERR_GENERIC; /* fatal errors */
	}

	return pd_decode_command(pd, buf, len);
}

static int pd_receive_and_process_command(struct osdp_pd *pd)
{
	uint8_t *buf;
	int len, err, remaining;

	buf = pd->rx_buf + pd->rx_buf_len;
	remaining = sizeof(pd->rx_buf) - pd->rx_buf_len;

	len = pd->channel.recv(pd->channel.data, buf, remaining);
	if (len <= 0) { /* No data received */
		return OSDP_PD_ERR_NO_DATA;
	}

	/**
	 * We received some data on the bus; update pd->tstamp. A rouge CP can
	 * send one byte at a time to extend this command's window but that
	 * shouldn't cause any issues related to secure channel as it has it's
	 * own timestamp.
	 */
	pd->tstamp = osdp_millis_now();
	pd->rx_buf_len += len;

	if (IS_ENABLED(CONFIG_OSDP_PACKET_TRACE)) {
		/**
		 * A crude way of identifying and not printing poll messages
		 * when CONFIG_OSDP_PACKET_TRACE is enabled. This is an early
		 * print to catch errors so keeping it simple.
		 */
		if (pd->rx_buf_len > 8 &&
		    pd->rx_buf[6] != CMD_POLL && pd->rx_buf[8] != CMD_POLL) {
			osdp_dump("PD received", pd->rx_buf, pd->rx_buf_len);
		}
	}

	err = pd_decode_packet(pd, &len);

	if (err == OSDP_PD_ERR_NO_DATA) {
		return err;
	}

	/* We are done with the packet (error or not). Remove processed bytes */
	remaining = pd->rx_buf_len - len;
	if (remaining) {
		memmove(pd->rx_buf, pd->rx_buf + len, remaining);
	}

	/**
	 * Store remaining length that needs to be processed.
	 * State machine will be updated accordingly.
	 */
	pd->rx_buf_len = remaining;

	return err;
}

static inline void pd_error_reset(struct osdp_pd *pd)
{
	sc_deactivate(pd);
	if (pd->channel.flush) {
		pd->channel.flush(pd->channel.data);
	}
	pd->rx_buf_len = 0;
}

void osdp_update(struct osdp *ctx)
{
	int ret;
	struct osdp_pd *pd = osdp_to_pd(ctx, 0);

#ifdef CONFIG_OSDP_SC_ENABLED
	/**
	 * If secure channel is established, we need to make sure that
	 * the session is valid before accepting a command.
	 */
	if (sc_is_active(pd) &&
	    osdp_millis_since(pd->sc_tstamp) > OSDP_PD_SC_TIMEOUT_MS) {
		LOG_INF("PD SC session timeout!");
		sc_deactivate(pd);
	}
#endif

	ret = pd_receive_and_process_command(pd);

	if (ret == OSDP_PD_ERR_IGNORE) {
		return;
	}

	if (ret == OSDP_PD_ERR_NO_DATA) {
		if (pd->rx_buf_len == 0 ||
		    osdp_millis_since(pd->tstamp) < OSDP_RESP_TOUT_MS) {
			return;
		}
		LOG_DBG("rx_buf: %d", pd->rx_buf_len);
		osdp_dump("Buf", pd->rx_buf, pd->rx_buf_len);
	}

	if (ret != OSDP_PD_ERR_NONE && ret != OSDP_PD_ERR_REPLY) {
		LOG_ERR("CMD receive error/timeout - err:%d", ret);
		pd_error_reset(pd);
		return;
	}

#ifdef CONFIG_OSDP_SC_ENABLED
	if (ret == OSDP_PD_ERR_NONE && sc_is_active(pd)) {
		pd->sc_tstamp = osdp_millis_now();
	}
#endif

	ret = pd_send_reply(pd);
	if (ret != OSDP_PD_ERR_NONE) {
		/**
		 * PD received and decoded a valid command from CP but failed to
		 * send the intended response?? This should not happen; but if
		 * it did, we cannot do anything about it, just complain about
		 * it and limp back home.
		 */
		LOG_WRN("REPLY send failed! CP may be waiting..");
		return;
	}

}

static void osdp_pd_set_attributes(struct osdp_pd *pd, struct osdp_pd_cap *cap,
				   struct osdp_pd_id *id)
{
	int fc;

	while (cap && ((fc = cap->function_code) > 0)) {
		if (fc >= OSDP_PD_CAP_SENTINEL) {
			break;
		}
		pd->cap[fc].function_code = cap->function_code;
		pd->cap[fc].compliance_level = cap->compliance_level;
		pd->cap[fc].num_items = cap->num_items;
		cap++;
	}
	if (id != NULL) {
		memcpy(&pd->id, id, sizeof(struct osdp_pd_id));
	}
}

int osdp_setup(struct osdp *ctx, uint8_t *key)
{
	ARG_UNUSED(key);
	struct osdp_pd *pd;

	if (NUM_PD(ctx) != 1) {
		return -1;
	}
	pd = osdp_to_pd(ctx, 0);
	osdp_pd_set_attributes(pd, osdp_pd_cap, &osdp_pd_id);
	SET_FLAG(pd, PD_FLAG_PD_MODE);
#ifdef CONFIG_OSDP_SC_ENABLED
	if (key == NULL) {
		LOG_WRN("SCBK not provided. PD is in INSTALL_MODE");
		SET_FLAG(pd, PD_FLAG_INSTALL_MODE);
	} else {
		memcpy(pd->sc.scbk, key, 16);
	}
	SET_FLAG(pd, PD_FLAG_SC_CAPABLE);
#endif
	return 0;
}

/* --- Exported Methods --- */

void osdp_pd_set_command_callback(pd_command_callback_t cb, void *arg)
{
	struct osdp_pd *pd = osdp_to_pd(osdp_get_ctx(), 0);

	pd->command_callback_arg = arg;
	pd->command_callback = cb;
}

int osdp_pd_notify_event(const struct osdp_event *event)
{
	struct osdp_event *ev;
	struct osdp_pd *pd = osdp_to_pd(osdp_get_ctx(), 0);

	ev = pd_event_alloc(pd);
	if (ev == NULL) {
		return -1;
	}

	memcpy(ev, event, sizeof(struct osdp_event));
	pd_event_enqueue(pd, ev);
	return 0;
}
