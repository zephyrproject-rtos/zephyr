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
 * - Lots of byte-order API clean up
 * - Var / parameter type cleanup
 * - Replace magic #'s with defines
 */

#define LOG_MODULE_NAME net_lwm2m_oma_tlv
#define LOG_LEVEL CONFIG_LWM2M_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <stdint.h>
#include <sys/byteorder.h>

#include "lwm2m_rw_oma_tlv.h"
#include "lwm2m_engine.h"
#ifdef CONFIG_LWM2M_RD_CLIENT_SUPPORT
#include "lwm2m_rd_client.h"
#endif
#include "lwm2m_util.h"

enum {
	OMA_TLV_TYPE_OBJECT_INSTANCE   = 0,
	OMA_TLV_TYPE_RESOURCE_INSTANCE = 1,
	OMA_TLV_TYPE_MULTI_RESOURCE    = 2,
	OMA_TLV_TYPE_RESOURCE          = 3
};

struct oma_tlv {
	uint8_t  type;
	uint16_t id; /* can be 8-bit or 16-bit when serialized */
	uint32_t length;
};

struct tlv_out_formatter_data {
	/* offset position storage */
	uint16_t mark_pos;
	uint16_t mark_pos_oi;
	uint16_t mark_pos_ri;
	uint8_t writer_flags;
};

static uint8_t get_len_type(const struct oma_tlv *tlv)
{
	if (tlv->length < 8) {
		return 0;
	} else if (tlv->length < 0x100) {
		return 1;
	} else if (tlv->length < 0x10000) {
		return 2;
	}

	return 3;
}

static uint8_t tlv_calc_type(uint8_t flags)
{
	return flags & WRITER_RESOURCE_INSTANCE ?
			OMA_TLV_TYPE_RESOURCE_INSTANCE : OMA_TLV_TYPE_RESOURCE;
}

static uint16_t tlv_calc_id(uint8_t flags, struct lwm2m_obj_path *path)
{
	return flags & WRITER_RESOURCE_INSTANCE ?
			path->res_inst_id : path->res_id;
}

static void tlv_setup(struct oma_tlv *tlv, uint8_t type, uint16_t id,
		      uint32_t buflen)
{
	if (tlv) {
		tlv->type = type;
		tlv->id = id;
		tlv->length = buflen;
	}
}

