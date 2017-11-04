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

enum {
	OMA_TLV_TYPE_OBJECT_INSTANCE   = 0,
	OMA_TLV_TYPE_RESOURCE_INSTANCE = 1,
	OMA_TLV_TYPE_MULTI_RESOURCE    = 2,
	OMA_TLV_TYPE_RESOURCE          = 3
};

struct oma_tlv {
	u8_t  type;
	u16_t id; /* can be 8-bit or 16-bit when serialized */
	u32_t length;
};

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
		      u32_t buflen)
{
	if (tlv) {
		tlv->type = type;
		tlv->id = id;
		tlv->length = buflen;
	}
}

static int oma_tlv_put_u8(struct lwm2m_output_context *out,
			  u8_t value, bool insert)
{
	if (insert) {
		if (!net_pkt_insert(out->out_cpkt->pkt, out->frag,
				    out->offset, 1, &value,
				    BUF_ALLOC_TIMEOUT)) {
			return -ENOMEM;
		}

		out->offset++;
	} else {
		out->frag = net_pkt_write(out->out_cpkt->pkt, out->frag,
				  out->offset, &out->offset, 1, &value,
				  BUF_ALLOC_TIMEOUT);
		if (!out->frag && out->offset == 0xffff) {
			return -ENOMEM;
		}
	}

	return 0;
}

static size_t oma_tlv_put(const struct oma_tlv *tlv,
			  struct lwm2m_output_context *out,
			  u8_t *value, bool insert)
{
	size_t pos;
	int ret, i;
	u8_t len_type, tmp;

	/* len_type is the same as number of bytes required for length */
	len_type = get_len_type(tlv);

	/* first type byte in TLV header */
	tmp = (tlv->type << 6) |
	      (tlv->id > 255 ? (1 << 5) : 0) |
	      (len_type << 3) |
	      (len_type == 0 ? tlv->length : 0);

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
	if (value != NULL && tlv->length > 0) {
		if (insert) {
			if (!net_pkt_insert(out->out_cpkt->pkt, out->frag,
					    out->offset, tlv->length, value,
					    BUF_ALLOC_TIMEOUT)) {
				/* TODO: Generate error? */
				return 0;
			}

			out->offset += tlv->length;
		} else {
			out->frag = net_pkt_write(out->out_cpkt->pkt, out->frag,
						  out->offset, &out->offset,
						  tlv->length, value,
						  BUF_ALLOC_TIMEOUT);
			if (!out->frag && out->offset == 0xffff) {
				/* TODO: Generate error? */
				return 0;
			}
		}
	}

	return pos + tlv->length;
}

static size_t oma_tlv_get(struct oma_tlv *tlv,
			  struct lwm2m_input_context *in,
			  bool dont_advance)
{
	struct net_buf *tmp_frag;
	u8_t len_type;
	u8_t len_pos = 1;
	size_t tlv_len;
	u16_t tmp_offset;
	u8_t buf[2];

	tmp_frag = in->frag;
	tmp_offset = in->offset;
	tmp_frag = net_frag_read_u8(tmp_frag, tmp_offset, &tmp_offset, &buf[0]);

	if (!tmp_frag && tmp_offset == 0xffff) {
		goto error;
	}

	tlv->type = (buf[0] >> 6) & 3;
	len_type = (buf[0] >> 3) & 3;
	len_pos = 1 + (((buf[0] & (1 << 5)) != 0) ? 2 : 1);

	tmp_frag = net_frag_read_u8(tmp_frag, tmp_offset, &tmp_offset, &buf[1]);
	if (!tmp_frag && tmp_offset == 0xffff) {
		return 0;
	}

	tlv->id = buf[1];

	/* if len_pos > 2 it means that there are more ID to read */
	if (len_pos > 2) {
		tmp_frag = net_frag_read_u8(tmp_frag, tmp_offset,
					    &tmp_offset, &buf[1]);
		if (!tmp_frag && tmp_offset == 0xffff) {
			goto error;
		}

		tlv->id = (tlv->id << 8) + buf[1];
	}

	if (len_type == 0) {
		tlv_len = buf[0] & 7;
	} else {
		/* read the length */
		tlv_len = 0;
		while (len_type > 0) {
			tmp_frag = net_frag_read_u8(tmp_frag, tmp_offset,
						    &tmp_offset, &buf[1]);
			if (!tmp_frag && tmp_offset == 0xffff) {
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
		in->frag = tmp_frag;
		in->offset = tmp_offset;
	}

	return len_pos + tlv_len;

error:
	/* TODO: Generate error? */
	if (!dont_advance) {
		in->frag = tmp_frag;
		in->offset = tmp_offset;
	}

	return 0;
}

static size_t put_begin_ri(struct lwm2m_output_context *out,
			   struct lwm2m_obj_path *path)
{
	/* set some flags in state */
	out->writer_flags |= WRITER_RESOURCE_INSTANCE;

	/*
	 * store position for inserting TLV when we know the length
	 */
	out->mark_frag_ri = out->frag;
	out->mark_pos_ri = out->offset;
	return 0;
}

static size_t put_end_ri(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path)
{
	struct oma_tlv tlv;
	struct net_buf *tmp_frag;
	u16_t tmp_pos;
	u32_t len = 0;

	out->writer_flags &= ~WRITER_RESOURCE_INSTANCE;

	/* loop through buffers and add lengths */
	tmp_frag = out->mark_frag_ri;
	tmp_pos = out->mark_pos_ri;
	while (tmp_frag != out->frag) {
		len += tmp_frag->len - tmp_pos;
		tmp_pos = 0;
	}

	len += out->offset - tmp_pos;

	/* backup out location */
	tmp_frag = out->frag;
	tmp_pos = out->offset;

	/* use stored location */
	out->frag = out->mark_frag_ri;
	out->offset = out->mark_pos_ri;

	/* update the length */
	tlv_setup(&tlv, OMA_TLV_TYPE_MULTI_RESOURCE, path->res_id,
		  len);
	len = oma_tlv_put(&tlv, out, NULL, true) - tlv.length;

	/* restore out location + newly inserted */
	out->frag = tmp_frag;
	out->offset = tmp_pos + len;

	return 0;
}

static size_t put_s8(struct lwm2m_output_context *out,
		     struct lwm2m_obj_path *path, s8_t value)
{
	size_t len;
	struct oma_tlv tlv;

	tlv_setup(&tlv, tlv_calc_type(out->writer_flags),
		  tlv_calc_id(out->writer_flags, path), sizeof(value));

	len = oma_tlv_put(&tlv, out, (u8_t *)&value, false);
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
		  tlv_calc_id(out->writer_flags, path), sizeof(net_value));

	len = oma_tlv_put(&tlv, out, (u8_t *)&net_value, false);
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
		  tlv_calc_id(out->writer_flags, path), sizeof(net_value));

	len = oma_tlv_put(&tlv, out, (u8_t *)&net_value, false);
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
		  tlv_calc_id(out->writer_flags, path), sizeof(net_value));

	len = oma_tlv_put(&tlv, out, (u8_t *)&net_value, false);
	return len;
}

