/*
 * Copyright (c) 2017 Linaro Limited
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

#define SYS_LOG_DOMAIN "lib/lwm2m_json"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LWM2M_LEVEL
#include <logging/sys_log.h>

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

#include "lwm2m_object.h"
#include "lwm2m_rw_json.h"
#include "lwm2m_rw_plain_text.h"
#include "lwm2m_engine.h"

#define T_NONE		0
#define T_STRING_B	1
#define T_STRING	2
#define T_NAME		4
#define T_VNUM		5
#define T_OBJ		6
#define T_VAL		7

#define SEPARATOR(f)	((f & WRITER_OUTPUT_VALUE) ? "," : "")

/* writer modes */
#define MODE_NONE      0
#define MODE_INSTANCE  1
#define MODE_VALUE     2
#define MODE_READY     3

/* Simlified JSON style reader for reading in values from a LWM2M JSON string */
int json_next_token(struct lwm2m_input_context *in, struct json_data *json)
{
	int pos;
	u8_t type = T_NONE;
	u8_t vpos_start = 0;
	u8_t vpos_end = 0;
	u8_t cont;
	u8_t wscount = 0;
	u8_t c;

	json->name_len = 0;
	json->value_len = 0;
	cont = 1;
	pos = in->inpos;

	/* We will be either at start, or at a specific position */
	while (pos < in->insize && cont) {
		c = in->inbuf[pos++];

		switch (c) {

		case '{':
			type = T_OBJ;
			break;

		case '}':
		case ',':
			if (type == T_VAL || type == T_STRING) {
				json->value = &in->inbuf[vpos_start];
				json->value_len = vpos_end - vpos_start -
						  wscount;
				type = T_NONE;
				cont = 0;
			}

			wscount = 0;
			break;

		case '\\':
			/* stuffing */
			if (pos < in->insize) {
				pos++;
				vpos_end = pos;
			}

			break;

		case '"':
			if (type == T_STRING_B) {
				type = T_STRING;
				vpos_end = pos - 1;
				wscount = 0;
			} else {
				type = T_STRING_B;
				vpos_start = pos;
			}

			break;

		case ':':
			if (type == T_STRING) {
				json->name = &in->inbuf[vpos_start];
				json->name_len = vpos_end - vpos_start;
				vpos_start = vpos_end = pos;
				type = T_VAL;
			} else {
				/* Could be in string or at illegal pos */
				if (type != T_STRING_B) {
					SYS_LOG_ERR("ERROR - illegal ':'");
				}
			}

			break;

		/* ignore whitespace */
		case ' ':
		case '\n':
		case '\t':
			if (type != T_STRING_B) {
				if (vpos_start == pos - 1) {
					vpos_start = pos;
				} else {
					wscount++;
				}
			}

			/* fallthrough */

		default:
			vpos_end = pos;

		}
	}

	if (cont == 0 && pos < in->insize) {
		in->inpos = pos;
	}

	/* OK if cont == 0 othewise we failed */
	return (cont == 0 && pos < in->insize);
}

static size_t put_begin(struct lwm2m_output_context *out,
			struct lwm2m_obj_path *path)
{
	int len;

	len = snprintf(&out->outbuf[out->outlen],
		       out->outsize - out->outlen,
		       "{\"bn\":\"/%u/%u/\",\"e\":[",
		       path->obj_id, path->obj_inst_id);
	out->writer_flags = 0; /* set flags to zero */
	if (len < 0 || len >= out->outsize) {
		return 0;
	}

	out->outlen += len;
	return (size_t)len;
}

static size_t put_end(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path)
{
	int len;

	len = snprintf(&out->outbuf[out->outlen],
		       out->outsize - out->outlen, "]}");
	if (len < 0 || len >= (out->outsize - out->outlen)) {
		return 0;
	}

	out->outlen += len;
	return (size_t)len;
}

static size_t put_begin_ri(struct lwm2m_output_context *out,
			   struct lwm2m_obj_path *path)
{
	out->writer_flags |= WRITER_RESOURCE_INSTANCE;
	return 0;
}

static size_t put_end_ri(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path)
{
	out->writer_flags &= ~WRITER_RESOURCE_INSTANCE;
	return 0;
}

