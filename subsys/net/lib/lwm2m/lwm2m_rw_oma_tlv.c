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
 * - Lots of byte-order API clean up
 * - Var / parameter type cleanup
 * - Replace magic #'s with defines
 */

#define SYS_LOG_DOMAIN "lib/lwm2m_oma_tlv"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_LWM2M_LEVEL
#include <logging/sys_log.h>
#include <string.h>
#include <stdint.h>
#include <misc/byteorder.h>

#include "lwm2m_rw_oma_tlv.h"
#include "lwm2m_engine.h"

static u8_t get_len_type(const struct oma_tlv *tlv)
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

static u8_t tlv_calc_type(u8_t flags)
{
	return flags & WRITER_RESOURCE_INSTANCE ?
			OMA_TLV_TYPE_RESOURCE_INSTANCE : OMA_TLV_TYPE_RESOURCE;
}

static u16_t tlv_calc_id(u8_t flags, struct lwm2m_obj_path *path)
{
	return flags & WRITER_RESOURCE_INSTANCE ?
			path->res_inst_id : path->res_id;
}

static void tlv_setup(struct oma_tlv *tlv, u8_t type, u16_t id,
		     u32_t len, const u8_t *value)
{
	if (tlv) {
		tlv->type = type;
		tlv->id = id;
		tlv->length = len;
		tlv->value = value;
	}
}

static size_t oma_tlv_put(const struct oma_tlv *tlv, u8_t *buffer, size_t len)
{
	int pos;
	u8_t len_type;

	/* len type is the same as number of bytes required for length */
	len_type = get_len_type(tlv);
	pos = 1 + len_type;
	/* ensure that we do not write too much */
	if (len < tlv->length + pos) {
		SYS_LOG_ERR("OMA-TLV: Could not write the TLV - buffer overflow"
			    " (len:%zd < tlv->length:%d + pos:%d)",
			    len, tlv->length, pos);
		return 0;
	}

	/* first type byte in TLV header */
	buffer[0] = (tlv->type << 6) |
		    (tlv->id > 255 ? (1 << 5) : 0) |
		    (len_type << 3) |
		    (len_type == 0 ? tlv->length : 0);
	pos = 1;

	/* The ID */
	if (tlv->id > 255) {
		buffer[pos++] = (tlv->id >> 8) & 0xff;
	}

	buffer[pos++] = tlv->id & 0xff;

	/* Add length if needed - unrolled loop ? */
	if (len_type > 2) {
		buffer[pos++] = (tlv->length >> 16) & 0xff;
	}

	if (len_type > 1) {
		buffer[pos++] = (tlv->length >> 8) & 0xff;
	}

	if (len_type > 0) {
		buffer[pos++] = tlv->length & 0xff;
	}

	/* finally add the value */
	if (tlv->value != NULL && tlv->length > 0) {
		memcpy(&buffer[pos], tlv->value, tlv->length);
	}

	/* TODO: Add debug print of TLV */
	return pos + tlv->length;
}

size_t oma_tlv_get(struct oma_tlv *tlv, const u8_t *buffer, size_t len)
{
	u8_t len_type;
	u8_t len_pos = 1;
	size_t tlv_len;

	tlv->type = (buffer[0] >> 6) & 3;
	len_type = (buffer[0] >> 3) & 3;
	len_pos = 1 + (((buffer[0] & (1 << 5)) != 0) ? 2 : 1);
	tlv->id = buffer[1];

	/* if len_pos > 2 it means that there are more ID to read */
	if (len_pos > 2) {
		tlv->id = (tlv->id << 8) + buffer[2];
	}

	if (len_type == 0) {
		tlv_len = buffer[0] & 7;
	} else {
		/* read the length */
		tlv_len = 0;
		while (len_type > 0) {
			tlv_len = tlv_len << 8 | buffer[len_pos++];
			len_type--;
		}
	}

	/* and read out the data??? */
	tlv->length = tlv_len;
	tlv->value = &buffer[len_pos];
	return len_pos + tlv_len;
}

static size_t put_begin_ri(struct lwm2m_output_context *out,
			   struct lwm2m_obj_path *path)
{
	/* set some flags in state */
	struct oma_tlv tlv;
	size_t len;

	out->writer_flags |= WRITER_RESOURCE_INSTANCE;
	tlv_setup(&tlv, OMA_TLV_TYPE_MULTI_RESOURCE, path->res_id, 8, NULL);

	/* we remove the nonsense payload here (len = 8) */
	len = oma_tlv_put(&tlv, &out->outbuf[out->outlen],
			    out->outsize - out->outlen) - 8;
	/*
	 * store position for deciding where to re-write the TLV when we
	 * know the length - NOTE: either this or memmov of buffer later...
	 */
	out->mark_pos_ri = out->outlen;
	out->outlen += len;
	return len;
}

