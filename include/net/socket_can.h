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

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <net/net_ip.h>
#include <can.h>

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

/* Socket CAN MTU size */
#define CAN_MTU		(sizeof(struct can_msg))

/**
 * struct sockaddr_can - The sockaddr structure for CAN sockets
 *
 */
struct sockaddr_can {
	sa_family_t can_family;
	int         can_ifindex;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_CAN_H_ */
