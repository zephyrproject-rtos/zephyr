/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "openthread_border_router.h"
#include <openthread.h>
#include <openthread/backbone_router_ftd.h>
#include <openthread/border_router.h>
#include <openthread/border_routing.h>
#include <openthread/dnssd_server.h>
#include <openthread/srp_server.h>
#include <openthread/link.h>
#include <openthread/mdns.h>
#include <openthread/thread.h>
#include <openthread/platform/infra_if.h>
#include <openthread/platform/entropy.h>
#include <platform-zephyr.h>
#include <route.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <ipv6.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/openthread.h>

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static struct net_mgmt_event_callback ail_net_event_connection_cb;
static struct net_mgmt_event_callback ail_net_event_address_cb;
#if defined(CONFIG_NET_IPV4)
static struct net_mgmt_event_callback ail_net_event_ipv4_addr_add_cb;
#endif /* CONFIG_NET_IPV4 */
static uint32_t ail_iface_index;
static struct net_if *ail_iface_ptr;
static bool is_border_router_started;
char otbr_vendor_name[] = OTBR_VENDOR_NAME;
char otbr_base_service_instance_name[] = OTBR_BASE_SERVICE_INSTANCE_NAME;
char otbr_model_name[] = OTBR_MODEL_NAME;

static void openthread_border_router_process(struct k_work *work);
K_WORK_DEFINE(openthread_border_router_work, openthread_border_router_process);

/* FIFO used for queuing up messages sent for Border Router. */
static K_FIFO_DEFINE(border_router_msg_rx_fifo);

K_MEM_SLAB_DEFINE_STATIC(border_router_messages_slab, sizeof(struct otbr_msg_ctx),
		  CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_MSG_POOL_NUM, sizeof(void *));

static const char *create_base_name(otInstance *ot_instance, char *base_name);

#if defined(CONFIG_NET_IPV4)
static void openthread_border_router_check_for_dhcpv4_addr(struct net_if *iface,
							   struct net_if_addr *addr,
							   void *user_data);
#endif /* CONFIG_NET_IPV4 */

int openthread_start_border_router_services(struct net_if *ot_iface, struct net_if *ail_iface)
{
	int error = 0;
	otInstance *instance = openthread_get_default_instance();
	ail_iface_index = (uint32_t)net_if_get_by_iface(ail_iface);
	ail_iface_ptr = ail_iface;

	net_if_flag_set(ot_iface, NET_IF_FORWARD_MULTICASTS);

	openthread_mutex_lock();

	if (otMdnsSetLocalHostName(instance,
				   create_base_name(instance, otbr_vendor_name)) != OT_ERROR_NONE) {
		error = -EIO;
		goto exit;
	}

	/* Initialize platform modules first */
	if (trel_plat_init(instance, ail_iface_ptr) != OT_ERROR_NONE) {
		error = -EIO;
		goto exit;
	}

	if (infra_if_init(instance, ail_iface_ptr) != OT_ERROR_NONE) {
		error = -EIO;
		goto exit;
	}
	if (udp_plat_init(instance, ail_iface_ptr, ot_iface) != OT_ERROR_NONE) {
		error = -EIO;
		goto exit;
	}
	if (mdns_plat_socket_init(instance, ail_iface_index) != OT_ERROR_NONE) {
		error = -EIO;
		goto exit;
	}
	if (dhcpv6_pd_client_init(instance, ail_iface_index) != OT_ERROR_NONE) {
		error = -EIO;
		goto exit;
	}

	if (border_agent_init(instance) != OT_ERROR_NONE) {
		error = -EIO;
		goto exit;
	}
#if defined(CONFIG_OPENTHREAD_DNS_UPSTREAM_QUERY)
	if (dns_upstream_resolver_init(instance) != OT_ERROR_NONE) {
		error = -EIO;
		goto exit;
	}
#endif /* CONFIG_OPENTHREAD_DNS_UPSTREAM_QUERY */

	/* Call OpenThread API */
	if (otBorderRoutingInit(instance, ail_iface_index, true) != OT_ERROR_NONE) {
		error = -EIO;
		goto exit;
	}
	if (otBorderRoutingSetEnabled(instance, true) != OT_ERROR_NONE) {
		error = -EIO;
		goto exit;
	}
	if (otPlatInfraIfStateChanged(instance, ail_iface_index, true) != OT_ERROR_NONE) {
		error = -EIO;
		goto exit;
	}

	otBorderRoutingDhcp6PdSetEnabled(instance, true);
	otBackboneRouterSetEnabled(instance, true);
	otSrpServerSetAutoEnableMode(instance, true);

#if defined(CONFIG_OPENTHREAD_DNS_UPSTREAM_QUERY)
	otDnssdUpstreamQuerySetEnabled(instance, true);
#endif /* CONFIG_OPENTHREAD_DNS_UPSTREAM_QUERY */

	openthread_mutex_unlock();

	is_border_router_started = true;

exit:
	if (error) {
		openthread_mutex_unlock();
		return error;
	}

	return error;
}

