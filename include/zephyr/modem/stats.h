/*
 * Copyright (c) 2024 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <zephyr/kernel.h>

#ifndef ZEPHYR_MODEM_STATS_
#define ZEPHYR_MODEM_STATS_

/**
 * @cond INTERNAL_HIDDEN
 */

/** Modem statistics buffer structure */
struct modem_stats_buffer {
	sys_snode_t node;
	char name[CONFIG_MODEM_STATS_BUFFER_NAME_SIZE];
	uint32_t max_used;
	uint32_t size;
};

/**
 * @endcond
 */

/**
 * @brief Initialize modem statistics buffer
 *
 * @param buffer Modem statistics buffer instance
 * @param name Name of buffer instance
 * @param size Size of buffer
 */
void modem_stats_buffer_init(struct modem_stats_buffer *buffer,
			     const char *name, uint32_t size);

/**
 * @brief Advertise modem statistics buffer size
 *
 * @param buffer Modem statistics buffer instance
 * @param length Length of buffer
 *
 * @note Invoke when buffer size changes
 * @note Safe to invoke from ISR
 */
void modem_stats_buffer_advertise_length(struct modem_stats_buffer *buffer, uint32_t length);

#endif /* ZEPHYR_MODEM_STATS_ */
