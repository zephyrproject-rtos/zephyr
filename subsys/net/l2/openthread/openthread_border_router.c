/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "openthread_border_router.h"
#include <openthread.h>
#include <openthread/border_agent.h>
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
#include <zephyr/sys/util.h>

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_NAT64_TRANSLATOR)
#include <openthread/nat64.h>
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_NAT64_TRANSLATOR */

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static struct net_mgmt_event_callback ail_net_event_connection_cb;
static struct net_mgmt_event_callback ail_net_event_ipv6_addr_cb;
static bool border_router_ipv6_services_running;
#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
static struct net_mgmt_event_callback ail_net_event_ipv4_addr_cb;
static bool has_ipv4_connectivity;
static bool border_router_ipv4_services_running;
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */
static uint32_t ail_iface_index;
static struct net_if *ail_iface_ptr;
static struct net_if *ot_iface_ptr;
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
static void openthread_border_router_add_or_rm_route_to_multicast_groups(bool add);
#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_NAT64_TRANSLATOR)
static bool nat64_translator_enabled;
static int openthread_border_router_start_nat64_service(void);
static void openthread_border_router_stop_nat64_service(void);
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_NAT64_TRANSLATOR */

int openthread_start_border_router_services_ipv6(struct net_if *ot_iface, struct net_if *ail_iface)
{
	int error = 0;
	otInstance *instance = openthread_get_default_instance();
	ail_iface_index = (uint32_t)net_if_get_by_iface(ail_iface);
	ail_iface_ptr = ail_iface;
	ot_iface_ptr = ot_iface;

	net_if_flag_set(ot_iface, NET_IF_FORWARD_MULTICASTS);
	net_if_flag_set(ail_iface, NET_IF_FORWARD_MULTICASTS);

	openthread_border_router_add_or_rm_route_to_multicast_groups(true);

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

	if (IS_ENABLED(CONFIG_OPENTHREAD_DNS_UPSTREAM_QUERY)) {
		if (dns_upstream_resolver_init(instance) != OT_ERROR_NONE) {
			error = -EIO;
			goto exit;
		}
	}

	/* Call OpenThread API */
	if (otBorderRoutingInit(instance, ail_iface_index, true) != OT_ERROR_NONE) {
		error = -EIO;
		goto exit;
	}
	if (otBorderRoutingSetEnabled(instance, true) != OT_ERROR_NONE) {
		error = -EIO;
		goto exit;
	}
	if (!otBorderAgentIsEnabled(instance)) {
		otBorderAgentSetEnabled(instance, true);
	}
	if (otPlatInfraIfStateChanged(instance, ail_iface_index, true) != OT_ERROR_NONE) {
		error = -EIO;
		goto exit;
	}

	if (IS_ENABLED(CONFIG_OPENTHREAD_DNS_UPSTREAM_QUERY)) {
		otDnssdUpstreamQuerySetEnabled(instance, true);
	}

	otBorderRoutingDhcp6PdSetEnabled(instance, true);
	otBackboneRouterSetEnabled(instance, true);
	otSrpServerSetAutoEnableMode(instance, true);

	openthread_mutex_unlock();

	is_border_router_started = true;
	border_router_ipv6_services_running = true;

exit:
	if (error) {
		openthread_mutex_unlock();
		return error;
	}

	return error;
}

int openthread_start_border_router_services_ipv4(struct net_if *ot_iface, struct net_if *ail_iface)
{
	int error = 0;

	openthread_mutex_lock();

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_NAT64_TRANSLATOR)

	if (openthread_border_router_start_nat64_service() == 0) {
		openthread_border_router_set_nat64_translator_enabled(true);
		border_router_ipv4_services_running = true;
	} else {
		error = -EIO;
	}

#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_NAT64_TRANSLATOR */

	openthread_mutex_unlock();

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
		(void)infra_if_deinit();
		infra_if_stop_icmp6_listener();
		otBorderAgentSetEnabled(instance, false);
		udp_plat_deinit();
#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_NAT64_TRANSLATOR)
		openthread_border_router_stop_nat64_service();
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_NAT64_TRANSLATOR */

		openthread_border_router_add_or_rm_route_to_multicast_groups(false);

	}
exit:
	if (error == 0) {
		is_border_router_started = false;
		border_router_ipv6_services_running = false;
	}
	openthread_mutex_unlock();
	return error;
}

