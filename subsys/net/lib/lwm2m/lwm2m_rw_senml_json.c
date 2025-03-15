/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_senml_json
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <zephyr/sys/base64.h>
#include <zephyr/sys/slist.h>
#include <zephyr/data/json.h>

#include "lwm2m_object.h"
#include "lwm2m_rw_senml_json.h"
#include "lwm2m_engine.h"
#include "lwm2m_util.h"

#define BASE64_OUTPUT_MIN_LENGTH 4
#define BASE64_MODULO_LENGTH(x) (x % BASE64_OUTPUT_MIN_LENGTH)
#define BASE64_BYTES_TO_MODULO(x) (BASE64_OUTPUT_MIN_LENGTH - x)

struct senml_string_bn_payload {
	const char *base_name;
	const char *name;
	const char *val_string;
};

struct senml_string_payload {
	const char *name;
	const char *val_string;
};

struct senml_opaque_payload {
	const char *name;
	struct json_obj_token val_opaque;
};

struct senml_opaque_bn_payload {
	const char *base_name;
	const char *name;
	struct json_obj_token val_opaque;
};

struct senml_boolean_payload {
	const char *name;
	bool val_bool;
};

struct senml_boolean_t_payload {
	const char *name;
	bool val_bool;
	struct json_obj_token time;
};

struct senml_boolean_bn_payload {
	const char *base_name;
	const char *name;
	bool val_bool;
};

struct senml_boolean_bn_t_payload {
	const char *base_name;
	const char *name;
	bool val_bool;
	struct json_obj_token base_time;
};

struct senml_float_payload {
	const char *name;
	struct json_obj_token val_float;
};

struct senml_float_t_payload {
	const char *name;
	struct json_obj_token val_float;
	struct json_obj_token time;
};

struct senml_float_bn_payload {
	const char *base_name;
	const char *name;
	struct json_obj_token val_float;
};

struct senml_float_bn_t_payload {
	const char *base_name;
	const char *name;
	struct json_obj_token val_float;
	struct json_obj_token base_time;
};

struct senml_json_object {
	union {
		struct senml_float_payload  float_obj;
		struct senml_float_t_payload float_t_obj;
		struct senml_float_bn_payload float_bn_obj;
		struct senml_float_bn_t_payload float_bn_t_obj;
		struct senml_boolean_payload boolean_obj;
		struct senml_boolean_t_payload boolean_t_obj;
		struct senml_boolean_bn_payload boolean_bn_obj;
		struct senml_boolean_bn_t_payload boolean_bn_t_obj;
		struct senml_opaque_payload  opaque_obj;
		struct senml_opaque_bn_payload  opaque_bn_obj;
		struct senml_string_payload string_obj;
		struct senml_string_bn_payload string_bn_obj;
	} obj;
};

struct json_out_formatter_data {
	uint8_t writer_flags;
	struct lwm2m_obj_path base_name;
	bool add_base_name_to_start;
	bool historical_data;
	char bn_string[sizeof("/65535/65535/") + 1];
	char name_string[sizeof("/65535/65535/") + 1];
	char timestamp_buffer[42];
	int timestamp_length;
	time_t base_time;
	struct senml_json_object json;
	struct lwm2m_output_context *out;
};

/* Decode payload structure */
struct senml_context {
	const char *base_name;
	const char *name;
	char *val_object_link;
	const char *val_string;
	struct json_obj_token val_opaque;
	struct json_obj_token val_float;
	bool val_bool;
};

/* Decode description structure for parsing LwM2m SenML-JSON object*/
static const struct json_obj_descr senml_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_context, "bn",
				  base_name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_context, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_context, "v",
				  val_float, JSON_TOK_FLOAT),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_context, "vb",
				  val_bool, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_context, "vlo",
				  val_object_link, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_context, "vd",
				  val_opaque, JSON_TOK_OPAQUE),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_context, "vs",
				  val_string, JSON_TOK_STRING),
};

#define	JSON_BN_TYPE	1
#define	JSON_N_TYPE	2
#define	JSON_V_TYPE	4
#define	JSON_VB_TYPE	8
#define	JSON_VLO_TYPE	16
#define	JSON_VD_TYPE	32
#define	JSON_VS_TYPE	64

#define JSON_NAME_MASK (JSON_BN_TYPE + JSON_N_TYPE)
#define JSON_VALUE_MASK	(JSON_V_TYPE + JSON_VB_TYPE + JSON_VLO_TYPE + JSON_VD_TYPE + JSON_VS_TYPE)

#define JSON_BIT_CHECK(mask, x) (mask & x)

static const struct json_obj_descr senml_float_bn_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_float_bn_payload, "bn",
				  base_name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_float_bn_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_float_bn_payload, "v",
				  val_float, JSON_TOK_FLOAT),
};

static const struct json_obj_descr senml_float_bn_t_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_float_bn_t_payload, "bn",
				  base_name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_float_bn_t_payload, "bt",
				  base_time, JSON_TOK_FLOAT),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_float_bn_t_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_float_bn_t_payload, "v",
				  val_float, JSON_TOK_FLOAT),
};

