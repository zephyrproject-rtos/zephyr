/*
 * Copyright (c) 2019 Alexander Wachter.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_l2_canbus, CONFIG_NET_L2_CANBUS_LOG_LEVEL);

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/net_pkt.h>
#include <net/can.h>
#include "canbus_internal.h"
#include <6lo.h>
#include <timeout_q.h>
#include <string.h>
#include <misc/byteorder.h>
#include <net/ethernet.h>
#include <net/net_ip.h>
#include <string.h>

#define NET_CAN_WFTMAX 2
#define NET_CAN_ALLOC_TIMEOUT K_MSEC(100)

/* Minimal separation time betwee frames */
#define NET_CAN_STMIN CONFIG_NET_L2_CANBUS_STMIN
#define NET_CAN_BS CONFIG_NET_L2_CANBUS_BS

#define NET_CAN_DAD_SEND_RETRY 5
#define NET_CAN_DAD_TIMEOUT K_MSEC(100)

extern u16_t net_calc_chksum(struct net_pkt *pkt, u8_t proto);

static struct canbus_l2_ctx l2_ctx;

static struct k_work_q net_canbus_workq;
K_THREAD_STACK_DEFINE(net_canbus_stack, 512);

char *net_sprint_addr(sa_family_t af, const void *addr);

#if CONFIG_NET_L2_CANBUS_LOG_LEVEL >= LOG_LEVEL_DBG
static void canbus_print_ip_hdr(struct net_ipv6_hdr *ip_hdr)
{
	u8_t version = (ip_hdr->vtc >> 4);
	u8_t tc = ((ip_hdr->vtc & 0x0F) << 4) | ((ip_hdr->tcflow & 0xF0 >> 4));
	u32_t flow = ((ip_hdr->tcflow & 0x0F) << 16) | ip_hdr->flow;

	NET_DBG("IP header: Version: 0x%x, TC: 0x%x, Flow Label: 0x%x, "
		"Payload Length: %u, Next Header: 0x%x, Hop Limit: %u, "
		"Src: %s, Dest: %s",
		version, tc, flow, ntohs(ip_hdr->len), ip_hdr->nexthdr,
		ip_hdr->hop_limit,
		log_strdup(net_sprint_addr(AF_INET6, &ip_hdr->src)),
		log_strdup(net_sprint_addr(AF_INET6, &ip_hdr->dst)));
}
#else
#define canbus_print_ip_hdr(...)
#endif

static void canbus_free_tx_ctx(struct canbus_isotp_tx_ctx *ctx)
{
	k_mutex_lock(&l2_ctx.tx_ctx_mtx, K_FOREVER);
	ctx->state = NET_CAN_TX_STATE_UNUSED;
	k_mutex_unlock(&l2_ctx.tx_ctx_mtx);
}

static void canbus_free_rx_ctx(struct canbus_isotp_rx_ctx *ctx)
{
	k_mutex_lock(&l2_ctx.rx_ctx_mtx, K_FOREVER);
	ctx->state = NET_CAN_RX_STATE_UNUSED;
	k_mutex_unlock(&l2_ctx.rx_ctx_mtx);
}

static void canbus_tx_finish(struct net_pkt *pkt)
{
	struct canbus_isotp_tx_ctx *ctx = pkt->canbus_tx_ctx;

	if (ctx->state != NET_CAN_TX_STATE_RESET) {
		z_abort_timeout(&ctx->timeout);
	}

	canbus_free_tx_ctx(ctx);
	net_pkt_unref(pkt);
	k_sem_give(&l2_ctx.tx_sem);
}

static void canbus_rx_finish(struct net_pkt *pkt)
{
	struct canbus_isotp_rx_ctx *ctx = pkt->canbus_rx_ctx;

	canbus_free_rx_ctx(ctx);
}

static void canbus_tx_report_err(struct net_pkt *pkt)
{
	canbus_tx_finish(pkt);
}

static void canbus_rx_report_err(struct net_pkt *pkt)
{
	canbus_rx_finish(pkt);
	net_pkt_unref(pkt);
}

static void rx_err_work_handler(struct k_work *item)
{
	struct net_pkt *pkt = CONTAINER_OF(item, struct net_pkt, work);

	canbus_rx_report_err(pkt);
}

static void canbus_rx_report_err_from_isr(struct net_pkt *pkt)
{
	k_work_init(&pkt->work, rx_err_work_handler);
	k_work_submit_to_queue(&net_canbus_workq, &pkt->work);
}

static void canbus_tx_timeout(struct _timeout *t)
{
	struct canbus_isotp_tx_ctx *ctx =
		CONTAINER_OF(t, struct canbus_isotp_tx_ctx, timeout);

	NET_ERR("TX Timeout. CTX: %p", ctx);
	ctx->state = NET_CAN_TX_STATE_ERR;
	k_work_submit_to_queue(&net_canbus_workq, &ctx->pkt->work);
}

static void canbus_rx_timeout(struct _timeout *t)
{
	struct canbus_isotp_rx_ctx *ctx =
		CONTAINER_OF(t, struct canbus_isotp_rx_ctx, timeout);

	NET_ERR("RX Timeout. CTX: %p", ctx);
	ctx->state = NET_CAN_RX_STATE_TIMEOUT;
	canbus_rx_report_err_from_isr(ctx->pkt);
}

static void canbus_st_min_timeout(struct _timeout *t)
{
	struct canbus_isotp_tx_ctx *ctx =
		CONTAINER_OF(t, struct canbus_isotp_tx_ctx, timeout);

	k_work_submit_to_queue(&net_canbus_workq, &ctx->pkt->work);
}

static s32_t canbus_stmin_to_ticks(u8_t stmin)
{
	s32_t time_ms;

	/* According to ISO 15765-2 stmin should be 127ms if value is corrupt */
	if (stmin > NET_CAN_STMIN_MAX ||
	    (stmin > NET_CAN_STMIN_MS_MAX && stmin < NET_CAN_STMIN_US_BEGIN)) {
		time_ms = K_MSEC(NET_CAN_STMIN_MS_MAX);
	} else if (stmin >= NET_CAN_STMIN_US_BEGIN) {
		/* This should be 100us-900us but zephyr can't handle that */
		time_ms = K_MSEC(1);
	} else {
		time_ms = stmin;
	}

	return z_ms_to_ticks(time_ms);
}

static u16_t canbus_get_lladdr(struct net_linkaddr *net_lladdr)
{
	NET_ASSERT(net_lladdr->len == sizeof(u16_t));

	return sys_be16_to_cpu(UNALIGNED_GET((u16_t *)net_lladdr->addr));
}

static u16_t canbus_get_src_lladdr(struct net_pkt *pkt)
{
	return net_pkt_lladdr_src(pkt)->type == NET_LINK_CANBUS ?
	       canbus_get_lladdr(net_pkt_lladdr_src(pkt)) :
	       NET_CAN_ETH_TRANSLATOR_ADDR;
}

static u16_t canbus_get_dest_lladdr(struct net_pkt *pkt)
{
	return net_pkt_lladdr_dst(pkt)->type == NET_LINK_CANBUS &&
	       net_pkt_lladdr_dst(pkt)->len == sizeof(struct net_canbus_lladdr) ?
	       canbus_get_lladdr(net_pkt_lladdr_dst(pkt)) :
	       NET_CAN_ETH_TRANSLATOR_ADDR;
}

static inline bool canbus_dest_is_mcast(struct net_pkt *pkt)
{
	u16_t lladdr_be = UNALIGNED_GET((u16_t *)net_pkt_lladdr_dst(pkt)->addr);

	return (sys_be16_to_cpu(lladdr_be) & CAN_NET_IF_IS_MCAST_BIT);
}

static bool canbus_src_is_translator(struct net_pkt *pkt)
{
	return ((canbus_get_src_lladdr(pkt) & CAN_NET_IF_ADDR_MASK) ==
		NET_CAN_ETH_TRANSLATOR_ADDR);
}

static bool canbus_dest_is_translator(struct net_pkt *pkt)
{
	return (net_pkt_lladdr_dst(pkt)->type == NET_LINK_ETHERNET ||
		net_pkt_lladdr_dst(pkt)->len == sizeof(struct net_eth_addr));
}

#if defined(CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR)
static bool canbus_is_for_translator(struct net_pkt *pkt)
{
	return ((net_pkt_lladdr_dst(pkt)->type == NET_LINK_CANBUS) &&
		(canbus_get_lladdr(net_pkt_lladdr_dst(pkt)) ==
		 NET_CAN_ETH_TRANSLATOR_ADDR));
}
#else
#define canbus_is_for_translator(...) false
#endif /* CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR */

