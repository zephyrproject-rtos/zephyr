/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_lwm2m_senml_json
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>
#include <sys/base64.h>
#include <sys/slist.h>

#include "lwm2m_object.h"
#include "lwm2m_rw_senml_json.h"
#include "lwm2m_rw_plain_text.h"
#include "lwm2m_engine.h"
#include "lwm2m_util.h"

#define T_OBJECT_BEGIN BIT(0)
#define T_OBJECT_END BIT(1)
#define T_STRING_BEGIN BIT(2)
#define T_STRING_END BIT(3)
#define T_VALUE BIT(4)

#define SENML_JSON_BASE_NAME_ATTRIBUTE 0
#define SENML_JSON_BASE_TIME_ATTRIBUTE 1
#define SENML_JSON_NAME_ATTRIBUTE 2
#define SENML_JSON_TIME_ATTRIBUTE 3
#define SENML_JSON_FLOAT_VALUE_ATTRIBUTE 4
#define SENML_JSON_BOOLEAN_VALUE_ATTRIBUTE 5
#define SENML_JSON_OBJ_LINK_VALUE_ATTRIBUTE 6
#define SENML_JSON_OPAQUE_VALUE_ATTRIBUTE 7
#define SENML_JSON_STRING_VALUE_ATTRIBUTE 8
#define SENML_JSON_STRING_BLOCK_DATA 9
#define SENML_JSON_UNKNOWN_ATTRIBUTE 255

#define SEPARATOR(f) ((f & WRITER_OUTPUT_VALUE) ? "," : "")

#define TOKEN_BUF_LEN 64

struct json_out_formatter_data {
	/* flags */
	uint8_t writer_flags;

	/* base name */
	struct lwm2m_obj_path base_name;
	/* Add Base name */
	bool base_name_used;
	bool add_base_name_to_start;
};

struct json_in_formatter_data {
	/* name info */
	uint16_t name_offset;
	uint16_t name_len;

	/* value info */
	uint16_t value_offset;
	/* Value length */
	uint16_t value_len;

	/* state */
	uint16_t offset;

	/* flags */
	uint8_t json_flags;
};

/* some temporary buffer space for format conversions */
static char json_buffer[TOKEN_BUF_LEN];

static int parse_path(const uint8_t *buf, uint16_t buflen, struct lwm2m_obj_path *path);

