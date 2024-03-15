/* main.c - Application main entry point */

/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_L2_VIRTUAL_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/random/random.h>

#include <zephyr/ztest.h>

#include <zephyr/net/dummy.h>
#include <zephyr/net/buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/virtual_mgmt.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_l2.h>

#include "ipv4.h"
#include "ipv6.h"
#include "udp_internal.h"

bool arp_add(struct net_if *iface, struct in_addr *src,
	     struct net_eth_addr *hwaddr);

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#define PKT_ALLOC_TIME K_MSEC(50)
#define TEST_PORT 9999

static char *test_data = "Test data to be sent";

/* Interface 1 addresses */
static struct in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct in_addr my_addr = { { { 192, 0, 2, 1 } } };

/* Interface 2 addresses */
static struct in6_addr my_addr2 = { { { 0x20, 0x01, 0x0d, 0xb8, 2, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Interface 3 addresses */
static struct in6_addr my_addr3 = { { { 0x20, 0x01, 0x0d, 0xb8, 3, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Extra address is assigned to ll_addr */
static struct in6_addr ll_addr = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
				       0x04 } } };

struct sockaddr virtual_addr;
struct sockaddr peer_addr;

#define MTU 1024

/* Keep track of all virtual interfaces */
static struct net_if *virtual_interfaces[1];
static struct net_if *eth_interfaces[2];
static struct net_if *dummy_interfaces[2];

static struct net_context *udp_ctx;

static bool test_failed;
static bool test_started;
static bool data_received;

static K_SEM_DEFINE(wait_data, 0, UINT_MAX);

#define WAIT_TIME K_SECONDS(1)

struct eth_context {
	struct net_if *iface;
	uint8_t mac_addr[6];
};

static struct eth_context eth_context;
static uint8_t expecting_outer;
static uint8_t expecting_inner;
static int header_len;

static void eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_context *context = dev->data;

	net_if_set_link_addr(iface, context->mac_addr,
			     sizeof(context->mac_addr),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_context *context = dev->data;

	zassert_equal_ptr(&eth_context, context,
			  "Context pointers do not match (%p vs %p)",
			  eth_context, context);

	if (!pkt->buffer) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	if (test_started) {
		uint8_t outer, inner;
		int ret;

		net_pkt_set_overwrite(pkt, true);

		net_pkt_hexdump(pkt, "pkt");
		net_pkt_skip(pkt, sizeof(struct net_eth_hdr));

		ret = net_pkt_read_u8(pkt, &outer);
		zassert_equal(ret, 0, "Cannot read outer protocol type");
		zassert_equal(outer, expecting_outer,
			      "Unexpected outer protocol 0x%02x, "
			      "expecting 0x%02x",
			      outer, expecting_outer);
		net_pkt_skip(pkt, header_len - 1);

		ret = net_pkt_read_u8(pkt, &inner);
		zassert_equal(ret, 0, "Cannot read inner protocol type");
		zassert_equal(inner, expecting_inner,
			      "Unexpected inner protocol 0x%02x, "
			      "expecting 0x%02x",
			      inner, expecting_inner);

		k_sem_give(&wait_data);
	}

	net_pkt_unref(pkt);

	return 0;
}

static enum ethernet_hw_caps eth_capabilities(const struct device *dev)
{
	return 0;
}

static struct ethernet_api api_funcs = {
	.iface_api.init = eth_iface_init,

