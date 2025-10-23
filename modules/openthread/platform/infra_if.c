/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "openthread/platform/infra_if.h"
#include "icmpv6.h"
#include "ipv6.h"
#include "openthread_border_router.h"
#include <common/code_utils.hpp>
#include <platform-zephyr.h>
#include <route.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/icmp.h>
#include <zephyr/net/mld.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/openthread.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/icmp.h>
#include <icmpv6.h>

static struct otInstance *ot_instance;
static struct net_if *ail_iface_ptr;
static uint32_t ail_iface_index;
static struct net_icmp_ctx ra_ctx;
static struct net_icmp_ctx rs_ctx;
static struct net_icmp_ctx na_ctx;

static void infra_if_handle_backbone_icmp6(struct otbr_msg_ctx *msg_ctx_ptr);
static void handle_ra_from_ot(const uint8_t *buffer, uint16_t buffer_length);

otError otPlatInfraIfSendIcmp6Nd(uint32_t aInfraIfIndex, const otIp6Address *aDestAddress,
				 const uint8_t *aBuffer, uint16_t aBufferLength)
{
	otError error = OT_ERROR_NONE;
	struct net_pkt *pkt = NULL;
	struct in6_addr dst = {0};
	const struct in6_addr *src;

	if (aBuffer[0] == NET_ICMPV6_RA) {
		handle_ra_from_ot(aBuffer, aBufferLength);
	}

	memcpy(&dst, aDestAddress, sizeof(otIp6Address));

	src = net_if_ipv6_select_src_addr(ail_iface_ptr, &dst);
	VerifyOrExit(!net_ipv6_is_addr_unspecified(src), error = OT_ERROR_FAILED);

	pkt = net_pkt_alloc_with_buffer(ail_iface_ptr, aBufferLength, AF_INET6, IPPROTO_ICMPV6,
					K_MSEC(100));
	VerifyOrExit(pkt, error = OT_ERROR_FAILED);

	net_pkt_set_ipv6_hop_limit(pkt, NET_IPV6_ND_HOP_LIMIT);

	VerifyOrExit(net_ipv6_create(pkt, src, &dst) == 0, error = OT_ERROR_FAILED);
	VerifyOrExit(net_pkt_write(pkt, aBuffer, aBufferLength) == 0, error = OT_ERROR_FAILED);
	net_pkt_cursor_init(pkt);

	VerifyOrExit(net_ipv6_finalize(pkt, IPPROTO_ICMPV6) == 0, error = OT_ERROR_FAILED);
	VerifyOrExit(net_send_data(pkt) == 0, error = OT_ERROR_FAILED);

exit:
	if (error == OT_ERROR_FAILED) {
		if (pkt != NULL) {
			net_pkt_unref(pkt);
			pkt = NULL;
		}
	}

	return error;
}

otError otPlatInfraIfDiscoverNat64Prefix(uint32_t aInfraIfIndex)
{
	OT_UNUSED_VARIABLE(aInfraIfIndex);

	return OT_ERROR_NOT_IMPLEMENTED;
}

bool otPlatInfraIfHasAddress(uint32_t aInfraIfIndex, const otIp6Address *aAddress)
{
	struct net_if_addr *ifaddr = NULL;
	struct in6_addr addr = {0};

	memcpy(addr.s6_addr, aAddress->mFields.m8, sizeof(otIp6Address));

	STRUCT_SECTION_FOREACH(net_if, tmp) {
		if (net_if_get_by_iface(tmp) != aInfraIfIndex) {
			continue;
		}
		ifaddr = net_if_ipv6_addr_lookup_by_iface(tmp, &addr);
		if (ifaddr != NULL) {
			return true;
		} else {
			return false;
		}
	}
	return false;
}

otError otPlatGetInfraIfLinkLayerAddress(otInstance *aInstance, uint32_t aIfIndex,
					 otPlatInfraIfLinkLayerAddress *aInfraIfLinkLayerAddress)
{
	OT_UNUSED_VARIABLE(aInstance);
	struct net_if *iface = net_if_get_by_index(aIfIndex);
	struct net_linkaddr *link_addr = net_if_get_link_addr(iface);

	aInfraIfLinkLayerAddress->mLength = link_addr->len;
	memcpy(aInfraIfLinkLayerAddress->mAddress, link_addr->addr, link_addr->len);

	return OT_ERROR_NONE;
}

otError infra_if_init(otInstance *instance, struct net_if *ail_iface)
{
	otError error = OT_ERROR_NONE;
	struct in6_addr addr = {0};

	ot_instance = instance;
	ail_iface_ptr = ail_iface;
	ail_iface_index = (uint32_t)net_if_get_by_iface(ail_iface_ptr);
	net_ipv6_addr_create_ll_allrouters_mcast(&addr);
	VerifyOrExit(net_ipv6_mld_join(ail_iface, &addr) == 0, error = OT_ERROR_FAILED);

exit:
	return error;
}

