/*
 * Copyright (c) 2017 Linaro Limited
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

#define SYS_LOG_DOMAIN "lib/lwm2m_plain_text"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LWM2M_LEVEL
#include <logging/sys_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lwm2m_object.h"
#include "lwm2m_rw_plain_text.h"
#include "lwm2m_engine.h"

size_t plain_text_put_float32fix(u8_t *outbuf, size_t outlen,
				 float32_value_t *value)
{
	int n, o = 0;

	if (outlen == 0) {
		return 0;
	}

	if (value->val1 < 0) {
		*outbuf++ = '-';
		outlen--;
		o = 1;
		value->val1 = -value->val1;
	}

	n = snprintf(outbuf, outlen, "%d.%d", value->val1, value->val2);

	if (n < 0 || n >= outlen) {
		return 0;
	}

	return n + o;
}

size_t plain_text_put_float64fix(u8_t *outbuf, size_t outlen,
				 float64_value_t *value)
{
	int n, o = 0;

	if (outlen == 0) {
		return 0;
	}

	if (value->val1 < 0) {
		*outbuf++ = '-';
		outlen--;
		o = 1;
		value->val1 = -value->val1;
	}

	n = snprintf(outbuf, outlen, "%lld.%lld", value->val1, value->val2);

	if (n < 0 || n >= outlen) {
		return 0;
	}

	return n + o;
}

static size_t put_s32(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, s32_t value)
{
	int len;

	len = snprintf(&out->outbuf[out->outlen], out->outsize - out->outlen,
		       "%d", value);
	if (len < 0 || len >= (out->outsize - out->outlen)) {
		return 0;
	}

	out->outlen += len;
	return (size_t)len;
}

static size_t put_s8(struct lwm2m_output_context *out,
		     struct lwm2m_obj_path *path, s8_t value)
{
	return put_s32(out, path, (s32_t)value);
}

static size_t put_s16(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, s16_t value)
{
	return put_s32(out, path, (s32_t)value);
}

static size_t put_s64(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, s64_t value)
{
	int len;

	len = snprintf(&out->outbuf[out->outlen], out->outsize - out->outlen,
		       "%lld", value);
	if (len < 0 || len >= (out->outsize - out->outlen)) {
		return 0;
	}

	out->outlen += len;
	return (size_t)len;
}

static size_t put_float32fix(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path,
			     float32_value_t *value)
{
	size_t len;

	len = plain_text_put_float32fix(&out->outbuf[out->outlen],
					out->outsize - out->outlen,
					value);
	out->outlen += len;
	return len;
}

static size_t put_float64fix(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path,
			     float64_value_t *value)
{
	size_t len;

	len = plain_text_put_float64fix(&out->outbuf[out->outlen],
					out->outsize - out->outlen,
					value);
	out->outlen += len;
	return len;
}

static size_t put_string(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path,
			 const char *value, size_t strlen)
{
	if (strlen >= (out->outsize - out->outlen)) {
		return 0;
	}

	memmove(&out->outbuf[out->outlen], value, strlen);
	out->outbuf[strlen] = '\0';
	out->outlen += strlen;
	return strlen;
}

static size_t put_bool(struct lwm2m_output_context *out,
		       struct lwm2m_obj_path *path,
		       bool value)
{
	if ((out->outsize - out->outlen) > 0) {
		if (value) {
			out->outbuf[out->outlen] = '1';
		} else {
			out->outbuf[out->outlen] = '0';
		}

		out->outlen += 1;
		return 1;
	}

	return 0;
}

static size_t get_s32(struct lwm2m_input_context *in, s32_t *value)
{
	int i;
	bool neg = false;

	*value = 0;
	for (i = 0; i < in->insize; i++) {
		if (isdigit(in->inbuf[i])) {
			*value = *value * 10 + (in->inbuf[i] - '0');
		} else if (in->inbuf[i] == '-' && i == 0) {
			neg = true;
		} else {
			break;
		}
	}

	if (neg) {
		*value = -*value;
	}

	in->last_value_len = i;
	return i;
}

static size_t get_s64(struct lwm2m_input_context *in, s64_t *value)
{
	int i;
	bool neg = false;

	*value = 0;
	for (i = 0; i < in->insize; i++) {
		if (isdigit(in->inbuf[i])) {
			*value = *value * 10 + (in->inbuf[i] - '0');
		} else if (in->inbuf[i] == '-' && i == 0) {
			neg = true;
		} else {
			break;
		}
	}

	if (neg) {
		*value = -*value;
	}

	in->last_value_len = i;
	return i;
}

static size_t get_string(struct lwm2m_input_context *in,
			 u8_t *value, size_t strlen)
{
	/* The outbuffer can't contain the full string including ending zero */
	if (strlen <= in->insize) {
		return 0;
	}

	memcpy(value, in->inbuf, in->insize);
	value[in->insize] = '\0';
	in->last_value_len = in->insize;
	return in->insize;
}

