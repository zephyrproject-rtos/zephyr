/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_l2_ppp, CONFIG_NET_L2_PPP_LOG_LEVEL);

#include <stdlib.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/ppp.h>
#include <zephyr/sys/iterable_sections.h>

#include "net_private.h"

#include "ppp_stats.h"
#include "ppp_internal.h"

static K_FIFO_DEFINE(tx_queue);

#if defined(CONFIG_NET_TC_THREAD_COOPERATIVE)
/* Lowest priority cooperative thread */
#define THREAD_PRIORITY K_PRIO_COOP(CONFIG_NET_L2_PPP_THREAD_PRIO)
#else
#define THREAD_PRIORITY K_PRIO_PREEMPT(CONFIG_NET_L2_PPP_THREAD_PRIO)
#endif

static void tx_handler(void *p1, void *p2, void *p3);

static K_THREAD_DEFINE(tx_handler_thread, CONFIG_NET_L2_PPP_TX_STACK_SIZE,
		       tx_handler, NULL, NULL, NULL,
		       THREAD_PRIORITY, 0, 0);

static const struct ppp_protocol_handler *ppp_lcp;

static void ppp_update_rx_stats(struct net_if *iface,
				struct net_pkt *pkt, size_t length)
{
#if defined(CONFIG_NET_STATISTICS_PPP)
	ppp_stats_update_bytes_rx(iface, length);
	ppp_stats_update_pkts_rx(iface);
#endif /* CONFIG_NET_STATISTICS_PPP */
}

static void ppp_update_tx_stats(struct net_if *iface,
				struct net_pkt *pkt, size_t length)
{
#if defined(CONFIG_NET_STATISTICS_PPP)
	ppp_stats_update_bytes_tx(iface, length);
	ppp_stats_update_pkts_tx(iface);
#endif /* CONFIG_NET_STATISTICS_PPP */
}

#if defined(CONFIG_NET_TEST)
typedef enum net_verdict (*ppp_l2_callback_t)(struct net_if *iface,
					      struct net_pkt *pkt);

static ppp_l2_callback_t testing_cb;

void ppp_l2_register_pkt_cb(ppp_l2_callback_t cb)
{
	testing_cb = cb;
}
#endif

static enum net_verdict process_ppp_msg(struct net_if *iface,
					struct net_pkt *pkt)
{
	struct ppp_context *ctx = net_if_l2_data(iface);
	enum net_verdict verdict = NET_DROP;
	uint16_t protocol;
	int ret;

	if (!ctx->is_ready_to_serve) {
		goto quit;
	}

	ret = net_pkt_read_be16(pkt, &protocol);
	if (ret < 0) {
		goto quit;
	}

	if ((IS_ENABLED(CONFIG_NET_IPV4) && protocol == PPP_IP) ||
	    (IS_ENABLED(CONFIG_NET_IPV6) && protocol == PPP_IPV6)) {
		/* Remove the protocol field so that IP packet processing
		 * continues properly in net_core.c:process_data()
		 */
		(void)net_buf_pull_be16(pkt->buffer);
		net_pkt_cursor_init(pkt);
		return NET_CONTINUE;
	}

	STRUCT_SECTION_FOREACH(ppp_protocol_handler, proto) {
		if (proto->protocol != protocol) {
			continue;
		}

		return proto->handler(ctx, iface, pkt);
	}

	switch (protocol) {
	case PPP_IP:
	case PPP_IPV6:
	case PPP_ECP:
	case PPP_CCP:
	case PPP_LCP:
	case PPP_IPCP:
	case PPP_IPV6CP:
		ppp_send_proto_rej(iface, pkt, protocol);
		break;
	default:
		break;
	}

	NET_DBG("%s protocol %s%s(0x%02x)",
		ppp_proto2str(protocol) ? "Unhandled" : "Unknown",
		ppp_proto2str(protocol),
		ppp_proto2str(protocol) ? " " : "",
		protocol);

quit:
	return verdict;
}

static enum net_verdict ppp_recv(struct net_if *iface,
				 struct net_pkt *pkt)
{
	enum net_verdict verdict;

#if defined(CONFIG_NET_TEST)
	/* If we are running a PPP unit test, then feed the packet
	 * back to test app for verification.
	 */
	if (testing_cb) {
		return testing_cb(iface, pkt);
	}
#endif

	ppp_update_rx_stats(iface, pkt, net_pkt_get_len(pkt));

	if (CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG) {
		net_pkt_hexdump(pkt, "recv L2");
	}

	verdict = process_ppp_msg(iface, pkt);

	switch (verdict) {
	case NET_OK:
		net_pkt_unref(pkt);
		break;
	case NET_DROP:
		ppp_stats_update_drop_rx(iface);
		break;
	case NET_CONTINUE:
		break;
	}

	return verdict;
}