static size_t put_string(struct lwm2m_output_context *out,
			 struct lwm2m_obj_path *path,
			 char *buf, size_t buflen)
{
	size_t len;
	struct oma_tlv tlv;

	tlv_setup(&tlv, tlv_calc_type(out->writer_flags),
		  path->res_id, (u32_t)buflen);
	len = oma_tlv_put(&tlv, out, (u8_t *)buf, false);
	return len;
}

/* use binary32 format */
static size_t put_float32fix(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path,
			     float32_value_t *value)
{
	size_t len;
	struct oma_tlv tlv;
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
		  tlv_calc_id(out->writer_flags, path), 4);
	len = oma_tlv_put(&tlv, out, b, false);
	return len;
}

static size_t put_float64fix(struct lwm2m_output_context *out,
			     struct lwm2m_obj_path *path,
			     float64_value_t *value)
{
	size_t len;
	s64_t binary64 = 0, net_binary64 = 0;
	struct oma_tlv tlv;

	/* TODO */

	net_binary64 = sys_cpu_to_be64(binary64);
	tlv_setup(&tlv, tlv_calc_type(out->writer_flags),
		  tlv_calc_id(out->writer_flags, path),
		  sizeof(net_binary64));
	len = oma_tlv_put(&tlv, out, (u8_t *)&net_binary64, false);
	return len;
}

static size_t put_bool(struct lwm2m_output_context *out,
		       struct lwm2m_obj_path *path, bool value)
{
	return put_s8(out, path, value != 0 ? 1 : 0);
}

static size_t get_number(struct lwm2m_input_context *in, s64_t *value,
			 u8_t max_len)
{
	struct oma_tlv tlv;
	size_t size = oma_tlv_get(&tlv, in, false);
	s64_t temp;

	*value = 0;
	if (size > 0) {
		if (tlv.length > max_len) {
			SYS_LOG_ERR("invalid length: %u", tlv.length);
			return 0;
		}

		in->frag = net_frag_read(in->frag, in->offset, &in->offset,
					 tlv.length, (u8_t *)&temp);
		if (!in->frag && in->offset == 0xffff) {
			/* TODO: Generate error? */
			return 0;
		}

		switch (tlv.length) {
		case 1:
			*value = (s8_t)temp;
			break;
		case 2:
			*value = sys_cpu_to_be16((s16_t)temp);
			break;
		case 4:
			*value = sys_cpu_to_be32((s32_t)temp);
			break;
		case 8:
			*value = sys_cpu_to_be64(temp);
			break;
		default:
			SYS_LOG_ERR("invalid length: %u", tlv.length);
			return 0;
		}
	}

	return size;
}

static size_t get_s64(struct lwm2m_input_context *in, s64_t *value)
{
	return get_number(in, value, 8);
}