static size_t canbus_total_lladdr_len(struct net_pkt *pkt)
{
	/* This pkt will be farowarded to Ethernet
	 * Destination MAC is carried inline, source is going to be extended
	 */
	if (IS_ENABLED(CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR) &&
	    canbus_is_for_translator(pkt)) {
		return sizeof(struct net_eth_addr) +
		       sizeof(struct net_canbus_lladdr);
	}

	return 2U * sizeof(struct net_canbus_lladdr);
}

static inline void canbus_cpy_lladdr(struct net_pkt *dst, struct net_pkt *src)
{
	struct net_linkaddr *lladdr;

	lladdr = net_pkt_lladdr_dst(dst);
	lladdr->addr = net_pkt_cursor_get_pos(dst);
	net_pkt_write(dst, net_pkt_lladdr_dst(src)->addr,
		      sizeof(struct net_canbus_lladdr));
	lladdr->len = sizeof(struct net_canbus_lladdr);
	lladdr->type = NET_LINK_CANBUS;

	if (IS_ENABLED(CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR) &&
	    canbus_is_for_translator(src)) {
		/* Make room for address extension */
		net_pkt_skip(dst, sizeof(struct net_eth_addr) -
			     sizeof(struct net_canbus_lladdr));
	}

	lladdr = net_pkt_lladdr_src(dst);
	lladdr->addr = net_pkt_cursor_get_pos(dst);

	if (canbus_src_is_translator(src)) {
		net_pkt_copy(dst, src, sizeof(struct net_eth_addr));
		lladdr->len = sizeof(struct net_eth_addr);
		lladdr->type = NET_LINK_ETHERNET;
		NET_DBG("Inline MAC: %02x:%02x:%02x:%02x:%02x:%02x",
			lladdr->addr[0], lladdr->addr[1], lladdr->addr[2],
			lladdr->addr[3], lladdr->addr[4], lladdr->addr[5]);
	} else {
		net_pkt_write(dst, net_pkt_lladdr_src(src)->addr,
			      sizeof(struct net_canbus_lladdr));
		lladdr->len = sizeof(struct net_canbus_lladdr);
		lladdr->type = NET_LINK_CANBUS;
	}
}


static struct canbus_isotp_rx_ctx *canbus_get_rx_ctx(u8_t state,
						     u16_t src_addr)
{
	int i;
	struct canbus_isotp_rx_ctx *ret = NULL;

	k_mutex_lock(&l2_ctx.rx_ctx_mtx, K_FOREVER);
	for (i = 0; i < ARRAY_SIZE(l2_ctx.rx_ctx); i++) {
		struct canbus_isotp_rx_ctx *ctx = &l2_ctx.rx_ctx[i];

		if (ctx->state == state) {
			if (state == NET_CAN_RX_STATE_UNUSED) {
				ctx->state = NET_CAN_RX_STATE_RESET;
				z_init_timeout(&ctx->timeout);
				ret = ctx;
				break;
			}

			if (canbus_get_src_lladdr(ctx->pkt) == src_addr) {
				ret = ctx;
				break;
			}
		}
	}

	k_mutex_unlock(&l2_ctx.rx_ctx_mtx);
	return ret;
}

static struct canbus_isotp_tx_ctx *canbus_get_tx_ctx(u8_t state,
						     u16_t dest_addr)
{
	int i;
	struct canbus_isotp_tx_ctx *ret = NULL;

	k_mutex_lock(&l2_ctx.tx_ctx_mtx, K_FOREVER);
	for (i = 0; i < ARRAY_SIZE(l2_ctx.tx_ctx); i++) {
		struct canbus_isotp_tx_ctx *ctx = &l2_ctx.tx_ctx[i];

		if (ctx->state == state) {
			if (state == NET_CAN_TX_STATE_UNUSED) {
				ctx->state = NET_CAN_TX_STATE_RESET;
				z_init_timeout(&ctx->timeout);
				ret = ctx;
				break;
			}

			if (ctx->dest_addr.addr == dest_addr) {
				ret = ctx;
				break;
			}
		}
	}

	k_mutex_unlock(&l2_ctx.tx_ctx_mtx);
	return ret;
}

static inline u16_t canbus_receive_get_ff_length(struct net_pkt *pkt)
{
	u16_t len;
	int ret;

	ret = net_pkt_read_be16(pkt, &len);
	if (ret < 0) {
		NET_ERR("Can't read length");
	}

	return len & 0x0FFF;
}

static inline size_t canbus_get_sf_length(struct net_pkt *pkt)
{
	size_t len;

	net_buf_pull_u8(pkt->frags);
	len = net_buf_pull_u8(pkt->frags);

	return len;
}

static inline void canbus_set_frame_datalength(struct zcan_frame *frame,
					       u8_t length)
{
	/* TODO: Needs update when CAN FD support is added */
	NET_ASSERT(length <= NET_CAN_DL);
	frame->dlc = length;
}

static enum net_verdict canbus_finish_pkt(struct net_pkt *pkt)
{
	/* Pull the ll addresses to ignore them in upper layers */
	net_buf_pull(pkt->buffer, net_pkt_lladdr_dst(pkt)->len +
		     net_pkt_lladdr_src(pkt)->len);

	if (IS_ENABLED(CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR) &&
	    canbus_is_for_translator(pkt)) {
		/* Pull room for address extension */
		net_buf_pull(pkt->buffer, sizeof(struct net_eth_addr) -
			     net_pkt_lladdr_src(pkt)->len);
		/* Set the destination address to the inline MAC and pull it */
		net_pkt_cursor_init(pkt);
		net_pkt_lladdr_dst(pkt)->addr = net_pkt_cursor_get_pos(pkt);
		net_pkt_lladdr_dst(pkt)->type = NET_LINK_ETHERNET;
		net_pkt_lladdr_dst(pkt)->len = sizeof(struct net_eth_addr);
		net_buf_pull(pkt->buffer, sizeof(struct net_eth_addr));
	}

	net_pkt_cursor_init(pkt);
	if (!net_6lo_uncompress(pkt)) {
		NET_ERR("6lo uncompression failed");
		return NET_DROP;
	}

	net_pkt_cursor_init(pkt);

	return NET_CONTINUE;
}

static inline u32_t canbus_addr_to_id(u16_t dest, u16_t src)
{
	return (dest << CAN_NET_IF_ADDR_DEST_POS) |
	       (src << CAN_NET_IF_ADDR_SRC_POS);
}

static void canbus_set_frame_addr(struct zcan_frame *frame,
				  const struct net_canbus_lladdr *dest,
				  const struct net_canbus_lladdr *src,
				  bool mcast)
{
	frame->id_type = CAN_EXTENDED_IDENTIFIER;
	frame->rtr = CAN_DATAFRAME;

	frame->ext_id = canbus_addr_to_id(dest->addr, src->addr);

	if (mcast) {
		frame->ext_id |= CAN_NET_IF_ADDR_MCAST_MASK;
	}
}

static void canbus_set_frame_addr_pkt(struct zcan_frame *frame,
				      struct net_pkt *pkt,
				      struct net_canbus_lladdr *dest_addr,
				      bool mcast)
{
	struct net_canbus_lladdr src_addr;

	if (IS_ENABLED(CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR) &&
	    net_pkt_lladdr_src(pkt)->type == NET_LINK_ETHERNET) {
		src_addr.addr = NET_CAN_ETH_TRANSLATOR_ADDR;
	} else {
		src_addr.addr = canbus_get_lladdr(net_if_get_link_addr(pkt->iface));
	}

	canbus_set_frame_addr(frame, dest_addr, &src_addr, mcast);
}

static void canbus_fc_send_cb(u32_t err_flags, void *arg)
{
	if (err_flags) {
		NET_ERR("Sending FC frame failed: %d", err_flags);
	}
}

static int canbus_send_fc(struct device *net_can_dev,
			  struct net_canbus_lladdr *dest,
			  struct net_canbus_lladdr *src, u8_t fs)
{
	const struct net_can_api *api = net_can_dev->driver_api;
	struct zcan_frame frame = {
		.id_type = CAN_EXTENDED_IDENTIFIER,
		.rtr = CAN_DATAFRAME,
	};

	NET_ASSERT(!(fs & NET_CAN_PCI_TYPE_MASK));