static int oma_tlv_put_u8(struct lwm2m_output_context *out,
			  uint8_t value, bool insert)
{
	struct tlv_out_formatter_data *fd;
	int ret;

	if (insert) {
		fd = engine_get_out_user_data(out);
		if (!fd) {
			return 0;
		}

		ret = buf_insert(CPKT_BUF_WRITE(out->out_cpkt),
				 fd->mark_pos, &value, 1);
		if (ret < 0) {
			return ret;
		}

		fd->mark_pos++;
	} else {
		ret = buf_append(CPKT_BUF_WRITE(out->out_cpkt), &value, 1);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

static size_t oma_tlv_put(const struct oma_tlv *tlv,
			  struct lwm2m_output_context *out,
			  uint8_t *value, bool insert)
{
	size_t pos;
	int ret, i;
	uint8_t len_type, tmp;

	/* len_type is the same as number of bytes required for length */
	len_type = get_len_type(tlv);

	/* first type byte in TLV header */
	tmp = (tlv->type << 6) |
	      (tlv->id > 255 ? (1 << 5) : 0) |
	      (len_type << 3) |
	      (len_type == 0U ? tlv->length : 0);

	ret = oma_tlv_put_u8(out, tmp, insert);
	if (ret < 0) {
		/* TODO: Generate error? */
		return 0;
	}

	pos = 1;

	/* The ID */
	if (tlv->id > 255) {
		ret = oma_tlv_put_u8(out, (tlv->id >> 8) & 0xff, insert);
		if (ret < 0) {
			/* TODO: Generate error? */
			return 0;
		}

		pos++;
	}

	ret = oma_tlv_put_u8(out, tlv->id & 0xff, insert);
	if (ret < 0) {
		/* TODO: Generate error? */
		return 0;
	}

	pos++;

	for (i = 2; i >= 0; i--) {
		if (len_type > i) {
			ret = oma_tlv_put_u8(out,
					     (tlv->length >> (i * 8)) & 0xff,
					     insert);
			if (ret < 0) {
				/* TODO: Generate error? */
				return 0;
			}

			pos++;
		}
	}

	/* finally add the value */
	if (value != NULL && tlv->length > 0 && !insert) {
		if (buf_append(CPKT_BUF_WRITE(out->out_cpkt),
			       value, tlv->length) < 0) {
			/* TODO: Generate error? */
			return 0;
		}
	}

	return pos + tlv->length;
}

static size_t oma_tlv_get(struct oma_tlv *tlv,
			  struct lwm2m_input_context *in,
			  bool dont_advance)
{
	uint8_t len_type;
	uint8_t len_pos = 1U;
	size_t tlv_len;
	uint16_t tmp_offset;
	uint8_t buf[2];

	tmp_offset = in->offset;
	if (buf_read_u8(&buf[0], CPKT_BUF_READ(in->in_cpkt), &tmp_offset) < 0) {
		goto error;
	}

	tlv->type = (buf[0] >> 6) & 3;
	len_type = (buf[0] >> 3) & 3;
	len_pos = 1 + (((buf[0] & (1 << 5)) != 0U) ? 2 : 1);

	if (buf_read_u8(&buf[1], CPKT_BUF_READ(in->in_cpkt), &tmp_offset) < 0) {
		return 0;
	}

	tlv->id = buf[1];

	/* if len_pos > 2 it means that there are more ID to read */
	if (len_pos > 2) {
		if (buf_read_u8(&buf[1], CPKT_BUF_READ(in->in_cpkt),
				&tmp_offset) < 0) {
			goto error;
		}

		tlv->id = (tlv->id << 8) + buf[1];
	}

	if (len_type == 0U) {
		tlv_len = buf[0] & 7;
	} else {
		/* read the length */
		tlv_len = 0;
		while (len_type > 0) {
			if (buf_read_u8(&buf[1], CPKT_BUF_READ(in->in_cpkt),
					&tmp_offset) < 0) {
				goto error;
			}

			len_pos++;
			tlv_len = tlv_len << 8 | buf[1];
			len_type--;
		}
	}

	/* and read out the data??? */
	tlv->length = tlv_len;

	if (!dont_advance) {
		in->offset = tmp_offset;
	}

	return len_pos + tlv_len;

error:
	/* TODO: Generate error? */
	if (!dont_advance) {
		in->offset = tmp_offset;
	}

	return 0;
}

static size_t put_begin_tlv(struct lwm2m_output_context *out, uint16_t *mark_pos,
			    uint8_t *writer_flags, int writer_flag)
{
	/* set flags */
	*writer_flags |= writer_flag;

	/*
	 * store position for inserting TLV when we know the length
	 */
	*mark_pos = out->out_cpkt->offset;

	return 0;
}

static size_t put_end_tlv(struct lwm2m_output_context *out, uint16_t mark_pos,
			  uint8_t *writer_flags, uint8_t writer_flag,
			  int tlv_type, int tlv_id)
{
	struct tlv_out_formatter_data *fd;
	struct oma_tlv tlv;
	uint32_t len = 0U;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return 0;
	}

	*writer_flags &= ~writer_flag;

	len = out->out_cpkt->offset - mark_pos;

	/* use stored location */
	fd->mark_pos = mark_pos;

	/* set instance length */
	tlv_setup(&tlv, tlv_type, tlv_id, len);
	len = oma_tlv_put(&tlv, out, NULL, true) - tlv.length;

	return len;
}

static size_t put_begin_oi(struct lwm2m_output_context *out,
			   struct lwm2m_obj_path *path)
{
	struct tlv_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return 0;
	}

	return put_begin_tlv(out, &fd->mark_pos_oi, &fd->writer_flags, 0);
}

static size_t put_end_oi(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path)
{
	struct tlv_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return 0;
	}

	return put_end_tlv(out, fd->mark_pos_oi, &fd->writer_flags, 0,
			   OMA_TLV_TYPE_OBJECT_INSTANCE, path->obj_inst_id);
}