	.get_capabilities = eth_capabilities,
	.send = eth_tx,
};

static void generate_mac(uint8_t *mac_addr)
{
	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	mac_addr[0] = 0x00;
	mac_addr[1] = 0x00;
	mac_addr[2] = 0x5E;
	mac_addr[3] = 0x00;
	mac_addr[4] = 0x53;
	mac_addr[5] = sys_rand32_get();
}

static int eth_init(const struct device *dev)
{
	struct eth_context *context = dev->data;

	generate_mac(context->mac_addr);

	return 0;
}

ETH_NET_DEVICE_INIT(eth_test, "eth_test",
		    eth_init, NULL,
		    &eth_context, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &api_funcs, NET_ETH_MTU);

struct net_if_test {
	uint8_t idx; /* not used for anything, just a dummy value */
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

static uint8_t *net_iface_get_mac(const struct device *dev)
{
	struct net_if_test *data = dev->data;

	if (data->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		data->mac_addr[0] = 0x00;
		data->mac_addr[1] = 0x00;
		data->mac_addr[2] = 0x5E;
		data->mac_addr[3] = 0x00;
		data->mac_addr[4] = 0x53;
		data->mac_addr[5] = sys_rand32_get();
	}

	data->ll_addr.addr = data->mac_addr;
	data->ll_addr.len = 6U;

	return data->mac_addr;
}

static void net_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_iface_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static int sender_iface(const struct device *dev, struct net_pkt *pkt)
{
	return 0;
}

struct net_if_test net_iface1_data;
struct net_if_test net_iface2_data;

static struct dummy_api net_iface_api = {
	.iface_api.init = net_iface_init,
	.send = sender_iface,
};

/* For testing purposes, create two dummy network interfaces so we can check
 * that attaching virtual interface work ok.
 */
NET_DEVICE_INIT_INSTANCE(eth_test_dummy1,
			 "iface1",
			 iface1,
			 NULL,
			 NULL,
			 &net_iface1_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 DUMMY_L2,
			 NET_L2_GET_CTX_TYPE(DUMMY_L2),
			 127);

NET_DEVICE_INIT_INSTANCE(eth_test_dummy2,
			 "iface2",
			 iface2,
			 NULL,
			 NULL,
			 &net_iface2_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 DUMMY_L2,
			 NET_L2_GET_CTX_TYPE(DUMMY_L2),
			 127);

struct user_data {
	int eth_if_count;
	int dummy_if_count;
	int virtual_if_count;
	int total_if_count;
};

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
static const char *iface2str(struct net_if *iface)
{
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		return "Ethernet";
	}

	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		return "Dummy";
	}

	if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
		return "Virtual";
	}

	return "<unknown type>";
}
#endif

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct user_data *ud = user_data;
	static int starting_eth_idx = 1;

	/*
	 * The below code is to only use struct net_if devices defined in this
	 * test as board on which it is run can have its own set of interfaces.
	 *
	 * As a result one will not rely on linker's specific 'net_if_area'
	 * placement.
	 */
	if ((iface != net_if_lookup_by_dev(DEVICE_GET(eth_test_dummy1))) &&
	    (iface != net_if_lookup_by_dev(DEVICE_GET(eth_test_dummy2))) &&
	    (iface != net_if_lookup_by_dev(DEVICE_GET(eth_test))) &&
	    (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)))
		return;

	DBG("Interface %p (%s) [%d]\n", iface, iface2str(iface),
	    net_if_get_by_iface(iface));

	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		if (PART_OF_ARRAY(NET_IF_GET_NAME(eth_test, 0), iface)) {
			if (!eth_interfaces[0]) {
				/* Just use the first interface */
				eth_interfaces[0] = iface;
				ud->eth_if_count++;
			}
		} else {
			if (ud->eth_if_count > ARRAY_SIZE(eth_interfaces)) {
				goto out;
			}

			eth_interfaces[starting_eth_idx++] = iface;
			ud->eth_if_count++;
		}
	}

	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		dummy_interfaces[ud->dummy_if_count++] = iface;

		zassert_true(ud->dummy_if_count <= 2,
			     "Too many dummy interfaces");
	}

	if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
		virtual_interfaces[ud->virtual_if_count++] = iface;

		zassert_true(ud->virtual_if_count <= 3,
			     "Too many virtual interfaces");
	} else {
		/* By default all interfaces are down initially */
		/* Virtual interfaces are down initially */
		net_if_down(iface);
	}

out:
	ud->total_if_count++;
}

static void test_virtual_setup(void)
{
	struct user_data ud = { 0 };

	/* Make sure we have enough virtual interfaces */
	net_if_foreach(iface_cb, &ud);

	zassert_equal(ud.virtual_if_count, ARRAY_SIZE(virtual_interfaces),
		      "Invalid number of virtual interfaces, "
		      "was %d should be %zu",
		      ud.virtual_if_count, ARRAY_SIZE(virtual_interfaces));

	zassert_true(ud.eth_if_count <= ARRAY_SIZE(eth_interfaces),
		      "Invalid number of eth interfaces, "
		      "was %d should be %zu",
		      ud.eth_if_count, ARRAY_SIZE(eth_interfaces));

	zassert_equal(ud.dummy_if_count, ARRAY_SIZE(dummy_interfaces),
		      "Invalid number of dummy interfaces, "
		      "was %d should be %zu",
		      ud.dummy_if_count, ARRAY_SIZE(dummy_interfaces));
}

