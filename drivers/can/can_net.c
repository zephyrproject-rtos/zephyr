/*
 * Copyright (c) 2019 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <net/can.h>
#include <net/net_pkt.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(net_can, CONFIG_CAN_NET_LOG_LEVEL);

struct mcast_filter_mapping {
	const struct in6_addr *addr;
	int filter_id;
};

struct net_can_context {
	const struct device *can_dev;
	struct net_if *iface;
	int recv_filter_id;
	struct mcast_filter_mapping mcast_mapping[NET_IF_MAX_IPV6_MADDR];
#ifdef CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR
	int eth_bridge_filter_id;
	int all_mcast_filter_id;
#endif
};

static struct net_if_mcast_monitor mcast_monitor;

struct mcast_filter_mapping *can_get_mcast_filter(struct net_can_context *ctx,
					      const struct in6_addr *addr)
{
	struct mcast_filter_mapping *map = ctx->mcast_mapping;

	for (int i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		if (map[i].addr == addr) {
			return &map[i];
		}
	}

	return NULL;
}

static inline uint8_t can_get_frame_datalength(struct zcan_frame *frame)
{
	/* TODO: Needs update when CAN FD support is added */
	return frame->dlc;
}

static inline uint16_t can_get_lladdr_src(struct zcan_frame *frame)
{
	return (frame->ext_id >> CAN_NET_IF_ADDR_SRC_POS) &
	       CAN_NET_IF_ADDR_MASK;
}

static inline uint16_t can_get_lladdr_dest(struct zcan_frame *frame)
{
	uint16_t addr = (frame->ext_id >> CAN_NET_IF_ADDR_DEST_POS) &
		     CAN_NET_IF_ADDR_MASK;

	if (frame->ext_id & CAN_NET_IF_ADDR_MCAST_MASK) {
		addr |= CAN_NET_IF_IS_MCAST_BIT;
	}

	return addr;
}

static inline void can_set_lladdr(struct net_pkt *pkt, struct zcan_frame *frame)
{
	struct net_buf *buf = pkt->buffer;

	/* Put the destination at the beginning of the pkt.
	 * The net_canbus_lladdr has a size if 14 bits. To convert it to
	 * network byte order, we treat it as 16 bits here.
	 */
	net_pkt_lladdr_dst(pkt)->addr = buf->data;
	net_pkt_lladdr_dst(pkt)->len = sizeof(struct net_canbus_lladdr);
	net_pkt_lladdr_dst(pkt)->type = NET_LINK_CANBUS;
	net_buf_add_be16(buf, can_get_lladdr_dest(frame));
	net_buf_pull(buf, sizeof(uint16_t));

	/* Do the same as above for the source address */
	net_pkt_lladdr_src(pkt)->addr = buf->data;
	net_pkt_lladdr_src(pkt)->len = sizeof(struct net_canbus_lladdr);
	net_pkt_lladdr_src(pkt)->type = NET_LINK_CANBUS;
	net_buf_add_be16(buf, can_get_lladdr_src(frame));
	net_buf_pull(buf, sizeof(uint16_t));
}

static int net_can_send(const struct device *dev,
			const struct zcan_frame *frame,
			can_tx_callback_t cb, void *cb_arg, k_timeout_t timeout)
{
	struct net_can_context *ctx = dev->data;

	NET_ASSERT(frame->id_type == CAN_EXTENDED_IDENTIFIER);
	return can_send(ctx->can_dev, frame, timeout, cb, cb_arg);
}

static void net_can_recv(struct zcan_frame *frame, void *arg)
{
	struct net_can_context *ctx = (struct net_can_context *)arg;
	size_t pkt_size = 2 * sizeof(struct net_canbus_lladdr) +
			  can_get_frame_datalength(frame);
	struct net_pkt *pkt;
	int ret;

	NET_DBG("Frame with ID 0x%x received", frame->ext_id);
	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, pkt_size, AF_UNSPEC, 0,
					   K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("Failed to obtain net_pkt with size of %d", pkt_size);
		goto drop;
	}

	pkt->canbus_rx_ctx = NULL;

	can_set_lladdr(pkt, frame);
	net_pkt_cursor_init(pkt);
	ret = net_pkt_write(pkt, frame->data, can_get_frame_datalength(frame));
	if (ret) {
		LOG_ERR("Failed to append frame data to net_pkt");
		goto drop;
	}

	ret = net_recv_data(ctx->iface, pkt);
	if (ret < 0) {
		LOG_ERR("Packet dropped by NET stack");
		goto drop;
	}

	return;

drop:
	NET_INFO("pkt dropped");

	if (pkt) {
		net_pkt_unref(pkt);
	}
}

