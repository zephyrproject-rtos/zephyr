/** @file
 * @brief LLDP related functions
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_LLDP)
#define SYS_LOG_DOMAIN "net/lldp"
#define NET_LOG_ENABLED 1
#endif

#include <errno.h>
#include <stdlib.h>

#include <net/net_core.h>
#include <net/ethernet.h>
#include <net/net_mgmt.h>
#include <net/lldp.h>

static struct net_mgmt_event_callback cb;

/* Have only one timer in order to save memory */
static struct k_delayed_work lldp_tx_timer;

/* Track currently active timers */
static sys_slist_t lldp_ifaces;

#define BUF_ALLOC_TIMEOUT K_MSEC(50)

static int lldp_find(struct ethernet_context *ctx, struct net_if *iface)
{
	int i, found = -1;

	for (i = 0; i < ARRAY_SIZE(ctx->lldp); i++) {
		if (ctx->lldp[i].iface == iface) {
			return i;
		}

		if (found < 0 && ctx->lldp[i].iface == NULL) {
			found = i;
		}
	}

	if (found >= 0) {
		ctx->lldp[found].iface = iface;
		return found;
	}

	return -ENOENT;
}

static void lldp_submit_work(u32_t timeout)
{
	if (!k_delayed_work_remaining_get(&lldp_tx_timer) ||
	    timeout < k_delayed_work_remaining_get(&lldp_tx_timer)) {
		k_delayed_work_cancel(&lldp_tx_timer);
		k_delayed_work_submit(&lldp_tx_timer, timeout);

		NET_DBG("Next wakeup in %d ms",
			k_delayed_work_remaining_get(&lldp_tx_timer));
	}
}

static bool lldp_check_timeout(s64_t start, u32_t time, s64_t timeout)
{
	start += time;
	start = abs(start);

	if (start > timeout) {
		return false;
	}

	return true;
}

static bool lldp_timedout(struct ethernet_lldp *lldp, s64_t timeout)
{
	return lldp_check_timeout(lldp->tx_timer_start,
				  lldp->tx_timer_timeout,
				  timeout);
}

static int lldp_send(struct ethernet_lldp *lldp)
{
	static const struct net_eth_addr lldp_multicast_eth_addr = {
		{ 0x01, 0x80, 0xc2, 0x00, 0x00, 0x0e }
	};
	int ret = 0;
	struct net_eth_hdr *hdr;
	struct net_pkt *pkt;
	struct net_buf *frag;
	u16_t pos;

	if (!lldp->lldpdu) {
		/* The ethernet driver has not set the lldpdu pointer */
		NET_DBG("The LLDPDU is not set for lldp %p", lldp);
		ret = -EINVAL;
		goto out;
	}

	pkt = net_pkt_get_reserve_tx(net_if_get_ll_reserve(lldp->iface,
							   NULL),
				     BUF_ALLOC_TIMEOUT);
	if (!pkt) {
		ret = -ENOMEM;
		goto out;
	}

	frag = net_pkt_get_frag(pkt, BUF_ALLOC_TIMEOUT);
	if (!frag) {
		net_pkt_unref(pkt);
		ret = -ENOMEM;
		goto out;
	}

	net_pkt_frag_add(pkt, frag);

	net_buf_add(frag, sizeof(struct net_lldpdu));

	if (!net_pkt_write(pkt, frag, 0, &pos, sizeof(struct net_lldpdu),
			   (u8_t *)lldp->lldpdu, BUF_ALLOC_TIMEOUT)) {
		net_pkt_unref(pkt);
		ret = -ENOMEM;
		goto out;
	}

	net_pkt_lladdr_src(pkt)->addr = net_if_get_link_addr(lldp->iface)->addr;
	net_pkt_lladdr_src(pkt)->len = sizeof(struct net_eth_addr);
	net_pkt_lladdr_dst(pkt)->addr = (u8_t *)lldp_multicast_eth_addr.addr;
	net_pkt_lladdr_dst(pkt)->len = sizeof(struct net_eth_addr);

	hdr = NET_ETH_HDR(pkt);
	hdr->type = htons(NET_ETH_PTYPE_LLDP);

	net_pkt_set_iface(pkt, lldp->iface);

	if (net_if_send_data(lldp->iface, pkt) == NET_DROP) {
		net_pkt_unref(pkt);
		ret = -EIO;
	}

out:
	lldp->tx_timer_start = k_uptime_get();

	return ret;
}

static u32_t lldp_manage_timeouts(struct ethernet_lldp *lldp, s64_t timeout)
{
	s32_t next_timeout;

	if (lldp_timedout(lldp, timeout)) {
		lldp_send(lldp);
	}

	next_timeout = timeout - (lldp->tx_timer_start +
				  lldp->tx_timer_timeout);

	return abs(next_timeout);
}

