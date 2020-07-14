/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Copyright (c) 2015, Yanzi Networks AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Original Authors:
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

/*
 * Zephyr Contribution by Michael Scott <michael.scott@linaro.org>
 * - Zephyr code style changes / code cleanup
 * - Move to Zephyr APIs where possible
 * - Convert to Zephyr int/uint types
 * - Remove engine dependency (replace with writer context)
 * - Add write int64 function
 */

/*
 * TODO:
 * - Type cleanups
 * - Cleanup integer parsing
 */

#define LOG_MODULE_NAME net_lwm2m_plain_text
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lwm2m_object.h"
#include "lwm2m_rw_plain_text.h"
#include "lwm2m_engine.h"

/* some temporary buffer space for format conversions */
static char pt_buffer[42]; /* can handle float64 format */

size_t plain_text_put_format(struct lwm2m_output_context *out,
			     const char *format, ...)
{
	va_list vargs;
	int n;

	va_start(vargs, format);
	n = vsnprintk(pt_buffer, sizeof(pt_buffer), format, vargs);
	va_end(vargs);
	if (n < 0) {
		/* TODO: generate an error? */
		return 0;
	}

	n = strlen(pt_buffer);
	if (buf_append(CPKT_BUF_WRITE(out->out_cpkt), pt_buffer, n) < 0) {
		return 0;
	}

	return (size_t)n;
}

static size_t put_s32(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, int32_t value)
{
	return plain_text_put_format(out, "%d", value);
}

static size_t put_s8(struct lwm2m_output_context *out,
		     struct lwm2m_obj_path *path, int8_t value)
{
	return plain_text_put_format(out, "%d", value);
}

static size_t put_s16(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, int16_t value)
{
	return plain_text_put_format(out, "%d", value);
}

static size_t put_s64(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, int64_t value)
{
	return plain_text_put_format(out, "%lld", value);
}

size_t plain_text_put_float32fix(struct lwm2m_output_context *out,
				 struct lwm2m_obj_path *path,
				 float32_value_t *value)
{
	size_t len;
	char buf[sizeof("000000")];

	/* value of 123 -> "000123" -- ignore sign */
	len = snprintk(buf, sizeof(buf), "%06d", abs(value->val2));
	if (len != 6U) {
		strcpy(buf, "0");
	} else {
		/* clear ending zeroes, but leave 1 if needed */
		while (len > 1U && buf[len - 1] == '0') {
			buf[--len] = '\0';
		}
	}

	return plain_text_put_format(out, "%s%d.%s",
				     /* handle negative val2 when val1 is 0 */
				     (value->val1 == 0 && value->val2 < 0) ?
						"-" : "",
				     value->val1, buf);
}

size_t plain_text_put_float64fix(struct lwm2m_output_context *out,
				 struct lwm2m_obj_path *path,
				 float64_value_t *value)
{
	size_t len;
	char buf[sizeof("000000000")];

	/* value of 123 -> "000000123" -- ignore sign */
	len = snprintf(buf, sizeof(buf), "%09lld",
		       (long long int)abs(value->val2));
	if (len != 9U) {
		strcpy(buf, "0");
	} else {
		/* clear ending zeroes, but leave 1 if needed */
		while (len > 1U && buf[len - 1] == '0') {
			buf[--len] = '\0';
		}
	}

	return plain_text_put_format(out, "%s%lld.%s",
				     /* handle negative val2 when val1 is 0 */
				     (value->val1 == 0 && value->val2 < 0) ?
						"-" : "",
				     value->val1, buf);
}

static size_t put_string(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path,
			 char *buf, size_t buflen)
{
	if (buf_append(CPKT_BUF_WRITE(out->out_cpkt), buf, buflen) < 0) {
		return 0;
	}

	return buflen;
}

static size_t put_bool(struct lwm2m_output_context *out,
		       struct lwm2m_obj_path *path,
		       bool value)
{
	if (value) {
		return plain_text_put_format(out, "%u", 1);
	} else {
		return plain_text_put_format(out, "%u", 0);
	}
}

static size_t put_objlnk(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path,
			 struct lwm2m_objlnk *value)
{
	return plain_text_put_format(out, "%u:%u", value->obj_id,
				     value->obj_inst);
}

static int get_length_left(struct lwm2m_input_context *in)
{
	return in->in_cpkt->offset - in->offset;
}

static size_t plain_text_read_number(struct lwm2m_input_context *in,
				     int64_t *value1,
				     int64_t *value2,
				     bool accept_sign, bool accept_dot)
{
	int64_t *counter = value1;
	int i = 0;
	bool neg = false;
	bool dot_found = false;
	uint8_t tmp;

	/* initialize values to 0 */
	*value1 = 0;
	if (value2) {
		*value2 = 0;
	}

	while (in->offset < in->in_cpkt->offset) {
		if (buf_read_u8(&tmp, CPKT_BUF_READ(in->in_cpkt),
				&in->offset) < 0) {
			break;
		}

		if (tmp == '-' && accept_sign && i == 0) {
			neg = true;
		} else if (tmp == '.' && i > 0 && accept_dot && !dot_found &&
			   value2) {
			dot_found = true;
			counter = value2;
		} else if (isdigit(tmp)) {
			*counter = *counter * 10 + (tmp - '0');
		} else {
			/* anything else stop reading */
			in->offset--;
			break;
		}

		i++;
	}

	if (neg) {
		*value1 = -*value1;
	}

	return i;
}

