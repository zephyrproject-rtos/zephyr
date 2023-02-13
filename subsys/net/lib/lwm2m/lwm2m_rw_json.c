/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2018-2019 Foundries.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Copyright (c) 2016, Eistec AB.
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
 *         Joakim Nohlgård <joakim.nohlgard@eistec.se>
 *         Joakim Eriksson <joakime@sics.se> added JSON reader parts
 */

/*
 * Zephyr Contribution by Michael Scott <michael.scott@linaro.org>
 * - Zephyr code style changes / code cleanup
 * - Move to Zephyr APIs where possible
 * - Convert to Zephyr int/uint types
 * - Remove engine dependency (replace with writer/reader context)
 * - Add write / read int64 functions
 */

/*
 * TODO:
 * - Debug formatting errors in Leshan
 * - Replace magic #'s with defines
 */

#define LOG_MODULE_NAME net_lwm2m_json
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <zephyr/data/json.h>

#include "lwm2m_object.h"
#include "lwm2m_rw_json.h"
#include "lwm2m_engine.h"
#include "lwm2m_util.h"

struct json_string_payload {
	const char *name;
	const char *val_string;
};

struct json_boolean_payload {
	const char *name;
	bool val_bool;
};

struct json_float_payload {
	const char *name;
	struct json_obj_token val_float;
};

struct json_array_object {
	union {
		struct json_float_payload  float_obj;
		struct json_boolean_payload boolean_obj;
		struct json_string_payload string_obj;
	} obj;
};

/* Decode payload structure */
struct json_context {
	const char *base_name;
	struct json_obj_token obj_array;
};

/* Decode description structure for parsing LwM2m JSON main object*/
static const struct json_obj_descr json_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_context, "bn",
				  base_name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_context, "e",
				  obj_array, JSON_TOK_OBJ_ARRAY),
};

#define	JSON_BN_TYPE	1
#define	JSON_E_TYPE		2

/* Decode payload structure */
struct json_obj_struct {
	const char *name;
	char *val_object_link;
	const char *val_string;
	struct json_obj_token val_float;
	bool val_bool;
};

/* Decode description structure for parsing LwM2m JSON Arrary object*/
static const struct json_obj_descr json_obj_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_obj_struct, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_obj_struct, "v",
				  val_float, JSON_TOK_FLOAT),
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_obj_struct, "bv",
				  val_bool, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_obj_struct, "ov",
				  val_object_link, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_obj_struct, "sv",
				  val_string, JSON_TOK_STRING),
};

#define	JSON_N_TYPE	1
#define	JSON_V_TYPE	2
#define	JSON_BV_TYPE	4
#define	JSON_OV_TYPE	8
#define	JSON_SV_TYPE	16

#define JSON_NAME_MASK (JSON_N_TYPE)
#define JSON_VAL_MASK (JSON_V_TYPE + JSON_BV_TYPE + JSON_OV_TYPE + JSON_SV_TYPE)

static const struct json_obj_descr json_float_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_float_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_float_payload, "v",
				  val_float, JSON_TOK_FLOAT),
};

static const struct json_obj_descr json_boolean_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_boolean_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_boolean_payload, "bv",
				  val_bool, JSON_TOK_TRUE),
};

static const struct json_obj_descr json_obj_lnk_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_string_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_string_payload, "ov",
				  val_string, JSON_TOK_STRING),
};

static const struct json_obj_descr json_string_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_string_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct json_string_payload, "sv",
				  val_string, JSON_TOK_STRING),
};

struct json_out_formatter_data {
	uint8_t writer_flags;
	char name_string[sizeof("/65535/65535/") + 1];
	struct json_array_object json;
	struct lwm2m_output_context *out;
};

struct json_in_formatter_data {
	uint8_t json_flags;
	int object_bit_field;
	struct json_obj_struct array_object;
};

/* some temporary buffer space for format conversions */
static char pt_buffer[42];

static int init_object_name_parameters(struct json_out_formatter_data *fd,
				       struct lwm2m_obj_path *path)
{
	int ret;

