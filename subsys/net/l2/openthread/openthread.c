/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_l2_openthread, CONFIG_OPENTHREAD_L2_LOG_LEVEL);

#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/openthread.h>

#include <net_private.h>

#include <zephyr/init.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/__assert.h>

#include <openthread/thread.h>

#include <platform-zephyr.h>

#include "openthread_utils.h"

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER)
#include "openthread_border_router.h"
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER */

static struct net_linkaddr *ll_addr;
static struct openthread_state_changed_callback ot_l2_state_changed_cb;

#define PKT_IS_IPv4(_p) ((NET_IPV6_HDR(_p)->vtc & 0xf0) == 0x40)

#ifdef CONFIG_NET_MGMT_EVENT
static struct net_mgmt_event_callback ip6_addr_cb;

static void ipv6_addr_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				    struct net_if *iface)
{
	if (net_if_l2(iface) != &NET_L2_GET_NAME(OPENTHREAD)) {
		return;
	}

#ifdef CONFIG_NET_MGMT_EVENT_INFO
	struct openthread_context *ot_context = net_if_l2_data(iface);

	if (cb->info == NULL || cb->info_length != sizeof(struct in6_addr)) {
		return;
	}

	if (mgmt_event == NET_EVENT_IPV6_ADDR_ADD) {
		add_ipv6_addr_to_ot(ot_context, (const struct in6_addr *)cb->info);
	} else if (mgmt_event == NET_EVENT_IPV6_MADDR_ADD) {
		add_ipv6_maddr_to_ot(ot_context, (const struct in6_addr *)cb->info);
	}
#else
	NET_WARN("No address info provided with event, "
		 "please enable CONFIG_NET_MGMT_EVENT_INFO");
#endif /* CONFIG_NET_MGMT_EVENT_INFO */
}

#endif /* CONFIG_NET_MGMT_EVENT */

#ifndef CONFIG_HDLC_RCP_IF
void otPlatRadioGetIeeeEui64(otInstance *instance, uint8_t *ieee_eui64)
{
	ARG_UNUSED(instance);

	memcpy(ieee_eui64, ll_addr->addr, ll_addr->len);
}
#endif /* CONFIG_HDLC_RCP_IF */

static void ot_l2_state_changed_handler(uint32_t flags, void *context)
{
	struct openthread_context *ot_context = context;
	struct openthread_state_changed_cb *entry, *next;

#if defined(CONFIG_OPENTHREAD_INTERFACE_EARLY_UP)
	bool is_up = otIp6IsEnabled(openthread_get_default_instance());

	if (is_up) {
		net_if_dormant_off(ot_context->iface);
	} else {
		net_if_dormant_on(ot_context->iface);
	}
#else
	if (flags & OT_CHANGED_THREAD_ROLE) {
		switch (otThreadGetDeviceRole(openthread_get_default_instance())) {
		case OT_DEVICE_ROLE_CHILD:
		case OT_DEVICE_ROLE_ROUTER:
		case OT_DEVICE_ROLE_LEADER:
			net_if_dormant_off(ot_context->iface);
			break;

		case OT_DEVICE_ROLE_DISABLED:
		case OT_DEVICE_ROLE_DETACHED:
		default:
			net_if_dormant_on(ot_context->iface);
			break;
		}
	}
#endif

	if (flags & OT_CHANGED_IP6_ADDRESS_REMOVED) {
		rm_ipv6_addr_from_zephyr(ot_context);
		NET_DBG("Ipv6 address removed");
	}

	if (flags & OT_CHANGED_IP6_ADDRESS_ADDED) {
		add_ipv6_addr_to_zephyr(ot_context);
		NET_DBG("Ipv6 address added");
	}

	if (flags & OT_CHANGED_IP6_MULTICAST_UNSUBSCRIBED) {
		rm_ipv6_maddr_from_zephyr(ot_context);
		NET_DBG("Ipv6 multicast address removed");
	}

	if (flags & OT_CHANGED_IP6_MULTICAST_SUBSCRIBED) {
		add_ipv6_maddr_to_zephyr(ot_context);
		NET_DBG("Ipv6 multicast address added");
	}

#if defined(CONFIG_OPENTHREAD_NAT64_TRANSLATOR)

	if (flags & OT_CHANGED_NAT64_TRANSLATOR_STATE) {
		NET_DBG("Nat64 translator state changed to %x",
			otNat64GetTranslatorState(openthread_get_default_instance()));
	}

#endif /* CONFIG_OPENTHREAD_NAT64_TRANSLATOR */

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&ot_context->state_change_cbs, entry, next, node) {
		if (entry->state_changed_cb != NULL) {
			entry->state_changed_cb(flags, ot_context, entry->user_data);
		}
	}
}

