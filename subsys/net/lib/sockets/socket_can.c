/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_sock_can

#define NET_LOG_LEVEL CONFIG_NET_SOCKETS_LOG_LEVEL

#include <net/net_context.h>
#include <net/socket_can.h>
#include <device.h>
#include <misc/printk.h>
#include <kernel.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/buf.h>
#include <net/net_pkt.h>
#include <net/net_l2.h>
#include <zephyr.h>
#include <can.h>
#include <net/socket.h>

int socket_can_get_opt(struct net_context *context, int optname,
		       void *optval, socklen_t *optlen)
{
	int ret = 0;
	struct device *dev;
	struct net_if *iface;
	struct socket_can_context *sock_ctx;

	if (!context) {
		ret = -EBADF;
		goto err;
	}

	if (!optval || !optlen) {
		ret = -EINVAL;
		goto err;
	}

	iface = net_context_get_iface(context);
	if (iface == NULL) {
		ret = -EBADF;
		goto err;
	}

	dev = net_if_get_device(iface);
	if (dev == NULL) {
		ret = -EBADF;
		goto err;
	}

	sock_ctx = dev->driver_data;

	switch (optname) {
	case SOCKET_CAN_GET_IF_INDEX:
		*((u8_t *)optval) = net_if_get_by_iface(sock_ctx->iface);
		*optlen = sizeof(u8_t);
		break;
	default:
		ret = ENOPROTOOPT;
	}

err:
	return ret;

}

int socket_can_set_opt(struct net_context *context, int optname,
		       const void *optval, socklen_t optlen)
{
	int ret = 0;
	struct device *dev;
	struct can_filter *filter;
	struct socket_can_mode *mode;
	struct device *can_dev;
	struct net_if *iface;
	struct socket_can_context *sock_ctx;
	const struct socket_can_driver_api_t *api;

	ARG_UNUSED(optlen);

	if (!context) {
		ret = -EBADF;
		goto err;
	}

	if (!optval) {
		ret = -EINVAL;
		goto err;
	}

	iface = net_context_get_iface(context);
	if (iface == NULL) {
		ret = -EBADF;
		goto err;
	}

	dev = net_if_get_device(iface);
	if (dev == NULL) {
		ret = -EBADF;
		goto err;
	}

	sock_ctx = dev->driver_data;
	if (sock_ctx == NULL) {
		ret = -EBADF;
		goto err;
	}

	can_dev = sock_ctx->can_dev;

	if (can_dev == NULL) {
		ret = -EBADF;
		goto err;
	}

	switch (optname) {
	case SOCKET_CAN_SET_FILTER:
		filter = (struct can_filter *)optval;
		api = dev->driver_api;
		if (api == NULL) {
			ret = -EBADF;
			goto err;
		}
		if (api->config_filter) {
			ret = api->config_filter(dev, filter);
		} else {
			ret = -ENOPROTOOPT;
			goto err;
		}
		break;
	case SOCKET_CAN_SET_MODE:
		mode = (struct socket_can_mode *)optval;
		ret = can_configure(can_dev,
				    mode->op_mode, mode->baud_rate);
		break;
	default:
		ret = -ENOPROTOOPT;
	}

err:
	return ret;

}

void socket_can_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct socket_can_context *context = dev->driver_data;

	context->iface = iface;
}

static void socket_can_tx_callback(u32_t flags)
{
	NET_DBG("TX CALLBACK OCCURRED flags = %x", flags);
	if (flags != CAN_TX_OK) {
		NET_DBG("SOCKET_CAN_TRANSMISSION FAILED ERR = %d", flags);
	}
}

int socket_can_iface_send(struct net_if *iface, struct net_pkt *pkt)
{
	int ret = CAN_TX_ERR;
	struct device *dev;
	struct socket_can_context *context;
	struct device *can_dev;
	struct net_buf *buf;
	struct can_msg *msg;

	if (pkt == NULL) {
		goto err;
	}
	dev = net_if_get_device(iface);
	if (dev == NULL) {
		goto err;
	}
	context = dev->driver_data;
	if (context == NULL) {
		goto err;
	}
	can_dev = context->can_dev;

	buf = pkt->frags;
	if ((buf == NULL) || (buf->data == NULL)) {
		goto err;
	}
	msg = (struct can_msg *)(buf->data);

	ret = can_send(can_dev, msg, K_FOREVER, socket_can_tx_callback);
err:
	return ret;
}

bool socket_can_check_matched_id_filter(struct net_pkt *pkt)
{
	struct net_if *iface = NULL;
	struct net_context *ctx = NULL;
	struct device *dev = NULL;
	const struct socket_can_driver_api_t *api = NULL;
	bool result = false;

	if (pkt == NULL) {
		goto err;
	}

	ctx = net_pkt_context(pkt);
	if (ctx == NULL) {
		goto err;
	}

	iface = net_context_get_iface(ctx);
	if (iface == NULL) {
		goto err;
	}

	dev = net_if_get_device(iface);
	if (dev == NULL) {
		goto err;
	}

	api = dev->driver_api;
	if (api == NULL) {
		goto err;
	}

	if (api->check_matched_filter) {
		result = api->check_matched_filter(dev, pkt);
	}

err:
	return result;
}

