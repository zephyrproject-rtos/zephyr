/*
 * Copyright 2024 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/parse_vcard.h>

#define BEGIN_VCARD_STR "BEGIN:VCARD"
#define END_VCARD_STR   "END:VCARD"
#define VCARD_N_STR     "N"
#define VCARD_FN_STR    "FN"
#define VCARD_TEL_STR   "TEL"

enum {
	P_VCARD_STATE_BEGIN,
	P_VCARD_STATE_VALUE,
};

struct vcard_string_type {
	uint8_t type;
	uint8_t len;
	char *str;
};

static const char list_cardhdl[] = "card handle=\"";
static const char list_name[] = "name=\"";

static const struct vcard_string_type str_cmp[] = {
	{VCARD_TYPE_VERSION, 7, "VERSION"},
	{VCARD_TYPE_FN, 2, "FN"},
	{VCARD_TYPE_N, 1, "N"},
	{VCARD_TYPE_TEL, 3, "TEL"},
	{VCARD_TYPE_TZ, 2, "TZ"},
	{VCARD_TYPE_TS, 20, "X-IRMC-CALL-DATETIME"},
};

static uint16_t parse_vcard_find_line(uint8_t *data, uint16_t len)
{
	uint16_t i = 0, next_pos;

	while (i < len) {
		if (data[i] == '\x0d' || data[i] == '\x0a') {
			data[i] = '\0';
			next_pos = i + 1;
			if (((i + 1) < len) && (data[i + 1] == '\x0d' || data[i + 1] == '\x0a')) {
				data[i + 1] = '\0';
				return (next_pos + 1);
			} else {
				return next_pos;
			}
		}

		i++;
	}

	return 0;
}

static uint8_t parse_vcard_add_one_result(struct parse_vcard_result *result, uint8_t max_result,
					  uint8_t result_cnt, uint8_t type, uint8_t *data)
{
	if (result_cnt >= max_result) {
		return 0;
	}

	result[result_cnt].type = type;
	result[result_cnt].len = strlen(data);
	result[result_cnt].data = data;
	return 1;
}

static uint16_t pbap_parse_one_vcard(uint8_t *data, uint16_t len, struct parse_vcard_result *result,
				     uint8_t *result_size)
{
	uint16_t cur_pos, f_pos;
	uint8_t result_cnt, state, i;

	cur_pos = 0;
	f_pos = 0;
	result_cnt = 0;
	state = P_VCARD_STATE_BEGIN;

	while (cur_pos < len) {
		f_pos = parse_vcard_find_line(&data[cur_pos], (len - cur_pos));
		if (f_pos == 0) {
			/* Parse end */
			cur_pos = len;
			break;
		}

		if ((state == P_VCARD_STATE_BEGIN) &&
		    (strncmp(BEGIN_VCARD_STR, &data[cur_pos], strlen(BEGIN_VCARD_STR)) == 0)) {
			state = P_VCARD_STATE_VALUE;
		} else {
			if (strcmp(END_VCARD_STR, &data[cur_pos]) == 0) {
				/* Parse one vcard finish */
				cur_pos += f_pos;
				break;
			} else {
				for (i = 0; i < ARRAY_SIZE(str_cmp); i++) {
					if ((!strncmp(str_cmp[i].str, &data[cur_pos],
						      str_cmp[i].len)) &&
					    (data[cur_pos + str_cmp[i].len] == ';' ||
					     data[cur_pos + str_cmp[i].len] == ':')) {
						result_cnt += parse_vcard_add_one_result(
							result, *result_size, result_cnt,
							str_cmp[i].type, &data[cur_pos]);
						break;
					}
				}
			}
		}

		cur_pos += f_pos;
	}

	*result_size = result_cnt;
	return cur_pos;
}

uint16_t pbap_parse_copy_to_cached(uint8_t *data, uint16_t len, struct parse_cached_data *vcached)
{
	uint16_t copy_len;

	copy_len = PBAP_VCARD_MAX_CACHED - vcached->cached_len;
	copy_len = (copy_len > len) ? len : copy_len;

	if (copy_len) {
		memcpy(&vcached->cached[vcached->cached_len], data, copy_len);
		vcached->cached_len += copy_len;
	}

	return copy_len;
}

void pbap_parse_vcached_move(struct parse_cached_data *vcached)
{
	uint16_t i, len;

	if (vcached->parse_pos) {
		if (vcached->parse_pos >= vcached->cached_len) {
			vcached->parse_pos = 0;
			vcached->cached_len = 0;
		} else {
			len = vcached->cached_len - vcached->parse_pos;
			for (i = 0; i < len; i++) {
				vcached->cached[i] = vcached->cached[i + vcached->parse_pos];
			}
			vcached->parse_pos = 0;
			vcached->cached_len = len;
		}
	}
}

static uint16_t parse_line_start_pos(uint8_t *data, uint16_t len)
{
	uint16_t i = 0;

	while (i < len) {
		if (data[i] != '\x0d' && data[i] != '\x0a') {
			return i;
		}

		i++;
	}

	return 0;
}