static size_t put_begin_ri(struct lwm2m_output_context *out,
			   struct lwm2m_obj_path *path)
{
	struct tlv_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return 0;
	}

	return put_begin_tlv(out, &fd->mark_pos_ri, &fd->writer_flags,
			     WRITER_RESOURCE_INSTANCE);
}

static size_t put_end_ri(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path)
{
	struct tlv_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return 0;
	}

	return put_end_tlv(out, fd->mark_pos_ri, &fd->writer_flags,
			   WRITER_RESOURCE_INSTANCE,
			   OMA_TLV_TYPE_MULTI_RESOURCE, path->res_id);
}

static size_t put_s8(struct lwm2m_output_context *out,
		     struct lwm2m_obj_path *path, int8_t value)
{
	struct tlv_out_formatter_data *fd;
	size_t len;
	struct oma_tlv tlv;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return 0;
	}

	tlv_setup(&tlv, tlv_calc_type(fd->writer_flags),
		  tlv_calc_id(fd->writer_flags, path), sizeof(value));

	len = oma_tlv_put(&tlv, out, (uint8_t *)&value, false);
	return len;
}

static size_t put_s16(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, int16_t value)
{
	struct tlv_out_formatter_data *fd;
	size_t len;
	struct oma_tlv tlv;
	int16_t net_value;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return 0;
	}

	net_value = sys_cpu_to_be16(value);
	tlv_setup(&tlv, tlv_calc_type(fd->writer_flags),
		  tlv_calc_id(fd->writer_flags, path), sizeof(net_value));

	len = oma_tlv_put(&tlv, out, (uint8_t *)&net_value, false);
	return len;
}

static size_t put_s32(struct lwm2m_output_context *out,
			struct lwm2m_obj_path *path, int32_t value)
{
	struct tlv_out_formatter_data *fd;
	size_t len;
	struct oma_tlv tlv;
	int32_t net_value;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return 0;
	}

	net_value = sys_cpu_to_be32(value);
	tlv_setup(&tlv, tlv_calc_type(fd->writer_flags),
		  tlv_calc_id(fd->writer_flags, path), sizeof(net_value));

	len = oma_tlv_put(&tlv, out, (uint8_t *)&net_value, false);
	return len;
}

static size_t put_s64(struct lwm2m_output_context *out,
			struct lwm2m_obj_path *path, int64_t value)
{
	struct tlv_out_formatter_data *fd;
	size_t len;
	struct oma_tlv tlv;
	int64_t net_value;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return 0;
	}

	net_value = sys_cpu_to_be64(value);
	tlv_setup(&tlv, tlv_calc_type(fd->writer_flags),
		  tlv_calc_id(fd->writer_flags, path), sizeof(net_value));

	len = oma_tlv_put(&tlv, out, (uint8_t *)&net_value, false);
	return len;
}

static size_t put_string(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path,
			 char *buf, size_t buflen)
{
	struct tlv_out_formatter_data *fd;
	size_t len;
	struct oma_tlv tlv;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return 0;
	}

	tlv_setup(&tlv, tlv_calc_type(fd->writer_flags),
		  tlv_calc_id(fd->writer_flags, path), (uint32_t)buflen);
	len = oma_tlv_put(&tlv, out, (uint8_t *)buf, false);
	return len;
}

static size_t put_float32fix(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path,
			     float32_value_t *value)
{
	struct tlv_out_formatter_data *fd;
	size_t len;
	struct oma_tlv tlv;
	int ret;
	uint8_t b32[4];

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return 0;
	}

	ret = lwm2m_f32_to_b32(value, b32, sizeof(b32));
	if (ret < 0) {
		LOG_ERR("float32 conversion error: %d", ret);
		return 0;
	}

	tlv_setup(&tlv, tlv_calc_type(fd->writer_flags),
		  tlv_calc_id(fd->writer_flags, path), sizeof(b32));
	len = oma_tlv_put(&tlv, out, b32, false);
	return len;
}

static size_t put_float64fix(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path,
			     float64_value_t *value)
{
	struct tlv_out_formatter_data *fd;
	size_t len;
	struct oma_tlv tlv;
	uint8_t b64[8];
	int ret;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return 0;
	}

	ret = lwm2m_f64_to_b64(value, b64, sizeof(b64));
	if (ret < 0) {
		LOG_ERR("float64 conversion error: %d", ret);
		return 0;
	}

	tlv_setup(&tlv, tlv_calc_type(fd->writer_flags),
		  tlv_calc_id(fd->writer_flags, path), sizeof(b64));
	len = oma_tlv_put(&tlv, out, b64, false);
	return len;
}