static size_t get_s32(struct lwm2m_input_context *in, s32_t *value)
{
	s64_t temp;
	size_t size;

	*value = 0;
	size = get_number(in, &temp, 4);
	if (size > 0) {
		*value = (s32_t)temp;
	}

	return size;
}

static size_t get_string(struct lwm2m_input_context *in,
			 u8_t *buf, size_t buflen)
{
	struct oma_tlv tlv;
	size_t size = oma_tlv_get(&tlv, in, false);

	if (size > 0) {
		if (buflen <= tlv.length) {
			/* TODO: Generate error? */
			return 0;
		}

		in->frag = net_frag_read(in->frag, in->offset, &in->offset,
					 tlv.length, buf);
		if (!in->frag && in->offset == 0xffff) {
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
	int e;
	s32_t val;
	int sign = false;
	int bits = 0;
	u8_t values[4];

	if (size > 0) {
		/* TLV needs to be 4 bytes */
		in->frag = net_frag_read(in->frag, in->offset, &in->offset,
					 4, values);
		if (!in->frag && in->offset == 0xffff) {
			/* TODO: Generate error? */
			return 0;
		}

		/* sign */
		sign = (values[0] & 0x80) != 0;

		e = ((values[0] << 1) & 0xff) | (values[1] >> 7);
		val = (((long)values[1] & 0x7f) << 16) |
		      (values[2] << 8) |
		      values[3];
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
	}

	return size;
}

static size_t get_float64fix(struct lwm2m_input_context *in,
			     float64_value_t *value)
{
	struct oma_tlv tlv;
	size_t size = oma_tlv_get(&tlv, in, false);

	if (size > 0) {
		if (tlv.length != 8) {
			SYS_LOG_ERR("invalid length: %u (not 8)", tlv.length);
			return 0;
		}

		/* TODO */
	}

	return size;
}

static size_t get_bool(struct lwm2m_input_context *in, bool *value)
{
	s64_t temp;
	size_t size;

	*value = 0;
	size = get_number(in, &temp, 2);
	if (size > 0) {
		*value = (temp != 0);
	}

	return size;
}

static size_t get_opaque(struct lwm2m_input_context *in,
			 u8_t *value, size_t buflen, bool *last_block)
{
	struct oma_tlv tlv;

	oma_tlv_get(&tlv, in, false);
	in->opaque_len = tlv.length;
	return lwm2m_engine_get_opaque_more(in, value, buflen, last_block);
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
	put_bool,
	NULL
};

const struct lwm2m_reader oma_tlv_reader = {
	get_s32,
	get_s64,
	get_string,
	get_float32fix,
	get_float64fix,
	get_bool,
	get_opaque
};

static int do_write_op_tlv_item(struct lwm2m_engine_context *context)
{
	struct lwm2m_engine_obj_inst *obj_inst = NULL;
	struct lwm2m_engine_res_inst *res = NULL;
	struct lwm2m_engine_obj_field *obj_field = NULL;
	u8_t created = 0;
	int ret, i;

	ret = lwm2m_get_or_create_engine_obj(context, &obj_inst, &created);
	if (ret < 0) {
		return ret;
	}

	obj_field = lwm2m_get_engine_obj_field(obj_inst->obj,
					       context->path->res_id);
	/* if obj_field is not found, treat as an optional resource */
	if (!obj_field) {
		if (context->operation == LWM2M_OP_CREATE) {
			return -ENOTSUP;
		}

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
	int ret;

	while (true) {
		/*
		 * This initial read of TLV data won't advance frag/offset.
		 * We need tlv.type to determine how to proceed.
		 */
		len = oma_tlv_get(&tlv, in, true);
		if (len == 0) {
			break;
		}

		if (tlv.type == OMA_TLV_TYPE_OBJECT_INSTANCE) {
			struct oma_tlv tlv2;
			int len2;
			int pos = 0;

			oma_tlv_get(&tlv, in, false);
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
			       (len2 = oma_tlv_get(&tlv2, in, true))) {
				if (tlv2.type == OMA_TLV_TYPE_RESOURCE) {
					path->res_id = tlv2.id;
					path->level = 3;
					ret = do_write_op_tlv_item(context);
					/*
					 * ignore errors for CREATE op
					 * TODO: support BOOTSTRAP WRITE where
					 * optional resources are ignored
					 */
					if (ret < 0 &&
					    (context->operation !=
					     LWM2M_OP_CREATE ||
					     ret != -ENOTSUP)) {
						return ret;
					}
				}

				pos += len2;
			}
		} else if (tlv.type == OMA_TLV_TYPE_RESOURCE) {
			path->res_id = tlv.id;
			path->level = 3;
			ret = do_write_op_tlv_item(context);
			/*
			 * ignore errors for CREATE op
			 * TODO: support BOOTSTRAP WRITE where optional
			 * resources are ignored
			 */
			if (ret < 0 && (context->operation != LWM2M_OP_CREATE ||
					ret != -ENOTSUP)) {
				return ret;
			}
		}
	}

	return 0;
}
