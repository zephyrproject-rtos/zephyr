/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_l2_openthread, CONFIG_OPENTHREAD_L2_LOG_LEVEL);

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/openthread.h>

#include <openthread/ip6.h>
#include <openthread/thread.h>

#include "openthread_utils.h"

#define ALOC16_MASK 0xfc

static bool is_anycast_locator(const otNetifAddress *address)
{
	return address->mAddress.mFields.m16[4] == htons(0x0000) &&
	       address->mAddress.mFields.m16[5] == htons(0x00ff) &&
	       address->mAddress.mFields.m16[6] == htons(0xfe00) &&
	       address->mAddress.mFields.m8[14] == ALOC16_MASK;
}

static bool is_mesh_local(struct openthread_context *context,
			  const uint8_t *address)
{
	const otMeshLocalPrefix *ml_prefix =
				otThreadGetMeshLocalPrefix(context->instance);

	return (memcmp(address, ml_prefix->m8, sizeof(ml_prefix->m8)) == 0);
}

int pkt_list_add(struct openthread_context *context, struct net_pkt *pkt)
{
	uint16_t i_idx = context->pkt_list_in_idx;

	if (context->pkt_list_full) {
		return -ENOMEM;
	}

	i_idx++;
	if (i_idx == CONFIG_OPENTHREAD_PKT_LIST_SIZE) {
		i_idx = 0U;
	}

	if (i_idx == context->pkt_list_out_idx) {
		context->pkt_list_full = 1U;
	}

	context->pkt_list[context->pkt_list_in_idx].pkt = pkt;
	context->pkt_list_in_idx = i_idx;

	return 0;
}

void pkt_list_remove_first(struct openthread_context *context)
{
	uint16_t idx = context->pkt_list_in_idx;

	if (idx == 0U) {
		idx = CONFIG_OPENTHREAD_PKT_LIST_SIZE - 1;
	} else {
		idx--;
	}
	context->pkt_list_in_idx = idx;

	if (context->pkt_list_full) {
		context->pkt_list_full = 0U;
	}
}

struct net_pkt *pkt_list_peek(struct openthread_context *context)
{
	if ((context->pkt_list_in_idx == context->pkt_list_out_idx) &&
	    (!context->pkt_list_full)) {

		return NULL;
	}
	return context->pkt_list[context->pkt_list_out_idx].pkt;
}

void pkt_list_remove_last(struct openthread_context *context)
{
	if ((context->pkt_list_in_idx == context->pkt_list_out_idx) &&
	    (!context->pkt_list_full)) {

		return;
	}

	context->pkt_list_out_idx++;
	if (context->pkt_list_out_idx == CONFIG_OPENTHREAD_PKT_LIST_SIZE) {
		context->pkt_list_out_idx = 0U;
	}

	context->pkt_list_full = 0U;
}

void add_ipv6_addr_to_zephyr(struct openthread_context *context)
{
	const otNetifAddress *address;
	struct net_if_addr *if_addr;

	for (address = otIp6GetUnicastAddresses(context->instance);
	     address; address = address->mNext) {

		if (address->mRloc || is_anycast_locator(address)) {
			continue;
		}

		if (CONFIG_OPENTHREAD_L2_LOG_LEVEL == LOG_LEVEL_DBG) {
			char buf[NET_IPV6_ADDR_LEN];

			NET_DBG("Adding %s",
				net_addr_ntop(AF_INET6,
				       (struct in6_addr *)(&address->mAddress),
				       buf, sizeof(buf)));
		}

		/* Thread and SLAAC are clearly AUTOCONF, handle
		 * manual/NCP addresses in the same way
		 */
		if ((address->mAddressOrigin == OT_ADDRESS_ORIGIN_THREAD) ||
		    (address->mAddressOrigin == OT_ADDRESS_ORIGIN_SLAAC)) {
			if_addr = net_if_ipv6_addr_add(
					context->iface,
					(struct in6_addr *)(&address->mAddress),
					NET_ADDR_AUTOCONF, 0);
		} else if (address->mAddressOrigin ==
			   OT_ADDRESS_ORIGIN_DHCPV6) {
			if_addr = net_if_ipv6_addr_add(
					context->iface,
					(struct in6_addr *)(&address->mAddress),
					NET_ADDR_DHCP, 0);
		} else if (address->mAddressOrigin ==
			  OT_ADDRESS_ORIGIN_MANUAL) {
			if_addr = net_if_ipv6_addr_add(
					context->iface,
					(struct in6_addr *)(&address->mAddress),
					NET_ADDR_MANUAL, 0);
		} else {
			NET_ERR("Unknown OpenThread address origin ignored.");
			continue;
		}

		if (if_addr == NULL) {
			NET_ERR("Cannot add OpenThread unicast address");
			continue;
		}

		if_addr->is_mesh_local = is_mesh_local(
					context, address->mAddress.mFields.m8);

		/* Mark address as deprecated if it is not preferred. */
		if_addr->addr_state =
			address->mPreferred ? NET_ADDR_PREFERRED : NET_ADDR_DEPRECATED;
	}
}

