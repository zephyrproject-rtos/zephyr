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
 * - Research using Zephyr JSON lib for json_next_token()
 */

#define LOG_MODULE_NAME net_lwm2m_json
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>

#include "lwm2m_object.h"
#include "lwm2m_rw_json.h"
#include "lwm2m_rw_plain_text.h"
#include "lwm2m_engine.h"

#define T_OBJECT_BEGIN	BIT(0)
#define T_OBJECT_END	BIT(1)
#define T_STRING_BEGIN	BIT(2)
#define T_STRING_END	BIT(3)
#define T_VALUE		BIT(4)

#define SEPARATOR(f)	((f & WRITER_OUTPUT_VALUE) ? "," : "")

/* writer modes */
#define MODE_NONE      0
#define MODE_INSTANCE  1
#define MODE_VALUE     2
#define MODE_READY     3

#define TOKEN_BUF_LEN	64

struct json_out_formatter_data {
	/* offset position storage */
	uint16_t mark_pos_ri;

	/* flags */
	uint8_t writer_flags;

	/* path storage */
	uint8_t path_level;
};

struct json_in_formatter_data {
	/* name info */
	uint16_t name_offset;
	uint16_t name_len;

	/* value info */
	uint16_t value_offset;
	uint16_t value_len;

	/* state */
	uint16_t offset;

	/* flags */
	uint8_t json_flags;
};

/* some temporary buffer space for format conversions */
static char json_buffer[TOKEN_BUF_LEN];

static void json_add_char(struct lwm2m_input_context *in,
			  struct json_in_formatter_data *fd)
{
	if ((fd->json_flags & T_VALUE) ||
	    ((fd->json_flags & T_STRING_BEGIN) &&
	    !(fd->json_flags & T_STRING_END))) {
		if (fd->json_flags & T_VALUE) {
			fd->value_len++;
			if (fd->value_len == 1U) {
				fd->value_offset = fd->offset;
			}
		} else {
			fd->name_len++;
			if (fd->name_len == 1U) {
				fd->name_offset = fd->offset;
			}
		}
	}
}

/* Simlified JSON style reader for reading in values from a LWM2M JSON string */
static int json_next_token(struct lwm2m_input_context *in,
			   struct json_in_formatter_data *fd)
{
	uint8_t cont, c = 0;
	bool escape = false;

	(void)memset(fd, 0, sizeof(*fd));
	cont = 1U;

	/* We will be either at start, or at a specific position */
	while (in->offset < in->in_cpkt->offset && cont) {
		fd->offset = in->offset;
		if (buf_read_u8(&c, CPKT_BUF_READ(in->in_cpkt),
				&in->offset) < 0) {
			break;
		}

		if (c == '\\') {
			escape = true;
			/* Keep track of the escape codes */
			json_add_char(in, fd);
			continue;
		}

		switch (c) {

		case '[':
			if (!escape) {
				fd->json_flags |= T_OBJECT_BEGIN;
				cont = 0U;
			} else {
				json_add_char(in, fd);
			}
			break;
		case ']':
			if (!escape) {
				fd->json_flags |= T_OBJECT_END;
				cont = 0U;
			} else {
				json_add_char(in, fd);
			}
			break;
		case '{':
			if (!escape) {
				fd->json_flags |= T_OBJECT_BEGIN;
			} else {
				json_add_char(in, fd);
			}

			break;

		case '}':
		case ',':
			if (!escape) {
				cont = 0U;
			} else {
				json_add_char(in, fd);
			}

			break;

		case '"':
			if (!escape) {
				if (fd->json_flags & T_STRING_BEGIN) {
					fd->json_flags &= ~T_STRING_BEGIN;
					fd->json_flags |= T_STRING_END;
				} else {
					fd->json_flags &= ~T_STRING_END;
					fd->json_flags |= T_STRING_BEGIN;
				}
			} else {
				json_add_char(in, fd);
			}

			break;

		case ':':
			if (!escape) {
				fd->json_flags &= ~T_STRING_END;
				fd->json_flags |= T_VALUE;
			} else {
				json_add_char(in, fd);
			}

			break;

		/* ignore whitespace */
		case ' ':
		case '\n':
		case '\t':
			if (!(fd->json_flags & T_STRING_BEGIN)) {
				break;
			}

			__fallthrough;

		default:
			json_add_char(in, fd);

		}

		if (escape) {
			escape = false;
		}
	}

