/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <ztest.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <linker/sections.h>

#include <tc_util.h>

#include <net/ethernet.h>
#include <net/buf.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/net_context.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"
#include "ipv6.h"
#include "icmpv6.h"
#include "nbr.h"
#include "rpl.h"

#if defined(CONFIG_NET_DEBUG_RPL)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static struct net_context *udp_ctx;

static struct in6_addr in6addr_my = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct in6_addr peer_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					 0, 0, 0, 0, 0, 0, 0, 0x2 } } };
static struct in6_addr in6addr_ll = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
					  0x04 } } };
static struct in6_addr in6addr_mcast = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					     0, 0, 0, 0, 0, 0, 0, 0x1 } } };

static struct net_linkaddr_storage lladdr_src_storage = {
	.addr = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 },
	.len = NET_LINK_ADDR_MAX_LENGTH
};
static struct net_linkaddr lladdr_src = {
	.addr = lladdr_src_storage.addr,
	.len = NET_LINK_ADDR_MAX_LENGTH
};

static bool test_failed;
static bool data_failure;
static bool feed_data; /* feed data back to IP stack */

static int msg_sending;
static int expected_icmpv6 = NET_ICMPV6_RPL;

static struct k_sem wait_data;

static struct net_if_link_cb link_cb;
static bool link_cb_called;

#define WAIT_TIME 250

