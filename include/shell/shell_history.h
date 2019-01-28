/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_HISTORY_H__
#define SHELL_HISTORY_H__

#include <zephyr.h>
#include <misc/util.h>
#include <misc/dlist.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


struct shell_history {
	struct k_mem_slab *mem_slab;
	sys_dlist_t list;
	sys_dnode_t *current;
};

struct shell_history_item {
	sys_dnode_t dnode;
	u16_t len;
	char data[];
};

#if CONFIG_SHELL_HISTORY
#define SHELL_HISTORY_DEFINE(_name, block_size, block_count)	\
								\
	K_MEM_SLAB_DEFINE(_name##_history_memslab,		\
		 ROUND_UP(block_size + sizeof(struct shell_history_item), \
			  sizeof(void *)), block_count, 4);		\
	static struct shell_history _name##_history = {		\
		.mem_slab = &_name##_history_memslab		\
	}
#define SHELL_HISTORY_PTR(_name) (&_name##_history)
#else /* CONFIG_SHELL_HISTORY */
#define SHELL_HISTORY_DEFINE(_name, block_size, block_count) /*empty*/
#define SHELL_HISTORY_PTR(_name) NULL
#endif


void shell_history_init(struct shell_history *history);

void shell_history_purge(struct shell_history *history);

void shell_history_mode_exit(struct shell_history *history);

/* returns true if remains in history mode.*/
bool shell_history_get(struct shell_history *history, bool up,
		       u8_t *dst, u16_t *len);

void shell_history_put(struct shell_history *history, u8_t *line, size_t len);

static inline bool shell_history_active(struct shell_history *history)
{
	return (history->current) ? true : false;
}

#ifdef __cplusplus
}
#endif

#endif /* SHELL_HISTORY_H__ */
