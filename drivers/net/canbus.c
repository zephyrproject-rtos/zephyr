/*
 * Copyright (c) 2022 Vestas Wind Systems A/S
 * Copyright (c) 2022 Alexander Wachter
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/canbus.h>
#include <zephyr/net/socketcan.h>
#include <zephyr/drivers/can.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_canbus, CONFIG_NET_CANBUS_LOG_LEVEL);

#define SEND_TIMEOUT K_MSEC(100)

struct net_canbus_context {
	struct net_if *iface;
};

struct net_canbus_config {
	const struct device *can_dev;
};

static void net_canbus_recv(const struct device *dev, struct can_frame *frame, void *user_data)
{
	struct net_canbus_context *ctx = user_data;
	struct net_pkt *pkt;
	int ret;

	ARG_UNUSED(dev);

	LOG_DBG("pkt on interface %p", ctx->iface);
	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, sizeof(struct can_frame),
					   AF_CAN, 0, K_NO_WAIT);
	if (pkt == NULL) {
		LOG_ERR("Failed to obtain net_pkt");
		return;
	}

	if (net_pkt_write(pkt, frame, sizeof(struct can_frame))) {
		LOG_ERR("Failed to append RX data");
		net_pkt_unref(pkt);
		return;
	}

	ret = net_recv_data(ctx->iface, pkt);
	if (ret < 0) {
		LOG_DBG("net_recv_data failed [%d]", ret);
		net_pkt_unref(pkt);
	}
}

static int net_canbus_setsockopt(const struct device *dev, void *obj, int level,
				 int optname, const void *optval, socklen_t optlen)
{
	const struct net_canbus_config *cfg = dev->config;
	struct net_canbus_context *context = dev->data;
	struct net_context *ctx = obj;
	int ret;

	if (level != SOL_CAN_RAW && optname != CAN_RAW_FILTER) {
		errno = EINVAL;
		return -1;
	}

	__ASSERT_NO_MSG(optlen == sizeof(struct can_filter));

	ret = can_add_rx_filter(cfg->can_dev, net_canbus_recv, context, optval);
	if (ret == -ENOSPC) {
		errno = ENOSPC;
		return -1;
	}

	net_context_set_can_filter_id(ctx, ret);

	return 0;
}

static void net_canbus_close(const struct device *dev, int filter_id)
{
	const struct net_canbus_config *cfg = dev->config;

	can_remove_rx_filter(cfg->can_dev, filter_id);
}

static void net_canbus_send_tx_callback(const struct device *dev, int error, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (error != 0) {
		LOG_DBG("CAN bus TX error [%d]", error);
	}
}

static int net_canbus_send(const struct device *dev, struct net_pkt *pkt)
{
	const struct net_canbus_config *cfg = dev->config;
	int ret;

	if (net_pkt_family(pkt) != AF_CAN) {
		return -EPFNOSUPPORT;
	}

	ret = can_send(cfg->can_dev, (struct can_frame *)pkt->frags->data,
		       SEND_TIMEOUT, net_canbus_send_tx_callback, NULL);

	if (ret == 0) {
		net_pkt_unref(pkt);
	} else {
		LOG_DBG("Cannot send CAN msg (%d)", ret);
	}

	/* If something went wrong, then we need to return negative value to
	 * net_if.c:net_if_tx() so that the net_pkt will get released.
	 */
	return ret;
}

static void net_canbus_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_canbus_context *context = dev->data;

	context->iface = iface;

	LOG_DBG("Init CAN interface %p dev %p", iface, dev);
}

static int net_canbus_init(const struct device *dev)
{
	const struct net_canbus_config *cfg = dev->config;

	if (!device_is_ready(cfg->can_dev)) {
		LOG_ERR("CAN device not ready");
		return -ENODEV;
	}

	return 0;
}

static struct canbus_api net_canbus_api = {
	.iface_api.init = net_canbus_iface_init,
	.send = net_canbus_send,
	.close = net_canbus_close,
	.setsockopt = net_canbus_setsockopt,
};

static struct net_canbus_context net_canbus_ctx;

static const struct net_canbus_config net_canbus_cfg = {
	.can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus))
};

NET_DEVICE_INIT(net_canbus, "NET_CANBUS", net_canbus_init, NULL, &net_canbus_ctx, &net_canbus_cfg,
		CONFIG_NET_CANBUS_INIT_PRIORITY, &net_canbus_api, CANBUS_RAW_L2,
		NET_L2_GET_CTX_TYPE(CANBUS_RAW_L2), CAN_MTU);