static void handle_ra_from_ot(const uint8_t *buffer, uint16_t buffer_length)
{
	struct net_if *ot_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(OPENTHREAD));
	struct net_if_ipv6_prefix *prefix_added = NULL;
	struct net_route_entry *route_added = NULL;
	struct in6_addr rio_prefix = {0};
	struct net_if_addr *ifaddr = NULL;
	struct in6_addr addr_to_add_from_pio = {0};
	struct in6_addr nexthop = {0};
	uint8_t i = sizeof(struct net_icmp_hdr) + sizeof(struct net_icmpv6_ra_hdr);

	while (i + sizeof(struct net_icmpv6_nd_opt_hdr) <= buffer_length) {
		const struct net_icmpv6_nd_opt_hdr *opt_hdr =
			(const struct net_icmpv6_nd_opt_hdr *)&buffer[i];

		i += sizeof(struct net_icmpv6_nd_opt_hdr);
		switch (opt_hdr->type) {
		case NET_ICMPV6_ND_OPT_PREFIX_INFO:
			const struct net_icmpv6_nd_opt_prefix_info *pio =
				(const struct net_icmpv6_nd_opt_prefix_info *)&buffer[i];
			prefix_added = net_if_ipv6_prefix_add(ail_iface_ptr,
							      (struct in6_addr *)pio->prefix,
							      pio->prefix_len, pio->valid_lifetime);
			i += sizeof(struct net_icmpv6_nd_opt_prefix_info);
			net_ipv6_addr_generate_iid(
				ail_iface_ptr, (struct in6_addr *)pio->prefix,
				COND_CODE_1(CONFIG_NET_IPV6_IID_STABLE,
				     ((uint8_t *)&ail_iface_ptr->config.ip.ipv6->network_counter),
				     (NULL)), COND_CODE_1(CONFIG_NET_IPV6_IID_STABLE,
				     (sizeof(ail_iface_ptr->config.ip.ipv6->network_counter)),
				     (0U)), 0U, &addr_to_add_from_pio,
							       net_if_get_link_addr(ail_iface_ptr));
			ifaddr = net_if_ipv6_addr_lookup(&addr_to_add_from_pio, NULL);
			if (ifaddr != NULL) {
				net_if_addr_set_lf(ifaddr, true);
			}
			net_if_ipv6_addr_add(ail_iface_ptr, &addr_to_add_from_pio,
					     NET_ADDR_AUTOCONF, pio->valid_lifetime);
			break;
		case NET_ICMPV6_ND_OPT_ROUTE:
			uint8_t prefix_field_len = (opt_hdr->len - 1) * 8;
			const otIp6Address *br_omr_addr = get_ot_slaac_address(ot_instance);

			const struct net_icmpv6_nd_opt_route_info *rio =
				(const struct net_icmpv6_nd_opt_route_info *)&buffer[i];

			i += sizeof(struct net_icmpv6_nd_opt_route_info);
			memcpy(&rio_prefix.s6_addr, &buffer[i], prefix_field_len);
			if (!otIp6IsAddressUnspecified(br_omr_addr)) {
				memcpy(&nexthop.s6_addr, br_omr_addr->mFields.m8,
				       sizeof(br_omr_addr->mFields.m8));
				net_ipv6_nbr_add(ot_iface, &nexthop, net_if_get_link_addr(ot_iface),
						 false, NET_IPV6_NBR_STATE_STALE);
				route_added = net_route_add(ot_iface, &rio_prefix, rio->prefix_len,
							    &nexthop, rio->route_lifetime,
							    rio->flags.prf);
			}
			break;
		default:
			break;
		}
	}
}

static int handle_icmp6_input(struct net_icmp_ctx *ctx, struct net_pkt *pkt,
			      struct net_icmp_ip_hdr *hdr,
			      struct net_icmp_hdr *icmp_hdr, void *user_data)
{
	uint16_t length = net_pkt_get_len(pkt);
	struct otbr_msg_ctx *req = NULL;
	otError error = OT_ERROR_NONE;

	VerifyOrExit(openthread_border_router_allocate_message((void **)&req) == OT_ERROR_NONE,
		     error = OT_ERROR_FAILED);

	if (net_buf_linearize(req->buffer, sizeof(req->buffer),
			      pkt->buffer, 0, length) == length) {
		req->length = length;
		memcpy(&req->addr, hdr->ipv6->src, sizeof(req->addr));
		req->cb = infra_if_handle_backbone_icmp6;

		openthread_border_router_post_message(req);
	} else {
		openthread_border_router_deallocate_message((void *)req);
		ExitNow(error = OT_ERROR_FAILED);
	}

exit:
	if (error == OT_ERROR_NONE) {
		return 0;
	}

	return -1;
}

static void infra_if_handle_backbone_icmp6(struct otbr_msg_ctx *msg_ctx_ptr)
{
	otPlatInfraIfRecvIcmp6Nd(
		ot_instance, ail_iface_index, &msg_ctx_ptr->addr,
		(const uint8_t *)&msg_ctx_ptr->buffer[sizeof(struct net_ipv6_hdr)],
		msg_ctx_ptr->length);
}

otError infra_if_start_icmp6_listener(void)
{
	otError error = OT_ERROR_NONE;

	VerifyOrExit(net_icmp_init_ctx(&ra_ctx, NET_ICMPV6_RA, 0, handle_icmp6_input) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(net_icmp_init_ctx(&rs_ctx, NET_ICMPV6_RS, 0, handle_icmp6_input) == 0,
		     error = OT_ERROR_FAILED);
	VerifyOrExit(net_icmp_init_ctx(&na_ctx, NET_ICMPV6_NA, 0, handle_icmp6_input) == 0,
		     error = OT_ERROR_FAILED);

exit:
	return error;
}

void infra_if_stop_icmp6_listener(void)
{
	(void)net_icmp_cleanup_ctx(&ra_ctx);
	(void)net_icmp_cleanup_ctx(&rs_ctx);
	(void)net_icmp_cleanup_ctx(&na_ctx);
}