	canbus_set_frame_addr(&frame, dest, src, false);

	frame.data[0] = NET_CAN_PCI_TYPE_FC | fs;
	/* BS (Block Size) */
	frame.data[1] = NET_CAN_BS;
	/* STmin (minimum Seperation Time) */
	frame.data[2] = NET_CAN_STMIN;
	canbus_set_frame_datalength(&frame, 3);

	NET_DBG("Sending FC to ID: 0x%08x", frame.ext_id);
	return api->send(net_can_dev, &frame, canbus_fc_send_cb, NULL,
			 K_FOREVER);
}

static int canbus_process_cf_data(struct net_pkt *frag_pkt,
				  struct canbus_isotp_rx_ctx *ctx)
{
	struct net_pkt *pkt = ctx->pkt;
	size_t data_len = net_pkt_get_len(frag_pkt) - 1;
	u8_t pci;
	int ret;

	pci = net_buf_pull_u8(frag_pkt->frags);

	if ((pci & NET_CAN_PCI_SN_MASK) != ctx->sn) {
		NET_ERR("Sequence number missmatch. Expect %u, got %u",
			ctx->sn, pci & NET_CAN_PCI_SN_MASK);
		goto err;
	}

	ctx->sn++;

	if (data_len > ctx->rem_len) {
		NET_DBG("Remove padding of %d bytes", data_len - ctx->rem_len);
		data_len = ctx->rem_len;
	}

	net_pkt_cursor_init(frag_pkt);
	NET_DBG("Appending CF data to pkt (%d bytes)", data_len);
	ret = net_pkt_copy(pkt, frag_pkt, data_len);
	if (ret < 0) {
		NET_ERR("Failed to write data to pkt [%d]", ret);
		goto err;
	}

	ctx->rem_len -= data_len;

	NET_DBG("%u bytes remaining", ctx->rem_len);

	return 0;
err:
	canbus_rx_report_err(pkt);
	return -1;
}

static enum net_verdict canbus_process_cf(struct net_pkt *pkt)
{
	struct canbus_isotp_rx_ctx *rx_ctx;
	enum net_verdict ret;
	struct device *net_can_dev;
	struct net_canbus_lladdr src, dest;
	bool mcast;

	mcast = canbus_dest_is_mcast(pkt);

	rx_ctx = canbus_get_rx_ctx(NET_CAN_RX_STATE_CF,
				   canbus_get_src_lladdr(pkt));
	if (!rx_ctx) {
		NET_INFO("Got CF but can't find a CTX that is waiting for it. "
			 "Src: 0x%04x", canbus_get_src_lladdr(pkt));
		return NET_DROP;
	}

	z_abort_timeout(&rx_ctx->timeout);

	ret = canbus_process_cf_data(pkt, rx_ctx);
	if (ret < 0) {
		return NET_DROP;
	}

	net_pkt_unref(pkt);

	if (rx_ctx->rem_len == 0) {
		rx_ctx->state = NET_CAN_RX_STATE_FIN;
		ret = net_recv_data(pkt->iface, rx_ctx->pkt);
		if (ret < 0) {
			NET_ERR("Packet dropped by NET stack");
			net_pkt_unref(pkt);
		}
	} else {
		z_add_timeout(&rx_ctx->timeout, canbus_rx_timeout,
			      z_ms_to_ticks(NET_CAN_BS_TIME));

		if (NET_CAN_BS != 0 && !mcast) {
			rx_ctx->act_block_nr++;
			if (rx_ctx->act_block_nr >= NET_CAN_BS) {
				NET_DBG("BS reached. Send FC");
				src.addr = canbus_get_src_lladdr(pkt);
				dest.addr = canbus_get_dest_lladdr(pkt);
				net_can_dev = net_if_get_device(pkt->iface);
				ret = canbus_send_fc(net_can_dev, &src, &dest,
						     NET_CAN_PCI_FS_CTS);
				if (ret) {
					NET_ERR("Failed to send FC CTS. BS: %d",
						NET_CAN_BS);
					canbus_rx_report_err(rx_ctx->pkt);
					return NET_OK;
				}

				rx_ctx->act_block_nr = 0;
			}
		}
	}

	return NET_OK;
}

static enum net_verdict canbus_process_ff(struct net_pkt *pkt)
{
	struct device *net_can_dev = net_if_get_device(pkt->iface);
	struct canbus_isotp_rx_ctx *rx_ctx = NULL;
	struct net_pkt *new_pkt = NULL;
	int ret;
	struct net_canbus_lladdr src, dest;
	u16_t msg_len;
	size_t new_pkt_len;
	u8_t data_len;
	bool mcast;

	mcast = canbus_dest_is_mcast(pkt);
	src.addr = canbus_get_src_lladdr(pkt);
	dest.addr = canbus_get_dest_lladdr(pkt);
	net_pkt_cursor_init(pkt);

	msg_len = canbus_receive_get_ff_length(pkt);

	new_pkt_len = msg_len + canbus_total_lladdr_len(pkt);

	new_pkt = net_pkt_rx_alloc_with_buffer(pkt->iface, new_pkt_len,
					       AF_INET6, 0,
					       NET_CAN_ALLOC_TIMEOUT);
	if (!new_pkt) {
		NET_ERR("Failed to obtain net_pkt with size of %d", new_pkt_len);

		if (!mcast) {
			canbus_send_fc(net_can_dev, &src, &dest,
				       NET_CAN_PCI_FS_OVFLW);
		}

		goto err;
	}

	rx_ctx = canbus_get_rx_ctx(NET_CAN_RX_STATE_UNUSED, 0);
	if (!rx_ctx) {
		NET_ERR("No rx context left");

		if (!mcast) {
			canbus_send_fc(net_can_dev, &src, &dest,
				       NET_CAN_PCI_FS_OVFLW);
		}

		goto err;
	}

	rx_ctx->act_block_nr = 0;
	rx_ctx->pkt = new_pkt;
	new_pkt->canbus_rx_ctx = rx_ctx;

	net_pkt_cursor_init(new_pkt);
	data_len = net_pkt_remaining_data(pkt);
	canbus_cpy_lladdr(new_pkt, pkt);
	rx_ctx->sn = 1;

	ret = net_pkt_copy(new_pkt, pkt, net_pkt_remaining_data(pkt));
	if (ret) {
		NET_ERR("Failed to write to pkt [%d]", ret);
		goto err;
	}

	rx_ctx->rem_len = msg_len - data_len;
	net_pkt_unref(pkt);

	if (!mcast) {
		/* switch src and dest because we are answering */
		ret = canbus_send_fc(net_can_dev, &src, &dest,
				     NET_CAN_PCI_FS_CTS);
		if (ret) {
			NET_ERR("Failed to send FC CTS");
			canbus_rx_report_err(new_pkt);
			return NET_OK;
		}
	}

	/* At this point we expect to get Consecutive frames directly */
	z_add_timeout(&rx_ctx->timeout, canbus_rx_timeout,
		      z_ms_to_ticks(NET_CAN_BS_TIME));

	rx_ctx->state = NET_CAN_RX_STATE_CF;

	NET_DBG("Processed FF from 0x%04x (%scast)"
		"Msg length: %u CTX: %p",
		src.addr, mcast ? "m" : "uni", msg_len, rx_ctx);

	return NET_OK;

err:
	if (new_pkt) {
		net_pkt_unref(new_pkt);
	}

	if (rx_ctx) {
		canbus_free_rx_ctx(rx_ctx);
	}

	return NET_DROP;
}

static enum net_verdict canbus_process_sf(struct net_pkt *pkt)
{
	size_t data_len;
	size_t pkt_len;

	net_pkt_set_family(pkt, AF_INET6);

	data_len = canbus_get_sf_length(pkt);
	pkt_len = net_pkt_get_len(pkt);

	if (data_len > pkt_len) {
		NET_ERR("SF datalen > pkt size");
		return NET_DROP;
	}

	if (pkt_len != data_len) {
		NET_DBG("Remove padding (%d byte)", pkt_len - data_len);
		net_pkt_update_length(pkt, data_len);
	}

	return canbus_finish_pkt(pkt);
}

static void canbus_tx_frame_isr(u32_t err_flags, void *arg)
{
	struct net_pkt *pkt = (struct net_pkt *)arg;
	struct canbus_isotp_tx_ctx *ctx = pkt->canbus_tx_ctx;

	ctx->tx_backlog--;

	if (ctx->state == NET_CAN_TX_STATE_WAIT_TX_BACKLOG) {
		if (ctx->tx_backlog > 0) {
			return;
		}

		ctx->state = NET_CAN_TX_STATE_FIN;
	}

	k_work_submit_to_queue(&net_canbus_workq, &pkt->work);
}

