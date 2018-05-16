/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <logging/log_msg.h>
#include <logging/log_ctrl.h>
#include <string.h>

#define MSG_SIZE sizeof(union log_msg_chunk)
#define NUM_OF_MSGS (CONFIG_LOG_BUFFER_SIZE / MSG_SIZE)

K_MEM_SLAB_DEFINE(log_msg_pool, MSG_SIZE, NUM_OF_MSGS, sizeof(u32_t));

void log_msg_get(struct log_msg *msg)
{
	atomic_inc(&msg->hdr.ref_cnt);
}

static void cont_free(struct log_msg_cont *cont)
{
	struct log_msg_cont *next;

	while (cont != NULL) {
		next = cont->next;
		k_mem_slab_free(&log_msg_pool, (void **)&cont);
		cont = next;
	}
}

static void msg_free(struct log_msg *msg)
{
	if (msg->hdr.params.generic.ext == 1) {
		cont_free(msg->data.ext.next);
	}
	k_mem_slab_free(&log_msg_pool, (void **)&msg);
}

union log_msg_chunk *log_msg_no_space_handle(void)
{
	union log_msg_chunk *msg = NULL;
	bool more;
	int err;

	if (IS_ENABLED(CONFIG_LOG_MODE_OVERFLOW)) {
		do {
			more = log_process(true);
			err = k_mem_slab_alloc(&log_msg_pool,
					       (void **)&msg,
					       K_NO_WAIT);
		} while ((err != 0) && more);
	}
	return msg;

}
void log_msg_put(struct log_msg *msg)
{
	atomic_dec(&msg->hdr.ref_cnt);

	if (msg->hdr.ref_cnt == 0) {
		msg_free(msg);
	}
}

u32_t log_msg_nargs_get(struct log_msg *msg)
{
	return msg->hdr.params.std.nargs;
}

u32_t log_msg_arg_get(struct log_msg *msg, u32_t arg_idx)
{
	u32_t arg;

	if (msg->hdr.params.std.nargs <= LOG_MSG_NARGS_SINGLE_CHUNK) {
		arg = msg->data.data.std.args[arg_idx];
	} else {
		if (arg_idx < LOG_MSG_NARGS_HEAD_CHUNK) {
			arg = msg->data.ext.data.std.args[arg_idx];
		} else {
			arg_idx -= LOG_MSG_NARGS_HEAD_CHUNK;
			arg = msg->data.ext.next->data.args[arg_idx];
		}
	}

	return arg;
}

const char *log_msg_str_get(struct log_msg *msg)
{
	if (log_msg_nargs_get(msg) <= LOG_MSG_NARGS_SINGLE_CHUNK) {
		return msg->data.data.std.str;
	} else {
		return msg->data.ext.data.std.str;
	}
}

struct log_msg *log_msg_create_n(const char *str,
					       u32_t *args,
					       u32_t nargs)
{
	struct  log_msg *msg;

	if (nargs <= LOG_MSG_NARGS_SINGLE_CHUNK) {
		msg = _log_msg_std_alloc();

		if (msg != NULL) {
			msg->hdr.params.std.nargs = nargs;
			msg->data.data.std.str = str;
			memcpy(msg->data.data.std.args, args, nargs);
		}
	} else {
		msg = _log_msg_ext_std_alloc();

		if (msg != NULL) {
			msg->hdr.params.std.nargs = nargs;
			msg->data.ext.data.std.str = str;
			/* Direct assignment will be faster than memcpy. */
			msg->data.ext.data.std.args[0] = args[0];
			msg->data.ext.data.std.args[1] = args[1];
			memcpy(msg->data.ext.next->data.args,
			      &args[LOG_MSG_NARGS_HEAD_CHUNK],
			      (nargs - LOG_MSG_NARGS_HEAD_CHUNK)*sizeof(u32_t));
		}
	}

	return msg;
}

struct log_msg *log_msg_hexdump_create(const u8_t *data, u32_t length)
{
	struct log_msg_cont **prev_cont;
	struct log_msg_cont *cont;
	struct log_msg *msg;
	u32_t chunk_length;

