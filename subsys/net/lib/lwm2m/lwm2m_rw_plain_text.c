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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "lwm2m_object.h"
#include "lwm2m_rw_plain_text.h"
#include "lwm2m_engine.h"
#include "lwm2m_util.h"

/* some temporary buffer space for format conversions */
static char pt_buffer[42];

int plain_text_put_format(struct lwm2m_output_context *out, const char *format,
			  ...)
{
	va_list vargs;
	int n, ret;

	va_start(vargs, format);
	n = vsnprintk(pt_buffer, sizeof(pt_buffer), format, vargs);
	va_end(vargs);
	if (n < 0 || n >= sizeof(pt_buffer)) {
		return -EINVAL;
	}

	ret = buf_append(CPKT_BUF_WRITE(out->out_cpkt), pt_buffer, n);
	if (ret < 0) {
		return ret;
	}

	return n;
}

static int put_s32(struct lwm2m_output_context *out,
		   struct lwm2m_obj_path *path, int32_t value)
{
	return plain_text_put_format(out, "%d", value);
}

static int put_s8(struct lwm2m_output_context *out, struct lwm2m_obj_path *path,
		  int8_t value)
{
	return plain_text_put_format(out, "%d", value);
}

static int put_s16(struct lwm2m_output_context *out,
		   struct lwm2m_obj_path *path, int16_t value)
{
	return plain_text_put_format(out, "%d", value);
}

static int put_s64(struct lwm2m_output_context *out,
		   struct lwm2m_obj_path *path, int64_t value)
{
	return plain_text_put_format(out, "%lld", value);
}

int plain_text_put_float(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path, double *value)
{
	int len, ret;

	len = lwm2m_ftoa(value, pt_buffer, sizeof(pt_buffer), 15);
	if (len < 0 || len >= sizeof(pt_buffer)) {
		LOG_ERR("Failed to encode float value");
		return -EINVAL;
	}

	ret = buf_append(CPKT_BUF_WRITE(out->out_cpkt), pt_buffer, len);
	if (ret < 0) {
		return ret;
	}

	return len;
}

static int put_string(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, char *buf, size_t buflen)
{
	int ret;

	ret = buf_append(CPKT_BUF_WRITE(out->out_cpkt), buf, buflen);
	if (ret < 0) {
		return ret;
	}

	return buflen;
}

static int put_bool(struct lwm2m_output_context *out,
		    struct lwm2m_obj_path *path, bool value)
{
	if (value) {
		return plain_text_put_format(out, "%u", 1);
	} else {
		return plain_text_put_format(out, "%u", 0);
	}
}

static int put_objlnk(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, struct lwm2m_objlnk *value)
{
	return plain_text_put_format(out, "%u:%u", value->obj_id,
				     value->obj_inst);
}

static int plain_text_read_int(struct lwm2m_input_context *in, int64_t *value,
			       bool accept_sign)
{
	int i = 0;
	bool neg = false;
	uint8_t tmp;

	if (in->offset >= in->in_cpkt->offset) {
		/* No remaining data in the payload. */
		return -ENODATA;
	}

	/* initialize values to 0 */
	*value = 0;

	while (in->offset < in->in_cpkt->offset) {
		if (buf_read_u8(&tmp, CPKT_BUF_READ(in->in_cpkt),
				&in->offset) < 0) {
			break;
		}

		if (tmp == '-' && accept_sign && i == 0) {
			neg = true;
		} else if (isdigit(tmp)) {
			*value = *value * 10 + (tmp - '0');
		} else {
			/* anything else stop reading */
			in->offset--;
			break;
		}

		i++;
	}

	if (neg) {
		*value = -*value;
	}

	return i;
}

static int get_s32(struct lwm2m_input_context *in, int32_t *value)
{
	int64_t tmp = 0;
	size_t len = 0;

	len = plain_text_read_int(in, &tmp, true);
	if (len > 0) {
		*value = (int32_t)tmp;
	}

	return len;
}

static int get_s64(struct lwm2m_input_context *in, int64_t *value)
{
	return plain_text_read_int(in, value, true);
}