static void test_address_setup(void)
{
	struct in_addr netmask = {{{ 255, 255, 255, 0 }}};
	struct net_if_addr *ifaddr;
	struct net_if *eth, *virt, *dummy1, *dummy2;
	int ret;

	eth = eth_interfaces[0];
	virt = virtual_interfaces[0];
	dummy1 = dummy_interfaces[0];
	dummy2 = dummy_interfaces[1];

	zassert_not_null(eth, "Eth Interface");
	zassert_not_null(virt, "Virtual Interface");
	zassert_not_null(dummy1, "Dummy Interface 1");
	zassert_not_null(dummy2, "Dummy Interface 2");

	ifaddr = net_if_ipv6_addr_add(eth, &my_addr1, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		    net_sprint_ipv6_addr(&my_addr1));
		zassert_not_null(ifaddr, "eth addr");
	}

	/* For testing purposes we need to set the addresses preferred */
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv4_addr_add(eth, &my_addr, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv4 address %s\n",
		    net_sprint_ipv4_addr(&my_addr));
		zassert_not_null(ifaddr, "eth addr");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_if_ipv4_set_netmask_by_addr(eth, &my_addr, &netmask);

	ifaddr = net_if_ipv6_addr_add(eth, &ll_addr, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		    net_sprint_ipv6_addr(&ll_addr));
		zassert_not_null(ifaddr, "ll_addr");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(virt, &my_addr2, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		    net_sprint_ipv6_addr(&my_addr2));
		zassert_not_null(ifaddr, "virt addr");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(dummy1, &my_addr3, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr3));
		zassert_not_null(ifaddr, "dummy1 addr");
	}

	net_if_up(eth);
	net_if_up(dummy1);
	net_if_up(dummy2);

	/* Set the virtual interface addresses */
	ret = net_ipaddr_parse(CONFIG_NET_TEST_TUNNEL_MY_ADDR,
			       strlen(CONFIG_NET_TEST_TUNNEL_MY_ADDR),
			       &virtual_addr);
	zassert_equal(ret, 1, "Cannot parse \"%s\"",
		      CONFIG_NET_TEST_TUNNEL_MY_ADDR);

	if (virtual_addr.sa_family == AF_INET) {
		ifaddr = net_if_ipv4_addr_add(virt,
					&net_sin(&virtual_addr)->sin_addr,
					NET_ADDR_MANUAL, 0);
		if (!ifaddr) {
			DBG("Cannot add IPv4 address %s\n",
			    net_sprint_ipv4_addr(
				    &net_sin(&virtual_addr)->sin_addr));
			zassert_not_null(ifaddr, "virt addr");
		}

		net_sin(&virtual_addr)->sin_port = htons(4242);

		net_if_ipv4_set_netmask_by_addr(virt,
						&net_sin(&virtual_addr)->sin_addr,
						&netmask);

	} else if (virtual_addr.sa_family == AF_INET6) {
		ifaddr = net_if_ipv6_addr_add(virt,
					&net_sin6(&virtual_addr)->sin6_addr,
					NET_ADDR_MANUAL, 0);
		if (!ifaddr) {
			DBG("Cannot add IPv6 address %s\n",
			    net_sprint_ipv6_addr(
				    &net_sin6(&virtual_addr)->sin6_addr));
			zassert_not_null(ifaddr, "virt addr");
		}

		net_sin6(&virtual_addr)->sin6_port = htons(4242);
	} else {
		zassert_not_null(NULL, "Invalid address family (%d)",
				 virtual_addr.sa_family);
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ret = net_ipaddr_parse(CONFIG_NET_TEST_TUNNEL_PEER_ADDR,
			       strlen(CONFIG_NET_TEST_TUNNEL_PEER_ADDR),
			       &peer_addr);
	zassert_equal(ret, 1, "Cannot parse \"%s\"",
		      CONFIG_NET_TEST_TUNNEL_PEER_ADDR);

	/* The interface might receive data which might fail the checks
	 * in the iface sending function, so we need to reset the failure
	 * flag.
	 */
	test_failed = false;
}