static inline int canbus_send_cf(struct net_pkt *pkt)
{
	struct canbus_isotp_tx_ctx *ctx = pkt->canbus_tx_ctx;
	struct device *net_can_dev = net_if_get_device(pkt->iface);
	const struct net_can_api *api = net_can_dev->driver_api;
	struct zcan_frame frame;
	struct net_pkt_cursor cursor_backup;
	int ret, len;

	canbus_set_frame_addr_pkt(&frame, pkt, &ctx->dest_addr, ctx->is_mcast);

	/* sn wraps around at 0xF automatically because it has a 4 bit size */
	frame.data[0] = NET_CAN_PCI_TYPE_CF | ctx->sn;

	len = MIN(ctx->rem_len, NET_CAN_DL - 1);

	canbus_set_frame_datalength(&frame, len + 1);

	net_pkt_cursor_backup(pkt, &cursor_backup);
	net_pkt_read(pkt, &frame.data[1], len);
	ret = api->send(net_can_dev, &frame, canbus_tx_frame_isr,
			pkt, K_NO_WAIT);
	if (ret == CAN_TX_OK) {
		ctx->sn++;
		ctx->rem_len -= len;
		ctx->act_block_nr--;
		ctx->tx_backlog++;
	} else {
		net_pkt_cursor_restore(pkt, &cursor_backup);
	}

	NET_DBG("CF sent. %d bytes left. CTX: %p", ctx->rem_len, ctx);

	return ret ? ret : ctx->rem_len;
}

static void canbus_tx_work(struct net_pkt *pkt)
{
	int ret;
	struct canbus_isotp_tx_ctx *ctx = pkt->canbus_tx_ctx;

	NET_ASSERT(ctx);

	switch (ctx->state) {
	case NET_CAN_TX_STATE_SEND_CF:
		do {
			ret = canbus_send_cf(ctx->pkt);
			if (!ret) {
				ctx->state = NET_CAN_TX_STATE_WAIT_TX_BACKLOG;
				break;
			}

			if (ret < 0 && ret != CAN_TIMEOUT) {
				NET_ERR("Failed to send CF. CTX: %p", ctx);
				canbus_tx_report_err(pkt);
				break;
			}

			if (ctx->opts.bs && !ctx->is_mcast &&
			    !ctx->act_block_nr) {
				NET_DBG("BS reached. Wait for FC again. CTX: %p",
					ctx);
				ctx->state = NET_CAN_TX_STATE_WAIT_FC;
				z_add_timeout(&ctx->timeout, canbus_tx_timeout,
					      z_ms_to_ticks(NET_CAN_BS_TIME));
				break;
			} else if (ctx->opts.stmin) {
				ctx->state = NET_CAN_TX_STATE_WAIT_ST;
				break;
			}
		} while (ret > 0);

		break;

	case NET_CAN_TX_STATE_WAIT_ST:
		NET_DBG("SM wait ST. CTX: %p", ctx);
		z_add_timeout(&ctx->timeout, canbus_st_min_timeout,
			      z_ms_to_ticks(canbus_stmin_to_ticks(ctx->opts.stmin)));
		ctx->state = NET_CAN_TX_STATE_SEND_CF;
		break;

	case NET_CAN_TX_STATE_ERR:
		NET_DBG("SM handle error. CTX: %p", ctx);
		canbus_tx_report_err(pkt);
		break;

	case NET_CAN_TX_STATE_FIN:
		canbus_tx_finish(ctx->pkt);
		NET_DBG("SM finish. CTX: %p", ctx);
		break;

	default:
		break;
	}
}

static void canbus_tx_work_handler(struct k_work *item)
{
	struct net_pkt *pkt = CONTAINER_OF(item, struct net_pkt, work);

	canbus_tx_work(pkt);
}

static enum net_verdict canbus_process_fc_data(struct canbus_isotp_tx_ctx *ctx,
					       struct net_pkt *pkt)
{
	struct net_buf *buf = pkt->frags;
	u8_t pci;

	pci = net_buf_pull_u8(buf);

	switch (pci & NET_CAN_PCI_FS_MASK) {
	case NET_CAN_PCI_FS_CTS:
		if (net_buf_frags_len(buf) != 2) {
			NET_ERR("Frame length error for CTS");
			canbus_tx_report_err(pkt);
			return NET_DROP;
		}

		ctx->state = NET_CAN_TX_STATE_SEND_CF;
		ctx->wft = 0;
		ctx->opts.bs = net_buf_pull_u8(buf);
		ctx->opts.stmin = net_buf_pull_u8(buf);
		ctx->act_block_nr = ctx->opts.bs;
		z_abort_timeout(&ctx->timeout);
		NET_DBG("Got CTS. BS: %d, STmin: %d. CTX: %p",
			ctx->opts.bs, ctx->opts.stmin, ctx);
		net_pkt_unref(pkt);
		return NET_OK;
	case NET_CAN_PCI_FS_WAIT:
		NET_DBG("Got WAIT frame. CTX: %p", ctx);
		z_abort_timeout(&ctx->timeout);
		z_add_timeout(&ctx->timeout, canbus_tx_timeout,
			      z_ms_to_ticks(NET_CAN_BS_TIME));
		if (ctx->wft >= NET_CAN_WFTMAX) {
			NET_INFO("Got to many wait frames. CTX: %p", ctx);
			ctx->state = NET_CAN_TX_STATE_ERR;
		}

		ctx->wft++;
		return NET_OK;
	case NET_CAN_PCI_FS_OVFLW:
		NET_ERR("Got overflow FC frame. CTX: %p", ctx);
		ctx->state = NET_CAN_TX_STATE_ERR;
		return NET_OK;
	default:
		NET_ERR("Invalid Frame Status. CTX: %p", ctx);
		ctx->state = NET_CAN_TX_STATE_ERR;
		break;
	}

	return NET_DROP;
}

static enum net_verdict canbus_process_fc(struct net_pkt *pkt)
{
	struct canbus_isotp_tx_ctx *tx_ctx;
	u16_t src_addr = canbus_get_src_lladdr(pkt);
	enum net_verdict ret;

	tx_ctx = canbus_get_tx_ctx(NET_CAN_TX_STATE_WAIT_FC, src_addr);
	if (!tx_ctx) {
		NET_WARN("Got FC frame from 0x%04x but can't find any "
			 "CTX waiting for it", src_addr);
		return NET_DROP;
	}

	ret = canbus_process_fc_data(tx_ctx, pkt);
	if (ret == NET_OK) {
		k_work_submit_to_queue(&net_canbus_workq, &tx_ctx->pkt->work);
	}

	return ret;
}

static inline int canbus_send_ff(struct net_pkt *pkt, size_t len, bool mcast,
				 struct net_canbus_lladdr *dest_addr)
{
	struct device *net_can_dev = net_if_get_device(pkt->iface);
	const struct net_can_api *api = net_can_dev->driver_api;
	struct net_linkaddr *lladdr_inline;
	struct zcan_frame frame;
	int ret, index = 0;

	canbus_set_frame_addr_pkt(&frame, pkt, dest_addr, mcast);
	canbus_set_frame_datalength(&frame, NET_CAN_DL);

	if (mcast) {
		NET_DBG("Sending FF (multicast). ID: 0x%08x. PKT len: %zu"
			" CTX: %p",
			frame.ext_id, len, pkt->canbus_tx_ctx);
	} else {
		NET_DBG("Sending FF (unicast). ID: 0x%08x. PKT len: %zu"
			" CTX: %p",
			frame.ext_id, len, pkt->canbus_tx_ctx);
	}

#if defined(CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR)
	NET_ASSERT(mcast || !(canbus_dest_is_translator(pkt) &&
			      canbus_src_is_translator(pkt)));

	if (canbus_src_is_translator(pkt)) {
		len += net_pkt_lladdr_src(pkt)->len;
	}
#endif
	if (!mcast && canbus_dest_is_translator(pkt)) {
		len += net_pkt_lladdr_dst(pkt)->len;
	}

	frame.data[index++] = NET_CAN_PCI_TYPE_FF | (len >> 8);
	frame.data[index++] = len & 0xFF;