static size_t get_s32(struct lwm2m_input_context *in, int32_t *value)
{
	int64_t tmp = 0;
	size_t len = 0;

	len = plain_text_read_number(in, &tmp, NULL, true, false);
	if (len > 0) {
		*value = (int32_t)tmp;
	}

	return len;
}

static size_t get_s64(struct lwm2m_input_context *in, int64_t *value)
{
	return plain_text_read_number(in, value, NULL, true, false);
}

static size_t get_string(struct lwm2m_input_context *in,
			 uint8_t *value, size_t buflen)
{
	uint16_t in_len = get_length_left(in);

	if (in_len > buflen) {
		/* TODO: generate warning? */
		in_len = buflen - 1;
	}

	if (buf_read(value, in_len, CPKT_BUF_READ(in->in_cpkt),
		     &in->offset) < 0) {
		value[0] = '\0';
		return 0;
	}

	value[in_len] = '\0';
	return (size_t)in_len;
}

static size_t get_float32fix(struct lwm2m_input_context *in,
			     float32_value_t *value)
{
	int64_t tmp1, tmp2;
	size_t len = 0;

	len = plain_text_read_number(in, &tmp1, &tmp2, true, true);
	if (len > 0) {
		value->val1 = (int32_t)tmp1;
		value->val2 = (int32_t)tmp2;
	}

	return len;
}

static size_t get_float64fix(struct lwm2m_input_context *in,
			     float64_value_t *value)
{
	return plain_text_read_number(in, &value->val1, &value->val2,
				      true, true);
}

static size_t get_bool(struct lwm2m_input_context *in,
		       bool *value)
{
	uint8_t tmp;

	if (buf_read_u8(&tmp, CPKT_BUF_READ(in->in_cpkt), &in->offset) < 0) {
		return 0;
	}

	if (tmp == '1' || tmp == '0') {
		*value = (tmp == '1') ? true : false;
		return 1;
	}

	return 0;
}

static size_t get_opaque(struct lwm2m_input_context *in,
			 uint8_t *value, size_t buflen, bool *last_block)
{
	in->opaque_len = get_length_left(in);
	return lwm2m_engine_get_opaque_more(in, value, buflen, last_block);
}


static size_t get_objlnk(struct lwm2m_input_context *in,
			 struct lwm2m_objlnk *value)
{
	int64_t tmp;
	size_t len;

	len = plain_text_read_number(in, &tmp, NULL, false, false);
	value->obj_id = (uint16_t)tmp;

	/* Skip ':' delimeter. */
	in->offset++;
	len++;

	len += plain_text_read_number(in, &tmp, NULL, false, false);
	value->obj_inst = (uint16_t)tmp;

	return len;
}

const struct lwm2m_writer plain_text_writer = {
	.put_s8 = put_s8,
	.put_s16 = put_s16,
	.put_s32 = put_s32,
	.put_s64 = put_s64,
	.put_string = put_string,
	.put_float32fix = plain_text_put_float32fix,
	.put_float64fix = plain_text_put_float64fix,
	.put_bool = put_bool,
	.put_objlnk = put_objlnk,
};

const struct lwm2m_reader plain_text_reader = {
	.get_s32 = get_s32,
	.get_s64 = get_s64,
	.get_string = get_string,
	.get_float32fix = get_float32fix,
	.get_float64fix = get_float64fix,
	.get_bool = get_bool,
	.get_opaque = get_opaque,
	.get_objlnk = get_objlnk,
};

int do_read_op_plain_text(struct lwm2m_message *msg, int content_format)
{
	/* Plain text can only return single resource */
	if (msg->path.level != 3U) {
		return -EPERM; /* NOT_ALLOWED */
	}

	return lwm2m_perform_read_op(msg, content_format);
}

int do_write_op_plain_text(struct lwm2m_message *msg)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_engine_obj_field *obj_field;
	struct lwm2m_engine_res *res = NULL;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	int ret, i;
	uint8_t created = 0U;

	ret = lwm2m_get_or_create_engine_obj(msg, &obj_inst, &created);
	if (ret < 0) {
		return ret;
	}

	obj_field = lwm2m_get_engine_obj_field(obj_inst->obj, msg->path.res_id);
	if (!obj_field) {
		return -ENOENT;
	}

	if (!LWM2M_HAS_PERM(obj_field, LWM2M_PERM_W)) {
		return -EPERM;
	}

	if (!obj_inst->resources || obj_inst->resource_count == 0U) {
		return -EINVAL;
	}

	for (i = 0; i < obj_inst->resource_count; i++) {
		if (obj_inst->resources[i].res_id == msg->path.res_id) {
			res = &obj_inst->resources[i];
			break;
		}
	}

	if (!res) {
		return -ENOENT;
	}

	for (i = 0; i < res->res_inst_count; i++) {
		if (res->res_instances[i].res_inst_id ==
		    msg->path.res_inst_id) {
			res_inst = &res->res_instances[i];
			break;
		}
	}

	if (!res_inst) {
		return -ENOENT;
	}

	if (msg->path.level < 3) {
		msg->path.level = 3U;
	}

	return lwm2m_write_handler(obj_inst, res, res_inst, obj_field, msg);
}