static bool add_neighbor(struct net_if *iface, struct in6_addr *addr)
{
	struct net_linkaddr_storage llstorage;
	struct net_linkaddr lladdr;
	struct net_nbr *nbr;

	llstorage.addr[0] = 0x01;
	llstorage.addr[1] = 0x02;
	llstorage.addr[2] = 0x33;
	llstorage.addr[3] = 0x44;
	llstorage.addr[4] = 0x05;
	llstorage.addr[5] = 0x06;

	lladdr.len = 6U;
	lladdr.addr = llstorage.addr;
	lladdr.type = NET_LINK_ETHERNET;

	nbr = net_ipv6_nbr_add(iface, addr, &lladdr, false,
			       NET_IPV6_NBR_STATE_REACHABLE);
	if (!nbr) {
		DBG("Cannot add dst %s to neighbor cache\n",
		    net_sprint_ipv6_addr(addr));
		return false;
	}

	return true;
}

static bool add_to_arp(struct net_if *iface, struct in_addr *addr)
{
#if defined(CONFIG_NET_ARP)
	struct net_eth_addr lladdr;

	lladdr.addr[0] = sys_rand32_get();
	lladdr.addr[1] = 0x08;
	lladdr.addr[2] = 0x09;
	lladdr.addr[3] = 0x10;
	lladdr.addr[4] = 0x11;
	lladdr.addr[5] = sys_rand32_get();

	return arp_add(iface, addr, &lladdr);
#else
	ARG_UNUSED(iface);
	ARG_UNUSED(addr);

	return true;
#endif
}

ZTEST(net_virtual, test_virtual_01_attach_and_detach)
{
	struct net_if *iface = virtual_interfaces[0];
	int ret;

	/* Attach virtual interface on top of Ethernet */

	ret = net_virtual_interface_attach(iface, eth_interfaces[0]);
	zassert_equal(ret, 0, "Cannot attach %d on top of %d (%d)",
		      net_if_get_by_iface(iface),
		      net_if_get_by_iface(eth_interfaces[0]),
		      ret);

	zassert_false(net_if_is_up(iface),
		      "Virtual interface %d should be down",
		      net_if_get_by_iface(iface));

	ret = net_if_up(iface);
	zassert_equal(ret, 0, "Cannot take virtual interface %d up (%d)",
		      net_if_get_by_iface(iface), ret);

	ret = net_virtual_interface_attach(iface,
					   NULL);
	zassert_equal(ret, 0, "Cannot deattach %d from %d (%d)",
		      net_if_get_by_iface(iface),
		      net_if_get_by_iface(eth_interfaces[0]),
		      ret);

	zassert_false(net_if_is_up(iface), "Virtual interface %d is still up",
		      net_if_get_by_iface(iface));
}

ZTEST(net_virtual, test_virtual_02_real_iface_down)
{
	struct net_if *iface = virtual_interfaces[0];
	int ret;

	/* Attach virtual interface on top of Ethernet */

	ret = net_virtual_interface_attach(iface, eth_interfaces[0]);
	zassert_equal(ret, 0, "Cannot attach %d on top of %d (%d)",
		      net_if_get_by_iface(iface),
		      net_if_get_by_iface(eth_interfaces[0]),
		      ret);

	zassert_false(net_if_is_up(iface),
		      "Virtual interface %d should be down",
		      net_if_get_by_iface(iface));

	ret = net_if_up(iface);
	zassert_equal(ret, 0, "Cannot take virtual interface %d up (%d)",
		      net_if_get_by_iface(iface), ret);

	zassert_true(net_if_is_up(iface),
		     "Virtual interface %d should be up",
		     net_if_get_by_iface(iface));
	zassert_true(net_if_is_up(eth_interfaces[0]),
		     "Real interface %d should be up",
		     net_if_get_by_iface(iface));

	/* Virtual interface should go down if the underlying iface is down */
	ret = net_if_down(eth_interfaces[0]);
	zassert_equal(ret, 0, "Cannot take real interface %d down (%d)",
		      net_if_get_by_iface(eth_interfaces[0]), ret);

	zassert_false(net_if_is_up(iface),
		      "Virtual interface %d should be down",
		      net_if_get_by_iface(iface));
	zassert_false(net_if_is_carrier_ok(iface),
		      "Virtual interface %d should be in carrier off",
		      net_if_get_by_iface(iface));
	zassert_equal(net_if_oper_state(iface), NET_IF_OPER_LOWERLAYERDOWN,
		      "Wrong operational state on %d (%d)",
		      net_if_get_by_iface(iface), net_if_oper_state(iface));

	/* Virtual interface should be brought up if the underlying iface is
	 * back up
	 */
	ret = net_if_up(eth_interfaces[0]);
	zassert_equal(ret, 0, "Cannot take real interface %d u (%d)",
		      net_if_get_by_iface(eth_interfaces[0]), ret);

	zassert_true(net_if_is_up(iface),
		     "Virtual interface %d should be up",
		     net_if_get_by_iface(iface));
	zassert_true(net_if_is_carrier_ok(iface),
		     "Virtual interface %d should be in carrier on",
		     net_if_get_by_iface(iface));

	ret = net_virtual_interface_attach(iface,
					   NULL);
	zassert_equal(ret, 0, "Cannot deattach %d from %d (%d)",
		      net_if_get_by_iface(iface),
		      net_if_get_by_iface(eth_interfaces[0]),
		      ret);

	zassert_false(net_if_is_up(iface), "Virtual interface %d is still up",
		      net_if_get_by_iface(iface));
}