static int get_string(struct lwm2m_input_context *in, uint8_t *value,
		      size_t buflen)
{
	uint16_t in_len;

	coap_packet_get_payload(in->in_cpkt, &in_len);

	if (in_len > buflen) {
		LOG_ERR("Buffer too small to accommodate string, truncating");
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

static int get_float(struct lwm2m_input_context *in, double *value)
{
	size_t i = 0, len = 0;
	bool has_dot = false;
	uint8_t tmp, buf[24];
	int ret;

	if (in->offset >= in->in_cpkt->offset) {
		/* No remaining data in the payload. */
		return -ENODATA;
	}

	while (in->offset < in->in_cpkt->offset) {
		if (buf_read_u8(&tmp, CPKT_BUF_READ(in->in_cpkt),
				&in->offset) < 0) {
			break;
		}

		if ((tmp == '-' && i == 0) || (tmp == '.' && !has_dot) ||
		    isdigit(tmp)) {
			len++;

			/* Copy only if it fits into provided buffer - we won't
			 * get better precision anyway.
			 */
			if (i < sizeof(buf) - 1) {
				buf[i++] = tmp;
			}

			if (tmp == '.') {
				has_dot = true;
			}
		} else {
			/* anything else stop reading */
			in->offset--;
			break;
		}
	}

	buf[i] = '\0';

	ret = lwm2m_atof(buf, value);
	if (ret != 0) {
		LOG_ERR("Failed to parse float value");
		return -EBADMSG;
	}

	return len;
}

static int get_bool(struct lwm2m_input_context *in, bool *value)
{
	uint8_t tmp;
	int ret;

	if (in->offset >= in->in_cpkt->offset) {
		/* No remaining data in the payload. */
		return -ENODATA;
	}

	ret = buf_read_u8(&tmp, CPKT_BUF_READ(in->in_cpkt), &in->offset);
	if (ret < 0) {
		return ret;
	}

	if (tmp != '1' && tmp != '0') {
		return -EBADMSG;
	}

	*value = (tmp == '1') ? true : false;
	return sizeof(uint8_t);
}

static int get_opaque(struct lwm2m_input_context *in, uint8_t *value,
		      size_t buflen, struct lwm2m_opaque_context *opaque,
		      bool *last_block)
{
	uint16_t in_len;

	if (opaque->remaining == 0) {
		coap_packet_get_payload(in->in_cpkt, &in_len);

		if (in_len == 0) {
			return -ENODATA;
		}

		if (in->block_ctx != NULL) {
			uint32_t block_num =
				in->block_ctx->ctx.current /
				coap_block_size_to_bytes(
					in->block_ctx->ctx.block_size);

			if (block_num == 0) {
				opaque->len = in->block_ctx->ctx.total_size;
			}

			if (opaque->len == 0) {
				/* No size1 option provided, use current
				 * payload size. This will reset on next packet
				 * received.
				 */
				opaque->remaining = in_len;
			} else {
				opaque->remaining = opaque->len;
			}

		} else {
			opaque->len = in_len;
			opaque->remaining = in_len;
		}
	}

	return lwm2m_engine_get_opaque_more(in, value, buflen,
					    opaque, last_block);
}

static int get_objlnk(struct lwm2m_input_context *in,
		      struct lwm2m_objlnk *value)
{
	int64_t tmp;
	int len, total_len;

	len = plain_text_read_int(in, &tmp, false);
	if (len <= 0) {
		return -ENODATA;
	}

	total_len = len;
	value->obj_id = (uint16_t)tmp;

	/* Skip ':' delimiter. */
	total_len++;
	in->offset++;

	len = plain_text_read_int(in, &tmp, false);
	if (len <= 0) {
		return -ENODATA;
	}

	total_len += len;
	value->obj_inst = (uint16_t)tmp;

	return total_len;
}

const struct lwm2m_writer plain_text_writer = {
	.put_s8 = put_s8,
	.put_s16 = put_s16,
	.put_s32 = put_s32,
	.put_s64 = put_s64,
	.put_string = put_string,
	.put_float = plain_text_put_float,
	.put_time = put_s64,
	.put_bool = put_bool,
	.put_objlnk = put_objlnk,
};

const struct lwm2m_reader plain_text_reader = {
	.get_s32 = get_s32,
	.get_s64 = get_s64,
	.get_string = get_string,
	.get_time = get_s64,
	.get_float = get_float,
	.get_bool = get_bool,
	.get_opaque = get_opaque,
	.get_objlnk = get_objlnk,
};

int do_read_op_plain_text(struct lwm2m_message *msg, int content_format)
{
	/* Plain text can only return single resource (instance) */
	if (msg->path.level < LWM2M_PATH_LEVEL_RESOURCE) {
		return -EPERM;
	} else if (msg->path.level > LWM2M_PATH_LEVEL_RESOURCE) {
		if (!IS_ENABLED(CONFIG_LWM2M_VERSION_1_1)) {
			return -ENOENT;
		} else if (msg->path.level > LWM2M_PATH_LEVEL_RESOURCE_INST) {
			return -ENOENT;
		}
	}

	return lwm2m_perform_read_op(msg, content_format);
}

int do_write_op_plain_text(struct lwm2m_message *msg)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_engine_obj_field *obj_field;
	struct lwm2m_engine_res *res = NULL;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	int ret;
	uint8_t created = 0U;

	ret = lwm2m_get_or_create_engine_obj(msg, &obj_inst, &created);
	if (ret < 0) {
		return ret;
	}

	ret = lwm2m_engine_validate_write_access(msg, obj_inst, &obj_field);
	if (ret < 0) {
		return ret;
	}

	ret = lwm2m_engine_get_create_res_inst(&msg->path, &res, &res_inst);
	if (ret < 0) {
		return -ENOENT;
	}

	if (msg->path.level < 3) {
		msg->path.level = 3U;
	}

	return lwm2m_write_handler(obj_inst, res, res_inst, obj_field, msg);
}
