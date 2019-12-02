/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRACE_PACKET_H
#define _TRACE_PACKET_H

#include <sys/slist.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tracing packet
 * @defgroup Tracing_packet Tracing packet
 * @{
 * @}
 */

enum tracing_direction {
	TRACING_IN = 0,
	TRACING_OUT
};

/**
 * @brief Tracing packet structure.
 */
struct tracing_packet {
	sys_snode_t list_node;
	u32_t length;
	enum tracing_direction direction;
	u8_t buf[CONFIG_TRACING_PACKET_BUF_SIZE];
};

/**
 * @brief Initialize tracing packet pool.
 */
void tracing_packet_pool_init(void);

/**
 * @brief Allocate tracing_packet from tracing packet pool.
 *
 * @return Allocated tracing_packet.
 */
struct tracing_packet *tracing_packet_alloc(void);

/**
 * @brief Free tracing_packet.
 *
 * @param packet Request free tracing_packet.
 */
void tracing_packet_free(struct tracing_packet *packet);

#ifdef __cplusplus
}
#endif

#endif