ZTEST(net_virtual, test_virtual_03_set_mtu)
{
	struct virtual_interface_req_params params = { 0 };
	struct net_if *iface = virtual_interfaces[0];
	int ret;

	ret = net_if_up(iface);
	zassert_equal(ret, 0, "Cannot take virtual interface %d up (%d)",
		      net_if_get_by_iface(iface), ret);

	params.mtu = MTU;

	ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_MTU,
		       iface, &params, sizeof(params));
	zassert_equal(ret, -EACCES, "Could set interface %d MTU to %d (%d)",
		      net_if_get_by_iface(iface), params.mtu, ret);

	ret = net_if_down(iface);
	zassert_equal(ret, 0, "Cannot take virtual interface %d down (%d)",
		      net_if_get_by_iface(iface), ret);

	ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_MTU,
		       iface, &params, sizeof(params));
	zassert_equal(ret, 0, "Cannot set interface %d MTU to %d (%d)",
		      net_if_get_by_iface(iface), params.mtu, ret);
}

ZTEST(net_virtual, test_virtual_04_get_mtu)
{
	struct virtual_interface_req_params params = { 0 };
	struct net_if *iface = virtual_interfaces[0];
	int ret;

	params.mtu = 0;

	ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_GET_MTU,
		       iface, &params, sizeof(params));
	zassert_equal(ret, 0, "Cannot get interface %d MTU (%d)",
		      net_if_get_by_iface(iface), params.mtu, ret);

	zassert_equal(params.mtu, MTU,
		      "MTU mismatch from interface %d, got %d should be %d",
		      net_if_get_by_iface(iface), params.mtu, MTU);
}

ZTEST(net_virtual, test_virtual_05_set_peer)
{
	struct virtual_interface_req_params params = { 0 };
	struct net_if *iface = virtual_interfaces[0];
	int ret;

	ret = net_if_up(iface);
	zassert_equal(ret, 0, "Cannot take virtual interface %d up (%d)",
		      net_if_get_by_iface(iface), ret);

	params.family = peer_addr.sa_family;
	if (params.family == AF_INET) {
		net_ipaddr_copy(&params.peer4addr,
				&net_sin(&peer_addr)->sin_addr);
	} else if (params.family == AF_INET6) {
		net_ipaddr_copy(&params.peer6addr,
				&net_sin6(&peer_addr)->sin6_addr);
	} else {
		zassert_true(false, "Invalid family (%d)", params.family);
	}

	ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PEER_ADDRESS,
		       iface, &params, sizeof(params));
	zassert_equal(ret, -EACCES, "Could set interface %d peer to %s (%d)",
		      net_if_get_by_iface(iface),
		      CONFIG_NET_TEST_TUNNEL_PEER_ADDR, ret);

	ret = net_if_down(iface);
	zassert_equal(ret, 0, "Cannot take virtual interface %d down (%d)",
		      net_if_get_by_iface(iface), ret);

	ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PEER_ADDRESS,
		       iface, &params, sizeof(params));
	zassert_equal(ret, 0, "Cannot set interface %d peer to %s (%d)",
		      net_if_get_by_iface(iface),
		      CONFIG_NET_TEST_TUNNEL_PEER_ADDR, ret);

	/* We should be attached now */
	ret = net_virtual_interface_attach(iface, dummy_interfaces[0]);
	zassert_equal(ret, -EALREADY, "Could attach %d on top of %d (%d)",
		      net_if_get_by_iface(iface),
		      net_if_get_by_iface(dummy_interfaces[0]),
		      ret);
}

