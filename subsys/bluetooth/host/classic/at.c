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

typedef bool (*pull_char_t)(uint8_t *c, void *data);

static void pull_chars(struct net_buf_simple *buf, pull_char_t cb, void *data)
{
	while (buf->len > 0) {
		if (!cb(buf->data, data)) {
			break;
		}
		net_buf_simple_pull_u8(buf);
	}
}

struct at_comma_data {
	bool found;
};

static bool pull_comma(uint8_t *c, void *data)
{
	struct at_comma_data *comma_data = data;

	if (comma_data->found) {
		return false;
	}

	comma_data->found = *c == ',';
	return comma_data->found;
}

static void next_list(struct at_client *at)
{
	struct at_comma_data data;

	data.found = false;

	pull_chars(&at->rsp_buf, pull_comma, &data);
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

static bool pull_space(uint8_t *c, void *data)
{
	return *c == ' ';
}

static void skip_space(struct at_client *at)
{
	pull_chars(&at->rsp_buf, pull_space, NULL);
}

struct at_number_data {
	uint32_t val;
	bool found;
};

static bool pull_number(uint8_t *c, void *data)
{
	struct at_number_data *num_data = data;

	if (isdigit((unsigned char)(*c)) == 0) {
		return false;
	}

	num_data->found = true;
	num_data->val = num_data->val * 10 + *c - (uint8_t)'0';
	return true;
}

int at_get_number(struct at_client *at, uint32_t *val)
{
	struct at_number_data data;

	skip_space(at);

	data.found = false;
	data.val = 0;

	pull_chars(&at->rsp_buf, pull_number, &data);

	if (!data.found) {
		return -ENODATA;
	}

	*val = data.val;

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
	/* Although the valid data is buf->data[0:buf->len], at->rsp_buf.len is set to 0 here.
	 * This is because it is unclear how much data needs to be processed.
	 * The value at->rsp_buf.len will be updated later when preprocess the received AT
	 * responses or unsolicited response code.
	 */
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

struct has_next_list_data {
	bool found;
};

static bool at_has_next_list_cb(uint8_t *c, void *data)
{
	struct has_next_list_data *list_data = data;

	list_data->found = *c != ')';
	return false;
}

bool at_has_next_list(struct at_client *at)
{
	struct has_next_list_data data;

	data.found = false;

	pull_chars(&at->rsp_buf, at_has_next_list_cb, &data);

	return data.found;
}

struct open_list_data {
	bool found;
};

static bool at_open_list_cb(uint8_t *c, void *data)
{
	struct open_list_data *list_data = data;

	if (list_data->found) {
		return false;
	}

	list_data->found = *c == '(';
	return list_data->found;
}

int at_open_list(struct at_client *at)
{
	struct open_list_data data;

	skip_space(at);

	data.found = false;

	pull_chars(&at->rsp_buf, at_open_list_cb, &data);

	if (!data.found) {
		return -ENODATA;
	}

	return 0;
}

struct close_list_data {
	bool found;
};

static bool at_close_list_cb(uint8_t *c, void *data)
{
	struct close_list_data *list_data = data;

	if (list_data->found) {
		return false;
	}

	list_data->found = *c == ')';
	return list_data->found;
}

int at_close_list(struct at_client *at)
{
	struct close_list_data data;

	skip_space(at);

	data.found = false;

	pull_chars(&at->rsp_buf, at_close_list_cb, &data);

	if (!data.found) {
		return -ENODATA;
	}
	next_list(at);

	return 0;
}

struct get_string_data {
	uint8_t *start;
	uint8_t *end;
};

static bool at_get_string_cb(uint8_t *c, void *data)
{
	struct get_string_data *str_data = data;

	if ((str_data->start != NULL) && (str_data->end != NULL)) {
		return false;
	}

	if (str_data->start == NULL) {
		if (*c != '"') {
			return false;
		}
		str_data->start = c + 1;
		return true;
	}

	if (*c == '"') {
		str_data->end = c;
	}

	return true;
}

int at_list_get_string(struct at_client *at, char *name, uint8_t len)
{
	struct get_string_data data;
	size_t str_len;

	skip_space(at);

	data.start = NULL;
	data.end = NULL;

	pull_chars(&at->rsp_buf, at_get_string_cb, &data);

	if (data.start >= data.end) {
		return -ENODATA;
	}

	str_len = data.end - data.start;
	if (str_len > (len - 1)) {
		return -ENOMEM;
	}

	memcpy(name, data.start, str_len);
	name[str_len] = '\0';

	skip_space(at);
	next_list(at);

	return 0;
}

struct get_range_connector_data {
	bool found;
};

static bool at_get_range_connector_cb(uint8_t *c, void *data)
{
	struct get_range_connector_data *connector_data = data;

	if (connector_data->found) {
		return false;
	}

	connector_data->found = *c == '-';

	return connector_data->found;
}

int at_list_get_range(struct at_client *at, uint32_t *min, uint32_t *max)
{
	struct get_range_connector_data data;
	uint32_t low, high;
	int err;

	err = at_get_number(at, &low);
	if (err < 0) {
		return err;
	}

	data.found = false;
	pull_chars(&at->rsp_buf, at_get_range_connector_cb, &data);

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

const char *at_get_string(struct at_client *at)
{
	struct get_string_data data;

	skip_space(at);

	data.start = NULL;
	data.end = NULL;

	pull_chars(&at->rsp_buf, at_get_string_cb, &data);

	if (data.start >= data.end) {
		return NULL;
	}

	data.end[0] = '\0';

	skip_space(at);
	next_list(at);

	return (const char *)data.start;
}

struct get_raw_string_data {
	uint8_t *start;
	uint8_t *end;
};

static bool at_get_raw_string_cb(uint8_t *c, void *data)
{
	struct get_raw_string_data *str_data = data;

	if ((*c == ',') || (*c == ')')) {
		return false;
	}

	if (str_data->start == NULL) {
		str_data->start = c;
	}
	str_data->end = c;
	return true;
}

const char *at_get_raw_string(struct at_client *at, size_t *string_len)
{
	struct get_raw_string_data data;

	skip_space(at);

	data.start = NULL;
	data.end = NULL;

	pull_chars(&at->rsp_buf, at_get_raw_string_cb, &data);

	if (data.start > data.end) {
		return NULL;
	}

	if (string_len != NULL) {
		*string_len = data.end - data.start + 1;
	}

	skip_space(at);
	next_list(at);

	return (const char *)data.start;
}