	/* OK if cont == 0 othewise we failed */
	return (cont == 0U);
}

static size_t put_begin(struct lwm2m_output_context *out,
			struct lwm2m_obj_path *path)
{
	int len = -1;

	if (path->level >= 2U) {
		len = snprintk(json_buffer, sizeof(json_buffer),
			       "{\"bn\":\"/%u/%u/\",\"e\":[",
			       path->obj_id, path->obj_inst_id);
	} else {
		len = snprintk(json_buffer, sizeof(json_buffer),
			       "{\"bn\":\"/%u/\",\"e\":[",
			       path->obj_id);
	}

	if (len < 0) {
		/* TODO: Generate error? */
		return 0;
	}


	if (buf_append(CPKT_BUF_WRITE(out->out_cpkt), json_buffer, len) < 0) {
		/* TODO: Generate error? */
		return 0;
	}

	return (size_t)len;
}

static size_t put_end(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path)
{
	if (buf_append(CPKT_BUF_WRITE(out->out_cpkt), "]}", 2) < 0) {
		/* TODO: Generate error? */
		return 0;
	}

	return 2;
}

static size_t put_begin_ri(struct lwm2m_output_context *out,
			   struct lwm2m_obj_path *path)
{
	struct json_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return 0;
	}

	fd->writer_flags |= WRITER_RESOURCE_INSTANCE;
	return 0;
}

static size_t put_end_ri(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path)
{
	struct json_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return 0;
	}

	fd->writer_flags &= ~WRITER_RESOURCE_INSTANCE;
	return 0;
}

static size_t put_char(struct lwm2m_output_context *out,
		       char c)
{
	if (buf_append(CPKT_BUF_WRITE(out->out_cpkt), &c, sizeof(c)) < 0) {
		/* TODO: Generate error? */
		return 0;
	}

	return 1;
}

static size_t put_json_prefix(struct lwm2m_output_context *out,
			      struct lwm2m_obj_path *path,
			      const char *format)
{
	struct json_out_formatter_data *fd;
	char *sep;
	int len = 0;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return 0;
	}

	sep = SEPARATOR(fd->writer_flags);
	if (fd->path_level >= 2U) {
		if (fd->writer_flags & WRITER_RESOURCE_INSTANCE) {
			len = snprintk(json_buffer, sizeof(json_buffer),
				       "%s{\"n\":\"%u/%u\",%s:",
				       sep, path->res_id, path->res_inst_id,
				       format);
		} else {
			len = snprintk(json_buffer, sizeof(json_buffer),
				       "%s{\"n\":\"%u\",%s:",
				       sep, path->res_id, format);
		}
	} else {
		if (fd->writer_flags & WRITER_RESOURCE_INSTANCE) {
			len = snprintk(json_buffer, sizeof(json_buffer),
				       "%s{\"n\":\"%u/%u/%u\",%s:",
				       sep, path->obj_inst_id, path->res_id,
				       path->res_inst_id, format);
		} else {
			len = snprintk(json_buffer, sizeof(json_buffer),
				       "%s{\"n\":\"%u/%u\",%s:",
				       sep, path->obj_inst_id, path->res_id,
				       format);
		}
	}

	if (len < 0) {
		/* TODO: Generate error? */
		return 0;
	}

	if (buf_append(CPKT_BUF_WRITE(out->out_cpkt), json_buffer, len) < 0) {
		/* TODO: Generate error? */
		return 0;
	}

	return len;
}

static size_t put_json_postfix(struct lwm2m_output_context *out)
{
	struct json_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return 0;
	}

	if (put_char(out, '}') < 1) {
		/* TODO: Generate error? */
		return 0;
	}

	fd->writer_flags |= WRITER_OUTPUT_VALUE;
	return 1;
}

static size_t put_s32(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, int32_t value)
{
	int len;

	len = put_json_prefix(out, path, "\"v\"");
	len += plain_text_put_format(out, "%d", value);
	len += put_json_postfix(out);

	return (size_t)len;
}

static size_t put_s16(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, int16_t value)
{
	return put_s32(out, path, (int32_t)value);
}