ZTEST(net_virtual, test_virtual_06_get_peer)
{
	struct virtual_interface_req_params params = { 0 };
	struct net_if *iface = virtual_interfaces[0];
	int ret;

	ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_GET_PEER_ADDRESS,
		       iface, &params, sizeof(params));
	zassert_equal(ret, 0, "Cannot get interface %d peer (%d)",
		      net_if_get_by_iface(iface), ret);

	zassert_equal(params.family, peer_addr.sa_family,
		      "Invalid family, should be %d was %d",
		      peer_addr.sa_family, params.family);
	if (params.family == AF_INET) {
		zassert_mem_equal(&params.peer4addr,
				  &net_sin(&peer_addr)->sin_addr,
				  sizeof(struct in_addr),
				  "Peer IPv4 address invalid");
	} else if (params.family == AF_INET6) {
		zassert_mem_equal(&params.peer6addr,
				  &net_sin6(&peer_addr)->sin6_addr,
				  sizeof(struct in6_addr),
				  "Peer IPv6 address invalid");
	} else {
		zassert_true(false, "Invalid family (%d)", params.family);
	}
}

ZTEST(net_virtual, test_virtual_07_verify_name)
{
#define NAME "foobar"
#define NAME2 "123456789"
	struct net_if *iface = virtual_interfaces[0];
	char *tmp = NAME;
	char buf[sizeof(NAME2)];
	char *name;

	net_virtual_set_name(iface, NAME);
	name = net_virtual_get_name(iface, buf, sizeof(buf));
	zassert_mem_equal(name, tmp, strlen(name), "Cannot get name");

	/* Check that the string is truncated */
	tmp = NAME2;
	net_virtual_set_name(iface, tmp);
	name = net_virtual_get_name(iface, buf, sizeof(buf));
	zassert_mem_equal(name, tmp, strlen(name), "Cannot get name");
	zassert_mem_equal(name, tmp, strlen(tmp) -
			  (sizeof(NAME2) - CONFIG_NET_L2_VIRTUAL_MAX_NAME_LEN),
			  "Cannot get name");
}

ZTEST(net_virtual, test_virtual_08_send_data_to_tunnel)
{
	struct virtual_interface_req_params params = { 0 };
	struct net_if *iface = virtual_interfaces[0];
	struct net_if *attached;
	struct sockaddr dst_addr, src_addr;
	void *addr;
	int addrlen;
	int ret;

	params.family = peer_addr.sa_family;
	if (params.family == AF_INET) {
		net_ipaddr_copy(&params.peer4addr,
				&net_sin(&peer_addr)->sin_addr);
		expecting_outer = 0x45;
		header_len = sizeof(struct net_ipv4_hdr);

		ret = add_to_arp(eth_interfaces[0],
				 &net_sin(&peer_addr)->sin_addr);
		zassert_true(ret, "Cannot add to arp");
	} else if (params.family == AF_INET6) {
		net_ipaddr_copy(&params.peer6addr,
				&net_sin6(&peer_addr)->sin6_addr);
		expecting_outer = 0x60;
		header_len = sizeof(struct net_ipv6_hdr);

		ret = add_neighbor(eth_interfaces[0],
				   &net_sin6(&peer_addr)->sin6_addr);
		zassert_true(ret, "Cannot add neighbor");
	} else {
		zassert_true(false, "Invalid family (%d)", params.family);
	}

	ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PEER_ADDRESS,
		       iface, &params, sizeof(params));
	zassert_equal(ret, 0, "Cannot set interface %d peer to %s (%d)",
		      net_if_get_by_iface(iface),
		      CONFIG_NET_TEST_TUNNEL_PEER_ADDR, ret);

	net_virtual_set_name(iface, CONFIG_NET_TEST_TUNNEL_NAME);

	attached = net_virtual_get_iface(iface);
	zassert_equal(eth_interfaces[0], attached,
		      "Not attached to Ethernet interface");

	ret = net_if_up(iface);
	zassert_equal(ret, 0, "Cannot take virtual interface %d up (%d)",
		      net_if_get_by_iface(iface), ret);

	memcpy(&dst_addr, &virtual_addr, sizeof(dst_addr));
	memcpy(&src_addr, &virtual_addr, sizeof(src_addr));

	if (dst_addr.sa_family == AF_INET) {
		net_sin(&dst_addr)->sin_addr.s4_addr[3] = 2;

		addr = &src_addr;
		addrlen = sizeof(struct sockaddr_in);

		expecting_inner = 0x45; /* IPv4 */

	} else if (dst_addr.sa_family == AF_INET6) {
		net_sin6(&dst_addr)->sin6_addr.s6_addr[15] = 2;

		addr = &src_addr;
		addrlen = sizeof(struct sockaddr_in6);

		expecting_inner = 0x60; /* IPv6 */
	} else {
		zassert_true(false, "Invalid family (%d)", dst_addr.sa_family);
		addrlen = 0;
	}

	ret = net_context_get(virtual_addr.sa_family, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_ctx);
	zassert_equal(ret, 0, "Create IP UDP context failed");

	ret = net_context_bind(udp_ctx, (struct sockaddr *)addr, addrlen);
	zassert_equal(ret, 0, "Context bind failure test failed");

	test_started = true;

	ret = net_context_sendto(udp_ctx, test_data, strlen(test_data),
				 &dst_addr, addrlen,
				 NULL, K_NO_WAIT, NULL);
	zassert_true(ret > 0, "Send UDP pkt failed");

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout");
	}

	net_context_unref(udp_ctx);
}