static inline int attach_mcast_filter(struct net_can_context *ctx,
				      const struct in6_addr *addr)
{
	static struct zcan_filter filter = {
		.id_type = CAN_EXTENDED_IDENTIFIER,
		.rtr = CAN_DATAFRAME,
		.rtr_mask = 1,
		.ext_id_mask = CAN_NET_IF_ADDR_MCAST_MASK |
			       CAN_NET_IF_ADDR_DEST_MASK
	};
	const uint16_t group =
		sys_be16_to_cpu(UNALIGNED_GET((&addr->s6_addr16[7])));
	int filter_id;

	filter.ext_id = CAN_NET_IF_ADDR_MCAST_MASK |
			((group & CAN_NET_IF_ADDR_MASK) <<
			 CAN_NET_IF_ADDR_DEST_POS);

	filter_id = can_attach_isr(ctx->can_dev, net_can_recv,
				   ctx, &filter);
	if (filter_id == CAN_NET_FILTER_NOT_SET) {
		return CAN_NET_FILTER_NOT_SET;
	}

	NET_DBG("Attached mcast filter. Group 0x%04x. Filter:%d",
		group, filter_id);

	return filter_id;
}

static void mcast_cb(struct net_if *iface, const struct in6_addr *addr,
		     bool is_joined)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_can_context *ctx = dev->data;
	struct mcast_filter_mapping *filter_mapping;
	int filter_id;

	if (is_joined) {
		filter_mapping = can_get_mcast_filter(ctx, NULL);
		if (!filter_mapping) {
			NET_ERR("Can't get a free filter_mapping");
		}

		filter_id = attach_mcast_filter(ctx, addr);
		if (filter_id < 0) {
			NET_ERR("Can't attach mcast filter");
			return;
		}

		filter_mapping->addr = addr;
		filter_mapping->filter_id = filter_id;
	} else {
		filter_mapping = can_get_mcast_filter(ctx, addr);
		if (!filter_mapping) {
			NET_ERR("No filter mapping found");
			return;
		}

		can_detach(ctx->can_dev, filter_mapping->filter_id);
		filter_mapping->addr = NULL;
	}
}

static void net_can_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct net_can_context *ctx = dev->data;

	ctx->iface = iface;

	NET_DBG("Init CAN network interface %p dev %p", iface, dev);

	net_6locan_init(iface);

	net_if_mcast_mon_register(&mcast_monitor, iface, mcast_cb);
}

static int can_attach_filter(const struct device *dev, can_rx_callback_t cb,
			     void *cb_arg,
			     const struct zcan_filter *filter)
{
	struct net_can_context *ctx = dev->data;

	return can_attach_isr(ctx->can_dev, cb, cb_arg, filter);
}

static void can_detach_filter(const struct device *dev, int filter_id)
{
	struct net_can_context *ctx = dev->data;

	if (filter_id >= 0) {
		can_detach(ctx->can_dev, filter_id);
	}
}

static inline int can_attach_unicast_filter(struct net_can_context *ctx)
{
	struct zcan_filter filter = {
		.id_type = CAN_EXTENDED_IDENTIFIER,
		.rtr = CAN_DATAFRAME,
		.rtr_mask = 1,
		.ext_id_mask = CAN_NET_IF_ADDR_DEST_MASK
	};
	const uint8_t *link_addr = net_if_get_link_addr(ctx->iface)->addr;
	const uint16_t dest = sys_be16_to_cpu(UNALIGNED_GET((uint16_t *) link_addr));
	int filter_id;

	filter.ext_id = (dest << CAN_NET_IF_ADDR_DEST_POS);

	filter_id = can_attach_isr(ctx->can_dev, net_can_recv,
				   ctx, &filter);
	if (filter_id == CAN_NET_FILTER_NOT_SET) {
		NET_ERR("Can't attach FF filter");
		return CAN_NET_FILTER_NOT_SET;
	}

	NET_DBG("Attached FF filter %d", filter_id);

	return filter_id;
}

#ifdef CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR
static inline int can_attach_eth_bridge_filter(struct net_can_context *ctx)
{
	const struct zcan_filter filter = {
		.id_type = CAN_EXTENDED_IDENTIFIER,
		.rtr = CAN_DATAFRAME,
		.rtr_mask = 1,
		.ext_id_mask = CAN_NET_IF_ADDR_DEST_MASK,
		.ext_id = (NET_CAN_ETH_TRANSLATOR_ADDR <<
			   CAN_NET_IF_ADDR_DEST_POS)
	};

	int filter_id;

	filter_id = can_attach_isr(ctx->can_dev, net_can_recv,
				   ctx, &filter);
	if (filter_id == CAN_NET_FILTER_NOT_SET) {
		NET_ERR("Can't attach ETH bridge filter");
		return CAN_NET_FILTER_NOT_SET;
	}

	NET_DBG("Attached ETH bridge filter %d", filter_id);

	return filter_id;
}