	/* According to ISO, FF has sn 0 and is incremented to one
	 * alltough it's not part of the FF frame
	 */
	pkt->canbus_tx_ctx->sn = 1;

	if (!mcast && canbus_dest_is_translator(pkt)) {
		lladdr_inline = net_pkt_lladdr_dst(pkt);
		memcpy(&frame.data[index], lladdr_inline->addr,
		       lladdr_inline->len);
		index += lladdr_inline->len;
	}

	if (IS_ENABLED(CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR) &&
	    net_pkt_lladdr_src(pkt)->type == NET_LINK_ETHERNET) {
		lladdr_inline = net_pkt_lladdr_src(pkt);
		memcpy(&frame.data[index], lladdr_inline->addr,
		       lladdr_inline->len);
		index += lladdr_inline->len;
	}

	net_pkt_read(pkt, &frame.data[index], NET_CAN_DL - index);
	pkt->canbus_tx_ctx->rem_len -= NET_CAN_DL - index;

	ret = api->send(net_can_dev, &frame, NULL, NULL, K_FOREVER);
	if (ret != CAN_TX_OK) {
		NET_ERR("Sending FF failed [%d]. CTX: %p",
			ret, pkt->canbus_tx_ctx);
	}

	return ret;
}

static inline int canbus_send_single_frame(struct net_pkt *pkt, size_t len,
					   bool mcast,
					   struct net_canbus_lladdr *dest_addr)
{
	struct device *net_can_dev = net_if_get_device(pkt->iface);
	const struct net_can_api *api = net_can_dev->driver_api;
	int index = 0;
	struct zcan_frame frame;
	struct net_linkaddr *lladdr_dest;
	int ret;

	canbus_set_frame_addr_pkt(&frame, pkt, dest_addr, mcast);

	frame.data[index++] = NET_CAN_PCI_TYPE_SF;
	frame.data[index++] = len;

	NET_ASSERT((len + (!mcast && canbus_dest_is_translator(pkt)) ?
		    net_pkt_lladdr_dst(pkt)->len : 0) <= NET_CAN_DL - 1);

	if (!mcast && canbus_dest_is_translator(pkt)) {
		lladdr_dest = net_pkt_lladdr_dst(pkt);
		memcpy(&frame.data[index], lladdr_dest->addr, lladdr_dest->len);
		index += lladdr_dest->len;
	}

	net_pkt_read(pkt, &frame.data[index], len);

	canbus_set_frame_datalength(&frame, len + index);

	ret = api->send(net_can_dev, &frame, NULL, NULL, K_FOREVER);
	if (ret != CAN_TX_OK) {
		NET_ERR("Sending SF failed [%d]", ret);
		return -EIO;
	}

	return 0;
}

static void canbus_start_sending_cf(struct _timeout *t)
{
	struct canbus_isotp_tx_ctx *ctx =
		CONTAINER_OF(t, struct canbus_isotp_tx_ctx, timeout);

	k_work_submit_to_queue(&net_canbus_workq, &ctx->pkt->work);
}

static int canbus_send_multiple_frames(struct net_pkt *pkt, size_t len,
				       bool mcast,
				       struct net_canbus_lladdr *dest_addr)
{
	struct canbus_isotp_tx_ctx *tx_ctx = NULL;
	int ret;

	tx_ctx = canbus_get_tx_ctx(NET_CAN_TX_STATE_UNUSED, 0);

	if (!tx_ctx) {
		NET_ERR("No tx context left");
		k_sem_give(&l2_ctx.tx_sem);
		return -EAGAIN;
	}

	tx_ctx->pkt = pkt;
	pkt->canbus_tx_ctx = tx_ctx;
	tx_ctx->is_mcast = mcast;
	tx_ctx->dest_addr = *dest_addr;
	tx_ctx->rem_len = net_pkt_get_len(pkt);
	tx_ctx->tx_backlog = 0;

	k_work_init(&pkt->work, canbus_tx_work_handler);

	ret = canbus_send_ff(pkt, len, mcast, dest_addr);
	if (ret != CAN_TX_OK) {
		NET_ERR("Failed to send FF [%d]", ret);
		canbus_tx_report_err(pkt);
		return -EIO;
	}

	if (!mcast) {
		z_add_timeout(&tx_ctx->timeout, canbus_tx_timeout,
			      z_ms_to_ticks(NET_CAN_BS_TIME));
		tx_ctx->state = NET_CAN_TX_STATE_WAIT_FC;
	} else {
		tx_ctx->state = NET_CAN_TX_STATE_SEND_CF;
		z_add_timeout(&tx_ctx->timeout, canbus_start_sending_cf,
			      z_ms_to_ticks(NET_CAN_FF_CF_TIME));
	}

	return 0;
}

static void canbus_ipv6_mcast_to_dest(struct net_pkt *pkt,
				      struct net_canbus_lladdr *dest_addr)
{
	dest_addr->addr =
		sys_be16_to_cpu(UNALIGNED_GET(&NET_IPV6_HDR(pkt)->dst.s6_addr16[7]));
}

static inline u16_t canbus_eth_to_can_addr(struct net_linkaddr *lladdr)
{
	return (sys_be16_to_cpu(UNALIGNED_GET((u16_t *)&lladdr->addr[4])) &
		CAN_NET_IF_ADDR_MASK);
}

static int canbus_send(struct net_if *iface, struct net_pkt *pkt)
{
	int ret = 0;
	int comp_len;
	size_t pkt_len, inline_lladdr_len;
	struct net_canbus_lladdr dest_addr;
	bool mcast;

	if (net_pkt_family(pkt) != AF_INET6) {
		return -EINVAL;
	}

	mcast = net_ipv6_is_addr_mcast(&NET_IPV6_HDR(pkt)->dst);
	if (mcast || canbus_dest_is_mcast(pkt)) {
		canbus_ipv6_mcast_to_dest(pkt, &dest_addr);
	} else if (IS_ENABLED(CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR) &&
		   net_pkt_lladdr_dst(pkt)->type == NET_LINK_ETHERNET) {
		struct net_linkaddr *lladdr = net_pkt_lladdr_dst(pkt);

		lladdr->type = NET_LINK_CANBUS;
		lladdr->len = sizeof(struct net_canbus_lladdr);
		dest_addr.addr = canbus_eth_to_can_addr(net_pkt_lladdr_dst(pkt));
		NET_DBG("Translated %02x:%02x:%02x:%02x:%02x:%02x to 0x%04x",
			lladdr->addr[0], lladdr->addr[1], lladdr->addr[2],
			lladdr->addr[3], lladdr->addr[4], lladdr->addr[5],
			dest_addr.addr);
	} else {
		dest_addr.addr = canbus_get_dest_lladdr(pkt);
	}

	net_pkt_cursor_init(pkt);
	canbus_print_ip_hdr((struct net_ipv6_hdr *)net_pkt_cursor_get_pos(pkt));
	comp_len = net_6lo_compress(pkt, true);
	if (comp_len < 0) {
		NET_ERR("IPHC failed [%d]", comp_len);
		return comp_len;
	}

	NET_DBG("IPv6 hdr compressed by %d bytes", comp_len);
	net_pkt_cursor_init(pkt);
	pkt_len = net_pkt_get_len(pkt);

	NET_DBG("Send CAN frame to 0x%04x%s", dest_addr.addr,
		mcast ? " (mcast)" : "");

	inline_lladdr_len = (!mcast && canbus_dest_is_translator(pkt)) ?
			    net_pkt_lladdr_dst(pkt)->len : 0;

	if ((pkt_len + inline_lladdr_len) > (NET_CAN_DL - 1)) {
		k_sem_take(&l2_ctx.tx_sem, K_FOREVER);
		ret = canbus_send_multiple_frames(pkt, pkt_len, mcast,
						  &dest_addr);
	} else {
		ret = canbus_send_single_frame(pkt, pkt_len, mcast, &dest_addr);
		canbus_tx_finish(pkt);
	}

	return ret;
}

static enum net_verdict canbus_process_frame(struct net_pkt *pkt)
{
	enum net_verdict ret = NET_DROP;
	u8_t pci_type;

	net_pkt_cursor_init(pkt);
	ret = net_pkt_read_u8(pkt, &pci_type);
	if (ret < 0) {
		NET_ERR("Can't read PCI");
	}
	pci_type = (pci_type & NET_CAN_PCI_TYPE_MASK) >> NET_CAN_PCI_TYPE_POS;