static struct net_pkt *create_outer(struct net_if *iface,
				    sa_family_t family,
				    enum net_ip_protocol proto,
				    size_t inner_len,
				    size_t outer_len)
{
	return net_pkt_alloc_with_buffer(iface, inner_len + outer_len,
					 family, proto, PKT_ALLOC_TIME);
}

static struct net_pkt *create_inner(struct net_if *iface,
				    sa_family_t family,
				    enum net_ip_protocol proto,
				    size_t inner_len,
				    size_t data_len)
{
	return net_pkt_alloc_with_buffer(iface, inner_len + data_len,
					 family, proto, PKT_ALLOC_TIME);
}

static void recv_data(struct net_context *context,
		      struct net_pkt *pkt,
		      union net_ip_header *ip_hdr,
		      union net_proto_header *proto_hdr,
		      int status,
		      void *user_data)
{
	data_received = true;
}

static void test_virtual_recv_data_from_tunnel(int remote_ip,
					       bool expected_ok)
{
	struct net_if *iface = virtual_interfaces[0];
	struct net_if *attached = eth_interfaces[0];
	struct sockaddr dst_addr, src_addr, inner_src;
	struct in_addr *outerv4, *innerv4;
	struct in6_addr *outerv6, *innerv6;
	size_t inner_len = sizeof(struct net_udp_hdr) +
		strlen(test_data);
	struct net_pkt *outer, *inner;
	enum net_verdict verdict;
	uint16_t src_port = 4242, dst_port = 4242;
	uint8_t next_header;
	size_t addrlen;
	int ret;

	memcpy(&dst_addr, &peer_addr, sizeof(dst_addr));
	memcpy(&src_addr, &peer_addr, sizeof(src_addr));
	memcpy(&inner_src, &virtual_addr, sizeof(inner_src));

	if (peer_addr.sa_family == AF_INET) {
		net_sin(&dst_addr)->sin_addr.s4_addr[3] = 1;
		net_sin(&src_addr)->sin_addr.s4_addr[3] = remote_ip;
		outerv4 = &net_sin(&peer_addr)->sin_addr;
	} else {
		net_sin6(&dst_addr)->sin6_addr.s6_addr[15] = 1;
		net_sin6(&src_addr)->sin6_addr.s6_addr[15] = remote_ip;
		outerv6 = &net_sin6(&peer_addr)->sin6_addr;
	}

	if (virtual_addr.sa_family == AF_INET) {
		net_sin(&inner_src)->sin_addr.s4_addr[3] = 2;
		innerv4 = &net_sin(&virtual_addr)->sin_addr;
		inner_len += sizeof(struct net_ipv4_hdr);
	} else {
		net_sin6(&inner_src)->sin6_addr.s6_addr[15] = 2;
		innerv6 = &net_sin6(&virtual_addr)->sin6_addr;
		inner_len += sizeof(struct net_ipv6_hdr);
	}

	if (peer_addr.sa_family == AF_INET) {
		outer = create_outer(attached, AF_INET, IPPROTO_IP,
				     sizeof(struct net_ipv4_hdr), 0);
		zassert_not_null(outer, "Cannot allocate %s pkt", outer);

		ret = net_ipv4_create(outer, &net_sin(&src_addr)->sin_addr,
				      &net_sin(&dst_addr)->sin_addr);
		zassert_equal(ret, 0, "Cannot create %s packet (%d)", "IPv4",
			      ret);
	} else {
		outer = create_outer(attached, AF_INET6, IPPROTO_IPV6,
				     sizeof(struct net_ipv6_hdr), 0);
		zassert_not_null(outer, "Cannot allocate %s pkt", outer);

		ret = net_ipv6_create(outer, &net_sin6(&src_addr)->sin6_addr,
				      &net_sin6(&dst_addr)->sin6_addr);
		zassert_equal(ret, 0, "Cannot create %s packet (%d)", "IPv6",
			      ret);
	}

	if (virtual_addr.sa_family == AF_INET) {
		inner = create_inner(iface, AF_INET, IPPROTO_IP,
				     sizeof(struct net_ipv4_hdr),
				     sizeof(struct net_udp_hdr) +
				     strlen(test_data));
		zassert_not_null(inner, "Cannot allocate %s pkt", inner);

		ret = net_ipv4_create(inner, &net_sin(&inner_src)->sin_addr,
				      innerv4);
		zassert_equal(ret, 0, "Cannot create outer %s (%d)", "IPv4",
			      ret);
		next_header = IPPROTO_IPIP;
		addrlen = sizeof(struct sockaddr_in);
	} else {
		inner = create_inner(iface, AF_INET6, IPPROTO_IPV6,
				     sizeof(struct net_ipv6_hdr),
				     sizeof(struct net_udp_hdr) +
				     strlen(test_data));
		zassert_not_null(inner, "Cannot allocate %s pkt", inner);

		ret = net_ipv6_create(inner, &net_sin6(&inner_src)->sin6_addr,
				      innerv6);
		zassert_equal(ret, 0, "Cannot create outer %s (%d)", "IPv6",
			      ret);
		next_header = IPPROTO_IPV6;
		addrlen = sizeof(struct sockaddr_in6);
	}

	ret = net_udp_create(inner, htons(src_port), htons(dst_port));
	zassert_equal(ret, 0, "Cannot create UDP (%d)", ret);

	net_pkt_write(inner, test_data, strlen(test_data));

	net_pkt_cursor_init(inner);
	net_ipv4_finalize(inner, IPPROTO_UDP);

	net_buf_frag_add(outer->buffer, inner->buffer);
	inner->buffer = NULL;
	net_pkt_unref(inner);

	net_pkt_cursor_init(outer);

	if (peer_addr.sa_family == AF_INET) {
		net_ipv4_finalize(outer, next_header);
	} else {
		net_ipv6_finalize(outer, next_header);
	}

	ret = net_context_get(virtual_addr.sa_family, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_ctx);
	zassert_equal(ret, 0, "Create IP UDP context failed");

	net_context_set_iface(udp_ctx, iface);

	ret = net_context_bind(udp_ctx, (struct sockaddr *)&virtual_addr,
			       addrlen);
	zassert_equal(ret, 0, "Context bind failure test failed");

	test_started = true;
	data_received = false;

	ret = net_context_recv(udp_ctx, recv_data, K_NO_WAIT, &wait_data);
	zassert_equal(ret, 0, "UDP recv failed");

	net_pkt_cursor_init(outer);

	if (peer_addr.sa_family == AF_INET) {
		verdict = net_ipv4_input(outer, false);
	} else {
		verdict = net_ipv6_input(outer, false);
	}

	if (expected_ok) {
		zassert_equal(verdict, NET_CONTINUE,
			      "Packet not accepted (%d)",
			      verdict);
	} else {
		zassert_equal(verdict, NET_DROP,
			      "Packet not dropped (%d)",
			      verdict);
	}

	net_context_put(udp_ctx);
}

ZTEST(net_virtual, test_virtual_09_recv_data_from_tunnel_ok)
{
	test_virtual_recv_data_from_tunnel(2, true);
}

ZTEST(net_virtual, test_virtual_10_recv_data_from_tunnel_fail)
{
	test_virtual_recv_data_from_tunnel(3, false);
}

static void *setup(void)
{
	test_virtual_setup();
	test_address_setup();
	return NULL;
}

ZTEST_SUITE(net_virtual, NULL, setup, NULL, NULL, NULL);
