/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <openthread/platform/infra_if.h>
#include <zephyr/net/ethernet.h>
#include <route.h>
#include <icmpv6.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/openthread.h>
#include <zephyr/net/icmp.h>
#include <platform-zephyr.h>
#include <openthread.h>
#include <openthread/backbone_router_ftd.h>
#include <openthread/border_router.h>
#include <openthread/border_routing.h>
#include "openthread_border_router.h"
#include <common/code_utils.hpp>

static struct net_mgmt_event_callback ail_net_event_connection_cb;
static struct net_icmp_ctx ra_ctx;
static struct net_icmp_ctx rs_ctx;
static uint32_t ail_iface_index;
static struct net_if *ail_iface_ptr;
static bool is_border_router_started;

int openthread_start_border_router_services(struct net_if *ot_iface, struct net_if *ail_iface)
{
	otError error = OT_ERROR_NONE;
	otInstance *instance = openthread_get_default_instance();

	ail_iface_index = (uint32_t)net_if_get_by_iface(ail_iface);
	ail_iface_ptr = ail_iface;

	net_if_flag_set(ot_iface, NET_IF_FORWARD_MULTICASTS);

	openthread_mutex_lock();
	/* Initialize platform modules first */
	VerifyOrExit(infra_if_init(instance, ail_iface_ptr) == OT_ERROR_NONE,
		     error = OT_ERROR_FAILED);

	/* Call OpenThread API */
	VerifyOrExit(otBorderRoutingInit(instance, ail_iface_index, true) == OT_ERROR_NONE,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(otBorderRoutingSetEnabled(instance, true) == OT_ERROR_NONE,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(otPlatInfraIfStateChanged(instance, ail_iface_index, true) == OT_ERROR_NONE,
		     error = OT_ERROR_FAILED);
	otBackboneRouterSetEnabled(instance, true);
	is_border_router_started = true;

exit:
	openthread_mutex_unlock();
	return error == OT_ERROR_NONE ? 0 : -EIO;
}

static int openthread_stop_border_router_services(struct net_if *ot_iface, struct net_if *ail_iface)
{
	otError error = OT_ERROR_FAILED;
	otInstance *instance = openthread_get_default_instance();

	openthread_mutex_lock();

	if (is_border_router_started) {
		/* Call OpenThread API */
		VerifyOrExit(otPlatInfraIfStateChanged(instance, ail_iface_index, false) ==
				     OT_ERROR_NONE,
			     error = OT_ERROR_FAILED);
		VerifyOrExit(otBorderRoutingSetEnabled(instance, false) == OT_ERROR_NONE,
			     error = OT_ERROR_FAILED);
		otBackboneRouterSetEnabled(instance, false);

		error = OT_ERROR_NONE;
		is_border_router_started = false;
	}
exit:
	openthread_mutex_unlock();
	return error == OT_ERROR_NONE ? 0 : -EIO;
}

void openthread_set_bbr_multicast_listener_cb(openthread_bbr_multicast_listener_cb cb,
					      void *context)
{
	__ASSERT(cb != NULL, "Receive callback is not set");
	__ASSERT(openthread_instance != NULL, "OpenThread instance is not initialized");

	openthread_mutex_lock();
	otBackboneRouterSetMulticastListenerCallback(openthread_get_default_instance(), cb,
						     context);
	openthread_mutex_unlock();
}

static int handle_ra_input(struct net_icmp_ctx *ctx, struct net_pkt *pkt,
			   struct net_icmp_ip_hdr *hdr, struct net_icmp_hdr *icmp_hdr,
			   void *user_data)
{
	otInstance *ot_instance = openthread_get_default_instance();
	otIp6Address src_addr = {0};
	uint16_t length = net_pkt_get_len(pkt);
	uint8_t payload[512] = {0};

	if (net_buf_linearize(payload, sizeof(payload), pkt->buffer, 0, length) ==
	    length) {
		memcpy(&src_addr, hdr->ipv6->src, sizeof(otIp6Address));
		/* TODO this will have to be executed on OT's context, not from here */
		otPlatInfraIfRecvIcmp6Nd(
			ot_instance, net_if_get_by_iface(net_pkt_iface(pkt)), &src_addr,
			(const uint8_t *)&payload[sizeof(struct net_ipv6_hdr)], length);
	}
	return 0;
}

static void ail_connection_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				   struct net_if *iface)
{

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	struct openthread_context *ot_context = openthread_get_default_context();

	switch (mgmt_event) {
	case NET_EVENT_IF_UP:
		(void)openthread_start_border_router_services(ot_context->iface, iface);
		break;
	case NET_EVENT_IF_DOWN:
		(void)openthread_stop_border_router_services(ot_context->iface, iface);
		break;
	default:
		break;
	}
}

static void ot_bbr_multicast_listener_handler(void *context,
					      otBackboneRouterMulticastListenerEvent event,
					      const otIp6Address *address)
{
	struct openthread_context *ot_context = context;
	struct in6_addr mcast_prefix = {0};

	memcpy(mcast_prefix.s6_addr, address->mFields.m32, sizeof(otIp6Address));

	if (event == OT_BACKBONE_ROUTER_MULTICAST_LISTENER_ADDED) {
		net_route_mcast_add((struct net_if *)ot_context->iface, &mcast_prefix, 16);
	} else {
		struct net_route_entry_mcast *route_to_del = net_route_mcast_lookup(&mcast_prefix);

		if (route_to_del != NULL) {
			net_route_mcast_del(route_to_del);
		}
	}
}

void openthread_border_router_init(struct openthread_context *ot_ctx)
{
	net_mgmt_init_event_callback(&ail_net_event_connection_cb, ail_connection_handler,
				     NET_EVENT_IF_UP | NET_EVENT_IF_DOWN);
	net_mgmt_add_event_callback(&ail_net_event_connection_cb);
	net_icmp_init_ctx(&ra_ctx, NET_ICMPV6_RA, 0, handle_ra_input);
	net_icmp_init_ctx(&rs_ctx, NET_ICMPV6_RS, 0, handle_ra_input);
	openthread_set_bbr_multicast_listener_cb(ot_bbr_multicast_listener_handler, (void *)ot_ctx);
}

const otIp6Address *get_ot_slaac_address(otInstance *instance)
{
	const otNetifAddress *unicast_addrs = otIp6GetUnicastAddresses(instance);
	otIp6Prefix omr_prefix_local = {0};
	otIp6Prefix addr_prefix = {0};

	if (otBorderRoutingGetOmrPrefix(instance, &omr_prefix_local) == OT_ERROR_NONE) {
		for (const otNetifAddress *addr = unicast_addrs; addr; addr = addr->mNext) {
			otIp6GetPrefix(&addr->mAddress, 64, &addr_prefix);
			if (otIp6ArePrefixesEqual(&omr_prefix_local, &addr_prefix)) {
				return &addr->mAddress;
			}
		}
	}
	return NULL;
}