static size_t put_s8(struct lwm2m_output_context *out,
		     struct lwm2m_obj_path *path, int8_t value)
{
	return put_s32(out, path, (int32_t)value);
}

static size_t put_s64(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, int64_t value)
{
	int len;

	len = put_json_prefix(out, path, "\"v\"");
	len += plain_text_put_format(out, "%lld", value);
	len += put_json_postfix(out);
	return (size_t)len;
}

static size_t put_string(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path,
			 char *buf, size_t buflen)
{
	size_t i;
	size_t len = 0;
	int res;

	res = put_json_prefix(out, path, "\"sv\"");
	res += put_char(out, '"');

	if (res < 0) {
		/* TODO: Generate error? */
		return 0;
	}

	len += res;

	for (i = 0; i < buflen; ++i) {
		/* Escape special characters */
		/* TODO: Handle UTF-8 strings */
		if (buf[i] < '\x20') {
			res = snprintk(json_buffer, sizeof(json_buffer),
				       "\\x%x", buf[i]);
			if (res < 0) {
				/* TODO: Generate error? */
				return 0;
			}

			if (buf_append(CPKT_BUF_WRITE(out->out_cpkt),
				       json_buffer, res) < 0) {
				/* TODO: Generate error? */
				return 0;
			}

			len += res;
			continue;
		} else if (buf[i] == '"' || buf[i] == '\\') {
			len += put_char(out, '\\');
		}

		len += put_char(out, buf[i]);
	}

	res = put_char(out, '"');
	if (res < 0) {
		/* TODO: Generate error? */
		return 0;
	}

	len += res;
	len += put_json_postfix(out);
	return len;
}

static size_t put_float32fix(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path,
			     float32_value_t *value)
{
	size_t len;

	len = put_json_prefix(out, path, "\"v\"");
	len += plain_text_put_float32fix(out, path, value);
	len += put_json_postfix(out);
	return len;
}

static size_t put_float64fix(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path,
			     float64_value_t *value)
{
	size_t len;

	len = put_json_prefix(out, path, "\"v\"");
	len += plain_text_put_float64fix(out, path, value);
	len += put_json_postfix(out);
	return len;
}

static size_t put_bool(struct lwm2m_output_context *out,
		       struct lwm2m_obj_path *path,
		       bool value)
{
	size_t len;

	len = put_json_prefix(out, path, "\"bv\"");
	len += plain_text_put_format(out, "%s", value ? "true" : "false");
	len += put_json_postfix(out);
	return (size_t)len;
}

static size_t put_objlnk(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path,
			 struct lwm2m_objlnk *value)
{
	size_t len;

	len = put_json_prefix(out, path, "\"ov\"");
	len += plain_text_put_format(out, "\"%u:%u\"", value->obj_id,
				     value->obj_inst);
	len += put_json_postfix(out);

	return len;
}

static size_t read_number(struct lwm2m_input_context *in,
			  int64_t *value1, int64_t *value2,
			  bool accept_sign, bool accept_dot)
{
	struct json_in_formatter_data *fd;
	int64_t *counter = value1;
	uint8_t *buf;
	size_t i = 0;
	bool neg = false;
	bool dot_found = false;
	char c;

	/* initialize values to 0 */
	*value1 = 0;
	if (value2) {
		*value2 = 0;
	}

	fd = engine_get_in_user_data(in);
	if (!fd) {
		return 0;
	}

	buf = in->in_cpkt->data + fd->value_offset;
	while (*(buf + i) && i < fd->value_len) {
		c = *(buf + i);
		if (c == '-' && accept_sign && i == 0) {
			neg = true;
		} else if (c == '.' && i > 0 && accept_dot && !dot_found &&
			   value2) {
			dot_found = true;
			counter = value2;
		} else if (isdigit(c)) {
			*counter = *counter * 10 + (c - '0');
		} else {
			/* anything else stop reading */
			break;
		}

		i++;
	}

	if (neg) {
		*value1 = -*value1;
	}

	return i;
}

static size_t get_s64(struct lwm2m_input_context *in, int64_t *value)
{
	return read_number(in, value, NULL, true, true);
}

static size_t get_s32(struct lwm2m_input_context *in, int32_t *value)
{
	int64_t tmp = 0;
	size_t len = 0;

	len = read_number(in, &tmp, NULL, true, true);
	if (len > 0) {
		*value = (int32_t)tmp;
	}

	return len;
}