static void lldp_tx_timeout(struct k_work *work)
{
	u32_t timeout_update = UINT32_MAX - 1;
	s64_t timeout = k_uptime_get();
	struct ethernet_lldp *current, *next;

	ARG_UNUSED(work);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&lldp_ifaces, current, next, node) {
		u32_t next_timeout;

		next_timeout = lldp_manage_timeouts(current, timeout);
		if (next_timeout < timeout_update) {
			timeout_update = next_timeout;
		}
	}

	if (timeout_update < (UINT32_MAX - 1)) {
		NET_DBG("Waiting for %u ms", timeout_update);

		k_delayed_work_submit(&lldp_tx_timer, timeout_update);
	}
}

static void lldp_start_timer(struct ethernet_context *ctx,
			     struct net_if *iface,
			     int slot)
{
	ctx->lldp[slot].iface = iface;

	sys_slist_append(&lldp_ifaces, &ctx->lldp[slot].node);

	ctx->lldp[slot].tx_timer_start = k_uptime_get();
	ctx->lldp[slot].tx_timer_timeout =
		K_SECONDS(CONFIG_NET_LLDP_TX_INTERVAL);

	lldp_submit_work(ctx->lldp[slot].tx_timer_timeout);
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

static int lldp_start(struct net_if *iface, u32_t mgmt_event)
{
	struct ethernet_context *ctx;
	int ret, slot;

	ret = lldp_check_iface(iface);
	if (ret < 0) {
		return ret;
	}

	ctx = net_if_l2_data(iface);

	ret = lldp_find(ctx, iface);
	if (ret < 0) {
		return ret;
	}

	slot = ret;

	if (mgmt_event == NET_EVENT_IF_DOWN) {
		sys_slist_find_and_remove(&lldp_ifaces,
					  &ctx->lldp[slot].node);

		if (sys_slist_is_empty(&lldp_ifaces)) {
			k_delayed_work_cancel(&lldp_tx_timer);
		}
	} else if (mgmt_event == NET_EVENT_IF_UP) {
		NET_DBG("Starting timer for iface %p", iface);
		lldp_start_timer(ctx, iface, slot);
	}

	return 0;
}

enum net_verdict net_lldp_recv(struct net_if *iface, struct net_pkt *pkt)
{
	struct ethernet_context *ctx;
	net_lldp_recv_cb_t cb;
	int ret;

	ret = lldp_check_iface(iface);
	if (ret < 0) {
		return NET_DROP;
	}

	ctx = net_if_l2_data(iface);

	ret = lldp_find(ctx, iface);
	if (ret < 0) {
		return NET_DROP;
	}

	cb = ctx->lldp[ret].cb;
	if (cb) {
		return cb(iface, pkt);
	}

	return NET_DROP;
}

int net_lldp_register_callback(struct net_if *iface, net_lldp_recv_cb_t cb)
{
	struct ethernet_context *ctx;
	int ret;

	ret = lldp_check_iface(iface);
	if (ret < 0) {
		return ret;
	}

	ctx = net_if_l2_data(iface);

	ret = lldp_find(ctx, iface);
	if (ret < 0) {
		return ret;
	}

	ctx->lldp[ret].cb = cb;

	return 0;
}

static void iface_event_handler(struct net_mgmt_event_callback *cb,
				u32_t mgmt_event, struct net_if *iface)
{
	lldp_start(iface, mgmt_event);
}

static void iface_cb(struct net_if *iface, void *user_data)
{
	/* If the network interface is already up, then call the sender
	 * immediately. If the interface is not ethernet one, then
	 * lldp_start() will return immediately.
	 */
	if (atomic_test_bit(iface->if_dev->flags, NET_IF_UP)) {
		lldp_start(iface, NET_EVENT_IF_UP);
	}
}

int net_lldp_config(struct net_if *iface, const struct net_lldpdu *lldpdu)
{
	struct ethernet_context *ctx = net_if_l2_data(iface);
	int i;

	i = lldp_find(ctx, iface);
	if (i < 0) {
		return i;
	}

	ctx->lldp[i].lldpdu = lldpdu;

	return 0;
}

void net_lldp_init(void)
{
	k_delayed_work_init(&lldp_tx_timer, lldp_tx_timeout);

	net_if_foreach(iface_cb, NULL);

	net_mgmt_init_event_callback(&cb, iface_event_handler,
				     NET_EVENT_IF_UP | NET_EVENT_IF_DOWN);
	net_mgmt_add_event_callback(&cb);
}
