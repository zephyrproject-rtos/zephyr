/** @file
 * @brief SocketCAN utilities.
 *
 * Utilities for SocketCAN support.
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKETCAN_UTILS_H_
#define ZEPHYR_INCLUDE_NET_SOCKETCAN_UTILS_H_

#include <zephyr/drivers/can.h>
#include <zephyr/net/socketcan.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SocketCAN utilities
 * @defgroup socket_can Network Core Library
 * @ingroup networking
 * @{
 */

/**
 * @brief Translate a @a socketcan_frame struct to a @a can_frame struct.
 *
 * @param sframe Pointer to sockecan_frame struct.
 * @param zframe Pointer to can_frame struct.
 */
static inline void socketcan_to_can_frame(const struct socketcan_frame *sframe,
					  struct can_frame *zframe)
{
	zframe->id_type = (sframe->can_id & BIT(31)) >> 31;
	zframe->rtr = (sframe->can_id & BIT(30)) >> 30;
	zframe->id = sframe->can_id & BIT_MASK(29);
	zframe->dlc = can_bytes_to_dlc(sframe->len);
	zframe->fd = !!(sframe->flags & CANFD_FDF);
	memcpy(zframe->data, sframe->data, MIN(sizeof(sframe->data), sizeof(zframe->data)));
}

/**
 * @brief Translate a @a can_frame struct to a @a socketcan_frame struct.
 *
 * @param zframe Pointer to can_frame struct.
 * @param sframe  Pointer to socketcan_frame struct.
 */
static inline void socketcan_from_can_frame(const struct can_frame *zframe,
					    struct socketcan_frame *sframe)
{
	sframe->can_id = (zframe->id_type << 31) | (zframe->rtr << 30) | zframe->id;
	sframe->len = can_dlc_to_bytes(zframe->dlc);
	if (zframe->fd) {
		sframe->flags = CANFD_FDF;
	}
	memcpy(sframe->data, zframe->data, MIN(sizeof(zframe->data), sizeof(sframe->data)));
}

/**
 * @brief Translate a @a socketcan_filter struct to a @a can_filter struct.
 *
 * @param sfilter Pointer to socketcan_filter struct.
 * @param zfilter Pointer to can_filter struct.
 */
static inline void socketcan_to_can_filter(const struct socketcan_filter *sfilter,
					   struct can_filter *zfilter)
{
	zfilter->id_type = (sfilter->can_id & BIT(31)) >> 31;
	zfilter->rtr = (sfilter->can_id & BIT(30)) >> 30;
	zfilter->id = sfilter->can_id & BIT_MASK(29);
	zfilter->rtr_mask = (sfilter->can_mask & BIT(30)) >> 30;
	zfilter->id_mask = sfilter->can_mask & BIT_MASK(29);
}

/**
 * @brief Translate a @a can_filter struct to a @a socketcan_filter struct.
 *
 * @param zfilter Pointer to can_filter struct.
 * @param sfilter Pointer to socketcan_filter struct.
 */
static inline void socketcan_from_can_filter(const struct can_filter *zfilter,
					     struct socketcan_filter *sfilter)
{
	sfilter->can_id = (zfilter->id_type << 31) |
		(zfilter->rtr << 30) | zfilter->id;
	sfilter->can_mask = (zfilter->rtr_mask << 30) |
		(zfilter->id_type << 31) | zfilter->id_mask;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKETCAN_H_ */
