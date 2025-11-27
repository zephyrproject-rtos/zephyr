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
	char c = ',';

	if (at->rsp_buf.len < sizeof(c)) {
		return;
	}

	if (at->rsp_buf.data[0] == (uint8_t)c) {
		net_buf_simple_pull(&at->rsp_buf, sizeof(c));
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
	char space = ' ';

	while (at->rsp_buf.len > 0) {
		if (at->rsp_buf.data[0] != (uint8_t)space) {
			break;
		}
		net_buf_simple_pull(&at->rsp_buf, sizeof(space));
	}
}

int at_get_number(struct at_client *at, uint32_t *val)
{
	skip_space(at);

	if ((at->rsp_buf.len == 0) || (isdigit((unsigned char)at->rsp_buf.data[0]) == 0)) {
		return -ENODATA;
	}

	*val = 0;

	while (at->rsp_buf.len > 0) {
		if (isdigit((unsigned char)at->rsp_buf.data[0]) == 0) {
			break;
		}

		*val = *val * 10 + at->rsp_buf.data[0] - (uint8_t)'0';
		net_buf_simple_pull_u8(&at->rsp_buf);
	}

	next_list(at);
	return 0;
}

static bool str_has_prefix(struct at_client *at, const char *prefix)
{
	size_t len = strlen(prefix);

	if (at->rsp_buf.len < len) {
		return false;
	}

	if (strncmp(at->rsp_buf.data, prefix, len) != 0) {
		return false;
	}

	return true;
}

static int at_parse_result(struct at_client *at, struct net_buf *buf,
			   enum at_result *result)
{
	char *ok = "OK";
	char *error = "ERROR";
	size_t ok_len = strlen(ok);
	size_t error_len = strlen(error);

	/* Map the result and check for end lf */
	if ((at->rsp_buf.len >= ok_len) && (strncmp(at->rsp_buf.data, ok, ok_len) == 0) &&
	    (at_check_byte(buf, '\n') == 0)) {
		*result = AT_RESULT_OK;
		return 0;
	}

	if ((at->rsp_buf.len >= error_len) && (strncmp(at->rsp_buf.data, error, error_len) == 0) &&
	    (at_check_byte(buf, '\n') == 0)) {
		*result = AT_RESULT_ERROR;
		return 0;
	}

	return -ENOMSG;
}

static void init_rsp_buffer(struct at_client *at, struct net_buf *buf)
{
	at->rsp_buf.data = buf->data;
	at->rsp_buf.__buf = buf->data;
	at->rsp_buf.size = buf->len;
	at->rsp_buf.len = 0;
}

static int get_cmd_value(struct at_client *at, struct net_buf *buf,
			 char stop_byte, enum at_cmd_state cmd_state)
{
	if (at->rsp_buf.len == 0) {
		init_rsp_buffer(at, buf);
	}

	while (buf->len > 0) {
		if (buf->data[0] == stop_byte) {
			at->cmd_state = cmd_state;
			net_buf_pull_u8(buf);
			break;
		}
		net_buf_simple_add(&at->rsp_buf, sizeof(uint8_t));
		net_buf_pull_u8(buf);
	}

	if (at->rsp_buf.len == 0) {
		return -ENODATA;
	}

	return 0;
}

static bool is_stop_byte(char target, char *stop_string)
{
	return (strchr(stop_string, target) != NULL);
}

static bool is_vgm_or_vgs(struct at_client *at)
{
	if (at->rsp_buf.len == 0) {
		return false;
	}

	if (!strncmp(at->rsp_buf.data, "VGM", strlen("VGM"))) {
		return true;
	}

	if (!strncmp(at->rsp_buf.data, "VGS", strlen("VGS"))) {
		return true;
	}

	return false;
}

static int get_response_string(struct at_client *at, struct net_buf *buf, char *stop_string,
			       enum at_state state)
{
	if (at->rsp_buf.len == 0) {
		init_rsp_buffer(at, buf);
	}

	while (buf->len > 0) {
		if (is_stop_byte(buf->data[0], stop_string)) {
			char stop_byte = buf->data[0];

			at->state = state;
			if ((stop_byte == '=') && !is_vgm_or_vgs(at)) {
				return -EINVAL;
			}
			net_buf_pull_u8(buf);
			break;
		}
		net_buf_simple_add(&at->rsp_buf, sizeof(uint8_t));
		net_buf_pull_u8(buf);
	}

	if (at->rsp_buf.len == 0) {
		return -ENODATA;
	}

	return 0;
}

static void reset_buffer(struct at_client *at)
{
	net_buf_simple_init_with_data(&at->rsp_buf, NULL, 0);
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
	return get_response_string(at, buf, ":=", AT_STATE_PROCESS_CMD);
}