static int ppp_send(struct net_if *iface, struct net_pkt *pkt)
{
	const struct ppp_api *api = net_if_get_device(iface)->api;
	struct ppp_context *ctx = net_if_l2_data(iface);
	int ret;

	if (CONFIG_NET_L2_PPP_LOG_LEVEL >= LOG_LEVEL_DBG) {
		net_pkt_hexdump(pkt, "send L2");
	}

	/* If PPP is not yet ready, then just give error to caller as there
	 * is no way to send before the PPP handshake is finished.
	 */
	if (ctx->phase != PPP_RUNNING && !net_pkt_is_ppp(pkt)) {
		return -ENETDOWN;
	}

	/* PPP drivers only support IP packet types, therefore in order to be
	 * able to use AF_PACKET family sockets with PPP, we need to translate
	 * L2 proto type to packet family.
	 */
	if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) &&
	    net_pkt_family(pkt) == AF_PACKET) {
		switch (net_pkt_ll_proto_type(pkt)) {
		case ETH_P_IP:
			net_pkt_set_family(pkt, AF_INET);
			break;
		case ETH_P_IPV6:
			net_pkt_set_family(pkt, AF_INET6);
			break;
		default:
			return -EPROTONOSUPPORT;
		}
	}

	ret = net_l2_send(api->send, net_if_get_device(iface), iface, pkt);
	if (!ret) {
		ret = net_pkt_get_len(pkt);
		ppp_update_tx_stats(iface, pkt, ret);
		net_pkt_unref(pkt);
	}

	return ret;
}

static enum net_l2_flags ppp_flags(struct net_if *iface)
{
	struct ppp_context *ctx = net_if_l2_data(iface);

	return ctx->ppp_l2_flags;
}

static void ppp_open_async(struct ppp_context *ctx)
{
	ppp_change_phase(ctx, PPP_ESTABLISH);

	if (ppp_lcp) {
		NET_DBG("Starting LCP");
		ppp_lcp->lower_up(ctx);
		ppp_lcp->open(ctx);
	}
}

static int ppp_up(struct net_if *iface)
{
	const struct ppp_api *ppp = net_if_get_device(iface)->api;

	if (ppp->start) {
		ppp->start(net_if_get_device(iface));
	}

	return 0;
}

static int ppp_lcp_close(struct ppp_context *ctx)
{
	if (ppp_lcp == NULL) {
		ppp_change_phase(ctx, PPP_DEAD);
	}

	if (ctx->phase == PPP_DEAD) {
		return 0;
	}

	k_sem_reset(&ctx->wait_ppp_link_terminated);
	ppp_lcp->close(ctx, "L2 Disabled");
	return k_sem_take(&ctx->wait_ppp_link_terminated,
			  K_MSEC(CONFIG_NET_L2_PPP_TIMEOUT *
				 (1 + CONFIG_NET_L2_PPP_MAX_TERMINATE_REQ_RETRANSMITS)));
}

static void ppp_lcp_lower_down_async(struct ppp_context *ctx)
{
	if (ctx->phase == PPP_DEAD) {
		return;
	}

	if (ppp_lcp == NULL) {
		ppp_change_phase(ctx, PPP_DEAD);
	} else {
		ppp_lcp->lower_down(ctx);
	}
}

static int ppp_lcp_lower_down(struct ppp_context *ctx)
{
	if (ppp_lcp == NULL) {
		ppp_change_phase(ctx, PPP_DEAD);
	}

	if (ctx->phase == PPP_DEAD) {
		return 0;
	}

	k_sem_reset(&ctx->wait_ppp_link_down);
	ppp_lcp->lower_down(ctx);
	return k_sem_take(&ctx->wait_ppp_link_down, K_MSEC(CONFIG_NET_L2_PPP_TIMEOUT));
}

/* Bring down network interface by terminating all protocols */
static int ppp_down(struct net_if *iface)
{
	const struct ppp_api *ppp = net_if_get_device(iface)->api;
	struct ppp_context *ctx = net_if_l2_data(iface);

	if (net_if_is_carrier_ok(iface)) {
		/* Terminate protocols and close LCP */
		if (ppp_lcp_close(ctx) < 0) {
			return -EAGAIN;
		}
	} else {
		/* Terminate protocols */
		if (ppp_lcp_lower_down(ctx) < 0) {
			return -EAGAIN;
		}
	}

	if (ppp->stop) {
		/* Inform L2 PPP device that PPP link is down */
		ppp->stop(net_if_get_device(iface));
	}

	return 0;
}

static int ppp_enable(struct net_if *iface, bool state)
{
	struct ppp_context *ctx = net_if_l2_data(iface);
	int ret;

	/* Set the desired network interface state */
	ctx->is_enabled = state;

	/* Attempt to enter desired state */
	if (state) {
		ret = ppp_up(iface);
	} else {
		ret = ppp_down(iface);
	}

	if (ret < 0) {
		/* Reset the desired state */
		ctx->is_enabled = !state;
	}

	return ret;
}

NET_L2_INIT(PPP_L2, ppp_recv, ppp_send, ppp_enable, ppp_flags);

#if defined(CONFIG_NET_SHELL)
static int get_ppp_context(int idx, struct ppp_context **ctx,
			   struct net_if **iface)
{
	*iface = net_if_get_by_index(idx);

	if (!*iface) {
		return -ENOENT;
	}

