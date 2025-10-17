/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SHELL_HISTORY_H_
#define ZEPHYR_INCLUDE_SHELL_HISTORY_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/dlist.h>
#include <zephyr/sys/ring_buffer.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


struct shell_history {
	struct k_heap *heap;
	sys_dlist_t list;
	sys_dnode_t *current;
};

/**
 * @brief Create shell history instance.
 *
 * @param _name History instance name.
 * @param _size Memory size dedicated for shell history.
 */
#define Z_SHELL_HISTORY_DEFINE(_name, _size)			\
	K_HEAP_DEFINE(_name##_heap, _size);			\
	static struct shell_history _name = {			\
		.heap = &_name##_heap,				\
		.list = SYS_DLIST_STATIC_INIT(&_name.list),	\
	}

/**
 * @brief Purge shell history.
 *
 * Function clears whole shell command history.
 *
 * @param history Shell history instance.
 *
 */
void z_shell_history_purge(struct shell_history *history);

/**
 * @brief Exit history browsing mode.
 *
 * @param history Shell history instance.
 */
void z_shell_history_mode_exit(struct shell_history *history);

/**
 * @brief Get next entry in shell command history.
 *
 * Function returns next (in given direction) stored line.
 *
 * @param[in]     history	Shell history instance.
 * @param[in]     up		Direction.
 * @param[out]    dst		Buffer where line is copied.
 * @param[in,out] len		Buffer size (input), amount of copied
 *				data (output).
 * @return True if remains in history mode.
 */
bool z_shell_history_get(struct shell_history *history, bool up,
			 uint8_t *dst, uint16_t *len);

/**
 * @brief Put line into shell command history.
 *
 * If history is full, oldest entry (or entries) is removed.
 *
 * @param history	Shell history instance.
 * @param line		Data.
 * @param len		Data length.
 *
 */
void z_shell_history_put(struct shell_history *history, uint8_t *line,
			 size_t len);

/**
 * @brief Get state of shell history.
 *
 * @param history	Shell history instance.
 *
 * @return True if in browsing mode.
 */
static inline bool z_shell_history_active(struct shell_history *history)
{
	return (history->current) ? true : false;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SHELL_HISTORY_H_ */
