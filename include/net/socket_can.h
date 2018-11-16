/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOCKET_CAN_H_
#define _SOCKET_CAN_H_

#include <device.h>
#include <net/net_ip.h>
#include <can.h>
#include <zephyr/types.h>

/** Max length of socket packet */
#define CAN_NET_MTU (72)

/**  Protocol family of socket can connection */
#define CAN_CONTEXT_FAMILY BIT(8)

/**  Context type of socket can connection */
#define CAN_CONTEXT_TYPE BIT(9)

/**  Context proto for socket can connection */
#define CAN_CONTEXT_PROTO BIT(10)

/**
 * @brief socket addr structure for AF_CAN protocol
 *
 * Socket address structure for AF_CAN protocol family
 *
 */
struct sockaddr_can {
	/** protocol family for socket can */
	sa_family_t can_family;
	/** net_if interface associated with socket */
	u8_t can_ifindex;
};

/**
 * @brief check matched CAN filter for incoming CAN message.
 *
 * This routine matches id with configured filters id and return true or false.
 * *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param pkt       incoming net_pkt
 *
 * @retval 1 if filter matched otherwise 0.
 */
typedef int (*socket_can_check_filter_matched_t)(struct device *dev,
						 struct net_pkt *pkt);

/**
 * @brief configure filter for accpetance filter.
 *
 * This routine configure can filter and register rx callback.
 * *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param filter    can_filter structure
 *
 * @retval return < 0 in case of error and 0 for success.
 */
typedef int (*socket_can_config_filter_t)(struct device *dev,
					  struct can_filter *filter);

int socket_can_get_opt(struct net_context *context, int optname,
		       void *optval, socklen_t *optlen);
int socket_can_set_opt(struct net_context *context, int optname,
		       const void *optval, socklen_t optlen);



/**
 * @brief socket can driver api structure
 * Socket CAN driver apis for iface interface functions and ioctl call.
 *
 */
struct socket_can_driver_api_t {
	/** iface interface for socket apis to send and init function*/
	struct net_if_api iface_api;
	/** check matching filter for loopback messages */
	socket_can_check_filter_matched_t check_matched_filter;
	/** configure filter with rx callback */
	socket_can_config_filter_t config_filter;
};

/**
 * @brief socket can context structure
 *
 * socket can driver specific structure.
 *
 */
struct socket_can_context {
	/** id of CAN instance */
	u32_t id;
	/** iface interface reference structure*/
	struct net_if *iface;
	/** can device structure reference */
	struct device *can_dev;
};

/**
 * @brief socket can mode structure
 *
 * This structure will be passed via setSocketOpt and configure can controller
 * operation mode as well as baud rate
 */
struct socket_can_mode {
	enum can_mode op_mode;
	u32_t baud_rate;
};

__syscall int socket_can_check_filter_matched(struct device *dev,
					      struct net_pkt *pkt);

static inline int _impl_socket_can_check_filter_matched(struct device *dev,
							struct net_pkt *pkt)
{
	const struct socket_can_driver_api_t *api = NULL;
	int result = 0;

	if (pkt == NULL) {
		goto err;
	}

	api = dev->driver_api;
	if (api && api->check_matched_filter) {
		result = api->check_matched_filter(dev, pkt);
	}
err:
	return result;
}

__syscall int socket_can_config_filter(struct device *dev,
				       struct can_filter *filter);

static inline int _impl_socket_can_config_filter(struct device *dev,
						 struct can_filter *filter)
{
	const struct socket_can_driver_api_t *api = NULL;
	int res = 0;

	if (filter == NULL) {
		res = -EBADF;
		goto err;
	}

	api = dev->driver_api;
	if (api && api->config_filter) {
		res = api->config_filter(dev, filter);
	}
err:
	return res;
}

#include <syscalls/socket_can.h>

#endif /*_SOCKET_CAN_H_ */
