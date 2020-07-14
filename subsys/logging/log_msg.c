/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <logging/log.h>
#include <logging/log_msg.h>
#include <logging/log_ctrl.h>
#include <logging/log_core.h>
#include <string.h>
#include <assert.h>

BUILD_ASSERT((sizeof(struct log_msg_ids) == sizeof(uint16_t)),
	     "Structure must fit in 2 bytes");

BUILD_ASSERT((sizeof(struct log_msg_generic_hdr) == sizeof(uint16_t)),
	     "Structure must fit in 2 bytes");

BUILD_ASSERT((sizeof(struct log_msg_std_hdr) == sizeof(uint16_t)),
	     "Structure must fit in 2 bytes");

BUILD_ASSERT((sizeof(struct log_msg_hexdump_hdr) == sizeof(uint16_t)),
	     "Structure must fit in 2 bytes");

BUILD_ASSERT((sizeof(union log_msg_head_data) ==
	      sizeof(struct log_msg_ext_head_data)),
	     "Structure must be same size");

#ifndef CONFIG_LOG_BUFFER_SIZE
#define CONFIG_LOG_BUFFER_SIZE 0
#endif

/* Define needed when CONFIG_LOG_BLOCK_IN_THREAD is disabled to satisfy
 * compiler. */
#ifndef CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS
#define CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS 0
#endif

#define MSG_SIZE sizeof(union log_msg_chunk)
#define NUM_OF_MSGS (CONFIG_LOG_BUFFER_SIZE / MSG_SIZE)

struct k_mem_slab log_msg_pool;
static uint8_t __noinit __aligned(sizeof(void *))
		log_msg_pool_buf[CONFIG_LOG_BUFFER_SIZE];

void log_msg_pool_init(void)
{
	k_mem_slab_init(&log_msg_pool, log_msg_pool_buf, MSG_SIZE, NUM_OF_MSGS);
}

/* Return true if interrupts were unlocked in the context of this call. */
static bool is_irq_unlocked(void)
{
	unsigned int key = arch_irq_lock();
	bool ret = arch_irq_unlocked(key);

	arch_irq_unlock(key);
	return ret;
}

/* Check if context can be blocked and pend on available memory slab. Context
 * can be blocked if in a thread and interrupts are not locked.
 */
static bool block_on_alloc(void)
{
	if (!IS_ENABLED(CONFIG_LOG_BLOCK_IN_THREAD)) {
		return false;
	}

	return (!k_is_in_isr() && is_irq_unlocked());
}

union log_msg_chunk *log_msg_chunk_alloc(void)
{
	union log_msg_chunk *msg = NULL;
	int err = k_mem_slab_alloc(&log_msg_pool, (void **)&msg,
		   block_on_alloc()
		   ? K_MSEC(CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS)
		   : K_NO_WAIT);

	if (err != 0) {
		msg = log_msg_no_space_handle();
	}

