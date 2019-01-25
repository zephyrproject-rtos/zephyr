/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_INCLUDE_NET_DUMMY_H_
#define ZEPHYR_INCLUDE_NET_DUMMY_H_

#include <net/net_if.h>
#include <net/net_pkt.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Dummy L2/driver support functions
 * @defgroup dummy Dummy L2/driver Support Functions
 * @ingroup networking
 * @{
 */

struct dummy_api {
	/**
	 * The net_if_api must be placed in first position in this
	 * struct so that we are compatible with network interface API.
	 */
	struct net_if_api iface_api;

	/** Send a network packet */
	int (*send)(struct device *dev, struct net_pkt *pkt);
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_NET_DUMMY_H_ */