static size_t put_bool(struct lwm2m_output_context *out,
		       struct lwm2m_obj_path *path, bool value)
{
	int8_t value_s8 = (value != 0 ? 1 : 0);

	return put_s8(out, path, value_s8);
}

static size_t put_opaque(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path,
			 char *buf, size_t buflen)
{
	return put_string(out, path, buf, buflen);
}

static size_t put_objlnk(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path,
			 struct lwm2m_objlnk *value)
{
	int32_t value_s32 = (value->obj_id << 16) | value->obj_inst;

	return put_s32(out, path, value_s32);
}

static size_t get_number(struct lwm2m_input_context *in, int64_t *value,
			 uint8_t max_len)
{
	struct oma_tlv tlv;
	size_t size = oma_tlv_get(&tlv, in, false);
	int64_t temp;

	*value = 0;
	if (size > 0) {
		if (tlv.length > max_len) {
			LOG_ERR("invalid length: %u", tlv.length);
			return 0;
		}

		if (buf_read((uint8_t *)&temp, tlv.length,
			     CPKT_BUF_READ(in->in_cpkt), &in->offset) < 0) {
			/* TODO: Generate error? */
			return 0;
		}

		switch (tlv.length) {
		case 1:
			*value = (int8_t)temp;
			break;
		case 2:
			*value = sys_cpu_to_be16((int16_t)temp);
			break;
		case 4:
			*value = sys_cpu_to_be32((int32_t)temp);
			break;
		case 8:
			*value = sys_cpu_to_be64(temp);
			break;
		default:
			LOG_ERR("invalid length: %u", tlv.length);
			return 0;
		}
	}

	return size;
}

static size_t get_s64(struct lwm2m_input_context *in, int64_t *value)
{
	return get_number(in, value, 8);
}

static size_t get_s32(struct lwm2m_input_context *in, int32_t *value)
{
	int64_t temp;
	size_t size;

	*value = 0;
	size = get_number(in, &temp, 4);
	if (size > 0) {
		*value = (int32_t)temp;
	}

	return size;
}

static size_t get_string(struct lwm2m_input_context *in,
			 uint8_t *buf, size_t buflen)
{
	struct oma_tlv tlv;
	size_t size = oma_tlv_get(&tlv, in, false);

	if (size > 0) {
		if (buflen <= tlv.length) {
			/* TODO: Generate error? */
			return 0;
		}

		if (buf_read(buf, tlv.length, CPKT_BUF_READ(in->in_cpkt),
			     &in->offset) < 0) {
			/* TODO: Generate error? */
			return 0;
		}

		buf[tlv.length] = '\0';
	}

	return size;
}

/* convert float to fixpoint */
static size_t get_float32fix(struct lwm2m_input_context *in,
			     float32_value_t *value)
{
	struct oma_tlv tlv;
	size_t size = oma_tlv_get(&tlv, in, false);
	uint8_t b32[4];
	int ret;

	if (size > 0) {
		if (tlv.length != 4U) {
			LOG_ERR("Invalid float32 length: %d", tlv.length);

			/* dummy read */
			while (tlv.length--) {
				if (buf_read_u8(b32,
						CPKT_BUF_READ(in->in_cpkt),
						&in->offset) < 0) {
					break;
				}
			}

			return 0;
		}

		/* read b32 in network byte order */
		if (buf_read(b32, tlv.length, CPKT_BUF_READ(in->in_cpkt),
			     &in->offset) < 0) {
			/* TODO: Generate error? */
			return 0;
		}

		ret = lwm2m_b32_to_f32(b32, sizeof(b32), value);
		if (ret < 0) {
			LOG_ERR("binary32 conversion error: %d", ret);
			return 0;
		}
	}

	return size;
}