static int openthread_stop_border_router_services(struct net_if *ot_iface,
						  struct net_if *ail_iface)
{
	int error = 0;
	otInstance *instance = openthread_get_default_instance();

	openthread_mutex_lock();

	if (is_border_router_started) {
		/* Call OpenThread API */
		if (otPlatInfraIfStateChanged(instance, ail_iface_index, false) != OT_ERROR_NONE) {
			error = -EIO;
			goto exit;
		}
		if (otBorderRoutingSetEnabled(instance, false) != OT_ERROR_NONE) {
			error = -EIO;
			goto exit;
		}
		otBackboneRouterSetEnabled(instance, false);
		border_agent_deinit();
		infra_if_stop_icmp6_listener();
		udp_plat_deinit();

	}
exit:
	if (!error) {
		is_border_router_started = false;
	}
	openthread_mutex_unlock();
	return error;
}

void openthread_set_bbr_multicast_listener_cb(openthread_bbr_multicast_listener_cb cb,
					      void *context)
{
	__ASSERT(cb != NULL, "Receive callback is not set");
	__ASSERT(openthread_instance != NULL, "OpenThread instance is not "
					      "initialized");

	openthread_mutex_lock();
	otBackboneRouterSetMulticastListenerCallback(openthread_get_default_instance(), cb,
						     context);
	openthread_mutex_unlock();
}

static void ail_connection_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				   struct net_if *iface)
{
	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if ((mgmt_event & (NET_EVENT_IF_UP | NET_EVENT_IF_DOWN)) != mgmt_event) {
		return;
	}

	struct openthread_context *ot_context = openthread_get_default_context();

	switch (mgmt_event) {
	case NET_EVENT_IF_UP:
#if defined(CONFIG_NET_IPV4)
		bool addr_present = false;

		net_if_ipv4_addr_foreach(iface, openthread_border_router_check_for_dhcpv4_addr,
					 &addr_present);
		if (!addr_present) {
			break;
		}
#endif /* CONFIG_NET_IPV4*/
		(void)openthread_start_border_router_services(ot_context->iface, iface);
		break;
	case NET_EVENT_IF_DOWN:
		(void)openthread_stop_border_router_services(ot_context->iface, iface);
		break;
	default:
		break;
	}

	mdns_plat_monitor_interface(iface);
}

static void ail_address_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				      struct net_if *iface)
{
	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if ((mgmt_event & (NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_ADDR_DEL |
			   NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL)) != mgmt_event) {
		return;
	}

	mdns_plat_monitor_interface(iface);
}

#if defined(CONFIG_NET_IPV4)
static void ail_ipv4_address_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
					   struct net_if *iface)
{
	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	struct openthread_context *ot_context = openthread_get_default_context();

	openthread_start_border_router_services(ot_context->iface, iface);