void openthread_set_bbr_multicast_listener_cb(openthread_bbr_multicast_listener_cb cb,
					      void *context)
{
	__ASSERT(cb != NULL, "Receive callback is not set");

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
		if (!net_if_is_wifi(iface)) {
			net_if_up(ot_context->iface);
		}
		(void)openthread_start_border_router_services_ipv6(ot_context->iface, iface);
		break;
	case NET_EVENT_IF_DOWN:
#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
		has_ipv4_connectivity = false;
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */
		(void)openthread_stop_border_router_services(ot_context->iface, iface);
		break;
	default:
		break;
	}

	mdns_plat_monitor_interface(iface);
}

static void ail_ipv6_address_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
					   struct net_if *iface)
{

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if ((mgmt_event & (NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_ADDR_DEL)) != mgmt_event) {
		return;
	}

	mdns_plat_monitor_interface(iface);
}

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
static void ail_ipv4_address_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
					   struct net_if *iface)
{
	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if ((mgmt_event & (NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL)) != mgmt_event) {
		return;
	}

	struct openthread_context *ot_context = openthread_get_default_context();

	switch (mgmt_event) {
	case NET_EVENT_IPV4_ADDR_ADD:
		has_ipv4_connectivity = true;
		if (!border_router_ipv4_services_running) {
			openthread_start_border_router_services_ipv4(ot_context->iface, iface);
		}
		break;
	case NET_EVENT_IPV4_ADDR_DEL:
		if (net_if_ipv4_get_global_addr(ail_iface_ptr, NET_ADDR_PREFERRED) == NULL) {
			/* Application should stop all IPV4 related services */
			openthread_border_router_stop_nat64_service();

			has_ipv4_connectivity = false;
		}

	}
	mdns_plat_monitor_interface(iface);
}
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */

static void ot_bbr_multicast_listener_handler(void *context,
					      otBackboneRouterMulticastListenerEvent event,
					      const otIp6Address *address)
{
	struct openthread_context *ot_context = (struct openthread_context *)context;
	struct net_in6_addr recv_addr = {0};
	struct net_if_mcast_addr *mcast_addr = NULL;
	struct net_route_entry_mcast *entry = NULL;

	memcpy(recv_addr.s6_addr, address->mFields.m32, sizeof(otIp6Address));

	if (event == OT_BACKBONE_ROUTER_MULTICAST_LISTENER_ADDED) {
		entry = net_route_mcast_add(ot_context->iface, &recv_addr,
					    NUM_BITS(struct net_in6_addr));
		if (entry != NULL) {
			/*
			 * No need to perform mcast_lookup explicitly as it's already done in
			 * net_if_ipv6_maddr_add call. If it's found, NULL will be returned
			 * and maddr_join will not be performed.
			 */
			mcast_addr = net_if_ipv6_maddr_add(ot_context->iface,
							   (const struct net_in6_addr *)&recv_addr);
			if (mcast_addr != NULL) {
				net_if_ipv6_maddr_join(ot_context->iface, mcast_addr);
			}
		}
	} else {
		struct net_route_entry_mcast *route_to_del =
			net_route_mcast_lookup_by_iface(&recv_addr, ot_context->iface);
		struct net_if_mcast_addr *addr_to_del;

		addr_to_del = net_if_ipv6_maddr_lookup(&recv_addr, &(ot_context->iface));
		if (route_to_del != NULL) {
			net_route_mcast_del(route_to_del);
		}

		if (addr_to_del != NULL && net_if_ipv6_maddr_is_joined(addr_to_del)) {
			net_if_ipv6_maddr_leave(ot_context->iface, addr_to_del);
			net_if_ipv6_maddr_rm(ot_context->iface,
					     (const struct net_in6_addr *)&recv_addr);
		}
	}
}

