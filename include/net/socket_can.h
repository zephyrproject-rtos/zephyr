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

#include <zephyr/types.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <drivers/can.h>

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
 * CAN L2 driver API. Used by socket CAN.
 */
struct canbus_api {
	/**
	 * The net_if_api must be placed in first position in this
	 * struct so that we are compatible with network interface API.
	 */
	struct net_if_api iface_api;

	/** Send a CAN packet by socket */
	int (*send)(const struct device *dev, struct net_pkt *pkt);

	/** Close the related CAN socket */
	void (*close)(const struct device *dev, int filter_id);

	/** Set socket CAN option */
	int (*setsockopt)(const struct device *dev, void *obj, int level,
			  int optname,
			  const void *optval, socklen_t optlen);

	/** Get socket CAN option */
	int (*getsockopt)(const struct device *dev, void *obj, int level,
			  int optname,
			  const void *optval, socklen_t *optlen);
};

/* Make sure that the network interface API is properly setup inside
 * CANBUS API struct (it is the first one).
 */
BUILD_ASSERT(offsetof(struct canbus_api, iface_api) == 0);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_SOCKET_CAN_H_ */
