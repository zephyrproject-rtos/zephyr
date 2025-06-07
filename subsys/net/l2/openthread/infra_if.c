#include "openthread/platform/infra_if.h"
#include <zephyr/net/net_if.h>
#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <route.h>
#include "infra_if.h"
#include "ipv6.h"
#include "icmpv6.h"
#include <zephyr/net/openthread.h>
#include <zephyr/net/mld.h>

#include "openthread_utils.h"

static otInstance *ot_instance;

static void handle_ra_from_ot(const uint8_t *aBuffer, uint16_t aBufferLength);

otError otPlatInfraIfSendIcmp6Nd(uint32_t aInfraIfIndex, const otIp6Address *aDestAddress,
				 const uint8_t *aBuffer, uint16_t aBufferLength)
{
	otError error = OT_ERROR_NONE;

	struct net_pkt *pkt;
	struct in6_addr dst;
	const struct in6_addr *src;

	if (aBuffer[0] == 134) { // Router Advertisement
		handle_ra_from_ot(aBuffer, aBufferLength);
		net_ipv6_addr_create_ll_allnodes_mcast(&dst);
	} else {
		net_ipv6_addr_create_ll_allrouters_mcast(&dst);
	}

	src = net_if_ipv6_select_src_addr(net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET)),
					  &dst);

	pkt = net_pkt_alloc_with_buffer(net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET)),
					aBufferLength, AF_INET6, IPPROTO_ICMPV6, K_MSEC(100));
	if (!pkt) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	net_pkt_set_ipv6_hop_limit(pkt, NET_IPV6_ND_HOP_LIMIT);

	if (net_ipv6_create(pkt, src, &dst)) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

	net_pkt_write(pkt, aBuffer, aBufferLength);
	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, IPPROTO_ICMPV6);
	net_send_data(pkt);
exit:
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
	struct in6_addr *addr = {0};
	memcpy(addr->s6_addr, aAddress->mFields.m8, sizeof(otIp6Address));

	STRUCT_SECTION_FOREACH(net_if, tmp) {
		if (net_if_get_by_iface(tmp) != aInfraIfIndex) {
			continue;
		}
		ifaddr = net_if_ipv6_addr_lookup_by_iface(tmp, addr);
		if (ifaddr) {
			return 1;
		} else {
			return 0;
		}
	}
	return 0;
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

otError InfraIfInit(otInstance *aInstance)
{
	otError error = OT_ERROR_NONE;
	ot_instance = aInstance;

	struct in6_addr addr;
	int ret;
	net_ipv6_addr_create_ll_allrouters_mcast(&addr);
	ret = net_ipv6_mld_join(net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET)), &addr);
	if (ret < 0) {
		error = OT_ERROR_FAILED;
		goto exit;
	}

exit:
	return error;
}

static void handle_ra_from_ot(const uint8_t *aBuffer, uint16_t aBufferLength)
{

	struct net_if *eth_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	struct net_if *ot_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(OPENTHREAD));

	struct net_icmp_hdr *icmp_hdr = {0};
	struct net_icmpv6_ra_hdr *ra_hdr = {0};
	struct net_icmpv6_nd_opt_hdr *opt_hdr = {0};

	struct net_icmpv6_nd_opt_prefix_info *pio = {0};
	struct net_if_ipv6_prefix *prefix_added = {0};
	struct net_icmpv6_nd_opt_route_info *rio = {0};
	struct net_route_entry *route_added = {0};
	struct in6_addr rio_prefix = {0};

	struct net_if_addr *ifaddr = {0};
	struct in6_addr addr_to_add_from_pio = {0};

	struct in6_addr nexthop = {0};

	uint8_t i = 0;

	icmp_hdr = (struct net_icmp_hdr *)&aBuffer[i];
	i += sizeof(struct net_icmp_hdr);
	ra_hdr = (struct net_icmpv6_ra_hdr *)&aBuffer[i];
	i += sizeof(struct net_icmpv6_ra_hdr);

	while (i + sizeof(struct net_icmpv6_nd_opt_hdr) <= aBufferLength) {
		opt_hdr = (struct net_icmpv6_nd_opt_hdr *)&aBuffer[i];
		i += sizeof(struct net_icmpv6_nd_opt_hdr);
		switch (opt_hdr->type) {
		case NET_ICMPV6_ND_OPT_PREFIX_INFO:
			pio = (struct net_icmpv6_nd_opt_prefix_info *)&aBuffer[i];
			prefix_added =
				net_if_ipv6_prefix_add(eth_iface, (struct in6_addr *)pio->prefix,
						       pio->prefix_len, pio->valid_lifetime);
			// net_if_ipv6_prefix_set_lf(prefix, false);
			// net_if_ipv6_prefix_set_timer(prefix, prefix_info->valid_lifetime);
			i += sizeof(struct net_icmpv6_nd_opt_prefix_info);
			// check if autonomous ...
			net_ipv6_addr_generate_iid(
				eth_iface, (struct in6_addr *)pio->prefix,
				COND_CODE_1(CONFIG_NET_IPV6_IID_STABLE,
				     ((uint8_t *)&eth_iface->config.ip.ipv6->network_counter),
				     (NULL)), COND_CODE_1(CONFIG_NET_IPV6_IID_STABLE,
				     (sizeof(eth_iface->config.ip.ipv6->network_counter)),
				     (0U)), 0U, &addr_to_add_from_pio,
							       net_if_get_link_addr(eth_iface));
			ifaddr = net_if_ipv6_addr_lookup(&addr_to_add_from_pio, NULL);
			if (ifaddr) {
				net_if_addr_set_lf(ifaddr, true);
			}
			net_if_ipv6_addr_add(eth_iface, &addr_to_add_from_pio, NET_ADDR_AUTOCONF,
					     pio->valid_lifetime);
			break;
		case NET_ICMPV6_ND_OPT_ROUTE:
			uint8_t prefix_field_len = (opt_hdr->len - 1) * 8;
			rio = (struct net_icmpv6_nd_opt_route_info *)&aBuffer[i];
			i += sizeof(struct net_icmpv6_nd_opt_route_info);
			memcpy(&rio_prefix.s6_addr, &aBuffer[i], prefix_field_len);
			const otIp6Address *br_omr_addr = get_ot_slaac_address(ot_instance);
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
		}
	}
}