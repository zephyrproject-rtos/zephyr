/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "lldp-app"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>

#include <zephyr.h>
#include <errno.h>

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/ethernet.h>

/* User data for the interface callback */
struct ud {
	struct net_if *first;
	struct net_if *second;
	struct net_if *third;
};

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct ud *ud = user_data;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	if (!ud->first) {
		ud->first = iface;
		return;
	}

	if (!ud->second) {
		ud->second = iface;
		return;
	}

	if (!ud->third) {
		ud->third = iface;
		return;
	}
}

static int setup_iface(struct net_if *iface, const char *ipv6_addr,
		       const char *ipv4_addr, u16_t vlan_tag)
{
	struct net_if_addr *ifaddr;
	struct in_addr addr4;
	struct in6_addr addr6;
	int ret;

	ret = net_eth_vlan_enable(iface, vlan_tag);
	if (ret < 0) {
		NET_ERR("Cannot enable VLAN for tag %d (%d)", vlan_tag, ret);
	}

	if (net_addr_pton(AF_INET6, ipv6_addr, &addr6)) {
		NET_ERR("Invalid address: %s", ipv6_addr);
		return -EINVAL;
	}

	ifaddr = net_if_ipv6_addr_add(iface, &addr6, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		NET_ERR("Cannot add %s to interface %p", ipv6_addr, iface);
		return -EINVAL;
	}

	if (net_addr_pton(AF_INET, ipv4_addr, &addr4)) {
		NET_ERR("Invalid address: %s", ipv6_addr);
		return -EINVAL;
	}

	ifaddr = net_if_ipv4_addr_add(iface, &addr4, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		NET_ERR("Cannot add %s to interface %p", ipv4_addr, iface);
		return -EINVAL;
	}

	NET_DBG("Interface %p VLAN tag %d setup done.", iface, vlan_tag);

	return 0;
}

static struct ud ud;

static int init_vlan(void)
{
	int ret;

	(void)memset(&ud, 0, sizeof(ud));

	net_if_foreach(iface_cb, &ud);

	/* This sample has two VLANs. For the second one we need to manually
	 * create IP address for this test. But first the VLAN needs to be
	 * added to the interface so that IPv6 DAD can work properly.
	 */
	ret = setup_iface(ud.second,
			  CONFIG_NET_SAMPLE_IFACE2_MY_IPV6_ADDR,
			  CONFIG_NET_SAMPLE_IFACE2_MY_IPV4_ADDR,
			  CONFIG_NET_SAMPLE_IFACE2_VLAN_TAG);
	if (ret < 0) {
		return ret;
	}

	ret = setup_iface(ud.third,
			  CONFIG_NET_SAMPLE_IFACE3_MY_IPV6_ADDR,
			  CONFIG_NET_SAMPLE_IFACE3_MY_IPV4_ADDR,
			  CONFIG_NET_SAMPLE_IFACE3_VLAN_TAG);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static enum net_verdict parse_lldp(struct net_if *iface, struct net_pkt *pkt)
{
	size_t len = net_pkt_get_len(pkt);
	struct net_buf *frag = pkt->frags;
	u16_t pos = 0;

	NET_DBG("iface %p Parsing LLDP, len %u", iface, len);

	while (frag) {
		u16_t type_length;

		frag = net_frag_read_be16(frag, pos, &pos, &type_length);
		if (!frag) {
			if (type_length == 0) {
				NET_DBG("End LLDP DU TLV");
				break;
			}

			NET_ERR("Parsing ended, pos %u", pos);
			break;
		}

		u16_t length = type_length & 0x1FF;
		u8_t type = (u8_t)(type_length >> 9);

		/* Skip for now data */
		frag = net_frag_skip(frag, pos, &pos, length);

		switch (type) {
		case LLDP_TLV_CHASSIS_ID:
			NET_DBG("Chassis ID");
			break;
		case LLDP_TLV_PORT_ID:
			NET_DBG("Port ID");
			break;
		case LLDP_TLV_TTL:
			NET_DBG("TTL");
			break;
		default:
			NET_DBG("TLV Not parsed");
			break;
		}

		NET_DBG("type_length %u type %u length %u pos %u",
			type_length, type, length, pos);
	}

	/* Let stack to free the packet */
	return NET_DROP;
}

static int init_app(void)
{
	if (init_vlan() < 0) {
		NET_ERR("Cannot setup VLAN");
	}

	net_lldp_register_callback(ud.first, parse_lldp);

	return 0;
}

void main(void)
{
	/* The application will setup VLAN but does nothing meaningful.
	 * The configuration will enable LLDP support so you should see
	 * LLDPDU messages sent to the network interface.
	 */
	init_app();
}