	return msg;
}

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
	uint32_t nargs = log_msg_nargs_get(msg);

	/* Free any transient string found in arguments. */
	if (log_msg_is_std(msg) && nargs) {
		uint32_t i;
		uint32_t smask = 0;

		for (i = 0; i < nargs; i++) {
			void *buf = (void *)log_msg_arg_get(msg, i);

			if (log_is_strdup(buf)) {
				if (smask == 0) {
					/* Do string arguments scan only when
					 * string duplication candidate detected
					 * since it is time consuming and free
					 * can be called from any context when
					 * log message is being dropped.
					 */
					smask = z_log_get_s_mask(
							log_msg_str_get(msg),
							nargs);
					if (smask == 0) {
						/* if no string argument is
						 * detected then stop searching
						 * for candidates.
						 */
						break;
					}
				}
				if (smask & BIT(i)) {
					log_free(buf);
				}
			}
		}
	} else if (IS_ENABLED(CONFIG_USERSPACE) &&
		   (log_msg_level_get(msg) != LOG_LEVEL_INTERNAL_RAW_STRING)) {
		/*
		 * When userspace support is enabled, the hex message metadata
		 * might be located in log_strdup() memory pool.
		 */
		const char *str = log_msg_str_get(msg);

		if (log_is_strdup(str)) {
			log_free((void *)(str));
		}
	}

	if (msg->hdr.params.generic.ext == 1) {
		cont_free(msg->payload.ext.next);
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
			log_dropped();
			err = k_mem_slab_alloc(&log_msg_pool,
					       (void **)&msg,
					       K_NO_WAIT);
		} while ((err != 0) && more);
	} else {
		log_dropped();
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

uint32_t log_msg_nargs_get(struct log_msg *msg)
{
	return msg->hdr.params.std.nargs;
}

static log_arg_t cont_arg_get(struct log_msg *msg, uint32_t arg_idx)
{
	struct log_msg_cont *cont;

	if (arg_idx < LOG_MSG_NARGS_HEAD_CHUNK) {
		return msg->payload.ext.data.args[arg_idx];
	}


	cont = msg->payload.ext.next;
	arg_idx -= LOG_MSG_NARGS_HEAD_CHUNK;

	while (arg_idx >= ARGS_CONT_MSG) {
		arg_idx -= ARGS_CONT_MSG;
		cont = cont->next;
	}

	return cont->payload.args[arg_idx];
}

log_arg_t log_msg_arg_get(struct log_msg *msg, uint32_t arg_idx)
{
	log_arg_t arg;

	/* Return early if requested argument not present in the message. */
	if (arg_idx >= msg->hdr.params.std.nargs) {
		return 0;
	}

	if (msg->hdr.params.std.nargs <= LOG_MSG_NARGS_SINGLE_CHUNK) {
		arg = msg->payload.single.args[arg_idx];
	} else {
		arg = cont_arg_get(msg, arg_idx);
	}

	return arg;
}

const char *log_msg_str_get(struct log_msg *msg)
{
	return msg->str;
}

/** @brief Allocate chunk for extended standard log message.
 *
 *  @details Extended standard log message is used when number of arguments
 *           exceeds capacity of one chunk. Extended message consists of two
 *           chunks. Such approach is taken to optimize memory usage and
 *           performance assuming that log messages with more arguments
 *           (@ref LOG_MSG_NARGS_SINGLE_CHUNK) are less common.
 *
 *  @return Allocated chunk of NULL.
 */
static struct log_msg *msg_alloc(uint32_t nargs)
{
	struct log_msg_cont *cont;
	struct log_msg_cont **next;
	struct  log_msg *msg = z_log_msg_std_alloc();
	int n = (int)nargs;

	if ((msg == NULL) || nargs <= LOG_MSG_NARGS_SINGLE_CHUNK) {
		return msg;
	}

	msg->hdr.params.std.nargs = 0U;
	msg->hdr.params.generic.ext = 1;
	n -= LOG_MSG_NARGS_HEAD_CHUNK;
	next = &msg->payload.ext.next;
	*next = NULL;

	while (n > 0) {
		cont = (struct log_msg_cont *)log_msg_chunk_alloc();

		if (cont == NULL) {
			msg_free(msg);
			return NULL;
		}

		*next = cont;
		cont->next = NULL;
		next = &cont->next;
		n -= ARGS_CONT_MSG;
	}

	return msg;
}

static void copy_args_to_msg(struct  log_msg *msg, log_arg_t *args, uint32_t nargs)
{
	struct log_msg_cont *cont = msg->payload.ext.next;

	if (nargs > LOG_MSG_NARGS_SINGLE_CHUNK) {
		(void)memcpy(msg->payload.ext.data.args, args,
		       LOG_MSG_NARGS_HEAD_CHUNK * sizeof(log_arg_t));
		nargs -= LOG_MSG_NARGS_HEAD_CHUNK;
		args += LOG_MSG_NARGS_HEAD_CHUNK;
	} else {
		(void)memcpy(msg->payload.single.args, args,
			     nargs * sizeof(log_arg_t));
		nargs  = 0U;
	}

	while (nargs != 0U) {
		uint32_t cpy_args = MIN(nargs, ARGS_CONT_MSG);

		(void)memcpy(cont->payload.args, args,
			     cpy_args * sizeof(log_arg_t));
		nargs -= cpy_args;
		args += cpy_args;
		cont = cont->next;
	}
}

struct log_msg *log_msg_create_n(const char *str, log_arg_t *args, uint32_t nargs)
{
	__ASSERT_NO_MSG(nargs < LOG_MAX_NARGS);

	struct  log_msg *msg = NULL;

	msg = msg_alloc(nargs);

	if (msg != NULL) {
		msg->str = str;
		msg->hdr.params.std.nargs = nargs;
		copy_args_to_msg(msg, args, nargs);
	}

	return msg;
}

struct log_msg *log_msg_hexdump_create(const char *str,
				       const uint8_t *data,
				       uint32_t length)
{
	struct log_msg_cont **prev_cont;
	struct log_msg_cont *cont;
	struct log_msg *msg;
	uint32_t chunk_length;

	/* Saturate length. */
	length = (length > LOG_MSG_HEXDUMP_MAX_LENGTH) ?
		 LOG_MSG_HEXDUMP_MAX_LENGTH : length;

	msg = (struct log_msg *)log_msg_chunk_alloc();
	if (msg == NULL) {
		return NULL;
	}

	/* all fields reset to 0, reference counter to 1 */
	msg->hdr.ref_cnt = 1;
	msg->hdr.params.hexdump.type = LOG_MSG_TYPE_HEXDUMP;
	msg->hdr.params.hexdump.length = length;
	msg->str = str;


	if (length > LOG_MSG_HEXDUMP_BYTES_SINGLE_CHUNK) {
		(void)memcpy(msg->payload.ext.data.bytes,
		       data,
		       LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK);
		msg->payload.ext.next = NULL;
		msg->hdr.params.generic.ext = 1;

		data += LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK;
		length -= LOG_MSG_HEXDUMP_BYTES_HEAD_CHUNK;
	} else {
		(void)memcpy(msg->payload.single.bytes, data, length);
		msg->hdr.params.generic.ext = 0;
		length = 0U;
	}

	prev_cont = &msg->payload.ext.next;

	while (length > 0) {
		cont = (struct log_msg_cont *)log_msg_chunk_alloc();
		if (cont == NULL) {
			msg_free(msg);
			return NULL;
		}

		*prev_cont = cont;
		cont->next = NULL;
		prev_cont = &cont->next;

		chunk_length = (length > HEXDUMP_BYTES_CONT_MSG) ?
			       HEXDUMP_BYTES_CONT_MSG : length;

		(void)memcpy(cont->payload.bytes, data, chunk_length);
		data += chunk_length;
		length -= chunk_length;
	}

	return msg;
}

static void log_msg_hexdump_data_op(struct log_msg *msg,
				    uint8_t *data,
				    size_t *length,
				    size_t offset,
				    bool put_op)
{
	uint32_t available_len = msg->hdr.params.hexdump.length;
	struct log_msg_cont *cont = NULL;
	uint8_t *head_data;
	uint32_t chunk_len;
	uint32_t req_len;
	uint32_t cpy_len;

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
		head_data = msg->payload.ext.data.bytes;
		cont = msg->payload.ext.next;
	} else {
		head_data = msg->payload.single.bytes;
		chunk_len = available_len;

	}

	if (offset < chunk_len) {
		cpy_len = req_len > chunk_len ? chunk_len : req_len;

		if (put_op) {
			(void)memcpy(&head_data[offset], data, cpy_len);
		} else {
			(void)memcpy(data, &head_data[offset], cpy_len);
		}

		req_len -= cpy_len;
		data += cpy_len;
	} else {
		offset -= chunk_len;
		chunk_len = HEXDUMP_BYTES_CONT_MSG;
		if (cont == NULL) {
			cont = msg->payload.ext.next;
		}

		while (offset >= chunk_len) {
			cont = cont->next;
			offset -= chunk_len;
		}
	}

	while (req_len > 0) {
		chunk_len = HEXDUMP_BYTES_CONT_MSG - offset;
		cpy_len = req_len > chunk_len ? chunk_len : req_len;

		if (put_op) {
			(void)memcpy(&cont->payload.bytes[offset],
				     data, cpy_len);
		} else {
			(void)memcpy(data, &cont->payload.bytes[offset],
				     cpy_len);
		}

		offset = 0;
		cont = cont->next;
		req_len -= cpy_len;
		data += cpy_len;
	}
}

void log_msg_hexdump_data_put(struct log_msg *msg,
			      uint8_t *data,
			      size_t *length,
			      size_t offset)
{
	log_msg_hexdump_data_op(msg, data, length, offset, true);
}

void log_msg_hexdump_data_get(struct log_msg *msg,
			      uint8_t *data,
			      size_t *length,
			      size_t offset)
{
	log_msg_hexdump_data_op(msg, data, length, offset, false);
}
