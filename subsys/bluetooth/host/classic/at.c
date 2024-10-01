/**
 * @file at.c
 * Generic AT command handling library implementation
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <zephyr/net_buf.h>

#include "at.h"

static void next_list(struct at_client *at)
{
	if (at->buf[at->pos] == ',') {
		at->pos++;
	}
}

int at_check_byte(struct net_buf *buf, char check_byte)
{
	const unsigned char *str = buf->data;

	if (*str != check_byte) {
		return -EINVAL;
	}
	net_buf_pull(buf, 1);

	return 0;
}

static void skip_space(struct at_client *at)
{
	while (at->buf[at->pos] == ' ') {
		at->pos++;
	}
}

int at_get_number(struct at_client *at, uint32_t *val)
{
	uint32_t i;

	skip_space(at);

	for (i = 0U, *val = 0U;
	     isdigit((unsigned char)at->buf[at->pos]) != 0;
	     at->pos++, i++) {
		*val = *val * 10U + at->buf[at->pos] - '0';
	}

	if (i == 0U) {
		return -ENODATA;
	}

	next_list(at);
	return 0;
}

static bool str_has_prefix(const char *str, const char *prefix)
{
	if (strncmp(str, prefix, strlen(prefix)) != 0) {
		return false;
	}

	return true;
}

static int at_parse_result(const char *str, struct net_buf *buf,
			   enum at_result *result)
{
	/* Map the result and check for end lf */
	if ((!strncmp(str, "OK", 2)) && (at_check_byte(buf, '\n') == 0)) {
		*result = AT_RESULT_OK;
		return 0;
	}

	if ((!strncmp(str, "ERROR", 5)) && (at_check_byte(buf, '\n')) == 0) {
		*result = AT_RESULT_ERROR;
		return 0;
	}

	return -ENOMSG;
}

static int get_cmd_value(struct at_client *at, struct net_buf *buf,
			 char stop_byte, enum at_cmd_state cmd_state)
{
	int cmd_len = 0;
	uint8_t pos = at->pos;
	const char *str = (char *)buf->data;

	while (cmd_len < buf->len && at->pos != at->buf_max_len) {
		if (*str != stop_byte) {
			at->buf[at->pos++] = *str;
			cmd_len++;
			str++;
			pos = at->pos;
		} else {
			cmd_len++;
			at->buf[at->pos] = '\0';
			at->pos = 0U;
			at->cmd_state = cmd_state;
			break;
		}
	}
	net_buf_pull(buf, cmd_len);

	if (pos == at->buf_max_len) {
		return -ENOBUFS;
	}

	return 0;
}

static int get_response_string(struct at_client *at, struct net_buf *buf,
			       char stop_byte, enum at_state state)
{
	int cmd_len = 0;
	uint8_t pos = at->pos;
	const char *str = (char *)buf->data;

	while (cmd_len < buf->len && at->pos != at->buf_max_len) {
		if (*str != stop_byte) {
			at->buf[at->pos++] = *str;
			cmd_len++;
			str++;
			pos = at->pos;
		} else {
			cmd_len++;
			at->buf[at->pos] = '\0';
			at->pos = 0U;
			at->state = state;
			break;
		}
	}
	net_buf_pull(buf, cmd_len);

	if (pos == at->buf_max_len) {
		return -ENOBUFS;
	}

	return 0;
}

static void reset_buffer(struct at_client *at)
{
	(void)memset(at->buf, 0, at->buf_max_len);
	at->pos = 0U;
}

static int at_state_start(struct at_client *at, struct net_buf *buf)
{
	int err;

	err = at_check_byte(buf, '\r');
	if (err < 0) {
		return err;
	}
	at->state = AT_STATE_START_CR;

	return 0;
}

static int at_state_start_cr(struct at_client *at, struct net_buf *buf)
{
	int err;

	err = at_check_byte(buf, '\n');
	if (err < 0) {
		return err;
	}
	at->state = AT_STATE_START_LF;

	return 0;
}

static int at_state_start_lf(struct at_client *at, struct net_buf *buf)
{
	reset_buffer(at);
	if (at_check_byte(buf, '+') == 0) {
		at->state = AT_STATE_GET_CMD_STRING;
		return 0;
	} else if (isalpha(*buf->data) != 0) {
		at->state = AT_STATE_GET_RESULT_STRING;
		return 0;
	}

	return -ENODATA;
}

