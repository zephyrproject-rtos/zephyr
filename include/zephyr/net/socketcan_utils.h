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
	memset(zframe, 0, sizeof(*zframe));

	zframe->flags |= (sframe->can_id & BIT(31)) != 0 ? CAN_FRAME_IDE : 0;
	zframe->flags |= (sframe->can_id & BIT(30)) != 0 ? CAN_FRAME_RTR : 0;
	zframe->flags |= (sframe->flags & CANFD_FDF) != 0 ? CAN_FRAME_FDF : 0;
	zframe->flags |= (sframe->flags & CANFD_BRS) != 0 ? CAN_FRAME_BRS : 0;
	zframe->id = sframe->can_id & BIT_MASK(29);
	zframe->dlc = can_bytes_to_dlc(sframe->len);

	if ((zframe->flags & CAN_FRAME_RTR) == 0U) {
		memcpy(zframe->data, sframe->data,
		       MIN(sframe->len, MIN(sizeof(sframe->data), sizeof(zframe->data))));
	}
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
	memset(sframe, 0, sizeof(*sframe));

	sframe->can_id = zframe->id;
	sframe->can_id |= (zframe->flags & CAN_FRAME_IDE) != 0 ? BIT(31) : 0;
	sframe->can_id |= (zframe->flags & CAN_FRAME_RTR) != 0 ? BIT(30) : 0;
	sframe->len = can_dlc_to_bytes(zframe->dlc);

	if ((zframe->flags & CAN_FRAME_FDF) != 0) {
		sframe->flags |= CANFD_FDF;
	}

	if ((zframe->flags & CAN_FRAME_BRS) != 0) {
		sframe->flags |= CANFD_BRS;
	}

	if ((zframe->flags & CAN_FRAME_RTR) == 0U) {
		memcpy(sframe->data, zframe->data,
		       MIN(sframe->len, MIN(sizeof(zframe->data), sizeof(sframe->data))));
	}
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
	memset(zfilter, 0, sizeof(*zfilter));

	zfilter->flags |= (sfilter->can_id & BIT(31)) != 0 ? CAN_FILTER_IDE : 0;
	zfilter->id = sfilter->can_id & BIT_MASK(29);
	zfilter->mask = sfilter->can_mask & BIT_MASK(29);
	zfilter->flags |= (sfilter->flags & CANFD_FDF) != 0 ? CAN_FILTER_FDF : 0;

	if ((sfilter->can_mask & BIT(30)) == 0) {
		zfilter->flags |= CAN_FILTER_DATA | CAN_FILTER_RTR;
	} else if ((sfilter->can_id & BIT(30)) == 0) {
		zfilter->flags |= CAN_FILTER_DATA;
	} else {
		zfilter->flags |= CAN_FILTER_RTR;
	}
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
	memset(sfilter, 0, sizeof(*sfilter));

	sfilter->can_id = zfilter->id;
	sfilter->can_id |= (zfilter->flags & CAN_FILTER_IDE) != 0 ? BIT(31) : 0;
	sfilter->can_id |= (zfilter->flags & CAN_FILTER_RTR) != 0 ? BIT(30) : 0;

	sfilter->can_mask = zfilter->mask;
	sfilter->can_mask |= (zfilter->flags & CAN_FILTER_IDE) != 0 ? BIT(31) : 0;

	if ((zfilter->flags & (CAN_FILTER_DATA | CAN_FILTER_RTR)) !=
		(CAN_FILTER_DATA | CAN_FILTER_RTR)) {
		sfilter->can_mask |= BIT(30);
	}

	if ((zfilter->flags & CAN_FILTER_FDF) != 0) {
		sfilter->flags |= CANFD_FDF;
	}
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKETCAN_H_ */