static size_t get_float64fix(struct lwm2m_input_context *in,
			     float64_value_t *value)
{
	struct oma_tlv tlv;
	size_t size = oma_tlv_get(&tlv, in, false);
	uint8_t b64[8];
	int ret;

	if (size > 0) {
		if (tlv.length != 8U) {
			LOG_ERR("invalid float64 length: %d", tlv.length);

			/* dummy read */
			while (tlv.length--) {
				if (buf_read_u8(b64,
						CPKT_BUF_READ(in->in_cpkt),
						&in->offset) < 0) {
					break;
				}
			}

			return 0;
		}

		/* read b64 in network byte order */
		if (buf_read(b64, tlv.length, CPKT_BUF_READ(in->in_cpkt),
			     &in->offset) < 0) {
			/* TODO: Generate error? */
			return 0;
		}

		ret = lwm2m_b64_to_f64(b64, sizeof(b64), value);
		if (ret < 0) {
			LOG_ERR("binary64 conversion error: %d", ret);
			return 0;
		}
	}

	return size;
}

static size_t get_bool(struct lwm2m_input_context *in, bool *value)
{
	int64_t temp;
	size_t size;

	*value = 0;
	size = get_number(in, &temp, 2);
	if (size > 0) {
		*value = (temp != 0);
	}

	return size;
}

static size_t get_opaque(struct lwm2m_input_context *in,
			 uint8_t *value, size_t buflen, bool *last_block)
{
	struct oma_tlv tlv;

	oma_tlv_get(&tlv, in, false);
	in->opaque_len = tlv.length;
	return lwm2m_engine_get_opaque_more(in, value, buflen, last_block);
}

static size_t get_objlnk(struct lwm2m_input_context *in,
			 struct lwm2m_objlnk *value)
{
	int32_t value_s32;
	size_t size;

	size = get_s32(in, &value_s32);

	value->obj_id = (value_s32 >> 16) & 0xFFFF;
	value->obj_inst = value_s32 & 0xFFFF;

	return size;
}

const struct lwm2m_writer oma_tlv_writer = {
	.put_begin_oi = put_begin_oi,
	.put_end_oi = put_end_oi,
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
	.put_opaque = put_opaque,
	.put_objlnk = put_objlnk,
};

const struct lwm2m_reader oma_tlv_reader = {
	.get_s32 = get_s32,
	.get_s64 = get_s64,
	.get_string = get_string,
	.get_float32fix = get_float32fix,
	.get_float64fix = get_float64fix,
	.get_bool = get_bool,
	.get_opaque = get_opaque,
	.get_objlnk = get_objlnk,
};

int do_read_op_tlv(struct lwm2m_message *msg, int content_format)
{
	struct tlv_out_formatter_data fd;
	int ret;

	(void)memset(&fd, 0, sizeof(fd));
	engine_set_out_user_data(&msg->out, &fd);
	ret = lwm2m_perform_read_op(msg, content_format);
	engine_clear_out_user_data(&msg->out);
	return ret;
}

static int do_write_op_tlv_dummy_read(struct lwm2m_message *msg)
{
	struct oma_tlv tlv;
	uint8_t read_char;

	oma_tlv_get(&tlv, &msg->in, false);
	while (tlv.length--) {
		if (buf_read_u8(&read_char, CPKT_BUF_READ(msg->in.in_cpkt),
				&msg->in.offset) < 0) {
			break;
		}
	}

	return 0;
}

