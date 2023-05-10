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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <string.h>
#include <stdint.h>
#include <zephyr/sys/byteorder.h>

#include "lwm2m_rw_oma_tlv.h"
#include "lwm2m_engine.h"
#include "lwm2m_rd_client.h"
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
			return -EINVAL;
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

static int oma_tlv_put(const struct oma_tlv *tlv,
		       struct lwm2m_output_context *out, uint8_t *value,
		       bool insert)
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
		return ret;
	}

	pos = 1;

	/* The ID */
	if (tlv->id > 255) {
		ret = oma_tlv_put_u8(out, (tlv->id >> 8) & 0xff, insert);
		if (ret < 0) {
			return ret;
		}

		pos++;
	}

	ret = oma_tlv_put_u8(out, tlv->id & 0xff, insert);
	if (ret < 0) {
		return ret;
	}

	pos++;

	for (i = 2; i >= 0; i--) {
		if (len_type > i) {
			ret = oma_tlv_put_u8(out,
					     (tlv->length >> (i * 8)) & 0xff,
					     insert);
			if (ret < 0) {
				return ret;
			}

			pos++;
		}
	}

	/* finally add the value */
	if (value != NULL && tlv->length > 0 && !insert) {
		ret = buf_append(CPKT_BUF_WRITE(out->out_cpkt),
				 value, tlv->length);
		if (ret < 0) {
			return ret;
		}
	}

	return pos + tlv->length;
}