void add_ipv6_addr_to_ot(struct openthread_context *context,
			 const struct in6_addr *addr6)
{
	struct otNetifAddress addr = { 0 };
	struct net_if_ipv6 *ipv6;
	struct net_if_addr *if_addr = NULL;
	int i;

	/* IPv6 struct should've already been allocated when we get an
	 * address added event.
	 */
	ipv6 = context->iface->config.ip.ipv6;
	if (ipv6 == NULL) {
		NET_ERR("No IPv6 container allocated");
		return;
	}

	/* Find the net_if_addr structure containing the newly added address. */
	for (i = NET_IF_MAX_IPV6_ADDR - 1; i >= 0; i--) {
		if (ipv6->unicast[i].is_used &&
		    net_ipv6_addr_cmp(&ipv6->unicast[i].address.in6_addr,
				      addr6)) {
			if_addr = &ipv6->unicast[i];
			break;
		}
	}

	if (if_addr == NULL) {
		NET_ERR("No corresponding net_if_addr found");
		return;
	}

	memcpy(&addr.mAddress, addr6, sizeof(addr.mAddress));

	if_addr->is_mesh_local = is_mesh_local(
			context, ipv6->unicast[i].address.in6_addr.s6_addr);

	addr.mValid = true;
	addr.mPreferred = (if_addr->addr_state == NET_ADDR_PREFERRED);
	addr.mPrefixLength = 64;

	if (if_addr->addr_type == NET_ADDR_AUTOCONF) {
		addr.mAddressOrigin = OT_ADDRESS_ORIGIN_SLAAC;
	} else if (if_addr->addr_type == NET_ADDR_DHCP) {
		addr.mAddressOrigin = OT_ADDRESS_ORIGIN_DHCPV6;
	} else if (if_addr->addr_type == NET_ADDR_MANUAL) {
		addr.mAddressOrigin = OT_ADDRESS_ORIGIN_MANUAL;
	} else {
		NET_ERR("Unknown address type");
		return;
	}

	openthread_api_mutex_lock(context);
	otIp6AddUnicastAddress(context->instance, &addr);
	openthread_api_mutex_unlock(context);

	if (CONFIG_OPENTHREAD_L2_LOG_LEVEL == LOG_LEVEL_DBG) {
		char buf[NET_IPV6_ADDR_LEN];

		NET_DBG("Added %s",
			net_addr_ntop(AF_INET6, &addr.mAddress, buf, sizeof(buf)));
	}
}

void add_ipv6_maddr_to_ot(struct openthread_context *context,
			  const struct in6_addr *addr6)
{
	struct otIp6Address addr;

	memcpy(&addr, addr6, sizeof(addr));

	openthread_api_mutex_lock(context);
	otIp6SubscribeMulticastAddress(context->instance, &addr);
	openthread_api_mutex_unlock(context);

	if (CONFIG_OPENTHREAD_L2_LOG_LEVEL == LOG_LEVEL_DBG) {
		char buf[NET_IPV6_ADDR_LEN];

		NET_DBG("Added multicast %s",
			net_addr_ntop(AF_INET6, &addr, buf, sizeof(buf)));
	}
}

