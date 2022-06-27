/*
 * Copyright (c) 2019 Intel Corporation.
 * Copyright (c) 2022 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket_can.h>
#include <zephyr/drivers/can.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(can_socketcan, CONFIG_CAN_LOG_LEVEL);

#define SEND_TIMEOUT K_MSEC(100)

struct socketcan_context {
	struct net_if *iface;
};

struct socketcan_config {
	const struct device *can_dev;
};

static void socketcan_recv(const struct device *dev, struct zcan_frame *frame, void *arg)
{
	struct socketcan_context *ctx = (struct socketcan_context *)arg;
	struct net_pkt *pkt;
	int ret;

	ARG_UNUSED(dev);

	LOG_DBG("pkt on interface %p", ctx->iface);
	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, sizeof(struct zcan_frame),
					   AF_CAN, 0, K_NO_WAIT);
	if (pkt == NULL) {
		LOG_ERR("Failed to obtain net_pkt");
		return;
	}

	if (net_pkt_write(pkt, frame, sizeof(struct zcan_frame))) {
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

static int socketcan_setsockopt(const struct device *dev, void *obj, int level,
				 int optname, const void *optval, socklen_t optlen)
{
	const struct socketcan_config *cfg = dev->config;
	struct socketcan_context *socket_context = dev->data;
	struct net_context *ctx = obj;
	int ret;

	if (level != SOL_CAN_RAW && optname != CAN_RAW_FILTER) {
		errno = EINVAL;
		return -1;
	}

	__ASSERT_NO_MSG(optlen == sizeof(struct zcan_filter));

	ret = can_add_rx_filter(cfg->can_dev, socketcan_recv, socket_context, optval);
	if (ret == -ENOSPC) {
		errno = ENOSPC;
		return -1;
	}

	net_context_set_filter_id(ctx, ret);

	return 0;
}

static void socketcan_close(const struct device *dev, int filter_id)
{
	const struct socketcan_config *cfg = dev->config;

	can_remove_rx_filter(cfg->can_dev, filter_id);
}

static void socketcan_send_tx_callback(const struct device *dev, int error, void *arg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(arg);

	if (error != 0) {
		LOG_DBG("socket CAN TX error [%d]", error);
	}
}

static int socketcan_send(const struct device *dev, struct net_pkt *pkt)
{
	const struct socketcan_config *cfg = dev->config;
	int ret;

	if (net_pkt_family(pkt) != AF_CAN) {
		return -EPFNOSUPPORT;
	}

	ret = can_send(cfg->can_dev, (struct zcan_frame *)pkt->frags->data,
		       SEND_TIMEOUT, socketcan_send_tx_callback, NULL);

	if (ret == 0) {
		net_pkt_unref(pkt);
	} else {
		LOG_DBG("Cannot send socket CAN msg (%d)", ret);
	}

	/* If something went wrong, then we need to return negative value to
	 * net_if.c:net_if_tx() so that the net_pkt will get released.
	 */
	return ret;
}

static void socketcan_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct socketcan_context *socket_context = dev->data;

	socket_context->iface = iface;

	LOG_DBG("Init CAN interface %p dev %p", iface, dev);
}

static int socketcan_init(const struct device *dev)
{
	const struct socketcan_config *cfg = dev->config;

	if (!device_is_ready(cfg->can_dev)) {
		LOG_ERR("CAN device not ready");
		return -ENODEV;
	}

	return 0;
}

static struct canbus_api socketcan_api = {
	.iface_api.init = socketcan_iface_init,
	.send = socketcan_send,
	.close = socketcan_close,
	.setsockopt = socketcan_setsockopt,
};

static struct socketcan_context socketcan_ctx;

static const struct socketcan_config socketcan_cfg = {
	.can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus))
};

NET_DEVICE_INIT(socket_can, "SOCKET_CAN", socketcan_init, NULL, &socketcan_ctx, &socketcan_cfg,
		CONFIG_CAN_SOCKETCAN_INIT_PRIORITY, &socketcan_api, CANBUS_RAW_L2,
		NET_L2_GET_CTX_TYPE(CANBUS_RAW_L2), CAN_MTU);