	/* Init Name string */
	if (fd->writer_flags & WRITER_RESOURCE_INSTANCE) {
		ret = snprintk(fd->name_string, sizeof(fd->name_string), "%u/%u", path->res_id,
			       path->res_inst_id);
	} else {
		ret = snprintk(fd->name_string, sizeof(fd->name_string), "%u", path->res_id);
	}

	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int number_to_string(const char *format, ...)
{
	va_list vargs;
	int n;

	va_start(vargs, format);
	n = vsnprintk(pt_buffer, sizeof(pt_buffer), format, vargs);
	va_end(vargs);
	if (n < 0 || n >= sizeof(pt_buffer)) {
		return -EINVAL;
	}

	return n;
}

static int float_to_string(double *value)
{
	int len;

	len = lwm2m_ftoa(value, pt_buffer, sizeof(pt_buffer), 15);
	if (len < 0 || len >= sizeof(pt_buffer)) {
		LOG_ERR("Failed to encode float value");
		return -EINVAL;
	}

	return len;
}

static int objlnk_to_string(struct lwm2m_objlnk *value)
{
	return snprintk(pt_buffer, sizeof(pt_buffer), "%u:%u", value->obj_id, value->obj_inst);
}

static int json_add_separator(struct lwm2m_output_context *out, struct json_out_formatter_data *fd)
{
	int len = 0;

	if (fd->writer_flags & WRITER_OUTPUT_VALUE) {
		/* Add separator */
		char separator = ',';

		len = buf_append(CPKT_BUF_WRITE(out->out_cpkt), &separator, sizeof(separator));
		if (len < 0) {
			return -ENOMEM;
		}
	}

	return len;
}

static void json_postprefix(struct json_out_formatter_data *fd)
{
	fd->writer_flags |= WRITER_OUTPUT_VALUE;
}

static int json_float_object_write(struct lwm2m_output_context *out,
				   struct json_out_formatter_data *fd, int float_string_length)
{
	int res, len;
	ssize_t o_len;
	const struct json_obj_descr *descr;
	size_t descr_len;
	void *obj_payload;

	len = json_add_separator(out, fd);
	if (len < 0) {
		return len;
	}

	descr = json_float_descr;
	descr_len = ARRAY_SIZE(json_float_descr);
	obj_payload = &fd->json.obj.float_obj;
	fd->json.obj.float_obj.name = fd->name_string;
	fd->json.obj.float_obj.val_float.start = pt_buffer;
	fd->json.obj.float_obj.val_float.length = float_string_length;

	/* Calculate length */
	o_len = json_calc_encoded_len(descr, descr_len, obj_payload);
	if (o_len < 0) {
		return -EINVAL;
	}

	/* Encode */
	res = json_obj_encode_buf(descr, descr_len, obj_payload,
				  CPKT_BUF_W_REGION(out->out_cpkt));
	if (res < 0) {
		return -ENOMEM;
	}

	len += o_len;
	out->out_cpkt->offset += len;
	json_postprefix(fd);
	return len;
}

static int json_string_object_write(struct lwm2m_output_context *out,
				    struct json_out_formatter_data *fd, char *buf)
{
	int res, len;
	ssize_t o_len;
	const struct json_obj_descr *descr;
	size_t descr_len;
	void *obj_payload;

	len = json_add_separator(out, fd);
	if (len < 0) {
		return len;
	}

	descr = json_string_descr;
	descr_len = ARRAY_SIZE(json_string_descr);
	obj_payload = &fd->json.obj.string_obj;
	fd->json.obj.string_obj.name = fd->name_string;
	fd->json.obj.string_obj.val_string = buf;

	/* Calculate length */
	o_len = json_calc_encoded_len(descr, descr_len, obj_payload);
	if (o_len < 0) {
		return -EINVAL;
	}

	/* Encode */
	res = json_obj_encode_buf(descr, descr_len, obj_payload,
				  CPKT_BUF_W_REGION(out->out_cpkt));
	if (res < 0) {
		return -ENOMEM;
	}

	len += o_len;
	out->out_cpkt->offset += len;
	json_postprefix(fd);
	return len;
}

static int json_boolean_object_write(struct lwm2m_output_context *out,
				     struct json_out_formatter_data *fd, bool value)
{
	int res, len;
	ssize_t o_len;
	const struct json_obj_descr *descr;
	size_t descr_len;
	void *obj_payload;