static size_t put_s32(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, s32_t value)
{
	u8_t *outbuf;
	size_t outlen;
	char *sep;
	int len;

	outbuf = &out->outbuf[out->outlen];
	outlen = out->outsize - out->outlen;
	sep = SEPARATOR(out->writer_flags);
	if (out->writer_flags & WRITER_RESOURCE_INSTANCE) {
		len = snprintf(outbuf, outlen, "%s{\"n\":\"%u/%u\",\"v\":%d}",
			       sep, path->res_id, path->res_inst_id, value);
	} else {
		len = snprintf(outbuf, outlen, "%s{\"n\":\"%u\",\"v\":%d}",
			       sep, path->res_id, value);
	}

	if (len < 0 || len >= outlen) {
		return 0;
	}

	SYS_LOG_DBG("JSON: Write int:%s", outbuf);
	out->writer_flags |= WRITER_OUTPUT_VALUE;
	out->outlen += len;
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
	u8_t *outbuf;
	size_t outlen;
	char *sep;
	int len;

	outbuf = &out->outbuf[out->outlen];
	outlen = out->outsize - out->outlen;
	sep = SEPARATOR(out->writer_flags);
	if (out->writer_flags & WRITER_RESOURCE_INSTANCE) {
		len = snprintf(outbuf, outlen, "%s{\"n\":\"%u/%u\",\"v\":%lld}",
			       sep, path->res_id, path->res_inst_id, value);
	} else {
		len = snprintf(outbuf, outlen, "%s{\"n\":\"%u\",\"v\":%lld}",
			       sep, path->res_id, value);
	}

	if (len < 0 || len >= outlen) {
		return 0;
	}

	SYS_LOG_DBG("JSON: Write int:%s", outbuf);
	out->writer_flags |= WRITER_OUTPUT_VALUE;
	out->outlen += len;
	return (size_t)len;
}

static size_t put_string(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path,
			 const char *value, size_t strlen)
{
	u8_t *outbuf;
	size_t outlen;
	char *sep;
	size_t i;
	size_t len = 0;
	int res;

	outbuf = &out->outbuf[out->outlen];
	outlen = out->outsize - out->outlen;
	sep = SEPARATOR(out->writer_flags);
	if (out->writer_flags & WRITER_RESOURCE_INSTANCE) {
		res = snprintf(outbuf, outlen, "%s{\"n\":\"%u/%u\",\"sv\":\"",
			       sep, path->res_id, path->res_inst_id);
	} else {
		res = snprintf(outbuf, outlen, "%s{\"n\":\"%u\",\"sv\":\"",
			       sep, path->res_id);
	}

	if (res < 0 || res >= outlen) {
		return 0;
	}

	len += res;
	for (i = 0; i < strlen && len < outlen; ++i) {
		/* Escape special characters */
		/* TODO: Handle UTF-8 strings */
		if (value[i] < '\x20') {
			res = snprintf(&outbuf[len], outlen - len, "\\x%x",
				       value[i]);

			if (res < 0 || res >= (outlen - len)) {
				return 0;
			}

			len += res;
			continue;
		} else if (value[i] == '"' || value[i] == '\\') {
			outbuf[len] = '\\';
			++len;
			if (len >= outlen) {
				return 0;
			}
		}

		outbuf[len] = value[i];
		++len;
		if (len >= outlen) {
			return 0;
		}
	}

	res = snprintf(&outbuf[len], outlen - len, "\"}");
	if (res < 0 || res >= (outlen - len)) {
		return 0;
	}

	SYS_LOG_DBG("JSON: Write string:%s", outbuf);
	len += res;
	out->writer_flags |= WRITER_OUTPUT_VALUE;
	out->outlen += len;
	return len;
}

static size_t put_float32fix(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path,
			     float32_value_t *value)
{
	u8_t *outbuf;
	size_t outlen;
	char *sep;
	size_t len = 0;
	int res;

	outbuf = &out->outbuf[out->outlen];
	outlen = out->outsize - out->outlen;
	sep = SEPARATOR(out->writer_flags);
	if (out->writer_flags & WRITER_RESOURCE_INSTANCE) {
		res = snprintf(outbuf, outlen, "%s{\"n\":\"%u/%u\",\"v\":",
			       sep, path->res_id, path->res_inst_id);
	} else {
		res = snprintf(outbuf, outlen, "%s{\"n\":\"%u\",\"v\":",
			       sep, path->res_id);
	}

	if (res <= 0 || res >= outlen) {
		return 0;
	}

	len += res;
	outlen -= res;
	res = plain_text_put_float32fix(&outbuf[len], outlen, value);
	if (res <= 0 || res >= outlen) {
		return 0;
	}

	len += res;
	outlen -= res;
	res = snprintf(&outbuf[len], outlen, "}");
	if (res <= 0 || res >= outlen) {
		return 0;
	}

	len += res;
	out->writer_flags |= WRITER_OUTPUT_VALUE;
	out->outlen += len;
	return len;
}

static size_t put_float64fix(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path,
			     float64_value_t *value)
{
	u8_t *outbuf;
	size_t outlen;
	char *sep;
	size_t len = 0;
	int res;

	outbuf = &out->outbuf[out->outlen];
	outlen = out->outsize - out->outlen;
	sep = SEPARATOR(out->writer_flags);
	if (out->writer_flags & WRITER_RESOURCE_INSTANCE) {
		res = snprintf(outbuf, outlen, "%s{\"n\":\"%u/%u\",\"v\":",
			       sep, path->res_id, path->res_inst_id);
	} else {
		res = snprintf(outbuf, outlen, "%s{\"n\":\"%u\",\"v\":",
			       sep, path->res_id);
	}

	if (res <= 0 || res >= outlen) {
		return 0;
	}

	len += res;
	outlen -= res;
	res = plain_text_put_float64fix(&outbuf[len], outlen, value);
	if (res <= 0 || res >= outlen) {
		return 0;
	}

	len += res;
	outlen -= res;
	res = snprintf(&outbuf[len], outlen, "}");
	if (res <= 0 || res >= outlen) {
		return 0;
	}

	len += res;
	out->writer_flags |= WRITER_OUTPUT_VALUE;
	out->outlen += len;
	return len;
}

