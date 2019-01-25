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

#define T_NONE		0
#define T_STRING_B	1
#define T_STRING	2
#define T_NAME		3
#define T_OBJ		4
#define T_VAL		5

#define SEPARATOR(f)	((f & WRITER_OUTPUT_VALUE) ? "," : "")

/* writer modes */
#define MODE_NONE      0
#define MODE_INSTANCE  1
#define MODE_VALUE     2
#define MODE_READY     3

#define TOKEN_BUF_LEN	64

struct json_data {
	u8_t name[TOKEN_BUF_LEN];
	u8_t value[TOKEN_BUF_LEN];
	u8_t name_len;
	u8_t value_len;
};

struct json_out_formatter_data {
	/* offset position storage */
	u16_t mark_pos_ri;
	u8_t writer_flags;
	u8_t path_level;
};

/* some temporary buffer space for format conversions */
static char json_buffer[64];

static void json_add_char(struct json_data *json, u8_t c, u8_t json_type)
{
	if (json_type == T_STRING_B) {
		json->name[json->name_len++] = c;
	} else if (json_type == T_STRING_B) {
		json->value[json->value_len++] = c;
	}
}

/* Simlified JSON style reader for reading in values from a LWM2M JSON string */
static int json_next_token(struct lwm2m_input_context *in,
			   struct json_data *json)
{
	u8_t json_type = T_NONE;
	u8_t cont;
	u8_t c;
	bool escape = false;

	(void)memset(json, 0, sizeof(struct json_data));
	cont = 1U;

	/* We will be either at start, or at a specific position */
	while (in->offset < in->in_cpkt->offset && cont) {
		if (buf_read_u8(&c, CPKT_BUF_READ(in->in_cpkt),
				&in->offset) < 0) {
			break;
		}

		if (c == '\\') {
			escape = true;
			continue;
		}

		switch (c) {

		case '{':
			if (!escape) {
				json_type = T_OBJ;
			} else {
				json_add_char(json, c, json_type);
			}

			break;

		case '}':
		case ',':
			if (!escape) {
				if (json_type == T_VAL ||
				    json_type == T_STRING) {
					json->value[json->value_len] = '\0';
					json_type = T_NONE;
					cont = 0U;
				}
			} else {
				json_add_char(json, c, json_type);
			}

			break;

		case '"':
			if (!escape && json_type == T_STRING_B) {
				json->name[json->name_len] = '\0';
				json_type = T_STRING;
			} else if (!escape) {
				json_type = T_STRING_B;
				json->name_len = 0U;
			} else {
				json_add_char(json, c, json_type);
			}

			break;

		case ':':
			if (json_type == T_STRING) {
				json_type = T_VAL;
			} else {
				json_add_char(json, c, json_type);
			}

			break;

		/* ignore whitespace */
		case ' ':
		case '\n':
		case '\t':
			if (json_type != T_STRING_B) {
				break;
			}

			/* fallthrough */

		default:
			json_add_char(json, c, json_type);

		}

		if (escape) {
			escape = false;
		}
	}

	/* OK if cont == 0 othewise we failed */
	return (cont == 0);
}

static size_t put_begin(struct lwm2m_output_context *out,
			struct lwm2m_obj_path *path)
{
	int len = -1;