	mdns_plat_monitor_interface(iface);
}
#endif /* CONFIG_NET_IPV4 */

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
	net_mgmt_init_event_callback(&ail_net_event_address_cb, ail_address_event_handler,
				     NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_ADDR_DEL |
				     NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL);
	net_mgmt_add_event_callback(&ail_net_event_address_cb);
#if defined(CONFIG_NET_IPV4)
	net_mgmt_init_event_callback(&ail_net_event_ipv4_addr_add_cb,
				     ail_ipv4_address_event_handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&ail_net_event_ipv4_addr_add_cb);
#endif /* CONFIG_NET_IPV4 */
	openthread_set_bbr_multicast_listener_cb(ot_bbr_multicast_listener_handler, (void *)ot_ctx);
	(void)infra_if_start_icmp6_listener();
}

void openthread_border_router_post_message(struct otbr_msg_ctx *msg_context)
{
	k_fifo_put(&border_router_msg_rx_fifo, msg_context);
	openthread_notify_border_router_work();
}

static void openthread_border_router_process(struct k_work *work)
{
	struct otbr_msg_ctx *context;

	(void)work;

	do {
		context =
			(struct otbr_msg_ctx *)k_fifo_get(&border_router_msg_rx_fifo, K_NO_WAIT);
		if (context != NULL) {
			if (context->socket == NULL) {
				context->cb(context);
			} else {
				otMessageSettings ot_message_settings = {
					true, OT_MESSAGE_PRIORITY_NORMAL};
				otMessage *ot_message = NULL;

				ot_message = otUdpNewMessage(openthread_get_default_instance(),
							     &ot_message_settings);

				otMessageAppend(ot_message, context->buffer,
						context->length);
				context->socket->mHandler(context->socket->mContext, ot_message,
							  &context->message_info);
				otMessageFree(ot_message);
			}
			openthread_border_router_deallocate_message((void *)context);
		}
	} while (context != NULL);

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

static const char *create_base_name(otInstance *ot_instance, char *base_name)
{
	const otExtAddress *extAddress = otLinkGetExtendedAddress(ot_instance);
	char *replace = strstr(base_name, "#");

	if (replace != NULL) {
		replace++; /* skip # */
	} else {
		return NULL;
	}
	sprintf(replace, "%02x%02x", extAddress->m8[6], extAddress->m8[7]);

	return (const char *)base_name;
}

int openthread_border_router_allocate_message(void **msg)
{
	int error = 0;

	if (k_mem_slab_alloc(&border_router_messages_slab, msg, K_NO_WAIT) != 0) {
		error = -EIO;
		goto exit;
	}
	memset(*msg, 0, sizeof(struct otbr_msg_ctx));

exit:
	return error;
}

void openthread_border_router_deallocate_message(void *msg)
{
	k_mem_slab_free(&border_router_messages_slab, msg);
}

#if defined(CONFIG_NET_IPV4)
static void openthread_border_router_check_for_dhcpv4_addr(struct net_if *iface,
							   struct net_if_addr *addr,
							   void *user_data)
{
	(void)iface;

	bool *is_addr_present = (bool *)user_data;

	if (addr->addr_type != NET_ADDR_DHCP) {
		return;
	}

	*is_addr_present = true;
}
#endif /* CONFIG_NET_IPV4 */

static bool openthread_border_router_has_multicast_listener(const uint8_t *address)
{
	otInstance *instance = openthread_get_default_instance();
	otBackboneRouterMulticastListenerIterator iterator = 0;
	otBackboneRouterMulticastListenerInfo info;

	while (otBackboneRouterMulticastListenerGetNext(instance, &iterator, &info) ==
	       OT_ERROR_NONE) {
		if (net_ipv6_addr_cmp_raw(info.mAddress.mFields.m8, address)) {
			return true;
		}
	}

	return false;
}

static bool openthread_border_router_can_forward_multicast(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	otInstance *instance = openthread_get_default_instance();
	struct net_ipv6_hdr *hdr;

	hdr = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &ipv6_access);
	if (hdr == NULL) {
		return false;
	}

	if (net_ipv6_is_addr_mcast_raw(hdr->dst)) {
		/* AIL to Thread network message */
		if (net_pkt_orig_iface(pkt) == ail_iface_ptr) {
			if (otBackboneRouterGetState(instance) ==
			    OT_BACKBONE_ROUTER_STATE_SECONDARY ||
			    otBackboneRouterGetState(instance) ==
			    OT_BACKBONE_ROUTER_STATE_DISABLED) {
				return false;
			}
			if (openthread_border_router_has_multicast_listener(hdr->dst)) {
				return true;
			}
			return false;
		/* Thread to AIL message*/
		} else {
			const otMeshLocalPrefix *ml_prefix = otThreadGetMeshLocalPrefix(instance);

			if (net_ipv6_get_addr_mcast_scope_raw(hdr->dst) < 0x04) { /* admin local */
				return false;
			}
			if (net_ipv6_is_prefix(hdr->src, ml_prefix->m8,
					       sizeof(otIp6NetworkPrefix) * 8U)) {
				return false;
			}

			return true;
		}
	}
	return false;
}

