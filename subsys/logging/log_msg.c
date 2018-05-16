/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <logging/log_msg.h>
#include <string.h>

#define MSG_SIZE sizeof(union log_msg_chunk)
#define NUM_OF_MSGS (CONFIG_LOG_BUFFER_SIZE / MSG_SIZE)

K_MEM_SLAB_DEFINE(log_msg_pool, MSG_SIZE, NUM_OF_MSGS, sizeof(u32_t));

void log_msg_get(struct log_msg *p_msg)
{
	atomic_inc(&p_msg->hdr.ref_cnt);
}

static void cont_free(struct log_msg_cont *p_cont)
{
	struct log_msg_cont *p_next;

	while (p_cont != NULL) {
		p_next = p_cont->p_next;
		k_mem_slab_free(&log_msg_pool, (void **)&p_cont);
		p_cont = p_next;
	}
}

static void msg_free(struct log_msg *p_msg)
{
	if (p_msg->hdr.params.generic.ext == 1) {
		cont_free(p_msg->data.ext.p_next);
	}
	k_mem_slab_free(&log_msg_pool, (void **)&p_msg);
}

void log_msg_put(struct log_msg *p_msg)
{
	atomic_dec(&p_msg->hdr.ref_cnt);

	if (p_msg->hdr.ref_cnt == 0) {
		msg_free(p_msg);
	}
}

u32_t log_msg_nargs_get(struct log_msg *p_msg)
{
	return p_msg->hdr.params.std.nargs;
}

u32_t log_msg_arg_get(struct log_msg *p_msg, u32_t arg_idx)
{
	if (p_msg->hdr.params.std.nargs <= LOG_MSG_NARGS_SINGLE_CHUNK) {
		return p_msg->data.data.std.args[arg_idx];
	} else   {
		if (arg_idx < (LOG_MSG_NARGS_SINGLE_CHUNK - 1)) {
			return p_msg->data.data.std.args[arg_idx];
		} else   {
			return p_msg->data.ext.p_next->data.args[arg_idx];
		}
	}
}

const char *log_msg_str_get(struct log_msg *p_msg)
{
	return p_msg->data.data.std.p_str;
}

struct log_msg *log_msg_hexdump_create(const u8_t *p_data, u32_t length)
{
	struct log_msg *p_msg;
	struct log_msg_cont *p_cont;
	struct log_msg_cont **pp_prev_cont;
	u32_t chunk_length;

	/* Saturate length. */
	length = (length > LOG_MSG_HEXDUMP_MAX_LENGTH) ?
		 LOG_MSG_HEXDUMP_MAX_LENGTH : length;

	int err = k_mem_slab_alloc(&log_msg_pool, (void **)&p_msg, K_NO_WAIT);
	if (err == -ENOMEM) {
		/* For now do not overwrite.*/
		return NULL;
	}

	/* all fields reset to 0, reference counter to 1 */
	p_msg->hdr.ref_cnt = 1;
	p_msg->hdr.params.hexdump.type = LOG_MSG_TYPE_HEXDUMP;
	p_msg->hdr.params.hexdump.raw_string = 0;
	p_msg->hdr.params.hexdump.length = length;


	if (length > LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK) {
		memcpy(p_msg->data.ext.data.bytes,
		       p_data,
		       LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK);

		p_msg->hdr.params.generic.ext = 1;
		p_msg->data.ext.p_next = NULL;

		p_data += LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK;
		length -= LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK;
	} else   {
		memcpy(p_msg->data.data.bytes, p_data, length);
		p_msg->hdr.params.generic.ext = 0;
		length = 0;
	}

	pp_prev_cont = &p_msg->data.ext.p_next;

	while (length > 0) {
		p_cont = NULL;
		err = k_mem_slab_alloc(&log_msg_pool,
				       (void **)&p_cont,
				       K_NO_WAIT);
		if (err == -ENOMEM) {
			/* For now do not overwrite.*/
			msg_free(p_msg);
			return NULL;
		}

		*pp_prev_cont = p_cont;
		p_cont->p_next = NULL;
		pp_prev_cont = &p_cont->p_next;

		chunk_length = (length > HEXDUMP_BYTES_CONT_MSG) ?
			       HEXDUMP_BYTES_CONT_MSG : length;

		memcpy(p_cont->data.bytes, p_data, chunk_length);
		p_data += chunk_length;
		length -= chunk_length;
	}

	return p_msg;
}

static void log_msg_hexdump_data_op(struct log_msg *p_msg,
				    u8_t *p_data,
				    u32_t *p_length,
				    u32_t offset,
				    bool put_op)
{
	u32_t req_len;
	u32_t available_len = p_msg->hdr.params.hexdump.length;
	u32_t chunk_len;
	u32_t cpy_len;
	struct log_msg_cont *p_cont = NULL;
	u8_t *p_head_data;

	if (offset >= available_len) {
		*p_length = 0;
		return;
	}

	if ((offset + *p_length) > available_len) {
		*p_length = available_len - offset;
	}

	req_len = *p_length;

	if (available_len > LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK) {
		chunk_len = LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK;
		p_head_data = p_msg->data.ext.data.bytes;
		p_cont = p_msg->data.ext.p_next;
	} else   {
		p_head_data = p_msg->data.data.bytes;
		chunk_len = available_len;

	}

	if (offset < chunk_len) {
		cpy_len = req_len > chunk_len ? chunk_len : req_len;

		if (put_op) {
			memcpy(&p_head_data[offset], p_data, cpy_len);
		} else   {
			memcpy(p_data, &p_head_data[offset], cpy_len);
		}

		req_len -= cpy_len;
		p_data += cpy_len;
	} else   {
		offset -= chunk_len;
		chunk_len = HEXDUMP_BYTES_CONT_MSG;

		while (offset > chunk_len) {
			p_cont = p_cont->p_next;
			offset -= chunk_len;
		}
	}

	while (req_len > 0) {
		chunk_len = HEXDUMP_BYTES_CONT_MSG - offset;
		cpy_len = req_len > chunk_len ? chunk_len : req_len;

		if (put_op) {
			memcpy(&p_cont->data.bytes[offset], p_data, cpy_len);
		} else   {
			memcpy(p_data, &p_cont->data.bytes[offset], cpy_len);
		}

		offset = 0;
		p_cont = p_cont->p_next;
		req_len -= cpy_len;
		p_data += cpy_len;
	}
}

void log_msg_hexdump_data_put(struct log_msg *p_msg,
			      u8_t *p_data,
			      u32_t *p_length,
			      u32_t offset)
{
	log_msg_hexdump_data_op(p_msg, p_data, p_length, offset, true);
}

void log_msg_hexdump_data_get(struct log_msg *p_msg,
			      u8_t *p_data,
			      u32_t *p_length,
			      u32_t offset)
{
	log_msg_hexdump_data_op(p_msg, p_data, p_length, offset, false);
}