static size_t get_string(struct lwm2m_input_context *in,
			 uint8_t *buf, size_t buflen)
{
	struct json_in_formatter_data *fd;
	int ret;

	fd = engine_get_in_user_data(in);
	if (!fd) {
		return 0;
	}

	if (fd->value_len > buflen) {
		/* TODO: generate warning? */
		fd->value_len = buflen - 1;
	}

	/* TODO: Handle escape codes */
	ret = buf_read(buf, fd->value_len, CPKT_BUF_READ(in->in_cpkt),
		       &fd->value_offset);
	if (ret < 0) {
		return 0;
	}

	return fd->value_len;
}

static size_t get_float32fix(struct lwm2m_input_context *in,
			     float32_value_t *value)
{
	int64_t tmp1, tmp2;
	size_t len;

	len = read_number(in, &tmp1, &tmp2, true, true);
	if (len > 0) {
		value->val1 = (int32_t)tmp1;
		value->val2 = (int32_t)tmp2;
	}

	return len;
}

static size_t get_float64fix(struct lwm2m_input_context *in,
			     float64_value_t *value)
{
	int64_t tmp1, tmp2;
	size_t len;

	len = read_number(in, &tmp1, &tmp2, true, true);
	if (len > 0) {
		value->val1 = tmp1;
		value->val2 = tmp2;
	}

	return len;
}

static size_t get_bool(struct lwm2m_input_context *in, bool *value)
{
	struct json_in_formatter_data *fd;

	fd = engine_get_in_user_data(in);
	if (!fd) {
		return 0;
	}

	if (strncmp(in->in_cpkt->data + fd->value_offset,
		    "true", 4) == 0) {
		*value = true;
	} else if (strncmp(in->in_cpkt->data + fd->value_offset,
		   "false", 5) == 0) {
		*value = false;
	}

	return fd->value_len;
}

static size_t get_opaque(struct lwm2m_input_context *in,
			 uint8_t *value, size_t buflen,
			 struct lwm2m_opaque_context *opaque,
			 bool *last_block)
{
	/* TODO */
	return 0;
}

static size_t get_objlnk(struct lwm2m_input_context *in,
			 struct lwm2m_objlnk *value)
{
	int64_t tmp;
	size_t len;
	uint16_t value_offset;
	struct json_in_formatter_data *fd;

	fd = engine_get_in_user_data(in);
	if (!fd) {
		return 0;
	}

	/* Store the original value offset. */
	value_offset = fd->value_offset;

	len = read_number(in, &tmp, NULL, false, false);
	value->obj_id = (uint16_t)tmp;

	len++;  /* +1 for ':' delimeter. */
	fd->value_offset += len;

	len += read_number(in, &tmp, NULL, false, false);
	value->obj_inst = (uint16_t)tmp;

	/* Restore the original value offset. */
	fd->value_offset = value_offset;

	return len;
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
	.put_float32fix = put_float32fix,
	.put_float64fix = put_float64fix,
	.put_bool = put_bool,
	.put_objlnk = put_objlnk,
};

const struct lwm2m_reader json_reader = {
	.get_s32 = get_s32,
	.get_s64 = get_s64,
	.get_string = get_string,
	.get_float32fix = get_float32fix,
	.get_float64fix = get_float64fix,
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
	/* save the level for output processing */
	fd.path_level = msg->path.level;
	ret = lwm2m_perform_read_op(msg, content_format);
	engine_clear_out_user_data(&msg->out);

	return ret;
}

static int parse_path(const uint8_t *buf, uint16_t buflen,
		      struct lwm2m_obj_path *path)
{
	int ret = 0;
	int pos = 0;
	uint16_t val;
	uint8_t c = 0U;