static const struct json_obj_descr senml_float_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_float_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_float_payload, "v",
				  val_float, JSON_TOK_FLOAT),
};

static const struct json_obj_descr senml_float_t_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_float_t_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_float_t_payload, "v",
				  val_float, JSON_TOK_FLOAT),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_float_t_payload, "t",
				  time, JSON_TOK_FLOAT),
};

static const struct json_obj_descr senml_boolean_bn_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_boolean_bn_payload, "bn",
				  base_name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_boolean_bn_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_boolean_bn_payload, "vb",
				  val_bool, JSON_TOK_TRUE),
};

static const struct json_obj_descr senml_boolean_bn_t_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_boolean_bn_t_payload, "bn",
				  base_name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_boolean_bn_t_payload, "bt",
				  base_time, JSON_TOK_FLOAT),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_boolean_bn_t_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_boolean_bn_t_payload, "vb",
				  val_bool, JSON_TOK_TRUE),
};

static const struct json_obj_descr senml_boolean_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_boolean_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_boolean_payload, "vb",
				  val_bool, JSON_TOK_TRUE),
};

static const struct json_obj_descr senml_boolean_t_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_boolean_t_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_boolean_t_payload, "vb",
				  val_bool, JSON_TOK_TRUE),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_boolean_t_payload, "t",
				  time, JSON_TOK_FLOAT),
};

static const struct json_obj_descr senml_obj_lnk_bn_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_string_bn_payload, "bn",
				  base_name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_string_bn_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_string_bn_payload, "vlo",
				  val_string, JSON_TOK_STRING),
};

static const struct json_obj_descr senml_obj_lnk_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_string_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_string_payload, "vlo",
				  val_string, JSON_TOK_STRING),
};

static const struct json_obj_descr senml_opaque_bn_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_opaque_bn_payload, "bn",
				  base_name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_opaque_bn_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_opaque_bn_payload, "vd",
				  val_opaque, JSON_TOK_OPAQUE),
};

static const struct json_obj_descr senml_opaque_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_opaque_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_opaque_payload, "vd",
				  val_opaque, JSON_TOK_OPAQUE),
};

static const struct json_obj_descr senml_string_bn_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_string_bn_payload, "bn",
				  base_name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_string_bn_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_string_bn_payload, "vs",
				  val_string, JSON_TOK_STRING),
};

static const struct json_obj_descr senml_string_descr[] = {
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_string_payload, "n",
				  name, JSON_TOK_STRING),
	JSON_OBJ_DESCR_PRIM_NAMED(struct senml_string_payload, "vs",
				  val_string, JSON_TOK_STRING),
};

struct json_in_formatter_data {
	int object_bit_field;
	struct senml_context senml_object;
};

/* some temporary buffer space for format conversions */
static char pt_buffer[42];

static int init_object_name_parameters(struct json_out_formatter_data *fd,
				       struct lwm2m_obj_path *path)
{
	int ret;