	/* Saturate length. */
	length = (length > LOG_MSG_HEXDUMP_MAX_LENGTH) ?
		 LOG_MSG_HEXDUMP_MAX_LENGTH : length;

	msg = (struct log_msg *)log_msg_chunk_alloc();
	if (!msg) {
		return NULL;
	}

	/* all fields reset to 0, reference counter to 1 */
	msg->hdr.ref_cnt = 1;
	msg->hdr.params.hexdump.type = LOG_MSG_TYPE_HEXDUMP;
	msg->hdr.params.hexdump.raw_string = 0;
	msg->hdr.params.hexdump.length = length;


	if (length > LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK) {
		memcpy(msg->data.ext.data.bytes,
		       data,
		       LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK);

		msg->hdr.params.generic.ext = 1;
		msg->data.ext.next = NULL;

		data += LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK;
		length -= LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK;
	} else {
		memcpy(msg->data.data.bytes, data, length);
		msg->hdr.params.generic.ext = 0;
		length = 0;
	}

	prev_cont = &msg->data.ext.next;

	while (length > 0) {
		cont = (struct log_msg_cont *)log_msg_chunk_alloc();
		if (!cont) {
			msg_free(msg);
			return NULL;
		}

		*prev_cont = cont;
		cont->next = NULL;
		prev_cont = &cont->next;

		chunk_length = (length > HEXDUMP_BYTES_CONT_MSG) ?
			       HEXDUMP_BYTES_CONT_MSG : length;

		memcpy(cont->data.bytes, data, chunk_length);
		data += chunk_length;
		length -= chunk_length;
	}

	return msg;
}

static void log_msg_hexdump_data_op(struct log_msg *msg,
				    u8_t *data,
				    size_t *length,
				    size_t offset,
				    bool put_op)
{
	u32_t available_len = msg->hdr.params.hexdump.length;
	struct log_msg_cont *cont = NULL;
	u8_t *head_data;
	u32_t chunk_len;
	u32_t req_len;
	u32_t cpy_len;

	if (offset >= available_len) {
		*length = 0;
		return;
	}

	if ((offset + *length) > available_len) {
		*length = available_len - offset;
	}

	req_len = *length;

	if (available_len > LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK) {
		chunk_len = LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK;
		head_data = msg->data.ext.data.bytes;
		cont = msg->data.ext.next;
	} else {
		head_data = msg->data.data.bytes;
		chunk_len = available_len;

	}

	if (offset < chunk_len) {
		cpy_len = req_len > chunk_len ? chunk_len : req_len;

		if (put_op) {
			memcpy(&head_data[offset], data, cpy_len);
		} else {
			memcpy(data, &head_data[offset], cpy_len);
		}

		req_len -= cpy_len;
		data += cpy_len;
	} else {
		offset -= chunk_len;
		chunk_len = HEXDUMP_BYTES_CONT_MSG;

		while (offset > chunk_len) {
			cont = cont->next;
			offset -= chunk_len;
		}
	}

	while (req_len > 0) {
		chunk_len = HEXDUMP_BYTES_CONT_MSG - offset;
		cpy_len = req_len > chunk_len ? chunk_len : req_len;

		if (put_op) {
			memcpy(&cont->data.bytes[offset], data, cpy_len);
		} else {
			memcpy(data, &cont->data.bytes[offset], cpy_len);
		}

		offset = 0;
		cont = cont->next;
		req_len -= cpy_len;
		data += cpy_len;
	}
}

void log_msg_hexdump_data_put(struct log_msg *msg,
			      u8_t *data,
			      size_t *length,
			      size_t offset)
{
	log_msg_hexdump_data_op(msg, data, length, offset, true);
}

void log_msg_hexdump_data_get(struct log_msg *msg,
			      u8_t *data,
			      size_t *length,
			      size_t offset)
{
	log_msg_hexdump_data_op(msg, data, length, offset, false);
}
