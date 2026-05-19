/** @file
 * @brief LLDP related functions
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_lldp, CONFIG_NET_LLDP_LOG_LEVEL);

#include <errno.h>
#include <stdlib.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/lldp.h>

static struct net_mgmt_event_callback cb;

static void lldp_tx_timeout(struct k_work *work);
/* Have only one timer in order to save memory */
static K_WORK_DELAYABLE_DEFINE(lldp_tx_timer, lldp_tx_timeout);

/* Track currently active timers */
static sys_slist_t lldp_ifaces;

#define BUF_ALLOC_TIMEOUT K_MSEC(50)

static void lldp_submit_work(k_timeout_t timeout)
{
	k_work_reschedule(&lldp_tx_timer, timeout);

	NET_DBG("Next wakeup in %d ms",
		k_ticks_to_ms_ceil32(
			k_work_delayable_remaining_get(&lldp_tx_timer)));
}

static void lldp_send(struct ethernet_lldp *lldp)
{
	static const struct net_eth_addr lldp_multicast_eth_addr = {
		{ 0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e }
	};
	struct ethernet_context *ctx = CONTAINER_OF(lldp, struct ethernet_context, lldp);
	int ret = 0;
	struct net_pkt *pkt;
	size_t len;

	if (!lldp->lldpdu) {
		/* The ethernet driver has not set the lldpdu pointer */
		NET_DBG("The LLDPDU is not set for lldp %p", lldp);
		return;
	}

	if (lldp->optional_du && lldp->optional_len) {
		len = sizeof(struct net_lldpdu) + lldp->optional_len;
	} else {
		len = sizeof(struct net_lldpdu);
	}

	if (IS_ENABLED(CONFIG_NET_LLDP_END_LLDPDU_TLV_ENABLED)) {
		len += sizeof(uint16_t);
	}

	pkt = net_pkt_alloc_with_buffer(ctx->iface, len, NET_AF_UNSPEC, 0,
					BUF_ALLOC_TIMEOUT);
	if (!pkt) {
		return;
	}

	net_pkt_set_lldp(pkt, true);
	net_pkt_set_ll_proto_type(pkt, NET_ETH_PTYPE_LLDP);

	ret = net_pkt_write(pkt, (uint8_t *)lldp->lldpdu,
			    sizeof(struct net_lldpdu));
	if (ret < 0) {
		net_pkt_unref(pkt);
		return;
	}

	if (lldp->optional_du && lldp->optional_len) {
		ret = net_pkt_write(pkt, (uint8_t *)lldp->optional_du,
				    lldp->optional_len);
		if (ret < 0) {
			net_pkt_unref(pkt);
			return;
		}
	}

	if (IS_ENABLED(CONFIG_NET_LLDP_END_LLDPDU_TLV_ENABLED)) {
		uint16_t tlv_end = net_htons(NET_LLDP_END_LLDPDU_VALUE);

		ret = net_pkt_write(pkt, (uint8_t *)&tlv_end, sizeof(tlv_end));
		if (ret < 0) {
			net_pkt_unref(pkt);
			return;
		}
	}

	(void)net_linkaddr_copy(net_pkt_lladdr_src(pkt),
				net_if_get_link_addr(ctx->iface));

	(void)net_linkaddr_set(net_pkt_lladdr_dst(pkt),
			       (uint8_t *)lldp_multicast_eth_addr.addr,
			       sizeof(struct net_eth_addr));

	/* send without timeout, so we do not risk being blocked by tx when
	 * being flooded
	 */
	if (net_if_try_send_data(ctx->iface, pkt, K_NO_WAIT) == NET_DROP) {
		net_pkt_unref(pkt);
		return;
	}

	NET_DBG("LLDP packet sent from iface %p", ctx->iface);

	return;
}

static void lldp_tx_timeout(struct k_work *work __unused)
{
	k_timepoint_t timeout_update = sys_timepoint_calc(K_FOREVER);
	k_timeout_t timeout;
	struct ethernet_lldp *current, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&lldp_ifaces, current, next, node) {
		if (sys_timepoint_expired(current->tx_timer_timeout)) {
			lldp_send(current);

			current->tx_timer_timeout =
				sys_timepoint_calc(K_SECONDS(CONFIG_NET_LLDP_TX_INTERVAL));
		}

		if (sys_timepoint_cmp(current->tx_timer_timeout, timeout_update) < 0) {
			timeout_update = current->tx_timer_timeout;
		}
	}

	timeout = sys_timepoint_timeout(timeout_update);
	if (!K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		lldp_submit_work(timeout);
	}
}

static void lldp_start_timer(struct ethernet_context *ctx)
{
	k_timeout_t timeout = K_NO_WAIT;

	/* exit if started */
	if (sys_slist_find(&lldp_ifaces, &(ctx->lldp.node), NULL)) {
		return;
	}

	ctx->lldp.tx_timer_timeout = sys_timepoint_calc(timeout);

	sys_slist_append(&lldp_ifaces, &(ctx->lldp.node));

	lldp_submit_work(timeout);
}