static int oma_tlv_get(struct oma_tlv *tlv, struct lwm2m_input_context *in,
		       bool dont_advance)
{
	uint8_t len_type;
	uint8_t len_pos = 1U;
	size_t tlv_len;
	uint16_t tmp_offset;
	uint8_t buf[2];
	int ret;

	tmp_offset = in->offset;
	ret = buf_read_u8(&buf[0], CPKT_BUF_READ(in->in_cpkt), &tmp_offset);
	if (ret < 0) {
		goto error;
	}

	tlv->type = (buf[0] >> 6) & 3;
	len_type = (buf[0] >> 3) & 3;
	len_pos = 1 + (((buf[0] & (1 << 5)) != 0U) ? 2 : 1);

	ret = buf_read_u8(&buf[1], CPKT_BUF_READ(in->in_cpkt), &tmp_offset);
	if (ret < 0) {
		goto error;
	}

	tlv->id = buf[1];

	/* if len_pos > 2 it means that there are more ID to read */
	if (len_pos > 2) {
		ret = buf_read_u8(&buf[1], CPKT_BUF_READ(in->in_cpkt),
				  &tmp_offset);
		if (ret < 0) {
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
			ret = buf_read_u8(&buf[1], CPKT_BUF_READ(in->in_cpkt),
					  &tmp_offset);
			if (ret < 0) {
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
	if (!dont_advance) {
		in->offset = tmp_offset;
	}

	return ret;
}

static int put_begin_tlv(struct lwm2m_output_context *out, uint16_t *mark_pos,
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

static int put_end_tlv(struct lwm2m_output_context *out, uint16_t mark_pos,
		       uint8_t *writer_flags, uint8_t writer_flag, int tlv_type,
		       int tlv_id)
{
	struct tlv_out_formatter_data *fd;
	struct oma_tlv tlv;
	int len;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
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

static int put_begin_oi(struct lwm2m_output_context *out,
			struct lwm2m_obj_path *path)
{
	struct tlv_out_formatter_data *fd;

	/* No need for oi level TLV constructs */
	if (path->level > LWM2M_PATH_LEVEL_OBJECT) {
		return 0;
	}

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	return put_begin_tlv(out, &fd->mark_pos_oi, &fd->writer_flags, 0);
}

static int put_end_oi(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path)
{
	struct tlv_out_formatter_data *fd;

	if (path->level > LWM2M_PATH_LEVEL_OBJECT) {
		return 0;
	}

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	return put_end_tlv(out, fd->mark_pos_oi, &fd->writer_flags, 0,
			   OMA_TLV_TYPE_OBJECT_INSTANCE, path->obj_inst_id);
}

static int put_begin_ri(struct lwm2m_output_context *out,
			struct lwm2m_obj_path *path)
{
	struct tlv_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	return put_begin_tlv(out, &fd->mark_pos_ri, &fd->writer_flags,
			     WRITER_RESOURCE_INSTANCE);
}

static int put_end_ri(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path)
{
	struct tlv_out_formatter_data *fd;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	/* Skip writing Multiple Resource TLV if path level is 4 */
	if (IS_ENABLED(CONFIG_LWM2M_VERSION_1_1) &&
		path->level == LWM2M_PATH_LEVEL_RESOURCE_INST) {
		return 0;
	}

	return put_end_tlv(out, fd->mark_pos_ri, &fd->writer_flags,
			   WRITER_RESOURCE_INSTANCE,
			   OMA_TLV_TYPE_MULTI_RESOURCE, path->res_id);
}

static int put_s8(struct lwm2m_output_context *out, struct lwm2m_obj_path *path,
		  int8_t value)
{
	struct tlv_out_formatter_data *fd;
	int len;
	struct oma_tlv tlv;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	tlv_setup(&tlv, tlv_calc_type(fd->writer_flags),
		  tlv_calc_id(fd->writer_flags, path), sizeof(value));

	len = oma_tlv_put(&tlv, out, (uint8_t *)&value, false);
	return len;
}

static int put_s16(struct lwm2m_output_context *out,
		   struct lwm2m_obj_path *path, int16_t value)
{
	struct tlv_out_formatter_data *fd;
	int len;
	struct oma_tlv tlv;
	int16_t net_value;

	if (INT8_MIN <= value && value <= INT8_MAX) {
		return put_s8(out, path, (int8_t)value);
	}

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	net_value = sys_cpu_to_be16(value);
	tlv_setup(&tlv, tlv_calc_type(fd->writer_flags),
		  tlv_calc_id(fd->writer_flags, path), sizeof(net_value));

	len = oma_tlv_put(&tlv, out, (uint8_t *)&net_value, false);
	return len;
}

static int put_s32(struct lwm2m_output_context *out,
		   struct lwm2m_obj_path *path, int32_t value)
{
	struct tlv_out_formatter_data *fd;
	int len;
	struct oma_tlv tlv;
	int32_t net_value;

	if (INT16_MIN <= value && value <= INT16_MAX) {
		return put_s16(out, path, (int16_t)value);
	}

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	net_value = sys_cpu_to_be32(value);
	tlv_setup(&tlv, tlv_calc_type(fd->writer_flags),
		  tlv_calc_id(fd->writer_flags, path), sizeof(net_value));

	len = oma_tlv_put(&tlv, out, (uint8_t *)&net_value, false);

	return len;
}

static int put_s64(struct lwm2m_output_context *out,
		   struct lwm2m_obj_path *path, int64_t value)
{
	struct tlv_out_formatter_data *fd;
	int len;
	struct oma_tlv tlv;
	int64_t net_value;

	if (INT32_MIN <= value && value <= INT32_MAX) {
		return put_s32(out, path, (int32_t)value);
	}

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	net_value = sys_cpu_to_be64(value);
	tlv_setup(&tlv, tlv_calc_type(fd->writer_flags),
		  tlv_calc_id(fd->writer_flags, path), sizeof(net_value));

	len = oma_tlv_put(&tlv, out, (uint8_t *)&net_value, false);
	return len;
}


static int put_time(struct lwm2m_output_context *out, struct lwm2m_obj_path *path, time_t value)
{
	return put_s64(out, path, (int64_t)value);
}

static int put_string(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, char *buf, size_t buflen)
{
	struct tlv_out_formatter_data *fd;
	int len;
	struct oma_tlv tlv;

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	tlv_setup(&tlv, tlv_calc_type(fd->writer_flags),
		  tlv_calc_id(fd->writer_flags, path), (uint32_t)buflen);
	len = oma_tlv_put(&tlv, out, (uint8_t *)buf, false);
	return len;
}

static int put_float(struct lwm2m_output_context *out,
		     struct lwm2m_obj_path *path, double *value)
{
	struct tlv_out_formatter_data *fd;
	int len;
	struct oma_tlv tlv;
	int ret;
	uint8_t b64[8];

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	ret = lwm2m_float_to_b64(value, b64, sizeof(b64));
	if (ret < 0) {
		LOG_ERR("float32 conversion error: %d", ret);
		return ret;
	}

	tlv_setup(&tlv, tlv_calc_type(fd->writer_flags),
		  tlv_calc_id(fd->writer_flags, path), sizeof(b64));
	len = oma_tlv_put(&tlv, out, b64, false);
	return len;
}

static int put_bool(struct lwm2m_output_context *out,
		    struct lwm2m_obj_path *path, bool value)
{
	int8_t value_s8 = (value != 0 ? 1 : 0);

	return put_s8(out, path, value_s8);
}

static int put_opaque(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, char *buf, size_t buflen)
{
	return put_string(out, path, buf, buflen);
}

static int put_objlnk(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, struct lwm2m_objlnk *value)
{
	struct tlv_out_formatter_data *fd;
	struct oma_tlv tlv;
	int32_t net_value = sys_cpu_to_be32(
				((value->obj_id) << 16) | value->obj_inst);

	fd = engine_get_out_user_data(out);
	if (!fd) {
		return -EINVAL;
	}

	tlv_setup(&tlv, tlv_calc_type(fd->writer_flags),
		  tlv_calc_id(fd->writer_flags, path), sizeof(net_value));

	return oma_tlv_put(&tlv, out, (uint8_t *)&net_value, false);
}

static int get_number(struct lwm2m_input_context *in, int64_t *value,
		      uint8_t max_len)
{
	struct oma_tlv tlv;
	int size;
	int64_t temp;
	int ret;

	size = oma_tlv_get(&tlv, in, false);
	if (size < 0) {
		return size;
	}

	if (tlv.length > max_len) {
		LOG_ERR("invalid length: %u", tlv.length);
		return -ENOMEM;
	}

	ret = buf_read((uint8_t *)&temp, tlv.length,
		       CPKT_BUF_READ(in->in_cpkt), &in->offset);
	if (ret < 0) {
		return ret;
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
		return -EBADMSG;
	}

	return size;
}

static int get_s64(struct lwm2m_input_context *in, int64_t *value)
{
	return get_number(in, value, 8);
}

static int get_time(struct lwm2m_input_context *in, time_t *value)
{
	int64_t temp64;
	int ret;

	ret = get_number(in, &temp64, 8);
	*value = (time_t)temp64;

	return ret;
}

static int get_s32(struct lwm2m_input_context *in, int32_t *value)
{
	int64_t temp;
	int size;

	size = get_number(in, &temp, 4);
	if (size < 0) {
		return size;
	}

	*value = (int32_t)temp;

	return size;
}

static int get_string(struct lwm2m_input_context *in, uint8_t *buf,
		      size_t buflen)
{
	struct oma_tlv tlv;
	int size;
	int ret;

	size = oma_tlv_get(&tlv, in, false);
	if (size < 0) {
		return size;
	}

	if (buflen <= tlv.length) {
		return -ENOMEM;
	}

	ret = buf_read(buf, tlv.length, CPKT_BUF_READ(in->in_cpkt),
		       &in->offset);
	if (ret < 0) {
		return ret;
	}

	buf[tlv.length] = '\0';

	return size;
}

/* convert float to fixpoint */
static int get_float(struct lwm2m_input_context *in, double *value)
{
	struct oma_tlv tlv;
	int size;
	uint8_t buf[8];
	int ret;

	size = oma_tlv_get(&tlv, in, false);
	if (size < 0) {
		return size;
	}

	if (tlv.length != 4U && tlv.length != 8U) {
		LOG_ERR("Invalid float length: %d", tlv.length);
		return -EBADMSG;
	}

	/* read float in network byte order */
	ret = buf_read(buf, tlv.length, CPKT_BUF_READ(in->in_cpkt),
		       &in->offset);
	if (ret < 0) {
		return ret;
	}

	if (tlv.length == 4U) {
		ret = lwm2m_b32_to_float(buf, 4, value);
	} else {
		ret = lwm2m_b64_to_float(buf, 8, value);
	}

	if (ret < 0) {
		LOG_ERR("binary%s conversion error: %d",
			tlv.length == 4U ? "32" : "64", ret);
		return ret;
	}

	return size;
}

static int get_bool(struct lwm2m_input_context *in, bool *value)
{
	int64_t temp;
	int size;

	size = get_number(in, &temp, 2);
	if (size < 0) {
		return size;

	}

	*value = (temp != 0);

	return size;
}

static int get_opaque(struct lwm2m_input_context *in, uint8_t *value,
		      size_t buflen, struct lwm2m_opaque_context *opaque,
		      bool *last_block)
{
	struct oma_tlv tlv;
	int size;

	/* Get the TLV header only on first read. */
	if (opaque->remaining == 0) {
		size = oma_tlv_get(&tlv, in, false);
		if (size < 0) {
			return size;
		}

		opaque->len = tlv.length;
		opaque->remaining = tlv.length;
	}

	return lwm2m_engine_get_opaque_more(in, value, buflen,
					    opaque, last_block);
}

static int get_objlnk(struct lwm2m_input_context *in,
		      struct lwm2m_objlnk *value)
{
	int32_t value_s32;
	int size;

	size = get_s32(in, &value_s32);
	if (size < 0) {
		return size;
	}

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
	.put_float = put_float,
	.put_time = put_time,
	.put_bool = put_bool,
	.put_opaque = put_opaque,
	.put_objlnk = put_objlnk,
};

const struct lwm2m_reader oma_tlv_reader = {
	.get_s32 = get_s32,
	.get_s64 = get_s64,
	.get_string = get_string,
	.get_time = get_time,
	.get_float = get_float,
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
	int ret;

	ret = lwm2m_get_or_create_engine_obj(msg, &obj_inst, &created);
	if (ret < 0) {
		goto error;
	}

	ret = lwm2m_engine_validate_write_access(msg, obj_inst, &obj_field);
	if (ret < 0) {
		goto error;
	}

	ret = lwm2m_engine_get_create_res_inst(&msg->path, &res, &res_inst);

	if (ret < 0) {
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

static int write_tlv_resource(struct lwm2m_message *msg, struct oma_tlv *tlv)
{
	int ret;

	if (msg->in.block_ctx) {
		msg->in.block_ctx->path.res_id = tlv->id;
	}

	msg->path.res_id = tlv->id;
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

	return 0;
}

static int lwm2m_multi_resource_tlv_parse(struct lwm2m_message *msg,
					  struct oma_tlv *multi_resource_tlv)
{
	struct oma_tlv tlv_resource_instance;
	int len2;
	int pos = 0;
	int ret;

	if (msg->in.block_ctx) {
		msg->in.block_ctx->path.res_id = multi_resource_tlv->id;
	}

	if (multi_resource_tlv->length == 0U) {
		/* No data for resource instances, so create only a resource */
		return write_tlv_resource(msg, multi_resource_tlv);
	}

	while (pos < multi_resource_tlv->length &&
	       (len2 = oma_tlv_get(&tlv_resource_instance, &msg->in, true))) {
		if (tlv_resource_instance.type != OMA_TLV_TYPE_RESOURCE_INSTANCE) {
			LOG_ERR("Multi resource id not supported %u %d", tlv_resource_instance.id,
				tlv_resource_instance.length);
			return -ENOTSUP;
		}

		msg->path.res_id = multi_resource_tlv->id;
		msg->path.res_inst_id = tlv_resource_instance.id;
		msg->path.level = LWM2M_PATH_LEVEL_RESOURCE_INST;
		ret = do_write_op_tlv_item(msg);

		/*
		 * Ignore errors on optional resources when doing
		 * BOOTSTRAP WRITE or CREATE operation.
		 */
		if (ret < 0 && !((ret == -ENOTSUP) &&
				 (msg->ctx->bootstrap_mode || msg->operation == LWM2M_OP_CREATE))) {
			return ret;
		}

		pos += len2;
	}

	return 0;
}

int do_write_op_tlv(struct lwm2m_message *msg)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	int len;
	struct oma_tlv tlv;
	int ret;

	/* In case of block transfer go directly to the
	 * message processing - consecutive blocks will not carry the TLV
	 * header.
	 */
	if (msg->in.block_ctx != NULL && msg->in.block_ctx->ctx.current > 0) {
		msg->path.res_id = msg->in.block_ctx->path.res_id;
		msg->path.level = 3U;
		ret = do_write_op_tlv_item(msg);
		if (ret < 0) {
			return ret;
		}

		return 0;
	}

	while (true) {
		/*
		 * This initial read of TLV data won't advance frag/offset.
		 * We need tlv.type to determine how to proceed.
		 */
		len = oma_tlv_get(&tlv, &msg->in, true);
		if (len < 0) {
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

				if (!msg->ctx->bootstrap_mode) {
					engine_trigger_update(true);
				}
			}

			while (pos < tlv.length &&
			       (len2 = oma_tlv_get(&tlv2, &msg->in, true))) {
				if (tlv2.type == OMA_TLV_TYPE_RESOURCE) {
					ret = write_tlv_resource(msg, &tlv2);
					if (ret) {
						return ret;
					}
				} else if (tlv2.type == OMA_TLV_TYPE_MULTI_RESOURCE) {
					oma_tlv_get(&tlv2, &msg->in, false);
					ret = lwm2m_multi_resource_tlv_parse(msg, &tlv2);
					if (ret) {
						return ret;
					}
				} else {
					/* Skip Unsupported TLV type */
					return -ENOTSUP;
				}

				pos += len2;
			}
		} else if (tlv.type == OMA_TLV_TYPE_RESOURCE) {
			if (msg->path.level < LWM2M_PATH_LEVEL_OBJECT_INST) {
				return -ENOTSUP;
			}
			ret = write_tlv_resource(msg, &tlv);
			if (ret) {
				return ret;
			}
		} else if (tlv.type == OMA_TLV_TYPE_MULTI_RESOURCE) {
			if (msg->path.level < LWM2M_PATH_LEVEL_OBJECT_INST) {
				return -ENOTSUP;
			}
			oma_tlv_get(&tlv, &msg->in, false);
			ret = lwm2m_multi_resource_tlv_parse(msg, &tlv);
			if (ret) {
				return ret;
			}
		} else {
			return -ENOTSUP;
		}
	}

	return 0;
}