	if (fd->add_base_name_to_start) {
		/* Init Base name string */
		ret = snprintk(fd->bn_string, sizeof(fd->bn_string), "/%u/%u/", path->obj_id,
			       path->obj_inst_id);
		if (ret < 0) {
			return ret;
		}
	}

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

static int put_begin(struct lwm2m_output_context *out, struct lwm2m_obj_path *path)
{
	int res;
	struct json_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	res = buf_append(CPKT_BUF_WRITE(out->out_cpkt), "[", 1);
	if (res < 0) {
		return res;
	}

	/* Init base level state for skip first object instance compare */
	fd->base_name.level = LWM2M_PATH_LEVEL_NONE;
	return 1;
}

static int put_end(struct lwm2m_output_context *out, struct lwm2m_obj_path *path)
{
	struct json_out_formatter_data *fd;
	int res;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	res = buf_append(CPKT_BUF_WRITE(out->out_cpkt), "]", 1);
	if (res < 0) {
		return res;
	}

	return 1;
}

static int put_begin_oi(struct lwm2m_output_context *out, struct lwm2m_obj_path *path)
{
	struct json_out_formatter_data *fd;
	bool update_base_name = false;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	if (fd->base_name.level == LWM2M_PATH_LEVEL_NONE) {
		update_base_name = true;

	} else if (fd->base_name.obj_id != path->obj_id ||
		   fd->base_name.obj_inst_id != path->obj_inst_id) {
		update_base_name = true;
	}

	if (update_base_name) {
		fd->base_name.level = LWM2M_PATH_LEVEL_OBJECT_INST;
		fd->base_name.obj_id = path->obj_id;
		fd->base_name.obj_inst_id = path->obj_inst_id;
	}

	fd->add_base_name_to_start = update_base_name;

	return 0;
}

static int put_begin_ri(struct lwm2m_output_context *out, struct lwm2m_obj_path *path)
{
	struct json_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	fd->writer_flags |= WRITER_RESOURCE_INSTANCE;
	return 0;
}

static int put_end_ri(struct lwm2m_output_context *out, struct lwm2m_obj_path *path)
{
	struct json_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	fd->writer_flags &= ~WRITER_RESOURCE_INSTANCE;
	return 0;
}

static int put_end_r(struct lwm2m_output_context *out, struct lwm2m_obj_path *path)
{
	struct json_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	/* Clear Historical Data */
	fd->historical_data = false;
	fd->base_time = 0;
	return 0;
}

static int number_to_string(char *buf, size_t buf_len, const char *format, ...)
{
	va_list vargs;
	int n;

	va_start(vargs, format);
	n = vsnprintk(buf, buf_len, format, vargs);
	va_end(vargs);
	if (n < 0 || n >= buf_len) {
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
	fd->add_base_name_to_start = false;
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

	if (fd->add_base_name_to_start) {
		if (fd->historical_data) {
			descr = senml_float_bn_t_descr;
			descr_len = ARRAY_SIZE(senml_float_bn_t_descr);
			obj_payload = &fd->json.obj.float_bn_t_obj;
			fd->json.obj.float_bn_t_obj.base_name = fd->bn_string;
			fd->json.obj.float_bn_t_obj.name = fd->name_string;
			fd->json.obj.float_bn_t_obj.val_float.start = pt_buffer;
			fd->json.obj.float_bn_t_obj.val_float.length = float_string_length;
			fd->json.obj.float_bn_t_obj.base_time.start = fd->timestamp_buffer;
			fd->json.obj.float_bn_t_obj.base_time.length = fd->timestamp_length;
		} else {
			descr = senml_float_bn_descr;
			descr_len = ARRAY_SIZE(senml_float_bn_descr);
			obj_payload = &fd->json.obj.float_bn_obj;
			fd->json.obj.float_bn_obj.base_name = fd->bn_string;
			fd->json.obj.float_bn_obj.name = fd->name_string;
			fd->json.obj.float_bn_obj.val_float.start = pt_buffer;
			fd->json.obj.float_bn_obj.val_float.length = float_string_length;
		}

	} else {
		if (fd->historical_data) {
			descr = senml_float_t_descr;
			descr_len = ARRAY_SIZE(senml_float_t_descr);
			obj_payload = &fd->json.obj.float_t_obj;
			fd->json.obj.float_t_obj.name = fd->name_string;
			fd->json.obj.float_t_obj.val_float.start = pt_buffer;
			fd->json.obj.float_t_obj.val_float.length = float_string_length;
			fd->json.obj.float_t_obj.time.start = fd->timestamp_buffer;
			fd->json.obj.float_t_obj.time.length = fd->timestamp_length;
		} else {
			descr = senml_float_descr;
			descr_len = ARRAY_SIZE(senml_float_descr);
			obj_payload = &fd->json.obj.float_obj;
			fd->json.obj.float_obj.name = fd->name_string;
			fd->json.obj.float_obj.val_float.start = pt_buffer;
			fd->json.obj.float_obj.val_float.length = float_string_length;
		}
	}

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

	if (fd->add_base_name_to_start) {
		descr = senml_string_bn_descr;
		descr_len = ARRAY_SIZE(senml_string_bn_descr);
		obj_payload = &fd->json.obj.string_bn_obj;
		fd->json.obj.string_bn_obj.base_name = fd->bn_string;
		fd->json.obj.string_bn_obj.name = fd->name_string;
		fd->json.obj.string_bn_obj.val_string = buf;

	} else {
		descr = senml_string_descr;
		descr_len = ARRAY_SIZE(senml_string_descr);
		obj_payload = &fd->json.obj.string_obj;
		fd->json.obj.string_obj.name = fd->name_string;
		fd->json.obj.string_obj.val_string = buf;
	}

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

	if (fd->add_base_name_to_start) {
		if (fd->historical_data) {
			descr = senml_boolean_bn_t_descr;
			descr_len = ARRAY_SIZE(senml_boolean_bn_t_descr);
			obj_payload = &fd->json.obj.boolean_bn_t_obj;
			fd->json.obj.boolean_bn_t_obj.base_name = fd->bn_string;
			fd->json.obj.boolean_bn_t_obj.name = fd->name_string;
			fd->json.obj.boolean_bn_t_obj.base_time.start = fd->timestamp_buffer;
			fd->json.obj.boolean_bn_t_obj.base_time.length = fd->timestamp_length;
			fd->json.obj.boolean_bn_t_obj.val_bool = value;
		} else {
			descr = senml_boolean_bn_descr;
			descr_len = ARRAY_SIZE(senml_boolean_bn_descr);
			obj_payload = &fd->json.obj.boolean_bn_obj;
			fd->json.obj.boolean_bn_obj.base_name = fd->bn_string;
			fd->json.obj.boolean_bn_obj.name = fd->name_string;
			fd->json.obj.boolean_bn_obj.val_bool = value;
		}

	} else {
		if (fd->historical_data) {
			descr = senml_boolean_t_descr;
			descr_len = ARRAY_SIZE(senml_boolean_t_descr);
			obj_payload = &fd->json.obj.boolean_t_obj;
			fd->json.obj.boolean_t_obj.name = fd->name_string;
			fd->json.obj.boolean_t_obj.time.start = fd->timestamp_buffer;
			fd->json.obj.boolean_t_obj.time.length = fd->timestamp_length;
			fd->json.obj.boolean_t_obj.val_bool = value;
		} else {
			descr = senml_boolean_descr;
			descr_len = ARRAY_SIZE(senml_boolean_descr);
			obj_payload = &fd->json.obj.boolean_obj;
			fd->json.obj.boolean_obj.name = fd->name_string;
			fd->json.obj.boolean_obj.val_bool = value;
		}
	}

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

	if (fd->add_base_name_to_start) {
		descr = senml_obj_lnk_bn_descr;
		descr_len = ARRAY_SIZE(senml_obj_lnk_bn_descr);
		obj_payload = &fd->json.obj.string_bn_obj;
		fd->json.obj.string_bn_obj.base_name = fd->bn_string;
		fd->json.obj.string_bn_obj.name = fd->name_string;
		fd->json.obj.string_bn_obj.val_string = pt_buffer;

	} else {
		descr = senml_obj_lnk_descr;
		descr_len = ARRAY_SIZE(senml_obj_lnk_descr);
		obj_payload = &fd->json.obj.string_obj;
		fd->json.obj.string_obj.name = fd->name_string;
		fd->json.obj.string_obj.val_string = pt_buffer;
	}

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

	len = number_to_string(pt_buffer, sizeof(pt_buffer), "%d", value);
	if (len < 0) {
		return len;
	}

	return json_float_object_write(out, fd, len);
}

static int put_s16(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, int16_t value)
{
	return put_s32(out, path, (int32_t)value);
}

static int put_s8(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, int8_t value)
{
	return put_s32(out, path, (int32_t)value);
}

static int put_s64(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, int64_t value)
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

	len = number_to_string(pt_buffer, sizeof(pt_buffer), "%lld", value);
	if (len < 0) {
		return len;
	}

	return json_float_object_write(out, fd, len);
}

static int put_time(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, time_t value)
{
	return put_s64(out, path, (int64_t)value);
}

static int put_string(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, char *buf,
			 size_t buflen)
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

static int put_float(struct lwm2m_output_context *out, struct lwm2m_obj_path *path,
			     double  *value)
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

static int put_bool(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, bool value)
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

#define SEN_BASE64_SIZE_T_MAX	((size_t) -1)

/*
 * Encode a buffer into base64 format
 */
static size_t base64_encoded_length(size_t slen)
{
	size_t n;

	if (slen == 0) {
		return 0;
	}

	n = slen / 3 + (slen % 3 != 0);

	if (n > (SEN_BASE64_SIZE_T_MAX - 1) / 4) {
		return -ENOMEM;
	}

	n *= 4;
	return  n;
}

static bool json_base64_encode_data(struct json_out_formatter_data *fd, const char *buf)
{
	if (fd->add_base_name_to_start) {
		if (fd->json.obj.opaque_bn_obj.val_opaque.start == buf) {
			return true;
		}
	} else if (fd->json.obj.opaque_obj.val_opaque.start == buf) {
		return true;
	}
	return false;
}

static int json_append_bytes_base64(const char *bytes, size_t len, void *data)
{
	struct json_out_formatter_data *fd = data;
	struct lwm2m_output_context *out;
	size_t temp_length = 0;

	out = fd->out;

	if (json_base64_encode_data(fd, bytes)) {
		if (base64_encode(CPKT_BUF_W_PTR(out->out_cpkt), CPKT_BUF_W_SIZE(out->out_cpkt),
				  &temp_length, bytes, len)) {
			/* No space available for base64 data */
			return -ENOMEM;
		}
		/* Update Data offset */
		out->out_cpkt->offset += temp_length;
	} else {
		if (buf_append(CPKT_BUF_WRITE(fd->out->out_cpkt), bytes, len) < 0) {
			return -ENOMEM;
		}
	}

	return 0;
}

static int put_opaque(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, char *buf,
			 size_t buflen)
{
	struct json_out_formatter_data *fd;
	int res, len;
	ssize_t o_len, encoded_length;
	const struct json_obj_descr *descr;
	size_t descr_len;
	void *obj_payload;

	fd = engine_get_out_user_data(out);

	if (!out->out_cpkt || !fd) {
		return -EINVAL;
	}

	if (init_object_name_parameters(fd, path)) {
		return -EINVAL;
	}

	encoded_length = base64_encoded_length(buflen);
	if (encoded_length < 0) {
		return -ENOMEM;
	}

	len = json_add_separator(out, fd);
	if (len < 0) {
		return len;
	}

	if (fd->add_base_name_to_start) {
		descr = senml_opaque_bn_descr;
		descr_len = ARRAY_SIZE(senml_opaque_bn_descr);
		obj_payload = &fd->json.obj.opaque_bn_obj;
		fd->json.obj.opaque_bn_obj.base_name = fd->bn_string;
		fd->json.obj.opaque_bn_obj.name = fd->name_string;
		fd->json.obj.opaque_bn_obj.val_opaque.start = buf;
		fd->json.obj.opaque_bn_obj.val_opaque.length = encoded_length;
	} else {
		descr = senml_opaque_descr;
		descr_len = ARRAY_SIZE(senml_opaque_descr);
		obj_payload = &fd->json.obj.opaque_obj;
		fd->json.obj.opaque_obj.name = fd->name_string;
		fd->json.obj.opaque_obj.val_opaque.start = buf;
		fd->json.obj.opaque_obj.val_opaque.length = encoded_length;
	}
	/* Store Out context pointer for data write */
	fd->out = out;

	/* Calculate length */
	o_len = json_calc_encoded_len(descr, descr_len, obj_payload);
	if (o_len < 0) {
		return -EINVAL;
	}

	/* Put Original payload length back */
	if (fd->add_base_name_to_start) {
		fd->json.obj.opaque_bn_obj.val_opaque.length = buflen;
	} else {
		fd->json.obj.opaque_obj.val_opaque.length = buflen;
	}

	/* Encode */
	res = json_obj_encode(descr, descr_len, obj_payload, json_append_bytes_base64, fd);
	if (res < 0) {
		return -ENOMEM;
	}

	json_postprefix(fd);
	len += o_len;
	return len;

}

static int put_objlnk(struct lwm2m_output_context *out, struct lwm2m_obj_path *path,
			 struct lwm2m_objlnk *value)
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

static int read_int(struct lwm2m_input_context *in, int64_t *value, bool accept_sign)
{
	struct json_in_formatter_data *fd;
	const uint8_t *buf;
	size_t i = 0;
	bool neg = false;
	char c;

	/* initialize values to 0 */
	*value = 0;

	fd = engine_get_in_user_data(in);
	if (!fd || !JSON_BIT_CHECK(fd->object_bit_field, JSON_V_TYPE)) {
		return -EINVAL;
	}

	size_t value_length = fd->senml_object.val_float.length;

	if (value_length == 0) {
		return -ENODATA;
	}

	buf = fd->senml_object.val_float.start;
	while (*(buf + i) && i < value_length) {
		c = *(buf + i);
		if (c == '-' && accept_sign && i == 0) {
			neg = true;
		} else if (isdigit(c) != 0) {
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

static int get_string(struct lwm2m_input_context *in, uint8_t *buf, size_t buflen)
{
	struct json_in_formatter_data *fd;
	size_t string_length;

	fd = engine_get_in_user_data(in);
	if (!fd || !JSON_BIT_CHECK(fd->object_bit_field, JSON_VS_TYPE)) {
		return -EINVAL;
	}

	string_length = strlen(fd->senml_object.val_string);

	if (string_length > buflen) {
		LOG_WRN("Buffer too small to accommodate string, truncating");
		string_length = buflen - 1;
	}
	memcpy(buf, fd->senml_object.val_string, string_length);

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
	const uint8_t *json_buf;

	fd = engine_get_in_user_data(in);
	if (!fd || !JSON_BIT_CHECK(fd->object_bit_field, JSON_V_TYPE)) {
		return -EINVAL;
	}

	size_t value_length = fd->senml_object.val_float.length;

	if (value_length == 0) {
		return -ENODATA;
	}

	json_buf = fd->senml_object.val_float.start;
	while (*(json_buf + len) && len < value_length) {
		tmp = *(json_buf + len);

		if ((tmp == '-' && i == 0) || (tmp == '.' && !has_dot) || isdigit(tmp) != 0) {
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
	}

	return len;
}

static int get_bool(struct lwm2m_input_context *in, bool *value)
{
	struct json_in_formatter_data *fd;

	fd = engine_get_in_user_data(in);
	if (!fd || !JSON_BIT_CHECK(fd->object_bit_field, JSON_VB_TYPE)) {
		return -EINVAL;
	}

	*value = fd->senml_object.val_bool;

	return 1;
}

static void base64_url_safe_decode(uint8_t *data_buf, size_t buf_len)
{
	uint8_t *ptr = data_buf;

	for (size_t i = 0; i < buf_len; i++) {
		if (*ptr == '-') {
			/* replace '-' with "+" */
			*ptr = '+';
		} else if (*ptr == '_') {
			/* replace '_' with "/" */
			*ptr = '/';
		}
		ptr++;
	}
}

static int store_padded_modulo(uint16_t *padded_length, uint8_t *padded_buf, uint8_t *data_tail,
			       uint16_t data_length)
{
	uint16_t padded_len = BASE64_MODULO_LENGTH(data_length);

	if (data_length < padded_len) {
		return -ENODATA;
	}
	*padded_length = padded_len;

	if (padded_len) {
		uint8_t *tail_ptr;

		tail_ptr = data_tail - padded_len;
		memcpy(padded_buf, tail_ptr, padded_len);
		for (size_t i = padded_len; i < BASE64_OUTPUT_MIN_LENGTH; i++) {
			padded_buf[i] = '=';
		}
	}
	return 0;
}

static int get_opaque(struct lwm2m_input_context *in, uint8_t *value, size_t buflen,
			 struct lwm2m_opaque_context *opaque, bool *last_block)
{
	struct json_in_formatter_data *fd;
	struct json_obj_token *val_opaque;
	uint8_t *data_ptr;
	int in_len, ret;
	uint16_t padded_length = 0;
	uint8_t padded_buf[BASE64_OUTPUT_MIN_LENGTH];
	size_t buffer_base64_length;

	fd = engine_get_in_user_data(in);
	if (!fd || !JSON_BIT_CHECK(fd->object_bit_field, JSON_VD_TYPE)) {
		return -EINVAL;
	}
	val_opaque = &fd->senml_object.val_opaque;

	data_ptr = val_opaque->start;

	/* Decode from url safe to normal */
	base64_url_safe_decode(data_ptr, val_opaque->length);

	if (opaque->offset == 0) {
		size_t original_size = val_opaque->length;
		size_t base64_length;

		ret = store_padded_modulo(&padded_length, padded_buf, data_ptr + original_size,
					  original_size);
		if (ret) {
			return ret;
		}

		if (base64_decode(data_ptr, val_opaque->length, &base64_length, data_ptr,
				  val_opaque->length) < 0) {
			return -ENODATA;
		}

		val_opaque->length = base64_length;
		if (padded_length) {
			if (base64_decode(padded_buf, BASE64_OUTPUT_MIN_LENGTH,
					  &buffer_base64_length, padded_buf,
					  BASE64_OUTPUT_MIN_LENGTH) < 0) {
				return -ENODATA;
			}
			/* Add padded tail */
			memcpy(data_ptr + val_opaque->length, padded_buf, buffer_base64_length);
			val_opaque->length += buffer_base64_length;
		}
		opaque->len = val_opaque->length;
	}

	in_len = opaque->len - opaque->offset;

	if (in_len > buflen) {
		in_len = buflen;
	}

	if (in_len > val_opaque->length) {
		in_len = val_opaque->length;
	}

	if (opaque->offset + in_len >= opaque->len) {
		*last_block = true;
	}
	/* Copy data to buffer */
	memcpy(value, data_ptr, in_len);

	return in_len;
}

static int get_objlnk(struct lwm2m_input_context *in, struct lwm2m_objlnk *value)
{
	int64_t tmp;
	int len, total_len;
	struct json_in_formatter_data *fd;
	char *demiliter_pos;

	fd = engine_get_in_user_data(in);
	if (!fd || !JSON_BIT_CHECK(fd->object_bit_field, JSON_VLO_TYPE)) {
		return -EINVAL;
	}

	demiliter_pos = strchr(fd->senml_object.val_object_link, ':');
	if (!demiliter_pos) {
		return -ENODATA;
	}

	fd->object_bit_field |= JSON_V_TYPE;
	fd->senml_object.val_float.start = fd->senml_object.val_object_link;
	fd->senml_object.val_float.length = strlen(fd->senml_object.val_object_link);

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
	fd->senml_object.val_float.start = demiliter_pos;
	fd->senml_object.val_float.length = strlen(demiliter_pos);

	len = read_int(in, &tmp, false);
	if (len <= 0) {
		return -ENODATA;
	}

	total_len += len;
	value->obj_inst = (uint16_t)tmp;

	return total_len;
}

static int put_data_timestamp(struct lwm2m_output_context *out, time_t value)
{
	struct json_out_formatter_data *fd;
	int len;

	fd = engine_get_out_user_data(out);

	if (!out->out_cpkt || !fd) {
		LOG_ERR("Timestamp fail");
		return -EINVAL;
	}

	len = number_to_string(fd->timestamp_buffer, sizeof(fd->timestamp_buffer), "%lld",
			       value - fd->base_time);

	if (len < 0) {
		return len;
	}

	if (fd->base_time == 0) {
		/* Store  base time */
		fd->base_time = value;
	}

	fd->timestamp_length = len;
	fd->historical_data = true;

	return 0;
}

const struct lwm2m_writer senml_json_writer = {
	.put_begin = put_begin,
	.put_end = put_end,
	.put_begin_oi = put_begin_oi,
	.put_begin_ri = put_begin_ri,
	.put_end_ri = put_end_ri,
	.put_end_r = put_end_r,
	.put_s8 = put_s8,
	.put_s16 = put_s16,
	.put_s32 = put_s32,
	.put_s64 = put_s64,
	.put_string = put_string,
	.put_time = put_time,
	.put_float = put_float,
	.put_bool = put_bool,
	.put_opaque = put_opaque,
	.put_objlnk = put_objlnk,
	.put_data_timestamp = put_data_timestamp,
};

const struct lwm2m_reader senml_json_reader = {
	.get_s32 = get_s32,
	.get_s64 = get_s64,
	.get_string = get_string,
	.get_time = get_time,
	.get_float = get_float,
	.get_bool = get_bool,
	.get_opaque = get_opaque,
	.get_objlnk = get_objlnk,
};

int do_read_op_senml_json(struct lwm2m_message *msg)
{
	struct lwm2m_obj_path_list temp;
	sys_slist_t lwm2m_path_list;
	int ret;
	struct json_out_formatter_data fd;

	(void)memset(&fd, 0, sizeof(fd));
	engine_set_out_user_data(&msg->out, &fd);
	/* Init list */
	sys_slist_init(&lwm2m_path_list);
	/* Init message here ready for response */
	temp.path = msg->path;
	/* Add one entry to list */
	sys_slist_append(&lwm2m_path_list, &temp.node);

	ret = lwm2m_perform_read_op(msg, LWM2M_FORMAT_APP_SEML_JSON);
	engine_clear_out_user_data(&msg->out);

	return ret;
}

static int lwm2m_senml_write_operation(struct lwm2m_message *msg, struct json_in_formatter_data *fd)
{
	struct lwm2m_engine_obj_field *obj_field = NULL;
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_engine_res *res = NULL;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	uint8_t created;
	int ret = 0;

	/* handle resource value */
	/* reset values */
	created = 0U;

	/* if valid, use the return value as level */
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
		/* if OPTIONAL and BOOTSTRAP-WRITE or CREATE use ENOTSUP */
		if ((msg->ctx->bootstrap_mode ||
				msg->operation == LWM2M_OP_CREATE) &&
			LWM2M_HAS_PERM(obj_field, BIT(LWM2M_FLAG_OPTIONAL))) {
			ret = -ENOTSUP;
			return ret;
		}

		ret = -ENOENT;
		return ret;
	}

	/* Write the resource value */
	ret = lwm2m_write_handler(obj_inst, res, res_inst, obj_field, msg);
	if (ret == -EACCES || ret == -ENOENT) {
		/* if read-only or non-existent data buffer move on */
		ret = 0;
	}

	return ret;
}

int do_write_op_senml_json(struct lwm2m_message *msg)
{
	struct json_in_formatter_data fd;
	struct json_obj json_object;
	struct lwm2m_obj_path resource_path;
	int ret = 0;
	uint8_t full_name[MAX_RESOURCE_LEN + 1] = {0};
	const char *base_name_ptr = NULL;
	char *data_ptr;
	uint16_t in_len;

	(void)memset(&fd, 0, sizeof(fd));
	engine_set_in_user_data(&msg->in, &fd);

	if (msg->in.block_ctx) {
		LOG_ERR("Json library not support Coap Block");
		ret = -EOPNOTSUPP;
		goto end_of_operation;
	}

	data_ptr = (char *)coap_packet_get_payload(msg->in.in_cpkt, &in_len);

	/* When No blockwise do Normal Init */
	if (json_arr_separate_object_parse_init(&json_object, data_ptr, in_len)) {
		ret = -EINVAL;
		goto end_of_operation;
	}

	while (1) {

		(void)memset(&fd.senml_object, 0, sizeof(fd.senml_object));
		fd.object_bit_field = json_arr_separate_parse_object(&json_object, senml_descr,
						  ARRAY_SIZE(senml_descr), &fd.senml_object);
		if (fd.object_bit_field == 0) {
			/* End of  */
			break;
		} else if (fd.object_bit_field < 0 ||
			   ((fd.object_bit_field & JSON_NAME_MASK) == 0 ||
			    (fd.object_bit_field & JSON_VALUE_MASK) == 0)) {
			LOG_ERR("Json Write Parse object fail %d", fd.object_bit_field);
			ret = -EINVAL;
			goto end_of_operation;
		}

		if (JSON_BIT_CHECK(fd.object_bit_field, JSON_BN_TYPE)) {
			base_name_ptr = fd.senml_object.base_name;
		}

		/* Create object resource path */
		if (base_name_ptr) {
			if (JSON_BIT_CHECK(fd.object_bit_field, JSON_N_TYPE)) {
				ret = snprintk(full_name, sizeof(full_name), "%s%s", base_name_ptr,
				       fd.senml_object.name);
			} else {
				ret = snprintk(full_name, sizeof(full_name), "%s", base_name_ptr);
			}
		} else {
			ret = snprintk(full_name, sizeof(full_name), "%s", fd.senml_object.name);
		}

		if (ret >= MAX_RESOURCE_LEN) {
			ret = -EINVAL;
			goto end_of_operation;
		}

		ret = lwm2m_string_to_path(full_name, &resource_path, '/');
		if (ret < 0) {
			LOG_ERR("Relative name too long");
			ret = -EINVAL;
			goto end_of_operation;
		}

		msg->path = resource_path;
		ret = lwm2m_senml_write_operation(msg, &fd);

		/*
		 * for OP_CREATE and BOOTSTRAP WRITE: errors on
		 * optional resources are ignored (ENOTSUP)
		 */
		if (ret < 0 && !((ret == -ENOTSUP) &&
				(msg->ctx->bootstrap_mode || msg->operation == LWM2M_OP_CREATE))) {
			goto end_of_operation;
		}
	}

	ret = 0;

end_of_operation:
	engine_clear_in_user_data(&msg->in);

	return ret;
}

static uint8_t json_parse_composite_read_paths(struct lwm2m_message *msg,
					       sys_slist_t *lwm2m_path_list,
					       sys_slist_t *lwm2m_path_free_list)
{
	struct lwm2m_obj_path path;
	struct json_obj json_object;
	struct senml_context ts;

	uint8_t full_name[MAX_RESOURCE_LEN + 1] = {0};
	uint8_t valid_path_cnt = 0;
	const char *base_name_ptr = NULL;
	char *data_ptr;
	int bit_field;
	uint16_t in_len;
	int ret;

	data_ptr = (char *)coap_packet_get_payload(msg->in.in_cpkt, &in_len);

	if (json_arr_separate_object_parse_init(&json_object, data_ptr, in_len)) {
		return 0;
	}

	while (1) {
		bit_field = json_arr_separate_parse_object(&json_object, senml_descr,
						  ARRAY_SIZE(senml_descr), &ts);
		if (bit_field < 0) {
			LOG_ERR("Json Read Parse object fail %d", bit_field);
			break;
		} else if (JSON_BIT_CHECK(bit_field, JSON_NAME_MASK) == 0) {
			break;
		}

		if (JSON_BIT_CHECK(bit_field, JSON_BN_TYPE)) {
			base_name_ptr = ts.base_name;
		}

		/* Create object resource path */
		if (base_name_ptr) {
			if (JSON_BIT_CHECK(bit_field, JSON_N_TYPE)) {
				ret = snprintk(full_name, sizeof(full_name), "%s%s", base_name_ptr,
				       ts.name);
			} else {
				ret = snprintk(full_name, sizeof(full_name), "%s", base_name_ptr);
			}
		} else {
			ret = snprintk(full_name, sizeof(full_name), "%s", ts.name);
		}

		if (ret < MAX_RESOURCE_LEN) {
			ret = lwm2m_string_to_path(full_name, &path, '/');
			if (ret == 0) {
				if (lwm2m_engine_add_path_to_list(
					    lwm2m_path_list, lwm2m_path_free_list, &path) == 0) {
					valid_path_cnt++;
				}
			}
		}
	}

	return valid_path_cnt;
}

int do_composite_read_op_for_parsed_list_senml_json(struct lwm2m_message *msg,
						    sys_slist_t *path_list)
{
	int ret;
	struct json_out_formatter_data fd;

	(void)memset(&fd, 0, sizeof(fd));
	engine_set_out_user_data(&msg->out, &fd);

	ret = lwm2m_perform_composite_read_op(msg, LWM2M_FORMAT_APP_SEML_JSON, path_list);
	engine_clear_out_user_data(&msg->out);

	return ret;
}

int do_composite_read_op_senml_json(struct lwm2m_message *msg)
{
	struct lwm2m_obj_path_list path_list_buf[CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE];
	sys_slist_t path_list;
	sys_slist_t free_list;
	uint8_t path_list_size;

	/* Init list */
	lwm2m_engine_path_list_init(&path_list, &free_list, path_list_buf,
				    CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE);

	/* Parse Path's from SenML JSO payload */
	path_list_size = json_parse_composite_read_paths(msg, &path_list, &free_list);
	if (path_list_size == 0) {
		LOG_ERR("No Valid Url at msg");
		return -ESRCH;
	}

	/* Clear path which are part are part of recursive path /1 will include /1/0/1 */
	lwm2m_engine_clear_duplicate_path(&path_list, &free_list);

	return do_composite_read_op_for_parsed_list(msg, LWM2M_FORMAT_APP_SEML_JSON, &path_list);
}

int do_send_op_senml_json(struct lwm2m_message *msg, sys_slist_t *lwm2m_path_list)
{
	struct json_out_formatter_data fd;
	int ret;

	(void)memset(&fd, 0, sizeof(fd));
	engine_set_out_user_data(&msg->out, &fd);

	ret = lwm2m_perform_composite_read_op(msg, LWM2M_FORMAT_APP_SEML_JSON, lwm2m_path_list);
	engine_clear_out_user_data(&msg->out);

	return ret;
}

int do_composite_observe_parse_path_senml_json(struct lwm2m_message *msg,
					       sys_slist_t *lwm2m_path_list,
					       sys_slist_t *lwm2m_path_free_list)
{
	uint8_t list_size;
	uint16_t original_offset;

	original_offset = msg->in.offset;
	/* Parse Path's from SenML JSON payload */
	list_size = json_parse_composite_read_paths(msg, lwm2m_path_list, lwm2m_path_free_list);
	if (list_size == 0) {
		LOG_ERR("No Valid Url at msg");
		return -ESRCH;
	}

	msg->in.offset = original_offset;
	return 0;
}