static void ot_receive_handler(otMessage *message, void *context)
{
	struct openthread_context *ot_context = context;

	uint16_t offset = 0U;
	uint16_t read_len;
	struct net_pkt *pkt;
	struct net_buf *pkt_buf;

	pkt = net_pkt_rx_alloc_with_buffer(ot_context->iface, otMessageGetLength(message),
					   AF_UNSPEC, 0, K_NO_WAIT);
	if (!pkt) {
		NET_ERR("Failed to reserve net pkt");
		goto out;
	}

	pkt_buf = pkt->buffer;

	while (1) {
		read_len = otMessageRead(message, offset, pkt_buf->data, net_buf_tailroom(pkt_buf));
		if (!read_len) {
			break;
		}

		net_buf_add(pkt_buf, read_len);

		offset += read_len;

		if (!net_buf_tailroom(pkt_buf)) {
			pkt_buf = pkt_buf->frags;
			if (!pkt_buf) {
				break;
			}
		}
	}

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER)
	if (!openthread_border_router_check_packet_forwarding_rules(pkt)) {
		NET_DBG("Not injecting packet to Zephyr net stack "
			"Packet not compliant with forwarding rules!");
		goto out;
	}
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER */

	NET_DBG("Injecting %s packet to Zephyr net stack",
		PKT_IS_IPv4(pkt) ? "translated IPv4" : "Ip6");

	if (IS_ENABLED(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6)) {
		if (IS_ENABLED(CONFIG_OPENTHREAD_NAT64_TRANSLATOR) && PKT_IS_IPv4(pkt)) {
			net_pkt_hexdump(pkt, "Received NAT64 IPv4 packet");
		} else {
			net_pkt_hexdump(pkt, "Received IPv6 packet");
		}
	}

	net_pkt_set_ll_proto_type(pkt, PKT_IS_IPv4(pkt) ? ETH_P_IP : ETH_P_IPV6);

	if (!pkt_list_is_full(ot_context)) {
		if (pkt_list_add(ot_context, pkt) != 0) {
			NET_ERR("pkt_list_add failed");
			goto out;
		}

		if (net_recv_data(ot_context->iface, pkt) < 0) {
			NET_ERR("net_recv_data failed");
			pkt_list_remove_first(ot_context);
			goto out;
		}

		pkt = NULL;
	} else {
		NET_INFO("Packet list is full");
	}
out:
	if (pkt) {
		net_pkt_unref(pkt);
	}

	otMessageFree(message);
}

static bool is_ipv6_frag(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	struct net_ipv6_hdr *hdr;

	hdr = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &ipv6_access);
	if (!hdr) {
		return false;
	}

	return hdr->nexthdr == NET_IPV6_NEXTHDR_FRAG ? true : false;
}

static enum net_verdict openthread_recv(struct net_if *iface, struct net_pkt *pkt)
{
	struct openthread_context *ot_context = net_if_l2_data(iface);

	if (pkt_list_peek(ot_context) == pkt) {
		pkt_list_remove_last(ot_context);
		NET_DBG("Got injected Ip6 packet, sending to upper layers");

		if (IS_ENABLED(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6)) {
			net_pkt_hexdump(pkt, "Injected IPv6 packet");
		}

		if (IS_ENABLED(CONFIG_OPENTHREAD_IP6_FRAGM) && is_ipv6_frag(pkt)) {
			return NET_DROP;
		}

		return NET_CONTINUE;
	}

	NET_DBG("Got 802.15.4 packet, sending to OT");

	if (IS_ENABLED(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6)) {
		net_pkt_hexdump(pkt, "Received 802.15.4 frame");
	}

	if (notify_new_rx_frame(pkt) != 0) {
		NET_ERR("Failed to queue RX packet for OpenThread");
		return NET_DROP;
	}

	return NET_OK;
}

int openthread_send(struct net_if *iface, struct net_pkt *pkt)
{
	int len = net_pkt_get_len(pkt);

	if (IS_ENABLED(CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6)) {
		net_pkt_hexdump(pkt, "IPv6 packet to send");
	}

	net_capture_pkt(iface, pkt);

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER)
	if (!openthread_border_router_check_packet_forwarding_rules(pkt)) {
		net_pkt_unref(pkt);
		return len;
	}
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER */

	if (notify_new_tx_frame(pkt) != 0) {
		net_pkt_unref(pkt);
	}

	return len;
}