static size_t put_end_ri(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path)
{
	/* clear out state info */
	int pos = 2; /* this is the length pos */
	size_t len;

	out->writer_flags &= ~WRITER_RESOURCE_INSTANCE;
	if (path->res_id > 0xff) {
		pos++;
	}

	len = out->outlen - out->mark_pos_ri;

	/* update the length byte... Assume TLV header is pos + 1 bytes. */
	out->outbuf[pos + out->mark_pos_ri] = len - (pos + 1);
	return 0;
}

static size_t put_s8(struct lwm2m_output_context *out,
		     struct lwm2m_obj_path *path, s8_t value)
{
	size_t len;
	struct oma_tlv tlv;

	tlv_setup(&tlv, tlv_calc_type(out->writer_flags),
		  tlv_calc_id(out->writer_flags, path), sizeof(value),
		  (u8_t *)&value);

	len = oma_tlv_put(&tlv, &out->outbuf[out->outlen],
			  out->outsize - out->outlen);
	out->outlen += len;
	return len;
}

static size_t put_s16(struct lwm2m_output_context *out,
		      struct lwm2m_obj_path *path, s16_t value)
{
	size_t len;
	struct oma_tlv tlv;
	s16_t net_value;

	net_value = sys_cpu_to_be16(value);
	tlv_setup(&tlv, tlv_calc_type(out->writer_flags),
		  tlv_calc_id(out->writer_flags, path), sizeof(net_value),
		  (u8_t *)&net_value);

	len = oma_tlv_put(&tlv, &out->outbuf[out->outlen],
			  out->outsize - out->outlen);
	out->outlen += len;
	return len;
}

static size_t put_s32(struct lwm2m_output_context *out,
			struct lwm2m_obj_path *path, s32_t value)
{
	size_t len;
	struct oma_tlv tlv;
	s32_t net_value;

	net_value = sys_cpu_to_be32(value);
	tlv_setup(&tlv, tlv_calc_type(out->writer_flags),
		  tlv_calc_id(out->writer_flags, path), sizeof(net_value),
		  (u8_t *)&net_value);

	len = oma_tlv_put(&tlv, &out->outbuf[out->outlen],
			  out->outsize - out->outlen);
	out->outlen += len;
	return len;
}

static size_t put_s64(struct lwm2m_output_context *out,
			struct lwm2m_obj_path *path, s64_t value)
{
	size_t len;
	struct oma_tlv tlv;
	s64_t net_value;

	net_value = sys_cpu_to_be64(value);
	tlv_setup(&tlv, tlv_calc_type(out->writer_flags),
		  tlv_calc_id(out->writer_flags, path), sizeof(net_value),
		  (u8_t *)&net_value);

	len = oma_tlv_put(&tlv, &out->outbuf[out->outlen],
			  out->outsize - out->outlen);
	out->outlen += len;
	return len;
}

static size_t put_string(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path,
			 const char *value, size_t strlen)
{
	size_t len;
	struct oma_tlv tlv;

	tlv_setup(&tlv, tlv_calc_type(out->writer_flags),
		  path->res_id, (u32_t)strlen,
		  (u8_t *)value);
	len = oma_tlv_put(&tlv, &out->outbuf[out->outlen],
			    out->outsize - out->outlen);
	out->outlen += len;
	return len;
}

/* use binary32 format */
static size_t put_float32fix(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path,
			     float32_value_t *value)
{
	size_t len;
	struct oma_tlv tlv;
	u8_t *buffer = &out->outbuf[out->outlen];
	int e = 0;
	s32_t val = 0;
	s32_t v;
	u8_t b[4];

	/*
	 * TODO: Currently, there is no standard API for handling the decimal
	 * portion of the float32_value structure.  In the future, we should use
	 * the value->val2 (decimal portion) to set the decimal mask and in the
	 * following binary float calculations.
	 *
	 * HACK BELOW: hard code the decimal mask to 0 (whole number)
	 */
	int bits = 0;

	v = value->val1;
	if (v < 0) {
		v = -v;
	}

	while (v > 1) {
		val = (val >> 1);

		if (v & 1) {
			val = val | (1L << 22);
		}

		v = (v >> 1);
		e++;
	}

	/* convert to the thing we should have */
	e = e - bits + 127;
	if (value->val1 == 0) {
		e = 0;
	}

	/* is this the right byte order? */
	b[0] = (value->val1 < 0 ? 0x80 : 0) | (e >> 1);
	b[1] = ((e & 1) << 7) | ((val >> 16) & 0x7f);
	b[2] = (val >> 8) & 0xff;
	b[3] = val & 0xff;

	tlv_setup(&tlv, tlv_calc_type(out->writer_flags),
		  tlv_calc_id(out->writer_flags, path),
		  4, b);
	len = oma_tlv_put(&tlv, buffer, out->outsize - out->outlen);
	out->outlen += len;

	return len;
}