	if (path->level >= 2) {
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
	if (fd->path_level >= 2) {
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

	if (put_char(out, '}') < 0) {
		/* TODO: Generate error? */
		return 0;
	}

	fd->writer_flags |= WRITER_OUTPUT_VALUE;
	return 1;
}

static size_t put_s32(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, s32_t value)
{
	int len;

	len = put_json_prefix(out, path, "\"v\"");
	len += plain_text_put_format(out, "%d", value);
	len += put_json_postfix(out);

	return (size_t)len;
}

static size_t put_s16(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, s16_t value)
{
	return put_s32(out, path, (s32_t)value);
}

static size_t put_s8(struct lwm2m_output_context *out,
		     struct lwm2m_obj_path *path, s8_t value)
{
	return put_s32(out, path, (s32_t)value);
}

static size_t put_s64(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, s64_t value)
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
	len += plain_text_put_format(out, "%d.%d", value->val1, value->val2);
	len += put_json_postfix(out);
	return len;
}

static size_t put_float64fix(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path,
			     float64_value_t *value)
{
	size_t len;

	len = put_json_prefix(out, path, "\"v\"");
	len += plain_text_put_format(out, "%lld.%lld",
				     value->val1, value->val2);
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
};

int do_read_op_json(struct lwm2m_engine_obj *obj, struct lwm2m_message *msg,
		    int content_format)
{
	struct json_out_formatter_data fd;
	int ret;

	(void)memset(&fd, 0, sizeof(fd));
	engine_set_out_user_data(&msg->out, &fd);
	/* save the level for output processing */
	fd.path_level = msg->path.level;
	ret = lwm2m_perform_read_op(obj, msg, content_format);
	engine_clear_out_user_data(&msg->out);

	return ret;
}

static int parse_path(const u8_t *buf, u16_t buflen,
		      struct lwm2m_obj_path *path)
{
	int ret = 0;
	int pos = 0;
	u16_t val;
	u8_t c = 0U;

	do {
		val = 0U;
		c = buf[pos];
		/* we should get a value first - consume all numbers */
		while (pos < buflen && isdigit(c)) {
			val = val * 10 + (c - '0');
			c = buf[++pos];
		}

		/*
		 * Slash will mote thing forward
		 * and the end will be when pos == pl
		 */
		if (c == '/' || pos == buflen) {
			LOG_DBG("Setting %u = %u", ret, val);
			if (ret == 0) {
				path->obj_id = val;
			} else if (ret == 1) {
				path->obj_inst_id = val;
			} else if (ret == 2) {
				path->res_id = val;
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

int do_write_op_json(struct lwm2m_engine_obj *obj, struct lwm2m_message *msg)
{
	struct lwm2m_engine_obj_field *obj_field;
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_engine_res_inst *res = NULL;
	struct json_data json;
	u8_t olv = 0U;
	u8_t created;
	int i, r;
	u8_t mode = MODE_NONE;

	olv = msg->path.level;

	while (json_next_token(&msg->in, &json)) {
		i = 0;
		created = 0U;
		if (json.name[0] == 'n') {
			msg->path.level = parse_path(json.value,
						     json.value_len,
						     &msg->path);
			if (i > 0) {
				r = lwm2m_get_or_create_engine_obj(msg,
								   &obj_inst,
								   &created);
				if (r < 0) {
					return r;
				}
				mode |= MODE_INSTANCE;
			}
		} else {
			/* HACK: assume value node: can it be anything else? */
			mode |= MODE_VALUE;
			/* FIXME swap msg->in.cpkt.pkt->frag w/ value buffer */
		}

		if (mode == MODE_READY) {
			if (!obj_inst) {
				return -EINVAL;
			}

			obj_field = lwm2m_get_engine_obj_field(obj,
							msg->path.res_id);
			/*
			 * if obj_field is not found,
			 * treat as an optional resource
			 */
			if (!obj_field) {
				/*
				 * TODO: support BOOTSTRAP WRITE where optional
				 * resources are ignored
				 */
				if (msg->operation != LWM2M_OP_CREATE) {
					return -ENOENT;
				}

				goto skip_optional;
			}

			if (!LWM2M_HAS_PERM(obj_field, LWM2M_PERM_W)) {
				return -EPERM;
			}

			if (!obj_inst->resources ||
			     obj_inst->resource_count == 0) {
				return -EINVAL;
			}

			for (i = 0; i < obj_inst->resource_count; i++) {
				if (obj_inst->resources[i].res_id ==
						msg->path.res_id) {
					res = &obj_inst->resources[i];
					break;
				}
			}

			if (!res) {
				return -ENOENT;
			}

			lwm2m_write_handler(obj_inst, res, obj_field, msg);

skip_optional:
			mode = MODE_NONE;
			/* FIXME: swap back original msg->in.cpkt.pkt->frag */
			msg->path.level = olv;
		}
	}

	return 0;
}