	len = json_add_separator(out, fd);
	if (len < 0) {
		return len;
	}

	descr = json_boolean_descr;
	descr_len = ARRAY_SIZE(json_boolean_descr);
	obj_payload = &fd->json.obj.boolean_obj;
	fd->json.obj.boolean_obj.name = fd->name_string;
	fd->json.obj.boolean_obj.val_bool = value;

	/* Calculate length */
	o_len = json_calc_encoded_len(descr, descr_len, obj_payload);
	if (o_len < 0) {
		return -EINVAL;
	}

	/* Encode */
	res = json_obj_encode_buf(descr, descr_len, obj_payload,
				  CPKT_BUF_W_REGION(out->out_cpkt));
	if (res < 0) {
		return -ENOMEM;
	}

	len += o_len;
	out->out_cpkt->offset += len;
	json_postprefix(fd);
	return len;
}

static int json_objlnk_object_write(struct lwm2m_output_context *out,
				    struct json_out_formatter_data *fd)
{
	int res, len;
	ssize_t o_len;
	const struct json_obj_descr *descr;
	size_t descr_len;
	void *obj_payload;

	len = json_add_separator(out, fd);
	if (len < 0) {
		return len;
	}

	descr = json_obj_lnk_descr;
	descr_len = ARRAY_SIZE(json_obj_lnk_descr);
	obj_payload = &fd->json.obj.string_obj;
	fd->json.obj.string_obj.name = fd->name_string;
	fd->json.obj.string_obj.val_string = pt_buffer;

	/* Calculate length */
	o_len = json_calc_encoded_len(descr, descr_len, obj_payload);
	if (o_len < 0) {
		return -EINVAL;
	}

	/* Encode */
	res = json_obj_encode_buf(descr, descr_len, obj_payload,
				  CPKT_BUF_W_REGION(out->out_cpkt));
	if (res < 0) {
		return -ENOMEM;
	}

	len += o_len;
	out->out_cpkt->offset += len;
	json_postprefix(fd);
	return len;
}

static int put_begin(struct lwm2m_output_context *out,
		     struct lwm2m_obj_path *path)
{
	int len = -1, res;

	if (path->level >= 2U) {
		len = snprintk(pt_buffer, sizeof(pt_buffer),
			       "{\"bn\":\"/%u/%u/\",\"e\":[",
			       path->obj_id, path->obj_inst_id);
	} else {
		len = snprintk(pt_buffer, sizeof(pt_buffer),
			       "{\"bn\":\"/%u/\",\"e\":[",
			       path->obj_id);
	}

	if (len < 0) {
		return len;
	}

	res = buf_append(CPKT_BUF_WRITE(out->out_cpkt), pt_buffer, len);
	if (res < 0) {
		return res;
	}

	return len;
}

static int put_end(struct lwm2m_output_context *out,
		   struct lwm2m_obj_path *path)
{
	int res;

	res = buf_append(CPKT_BUF_WRITE(out->out_cpkt), "]}", 2);
	if (res < 0) {
		return res;
	}

	return 2;
}

static int put_begin_ri(struct lwm2m_output_context *out,
			struct lwm2m_obj_path *path)
{
	struct json_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	fd->writer_flags |= WRITER_RESOURCE_INSTANCE;
	return 0;
}

static int put_end_ri(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path)
{
	struct json_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	fd->writer_flags &= ~WRITER_RESOURCE_INSTANCE;
	return 0;
}

static int put_s32(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, int32_t value)
{
	struct json_out_formatter_data *fd;
	int len = 0;

	fd = engine_get_out_user_data(out);

	if (!out->out_cpkt || !fd) {
		return -EINVAL;
	}

	if (init_object_name_parameters(fd, path)) {
		return -EINVAL;
	}

	len = number_to_string("%d", value);
	if (len < 0) {
		return len;
	}

	return json_float_object_write(out, fd, len);
}

static int put_s16(struct lwm2m_output_context *out,
		   struct lwm2m_obj_path *path, int16_t value)
{
	return put_s32(out, path, (int32_t)value);
}

static int put_s8(struct lwm2m_output_context *out, struct lwm2m_obj_path *path,
		  int8_t value)
{
	return put_s32(out, path, (int32_t)value);
}

static int put_s64(struct lwm2m_output_context *out,
		   struct lwm2m_obj_path *path, int64_t value)
{
	struct json_out_formatter_data *fd;
	int len;