static bool is_cmer(struct at_client *at)
{
	char *cmer = "CME ERROR";
	size_t len = strlen(cmer);

	if (at->rsp_buf.len < len) {
		return false;
	}

	if (strncmp(at->rsp_buf.data, cmer, len) == 0) {
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
	return get_response_string(at, buf, "\r", AT_STATE_PROCESS_RESULT);
}

static bool is_ring(struct at_client *at)
{
	char *ring = "RING";
	size_t len = strlen(ring);

	if (at->rsp_buf.len < len) {
		return false;
	}

	if (strncmp(at->rsp_buf.data, ring, len) == 0) {
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

	if (at_parse_result(at, buf, &result) == 0) {
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
	if (!str_has_prefix(at, prefix)) {
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

bool at_has_next_list(struct at_client *at)
{
	if (at->rsp_buf.len == 0) {
		return false;
	}

	if (at->rsp_buf.data[0] == ')') {
		return false;
	}

	return true;
}

int at_open_list(struct at_client *at)
{
	skip_space(at);

	/* The list shall start with '(' open parenthesis */
	if ((at->rsp_buf.len == 0) || (at->rsp_buf.data[0] != '(')) {
		return -ENODATA;
	}

	net_buf_simple_pull_u8(&at->rsp_buf);

	return 0;
}

int at_close_list(struct at_client *at)
{
	skip_space(at);

	if ((at->rsp_buf.len == 0) || (at->rsp_buf.data[0] != ')')) {
		return -ENODATA;
	}

	net_buf_simple_pull_u8(&at->rsp_buf);

	next_list(at);

	return 0;
}

int at_list_get_string(struct at_client *at, char *name, uint8_t len)
{
	uint8_t *start;
	size_t str_len;

	skip_space(at);

	if ((at->rsp_buf.len == 0) || (at->rsp_buf.data[0] != '"')) {
		return -ENODATA;
	}

	net_buf_simple_pull_u8(&at->rsp_buf);

	start = at->rsp_buf.data;

	while ((at->rsp_buf.len > 0) && (at->rsp_buf.data[0] != '"')) {
		net_buf_simple_pull_u8(&at->rsp_buf);
	}

	if ((at->rsp_buf.len == 0) || (at->rsp_buf.data[0] != '"')) {
		return -ENODATA;
	}

	str_len = at->rsp_buf.data - start;
	if (str_len > (len - 1)) {
		return -ENOMEM;
	}

	/* Pull char '"' from the buffer */
	net_buf_simple_pull_u8(&at->rsp_buf);

	memcpy(name, start, str_len);
	name[str_len] = '\0';

	skip_space(at);
	next_list(at);

	return 0;
}

int at_list_get_range(struct at_client *at, uint32_t *min, uint32_t *max)
{
	uint32_t low, high;
	int err;

	err = at_get_number(at, &low);
	if (err < 0) {
		return err;
	}

	if ((at->rsp_buf.len > 0) && (at->rsp_buf.data[0] == '-')) {
		net_buf_simple_pull_u8(&at->rsp_buf);
		goto out;
	}

	if ((at->rsp_buf.len == 0) || (isdigit((unsigned char)at->rsp_buf.data[0]) == 0)) {
		return -ENODATA;
	}
out:
	err = at_get_number(at, &high);
	if (err < 0) {
		return err;
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

char *at_get_string(struct at_client *at)
{
	char *string;

	skip_space(at);

	if ((at->rsp_buf.len == 0) || (at->rsp_buf.data[0] != '"')) {
		return NULL;
	}

	net_buf_simple_pull_u8(&at->rsp_buf);

	string = (char *)at->rsp_buf.data;

	while ((at->rsp_buf.len > 0) && (at->rsp_buf.data[0] != '"')) {
		net_buf_simple_pull_u8(&at->rsp_buf);
	}

	if ((at->rsp_buf.len == 0) || (at->rsp_buf.data[0] != '"')) {
		return NULL;
	}

	at->rsp_buf.data[0] = '\0';
	net_buf_simple_pull_u8(&at->rsp_buf);

	skip_space(at);
	next_list(at);

	return string;
}

char *at_get_raw_string(struct at_client *at, size_t *string_len)
{
	char *string;

	skip_space(at);

	if (at->rsp_buf.len == 0) {
		return NULL;
	}

	string = (char *)at->rsp_buf.data;

	while ((at->rsp_buf.len > 0) && (at->rsp_buf.data[0] != ',') &&
	       (at->rsp_buf.data[0] != ')')) {
		net_buf_simple_pull_u8(&at->rsp_buf);
	}

	if (string_len != NULL) {
		*string_len = (char *)at->rsp_buf.data - string;
	}

	skip_space(at);
	next_list(at);

	return string;
}
