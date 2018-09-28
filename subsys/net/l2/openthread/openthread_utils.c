/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_SYS_LOG_LEVEL CONFIG_OPENTHREAD_L2_LOG_LEVEL

#if defined(CONFIG_OPENTHREAD_L2_DEBUG)
#define NET_DOMAIN "net/openthread_l2"
#define NET_LOG_ENABLED 1
#endif

#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/openthread.h>

#include <openthread/openthread.h>

#include "openthread_utils.h"

int pkt_list_add(struct openthread_context *context, struct net_pkt *pkt)
{
	u16_t i_idx = context->pkt_list_in_idx;

	if (context->pkt_list_full) {
		return -ENOMEM;
	}

	i_idx++;
	if (i_idx == CONFIG_OPENTHREAD_PKT_LIST_SIZE) {
		i_idx = 0;
	}

	if (i_idx == context->pkt_list_out_idx) {
		context->pkt_list_full = 1;
	}

	context->pkt_list[context->pkt_list_in_idx].pkt = pkt;
	context->pkt_list_in_idx = i_idx;

	return 0;
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
		context->pkt_list_out_idx = 0;
	}

	context->pkt_list_full = 0;
}

void add_ipv6_addr_to_zephyr(struct openthread_context *context)
{
	const otNetifAddress *address;

	for (address = otIp6GetUnicastAddresses(context->instance);
	     address; address = address->mNext) {
#if CONFIG_OPENTHREAD_L2_LOG_LEVEL == SYS_LOG_LEVEL_DEBUG
		char buf[NET_IPV6_ADDR_LEN];

		NET_DBG("Adding %s",
			net_addr_ntop(AF_INET6,
				      (struct in6_addr *)(&address->mAddress),
				      buf, sizeof(buf)));
#endif
		net_if_ipv6_addr_add(context->iface,
				     (struct in6_addr *)(&address->mAddress),
				     NET_ADDR_ANY, 0);
	}
}

void add_ipv6_addr_to_ot(struct openthread_context *context)
{
	struct net_if *iface = context->iface;
	struct otNetifAddress addr;
	struct net_if_ipv6 *ipv6;
	int i;

	(void)memset(&addr, 0, sizeof(addr));

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		NET_DBG("Cannot allocate IPv6 address");
		return;
	}

	/* save the last added IP address for this interface */
	for (i = NET_IF_MAX_IPV6_ADDR - 1; i >= 0; i--) {
		if (ipv6->unicast[i].is_used) {
			memcpy(&addr.mAddress,
			       &ipv6->unicast[i].address.in6_addr,
			       sizeof(addr.mAddress));
			break;
		}
	}

	addr.mValid = true;
	addr.mPreferred = true;
	addr.mPrefixLength = 64;

	otIp6AddUnicastAddress(context->instance, &addr);

#if CONFIG_OPENTHREAD_L2_LOG_LEVEL == SYS_LOG_LEVEL_DEBUG
	{
		char buf[NET_IPV6_ADDR_LEN];

		NET_DBG("Added %s", net_addr_ntop(AF_INET6,
						  &addr.mAddress, buf,
						  sizeof(buf)));
	}
#endif
}

void add_ipv6_maddr_to_ot(struct openthread_context *context)
{
	struct otIp6Address addr;
	struct net_if_ipv6 *ipv6;
	int i;

	if (net_if_config_ipv6_get(context->iface, &ipv6) < 0) {
		NET_DBG("Cannot allocate IPv6 address");
		return;
	}

	/* save the last added IP address for this interface */
	for (i = NET_IF_MAX_IPV6_MADDR - 1; i >= 0; i--) {
		if (ipv6->mcast[i].is_used) {
			memcpy(&addr,
			       &ipv6->mcast[i].address.in6_addr,
			       sizeof(addr));
			break;
		}
	}

	otIp6SubscribeMulticastAddress(context->instance, &addr);

#if CONFIG_OPENTHREAD_L2_LOG_LEVEL == SYS_LOG_LEVEL_DEBUG
	{
		char buf[NET_IPV6_ADDR_LEN];

		NET_DBG("Added multicast %s",
			net_addr_ntop(AF_INET6, &addr,
				      buf, sizeof(buf)));
	}
#endif
}

void add_ipv6_maddr_to_zephyr(struct openthread_context *context)
{
	const otNetifMulticastAddress *maddress;

	for (maddress = otIp6GetMulticastAddresses(context->instance);
	     maddress; maddress = maddress->mNext) {
		if (net_if_ipv6_maddr_lookup(
				(struct in6_addr *)(&maddress->mAddress),
				&context->iface) != NULL) {
			continue;
		}

#if CONFIG_OPENTHREAD_L2_LOG_LEVEL == SYS_LOG_LEVEL_DEBUG
		char buf[NET_IPV6_ADDR_LEN];

		NET_DBG("Adding multicast %s",
			net_addr_ntop(AF_INET6,
				      (struct in6_addr *)(&maddress->mAddress),
				      buf, sizeof(buf)));
#endif
		net_if_ipv6_maddr_add(context->iface,
				      (struct in6_addr *)(&maddress->mAddress));
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
#if CONFIG_OPENTHREAD_L2_LOG_LEVEL == SYS_LOG_LEVEL_DEBUG
			char buf[NET_IPV6_ADDR_LEN];

			NET_DBG("Removing %s",
				net_addr_ntop(AF_INET6,
					      &zephyr_addr->address.in6_addr,
					      buf, sizeof(buf)));
#endif
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
#if CONFIG_OPENTHREAD_L2_LOG_LEVEL == SYS_LOG_LEVEL_DEBUG
			char buf[NET_IPV6_ADDR_LEN];

			NET_DBG("Removing multicast %s",
				net_addr_ntop(AF_INET6,
					      &zephyr_addr->address.in6_addr,
					      buf, sizeof(buf)));
#endif
			net_if_ipv6_maddr_rm(context->iface,
					     &zephyr_addr->address.in6_addr);
		}
	}
}