static int do_write_op_tlv_item(struct lwm2m_message *msg)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_engine_res *res = NULL;
	struct lwm2m_engine_res_inst *res_inst = NULL;
	struct lwm2m_engine_obj_field *obj_field = NULL;
	uint8_t created = 0U;
	int ret, i;

	ret = lwm2m_get_or_create_engine_obj(msg, &obj_inst, &created);
	if (ret < 0) {
		goto error;
	}

	obj_field = lwm2m_get_engine_obj_field(obj_inst->obj,
					       msg->path.res_id);
	if (!obj_field) {
		ret = -ENOENT;
		goto error;
	}

	if (!LWM2M_HAS_PERM(obj_field, LWM2M_PERM_W)) {
		ret = -EPERM;
		goto error;
	}

	if (!obj_inst->resources || obj_inst->resource_count == 0U) {
		ret = -EINVAL;
		goto error;
	}

	for (i = 0; i < obj_inst->resource_count; i++) {
		if (obj_inst->resources[i].res_id == msg->path.res_id) {
			res = &obj_inst->resources[i];
			break;
		}
	}

	if (res) {
		for (i = 0; i < res->res_inst_count; i++) {
			if (res->res_instances[i].res_inst_id ==
			    msg->path.res_inst_id) {
				res_inst = &res->res_instances[i];
				break;
			}
		}
	}

	if (!res || !res_inst) {
		/* if OPTIONAL and BOOTSTRAP-WRITE or CREATE use ENOTSUP */
		if ((msg->ctx->bootstrap_mode ||
		     msg->operation == LWM2M_OP_CREATE) &&
		    LWM2M_HAS_PERM(obj_field, BIT(LWM2M_FLAG_OPTIONAL))) {
			ret = -ENOTSUP;
			goto error;
		}

		ret = -ENOENT;
		goto error;
	}

	ret = lwm2m_write_handler(obj_inst, res, res_inst, obj_field, msg);
	if (ret == -EACCES || ret == -ENOENT) {
		/* if read-only or non-existent data buffer move on */
		do_write_op_tlv_dummy_read(msg);
		ret = 0;
	}

	return ret;

error:
	do_write_op_tlv_dummy_read(msg);
	return ret;
}

int do_write_op_tlv(struct lwm2m_message *msg)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	size_t len;
	struct oma_tlv tlv;
	int ret;

	/* In case of Firmware object Package resource go directly to the
	 * message processing - consecutive blocks will not carry the TLV
	 * header.
	 */
	if (msg->path.obj_id == 5 && msg->path.res_id == 0) {
		ret = do_write_op_tlv_item(msg);
		if (ret < 0) {
			return ret;
		}
	}

	while (true) {
		/*
		 * This initial read of TLV data won't advance frag/offset.
		 * We need tlv.type to determine how to proceed.
		 */
		len = oma_tlv_get(&tlv, &msg->in, true);
		if (len == 0) {
			break;
		}

		if (tlv.type == OMA_TLV_TYPE_OBJECT_INSTANCE) {
			struct oma_tlv tlv2;
			int len2;
			int pos = 0;

			oma_tlv_get(&tlv, &msg->in, false);
			msg->path.obj_inst_id = tlv.id;
			if (tlv.length == 0U) {
				/* Create only - no data */
				ret = lwm2m_create_obj_inst(
						msg->path.obj_id,
						msg->path.obj_inst_id,
						&obj_inst);
				if (ret < 0) {
					return ret;
				}

#ifdef CONFIG_LWM2M_RD_CLIENT_SUPPORT
				if (!msg->ctx->bootstrap_mode) {
					engine_trigger_update();
				}
#endif
			}

			while (pos < tlv.length &&
			       (len2 = oma_tlv_get(&tlv2, &msg->in, true))) {
				if (tlv2.type != OMA_TLV_TYPE_RESOURCE) {
					pos += len2;
					continue;
				}

				msg->path.res_id = tlv2.id;
				msg->path.level = 3U;
				ret = do_write_op_tlv_item(msg);
				/*
				 * ignore errors for CREATE op
				 * for OP_CREATE and BOOTSTRAP WRITE: errors on
				 * optional resources are ignored (ENOTSUP)
				 */
				if (ret < 0 &&
				    !((ret == -ENOTSUP) &&
				      (msg->ctx->bootstrap_mode ||
				       msg->operation == LWM2M_OP_CREATE))) {
					return ret;
				}

				pos += len2;
			}
		} else if (tlv.type == OMA_TLV_TYPE_RESOURCE) {
			msg->path.res_id = tlv.id;
			msg->path.level = 3U;
			ret = do_write_op_tlv_item(msg);
			/*
			 * ignore errors for CREATE op
			 * for OP_CREATE and BOOTSTRAP WRITE: errors on optional
			 * resources are ignored (ENOTSUP)
			 */
			if (ret < 0 &&
			    !((ret == -ENOTSUP) &&
			      (msg->ctx->bootstrap_mode ||
			       msg->operation == LWM2M_OP_CREATE))) {
				return ret;
			}
		} else {
			return -ENOTSUP;
		}
	}

	return 0;
}
