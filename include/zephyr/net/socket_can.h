/** @file
 * @brief Socket CAN definitions.
 *
 * Definitions for socket CAN support.
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKET_CAN_H_
#define ZEPHYR_INCLUDE_NET_SOCKET_CAN_H_

#include <zephyr/drivers/can.h>
#include <zephyr/types.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Socket CAN library
 * @defgroup socket_can Network Core Library
 * @ingroup networking
 * @{
 */

/* Protocols of the protocol family PF_CAN */
#define CAN_RAW 1

/* Socket CAN options */
#define SOL_CAN_BASE 100
#define SOL_CAN_RAW (SOL_CAN_BASE + CAN_RAW)

enum {
	CAN_RAW_FILTER = 1,
};

/* Socket CAN MTU size */
#define CAN_MTU		CAN_MAX_DLEN

/**
 * struct sockaddr_can - The sockaddr structure for CAN sockets
 *
 */
struct sockaddr_can {
	sa_family_t can_family;
	int         can_ifindex;
};

/**
 * @name Linux SocketCAN compatibility
 *
 * The following structures and functions provide compatibility with the CAN
 * frame and CAN filter formats used by Linux SocketCAN.
 *
 * @{
 */

/**
 * CAN Identifier structure for Linux SocketCAN compatibility.
 *
 * The fields in this type are:
 *
 * @code{.text}
 *
 * +------+--------------------------------------------------------------+
 * | Bits | Description                                                  |
 * +======+==============================================================+
 * | 0-28 | CAN identifier (11/29 bit)                                   |
 * +------+--------------------------------------------------------------+
 * |  29  | Error message frame flag (0 = data frame, 1 = error message) |
 * +------+--------------------------------------------------------------+
 * |  30  | Remote transmission request flag (1 = RTR frame)             |
 * +------+--------------------------------------------------------------+
 * |  31  | Frame format flag (0 = standard 11 bit, 1 = extended 29 bit) |
 * +------+--------------------------------------------------------------+
 *
 * @endcode
 */
typedef uint32_t socketcan_id_t;

/**
 * @brief CAN frame for Linux SocketCAN compatibility.
 */
struct socketcan_frame {
	/** 32-bit CAN ID + EFF/RTR/ERR flags. */
	socketcan_id_t can_id;

	/** The data length code (DLC). */
	uint8_t can_dlc;

	/** @cond INTERNAL_HIDDEN */
	uint8_t pad;   /* padding. */
	uint8_t res0;  /* reserved/padding. */
	uint8_t res1;  /* reserved/padding. */
	/** @endcond */

	/** The payload data. */
	uint8_t data[CAN_MAX_DLEN];
};

/**
 * @brief CAN filter for Linux SocketCAN compatibility.
 *
 * A filter is considered a match when `received_can_id & mask == can_id & can_mask`.
 */
struct socketcan_filter {
	/** The CAN identifier to match. */
	socketcan_id_t can_id;
	/** The mask applied to @a can_id for matching. */
	socketcan_id_t can_mask;
};

/**
 * @brief Translate a @a socketcan_frame struct to a @a can_frame struct.
 *
 * @param sframe Pointer to sockecan_frame struct.
 * @param zframe Pointer to can_frame struct.
 */
static inline void can_copy_frame_to_zframe(const struct socketcan_frame *sframe,
					    struct can_frame *zframe)
{
	zframe->id_type = (sframe->can_id & BIT(31)) >> 31;
	zframe->rtr = (sframe->can_id & BIT(30)) >> 30;
	zframe->id = sframe->can_id & BIT_MASK(29);
	zframe->dlc = sframe->can_dlc;
	memcpy(zframe->data, sframe->data, sizeof(zframe->data));
}

/**
 * @brief Translate a @a can_frame struct to a @a socketcan_frame struct.
 *
 * @param zframe Pointer to can_frame struct.
 * @param sframe  Pointer to socketcan_frame struct.
 */
static inline void can_copy_zframe_to_frame(const struct can_frame *zframe,
					    struct socketcan_frame *sframe)
{
	sframe->can_id = (zframe->id_type << 31) | (zframe->rtr << 30) | zframe->id;
	sframe->can_dlc = zframe->dlc;
	memcpy(sframe->data, zframe->data, sizeof(sframe->data));
}

/**
 * @brief Translate a @a socketcan_filter struct to a @a can_filter struct.
 *
 * @param sfilter Pointer to socketcan_filter struct.
 * @param zfilter Pointer to can_filter struct.
 */
static inline void can_copy_filter_to_zfilter(const struct socketcan_filter *sfilter,
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
static inline void can_copy_zfilter_to_filter(const struct can_filter *zfilter,
					      struct socketcan_filter *sfilter)
{
	sfilter->can_id = (zfilter->id_type << 31) |
		(zfilter->rtr << 30) | zfilter->id;
	sfilter->can_mask = (zfilter->rtr_mask << 30) |
		(zfilter->id_type << 31) | zfilter->id_mask;
}

/** @} */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_CAN_H_ */