static bool openthread_border_router_check_unicast_packet_forwarding_policy(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access, struct net_ipv6_hdr);
	struct net_ipv6_hdr *hdr;
	otInstance *instance = openthread_get_default_instance();
	otNetworkDataIterator iterator = OT_NETWORK_DATA_ITERATOR_INIT;
	const otMeshLocalPrefix *mesh_local_prefix = otThreadGetMeshLocalPrefix(instance);
	otBorderRouterConfig config;

	hdr = (struct net_ipv6_hdr *)net_pkt_get_data(pkt, &ipv6_access);
	if (hdr == NULL) {
		return false;
	}

	/* An IPv6 packet with a link-local source address or a link-local destination address
	 * is never forwarded.
	 */
	if (net_ipv6_is_ll_addr_raw(hdr->src) || net_ipv6_is_ll_addr_raw(hdr->dst)) {
		return false;
	}

	/* An IPv6 packet with mesh-local source address or a mesh-lolcal destination address
	 * is never forwarded between Thread network and AIL.
	 */
	if (mesh_local_prefix != NULL) {
		if (net_ipv6_is_prefix(hdr->src, mesh_local_prefix->m8,
				       sizeof(otIp6NetworkPrefix) * 8U) ||
		    net_ipv6_is_prefix(hdr->dst, mesh_local_prefix->m8,
				       sizeof(otIp6NetworkPrefix) * 8U)) {
			return false;
		}
	}

	/* Source address within the Thread network OMR prefix is never forwarded onto the
	 * Thread network from outside Thread network.
	 */

	/* Destination address within the Thread network OMR prefix is never forwarded from the
	 * Thread network outside Thread network.
	 */

	while (otNetDataGetNextOnMeshPrefix(instance, &iterator, &config) == OT_ERROR_NONE) {
		if (config.mDp) {
			continue;
		}
		if (net_pkt_orig_iface(pkt) == ail_iface_ptr) {
			if (net_ipv6_is_prefix(hdr->src, config.mPrefix.mPrefix.mFields.m8,
					       config.mPrefix.mLength)) {
				return false;
			}
		} else {
			if (net_ipv6_is_prefix(hdr->dst, config.mPrefix.mPrefix.mFields.m8,
					       config.mPrefix.mLength)) {
				return false;
			}
		}
	}

	return true;
}

bool openthread_border_router_check_packet_forwarding_rules(struct net_pkt *pkt)
{
	if (!openthread_border_router_can_forward_multicast(pkt)) {
		if (!openthread_border_router_check_unicast_packet_forwarding_policy(pkt)) {
			return false;
		}
	}

	return true;
}