static size_t put_bool(struct lwm2m_output_context *out,
		       struct lwm2m_obj_path *path,
		       bool value)
{
	u8_t *outbuf;
	size_t outlen;
	char *sep;
	int len;

	outbuf = &out->outbuf[out->outlen];
	outlen = out->outsize - out->outlen;
	sep = SEPARATOR(out->writer_flags);
	if (out->writer_flags & WRITER_RESOURCE_INSTANCE) {
		len = snprintf(outbuf, outlen, "%s{\"n\":\"%u/%u\",\"bv\":%s}",
			       sep, path->res_id, path->res_inst_id,
			       value ? "true" : "false");
	} else {
		len = snprintf(outbuf, outlen, "%s{\"n\":\"%u\",\"bv\":%s}",
			       sep, path->res_id, value ? "true" : "false");
	}

	if (len < 0 || len >= outlen) {
		return 0;
	}

	SYS_LOG_DBG("JSON: Write bool:%s", outbuf);
	out->writer_flags |= WRITER_OUTPUT_VALUE;
	out->outlen += len;
	return (size_t)len;
}

const struct lwm2m_writer json_writer = {
	put_begin,
	put_end,
	put_begin_ri,
	put_end_ri,
	put_s8,
	put_s16,
	put_s32,
	put_s64,
	put_string,
	put_float32fix,
	put_float64fix,
	put_bool
};

static int parse_path(const u8_t *strpath, u16_t strlen,
		      struct lwm2m_obj_path *path)
{
	int ret = 0;
	int pos = 0;
	u16_t val;
	u8_t c = 0;

	do {
		val = 0;
		c = strpath[pos];
		/* we should get a value first - consume all numbers */
		while (pos < strlen && c >= '0' && c <= '9') {
			val = val * 10 + (c - '0');
			c = strpath[++pos];
		}

		/*
		 * Slash will mote thing forward
		 * and the end will be when pos == pl
		 */
		if (c == '/' || pos == strlen) {
			SYS_LOG_DBG("Setting %u = %u", ret, val);
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
			SYS_LOG_ERR("Error: illegal char '%c' at pos:%d",
				    c, pos);
			return -1;
		}
	} while (pos < strlen);

	return ret;
}

int do_write_op_json(struct lwm2m_engine_obj *obj,
		     struct lwm2m_engine_context *context)
{
	struct lwm2m_input_context *in = context->in;
	struct lwm2m_obj_path *path = context->path;
	struct lwm2m_engine_obj_field *obj_field;
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_engine_res_inst *res = NULL;
	struct json_data json;
	u8_t olv = 0;
	u8_t *inbuf, created;
	int inpos, i, r;
	size_t insize;
	u8_t mode = MODE_NONE;

	olv    = path->level;
	inbuf  = in->inbuf;
	inpos  = in->inpos;
	insize = in->insize;

	while (json_next_token(in, &json)) {
		i = 0;
		created = 0;
		if (json.name[0] == 'n') {
			path->level = parse_path(json.value, json.value_len,
						 path);
			if (i > 0) {
				r = lwm2m_get_or_create_engine_obj(context,
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
			/* update values */
			inbuf = in->inbuf;
			inpos = in->inpos;
			in->inbuf = json.value;
			in->inpos = 0;
			in->insize = json.value_len;
		}

		if (mode == MODE_READY) {
			if (!obj_inst) {
				return -EINVAL;
			}

			obj_field = lwm2m_get_engine_obj_field(obj,
							       path->res_id);
			if (!obj_field) {
				goto skip_optional;
			}

			if ((obj_field->permissions & LWM2M_PERM_W) !=
			     LWM2M_PERM_W) {
				return -EPERM;
			}

			if (!obj_inst->resources ||
			     obj_inst->resource_count == 0) {
				return -EINVAL;
			}

			for (i = 0; i < obj_inst->resource_count; i++) {
				if (obj_inst->resources[i].res_id ==
						path->res_id) {
					res = &obj_inst->resources[i];
					break;
				}
			}

			if (!res) {
				return -ENOENT;
			}

			lwm2m_write_handler(obj_inst, res, obj_field, context);

skip_optional:
			mode = MODE_NONE;
			in->inbuf = inbuf;
			in->inpos = inpos;
			in->insize = insize;
			path->level = olv;
		}
	}

	return 0;
}