	fd = engine_get_out_user_data(out);

	if (!out->out_cpkt || !fd) {
		return -EINVAL;
	}

	if (init_object_name_parameters(fd, path)) {
		return -EINVAL;
	}

	len = number_to_string("%lld", value);
	if (len < 0) {
		return len;
	}

	return json_float_object_write(out, fd, len);
}

static int put_time(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, time_t value)
{
	return put_s64(out, path, (int64_t) value);
}

static int put_string(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, char *buf, size_t buflen)
{
	struct json_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);

	if (!out->out_cpkt || !fd) {
		return -EINVAL;
	}

	if (init_object_name_parameters(fd, path)) {
		return -EINVAL;
	}

	return json_string_object_write(out, fd, buf);
}

static int put_float(struct lwm2m_output_context *out,
		     struct lwm2m_obj_path *path, double *value)
{
	struct json_out_formatter_data *fd;
	int len;

	fd = engine_get_out_user_data(out);

	if (!out->out_cpkt || !fd) {
		return -EINVAL;
	}

	if (init_object_name_parameters(fd, path)) {
		return -EINVAL;
	}

	len = float_to_string(value);
	if (len < 0) {
		return len;
	}

	return json_float_object_write(out, fd, len);
}

static int put_bool(struct lwm2m_output_context *out,
		    struct lwm2m_obj_path *path, bool value)
{
	struct json_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);

	if (!out->out_cpkt || !fd) {
		return -EINVAL;
	}

	if (init_object_name_parameters(fd, path)) {
		return -EINVAL;
	}

	return json_boolean_object_write(out, fd, value);
}

static int put_objlnk(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, struct lwm2m_objlnk *value)
{
	struct json_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);

	if (!out->out_cpkt || !fd) {
		return -EINVAL;
	}

	if (init_object_name_parameters(fd, path)) {
		return -EINVAL;
	}

	if (objlnk_to_string(value) < 0) {
		return -EINVAL;
	}

	return json_objlnk_object_write(out, fd);
}

static int read_int(struct lwm2m_input_context *in, int64_t *value,
		    bool accept_sign)
{
	struct json_in_formatter_data *fd;
	uint8_t *buf;
	int i = 0;
	bool neg = false;
	char c;

	/* initialize values to 0 */
	*value = 0;

	fd = engine_get_in_user_data(in);
	if (!fd || (fd->object_bit_field & JSON_V_TYPE) == 0) {
		return -EINVAL;
	}

	if (fd->array_object.val_float.length == 0) {
		return -ENODATA;
	}

	buf = fd->array_object.val_float.start;
	while (*(buf + i) && i < fd->array_object.val_float.length) {
		c = *(buf + i);
		if (c == '-' && accept_sign && i == 0) {
			neg = true;
		} else if (isdigit(c)) {
			*value = *value * 10 + (c - '0');
		} else {
			/* anything else stop reading */
			break;
		}
		i++;
	}

	if (neg) {
		*value = -*value;
	}

	return i;
}

static int get_s64(struct lwm2m_input_context *in, int64_t *value)
{
	return read_int(in, value, true);
}

static int get_time(struct lwm2m_input_context *in, time_t *value)
{
	int64_t temp64;
	int ret;

	ret = read_int(in, &temp64, true);
	*value = (time_t)temp64;

	return ret;
}

static int get_s32(struct lwm2m_input_context *in, int32_t *value)
{
	int64_t tmp = 0;
	int len = 0;

	len = read_int(in, &tmp, true);
	if (len > 0) {
		*value = (int32_t)tmp;
	}

	return len;
}

static int get_string(struct lwm2m_input_context *in, uint8_t *buf,
		      size_t buflen)
{
	struct json_in_formatter_data *fd;
	size_t string_length;

	fd = engine_get_in_user_data(in);
	if (!fd || (fd->object_bit_field & JSON_SV_TYPE) == 0) {
		return -EINVAL;
	}

	string_length = strlen(fd->array_object.val_string);

	if (string_length > buflen) {
		LOG_WRN("Buffer too small to accommodate string, truncating");
		string_length = buflen - 1;
	}
	memcpy(buf, fd->array_object.val_string, string_length);

	/* add NULL */
	buf[string_length] = '\0';

	return string_length;
}