static int openthread_l2_init(struct net_if *iface)
{
	struct openthread_context *ot_l2_context = net_if_l2_data(iface);
	int error = 0;

	ot_l2_state_changed_cb.otCallback = ot_l2_state_changed_handler;
	ot_l2_state_changed_cb.user_data = (void *)ot_l2_context;

	ll_addr = net_if_get_link_addr(iface);

	error = openthread_init();
	if (error) {
		return error;
	}

	ot_l2_context->iface = iface;

	if (!IS_ENABLED(CONFIG_OPENTHREAD_COPROCESSOR)) {
		net_mgmt_init_event_callback(&ip6_addr_cb, ipv6_addr_event_handler,
					     NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_MADDR_ADD);
		net_mgmt_add_event_callback(&ip6_addr_cb);
		net_if_dormant_on(iface);

		openthread_set_receive_cb(ot_receive_handler, (void *)ot_l2_context);
#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER)
		openthread_border_router_init(ot_l2_context);
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER */

		/* To keep backward compatibility use the additional state change callback list from
		 * the ot l2 context and register the callback to the openthread module.
		 */
		sys_slist_init(&ot_l2_context->state_change_cbs);
		openthread_state_changed_callback_register(&ot_l2_state_changed_cb);
	}

	return error;
}

void ieee802154_init(struct net_if *iface)
{
	if (IS_ENABLED(CONFIG_IEEE802154_NET_IF_NO_AUTO_START)) {
		net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	}

	net_if_flag_set(iface, NET_IF_IPV6_NO_ND);
	net_if_flag_set(iface, NET_IF_IPV6_NO_MLD);

	int error = openthread_l2_init(iface);

	if (error) {
		NET_ERR("Failed to initialize OpenThread L2");
	}
}

static enum net_l2_flags openthread_flags(struct net_if *iface)
{
	/* TODO: Should report NET_L2_PROMISC_MODE if the radio driver
	 *       reports the IEEE802154_HW_PROMISC capability.
	 */
	return NET_L2_MULTICAST | NET_L2_MULTICAST_SKIP_JOIN_SOLICIT_NODE;
}

static int openthread_enable(struct net_if *iface, bool state)
{
	LOG_DBG("iface %p %s", iface, state ? "up" : "down");

	if (state) {
		if (IS_ENABLED(CONFIG_OPENTHREAD_MANUAL_START)) {
			return 0;
		}

		return openthread_run();
	}

	return openthread_stop();
}

struct openthread_context *openthread_get_default_context(void)
{
	struct net_if *iface;
	struct openthread_context *ot_context = NULL;

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(OPENTHREAD));
	if (!iface) {
		NET_ERR("There is no net interface for OpenThread");
		goto exit;
	}

	ot_context = net_if_l2_data(iface);
	if (!ot_context) {
		NET_ERR("There is no Openthread context in net interface data");
		goto exit;
	}

exit:
	return ot_context;
}

/* Keep deprecated functions and forward them to the OpenThread platform module */
int openthread_start(struct openthread_context *ot_context)
{
	ARG_UNUSED(ot_context);

	return openthread_run();
}

void openthread_api_mutex_lock(struct openthread_context *ot_context)
{
	/* The mutex is managed internally by the OpenThread module */
	ARG_UNUSED(ot_context);

	openthread_mutex_lock();
}

int openthread_api_mutex_try_lock(struct openthread_context *ot_context)
{
	/* The mutex is managed internally by the OpenThread module */
	ARG_UNUSED(ot_context);

	return openthread_mutex_try_lock();
}

void openthread_api_mutex_unlock(struct openthread_context *ot_context)
{
	/* The mutex is managed internally by the OpenThread module */
	ARG_UNUSED(ot_context);

	openthread_mutex_unlock();
}

/* Keep deprecated state change callback registration functions to keep backward compatibility.
 * The callbacks that are registered using these functions are run by the OpenThread module
 * as one of the platform callback. However, they will be not supported in the future after
 * deprecation period, so it is recommended to switch to
 * openthread_state_change_callback_register() instead.
 */
int openthread_state_changed_cb_register(struct openthread_context *ot_context,
					 struct openthread_state_changed_cb *cb)
{
	CHECKIF(cb == NULL || cb->state_changed_cb == NULL) {
		return -EINVAL;
	}

	openthread_mutex_lock();
	sys_slist_append(&ot_context->state_change_cbs, &cb->node);
	openthread_mutex_unlock();

	return 0;
}

int openthread_state_changed_cb_unregister(struct openthread_context *ot_context,
					   struct openthread_state_changed_cb *cb)
{
	bool removed;

	CHECKIF(cb == NULL) {
		return -EINVAL;
	}

	openthread_mutex_lock();
	removed = sys_slist_find_and_remove(&ot_context->state_change_cbs, &cb->node);
	openthread_mutex_unlock();

	if (!removed) {
		return -EALREADY;
	}

	return 0;
}

NET_L2_INIT(OPENTHREAD_L2, openthread_recv, openthread_send, openthread_enable, openthread_flags);