void add_ipv6_maddr_to_zephyr(struct openthread_context *context)
{
	const otNetifMulticastAddress *maddress;
	struct net_if_mcast_addr *zmaddr;

	for (maddress = otIp6GetMulticastAddresses(context->instance);
	     maddress; maddress = maddress->mNext) {
		if (net_if_ipv6_maddr_lookup(
				(struct in6_addr *)(&maddress->mAddress),
				&context->iface) != NULL) {
			continue;
		}

		if (CONFIG_OPENTHREAD_L2_LOG_LEVEL == LOG_LEVEL_DBG) {
			char buf[NET_IPV6_ADDR_LEN];

			NET_DBG("Adding multicast %s",
				net_addr_ntop(AF_INET6,
					      (struct in6_addr *)
					      (&maddress->mAddress),
					      buf, sizeof(buf)));
		}

		zmaddr = net_if_ipv6_maddr_add(context->iface,
				      (struct in6_addr *)(&maddress->mAddress));

		if (zmaddr &&
		    !(net_if_ipv6_maddr_is_joined(zmaddr) ||
		      net_ipv6_is_addr_mcast_iface(
				(struct in6_addr *)(&maddress->mAddress)) ||
		      net_ipv6_is_addr_mcast_link_all_nodes(
				(struct in6_addr *)(&maddress->mAddress)))) {

			net_if_ipv6_maddr_join(context->iface, zmaddr);
		}
	}
}

void rm_ipv6_addr_from_zephyr(struct openthread_context *context)
{
	struct in6_addr *ot_addr;
	struct net_if_addr *zephyr_addr;
	struct net_if_ipv6 *ipv6;
	int i;

	if (net_if_config_ipv6_get(context->iface, &ipv6) < 0) {
		NET_DBG("Cannot find IPv6 address");
		return;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		const otNetifAddress *address;
		bool used = false;

		zephyr_addr = &ipv6->unicast[i];
		if (!zephyr_addr->is_used) {
			continue;
		}

		for (address = otIp6GetUnicastAddresses(context->instance);
		     address; address = address->mNext) {

			ot_addr = (struct in6_addr *)(&address->mAddress);
			if (net_ipv6_addr_cmp(ot_addr,
					      &zephyr_addr->address.in6_addr)) {

				used = true;
				break;
			}
		}
		if (!used) {
			if (CONFIG_OPENTHREAD_L2_LOG_LEVEL == LOG_LEVEL_DBG) {
				char buf[NET_IPV6_ADDR_LEN];

				NET_DBG("Removing %s",
					net_addr_ntop(AF_INET6,
					      &zephyr_addr->address.in6_addr,
					      buf, sizeof(buf)));
			}

			net_if_ipv6_addr_rm(context->iface,
					    &zephyr_addr->address.in6_addr);
		}
	}
}

void rm_ipv6_maddr_from_zephyr(struct openthread_context *context)
{
	struct in6_addr *ot_addr;
	struct net_if_mcast_addr *zephyr_addr;
	struct net_if_ipv6 *ipv6;
	int i;

	if (net_if_config_ipv6_get(context->iface, &ipv6) < 0) {
		NET_DBG("Cannot find IPv6 address");
		return;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		const otNetifMulticastAddress *maddress;
		bool used = false;

		zephyr_addr = &ipv6->mcast[i];
		if (!zephyr_addr->is_used) {
			continue;
		}

		for (maddress = otIp6GetMulticastAddresses(context->instance);
		     maddress; maddress = maddress->mNext) {

			ot_addr = (struct in6_addr *)(&maddress->mAddress);
			if (net_ipv6_addr_cmp(ot_addr,
					      &zephyr_addr->address.in6_addr)) {

				used = true;
				break;
			}
		}
		if (!used) {
			if (CONFIG_OPENTHREAD_L2_LOG_LEVEL == LOG_LEVEL_DBG) {
				char buf[NET_IPV6_ADDR_LEN];

				NET_DBG("Removing multicast %s",
					net_addr_ntop(AF_INET6,
					      &zephyr_addr->address.in6_addr,
					      buf, sizeof(buf)));
			}

			net_if_ipv6_maddr_rm(context->iface,
					     &zephyr_addr->address.in6_addr);
		}
	}
}