static int get_float(struct lwm2m_input_context *in, double *value)
{
	struct json_in_formatter_data *fd;

	int i = 0, len = 0;
	bool has_dot = false;
	uint8_t tmp, buf[24];
	uint8_t *json_buf;

	fd = engine_get_in_user_data(in);
	if (!fd || (fd->object_bit_field & JSON_V_TYPE) == 0) {
		return -EINVAL;
	}

	size_t value_length = fd->array_object.val_float.length;

	if (value_length == 0) {
		return -ENODATA;
	}

	json_buf = fd->array_object.val_float.start;
	while (*(json_buf + len) && len < value_length) {
		tmp = *(json_buf + len);

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
			break;
		}
	}

	buf[i] = '\0';

	if (lwm2m_atof(buf, value) != 0) {
		LOG_ERR("Failed to parse float value");
		return -EBADMSG;
	}

	return len;
}

static int get_bool(struct lwm2m_input_context *in, bool *value)
{
	struct json_in_formatter_data *fd;

	fd = engine_get_in_user_data(in);
	if (!fd || (fd->object_bit_field & JSON_BV_TYPE) == 0) {
		return -EINVAL;
	}

	*value = fd->array_object.val_bool;

	return 1;
}

static int get_opaque(struct lwm2m_input_context *in, uint8_t *value,
		      size_t buflen, struct lwm2m_opaque_context *opaque,
		      bool *last_block)
{
	/* TODO */
	return -EOPNOTSUPP;
}

static int get_objlnk(struct lwm2m_input_context *in,
		      struct lwm2m_objlnk *value)
{
	int64_t tmp;
	int len, total_len;
	struct json_in_formatter_data *fd;
	char *demiliter_pos;

	fd = engine_get_in_user_data(in);
	if (!fd || (fd->object_bit_field & JSON_OV_TYPE) == 0) {
		return -EINVAL;
	}

	demiliter_pos = strchr(fd->array_object.val_object_link, ':');
	if (!demiliter_pos) {
		return -ENODATA;
	}

	fd->object_bit_field |= JSON_V_TYPE;
	fd->array_object.val_float.start = fd->array_object.val_object_link;
	fd->array_object.val_float.length = strlen(fd->array_object.val_object_link);

	/* Set String end for first item  */
	*demiliter_pos = '\0';

	len = read_int(in, &tmp, false);
	if (len <= 0) {
		return -ENODATA;
	}

	total_len = len;
	value->obj_id = (uint16_t)tmp;

	len++;  /* +1 for ':' delimiter. */
	demiliter_pos++;
	fd->array_object.val_float.start = demiliter_pos;
	fd->array_object.val_float.length = strlen(demiliter_pos);

	len = read_int(in, &tmp, false);
	if (len <= 0) {
		return -ENODATA;
	}

	total_len += len;
	value->obj_inst = (uint16_t)tmp;

	return total_len;
}

const struct lwm2m_writer json_writer = {
	.put_begin = put_begin,
	.put_end = put_end,
	.put_begin_ri = put_begin_ri,
	.put_end_ri = put_end_ri,
	.put_s8 = put_s8,
	.put_s16 = put_s16,
	.put_s32 = put_s32,
	.put_s64 = put_s64,
	.put_string = put_string,
	.put_float = put_float,
	.put_time = put_time,
	.put_bool = put_bool,
	.put_objlnk = put_objlnk,
};

const struct lwm2m_reader json_reader = {
	.get_s32 = get_s32,
	.get_s64 = get_s64,
	.get_string = get_string,
	.get_time = get_time,
	.get_float = get_float,
	.get_bool = get_bool,
	.get_opaque = get_opaque,
	.get_objlnk = get_objlnk,
};

int do_read_op_json(struct lwm2m_message *msg, int content_format)
{
	struct json_out_formatter_data fd;
	int ret;

	(void)memset(&fd, 0, sizeof(fd));
	engine_set_out_user_data(&msg->out, &fd);
	ret = lwm2m_perform_read_op(msg, content_format);
	engine_clear_out_user_data(&msg->out);

	return ret;
}