static size_t put_float64fix(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path,
			     float64_value_t *value)
{
	size_t len;
	s64_t binary64 = 0, net_binary64 = 0;
	struct oma_tlv tlv;
	u8_t *buffer = &out->outbuf[out->outlen];

	/* TODO */

	net_binary64 = sys_cpu_to_be64(binary64);
	tlv_setup(&tlv, tlv_calc_type(out->writer_flags),
		  tlv_calc_id(out->writer_flags, path),
		  sizeof(net_binary64), (u8_t *)&net_binary64);
	len = oma_tlv_put(&tlv, buffer, out->outsize - out->outlen);
	out->outlen += len;
	return len;
}

static size_t put_bool(struct lwm2m_output_context *out,
		       struct lwm2m_obj_path *path, bool value)
{
	return put_s8(out, path, value != 0 ? 1 : 0);
}

static size_t get_s32(struct lwm2m_input_context *in, s32_t *value)
{
	struct oma_tlv tlv;
	size_t size = oma_tlv_get(&tlv, in->inbuf, in->insize);

	*value = 0;
	if (size > 0) {
		switch (tlv.length) {
		case 1:
			*value = *(s8_t *)tlv.value;
			break;
		case 2:
			*value = sys_cpu_to_be16(*(s16_t *)tlv.value);
			break;
		case 4:
			*value = sys_cpu_to_be32(*(s32_t *)tlv.value);
			break;
		default:
			SYS_LOG_ERR("invalid length: %u", tlv.length);
			size = 0;
			tlv.length = 0;
		}

		in->last_value_len = tlv.length;
	}

	return size;
}

/* TODO: research to make sure this is correct */
static size_t get_s64(struct lwm2m_input_context *in, s64_t *value)
{
	struct oma_tlv tlv;
	size_t size = oma_tlv_get(&tlv, in->inbuf, in->insize);

	*value = 0;
	if (size > 0) {
		switch (tlv.length) {
		case 1:
			*value = *tlv.value;
			break;
		case 2:
			*value = sys_cpu_to_be16(*(s16_t *)tlv.value);
			break;
		case 4:
			*value = sys_cpu_to_be32(*(s32_t *)tlv.value);
			break;
		case 8:
			*value = sys_cpu_to_be64(*(s64_t *)tlv.value);
			break;
		default:
			SYS_LOG_ERR("invalid length: %u", tlv.length);
			size = 0;
			tlv.length = 0;
		}

		in->last_value_len = tlv.length;
	}

	return size;
}

static size_t get_string(struct lwm2m_input_context *in,
			 u8_t *value, size_t strlen)
{
	struct oma_tlv tlv;
	size_t size = oma_tlv_get(&tlv, in->inbuf, in->insize);

	if (size > 0) {
		if (strlen <= tlv.length) {
			/*
			 * The outbuffer can not contain the
			 * full string including ending zero
			 */
			return 0;
		}

		memcpy(value, tlv.value, tlv.length);
		value[tlv.length] = '\0';
		in->last_value_len = tlv.length;
	}

	return size;
}

/* convert float to fixpoint */
static size_t get_float32fix(struct lwm2m_input_context *in,
			     float32_value_t *value)
{
	struct oma_tlv tlv;
	size_t size = oma_tlv_get(&tlv, in->inbuf, in->insize);
	int e;
	s32_t val;
	int sign = (tlv.value[0] & 0x80) != 0;
	/* */
	int bits = 0;

	if (size > 0) {
		/* TLV needs to be 4 bytes */
		e = ((tlv.value[0] << 1) & 0xff) | (tlv.value[1] >> 7);
		val = (((long)tlv.value[1] & 0x7f) << 16) |
		      (tlv.value[2] << 8) |
		      tlv.value[3];
		e = e - 127 + bits;

		/* e is the number of times we need to roll the number */
		SYS_LOG_DBG("Actual e=%d", e);
		e = e - 23;
		SYS_LOG_DBG("E after sub %d", e);
		val = val | 1L << 23;
		if (e > 0) {
			val = val << e;
		} else {
			val = val >> -e;
		}

		value->val1 = sign ? -val : val;

		/*
		 * TODO: Currently, there is no standard API for handling the
		 * decimal portion of the float32_value structure.  In the
		 * future, once that is settled, we should calculate
		 * value->val2 in the above float calculations.
		 *
		 * HACK BELOW: hard code the decimal value 0
		 */
		value->val2 = 0;
		in->last_value_len = tlv.length;
	}

	return size;
}

