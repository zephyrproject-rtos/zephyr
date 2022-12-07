/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_CAN_H_
#define ZEPHYR_INCLUDE_NET_CAN_H_

#include <zephyr/types.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/drivers/can.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * CAN L2 network driver API.
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

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_CAN_H_ */