void openthread_border_router_init(struct openthread_context *ot_ctx)
{
	net_mgmt_init_event_callback(&ail_net_event_connection_cb, ail_connection_handler,
				     NET_EVENT_IF_UP | NET_EVENT_IF_DOWN);
	net_mgmt_add_event_callback(&ail_net_event_connection_cb);
	net_mgmt_init_event_callback(&ail_net_event_ipv6_addr_cb, ail_ipv6_address_event_handler,
				     NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_ADDR_DEL);
	net_mgmt_add_event_callback(&ail_net_event_ipv6_addr_cb);

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
		net_mgmt_init_event_callback(&ail_net_event_ipv4_addr_cb,
					     ail_ipv4_address_event_handler,
					     NET_EVENT_IPV4_ADDR_ADD | NET_EVENT_IPV4_ADDR_DEL);
		net_mgmt_add_event_callback(&ail_net_event_ipv4_addr_cb);
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */

	udp_plat_init_sockfd();
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

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4)
bool openthread_border_router_has_ipv4_connectivity(void)
{
	return has_ipv4_connectivity;
}
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_IPV4 */

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
		/* A secondary BBR should not forward onto an external iface
		 * or from external network.
		 */
		if (otBackboneRouterGetState(instance) == OT_BACKBONE_ROUTER_STATE_SECONDARY ||
		    otBackboneRouterGetState(instance) == OT_BACKBONE_ROUTER_STATE_DISABLED) {
			return false;
		}
		/* AIL to Thread network message */
		if (net_pkt_orig_iface(pkt) == ail_iface_ptr) {
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

	if (net_ipv6_is_addr_mcast_raw(hdr->dst)) {
		return false;
	}

	/*
	 * This is the case when a packet from OpenThread stack is sent via UDP platform.
	 * Packet will be eventually returned to OpenThread interface, but it won't have
	 * orig_iface set, an indication that the packet was not forwarded from another interface.
	 * In this case, this function should not check and let the packet be handled by
	 * 15.4 layer.
	 */

	if (net_pkt_orig_iface(pkt) != ail_iface_ptr && net_pkt_iface(pkt) == ot_iface_ptr) {
		return true;
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

static void openthread_border_router_add_or_rm_route_to_multicast_groups(bool add)
{
	static uint8_t mcast_group_idx[] = {
		0x04, /** Admin-Local scope multicast address */
		0x05, /** Site-Local scope multicast address */
		0x08, /** Organization-Local scope multicast address */
		0x0e, /** Global scope multicast address */
	};
	struct net_in6_addr addr = {0};
	struct net_if_mcast_addr *mcast_addr = NULL;
	struct net_route_entry_mcast *entry = NULL;

	ARRAY_FOR_EACH(mcast_group_idx, i) {

		net_ipv6_addr_create(&addr, (0xff << 8) | mcast_group_idx[i], 0, 0, 0, 0, 0, 0, 0);
		if (add) {
			entry = net_route_mcast_add(ail_iface_ptr, &addr, 16);
			if (entry != NULL) {
				mcast_addr = net_if_ipv6_maddr_add(ail_iface_ptr,
								(const struct net_in6_addr *)&addr);
				if (mcast_addr != NULL) {
					net_if_ipv6_maddr_join(ail_iface_ptr, mcast_addr);
				}
			}
		} else {
			entry = net_route_mcast_lookup(&addr);
			mcast_addr = net_if_ipv6_maddr_lookup(&addr, &(ail_iface_ptr));
			if (entry != NULL) {
				net_route_mcast_del(entry);
			}
			/* There is no need to check if address is joined,
			 * as `clear_joined_ipv6_mcast_groups` was previously
			 * called
			 */
			if (mcast_addr != NULL) {
				net_if_ipv6_maddr_leave(ail_iface_ptr, mcast_addr);
				net_if_ipv6_maddr_rm(ail_iface_ptr,
						(const struct net_in6_addr *)&addr);
			}
		}
	}
}

#if defined(CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_NAT64_TRANSLATOR)
void openthread_border_router_set_nat64_translator_enabled(bool enable)
{
	otInstance *instance = openthread_get_default_instance();

	if (nat64_translator_enabled != enable) {
		nat64_translator_enabled = enable;
		otNat64SetEnabled(instance, enable);
	}
}

static int openthread_border_router_start_nat64_service(void)
{
	int error = 0;
	otInstance *instance = openthread_get_default_instance();
	struct net_in_addr *ipv4_addr = NULL;
	struct net_in_addr ipv4_def_route = {0};
	bool translator_state = false;
	otIp4Cidr cidr;

	ipv4_addr = net_if_ipv4_get_global_addr(ail_iface_ptr, NET_ADDR_PREFERRED);
	ipv4_def_route = net_if_ipv4_get_gw(ail_iface_ptr);

	if (ipv4_addr != NULL && ipv4_def_route.s_addr != 0) {
		if (infra_if_nat64_init() == 0) {
			memcpy(&cidr.mAddress.mFields.m32, &(ipv4_addr->s_addr),
			       sizeof(otIp4Address));
			cidr.mLength = 32U;
			if (otNat64SetIp4Cidr(instance, &cidr) != 0) {
				error = -EIO;
				goto exit;
			}
			translator_state = nat64_translator_enabled;

			openthread_border_router_set_nat64_translator_enabled(translator_state);
		}
	}
exit:
	return error;
}

static void openthread_border_router_stop_nat64_service(void)
{
	if (border_router_ipv4_services_running) {
		otInstance *instance = openthread_get_default_instance();

		otNat64ClearIp4Cidr(instance);
		openthread_border_router_set_nat64_translator_enabled(false);
		(void)infra_if_nat64_deinit();

		border_router_ipv4_services_running = false;
	}
}
#endif /* CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_NAT64_TRANSLATOR */