static int at_state_get_cmd_string(struct at_client *at, struct net_buf *buf)
{
	return get_response_string(at, buf, ':', AT_STATE_PROCESS_CMD);
}

static bool is_cmer(struct at_client *at)
{
	if (strncmp(at->buf, "CME ERROR", 9) == 0) {
		return true;
	}

	return false;
}

static int at_state_process_cmd(struct at_client *at, struct net_buf *buf)
{
	if (is_cmer(at)) {
		at->state = AT_STATE_PROCESS_AG_NW_ERR;
		return 0;
	}

	if (at->resp) {
		at->resp(at, buf);
		at->resp = NULL;
		return 0;
	}
	at->state = AT_STATE_UNSOLICITED_CMD;
	return 0;
}

static int at_state_get_result_string(struct at_client *at, struct net_buf *buf)
{
	return get_response_string(at, buf, '\r', AT_STATE_PROCESS_RESULT);
}

static bool is_ring(struct at_client *at)
{
	if (strncmp(at->buf, "RING", 4) == 0) {
		return true;
	}

	return false;
}

static int at_state_process_result(struct at_client *at, struct net_buf *buf)
{
	enum at_cme cme_err;
	enum at_result result;

	if (is_ring(at)) {
		at->state = AT_STATE_UNSOLICITED_CMD;
		return 0;
	}

	if (at_parse_result(at->buf, buf, &result) == 0) {
		if (at->finish) {
			/* cme_err is 0 - Is invalid until result is
			 * AT_RESULT_CME_ERROR
			 */
			cme_err = 0;
			at->finish(at, result, cme_err);
		}
	}

	/* Reset the state to process unsolicited response */
	at->cmd_state = AT_CMD_START;
	at->state = AT_STATE_START;

	return 0;
}

int cme_handle(struct at_client *at)
{
	enum at_cme cme_err;
	uint32_t val;

	if (!at_get_number(at, &val) && val <= CME_ERROR_NETWORK_NOT_ALLOWED) {
		cme_err = val;
	} else {
		cme_err = CME_ERROR_UNKNOWN;
	}

	if (at->finish) {
		at->finish(at, AT_RESULT_CME_ERROR, cme_err);
	}

	return 0;
}

static int at_state_process_ag_nw_err(struct at_client *at, struct net_buf *buf)
{
	at->cmd_state = AT_CMD_GET_VALUE;
	return at_parse_cmd_input(at, buf, NULL, cme_handle,
				  AT_CMD_TYPE_NORMAL);
}

static int at_state_unsolicited_cmd(struct at_client *at, struct net_buf *buf)
{
	if (at->unsolicited) {
		return at->unsolicited(at, buf);
	}

	return -ENODATA;
}

/* The order of handler function should match the enum at_state */
static handle_parse_input_t parser_cb[] = {
	at_state_start, /* AT_STATE_START */
	at_state_start_cr, /* AT_STATE_START_CR */
	at_state_start_lf, /* AT_STATE_START_LF */
	at_state_get_cmd_string, /* AT_STATE_GET_CMD_STRING */
	at_state_process_cmd, /* AT_STATE_PROCESS_CMD */
	at_state_get_result_string, /* AT_STATE_GET_RESULT_STRING */
	at_state_process_result, /* AT_STATE_PROCESS_RESULT */
	at_state_process_ag_nw_err, /* AT_STATE_PROCESS_AG_NW_ERR */
	at_state_unsolicited_cmd /* AT_STATE_UNSOLICITED_CMD */
};

int at_parse_input(struct at_client *at, struct net_buf *buf)
{
	int ret;

	while (buf->len) {
		if (at->state < AT_STATE_START || at->state >= AT_STATE_END) {
			return -EINVAL;
		}
		ret = parser_cb[at->state](at, buf);
		if (ret < 0) {
			/* Reset the state in case of error */
			at->cmd_state = AT_CMD_START;
			at->state = AT_STATE_START;
			return ret;
		}
	}

	return 0;
}

static int at_cmd_start(struct at_client *at, struct net_buf *buf,
			const char *prefix, parse_val_t func,
			enum at_cmd_type type)
{
	if (!str_has_prefix(at->buf, prefix)) {
		if (type == AT_CMD_TYPE_NORMAL) {
			at->state = AT_STATE_UNSOLICITED_CMD;
		}
		return -ENODATA;
	}