static size_t get_float64fix(struct lwm2m_input_context *in,
			     float64_value_t *value)
{
	struct oma_tlv tlv;
	size_t size = oma_tlv_get(&tlv, in->inbuf, in->insize);

	if (size > 0) {
		if (tlv.length != 8) {
			SYS_LOG_ERR("invalid length: %u (not 8)", tlv.length);
			return 0;
		}

		/* TODO */

		in->last_value_len = tlv.length;
	}

	return size;
}

static size_t get_bool(struct lwm2m_input_context *in, bool *value)
{
	struct oma_tlv tlv;
	size_t size = oma_tlv_get(&tlv, in->inbuf, in->insize);
	int i;
	s32_t temp = 0;

	if (size > 0) {
		/* will probably need to handle MSB as a sign bit? */
		for (i = 0; i < tlv.length; i++) {
			temp = (temp << 8) | tlv.value[i];
		}
		*value = (temp != 0);
		in->last_value_len = tlv.length;
	}

	return size;
}

const struct lwm2m_writer oma_tlv_writer = {
	NULL,
	NULL,
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

const struct lwm2m_reader oma_tlv_reader = {
	get_s32,
	get_s64,
	get_string,
	get_float32fix,
	get_float64fix,
	get_bool
};

static int do_write_op_tlv_item(struct lwm2m_engine_context *context,
				u8_t *data, int len)
{
	struct lwm2m_input_context *in = context->in;
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_engine_res_inst *res = NULL;
	struct lwm2m_engine_obj_field *obj_field = NULL;
	u8_t created = 0;
	int ret, i;

	in->inbuf = data;
	in->inpos = 0;
	in->insize = len;

	ret = lwm2m_get_or_create_engine_obj(context, &obj_inst, &created);
	if (ret < 0) {
		return ret;
	}

	obj_field = lwm2m_get_engine_obj_field(obj_inst->obj,
					       context->path->res_id);
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
		if (obj_inst->resources[i].res_id == context->path->res_id) {
			res = &obj_inst->resources[i];
			break;
		}
	}

	if (!res) {
		return -ENOENT;
	}

	return lwm2m_write_handler(obj_inst, res, obj_field, context);
}

int do_write_op_tlv(struct lwm2m_engine_obj *obj,
		    struct lwm2m_engine_context *context)
{
	struct lwm2m_input_context *in = context->in;
	struct lwm2m_obj_path *path = context->path;
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	size_t len;
	struct oma_tlv tlv;
	int tlvpos = 0, ret;
	u16_t insize = in->insize;
	u8_t *inbuf = in->inbuf;

	while (tlvpos < insize) {
		len = oma_tlv_get(&tlv, &inbuf[tlvpos],
				  insize - tlvpos);

		SYS_LOG_DBG("Got TLV format First is: type:%d id:%d "
			    "len:%d (p:%d len:%d/%d)",
			    tlv.type, tlv.id, (int) tlv.length,
			    (int) tlvpos, (int) len, (int) insize);

		if (tlv.type == OMA_TLV_TYPE_OBJECT_INSTANCE) {
			struct oma_tlv tlv2;
			int len2;
			int pos = 0;

			path->obj_inst_id = tlv.id;
			if (tlv.length == 0) {
				/* Create only - no data */
				ret = lwm2m_create_obj_inst(path->obj_id,
							    path->obj_inst_id,
							    &obj_inst);
				if (ret < 0) {
					return ret;
				}
			}

			while (pos < tlv.length &&
			       (len2 = oma_tlv_get(&tlv2, &tlv.value[pos],
						    tlv.length - pos))) {
				if (tlv2.type == OMA_TLV_TYPE_RESOURCE) {
					path->res_id = tlv2.id;
					path->level = 3;
					/* ignore errors */
					do_write_op_tlv_item(
							context,
							(u8_t *)&tlv.value[pos],
							len2);
				}

				pos += len2;
			}

			zoap_header_set_code(context->out->out_zpkt,
					     ZOAP_RESPONSE_CODE_CREATED);
		} else if (tlv.type == OMA_TLV_TYPE_RESOURCE) {
			path->res_id = tlv.id;
			path->level = 3;
			ret = do_write_op_tlv_item(context,
						   &inbuf[tlvpos], len);
			if (ret < 0) {
				return ret;
			}
		}

		tlvpos += len;
	}

	return 0;
}
