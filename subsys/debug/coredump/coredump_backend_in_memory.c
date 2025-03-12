/*
 * Copyright (c) 2025 Bang & Olufsen.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/debug/coredump.h>

#include "coredump_internal.h"

LOG_MODULE_REGISTER(coredump, CONFIG_DEBUG_COREDUMP_LOG_LEVEL);

#define IN_MEMORY_CANARY_SIZE 4
#define IN_MEMORY_COREDUMP_SIZE_RECORD sizeof(size_t)

/**
 * In-memory coredump space is arranged that way:
 * CANARY+ Recorded coredump size + Coredump + Left space (if any) + CANARY
 */
#define IN_MEMORY_SPACE CONFIG_DEBUG_COREDUMP_BACKEND_IN_MEMORY_SIZE + \
	(IN_MEMORY_CANARY_SIZE * 2) + IN_MEMORY_COREDUMP_SIZE_RECORD
#define IN_MEMORY_START IN_MEMORY_CANARY_SIZE + IN_MEMORY_COREDUMP_SIZE_RECORD
#define IN_MEMORY_END IN_MEMORY_SPACE - IN_MEMORY_CANARY_SIZE

static const uint8_t in_memory_canary[IN_MEMORY_CANARY_SIZE] = {
	0xDE, 0xB0, 0xDE, 0xB0
};

static uint8_t __noinit_named(_in_memory_coredump)
	__aligned(4) in_memory_coredump[IN_MEMORY_SPACE];

static size_t *coredump_size =
	(size_t *)&in_memory_coredump[IN_MEMORY_CANARY_SIZE];
static uint8_t *cur_ptr;


static inline void in_memory_invalidate(void)
{
	memset(in_memory_coredump, 0, IN_MEMORY_CANARY_SIZE);
	memset(&in_memory_coredump[IN_MEMORY_END], 0, IN_MEMORY_CANARY_SIZE);
	*coredump_size = 0;
	cur_ptr = NULL;
}
static inline void in_memory_erase(void)
{
	LOG_DBG("Erasing in-memory coredump\n");

	in_memory_invalidate();
}

static int in_memory_is_valid(void)
{
	if (!memcmp(in_memory_coredump,
		    in_memory_canary, IN_MEMORY_CANARY_SIZE) &&
	    !memcmp(&in_memory_coredump[IN_MEMORY_END],
		    in_memory_canary, IN_MEMORY_CANARY_SIZE)) {
		return 1;
	}

	return 0;
}

static int in_memory_copy_to(struct coredump_cmd_copy_arg *copy_arg)
{
	LOG_DBG("Copy to: %p offset: %lu length: %lu",
		(void *)copy_arg->buffer, copy_arg->offset,
		(unsigned long)copy_arg->length);

	if (copy_arg->buffer == NULL ||
	    copy_arg->offset >= IN_MEMORY_END ||
	    (copy_arg->length + copy_arg->offset) >= IN_MEMORY_END) {
		return -EINVAL;
	}

	if (in_memory_is_valid() == 0) {
		return -EIO;
	}

	memcpy(copy_arg->buffer,
	       &in_memory_coredump[IN_MEMORY_START + copy_arg->offset],
	       copy_arg->length);

	return 0;
}

static void coredump_in_memory_backend_start(void)
{
	in_memory_erase();

	LOG_ERR(COREDUMP_PREFIX_STR "LOCATION %p", (void *)in_memory_coredump);

	memcpy(in_memory_coredump, in_memory_canary, IN_MEMORY_CANARY_SIZE);
	cur_ptr = &in_memory_coredump[IN_MEMORY_START];

	*coredump_size = 0;

	while (LOG_PROCESS()) {
		;
	}

	LOG_PANIC();
	LOG_ERR(COREDUMP_PREFIX_STR COREDUMP_BEGIN_STR);
}

static void coredump_in_memory_backend_end(void)
{
	memcpy(&in_memory_coredump[IN_MEMORY_END],
	       in_memory_canary, IN_MEMORY_CANARY_SIZE);

	*coredump_size = cur_ptr - &in_memory_coredump[IN_MEMORY_START];

	LOG_ERR(COREDUMP_PREFIX_STR COREDUMP_END_STR);
}

static void coredump_in_memory_backend_buffer_output(uint8_t *buf,
						     size_t buflen)
{
	int space;

	LOG_DBG("Output buffer size %lu", (unsigned long)buflen);

	if (cur_ptr == &in_memory_coredump[IN_MEMORY_END]) {
		/* Once full, we silently ignore the request */
		return;
	}

	space = &in_memory_coredump[IN_MEMORY_END] - cur_ptr;
	if (buflen < space) {
		space = buflen;
	}

	memcpy(cur_ptr, buf, space);
	cur_ptr += space;
}

static int coredump_in_memory_backend_query(enum coredump_query_id query_id,
					    void *arg)
{
	switch (query_id) {
	case COREDUMP_QUERY_GET_ERROR:
		break;
	case COREDUMP_QUERY_HAS_STORED_DUMP:
		return in_memory_is_valid();
	case COREDUMP_QUERY_GET_STORED_DUMP_SIZE:
		if (in_memory_is_valid() == 1) {
			return *coredump_size;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int coredump_in_memory_backend_cmd(enum coredump_cmd_id cmd_id,
					  void *arg)
{
	switch (cmd_id) {
	case COREDUMP_CMD_CLEAR_ERROR:
		break;
	case COREDUMP_CMD_VERIFY_STORED_DUMP:
		return in_memory_is_valid();
	case COREDUMP_CMD_ERASE_STORED_DUMP:
		in_memory_erase();
		break;
	case COREDUMP_CMD_COPY_STORED_DUMP:
		if (arg == NULL) {
			return -EINVAL;
		}
		return in_memory_copy_to((struct coredump_cmd_copy_arg *)arg);
	case COREDUMP_CMD_INVALIDATE_STORED_DUMP:
		in_memory_invalidate();
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

struct coredump_backend_api coredump_backend_in_memory = {
	.start = coredump_in_memory_backend_start,
	.end = coredump_in_memory_backend_end,
	.buffer_output = coredump_in_memory_backend_buffer_output,
	.query = coredump_in_memory_backend_query,
	.cmd = coredump_in_memory_backend_cmd,
};