static uint16_t parse_line_end_pos(uint8_t *data, uint16_t len)
{
	uint16_t i = 0, next_pos;

	while (i < len) {
		if (data[i] == '\x0d' || data[i] == '\x0a') {
			next_pos = i + 1;
			if ((next_pos < len) &&
			    (data[next_pos] == '\x0d' || data[next_pos] == '\x0a')) {
				return next_pos;
			} else {
				return i;
			}
		}

		i++;
	}

	return 0;
}

static uint16_t parse_find_one_vcard(struct parse_cached_data *vcached)
{
	uint16_t parse_pos, line_start_pos, line_end_pos;
	uint8_t find_start = 0;

	parse_pos = vcached->parse_pos;

	while (parse_pos < vcached->cached_len) {
		line_start_pos = parse_line_start_pos(&vcached->cached[parse_pos],
						      (vcached->cached_len - parse_pos));
		line_end_pos = parse_line_end_pos(&vcached->cached[parse_pos],
						  (vcached->cached_len - parse_pos));

		if (!line_end_pos) {
			return 0;
		}

		line_start_pos += parse_pos;
		line_end_pos += parse_pos;

		if (!find_start) {
			if (((line_end_pos - line_start_pos) >= strlen(BEGIN_VCARD_STR)) &&
			    strncmp(BEGIN_VCARD_STR, &vcached->cached[line_start_pos],
				    strlen(BEGIN_VCARD_STR)) == 0) {
				find_start = 1;
			} else {
				vcached->parse_pos = (line_end_pos + 1);
			}
		} else {
			if (strncmp(END_VCARD_STR, &vcached->cached[line_start_pos],
				    strlen(END_VCARD_STR)) == 0) {
				return line_end_pos;
			}
		}

		parse_pos = line_end_pos + 1;
	}

	return 0;
}

void pbap_parse_vcached_vcard(struct parse_cached_data *vcached, struct parse_vcard_result *result,
			      uint8_t *result_size)
{
	uint16_t vcard_end_pos;

	vcard_end_pos = parse_find_one_vcard(vcached);
	if (vcard_end_pos) {
		pbap_parse_one_vcard(&vcached->cached[vcached->parse_pos],
				     (vcard_end_pos - vcached->parse_pos + 1), result, result_size);
		vcached->parse_pos = vcard_end_pos + 1;
	} else {
		*result_size = 0;
	}
}

static uint16_t parse_find_one_list(struct parse_cached_data *vcached)
{
	uint16_t line_end_pos = 0, i;
	uint8_t find_start = 0;

	i = vcached->parse_pos;
	while (i < vcached->cached_len) {
		if (!find_start) {
			vcached->parse_pos = i;
			if (vcached->cached[i] == '<') {
				find_start = 1;
			}
		} else {
			if (vcached->cached[i] == '>') {
				line_end_pos = i;
				break;
			}
		}
		i++;
	}

	return line_end_pos;
}

static void pbap_parse_one_list(uint8_t *data, uint16_t len, struct parse_vcard_result *result,
				uint8_t *result_cnt, uint8_t *result_size)
{
	uint16_t i, j, pos;

	for (i = 0; i < len; i++) {
		if ((data[i] == list_cardhdl[0]) &&
		    (!strncmp(&data[i], list_cardhdl, strlen(list_cardhdl)))) {
			pos = i + strlen(list_cardhdl);
			for (j = pos; j < len; j++) {
				if ((data[j] == '\"') && (j != pos)) {
					data[j] = '\0';
					(*result_cnt) += parse_vcard_add_one_result(
						result, *result_size, (*result_cnt),
						VCARD_TYPE_CARD_HDL, &data[pos]);
					i = j;
					break;
				}
			}
		}

		if ((*result_cnt) >= (*result_size)) {
			break;
		}

		if ((data[i] == list_name[0]) &&
		    (!strncmp(&data[i], list_name, strlen(list_name)))) {
			pos = i + strlen(list_name);
			for (j = pos; j < len; j++) {
				if ((data[j] == '\"') && (j != pos)) {
					data[j] = '\0';
					(*result_cnt) += parse_vcard_add_one_result(
						result, *result_size, (*result_cnt), VCARD_TYPE_N,
						&data[pos]);
					i = j;
					break;
				}
			}
		}

		if ((*result_cnt) >= (*result_size)) {
			break;
		}
	}
}

void pbap_parse_vcached_list(struct parse_cached_data *vcached, struct parse_vcard_result *result,
			     uint8_t *result_size)
{
	uint16_t line_end_pos;
	uint8_t parse_line_cnt = 0, result_cnt = 0;
	uint8_t parse_max_line = (*result_size) / 2;

	while (parse_line_cnt < parse_max_line) {
		line_end_pos = parse_find_one_list(vcached);
		if (line_end_pos) {
			pbap_parse_one_list(&vcached->cached[vcached->parse_pos],
					    (line_end_pos - vcached->parse_pos + 1), result,
					    &result_cnt, result_size);
			vcached->parse_pos = line_end_pos + 1;
			parse_line_cnt++;
		} else {
			break;
		}
	}

	*result_size = result_cnt;
}