	switch (pci_type) {
	case NET_CAN_PCI_SF:
		ret = canbus_process_sf(pkt);
		break;
	case NET_CAN_PCI_FF:
		ret = canbus_process_ff(pkt);
		break;
	case NET_CAN_PCI_CF:
		ret = canbus_process_cf(pkt);
		break;
	case NET_CAN_PCI_FC:
		ret = canbus_process_fc(pkt);
		break;
	default:
		NET_ERR("Unknown PCI number %u", pci_type);
		break;
	}

	return ret;
}

#if defined(CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR)
static void forward_eth_frame(struct net_pkt *pkt, struct net_if *canbus_iface)
{
	pkt->iface = canbus_iface;
	net_if_queue_tx(canbus_iface, pkt);
}

static struct net_ipv6_hdr *get_ip_hdr_from_eth_frame(struct net_pkt *pkt)
{
	return (struct net_ipv6_hdr *)((u8_t *)net_pkt_data(pkt) +
				       sizeof(struct net_eth_hdr));
}

enum net_verdict net_canbus_translate_eth_frame(struct net_if *iface,
						struct net_pkt *pkt)
{
	struct net_linkaddr *lladdr = net_pkt_lladdr_dst(pkt);
	struct net_pkt *clone_pkt;
	struct net_if *canbus_iface;

	/* Forward only IPv6 frames */
	if ((get_ip_hdr_from_eth_frame(pkt)->vtc & 0xf0) != 0x60) {
		return NET_CONTINUE;
	}

	/* This frame is for the Ethernet interface itself */
	if (net_linkaddr_cmp(net_if_get_link_addr(iface), lladdr)) {
		NET_DBG("Frame is for Ethernet only %02x:%02x:%02x:%02x:%02x:%02x",
			lladdr->addr[0], lladdr->addr[1], lladdr->addr[2],
			lladdr->addr[3], lladdr->addr[4], lladdr->addr[5]);
		return NET_CONTINUE;
	}

	canbus_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(CANBUS));

	net_pkt_cursor_init(pkt);
	/* Forward all broadcasts */
	if (net_eth_is_addr_broadcast((struct net_eth_addr *)lladdr->addr) ||
	    net_eth_is_addr_multicast((struct net_eth_addr *)lladdr->addr)) {
		if (!canbus_iface || !net_if_is_up(canbus_iface)) {
			NET_ERR("No canbus iface");
			return NET_CONTINUE;
		}

		clone_pkt = net_pkt_shallow_clone(pkt, NET_CAN_ALLOC_TIMEOUT);
		if (clone_pkt) {
			NET_DBG("Frame is %scast %02x:%02x:%02x:%02x:%02x:%02x,",
				net_eth_is_addr_broadcast(
					(struct net_eth_addr *)lladdr->addr) ? "broad" :
				"multi",
				lladdr->addr[0], lladdr->addr[1], lladdr->addr[2],
				lladdr->addr[3], lladdr->addr[4], lladdr->addr[5]);
			net_pkt_set_family(clone_pkt, AF_INET6);
			forward_eth_frame(clone_pkt, canbus_iface);
		} else {
			NET_ERR("PKT forwarding: cloning failed");
		}

		return NET_CONTINUE;
	}

	if (!canbus_iface || !net_if_is_up(canbus_iface)) {
		NET_ERR("No canbus iface");
		return NET_DROP;
	}

	/* This frame is for 6LoCAN only */
	net_pkt_set_family(pkt, AF_INET6);
	net_buf_pull(pkt->buffer, sizeof(struct net_eth_hdr));
	forward_eth_frame(pkt, canbus_iface);
	NET_DBG("Frame is for CANBUS: 0x%04x", canbus_get_dest_lladdr(pkt));

	return NET_OK;
}

static void forward_can_frame(struct net_pkt *pkt, struct net_if *eth_iface)
{
	net_pkt_set_iface(pkt, eth_iface);
	net_if_queue_tx(eth_iface, pkt);
}

static void rewrite_icmp_hdr(struct net_pkt *pkt, struct net_icmp_hdr *icmp_hdr)
{
	int ret;

	net_pkt_cursor_init(pkt);
	net_pkt_skip(pkt, sizeof(struct net_ipv6_hdr));
	ret = net_icmpv6_create(pkt, icmp_hdr->type, icmp_hdr->code);
	if (ret) {
		NET_ERR("Can't create ICMP HDR");
		return;
	}

	net_pkt_cursor_init(pkt);
	net_pkt_skip(pkt, sizeof(struct net_ipv6_hdr));
	ret = net_icmpv6_finalize(pkt);
	if (ret) {
		NET_ERR("Can't finalize ICMP HDR");
	}
}

static void extend_llao(struct net_pkt *pkt, struct net_linkaddr *mac_addr)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access, struct net_icmp_hdr);
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_opt_access,
					      struct net_icmpv6_nd_opt_hdr);
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(llao_access,
					      struct net_eth_addr);
	struct net_pkt_cursor cursor_backup;
	struct net_icmp_hdr *icmp_hdr;
	struct net_icmpv6_nd_opt_hdr *icmp_opt_hdr;
	u8_t *llao, llao_backup[2];
	int ret;

	net_pkt_cursor_backup(pkt, &cursor_backup);
	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);
	net_pkt_skip(pkt, sizeof(struct net_ipv6_hdr));

	if (net_calc_chksum(pkt, IPPROTO_ICMPV6) != 0U) {
		NET_ERR("Invalid checksum");
		return;
	}

	icmp_hdr = (struct net_icmp_hdr *)net_pkt_get_data(pkt, &icmp_access);
	if (!icmp_hdr) {
		NET_ERR("No ICMP6 HDR");
		goto done;
	}

	switch (icmp_hdr->type) {

	case NET_ICMPV6_NS:
		net_pkt_skip(pkt, sizeof(struct net_icmpv6_ns_hdr));
		NET_DBG("Extend NS SLLAO");
		break;

	case NET_ICMPV6_NA:
		net_pkt_skip(pkt, sizeof(struct net_icmpv6_na_hdr));
		NET_DBG("Extend NA TLLAO");
		break;

	case NET_ICMPV6_RS:
		net_pkt_skip(pkt, sizeof(struct net_icmpv6_rs_hdr));
		NET_DBG("Extend RS SLLAO");
		break;

	case NET_ICMPV6_RA:
		net_pkt_skip(pkt, sizeof(struct net_icmpv6_ra_hdr));
		NET_DBG("Extend RA SLLAO");
		break;

	default:
		goto done;
	}

	net_pkt_acknowledge_data(pkt, &icmp_access);

	icmp_opt_hdr = (struct net_icmpv6_nd_opt_hdr *)
		       net_pkt_get_data(pkt, &icmp_opt_access);
	if (!icmp_opt_hdr) {
		NET_DBG("No LLAO opt to extend");
		goto done;
	}

	net_pkt_acknowledge_data(pkt, &icmp_opt_access);

	if (icmp_opt_hdr->type != NET_ICMPV6_ND_OPT_SLLAO &&
	    (icmp_hdr->type == NET_ICMPV6_NA &&
	     icmp_opt_hdr->type != NET_ICMPV6_ND_OPT_TLLAO)) {
		NET_DBG("opt was not LLAO");
		goto done;
	}

	if (icmp_opt_hdr->len != 1) {
		NET_ERR("LLAO len is %u. This should be 1 for 6LoCAN",
			icmp_opt_hdr->len);
		goto done;
	}

	llao = (u8_t *)net_pkt_get_data(pkt, &llao_access);
	if (!llao) {
		NET_ERR("Can't read LLAO");
		goto done;
	}

	memcpy(llao_backup, llao, sizeof(struct net_canbus_lladdr));
	memcpy(llao, mac_addr->addr, mac_addr->len);

	llao[4] = (llao[4] & 0xC0) | llao_backup[0];
	llao[5] = llao_backup[1];

	ret = net_pkt_set_data(pkt, &llao_access);
	if (ret < 0) {
		NET_ERR("Failed to write MAC to LLAO [%d]", ret);
		goto done;
	}

	rewrite_icmp_hdr(pkt, icmp_hdr);

	NET_DBG("LLAO extended to %02x:%02x:%02x:%02x:%02x:%02x",
		llao[0], llao[1], llao[2], llao[3], llao[4], llao[5]);

done:
	net_pkt_cursor_restore(pkt, &cursor_backup);
}