	(void)memset(path, 0, sizeof(*path));
	do {
		val = 0U;
		c = buf[pos];
		/* we should get a value first - consume all numbers */
		while (pos < buflen && isdigit(c)) {
			val = val * 10U + (c - '0');
			c = buf[++pos];
		}

		/* slash will mote thing forward */
		if (pos == 0 && c == '/') {
			/* skip leading slashes */
			pos++;
		} else if (c == '/' || pos == buflen) {
			LOG_DBG("Setting %u = %u", ret, val);
			if (ret == 0) {
				path->obj_id = val;
			} else if (ret == 1) {
				path->obj_inst_id = val;
			} else if (ret == 2) {
				path->res_id = val;
			} else if (ret == 3) {
				path->res_inst_id = val;
			}

			ret++;
			pos++;
		} else {
			LOG_ERR("Error: illegal char '%c' at pos:%d",
				c, pos);
			return -1;
		}
	} while (pos < buflen);

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
	int ret = 0, index;
	uint8_t value[TOKEN_BUF_LEN];
	uint8_t base_name[MAX_RESOURCE_LEN];
	uint8_t full_name[MAX_RESOURCE_LEN];
	uint8_t created;

	(void)memset(&fd, 0, sizeof(fd));
	engine_set_in_user_data(&msg->in, &fd);

	/* store a copy of the original path */
	memcpy(&orig_path, &msg->path, sizeof(msg->path));

	/* PARSE base name "bn" */
	json_next_token(&msg->in, &fd);
	/* TODO: validate name == "bn" */
	if (buf_read(base_name, fd.value_len,
		     CPKT_BUF_READ(msg->in.in_cpkt),
		     &fd.value_offset) < 0) {
		LOG_ERR("Error parsing base name!");
		return -EINVAL;
	}

	/* skip to elements */
	json_next_token(&msg->in, &fd);
	/* TODO: validate name == "bv" */

	while (json_next_token(&msg->in, &fd)) {

		if (!(fd.json_flags & T_VALUE)) {
			continue;
		}

		if (buf_read(value, fd.name_len,
			     CPKT_BUF_READ(msg->in.in_cpkt),
			     &fd.name_offset) < 0) {
			LOG_ERR("Error parsing name!");
			continue;
		}

		/* handle resource name */
		if (value[0] == 'n') {
			/* reset values */
			created = 0U;

			/* get value for relative path */
			if (buf_read(value, fd.value_len,
				     CPKT_BUF_READ(msg->in.in_cpkt),
				     &fd.value_offset) < 0) {
				LOG_ERR("Error parsing relative path!");
				continue;
			}

			/* combine base_name + name */
			snprintk(full_name, sizeof(full_name), "%s%s",
				 base_name, value);

			/* parse full_name into path */
			ret = parse_path(full_name, strlen(full_name),
					 &msg->path);
			if (ret < 0) {
				break;
			}

			/* if valid, use the return value as level */
			msg->path.level = ret;

			ret = lwm2m_get_or_create_engine_obj(msg, &obj_inst,
							     &created);
			if (ret < 0) {
				break;
			}

			obj_field = lwm2m_get_engine_obj_field(
							obj_inst->obj,
							msg->path.res_id);
			/*
			 * if obj_field is not found,
			 * treat as an optional resource
			 */
			if (!obj_field) {
				ret = -ENOENT;
				break;
			}

			/*
			 * TODO: support BOOTSTRAP WRITE where optional
			 * resources are ignored
			 */

			if (!LWM2M_HAS_PERM(obj_field, LWM2M_PERM_W)) {
				ret = -EPERM;
				break;
			}

			if (!obj_inst->resources ||
			    obj_inst->resource_count == 0U) {
				ret = -EINVAL;
				break;
			}

			for (index = 0; index < obj_inst->resource_count;
			     index++) {
				if (obj_inst->resources[index].res_id ==
				    msg->path.res_id) {
					res = &obj_inst->resources[index];
					break;
				}
			}

			if (!res) {
				ret = -ENOENT;
				break;
			}

			for (index = 0; index < res->res_inst_count; index++) {
				if (res->res_instances[index].res_inst_id ==
				    msg->path.res_inst_id) {
					res_inst = &res->res_instances[index];
					break;
				}
			}

			if (!res_inst) {
				ret = -ENOENT;
				break;
			}
		} else if (res && res_inst) {
			/* handle value assignment */
			ret = lwm2m_write_handler(obj_inst, res, res_inst,
						  obj_field, msg);
			if (orig_path.level >= 3U && ret < 0) {
				/* return errors on a single write */
				break;
			}
		} else {
			/* complain about error? */
		}
	}

	engine_clear_in_user_data(&msg->in);

	return ret;
}
