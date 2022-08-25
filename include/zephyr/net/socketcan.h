/** @file
 * @brief SocketCAN definitions.
 *
 * Definitions for SocketCAN support.
 */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_SOCKETCAN_H_
#define ZEPHYR_INCLUDE_NET_SOCKETCAN_H_

#include <zephyr/types.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SocketCAN library
 * @defgroup socket_can Network Core Library
 * @ingroup networking
 * @{
 */

/* Protocols of the protocol family PF_CAN */
#define CAN_RAW 1

/* SocketCAN options */
#define SOL_CAN_BASE 100
#define SOL_CAN_RAW (SOL_CAN_BASE + CAN_RAW)

enum {
	CAN_RAW_FILTER = 1,
};

/* SocketCAN MTU size compatible with Linux */
#ifdef CONFIG_CAN_FD_MODE
#define SOCKETCAN_MAX_DLEN 64U
#define CANFD_MTU (sizeof(struct socketcan_frame))
#define CAN_MTU (CANFD_MTU - 56U)
#else /* CONFIG_CAN_FD_MODE */
#define SOCKETCAN_MAX_DLEN 8U
#define CAN_MTU (sizeof(struct socketcan_frame))
#endif /* !CONFIG_CAN_FD_MODE */

/* CAN-FD specific flags from Linux Kernel (include/uapi/linux/can.h) */
#define CANFD_BRS 0x01 /* bit rate switch (second bitrate for payload data) */
#define CANFD_ESI 0x02 /* error state indicator of the transmitting node */
#define CANFD_FDF 0x04 /* mark CAN FD for dual use of struct canfd_frame */

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
	/** Frame payload length in bytes. */
	uint8_t len;
	/** Additional flags for CAN FD. */
	uint8_t flags;
	/** @cond INTERNAL_HIDDEN */
	uint8_t res0;  /* reserved/padding. */
	uint8_t res1;  /* reserved/padding. */
	/** @endcond */

	/** The payload data. */
	uint8_t data[SOCKETCAN_MAX_DLEN];
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

/** @} */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKETCAN_H_ */