	if (type == AT_CMD_TYPE_OTHER) {
		/* Skip for Other type such as ..RING.. which does not have
		 * values to get processed.
		 */
		at->cmd_state = AT_CMD_PROCESS_VALUE;
	} else {
		at->cmd_state = AT_CMD_GET_VALUE;
	}

	return 0;
}

static int at_cmd_get_value(struct at_client *at, struct net_buf *buf,
			    const char *prefix, parse_val_t func,
			    enum at_cmd_type type)
{
	/* Reset buffer before getting the values */
	reset_buffer(at);
	return get_cmd_value(at, buf, '\r', AT_CMD_PROCESS_VALUE);
}

static int at_cmd_process_value(struct at_client *at, struct net_buf *buf,
				const char *prefix, parse_val_t func,
				enum at_cmd_type type)
{
	int ret;

	ret = func(at);
	at->cmd_state = AT_CMD_STATE_END_LF;

	return ret;
}

static int at_cmd_state_end_lf(struct at_client *at, struct net_buf *buf,
			       const char *prefix, parse_val_t func,
			       enum at_cmd_type type)
{
	int err;

	err = at_check_byte(buf, '\n');
	if (err < 0) {
		return err;
	}

	at->cmd_state = AT_CMD_START;
	at->state = AT_STATE_START;
	return 0;
}

/* The order of handler function should match the enum at_cmd_state */
static handle_cmd_input_t cmd_parser_cb[] = {
	at_cmd_start, /* AT_CMD_START */
	at_cmd_get_value, /* AT_CMD_GET_VALUE */
	at_cmd_process_value, /* AT_CMD_PROCESS_VALUE */
	at_cmd_state_end_lf /* AT_CMD_STATE_END_LF */
};

int at_parse_cmd_input(struct at_client *at, struct net_buf *buf,
		       const char *prefix, parse_val_t func,
		       enum at_cmd_type type)
{
	int ret;

	while (buf->len) {
		if (at->cmd_state < AT_CMD_START ||
		    at->cmd_state >= AT_CMD_STATE_END) {
			return -EINVAL;
		}
		ret = cmd_parser_cb[at->cmd_state](at, buf, prefix, func, type);
		if (ret < 0) {
			return ret;
		}
		/* Check for main state, the end of cmd parsing and return. */
		if (at->state == AT_STATE_START) {
			return 0;
		}
	}

	return 0;
}

int at_has_next_list(struct at_client *at)
{
	return at->buf[at->pos] != '\0';
}

int at_open_list(struct at_client *at)
{
	skip_space(at);

	/* The list shall start with '(' open parenthesis */
	if (at->buf[at->pos] != '(') {
		return -ENODATA;
	}
	at->pos++;

	return 0;
}

int at_close_list(struct at_client *at)
{
	skip_space(at);

	if (at->buf[at->pos] != ')') {
		return -ENODATA;
	}
	at->pos++;

	next_list(at);

	return 0;
}

int at_list_get_string(struct at_client *at, char *name, uint8_t len)
{
	int i = 0;

	skip_space(at);

	if (at->buf[at->pos] != '"') {
		return -ENODATA;
	}
	at->pos++;

	while (at->buf[at->pos] != '\0' && at->buf[at->pos] != '"') {
		if (i == len) {
			return -ENODATA;
		}
		name[i++] = at->buf[at->pos++];
	}

	if (i == len) {
		return -ENODATA;
	}

	name[i] = '\0';

	if (at->buf[at->pos] != '"') {
		return -ENODATA;
	}
	at->pos++;

	skip_space(at);
	next_list(at);

	return 0;
}

int at_list_get_range(struct at_client *at, uint32_t *min, uint32_t *max)
{
	uint32_t low, high;
	int ret;

	ret = at_get_number(at, &low);
	if (ret < 0) {
		return ret;
	}

	if (at->buf[at->pos] == '-') {
		at->pos++;
		goto out;
	}

	if (isdigit((unsigned char)at->buf[at->pos]) == 0) {
		return -ENODATA;
	}
out:
	ret = at_get_number(at, &high);
	if (ret < 0) {
		return ret;
	}

	*min = low;
	*max = high;

	next_list(at);

	return 0;
}

void at_register_unsolicited(struct at_client *at, at_resp_cb_t unsolicited)
{
	at->unsolicited = unsolicited;
}

void at_register(struct at_client *at, at_resp_cb_t resp, at_finish_cb_t finish)
{
	at->resp = resp;
	at->finish = finish;
	at->state = AT_STATE_START;
}