	if (net_if_l2(*iface) != &NET_L2_GET_NAME(PPP)) {
		return -ENODEV;
	}

	*ctx = net_if_l2_data(*iface);

	return 0;
}

static void echo_reply_handler(void *user_data, size_t user_data_len)
{
	struct ppp_context *ctx = user_data;
	uint32_t end_time = k_cycle_get_32();
	uint32_t time_diff;

	time_diff = end_time - ctx->shell.echo_req_data;
	ctx->shell.echo_req_data =
		k_cyc_to_ns_floor64(time_diff) / 1000;

	k_sem_give(&ctx->shell.wait_echo_reply);
}

int net_ppp_ping(int idx, int32_t timeout)
{
	struct ppp_context *ctx;
	struct net_if *iface;
	int ret;

	ret = get_ppp_context(idx, &ctx, &iface);
	if (ret < 0) {
		return ret;
	}

	ctx->shell.echo_req_data = k_cycle_get_32();
	ctx->shell.echo_reply.cb = echo_reply_handler;
	ctx->shell.echo_reply.user_data = ctx;
	ctx->shell.echo_reply.user_data_len = sizeof(ctx);

	ret = ppp_send_pkt(&ctx->lcp.fsm, iface, PPP_ECHO_REQ, 0,
			   UINT_TO_POINTER(ctx->shell.echo_req_data),
			   sizeof(ctx->shell.echo_req_data));
	if (ret < 0) {
		return ret;
	}

	ret = k_sem_take(&ctx->shell.wait_echo_reply, K_MSEC(timeout));

	ctx->shell.echo_reply.cb = NULL;

	if (ret < 0) {
		return ret;
	}

	/* Returns amount of microseconds waited */
	return ctx->shell.echo_req_data;
}

struct ppp_context *net_ppp_context_get(int idx)
{
	struct ppp_context *ctx;
	struct net_if *iface;
	int ret;

	if (idx == 0) {
		iface = net_if_get_first_by_type(&NET_L2_GET_NAME(PPP));
		if (!iface) {
			return NULL;
		}

		return net_if_l2_data(iface);
	}

	ret = get_ppp_context(idx, &ctx, &iface);
	if (ret < 0) {
		return NULL;
	}

	return ctx;
}
#endif

const struct ppp_protocol_handler *ppp_lcp_get(void)
{
	return ppp_lcp;
}

void ppp_queue_pkt(struct net_pkt *pkt)
{
	k_fifo_put(&tx_queue, pkt);
}

static void tx_handler(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct net_pkt *pkt;
	int ret;

	NET_DBG("PPP TX started");

	k_thread_name_set(NULL, "ppp_tx");

	while (1) {
		pkt = k_fifo_get(&tx_queue, K_FOREVER);
		if (pkt == NULL) {
			continue;
		}

		ret = net_send_data(pkt);
		if (ret < 0) {
			net_pkt_unref(pkt);
		}
	}
}

static void net_ppp_mgmt_evt_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				     struct net_if *iface)
{
	struct ppp_context *ctx;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(PPP)) {
		return;
	}

	ctx = net_if_l2_data(iface);

	if (net_if_is_carrier_ok(iface)) {
		ppp_mgmt_raise_carrier_on_event(iface);
	} else {
		ppp_mgmt_raise_carrier_off_event(iface);
	}

	if (mgmt_event == NET_EVENT_IF_UP) {
		ppp_open_async(ctx);
		return;
	}

	if ((mgmt_event == NET_EVENT_IF_DOWN) && (!net_if_is_carrier_ok(iface))) {
		ppp_lcp_lower_down_async(ctx);
	}
}

void net_ppp_init(struct net_if *iface)
{
	struct ppp_context *ctx = net_if_l2_data(iface);
	uint8_t count = 0;

	NET_DBG("Initializing PPP L2 %p for iface %p", ctx, iface);

	memset(ctx, 0, sizeof(*ctx));

	ctx->ppp_l2_flags = NET_L2_MULTICAST | NET_L2_POINT_TO_POINT;
	ctx->iface = iface;
	k_sem_init(&ctx->wait_ppp_link_terminated, 0, 1);
	k_sem_init(&ctx->wait_ppp_link_down, 0, 1);

#if defined(CONFIG_NET_SHELL)
	k_sem_init(&ctx->shell.wait_echo_reply, 0, K_SEM_MAX_LIMIT);
#endif

	net_mgmt_init_event_callback(&ctx->mgmt_evt_cb, net_ppp_mgmt_evt_handler,
				     (NET_EVENT_IF_UP | NET_EVENT_IF_DOWN));

	net_mgmt_add_event_callback(&ctx->mgmt_evt_cb);

	STRUCT_SECTION_FOREACH(ppp_protocol_handler, proto) {
		if (proto->protocol == PPP_LCP) {
			ppp_lcp = proto;
		}

		proto->init(ctx);
		count++;
	}

	if (count == 0) {
		NET_ERR("There are no PPP protocols configured!");
		return;
	}

	if (ppp_lcp == NULL) {
		NET_ERR("No LCP found!");
		return;
	}

	ctx->is_ready_to_serve = true;
}