struct net_rpl_test {
	u8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_rpl_dev_init(struct device *dev)
{
	return 0;
}

static u8_t *net_rpl_get_mac(struct device *dev)
{
	struct net_rpl_test *rpl = dev->driver_data;

	if (rpl->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		rpl->mac_addr[0] = 0x00;
		rpl->mac_addr[1] = 0x00;
		rpl->mac_addr[2] = 0x5E;
		rpl->mac_addr[3] = 0x00;
		rpl->mac_addr[4] = 0x53;
		rpl->mac_addr[5] = sys_rand32_get();
	}

	return rpl->mac_addr;
}

static void net_rpl_iface_init(struct net_if *iface)
{
	u8_t *mac = net_rpl_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static void set_pkt_ll_addr(struct device *dev, struct net_pkt *pkt)
{
	struct net_rpl_test *rpl = dev->driver_data;

	struct net_linkaddr *src = net_pkt_lladdr_src(pkt);
	struct net_linkaddr *dst = net_pkt_lladdr_dst(pkt);

	dst->len = lladdr_src.len;
	dst->addr = lladdr_src.addr;

	src->len = sizeof(rpl->mac_addr);
	src->addr = rpl->mac_addr;
}

#define NET_ICMP_HDR(pkt) ((struct net_icmp_hdr *)net_pkt_icmp_data(pkt))

static int tester_send(struct net_if *iface, struct net_pkt *pkt)
{
	if (!pkt->frags) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	set_pkt_ll_addr(net_if_get_device(iface), pkt);

	/* By default we assume that the test is ok */
	data_failure = false;

	if (feed_data) {
		net_pkt_lladdr_swap(pkt);

		if (net_recv_data(iface, pkt) < 0) {
			TC_ERROR("Data receive failed.");
			net_pkt_unref(pkt);
			test_failed = true;
		}

		k_sem_give(&wait_data);

		return 0;
	}

	DBG("pkt %p to be sent len %lu\n", pkt, net_pkt_get_len(pkt));

#if defined(CONFIG_NET_DEBUG_RPL)
	net_hexdump_frags("recv", pkt, false);
#endif

	if (NET_ICMP_HDR(pkt)->type != expected_icmpv6) {
		DBG("ICMPv6 type %d, expected %d\n",
		    NET_ICMP_HDR(pkt)->type, expected_icmpv6);

		data_failure = true;
	}

	/* If we are not sending what is expected, then mark it as a failure
	 */
	if (msg_sending) {
		if (msg_sending != NET_ICMP_HDR(pkt)->code) {
			DBG("Received code %d, expected %d\n",
			    NET_ICMP_HDR(pkt)->code, msg_sending);

			data_failure = true;
		} else {
			/* Pass sent DIO message back to us */
			if (msg_sending == NET_RPL_DODAG_INFO_OBJ) {
				net_pkt_lladdr_swap(pkt);

				if (!net_recv_data(iface, pkt)) {
					/* We must not unref the msg,
					 * as it should be unfreed by
					 * the upper stack.
					 */
					goto out;
				}
			}
		}
	}

	net_pkt_unref(pkt);

out:
	if (data_failure) {
		test_failed = true;
	}

	msg_sending = 0;

	k_sem_give(&wait_data);

	return 0;
}

struct net_rpl_test net_rpl_data;

static struct net_if_api net_rpl_if_api = {
	.init = net_rpl_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(net_rpl_test, "net_rpl_test",
		net_rpl_dev_init, &net_rpl_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_rpl_if_api, _ETH_L2_LAYER,
		_ETH_L2_CTX_TYPE, 127);

static void send_link_cb(struct net_if *iface, struct net_linkaddr *lladdr,
			 int status)
{
	link_cb_called = true;
}

static void test_init(void)
{
	struct net_if_addr *ifaddr;
	struct net_if_mcast_addr *maddr;
	struct net_if *iface = net_if_get_default();
	struct net_rpl_dag *dag;

	zassert_not_null(iface, "Interface is NULL");

	ifaddr = net_if_ipv6_addr_add(iface, &in6addr_my,
				      NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Cannot add IPv6 address");

	/* For testing purposes we need to set the adddresses preferred */
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface, &in6addr_ll,
				      NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr,
			"Cannot add IPv6 address");

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_ipv6_addr_create(&in6addr_mcast, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001);

	maddr = net_if_ipv6_maddr_add(iface, &in6addr_mcast);

	zassert_not_null(maddr, "Cannot add multicast IPv6 address");

	/* The semaphore is there to wait the data to be received. */
	k_sem_init(&wait_data, 0, UINT_MAX);

	net_if_register_link_cb(&link_cb, send_link_cb);

	/* Creating a new RPL DAG */
	net_rpl_set_root(iface, NET_RPL_DEFAULT_INSTANCE, &in6addr_my);
	dag = net_rpl_get_any_dag();
	net_rpl_set_prefix(iface, dag, &in6addr_my, 64);
}

static void net_ctx_create(void)
{
	int ret;

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_ctx);
	zassert_equal(ret, 0,
			"Context create IPv6 UDP test failed");
}

static void test_rpl_mcast_addr(void)
{
	struct in6_addr rpl_mcast = { { { 0xff, 0x02, 0, 0, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x1a } } };
	struct in6_addr addr;
	bool ret;

	ret = net_rpl_is_ipv6_addr_mcast(&rpl_mcast);
	zassert_true(ret, "RPL multicast address check failed.");

	net_rpl_create_mcast_address(&addr);

	ret = net_rpl_is_ipv6_addr_mcast(&addr);
	zassert_true(ret, "Generated RPL multicast address check failed.");
}

static void test_dio_dummy_input(void)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	int ret;

	pkt = net_pkt_get_tx(udp_ctx, K_FOREVER);
	frag = net_pkt_get_data(udp_ctx, K_FOREVER);

	net_pkt_frag_add(pkt, frag);

	msg_sending = NET_RPL_DODAG_INFO_OBJ;

	set_pkt_ll_addr(net_if_get_device(net_if_get_default()), pkt);

	ret = net_icmpv6_input(pkt, NET_ICMPV6_RPL, msg_sending);
	if (!ret) {
		zassert_true(0, "Callback is not called properly");
	}

	data_failure = false;
	k_sem_take(&wait_data, WAIT_TIME);

	zassert_false(data_failure,
			"Unexpected ICMPv6 code received");

	data_failure = false;
}

static void test_dis_sending(void)
{
	struct net_if *iface;
	int ret;

	iface = net_if_get_default();

	msg_sending = NET_RPL_DODAG_SOLICIT;

	ret = net_rpl_dis_send(NULL, iface);
	if (ret) {
		zassert_true(0, "Cannot send DIS");
	}

	k_sem_take(&wait_data, WAIT_TIME);

	if (data_failure) {
		data_failure = false;
		zassert_true(0, "Unexpected ICMPv6 code received");
	}

	data_failure = false;
}

static void test_dao_sending_fail(void)
{
	struct net_if *iface = NULL, *iface_def;
	struct in6_addr *prefix, *prefix2;
	struct net_rpl_instance instance = {
		.instance_id = 42,
	};
	struct net_rpl_dag dag = {
		.dag_id = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0x2 } } },
		.instance = &instance,
	};
	struct net_rpl_parent parent = {
		.dag = &dag,
	};
	int ret;

	iface_def = net_if_get_default();
	prefix2 = net_if_ipv6_get_global_addr(&iface_def);

	prefix = net_if_ipv6_get_global_addr(&iface);
	zassert_not_null(prefix,
			"Will not send DAO as no global address was found");

	zassert_equal_ptr(iface, iface_def,
			"Network interface mismatch");

	zassert_equal_ptr(prefix, prefix2,
				"Network interface mismatch or not set");

	msg_sending = NET_RPL_DEST_ADV_OBJ;

	/* The sending should fail at this point because the neighbor
	 * is not suppose to be found in neighbor cache.
	 */
	ret = net_rpl_dao_send(iface, &parent, prefix, 100);
	if (!ret) {
		zassert_true(0, "DAO send succeed but should not have");
	}
}

