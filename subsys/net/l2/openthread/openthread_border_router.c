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
#include <openthread/link.h>
#include <openthread/mdns.h>
#include <openthread/platform/infra_if.h>
#include <openthread/platform/entropy.h>
#include <platform-zephyr.h>
#include <route.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/openthread.h>

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static struct net_mgmt_event_callback ail_net_event_connection_cb;
static struct net_mgmt_event_callback ail_net_event_address_cb;
static uint32_t ail_iface_index;
static struct net_if *ail_iface_ptr;
static bool is_border_router_started;
static otMdnsHost mdns_host;
static otIp6Address mdns_host_address_list[6];
char vendor_name[] = VENDOR_NAME;
char base_service_instance_name[] = BASE_SERVICE_INSTANCE_NAME;
char model_name[] = MODEL_NAME;

void openthread_border_router_process(struct k_work *work);
K_WORK_DEFINE(openthread_border_router_work, openthread_border_router_process);

/* FIFO used for queuing up messages sent for Border Router. */
K_FIFO_DEFINE(border_router_msg_rx_fifo);

K_MEM_SLAB_DEFINE(border_router_messages_slab, sizeof(struct otbr_msg_ctx),
		  CONFIG_OPENTHREAD_ZEPHYR_BORDER_ROUTER_MSG_POOL_NUM, sizeof(void *));

static void openthread_border_router_create_mdns_host(otInstance *instance);
static bool update_mdns_host_address_list(void);
static void mdns_handle_register_cb(otInstance *instance, otMdnsRequestId req_id, otError error);
static const char *create_base_name(otInstance *ot_instance, char *base_name);
static const char *create_alternative_base_name(char *base_name);

int openthread_start_border_router_services(struct net_if *ot_iface, struct net_if *ail_iface)
{
	int error = 0;
	otInstance *instance = openthread_get_default_instance();

	ail_iface_index = (uint32_t)net_if_get_by_iface(ail_iface);
	ail_iface_ptr = ail_iface;

	net_if_flag_set(ot_iface, NET_IF_FORWARD_MULTICASTS);

	openthread_border_router_create_mdns_host(instance);

	openthread_mutex_lock();
	/* Initialize platform modules first */
	if (trel_plat_init(instance, ail_iface_index) != OT_ERROR_NONE) {
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
	otBackboneRouterSetEnabled(instance, true);

	if (otMdnsRegisterHost(instance, &mdns_host, 0, mdns_handle_register_cb) != OT_ERROR_NONE) {
		error = -EIO;
		goto exit;
	}

	openthread_mutex_unlock();

	is_border_router_started = true;

exit:
	if (error) {
		openthread_mutex_unlock();
		return error;
	}

	return error;
}

static int openthread_stop_border_router_services(struct net_if *ot_iface, struct net_if *ail_iface)
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
		(void)openthread_start_border_router_services(ot_context->iface, iface);
		break;
	case NET_EVENT_IF_DOWN:
		(void)openthread_stop_border_router_services(ot_context->iface, iface);
		break;
	default:
		break;
	}
}

static void ail_address_event_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				      struct net_if *iface)
{
	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if ((mgmt_event & (NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_ADDR_DEL)) != mgmt_event) {
		return;
	}

	switch (mgmt_event) {
	case NET_EVENT_IPV6_ADDR_ADD:
		if (update_mdns_host_address_list()) {
			otMdnsRegisterHost(openthread_get_default_instance(), &mdns_host, 0,
					   mdns_handle_register_cb);
		}
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
	net_mgmt_init_event_callback(&ail_net_event_address_cb, ail_address_event_handler,
				     NET_EVENT_IPV6_ADDR_ADD | NET_EVENT_IPV6_ADDR_DEL);
	net_mgmt_add_event_callback(&ail_net_event_address_cb);
	openthread_set_bbr_multicast_listener_cb(ot_bbr_multicast_listener_handler, (void *)ot_ctx);
	(void)infra_if_start_ra_listener();

}

void openthread_border_router_post_message(struct otbr_msg_ctx *msg_context)
{
	k_fifo_put(&border_router_msg_rx_fifo, msg_context);
	openthread_notify_border_router_work();
}

void openthread_border_router_process(struct k_work *work)
{
	struct otbr_msg_ctx *context;

	(void)work;

	do {
		context =
			(struct otbr_msg_ctx *)k_fifo_get(&border_router_msg_rx_fifo, K_NO_WAIT);
		if (context) {
			if (!context->socket) {
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
			k_mem_slab_free(&border_router_messages_slab, (void *)context);
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

static void mdns_handle_register_cb(otInstance *instance, otMdnsRequestId req_id, otError error)
{
	(void)req_id;
	if (error == OT_ERROR_NONE) {
		border_agent_init(instance);
	} else {
		/* rename */
		mdns_host.mHostName = create_alternative_base_name((char *)mdns_host.mHostName);
		otMdnsRegisterHost(instance, &mdns_host, 0, mdns_handle_register_cb);
	}
}

static bool update_mdns_host_address_list(void)
{
	bool addr_change = false, should_add = false;
	static uint32_t new_ip6_addr_num;
	uint8_t address_iterator, stored_iterator;
	struct net_if_addr *unicast = NULL;
	struct net_if_ipv6 *ipv6 = NULL;

	if (!net_if_is_up(ail_iface_ptr) || !ail_iface_ptr) {
		return false;
	}

	ipv6 = ail_iface_ptr->config.ip.ipv6;

	for (address_iterator = 0;
	     address_iterator < NET_IF_MAX_IPV6_ADDR && new_ip6_addr_num < NET_IF_MAX_IPV6_ADDR;
	     address_iterator++) {
		should_add = true;
		unicast = &ipv6->unicast[address_iterator];
		if (unicast->is_used) {
			for (stored_iterator = 0; stored_iterator < NET_IF_MAX_IPV6_ADDR;
			     stored_iterator++) {
				if (0 ==
				    memcmp(&mdns_host_address_list[stored_iterator].mFields.m32,
					   &unicast->address.in6_addr.s6_addr32,
					   sizeof(mdns_host_address_list[stored_iterator]
							  .mFields.m32))) {
					should_add = false;
					break;
				}
			}
			if (stored_iterator == NET_IF_MAX_IPV6_ADDR) {
				addr_change |= true;
			}
			if (should_add) {
				memcpy(&mdns_host_address_list[new_ip6_addr_num++].mFields.m32,
				       &unicast->address.in6_addr.s6_addr32,
				       sizeof(mdns_host_address_list[0].mFields.m32));
			}
		}
	}

	addr_change |= (new_ip6_addr_num != mdns_host.mAddressesLength) ? true : false;
	mdns_host.mAddressesLength = new_ip6_addr_num;

	return addr_change;
}

static void openthread_border_router_create_mdns_host(otInstance *instance)
{

	mdns_host.mHostName = create_base_name(instance, vendor_name);
	mdns_host.mAddressesLength = 0;
	mdns_host.mTtl = 120;
	mdns_host.mInfraIfIndex = ail_iface_index;
	mdns_host.mAddresses = mdns_host_address_list;

	otMdnsSetLocalHostName(instance, mdns_host.mHostName);
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

static const char *create_alternative_base_name(char *base_name)
{
	char *replace = strstr(base_name, "#");
	uint8_t random[2] = {0};

	if (replace != NULL) {
		replace++; /* skip # */
	} else {
		return NULL;
	}
	otPlatEntropyGet(random, sizeof(random));
	sprintf(replace, "%02x%02x", random[0], random[1]);
	return base_name;
}
