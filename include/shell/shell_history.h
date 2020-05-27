/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SHELL_HISTORY_H__
#define SHELL_HISTORY_H__

#include <zephyr.h>
#include <sys/util.h>
#include <sys/dlist.h>
#include <sys/ring_buffer.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif


struct shell_history {
	struct ring_buf *ring_buf;
	sys_dlist_t list;
	sys_dnode_t *current;
};

/**
 * @brief Create shell history instance.
 *
 * @param _name History instance name.
 * @param _size Memory dedicated for shell history.
 */
#define SHELL_HISTORY_DEFINE(_name, _size) \
	static uint8_t __noinit __aligned(sizeof(void *)) \
			_name##_ring_buf_data[_size]; \
	static struct ring_buf _name##_ring_buf = \
		{ \
			.size = _size, \
			.buf =  { .buf8 = _name##_ring_buf_data } \
		}; \
	static struct shell_history _name = { \
		.ring_buf = &_name##_ring_buf \
	}


/**
 * @brief Initialize shell history module.
 *
 * @param history Shell history instance.
 */
void shell_history_init(struct shell_history *history);

/**
 * @brief Purge shell history.
 *
 * Function clears whole shell command history.
 *
 * @param history Shell history instance.
 *
 */
void shell_history_purge(struct shell_history *history);

/**
 * @brief Exit history browsing mode.
 *
 * @param history Shell history instance.
 */
void shell_history_mode_exit(struct shell_history *history);

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
bool shell_history_get(struct shell_history *history, bool up,
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
void shell_history_put(struct shell_history *history, uint8_t *line, size_t len);

/**
 * @brief Get state of shell history.
 *
 * @param history	Shell history instance.
 *
 * @return True if in browsing mode.
 */
static inline bool shell_history_active(struct shell_history *history)
{
	return (history->current) ? true : false;
}

#ifdef __cplusplus
}
#endif

#endif /* SHELL_HISTORY_H__ */