static bool net_test_send_ns(void)
{
	struct net_if *iface = net_if_get_default();
	struct net_nbr *nbr;
	int ret;

	/* As we are sending a node reachability NS (RFC 4861 ch 4.3),
	 * we need to add the neighbor to the cache, otherwise we cannot
	 * send a NS with unicast destination address.
	 */
	nbr = net_ipv6_nbr_add(iface,
			       &in6addr_my,
			       net_if_get_link_addr(iface),
			       false,
			       NET_IPV6_NBR_STATE_REACHABLE);
	if (!nbr) {
		TC_ERROR("Cannot add to neighbor cache\n");
		return false;
	}

	ret = net_ipv6_send_ns(iface,
			       NULL,
			       &peer_addr,
			       &in6addr_my,
			       &in6addr_my,
			       false);
	if (ret < 0) {
		TC_ERROR("Cannot send NS (%d)\n", ret);
		return false;
	}

	return true;
}

static bool net_test_nbr_lookup_ok(void)
{
	struct net_linkaddr_storage *llstorage;
	struct net_nbr *nbr;

	nbr = net_ipv6_nbr_lookup(net_if_get_default(),
				  &peer_addr);
	if (!nbr) {
		TC_ERROR("Neighbor %s not found in cache\n",
			 net_sprint_ipv6_addr(&peer_addr));
		return false;
	}

	/* Set the ll address in the neighbor so that following
	 * tests work ok.
	 */
	llstorage = net_nbr_get_lladdr(nbr->idx);
	memcpy(llstorage->addr, lladdr_src.addr, lladdr_src.len);
	llstorage->len = lladdr_src.len;

	DBG("[%d] Neighbor %s lladdr %s\n", nbr->idx,
	    net_sprint_ipv6_addr(&peer_addr),
	    net_sprint_ll_addr(llstorage->addr, llstorage->len));

	net_ipv6_nbr_data(nbr)->state = NET_IPV6_NBR_STATE_REACHABLE;

	return true;
}

