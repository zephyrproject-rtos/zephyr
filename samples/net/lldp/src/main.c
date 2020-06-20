/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_lldp_sample, LOG_LEVEL_DBG);

#include <zephyr.h>

#include <zephyr.h>
#include <errno.h>

#include <net/net_core.h>
#include <net/net_l2.h>
#include <net/net_if.h>
#include <net/ethernet.h>

static struct lldp_system_name_tlv {
	uint16_t type_length;
	uint8_t name[4];
} __packed tlv = {
	.name = { 't', 'e', 's', 't' },
};

static void set_optional_tlv(struct net_if *iface)
{
	NET_DBG("");

	tlv.type_length = htons((LLDP_TLV_SYSTEM_NAME << 9) |
				((sizeof(tlv) - sizeof(uint16_t)) & 0x01ff));

	net_lldp_config_optional(iface, (uint8_t *)&tlv, sizeof(tlv));
}

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
		       const char *ipv4_addr, uint16_t vlan_tag)
{
	struct net_if_addr *ifaddr;
	struct in_addr addr4;
	struct in6_addr addr6;
	int ret;

	ret = net_eth_vlan_enable(iface, vlan_tag);
	if (ret < 0) {
		LOG_ERR("Cannot enable VLAN for tag %d (%d)", vlan_tag, ret);
	}

	if (net_addr_pton(AF_INET6, ipv6_addr, &addr6)) {
		LOG_ERR("Invalid address: %s", ipv6_addr);
		return -EINVAL;
	}

	ifaddr = net_if_ipv6_addr_add(iface, &addr6, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		LOG_ERR("Cannot add %s to interface %p", ipv6_addr, iface);
		return -EINVAL;
	}

	if (net_addr_pton(AF_INET, ipv4_addr, &addr4)) {
		LOG_ERR("Invalid address: %s", ipv6_addr);
		return -EINVAL;
	}

	ifaddr = net_if_ipv4_addr_add(iface, &addr4, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		LOG_ERR("Cannot add %s to interface %p", ipv4_addr, iface);
		return -EINVAL;
	}

	LOG_DBG("Interface %p VLAN tag %d setup done.", iface, vlan_tag);

	return 0;
}

static struct ud ud;

static int init_vlan(void)
{
	enum ethernet_hw_caps caps;
	int ret;

	(void)memset(&ud, 0, sizeof(ud));

	net_if_foreach(iface_cb, &ud);

	caps = net_eth_get_hw_capabilities(ud.first);
	if (!(caps & ETHERNET_HW_VLAN)) {
		LOG_DBG("Interface %p does not support %s", ud.first, "VLAN");
		return -ENOENT;
	}

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
	LOG_DBG("iface %p Parsing LLDP, len %zu", iface, net_pkt_get_len(pkt));

	net_pkt_cursor_init(pkt);

	while (1) {
		uint16_t type_length;
		uint16_t length;
		uint8_t type;

		if (net_pkt_read_be16(pkt, &type_length)) {
			LOG_DBG("End LLDP DU TLV");
			break;
		}

		length = type_length & 0x1FF;
		type = (uint8_t)(type_length >> 9);

		/* Skip for now data */
		if (net_pkt_skip(pkt, length)) {
			LOG_DBG("");
			break;
		}

		switch (type) {
		case LLDP_TLV_CHASSIS_ID:
			LOG_DBG("Chassis ID");
			break;
		case LLDP_TLV_PORT_ID:
			LOG_DBG("Port ID");
			break;
		case LLDP_TLV_TTL:
			LOG_DBG("TTL");
			break;
		default:
			LOG_DBG("TLV Not parsed");
			break;
		}

		LOG_DBG("type_length %u type %u length %u",
			type_length, type, length);
	}

	/* Let stack to free the packet */
	return NET_DROP;
}

static int init_app(void)
{
	enum ethernet_hw_caps caps;
	int ret;

	ret = init_vlan();
	if (ret < 0) {
		LOG_WRN("Cannot setup VLAN (%d)", ret);
	}

	caps = net_eth_get_hw_capabilities(ud.first);
	if (!(caps & ETHERNET_LLDP)) {
		LOG_ERR("Interface %p does not support %s", ud.first, "LLDP");
		LOG_ERR("Cannot continue!");
		return -ENOENT;
	}

	set_optional_tlv(ud.first);
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