int do_write_op_json(struct lwm2m_message *msg)
{
	struct lwm2m_engine_obj_field *obj_field = NULL;
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_engine_res *res = NULL;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	struct lwm2m_obj_path orig_path;
	struct json_in_formatter_data fd;
	struct json_obj json_object;
	struct json_context main_object;

	char *data_ptr;
	const char *base_name_ptr = NULL;
	uint16_t in_len;
	int ret = 0, obj_bit_field;

	uint8_t full_name[MAX_RESOURCE_LEN + 1] = {0};
	uint8_t created;

	(void)memset(&fd, 0, sizeof(fd));
	(void)memset(&main_object, 0, sizeof(main_object));
	engine_set_in_user_data(&msg->in, &fd);

	data_ptr = (char *)coap_packet_get_payload(msg->in.in_cpkt, &in_len);

	obj_bit_field =
		json_obj_parse(data_ptr, in_len, json_descr, ARRAY_SIZE(json_descr), &main_object);

	if (obj_bit_field < 0 || (obj_bit_field & 2) == 0 || main_object.obj_array.length == 0) {
		LOG_ERR("JSON object bits not valid %d", obj_bit_field);
		ret = -EINVAL;
		goto end_of_operation;
	}

	if (obj_bit_field & 1) {
		base_name_ptr = main_object.base_name;
	}

	/* store a copy of the original path */
	memcpy(&orig_path, &msg->path, sizeof(msg->path));

	/* When No blockwise do Normal Init */
	if (json_arr_separate_object_parse_init(&json_object, main_object.obj_array.start,
						main_object.obj_array.length)) {
		ret = -EINVAL;
		goto end_of_operation;
	}

	while (1) {
		(void)memset(&fd.array_object, 0, sizeof(fd.array_object));
		fd.object_bit_field = json_arr_separate_parse_object(
			&json_object, json_obj_descr, ARRAY_SIZE(json_obj_descr), &fd.array_object);
		if (fd.object_bit_field == 0) {
			/* End of  */
			break;
		} else if (fd.object_bit_field < 0 ||
			   ((fd.object_bit_field & JSON_VAL_MASK) == 0)) {
			LOG_ERR("Json Write Parse object fail %d", fd.object_bit_field);
			ret = -EINVAL;
			goto end_of_operation;
		}

		/* Create object resource path */
		if (base_name_ptr) {
			if (fd.object_bit_field & JSON_N_TYPE) {
				ret = snprintk(full_name, sizeof(full_name), "%s%s", base_name_ptr,
					       fd.array_object.name);
			} else {
				ret = snprintk(full_name, sizeof(full_name), "%s", base_name_ptr);
			}
		} else {
			if ((fd.object_bit_field & JSON_N_TYPE) == 0) {
				ret = -EINVAL;
				goto end_of_operation;
			}
			ret = snprintk(full_name, sizeof(full_name), "%s", fd.array_object.name);
		}

		if (ret >= MAX_RESOURCE_LEN) {
			ret = -EINVAL;
			goto end_of_operation;
		}

		/* handle resource value */
		/* reset values */
		created = 0U;

		/* parse full_name into path */
		ret = lwm2m_string_to_path(full_name, &msg->path, '/');
		if (ret < 0) {
			LOG_ERR("Relative name too long");
			ret = -EINVAL;
			goto end_of_operation;
		}

		ret = lwm2m_get_or_create_engine_obj(msg, &obj_inst, &created);
		if (ret < 0) {
			break;
		}

		ret = lwm2m_engine_validate_write_access(msg, obj_inst, &obj_field);
		if (ret < 0) {
			return ret;
		}

		ret = lwm2m_engine_get_create_res_inst(&msg->path, &res, &res_inst);
		if (ret < 0) {
			return -ENOENT;
		}

		/* Write the resource value */
		ret = lwm2m_write_handler(obj_inst, res, res_inst, obj_field, msg);
		if (orig_path.level >= 3U && ret < 0) {
			/* return errors on a single write */
			break;
		}
	}

end_of_operation:
	engine_clear_in_user_data(&msg->in);

	return ret;
}