static void json_add_char(struct lwm2m_input_context *in, struct json_in_formatter_data *fd)
{
	if ((fd->json_flags & T_VALUE) ||
	    ((fd->json_flags & T_STRING_BEGIN) && !(fd->json_flags & T_STRING_END))) {
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

static struct lwm2m_senml_json_context *seml_json_context_get(struct lwm2m_block_context *block_ctx)
{
	if (block_ctx) {
		return &block_ctx->senml_json_ctx;
	}

	return NULL;
}

static int json_atribute_decode(struct lwm2m_input_context *in, struct json_in_formatter_data *fd)
{
	uint8_t attrbute_name[3];

	if (fd->name_len == 0 || fd->name_len > 3) {
		if (fd->name_len == 0 && in->block_ctx && (fd->json_flags & T_VALUE) &&
		    (fd->json_flags & T_STRING_END)) {
			return SENML_JSON_STRING_BLOCK_DATA;
		}
		return SENML_JSON_UNKNOWN_ATTRIBUTE;
	}

	if (buf_read(attrbute_name, fd->name_len, CPKT_BUF_READ(in->in_cpkt), &fd->name_offset) <
	    0) {
		LOG_ERR("Error parsing attribute name!");
		return SENML_JSON_UNKNOWN_ATTRIBUTE;
	}

	if (fd->name_len == 1) {
		if (attrbute_name[0] == 'n') {
			return SENML_JSON_NAME_ATTRIBUTE;
		} else if (attrbute_name[0] == 't') {
			return SENML_JSON_TIME_ATTRIBUTE;
		} else if (attrbute_name[0] == 'v') {
			return SENML_JSON_FLOAT_VALUE_ATTRIBUTE;
		}

	} else if (fd->name_len == 2) {
		if (attrbute_name[0] == 'b') {
			if (attrbute_name[1] == 'n') {
				return SENML_JSON_BASE_NAME_ATTRIBUTE;
			} else if (attrbute_name[1] == 't') {
				return SENML_JSON_BASE_TIME_ATTRIBUTE;
			}
		} else if (attrbute_name[0] == 'v') {
			if (attrbute_name[1] == 'b') {
				return SENML_JSON_BOOLEAN_VALUE_ATTRIBUTE;
			} else if (attrbute_name[1] == 'd') {
				return SENML_JSON_OPAQUE_VALUE_ATTRIBUTE;
			} else if (attrbute_name[1] == 's') {
				return SENML_JSON_STRING_VALUE_ATTRIBUTE;
			}
		}
	} else if (fd->name_len == 3) {
		if (attrbute_name[0] == 'v' && attrbute_name[1] == 'l' && attrbute_name[2] == 'o') {
			return SENML_JSON_OBJ_LINK_VALUE_ATTRIBUTE;
		}
	}

	return SENML_JSON_UNKNOWN_ATTRIBUTE;
}

/* Parse SenML attribute & value pairs  */
static int json_next_token(struct lwm2m_input_context *in, struct json_in_formatter_data *fd)
{
	uint8_t cont, c = 0;
	bool escape = false;
	struct lwm2m_senml_json_context *block_ctx;

	(void)memset(fd, 0, sizeof(*fd));
	cont = 1U;
	block_ctx = seml_json_context_get(in->block_ctx);

	if (block_ctx && block_ctx->json_flags) {
		/* Store from last sequence */
		fd->json_flags = block_ctx->json_flags;
		block_ctx->json_flags = 0;
	}

	/* We will be either at start, or at a specific position */
	while (in->offset < in->in_cpkt->offset && cont) {
		fd->offset = in->offset;
		if (buf_read_u8(&c, CPKT_BUF_READ(in->in_cpkt), &in->offset) < 0) {
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

		case '}':
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

	/*
	 * Enable base name add if it is enabled
	 * Base name is only added one time at first resource data
	 */
	fd->add_base_name_to_start = fd->base_name_used;
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

	/* Clear flag. */
	fd->add_base_name_to_start = false;

	res = buf_append(CPKT_BUF_WRITE(out->out_cpkt), "]", 1);
	if (res < 0) {
		return res;
	}

	return 1;
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

static int put_char(struct lwm2m_output_context *out, char c)
{
	int res;

	res = buf_append(CPKT_BUF_WRITE(out->out_cpkt), &c, sizeof(c));
	if (res < 0) {
		return res;
	}

	return 1;
}

static int put_json_prefix(struct lwm2m_output_context *out, struct lwm2m_obj_path *path,
			      const char *format)
{
	struct json_out_formatter_data *fd;
	char *sep;
	int len = 0;
	int base_name_length = 0;

	fd = engine_get_out_user_data(out);

	/* Add separator after first added resource */
	sep = SEPARATOR(fd->writer_flags);
	if (!fd->base_name_used) {
		if (fd->writer_flags & WRITER_RESOURCE_INSTANCE) {
			len = snprintk(json_buffer, sizeof(json_buffer),
				       "%s{\"n\":\"/%u/%u/%u/%u\",%s:", sep, path->obj_id,
				       path->obj_inst_id, path->res_id, path->res_inst_id, format);
		} else {
			len = snprintk(json_buffer, sizeof(json_buffer),
				       "%s{\"n\":\"/%u/%u/%u\",%s:", sep, path->obj_id,
				       path->obj_inst_id, path->res_id, format);
		}
	} else {
		if (fd->add_base_name_to_start) {
			/* Generate base name */
			if (fd->base_name.level == LWM2M_PATH_LEVEL_NONE) {
				base_name_length = snprintk(json_buffer, sizeof(json_buffer),
							    "%s{\"bn\":\"/\",", sep);
			} else if (fd->base_name.level == LWM2M_PATH_LEVEL_OBJECT) {
				base_name_length =
					snprintk(json_buffer, sizeof(json_buffer),
						 "%s{\"bn\":\"/%u/\",", sep, path->obj_id);
			} else {
				base_name_length = snprintk(json_buffer, sizeof(json_buffer),
							    "%s{\"bn\":\"/%u/%u/\",", sep,
							    path->obj_id, path->obj_inst_id);
			}
			fd->add_base_name_to_start = false;
		} else {
			base_name_length = snprintk(json_buffer, sizeof(json_buffer), "%s{", sep);
		}

		if (base_name_length < 0) {
			return base_name_length;
		}

		if (buf_append(CPKT_BUF_WRITE(out->out_cpkt), json_buffer, base_name_length) < 0) {
			return -ENOMEM;
		}

		if (fd->base_name.level == LWM2M_PATH_LEVEL_NONE) {
			if (fd->writer_flags & WRITER_RESOURCE_INSTANCE) {
				len = snprintk(json_buffer, sizeof(json_buffer),
					       "\"n\":\"%u/%u/%u/%u\",%s:", path->obj_id,
					       path->obj_inst_id, path->res_id, path->res_inst_id,
					       format);
			} else {
				len = snprintk(json_buffer, sizeof(json_buffer),
					       "\"n\":\"%u/%u/%u\",%s:", path->obj_id,
					       path->obj_inst_id, path->res_id, format);
			}
		} else if (fd->base_name.level == LWM2M_PATH_LEVEL_OBJECT) {
			if (fd->writer_flags & WRITER_RESOURCE_INSTANCE) {
				len = snprintk(json_buffer, sizeof(json_buffer),
					       "\"n\":\"%u/%u/%u\",%s:", path->obj_inst_id,
					       path->res_id, path->res_inst_id, format);
			} else {
				len = snprintk(json_buffer, sizeof(json_buffer),
					       "\"n\":\"%u/%u\",%s:", path->obj_inst_id,
					       path->res_id, format);
			}
		} else {
			if (fd->writer_flags & WRITER_RESOURCE_INSTANCE) {
				len = snprintk(json_buffer, sizeof(json_buffer),
					       "\"n\":\"%u/%u\",%s:", path->res_id,
					       path->res_inst_id, format);
			} else {
				len = snprintk(json_buffer, sizeof(json_buffer),
					       "\"n\":\"%u\",%s:", path->res_id, format);
			}
		}
	}

	if (len < 0) {
		return len;
	}

	if (buf_append(CPKT_BUF_WRITE(out->out_cpkt), json_buffer, len)) {
		return -ENOMEM;
	}

	return len + base_name_length;
}

static int put_json_postfix(struct lwm2m_output_context *out)
{
	int len;
	struct json_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);

	len = put_char(out, '}');

	if (len < 0) {
		return len;
	}

	fd->writer_flags |= WRITER_OUTPUT_VALUE;
	return len;
}

static int put_s32(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, int32_t value)
{
	int res, len;

	if (!out->out_cpkt || !engine_get_out_user_data(out)) {
		return -EINVAL;
	}

	res = put_json_prefix(out, path, "\"v\"");
	if (res < 0) {
		return res;
	}
	len = res;

	res = plain_text_put_format(out, "%d", value);
	if (res < 0) {
		return res;
	}
	len += res;

	res = put_json_postfix(out);
	if (res < 0) {
		return res;
	}
	len += res;

	return len;
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
	int res, len;

	if (!out->out_cpkt || !engine_get_out_user_data(out)) {
		return -EINVAL;
	}

	res = put_json_prefix(out, path, "\"v\"");

	if (res < 0) {
		return res;
	}
	len = res;

	res = plain_text_put_format(out, "%lld", value);
	if (res < 0) {
		return res;
	}
	len += res;

	res = put_json_postfix(out);
	if (res < 0) {
		return res;
	}
	len += res;

	return len;
}

static int write_string_buffer(struct lwm2m_output_context *out, char *buf, size_t buflen)
{
	int res, len;

	res = put_char(out, '"');
	if (res < 0) {
		return res;
	}
	len = res;

	for (size_t i = 0; i < buflen; ++i) {
		/* Escape special characters */
		/* TODO: Handle UTF-8 strings */
		if (buf[i] < '\x20') {
			res = snprintk(json_buffer, sizeof(json_buffer), "\\x%x", buf[i]);
			if (res < 0) {
				return res;
			}

			if (buf_append(CPKT_BUF_WRITE(out->out_cpkt), json_buffer, res) < 0) {
				return -ENOMEM;
			}
			len += res;

			continue;
		} else if (buf[i] == '"' || buf[i] == '\\') {
			res = put_char(out, '\\');
			if (res < 0) {
				return res;
			}
			len += res;
		}
		res = put_char(out, buf[i]);
		if (res < 0) {
			return res;
		}
		len += res;
	}

	res = put_char(out, '"');
	if (res < 0) {
		return res;
	}
	len += res;

	return len;
}

static int put_string(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, char *buf,
			 size_t buflen)
{
	int res, len;

	if (!out->out_cpkt || !engine_get_out_user_data(out)) {
		return -EINVAL;
	}

	res = put_json_prefix(out, path, "\"vs\"");
	if (res < 0) {
		return res;
	}
	len = res;

	res = write_string_buffer(out, buf, buflen);

	if (res < 0) {
		return res;
	}
	len += res;

	res = put_json_postfix(out);
	if (res < 0) {
		return res;
	}
	len += res;

	return len;
}

static int put_float(struct lwm2m_output_context *out, struct lwm2m_obj_path *path,
			     double  *value)
{
	int res, len;

	if (!out->out_cpkt || !engine_get_out_user_data(out)) {
		return -EINVAL;
	}
	res = put_json_prefix(out, path, "\"v\"");

	if (res < 0) {
		return res;
	}
	len = res;

	res = plain_text_put_float(out, path, value);
	if (res < 0) {
		return res;
	}
	len += res;

	res = put_json_postfix(out);
	if (res < 0) {
		return res;
	}
	len += res;

	return len;
}

static int put_bool(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, bool value)
{
	int res, len;

	if (!out->out_cpkt || !engine_get_out_user_data(out)) {
		return -EINVAL;
	}

	res = put_json_prefix(out, path, "\"vb\"");
	if (res < 0) {
		return res;
	}
	len = res;

	res = plain_text_put_format(out, "%s", value ? "true" : "false");
	if (res < 0) {
		return res;
	}
	len += res;

	res = put_json_postfix(out);
	if (res < 0) {
		return res;
	}
	len += res;

	return len;
}

static int put_opaque(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, char *buf,
			 size_t buflen)
{
	int res, len;
	size_t temp_length;

	if (!out->out_cpkt || !engine_get_out_user_data(out)) {
		return -EINVAL;
	}
	res = put_json_prefix(out, path, "\"vd\"");
	if (res < 0) {
		return res;
	}
	len = res;

	res = put_char(out, '"');
	if (res < 0) {
		return res;
	}
	len += res;

	if (base64_encode(CPKT_BUF_W_PTR(out->out_cpkt), CPKT_BUF_W_SIZE(out->out_cpkt),
			  &temp_length, buf, buflen)) {
		/* No space available for base64 data */
		return -ENOMEM;
	}
	len += temp_length;

	res = put_char(out, '"');
	if (res < 0) {
		return res;
	}
	len += res;

	res = put_json_postfix(out);
	if (res < 0) {
		return res;
	}
	len += res;

	return len;
}

static int put_objlnk(struct lwm2m_output_context *out, struct lwm2m_obj_path *path,
			 struct lwm2m_objlnk *value)
{
	int res, len;

	if (!out->out_cpkt || !engine_get_out_user_data(out)) {
		return -EINVAL;
	}

	res = put_json_prefix(out, path, "\"vlo\"");
	if (res < 0) {
		return res;
	}
	len = res;

	res = plain_text_put_format(out, "\"%u:%u\"", value->obj_id, value->obj_inst);
	if (res < 0) {
		return res;
	}
	len += res;

	res = put_json_postfix(out);
	if (res < 0) {
		return res;
	}
	len += res;

	return len;
}

static int read_int(struct lwm2m_input_context *in, int64_t *value, bool accept_sign)
{
	struct json_in_formatter_data *fd;
	uint8_t *buf;
	size_t i = 0;
	bool neg = false;
	char c;

	/* initialize values to 0 */
	*value = 0;

	fd = engine_get_in_user_data(in);
	if (!fd) {
		return -EINVAL;
	}

	if (fd->value_len == 0) {
		return -ENODATA;
	}

	buf = in->in_cpkt->data + fd->value_offset;
	while (*(buf + i) && i < fd->value_len) {
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
	int ret;

	fd = engine_get_in_user_data(in);
	if (!fd) {
		return -EINVAL;
	}

	if (fd->value_len > buflen) {
		LOG_WRN("Buffer too small to accommodate string, truncating");
		fd->value_len = buflen - 1;
	}

	ret = buf_read(buf, fd->value_len, CPKT_BUF_READ(in->in_cpkt), &fd->value_offset);

	if (ret < 0) {
		return ret;
	}
	/* add NULL */
	buf[fd->value_len] = '\0';

	return fd->value_len;
}

static int get_float(struct lwm2m_input_context *in, double *value)
{
	struct json_in_formatter_data *fd;

	int i = 0, len = 0;
	bool has_dot = false;
	uint8_t tmp, buf[24];
	uint8_t *json_buf;

	fd = engine_get_in_user_data(in);
	if (!fd) {
		return -EINVAL;
	}

	json_buf = in->in_cpkt->data + fd->value_offset;
	while (*(json_buf + len) && len < fd->value_len) {
		tmp = *(json_buf + len);

		if ((tmp == '-' && i == 0) || (tmp == '.' && !has_dot) || isdigit(tmp)) {
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
	if (!fd) {
		return -EINVAL;
	}

	if (fd->value_len == 0) {
		return -ENODATA;
	}

	if (strncmp(in->in_cpkt->data + fd->value_offset, "true", 4) == 0) {
		*value = true;
	} else if (strncmp(in->in_cpkt->data + fd->value_offset, "false", 5) == 0) {
		*value = false;
	}

	return fd->value_len;
}

static int get_opaque(struct lwm2m_input_context *in, uint8_t *value, size_t buflen,
			 struct lwm2m_opaque_context *opaque, bool *last_block)
{
	struct json_in_formatter_data *fd;
	struct lwm2m_senml_json_context *block_ctx;
	int in_len;

	block_ctx = seml_json_context_get(in->block_ctx);

	fd = engine_get_in_user_data(in);
	if (!fd) {
		return -EINVAL;
	}

	uint8_t *data_ptr = in->in_cpkt->data + fd->value_offset;

	if (opaque->remaining == 0) {
		size_t original_size = fd->value_len;
		size_t base64_length;

		if (block_ctx) {
			if (block_ctx->base64_buf_len) {
				uint8_t module_buf[4];
				size_t buffer_module_length = 4 - block_ctx->base64_buf_len;

				if (fd->value_len < buffer_module_length) {
					return -ENODATA;
				}

				fd->value_len -= buffer_module_length;
				memcpy(module_buf, block_ctx->base64_mod_buf,
				       block_ctx->base64_buf_len);
				memcpy(module_buf + block_ctx->base64_buf_len, data_ptr,
				       buffer_module_length);

				size_t buffer_base64_length;

				if (base64_decode(module_buf, 4, &buffer_base64_length, module_buf,
						  4) < 0) {
					return -ENODATA;
				}

				block_ctx->base64_buf_len = 0;

				if (!in->block_ctx->last_block) {
					block_ctx->base64_buf_len = (fd->value_len % 4);
					if (fd->value_len < block_ctx->base64_buf_len) {
						return -ENODATA;
					}

					if (block_ctx->base64_buf_len) {
						uint8_t *data_tail_ptr;

						data_tail_ptr =
							data_ptr + (original_size -
								    block_ctx->base64_buf_len);
						memcpy(block_ctx->base64_mod_buf, data_tail_ptr,
						       block_ctx->base64_buf_len);
						fd->value_len -= block_ctx->base64_buf_len;
					}
				}
				/* Decode rest of data and do memmove */
				if (base64_decode(data_ptr, original_size, &base64_length,
						  data_ptr + buffer_module_length,
						  fd->value_len) < 0) {
					return -ENODATA;
				}
				fd->value_len = base64_length;
				/* Move decoded data by module result size from front */
				memmove(data_ptr + buffer_base64_length, data_ptr, fd->value_len);
				memcpy(data_ptr, module_buf, buffer_base64_length);
				fd->value_len += buffer_base64_length;
			} else {
				block_ctx->base64_buf_len = (fd->value_len % 4);
				if (fd->value_len < block_ctx->base64_buf_len) {
					return -ENODATA;
				}

				if (block_ctx->base64_buf_len) {
					uint8_t *data_tail_ptr =
						data_ptr +
						(original_size - block_ctx->base64_buf_len);

					memcpy(block_ctx->base64_mod_buf, data_tail_ptr,
					       block_ctx->base64_buf_len);
					fd->value_len -= block_ctx->base64_buf_len;
				}

				if (base64_decode(data_ptr, original_size, &base64_length, data_ptr,
						  fd->value_len) < 0) {
					return -ENODATA;
				}
				fd->value_len = base64_length;
			}
			/* Set zero because total length is unknown */
			opaque->len = 0;
		} else {
			if (base64_decode(data_ptr, fd->value_len, &base64_length, data_ptr,
					  fd->value_len) < 0) {
				return -ENODATA;
			}
			fd->value_len = base64_length;
			opaque->len = fd->value_len;
		}
		opaque->remaining = fd->value_len;
	}

	in_len = opaque->remaining;

	if (in_len > buflen) {
		in_len = buflen;
	}

	if (in_len > fd->value_len) {
		in_len = fd->value_len;
	}

	opaque->remaining -= in_len;
	if (opaque->remaining == 0U) {
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
	uint16_t value_offset;
	struct json_in_formatter_data *fd;

	fd = engine_get_in_user_data(in);
	if (!fd) {
		return -EINVAL;
	}

	/* Store the original value offset. */
	value_offset = fd->value_offset;

	len = read_int(in, &tmp, false);
	if (len <= 0) {
		return -ENODATA;
	}

	total_len = len;
	value->obj_id = (uint16_t)tmp;

	len++;  /* +1 for ':' delimeter. */
	fd->value_offset += len;

	len = read_int(in, &tmp, false);
	if (len <= 0) {
		return -ENODATA;
	}

	total_len += len;
	value->obj_inst = (uint16_t)tmp;

	/* Restore the original value offset. */
	fd->value_offset = value_offset;

	return total_len;
}

const struct lwm2m_writer senml_json_writer = {
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
	.put_bool = put_bool,
	.put_opaque = put_opaque,
	.put_objlnk = put_objlnk,
};

const struct lwm2m_reader senml_json_reader = {
	.get_s32 = get_s32,
	.get_s64 = get_s64,
	.get_string = get_string,
	.get_float = get_float,
	.get_bool = get_bool,
	.get_opaque = get_opaque,
	.get_objlnk = get_objlnk,
};

static uint8_t lwm2m_use_base_name(sys_slist_t *lwm_path_list)
{
	uint8_t recursive_path = 0;
	struct lwm2m_obj_path_list *entry;

	SYS_SLIST_FOR_EACH_CONTAINER(lwm_path_list, entry, node) {
		if (entry->path.level < 3) {
			recursive_path++;
		}
	}
	return recursive_path;
}

static void lwm2m_define_longest_match_url_for_base_name(struct json_out_formatter_data *fd,
							 sys_slist_t *lwm_path_list)
{
	struct lwm2m_obj_path_list *entry;

	/* Set base name use to false */
	fd->base_name_used = false;

	if (!lwm2m_use_base_name(lwm_path_list)) {
		/* do not use base at all */
		return;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(lwm_path_list, entry, node) {
		if (fd->base_name_used == false) {
			/* First at list is define compare for rest */
			fd->base_name.level = entry->path.level;
			fd->base_name.obj_id = entry->path.obj_id;
			fd->base_name.obj_inst_id = entry->path.obj_inst_id;
			fd->base_name.res_id = entry->path.res_id;
			fd->base_name.res_inst_id = entry->path.res_inst_id;

			fd->base_name_used = true;

			if (fd->base_name.level == LWM2M_PATH_LEVEL_NONE) {
				return;
			}
			continue;
		}

		if (entry->path.level == LWM2M_PATH_LEVEL_NONE ||
		    fd->base_name.obj_id != entry->path.obj_id) {
			/*
			 * Stop if Object ID is not match or compare url level is 0
			 * Define just "/" base name
			 */
			fd->base_name.level = LWM2M_PATH_LEVEL_NONE;
			return;
		}

		if (fd->base_name.level == LWM2M_PATH_LEVEL_OBJECT) {
			continue;
		}

		if (fd->base_name.level >= entry->path.level &&
		    fd->base_name.obj_inst_id != entry->path.obj_inst_id) {
			/* Define just "/obj_id/" base name */
			fd->base_name.level = LWM2M_PATH_LEVEL_OBJECT;
			continue;
		}

		if (fd->base_name.level == LWM2M_PATH_LEVEL_OBJECT_INST) {
			continue;
		}

		if (fd->base_name.level >= entry->path.level &&
		    fd->base_name.res_id != entry->path.res_id) {
			/* Do not continue deeper possible bn "/obj_id/obj_inst_id/" */
			fd->base_name.level = LWM2M_PATH_LEVEL_OBJECT_INST;
			continue;
		}

		if (fd->base_name.level == LWM2M_PATH_LEVEL_RESOURCE) {
			continue;
		}

		if (fd->base_name.level >= entry->path.level &&
		    fd->base_name.res_inst_id != entry->path.res_inst_id) {
			/* Do not continue deeper possible bn "/obj_id/obj_inst_id/res_id/" */
			fd->base_name.level = LWM2M_PATH_LEVEL_RESOURCE;
			continue;
		}
	}
}

void lwm2m_senml_json_context_init(struct lwm2m_senml_json_context *ctx)
{
	ctx->base_name_stored = false;
	ctx->full_name_true = false;
	ctx->base64_buf_len = 0;
	ctx->json_flags = 0;
}

int do_read_op_senml_json(struct lwm2m_message *msg)
{
	struct lwm2m_obj_path_list temp;
	sys_slist_t lwm_path_list;
	int ret;
	struct json_out_formatter_data fd;

	(void)memset(&fd, 0, sizeof(fd));
	engine_set_out_user_data(&msg->out, &fd);
	/* Init list */
	sys_slist_init(&lwm_path_list);
	/* Init message here ready for response */
	temp.path = msg->path;
	/* Add one entry to list */
	sys_slist_append(&lwm_path_list, &temp.node);

	/* Detect longest match base name to url */
	lwm2m_define_longest_match_url_for_base_name(&fd, &lwm_path_list);

	ret = lwm2m_perform_read_op(msg, LWM2M_FORMAT_APP_SEML_JSON);
	engine_clear_out_user_data(&msg->out);

	return ret;
}

static int parse_path(const uint8_t *buf, uint16_t buflen, struct lwm2m_obj_path *path)
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
			if (ret == LWM2M_PATH_LEVEL_NONE) {
				path->obj_id = val;
			} else if (ret == LWM2M_PATH_LEVEL_OBJECT) {
				path->obj_inst_id = val;
			} else if (ret == LWM2M_PATH_LEVEL_OBJECT_INST) {
				path->res_id = val;
			} else if (ret == LWM2M_PATH_LEVEL_RESOURCE) {
				path->res_inst_id = val;
			}

			ret++;
			pos++;
		} else {
			LOG_ERR("Error: illegal char '%c' at pos:%d", c, pos);
			return -1;
		}
	} while (pos < buflen);

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

	obj_field = lwm2m_get_engine_obj_field(obj_inst->obj, msg->path.res_id);
	/*
	 * if obj_field is not found,
	 * treat as an optional resource
	 */
	if (!obj_field) {
		return -ENOENT;
	}

	if (!LWM2M_HAS_PERM(obj_field, LWM2M_PERM_W) &&
	    !lwm2m_engine_bootstrap_override(msg->ctx, &msg->path)) {
		return -EPERM;
	}

	if (!obj_inst->resources || obj_inst->resource_count == 0U) {
		return -EINVAL;
	}

	for (int index = 0; index < obj_inst->resource_count; index++) {
		if (obj_inst->resources[index].res_id == msg->path.res_id) {
			res = &obj_inst->resources[index];
			break;
		}
	}

	if (!res) {
		return -ENOENT;
	}

	for (int index = 0; index < res->res_inst_count; index++) {
		if (res->res_instances[index].res_inst_id == msg->path.res_inst_id) {
			res_inst = &res->res_instances[index];
			break;
		}
	}

	if (!res_inst) {
		return -ENOENT;
	}

	/* Write the resource value */
	ret = lwm2m_write_handler(obj_inst, res, res_inst, obj_field, msg);
	return ret;
}

static int senml_json_path_to_string(uint8_t *buf, size_t buf_len, struct lwm2m_obj_path *path,
				     uint8_t path_level)
{
	int name_length;

	if (path_level == LWM2M_PATH_LEVEL_NONE) {
		name_length = snprintk(buf, buf_len, "/");
	} else if (path_level == LWM2M_PATH_LEVEL_OBJECT) {
		name_length = snprintk(buf, buf_len, "/%u/", path->obj_id);
	} else if (path_level == LWM2M_PATH_LEVEL_OBJECT_INST) {
		name_length = snprintk(buf, buf_len, "/%u/%u/", path->obj_id, path->obj_inst_id);
	} else if (path_level == LWM2M_PATH_LEVEL_RESOURCE) {
		name_length = snprintk(buf, buf_len, "/%u/%u/%u", path->obj_id, path->obj_inst_id,
				       path->res_id);
	} else {
		name_length = snprintk(buf, buf_len, "/%u/%u/%u/%u", path->obj_id,
				       path->obj_inst_id, path->res_id, path->res_inst_id);
	}

	if (name_length > 0) {
		buf[name_length] = '\0';
	}

	return name_length;
}

int do_write_op_senml_json(struct lwm2m_message *msg)
{
	struct json_in_formatter_data fd;
	int ret = 0;
	uint8_t name[MAX_RESOURCE_LEN + 1];
	uint8_t base_name[MAX_RESOURCE_LEN + 1] = {0};
	uint8_t full_name[MAX_RESOURCE_LEN + 1] = {0};
	struct lwm2m_obj_path resource_path;
	bool path_valid = false;
	bool data_value = false;
	struct lwm2m_senml_json_context *block_ctx;

	(void)memset(&fd, 0, sizeof(fd));
	engine_set_in_user_data(&msg->in, &fd);

	block_ctx = seml_json_context_get(msg->in.block_ctx);

	if (block_ctx && block_ctx->json_flags) {
		int name_length;

		/* Re-load Base name and Name data from context block */
		if (block_ctx->base_name_stored) {
			/* base name path generate to string */
			name_length = senml_json_path_to_string(base_name, sizeof(base_name),
								&block_ctx->base_name_path,
								block_ctx->base_name_path.level);

			if (name_length <= 0) {
				ret = -EINVAL;
				goto end_of_operation;
			}

			if (block_ctx->base_name_path.level >= LWM2M_PATH_LEVEL_RESOURCE &&
			    !block_ctx->full_name_true) {
				memcpy(full_name, base_name, MAX_RESOURCE_LEN + 1);
				ret = parse_path(full_name, strlen(full_name), &resource_path);
				if (ret < 0) {
					ret = -EINVAL;
					goto end_of_operation;
				}
				resource_path.level = ret;
				path_valid = true;
			}
		}

		if (block_ctx->full_name_true) {
			/* full name path generate to string */
			name_length = senml_json_path_to_string(full_name, sizeof(full_name),
								&block_ctx->base_name_path,
								block_ctx->resource_path_level);

			if (name_length <= 0) {
				ret = -EINVAL;
				goto end_of_operation;
			}

			ret = parse_path(full_name, strlen(full_name), &resource_path);
			if (ret < 0) {
				ret = -EINVAL;
				goto end_of_operation;
			}
			resource_path.level = ret;
			path_valid = true;
		}
	}

	/* Parse Attribute value pair */
	while (json_next_token(&msg->in, &fd)) {
		if (!(fd.json_flags & T_VALUE)) {
			continue;
		}

		data_value = false;

		switch (json_atribute_decode(&msg->in, &fd)) {
		case SENML_JSON_BASE_NAME_ATTRIBUTE:
			if (fd.value_len > MAX_RESOURCE_LEN) {
				LOG_ERR("Base name too long %u", fd.value_len);
				ret = -EINVAL;
				goto end_of_operation;
			}

			if (buf_read(base_name, fd.value_len, CPKT_BUF_READ(msg->in.in_cpkt),
				     &fd.value_offset) < 0) {
				LOG_ERR("Error parsing base name!");
				ret = -EINVAL;
				goto end_of_operation;
			}

			base_name[fd.value_len] = '\0';
			/* Relative name is optional - preinitialize full name with base name */
			snprintk(full_name, sizeof(full_name), "%s", base_name);
			ret = parse_path(full_name, strlen(full_name), &resource_path);
			if (ret < 0) {
				LOG_ERR("Relative name too long");
				ret = -EINVAL;
				goto end_of_operation;
			}
			resource_path.level = ret;

			if (resource_path.level) {
				path_valid = true;
			}

			if (block_ctx) {
				block_ctx->base_name_path = resource_path;
				block_ctx->base_name_stored = true;
			}

			break;
		case SENML_JSON_NAME_ATTRIBUTE:

			/* handle resource name */
			if (fd.value_len > MAX_RESOURCE_LEN) {
				LOG_ERR("Relative name too long");
				ret = -EINVAL;
				goto end_of_operation;
			}

			/* get value for relative path */
			if (buf_read(name, fd.value_len, CPKT_BUF_READ(msg->in.in_cpkt),
				     &fd.value_offset) < 0) {
				LOG_ERR("Error parsing relative path!");
				ret = -EINVAL;
				goto end_of_operation;
			}

			name[fd.value_len] = '\0';

			/* combine base_name + name */
			snprintk(full_name, sizeof(full_name), "%s%s", base_name, name);
			ret = parse_path(full_name, strlen(full_name), &resource_path);
			if (ret < 0) {
				LOG_ERR("Relative name too long");
				ret = -EINVAL;
				goto end_of_operation;
			}
			resource_path.level = ret;

			if (block_ctx) {
				/* Store Resource data Path to base name path but
				 * store separately path level
				 */
				uint8_t path_level = block_ctx->base_name_path.level;

				block_ctx->base_name_path = resource_path;
				block_ctx->resource_path_level = resource_path.level;
				block_ctx->base_name_path.level = path_level;
				block_ctx->full_name_true = true;
			}
			path_valid = true;
			break;

		case SENML_JSON_FLOAT_VALUE_ATTRIBUTE:
		case SENML_JSON_BOOLEAN_VALUE_ATTRIBUTE:
		case SENML_JSON_OBJ_LINK_VALUE_ATTRIBUTE:
		case SENML_JSON_OPAQUE_VALUE_ATTRIBUTE:
		case SENML_JSON_STRING_VALUE_ATTRIBUTE:
		case SENML_JSON_STRING_BLOCK_DATA:
			data_value = true;
			break;

		case SENML_JSON_UNKNOWN_ATTRIBUTE:
			LOG_ERR("Unknown attribute");
			ret = -EINVAL;
			goto end_of_operation;

		default:
			break;
		}

		if (data_value && path_valid) {
			/* parse full_name into path */
			if (block_ctx) {
				/* Store json Flags */
				block_ctx->json_flags = fd.json_flags;
			}

			msg->path = resource_path;
			ret = lwm2m_senml_write_operation(msg, &fd);

			if (ret < 0) {
				break;
			}
		}
	}
	/* Do we have a data value which is part of the CoAP blocking process */
	if ((fd.json_flags & T_VALUE) && !(fd.json_flags & T_OBJECT_END) && !data_value &&
	    block_ctx && fd.value_len) {
		if (!path_valid) {
			LOG_ERR("No path available for Coap Block sub sequency");
			ret = -EINVAL;
			goto end_of_operation;
		}
		/* Store Json File description flags */
		block_ctx->json_flags = fd.json_flags;
		msg->path = resource_path;
		ret = lwm2m_senml_write_operation(msg, &fd);
	}

end_of_operation:
	engine_clear_in_user_data(&msg->in);

	return ret;
}

static uint8_t json_parse_composite_read_paths(struct lwm2m_message *msg,
					       sys_slist_t *lwm_path_list,
					       sys_slist_t *lwm_path_free_list)
{
	struct json_in_formatter_data fd;
	struct lwm2m_obj_path path;
	bool path_valid;
	int ret;
	uint8_t name[MAX_RESOURCE_LEN];
	uint8_t base_name[MAX_RESOURCE_LEN + 1] = {0};
	uint8_t full_name[MAX_RESOURCE_LEN + 1] = {0};
	uint8_t valid_path_cnt = 0;

	while (json_next_token(&msg->in, &fd)) {
		if (!(fd.json_flags & T_VALUE)) {
			continue;
		}

		path_valid = false;
		switch (json_atribute_decode(&msg->in, &fd)) {
		case SENML_JSON_BASE_NAME_ATTRIBUTE:
			if (fd.value_len >= sizeof(base_name)) {
				LOG_ERR("Base name too long");
				break;
			}

			if (buf_read(base_name, fd.value_len, CPKT_BUF_READ(msg->in.in_cpkt),
				     &fd.value_offset) < 0) {
				LOG_ERR("Error parsing base name!");
				break;
			}

			base_name[fd.value_len] = '\0';

			/* Relative name is optional - preinitialize full name with base name */
			snprintk(full_name, sizeof(full_name), "%s", base_name);

			if (fd.json_flags & T_OBJECT_END) {
				path_valid = true;
			}
			break;

		case SENML_JSON_NAME_ATTRIBUTE:

			/* handle resource name */
			if (fd.value_len >= MAX_RESOURCE_LEN) {
				LOG_ERR("Relative name too long");
				break;
			}

			/* get value for relative path */
			if (buf_read(name, fd.value_len, CPKT_BUF_READ(msg->in.in_cpkt),
				     &fd.value_offset) < 0) {
				LOG_ERR("Error parsing relative path!");
				break;
			}

			name[fd.value_len] = '\0';

			/* combine base_name + name */
			snprintk(full_name, sizeof(full_name), "%s%s", base_name, name);
			path_valid = true;
			break;

		default:
			break;
		}

		if (path_valid) {
			ret = parse_path(full_name, strlen(full_name), &path);
			if (ret >= 0) {
				path.level = ret;
				if (lwm2m_engine_add_path_to_list(lwm_path_list, lwm_path_free_list,
								  &path) == 0) {
					valid_path_cnt++;
				}
			}
		}
	}
	return valid_path_cnt;
}

int do_composite_read_op_senml_json(struct lwm2m_message *msg)
{
	int ret;
	struct json_out_formatter_data fd;
	struct lwm2m_obj_path_list lwm2m_path_list_buf[CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE];
	sys_slist_t lwm_path_list;
	sys_slist_t lwm_path_free_list;
	uint8_t path_list_size;

	/* Init list */
	lwm2m_engine_path_list_init(&lwm_path_list, &lwm_path_free_list, lwm2m_path_list_buf,
				    CONFIG_LWM2M_COMPOSITE_PATH_LIST_SIZE);

	/* Parse Path's from SenML JSO payload */
	path_list_size = json_parse_composite_read_paths(msg, &lwm_path_list, &lwm_path_free_list);
	if (path_list_size == 0) {
		LOG_ERR("No Valid Url at msg");
		return -ESRCH;
	}

	/* Clear path which are part are part of recursive path /1 will include /1/0/1 */
	lwm2m_engine_clear_duplicate_path(&lwm_path_list, &lwm_path_free_list);

	(void)memset(&fd, 0, sizeof(fd));
	engine_set_out_user_data(&msg->out, &fd);

	/* Detect longest match base name to url */
	lwm2m_define_longest_match_url_for_base_name(&fd, &lwm_path_list);

	ret = lwm2m_perform_composite_read_op(msg, LWM2M_FORMAT_APP_SEML_JSON, &lwm_path_list);
	engine_clear_out_user_data(&msg->out);

	return ret;
}

int do_send_op_senml_json(struct lwm2m_message *msg, sys_slist_t *lwm_path_list)
{
	struct json_out_formatter_data fd;
	int ret;

	(void)memset(&fd, 0, sizeof(fd));
	engine_set_out_user_data(&msg->out, &fd);

	/* Detect longest match base name to url */
	lwm2m_define_longest_match_url_for_base_name(&fd, lwm_path_list);

	ret = lwm2m_perform_composite_read_op(msg, LWM2M_FORMAT_APP_SEML_JSON, lwm_path_list);
	engine_clear_out_user_data(&msg->out);

	return ret;
}