static inline int can_attach_all_mcast_filter(struct net_can_context *ctx)
{
	const struct zcan_filter filter = {
		.id_type = CAN_EXTENDED_IDENTIFIER,
		.rtr = CAN_DATAFRAME,
		.rtr_mask = 1,
		.ext_id_mask = CAN_NET_IF_ADDR_MCAST_MASK,
		.ext_id = CAN_NET_IF_ADDR_MCAST_MASK
	};

	int filter_id;

	filter_id = can_attach_isr(ctx->can_dev, net_can_recv,
				   ctx, &filter);
	if (filter_id == CAN_NET_FILTER_NOT_SET) {
		NET_ERR("Can't attach all mcast filter");
		return CAN_NET_FILTER_NOT_SET;
	}

	NET_DBG("Attached all mcast filter %d", filter_id);

	return filter_id;
}
#endif /*CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR*/

static int can_enable(const struct device *dev, bool enable)
{
	struct net_can_context *ctx = dev->data;

	if (enable) {
		if (ctx->recv_filter_id == CAN_NET_FILTER_NOT_SET) {
			ctx->recv_filter_id = can_attach_unicast_filter(ctx);
			if (ctx->recv_filter_id < 0) {
				return -EIO;
			}
		}

#ifdef CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR
		if (ctx->eth_bridge_filter_id == CAN_NET_FILTER_NOT_SET) {
			ctx->eth_bridge_filter_id = can_attach_eth_bridge_filter(ctx);
			if (ctx->eth_bridge_filter_id < 0) {
				can_detach(ctx->can_dev, ctx->recv_filter_id);
				return -EIO;
			}
		}

		if (ctx->all_mcast_filter_id == CAN_NET_FILTER_NOT_SET) {
			ctx->all_mcast_filter_id = can_attach_all_mcast_filter(ctx);
			if (ctx->all_mcast_filter_id < 0) {
				can_detach(ctx->can_dev, ctx->recv_filter_id);
				can_detach(ctx->can_dev, ctx->eth_bridge_filter_id);
				return -EIO;
			}
		}
#endif /*CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR*/
	} else {
		if (ctx->recv_filter_id != CAN_NET_FILTER_NOT_SET) {
			can_detach(ctx->can_dev, ctx->recv_filter_id);
		}

#ifdef CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR
		if (ctx->eth_bridge_filter_id != CAN_NET_FILTER_NOT_SET) {
			can_detach(ctx->can_dev, ctx->eth_bridge_filter_id);
		}

		if (ctx->all_mcast_filter_id != CAN_NET_FILTER_NOT_SET) {
			can_detach(ctx->can_dev, ctx->all_mcast_filter_id);
		}
#endif /*CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR*/
	}

	return 0;
}

static struct net_can_api net_can_api_inst = {
	.iface_api.init = net_can_iface_init,

	.send = net_can_send,
	.attach_filter = can_attach_filter,
	.detach_filter = can_detach_filter,
	.enable = can_enable,
};

static int net_can_init(const struct device *dev)
{
	const struct device *can_dev;
	struct net_can_context *ctx = dev->data;

	can_dev = device_get_binding(DT_CHOSEN_ZEPHYR_CAN_PRIMARY_LABEL);

	ctx->recv_filter_id = CAN_NET_FILTER_NOT_SET;
#ifdef CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR
	ctx->eth_bridge_filter_id = CAN_NET_FILTER_NOT_SET;
	ctx->all_mcast_filter_id = CAN_NET_FILTER_NOT_SET;
#endif /*CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR*/

	if (!can_dev) {
		NET_ERR("Can't get binding to CAN device %s",
			DT_CHOSEN_ZEPHYR_CAN_PRIMARY_LABEL);
		return -EIO;
	}

	NET_DBG("Init net CAN device %p (%s) for dev %p (%s)",
		dev, dev->name, can_dev, can_dev->name);

	ctx->can_dev = can_dev;

	return 0;
}

static struct net_can_context net_can_context_1;

NET_DEVICE_INIT(net_can_1, CONFIG_CAN_NET_NAME, net_can_init,
		device_pm_control_nop, &net_can_context_1, NULL,
		CONFIG_CAN_NET_INIT_PRIORITY,
		&net_can_api_inst,
		CANBUS_L2, NET_L2_GET_CTX_TYPE(CANBUS_L2), NET_CAN_MTU);