static void populate_nbr_cache(void)
{
	struct net_nbr *nbr;

	msg_sending = NET_ICMPV6_NS;
	feed_data = true;
	data_failure = false;

	zassert_true(net_test_send_ns(), NULL);

	k_sem_take(&wait_data, WAIT_TIME);

	feed_data = false;

	zassert_false(data_failure, NULL);

	data_failure = false;

	nbr = net_ipv6_nbr_add(net_if_get_default(),
			       &peer_addr,
			       &lladdr_src,
			       false,
			       NET_IPV6_NBR_STATE_REACHABLE);
	zassert_not_null(nbr, "Cannot add peer to neighbor cache");

	zassert_true(net_test_nbr_lookup_ok(), NULL);
}
#if 0
/* This test is currently disabled as it needs more TLC */
static bool test_dao_sending_ok(void)
{
	struct net_if *iface = NULL, *iface_def;
	struct in6_addr *prefix, *prefix2;
	struct net_rpl_dag *dag;
	int ret;

	iface_def = net_if_get_default();
	prefix2 = net_if_ipv6_get_global_addr(&iface_def);

	prefix = net_if_ipv6_get_global_addr(&iface);
	if (!prefix) {
		TC_ERROR("Will not send DAO as no global address was found.");
		return false;
	}

	if (iface != iface_def) {
		TC_ERROR("Network interface mismatch (%p vs %p)\n",
			 iface, iface_def);
		return false;
	}

	if (prefix != prefix2) {
		TC_ERROR("Network interface mismatch or not set (%p vs %p)\n",
			 prefix, prefix2);
		return false;
	}

	msg_sending = NET_RPL_DEST_ADV_OBJ;

	net_rpl_set_root(iface_def, NET_RPL_DEFAULT_INSTANCE, prefix);

	dag = net_rpl_get_any_dag();

	if (!net_rpl_set_prefix(iface, dag, &prefix, 64)) {
		TC_ERROR("Failed to create a new RPL DAG\n");
		return false;
	}

	ret = net_rpl_dao_send(iface, dag->preferred_parent, prefix, 100);
	if (ret) {
		TC_ERROR("%d: Cannot send DAO (%d)\n", __LINE__, ret);
		return false;
	}

	k_sem_take(&wait_data, WAIT_TIME);

	if (data_failure) {
		data_failure = false;
		TC_ERROR("%d: Unexpected ICMPv6 code received\n", __LINE__);
		return false;
	}

	data_failure = false;

	return true;
}

/* This test fails currently, it needs more TLC */
static bool test_link_cb(void)
{
	link_cb_called = false;
	msg_sending = 0;
	expected_icmpv6 = NET_ICMPV6_NS;

	net_test_send_ns();

	k_sem_take(&wait_data, WAIT_TIME);

	/* Restore earlier expected value, by default we only accept
	 * RPL ICMPv6 messages.
	 */
	expected_icmpv6 = NET_ICMPV6_RPL;

	if (!link_cb_called) {
		TC_ERROR("%d: Link cb not called\n", __LINE__);
		return false;
	}

	return true;
}
#endif

static void test_dio_receive_dest(void)
{
	struct net_if *iface = NULL, *iface_def;
	struct in6_addr *prefix, *prefix2;
	struct net_rpl_instance instance = {
		.instance_id = CONFIG_NET_RPL_DEFAULT_INSTANCE,
		.mop = NET_RPL_MOP_STORING_NO_MULTICAST,
		.min_hop_rank_inc = 100,
		.ocp = 1, /* MRH OF */
	};
	struct net_rpl_dag dag = {
		.dag_id = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0x1 } } },
		.instance = &instance,
		.prefix_info = {
			.prefix = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0 } } },
			.length = 64,
		},
		.version = 1,
		.rank = 2,
	};
	int ret;

	instance.current_dag = &dag;

	iface_def = net_if_get_default();
	prefix2 = net_if_ipv6_get_global_addr(&iface_def);

	prefix = net_if_ipv6_get_global_addr(&iface);
	zassert_not_null(prefix,
			 "Will not send DAO as no global address was found.");

	zassert_equal_ptr(iface, iface_def,
			  "Network interface mismatch");

	zassert_equal_ptr(prefix, prefix2,
			  "Network interface mismatch or not set");

	msg_sending = NET_RPL_DODAG_INFO_OBJ;

	ret = net_rpl_dio_send(iface, &instance, &peer_addr, &in6addr_my);
	zassert_false(ret, "cannot send DIO");

	k_sem_take(&wait_data, WAIT_TIME);

	zassert_false(data_failure, "Unexpected ICMPv6 code received");

	data_failure = false;
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_rpl,
			ztest_unit_test(test_init),
			ztest_unit_test(net_ctx_create),
			ztest_unit_test(test_rpl_mcast_addr),
			ztest_unit_test(test_dio_dummy_input),
			ztest_unit_test(test_dis_sending),
			ztest_unit_test(test_dao_sending_fail),
			ztest_unit_test(populate_nbr_cache),
			ztest_unit_test(test_dio_receive_dest));
	ztest_run_test_suite(test_rpl);
}
