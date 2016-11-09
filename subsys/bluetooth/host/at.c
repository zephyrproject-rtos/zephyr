/**
 * @file at.c
 * Generic AT command handling library implementation
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <net/buf.h>

#include "at.h"

int at_check_byte(struct net_buf *buf, char check_byte)
{
	const char *str = buf->data;

	if (*str != check_byte) {
		return -EINVAL;
	}
	net_buf_pull(buf, 1);

	return 0;
}

const char *skip_whitespace(const char *buf)
{
	while (*buf == ' ') {
		buf++;
	}

	return buf;
}

int at_get_number(const char *buf, uint32_t *val)
{
	uint32_t i;

	buf = skip_whitespace(buf);

	for (i = 0, *val = 0; isdigit(*buf); buf++, i++) {
		*val = *val * 10 + *buf - '0';
	}

	if (i == 0) {
		return -ENODATA;
	}

	return 0;
}

int str_has_prefix(const char *str, const char *prefix)
{
	if (strncmp(str, prefix, strlen(prefix)) != 0) {
		return -EINVAL;
	}

	return 0;
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
	int len = buf->len;
	const char *str = buf->data;

	while (cmd_len < len && at->buf_pos != at->buf_max_len) {
		if (*str != stop_byte) {
			at->buf[at->buf_pos] = *str;
			cmd_len++;
			str++;
			at->buf_pos++;
		} else {
			cmd_len++;
			at->buf[at->buf_pos] = '\0';
			at->cmd_state = cmd_state;
			break;
		}
	}
	net_buf_pull(buf, cmd_len);

	if (at->buf_pos == at->buf_max_len) {
		return -ENOBUFS;
	}

	return 0;
}

static int get_response_string(struct at_client *at, struct net_buf *buf,
			       char stop_byte, enum at_state state)
{
	int cmd_len = 0;
	int len = buf->len;
	const char *str = buf->data;

	while (cmd_len < len && at->buf_pos != at->buf_max_len) {
		if (*str != stop_byte) {
			at->buf[at->buf_pos] = *str;
			cmd_len++;
			str++;
			at->buf_pos++;
		} else {
			cmd_len++;
			at->buf[at->buf_pos] = '\0';
			at->state = state;
			break;
		}
	}
	net_buf_pull(buf, cmd_len);

	if (at->buf_pos == at->buf_max_len) {
		return -ENOBUFS;
	}

	return 0;
}

static void reset_buffer(struct at_client *at)
{
	memset(at->buf, 0, at->buf_max_len);
	at->buf_pos = 0;
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
	} else if (isalpha(*buf->data)) {
		at->state = AT_STATE_GET_RESULT_STRING;
		return 0;
	}

	return -ENODATA;
}

static int at_state_get_cmd_string(struct at_client *at, struct net_buf *buf)
{
	return get_response_string(at, buf, ':', AT_STATE_PROCESS_CMD);
}

static int at_state_process_cmd(struct at_client *at, struct net_buf *buf)
{
	if (at->resp) {
		at->resp(at, buf);
		return 0;
	}
	at->state = AT_STATE_UNSOLICITED_CMD;
	return 0;
}

static int at_state_get_result_string(struct at_client *at, struct net_buf *buf)
{
	return get_response_string(at, buf, '\r', AT_STATE_PROCESS_RESULT);
}

static int at_state_process_result(struct at_client *at, struct net_buf *buf)
{
	enum at_result result;

	if (at_parse_result(at->buf, buf, &result) == 0) {
		if (at->finish) {
			at->finish(at, buf, result);
		}
	}

	return 0;
}

static int at_state_unsolicited_cmd(struct at_client *at, struct net_buf *buf)
{
	/* TODO return error temporarily*/
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
			return ret;
		}
	}

	return 0;
}

static int cmd_start(struct at_client *at, struct net_buf *buf,
		     const char *prefix, parse_val_t func)
{
	if (str_has_prefix(at->buf, prefix) < 0) {
		at->state = AT_STATE_UNSOLICITED_CMD;
		return -ENODATA;
	}

	reset_buffer(at);
	at->cmd_state = CMD_GET_VALUE;
	return 0;
}

static int cmd_get_value(struct at_client *at, struct net_buf *buf,
			 const char *prefix, parse_val_t func)
{
	return get_cmd_value(at, buf, '\r', CMD_PROCESS_VALUE);
}

static int cmd_process_value(struct at_client *at, struct net_buf *buf,
			     const char *prefix, parse_val_t func)
{
	int ret;

	ret = func(at);
	at->cmd_state = CMD_STATE_END_LF;

	return ret;
}

static int cmd_state_end_lf(struct at_client *at, struct net_buf *buf,
			    const char *prefix, parse_val_t func)
{
	int err;

	err = at_check_byte(buf, '\n');
	if (err < 0) {
		return err;
	}

	at->cmd_state = CMD_START;
	at->state = AT_STATE_START;
	return 0;
}

/* The order of handler function should match the enum at_cmd_state */
static handle_cmd_input_t cmd_parser_cb[] = {
	cmd_start, /* CMD_START */
	cmd_get_value, /* CMD_GET_VALUE */
	cmd_process_value, /* CMD_PROCESS_VALUE */
	cmd_state_end_lf /* CMD_STATE_END_LF */
};

int at_parse_cmd_input(struct at_client *at, struct net_buf *buf,
		       const char *prefix, parse_val_t func)
{
	int ret;

	while (buf->len) {
		if (at->cmd_state < CMD_START ||
		    at->cmd_state >= CMD_STATE_END) {
			return -EINVAL;
		}
		ret = cmd_parser_cb[at->cmd_state](at, buf, prefix, func);
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

void at_register(struct at_client *at, at_resp_cb_t resp, at_finish_cb_t finish)
{
	at->resp = resp;
	at->finish = finish;
	at->state = AT_STATE_START;
}