static size_t get_float32fix(struct lwm2m_input_context *in,
			     float32_value_t *value)
{
	int i;
	bool dot = false, neg = false;
	s32_t counter = 0;

	value->val1 = 0;
	value->val2 = 0;
	for (i = 0; i < in->insize; i++) {
		if (isdigit(in->inbuf[i])) {
			counter = counter * 10 + (in->inbuf[i] - '0');
		} else if (in->inbuf[i] == '-' && i == 0) {
			neg = true;
		} else if (in->inbuf[i] == '.' && !dot) {
			value->val1 = counter;
			counter = 0;
			dot = true;
		} else {
			break;
		}
	}

	if (!dot) {
		value->val1 = counter;
	} else {
		value->val2 = counter;
	}

	SYS_LOG_DBG("READ FLOATFIX: '%s'(%d) => %d.%d",
		    in->inbuf, in->insize, value->val1, value->val2);
	if (neg) {
		value->val1 = -value->val1;
	}

	in->last_value_len = i;
	return i;
}

static size_t get_float64fix(struct lwm2m_input_context *in,
			     float64_value_t *value)
{
	int i;
	bool dot = false, neg = false;
	s64_t counter = 0;

	value->val1 = 0;
	value->val2 = 0;
	for (i = 0; i < in->insize; i++) {
		if (isdigit(in->inbuf[i])) {
			counter = counter * 10 + (in->inbuf[i] - '0');
		} else if (in->inbuf[i] == '-' && i == 0) {
			neg = true;
		} else if (in->inbuf[i] == '.' && !dot) {
			value->val1 = counter;
			counter = 0;
			dot = true;
		} else {
			break;
		}
	}

	if (!dot) {
		value->val1 = counter;
	} else {
		value->val2 = counter;
	}

	SYS_LOG_DBG("READ FLOATFIX: '%s'(%d) => %lld.%lld",
		    in->inbuf, in->insize, value->val1, value->val2);
	if (neg) {
		value->val1 = -value->val1;
	}

	in->last_value_len = i;
	return i;
}

static size_t get_bool(struct lwm2m_input_context *in,
		       bool *value)
{
	if (in->insize > 0) {
		if (*in->inbuf == '1' || *in->inbuf == '0') {
			*value = (*in->inbuf == '1') ? true : false;
			in->last_value_len = 1;
			return 1;
		}
	}

	return 0;
}

const struct lwm2m_writer plain_text_writer = {
	NULL,
	NULL,
	NULL,
	NULL,
	put_s8,
	put_s16,
	put_s32,
	put_s64,
	put_string,
	put_float32fix,
	put_float64fix,
	put_bool
};

const struct lwm2m_reader plain_text_reader = {
	get_s32,
	get_s64,
	get_string,
	get_float32fix,
	get_float64fix,
	get_bool
};

int do_write_op_plain_text(struct lwm2m_engine_obj *obj,
			   struct lwm2m_engine_context *context)
{
	struct lwm2m_obj_path *path = context->path;
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_engine_obj_field *obj_field;
	struct lwm2m_engine_res_inst *res = NULL;
	int ret, i;
	u8_t created = 0;

	ret = lwm2m_get_or_create_engine_obj(context, &obj_inst, &created);
	if (ret < 0) {
		return ret;
	}

	obj_field = lwm2m_get_engine_obj_field(obj, path->res_id);
	if (!obj_field) {
		return -ENOENT;
	}

	if ((obj_field->permissions & LWM2M_PERM_W) != LWM2M_PERM_W) {
		return -EPERM;
	}

	if (!obj_inst->resources || obj_inst->resource_count == 0) {
		return -EINVAL;
	}

	for (i = 0; i < obj_inst->resource_count; i++) {
		if (obj_inst->resources[i].res_id == path->res_id) {
			res = &obj_inst->resources[i];
			break;
		}
	}

	if (!res) {
		return -ENOENT;
	}

	context->path->level = 3;
	context->in->inpos = 0;
	return lwm2m_write_handler(obj_inst, res, obj_field, context);
}