static bool pkt_is_icmp(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	struct net_ipv6_hdr *ipv6_hdr =
		(struct net_ipv6_hdr *)net_pkt_get_data(pkt, &ipv6_access);

	if (!ipv6_hdr) {
		NET_ERR("No IPv6 HDR");
		return false;
	}

	return (ipv6_hdr->nexthdr == IPPROTO_ICMPV6);
}

static void swap_scr_lladdr(struct net_pkt *pkt, struct net_pkt *pkt_clone)
{
	struct net_linkaddr *lladdr_origin = net_pkt_lladdr_src(pkt);
	struct net_linkaddr *lladdr_clone = net_pkt_lladdr_src(pkt_clone);
	size_t offset;

	offset = lladdr_origin->addr - pkt->buffer->data;
	lladdr_clone->addr = pkt_clone->buffer->data + offset;
}

static void can_to_eth_lladdr(struct net_pkt *pkt, struct net_if *eth_iface,
			      bool bcast)
{
	u16_t src_can_addr = canbus_get_src_lladdr(pkt);
	struct net_linkaddr *lladdr_src = net_pkt_lladdr_src(pkt);
	struct net_linkaddr *lladdr_dst;

	if (bcast) {
		lladdr_dst = net_pkt_lladdr_dst(pkt);
		lladdr_dst->len = sizeof(struct net_eth_addr);
		lladdr_dst->type = NET_LINK_ETHERNET;
		lladdr_dst->addr = (u8_t *)net_eth_broadcast_addr()->addr;
	}

	lladdr_src->addr = net_pkt_lladdr_src(pkt)->addr -
			   (sizeof(struct net_eth_addr) - lladdr_src->len);
	memcpy(lladdr_src->addr, net_if_get_link_addr(eth_iface)->addr,
	       sizeof(struct net_eth_addr));
	lladdr_src->addr[4] = (lladdr_src->addr[4] & 0xC0) | (src_can_addr >> 8U);
	lladdr_src->addr[5] = src_can_addr & 0xFF;
	lladdr_src->len = sizeof(struct net_eth_addr);
	lladdr_src->type = NET_LINK_ETHERNET;
}

void translate_to_eth_frame(struct net_pkt *pkt, bool is_bcast,
			    struct net_if *eth_iface)
{
	struct net_linkaddr *dest_addr = net_pkt_lladdr_dst(pkt);
	struct net_linkaddr *src_addr = net_pkt_lladdr_src(pkt);
	bool is_icmp;

	is_icmp = pkt_is_icmp(pkt);

	can_to_eth_lladdr(pkt, eth_iface, is_bcast);
	canbus_print_ip_hdr((struct net_ipv6_hdr *)net_pkt_cursor_get_pos(pkt));
	NET_DBG("Forward frame to %02x:%02x:%02x:%02x:%02x:%02x. "
		"Src: %02x:%02x:%02x:%02x:%02x:%02x",
		dest_addr->addr[0], dest_addr->addr[1], dest_addr->addr[2],
		dest_addr->addr[3], dest_addr->addr[4], dest_addr->addr[5],
		src_addr->addr[0], src_addr->addr[1], src_addr->addr[2],
		src_addr->addr[3], src_addr->addr[4], src_addr->addr[5]);

	if (is_icmp) {
		extend_llao(pkt, net_if_get_link_addr(eth_iface));
	}
}

static enum net_verdict canbus_forward_to_eth(struct net_pkt *pkt)
{
	struct net_pkt *pkt_clone;
	struct net_if *eth_iface;

	eth_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	if (!eth_iface || !net_if_is_up(eth_iface)) {
		NET_ERR("No Ethernet iface available");
		if (canbus_is_for_translator(pkt)) {
			return NET_DROP;
		} else {
			return NET_CONTINUE;
		}
	}

	if (canbus_dest_is_mcast(pkt)) {
		/* net_pkt_clone can't be called on a pkt where
		 * net_buf_pull was called on. We need to clone
		 * first and then finish the pkt.
		 */
		pkt_clone = net_pkt_clone(pkt, NET_CAN_ALLOC_TIMEOUT);
		if (pkt_clone) {
			swap_scr_lladdr(pkt, pkt_clone);
			canbus_finish_pkt(pkt_clone);
			translate_to_eth_frame(pkt_clone, true, eth_iface);
			forward_can_frame(pkt_clone, eth_iface);
			NET_DBG("Len: %zu", net_pkt_get_len(pkt_clone));
		} else {
			NET_ERR("Failed to clone pkt");
		}
	}

	canbus_finish_pkt(pkt);

	if (net_pkt_lladdr_dst(pkt)->type == NET_LINK_ETHERNET) {
		translate_to_eth_frame(pkt, false, eth_iface);
		forward_can_frame(pkt, eth_iface);
		return NET_OK;
	}

	return NET_CONTINUE;
}
#else
#define canbus_forward_to_eth(...) 0
#endif /*CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR*/

static enum net_verdict canbus_recv(struct net_if *iface,
				    struct net_pkt *pkt)
{
	struct net_linkaddr *lladdr = net_pkt_lladdr_src(pkt);
	enum net_verdict ret = NET_DROP;

	if (pkt->canbus_rx_ctx) {
		if (lladdr->len == sizeof(struct net_canbus_lladdr)) {
			NET_DBG("Push reassembled packet from 0x%04x trough "
				"stack again", canbus_get_src_lladdr(pkt));
		} else {
			NET_DBG("Push reassembled packet from "
				"%02x:%02x:%02x:%02x:%02x:%02x trough stack again",
				lladdr->addr[0], lladdr->addr[1], lladdr->addr[2],
				lladdr->addr[3], lladdr->addr[4], lladdr->addr[5]);
		}

		if (pkt->canbus_rx_ctx->state == NET_CAN_RX_STATE_FIN) {
			canbus_rx_finish(pkt);

			if (IS_ENABLED(CONFIG_NET_L2_CANBUS_ETH_TRANSLATOR)) {
				ret = canbus_forward_to_eth(pkt);
			} else {
				canbus_finish_pkt(pkt);
				canbus_print_ip_hdr(NET_IPV6_HDR(pkt));
				ret = NET_CONTINUE;
			}
		} else {
			NET_ERR("Expected pkt in FIN state");
		}
	} else {
		ret = canbus_process_frame(pkt);
	}

	return ret;
}

static inline int canbus_send_dad_request(struct device *net_can_dev,
					  struct net_canbus_lladdr *ll_addr)
{
	const struct net_can_api *api = net_can_dev->driver_api;
	struct zcan_frame frame;
	int ret;

	canbus_set_frame_datalength(&frame, 0);
	frame.rtr = CAN_REMOTEREQUEST;
	frame.id_type = CAN_EXTENDED_IDENTIFIER;
	frame.ext_id = canbus_addr_to_id(ll_addr->addr,
					 sys_rand32_get() & CAN_NET_IF_ADDR_MASK);

	ret = api->send(net_can_dev, &frame, NULL, NULL, K_FOREVER);
	if (ret != CAN_TX_OK) {
		NET_ERR("Sending DAD request failed [%d]", ret);
		return -EIO;
	}

	return 0;
}

static void canbus_send_dad_resp_cb(u32_t err_flags, void *cb_arg)
{
	static u8_t fail_cnt;
	struct k_work *work = (struct k_work *)cb_arg;

	if (err_flags) {
		NET_ERR("Failed to send dad response [%u]", err_flags);
		if (err_flags != CAN_TX_BUS_OFF &&
		    fail_cnt < NET_CAN_DAD_SEND_RETRY) {
			k_work_submit_to_queue(&net_canbus_workq, work);
		}

		fail_cnt++;
	} else {
		fail_cnt = 0;
	}
}

static inline void canbus_send_dad_response(struct k_work *item)
{
	struct canbus_net_ctx *ctx = CONTAINER_OF(item, struct canbus_net_ctx,
						  dad_work);
	struct net_if *iface = ctx->iface;
	struct net_linkaddr *ll_addr = net_if_get_link_addr(iface);
	struct device *net_can_dev = net_if_get_device(iface);
	const struct net_can_api *api = net_can_dev->driver_api;
	struct zcan_frame frame;
	int ret;

	canbus_set_frame_datalength(&frame, 0);
	frame.rtr = CAN_DATAFRAME;
	frame.id_type = CAN_EXTENDED_IDENTIFIER;
	frame.ext_id = canbus_addr_to_id(NET_CAN_DAD_ADDR,
					 ntohs(UNALIGNED_GET((u16_t *) ll_addr->addr)));

	ret = api->send(net_can_dev, &frame, canbus_send_dad_resp_cb, item,
			K_FOREVER);
	if (ret != CAN_TX_OK) {
		NET_ERR("Sending SF failed [%d]", ret);
	} else {
		NET_INFO("DAD response sent");
	}
}

