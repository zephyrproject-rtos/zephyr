/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief CAN bus socket API definitions.
 */

#ifndef ZEPHYR_INCLUDE_NET_CANBUS_H_
#define ZEPHYR_INCLUDE_NET_CANBUS_H_

#include <zephyr/types.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socketcan.h>
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
			  const void *optval, net_socklen_t optlen);

	/** Get socket CAN option */
	int (*getsockopt)(const struct device *dev, void *obj, int level,
			  int optname,
			  const void *optval, net_socklen_t *optlen);
};

/* Make sure that the network interface API is properly setup inside
 * CANBUS API struct (it is the first one).
 */
BUILD_ASSERT(offsetof(struct canbus_api, iface_api) == 0);

/** @cond INTERNAL_HIDDEN */

struct z_net_canbus_context {
	struct net_if *iface;
};

struct z_net_canbus_config {
	const struct device *can_dev;
};

int z_net_canbus_init(const struct device *dev);

extern const struct canbus_api z_net_canbus_api;

/*
 * Network interface of a CAN controller, defined by CAN_DEVICE_DT_DEFINE() next to the
 * controller's device so that only controllers with a driver in the build get one.
 */
#define Z_NET_CANBUS_DEVICE_DT_DEFINE(node_id)                                                     \
	static struct z_net_canbus_context z_net_canbus_ctx_##node_id;                             \
	static const struct z_net_canbus_config z_net_canbus_cfg_##node_id = {                     \
		.can_dev = DEVICE_DT_GET(node_id),                                                 \
	};                                                                                         \
	NET_DEVICE_INIT(net_canbus_##node_id, DEVICE_DT_NAME(node_id), z_net_canbus_init, NULL,    \
			&z_net_canbus_ctx_##node_id, &z_net_canbus_cfg_##node_id,                  \
			CONFIG_NET_CANBUS_INIT_PRIORITY, &z_net_canbus_api, CANBUS_RAW_L2,         \
			NET_L2_GET_CTX_TYPE(CANBUS_RAW_L2), CAN_MTU);

/** @endcond */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_CANBUS_H_ */