static int lldp_check_iface(struct net_if *iface)
{
	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return -ENOENT;
	}

	if (!(net_eth_get_hw_capabilities(iface) & ETHERNET_LLDP)) {
		return -ESRCH;
	}

	return 0;
}

static void lldp_start(struct net_if *iface, uint64_t mgmt_event)
{
	struct ethernet_context *ctx;

	if (lldp_check_iface(iface) < 0) {
		return;
	}

	ctx = net_if_l2_data(iface);

	if (mgmt_event == NET_EVENT_IF_DOWN) {
		NET_DBG("Stopping timer for iface %p", iface);
		(void)sys_slist_find_and_remove(&lldp_ifaces, &(ctx->lldp.node));

		if (sys_slist_is_empty(&lldp_ifaces)) {
			k_work_cancel_delayable(&lldp_tx_timer);
		}
	} else if (mgmt_event == NET_EVENT_IF_UP) {
		NET_DBG("Starting timer for iface %p", iface);
		lldp_start_timer(ctx);
	}

	return;
}

static enum net_verdict net_lldp_recv(struct net_if *iface, uint16_t ptype, struct net_pkt *pkt)
{
	struct ethernet_context *ctx;
	net_lldp_recv_cb_t recv_cb;
	int ret;

	ARG_UNUSED(ptype);

	if (!net_eth_is_addr_lldp_multicast(
		    (struct net_eth_addr *)net_pkt_lladdr_dst(pkt)->addr)) {
		return NET_DROP;
	}

	ret = lldp_check_iface(iface);
	if (ret < 0) {
		return NET_DROP;
	}

	ctx = net_if_l2_data(iface);

	recv_cb = ctx->lldp.cb;
	if (recv_cb) {
		return recv_cb(iface, pkt);
	}

	return NET_DROP;
}

ETH_NET_L3_REGISTER(LLDP, NET_ETH_PTYPE_LLDP, net_lldp_recv);

int net_lldp_register_callback(struct net_if *iface, net_lldp_recv_cb_t recv_cb)
{
	struct ethernet_context *ctx;
	int ret;

	ret = lldp_check_iface(iface);
	if (ret < 0) {
		return ret;
	}

	ctx = net_if_l2_data(iface);

	ctx->lldp.cb = recv_cb;

	return 0;
}

static void iface_event_handler(struct net_mgmt_event_callback *evt_cb,
				uint64_t mgmt_event, struct net_if *iface)
{
	lldp_start(iface, mgmt_event);
}

static void iface_cb(struct net_if *iface, void *user_data)
{
	/* If the network interface is already up, then call the sender
	 * immediately. If the interface is not ethernet one, then
	 * lldp_start() will return immediately.
	 */
	if (net_if_oper_state(iface) == NET_IF_OPER_UP) {
		lldp_start(iface, NET_EVENT_IF_UP);
	}
}

int net_lldp_config(struct net_if *iface, const struct net_lldpdu *lldpdu)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	ctx->lldp.lldpdu = lldpdu;

	return 0;
}

int net_lldp_config_optional(struct net_if *iface, const uint8_t *tlv, size_t len)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);

	ctx->lldp.optional_du = tlv;
	ctx->lldp.optional_len = len;

	return 0;
}

static const struct net_lldpdu lldpdu = {
	.chassis_id = {
		.type_length = net_htons((LLDP_TLV_CHASSIS_ID << 9) |
			NET_LLDP_CHASSIS_ID_TLV_LEN),
		.subtype = CONFIG_NET_LLDP_CHASSIS_ID_SUBTYPE,
		.value = NET_LLDP_CHASSIS_ID_VALUE
	},
	.port_id = {
		.type_length = net_htons((LLDP_TLV_PORT_ID << 9) |
			NET_LLDP_PORT_ID_TLV_LEN),
		.subtype = CONFIG_NET_LLDP_PORT_ID_SUBTYPE,
		.value = NET_LLDP_PORT_ID_VALUE
	},
	.ttl = {
		.type_length = net_htons((LLDP_TLV_TTL << 9) |
			NET_LLDP_TTL_TLV_LEN),
		.ttl = net_htons(NET_LLDP_TTL)
	},
};

int net_lldp_set_lldpdu(struct net_if *iface)
{
	return net_lldp_config(iface, &lldpdu);
}

void net_lldp_unset_lldpdu(struct net_if *iface)
{
	net_lldp_config(iface, NULL);
	net_lldp_config_optional(iface, NULL, 0);
}

void net_lldp_init(void)
{
	net_mgmt_init_event_callback(&cb, iface_event_handler,
				     NET_EVENT_IF_UP | NET_EVENT_IF_DOWN);
	net_mgmt_add_event_callback(&cb);

	net_if_foreach(iface_cb, NULL);
}