static inline void canbus_detach_filter(struct device *net_can_dev,
					int filter_id)
{
	const struct net_can_api *api = net_can_dev->driver_api;

	api->detach_filter(net_can_dev, filter_id);
}

static void canbus_dad_resp_cb(struct zcan_frame *frame, void *arg)
{
	struct k_sem *dad_sem = (struct k_sem *)arg;

	k_sem_give(dad_sem);
}

static inline
int canbus_attach_dad_resp_filter(struct device *net_can_dev,
				  struct net_canbus_lladdr *ll_addr,
				  struct k_sem *dad_sem)
{
	const struct net_can_api *api = net_can_dev->driver_api;
	struct zcan_filter filter = {
		.id_type = CAN_EXTENDED_IDENTIFIER,
		.rtr = CAN_DATAFRAME,
		.rtr_mask = 1,
		.ext_id_mask = CAN_EXT_ID_MASK
	};
	int filter_id;

	filter.ext_id = canbus_addr_to_id(NET_CAN_DAD_ADDR, ll_addr->addr);

	filter_id = api->attach_filter(net_can_dev, canbus_dad_resp_cb,
				       dad_sem, &filter);
	if (filter_id == CAN_NO_FREE_FILTER) {
		NET_ERR("Can't attach dad response filter");
	}

	return filter_id;
}

static void canbus_dad_request_cb(struct zcan_frame *frame, void *arg)
{
	struct k_work *work = (struct k_work *)arg;

	k_work_submit_to_queue(&net_canbus_workq, work);
}

static inline int canbus_attach_dad_filter(struct device *net_can_dev,
					   struct net_canbus_lladdr *ll_addr,
					   struct k_work *dad_work)
{
	const struct net_can_api *api = net_can_dev->driver_api;
	struct zcan_filter filter = {
		.id_type = CAN_EXTENDED_IDENTIFIER,
		.rtr = CAN_REMOTEREQUEST,
		.rtr_mask = 1,
		.ext_id_mask = (CAN_NET_IF_ADDR_MASK << CAN_NET_IF_ADDR_DEST_POS)
	};
	int filter_id;

	filter.ext_id = canbus_addr_to_id(ll_addr->addr, 0);

	filter_id = api->attach_filter(net_can_dev, canbus_dad_request_cb,
				       dad_work, &filter);
	if (filter_id == CAN_NO_FREE_FILTER) {
		NET_ERR("Can't attach dad filter");
	}

	return filter_id;
}

static inline int canbus_init_ll_addr(struct net_if *iface)
{
	struct canbus_net_ctx *ctx = net_if_l2_data(iface);
	struct device *net_can_dev = net_if_get_device(iface);
	int dad_resp_filter_id = CAN_NET_FILTER_NOT_SET;
	struct net_canbus_lladdr ll_addr;
	int ret;
	struct k_sem dad_sem;

#if defined(CONFIG_NET_L2_CANBUS_USE_FIXED_ADDR)
	ll_addr.addr = CONFIG_NET_L2_CANBUS_FIXED_ADDR;
#else
	do {
		ll_addr.addr = sys_rand32_get() % (NET_CAN_MAX_ADDR + 1);
	} while (ll_addr.addr < NET_CAN_MIN_ADDR);
#endif

	/* Add address early for DAD response */
	ctx->ll_addr = sys_cpu_to_be16(ll_addr.addr);
	net_if_set_link_addr(iface, (u8_t *)&ctx->ll_addr, sizeof(ll_addr),
			     NET_LINK_CANBUS);

	dad_resp_filter_id = canbus_attach_dad_resp_filter(net_can_dev, &ll_addr,
							   &dad_sem);
	if (dad_resp_filter_id < 0) {
		return -EIO;
	}
	/*
	 * Attach this filter now to defend this address instantly.
	 * This filter is not called for own DAD because loopback is not
	 * enabled.
	 */
	ctx->dad_filter_id = canbus_attach_dad_filter(net_can_dev, &ll_addr,
						      &ctx->dad_work);
	if (ctx->dad_filter_id < 0) {
		ret = -EIO;
		goto dad_err;
	}

	k_sem_init(&dad_sem, 0, 1);
	ret = canbus_send_dad_request(net_can_dev, &ll_addr);
	if (ret) {
		ret = -EIO;
		goto dad_err;
	}

	ret = k_sem_take(&dad_sem, NET_CAN_DAD_TIMEOUT);
	canbus_detach_filter(net_can_dev, dad_resp_filter_id);
	dad_resp_filter_id = CAN_NET_FILTER_NOT_SET;

	if (ret != -EAGAIN) {
		NET_INFO("DAD failed");
		ret = -EAGAIN;
		goto dad_err;
	}

	return 0;

dad_err:
	net_if_set_link_addr(iface, NULL, 0, NET_LINK_CANBUS);
	if (ctx->dad_filter_id != CAN_NET_FILTER_NOT_SET) {
		canbus_detach_filter(net_can_dev, ctx->dad_filter_id);
		ctx->dad_filter_id = CAN_NET_FILTER_NOT_SET;
	}

	if (dad_resp_filter_id != CAN_NET_FILTER_NOT_SET) {
		canbus_detach_filter(net_can_dev, dad_resp_filter_id);
	}

	return ret;
}

void net_6locan_init(struct net_if *iface)
{
	struct canbus_net_ctx *ctx = net_if_l2_data(iface);
	u8_t thread_priority;
	int i;

	NET_DBG("Init CAN net interface");

	for (i = 0; i < ARRAY_SIZE(l2_ctx.tx_ctx); i++) {
		l2_ctx.tx_ctx[i].state = NET_CAN_TX_STATE_UNUSED;
	}

	for (i = 0; i < ARRAY_SIZE(l2_ctx.rx_ctx); i++) {
		l2_ctx.rx_ctx[i].state = NET_CAN_RX_STATE_UNUSED;
	}

	ctx->dad_filter_id = CAN_NET_FILTER_NOT_SET;
	ctx->iface = iface;
	k_work_init(&ctx->dad_work, canbus_send_dad_response);

	k_mutex_init(&l2_ctx.tx_ctx_mtx);
	k_mutex_init(&l2_ctx.rx_ctx_mtx);
	k_sem_init(&l2_ctx.tx_sem, 1, INT_MAX);

	/* This work queue should have precedence over the tx stream
	 * TODO thread_priority = tx_tc2thread(NET_TC_TX_COUNT -1) - 1;
	 */
	thread_priority = 6;

	k_work_q_start(&net_canbus_workq, net_canbus_stack,
		       K_THREAD_STACK_SIZEOF(net_canbus_stack),
		       K_PRIO_COOP(thread_priority));
	k_thread_name_set(&net_canbus_workq.thread, "isotp_work");
	NET_DBG("Workq started. Thread ID: %p", &net_canbus_workq.thread);
}

static int canbus_enable(struct net_if *iface, bool state)
{
	struct device *net_can_dev = net_if_get_device(iface);
	const struct net_can_api *api = net_can_dev->driver_api;
	struct canbus_net_ctx *ctx = net_if_l2_data(iface);
	int dad_retry_cnt, ret;

	NET_DBG("start to bring iface %p %s", iface, state ? "up" : "down");

	if (state) {
		for (dad_retry_cnt = CONFIG_NET_L2_CANBUS_DAD_RETRIES;
		     dad_retry_cnt; dad_retry_cnt--) {
			ret = canbus_init_ll_addr(iface);
			if (ret == 0) {
				break;
			} else if (ret == -EIO) {
				return -EIO;
			}
		}

		if (ret != 0) {
			return ret;
		}

	} else {
		if (ctx->dad_filter_id != CAN_NET_FILTER_NOT_SET) {
			canbus_detach_filter(net_can_dev, ctx->dad_filter_id);
		}
	}

	ret = api->enable(net_can_dev, state);
	if (!ret) {
		NET_DBG("Iface %p is up", iface);
	}

	return ret;
}

static enum net_l2_flags canbus_net_flags(struct net_if *iface)
{
	return NET_L2_MULTICAST;
}

NET_L2_INIT(CANBUS_L2, canbus_recv, canbus_send, canbus_enable,
	    canbus_net_flags);
