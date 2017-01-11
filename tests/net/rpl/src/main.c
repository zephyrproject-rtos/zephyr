/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <sections.h>

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
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_rpl_dev_init(struct device *dev)
{
	return 0;
}

static uint8_t *net_rpl_get_mac(struct device *dev)
{
	struct net_rpl_test *rpl = dev->driver_data;

	if (rpl->mac_addr[0] == 0x00) {
		/* 10-00-00-00-00 to 10-00-00-00-FF Documentation RFC7042 */
		rpl->mac_addr[0] = 0x10;
		rpl->mac_addr[1] = 0x00;
		rpl->mac_addr[2] = 0x00;
		rpl->mac_addr[3] = 0x00;
		rpl->mac_addr[4] = 0x00;
		rpl->mac_addr[5] = sys_rand32_get();
	}

	return rpl->mac_addr;
}

static void net_rpl_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_rpl_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr));
}

static void set_buf_ll_addr(struct device *dev, struct net_buf *buf)
{
	struct net_rpl_test *rpl = dev->driver_data;

	struct net_linkaddr *src = net_nbuf_ll_src(buf);
	struct net_linkaddr *dst = net_nbuf_ll_dst(buf);

	dst->len = lladdr_src.len;
	dst->addr = lladdr_src.addr;

	src->len = sizeof(rpl->mac_addr);
	src->addr = rpl->mac_addr;
}

static int tester_send(struct net_if *iface, struct net_buf *buf)
{
	if (!buf->frags) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	set_buf_ll_addr(iface->dev, buf);

	/* By default we assume that the test is ok */
	data_failure = false;

	if (feed_data) {
		net_nbuf_ll_swap(buf);

		if (net_recv_data(iface, buf) < 0) {
			TC_ERROR("Data receive failed.");
			net_nbuf_unref(buf);
			test_failed = true;
		}

		k_sem_give(&wait_data);

		return 0;
	}

	DBG("Buf %p to be sent len %lu\n", buf, net_buf_frags_len(buf));

#if 0
	net_hexdump_frags("recv", buf);
#endif

	if (NET_ICMP_BUF(buf)->type != expected_icmpv6) {
		DBG("ICMPv6 type %d, expected %d\n",
		    NET_ICMP_BUF(buf)->type, expected_icmpv6);

		data_failure = true;
	}

	/* If we are not sending what is expected, then mark it as a failure
	 */
	if (msg_sending) {
		if (msg_sending != NET_ICMP_BUF(buf)->code) {
			DBG("Received code %d, expected %d\n",
			    NET_ICMP_BUF(buf)->code, msg_sending);

			data_failure = true;
		} else {
			/* Pass sent DIO message back to us */
			if (msg_sending == NET_RPL_DODAG_INFO_OBJ) {
				net_nbuf_ll_swap(buf);

				if (!net_recv_data(iface, buf)) {
					/* We must not unref the msg,
					 * as it should be unfreed by
					 * the upper stack.
					 */
					goto out;
				}
			}
		}
	}

	net_nbuf_unref(buf);

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

static bool test_init(void)
{
	struct net_if_addr *ifaddr;
	struct net_if_mcast_addr *maddr;
	struct net_if *iface = net_if_get_default();
	struct net_rpl_dag *dag;

	if (!iface) {
		TC_ERROR("Interface is NULL\n");
		return false;
	}

	ifaddr = net_if_ipv6_addr_add(iface, &in6addr_my,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		TC_ERROR("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&in6addr_my));
		return false;
	}

	/* For testing purposes we need to set the adddresses preferred */
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface, &in6addr_ll,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		TC_ERROR("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&in6addr_ll));
		return false;
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_ipv6_addr_create(&in6addr_mcast, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001);

	maddr = net_if_ipv6_maddr_add(iface, &in6addr_mcast);
	if (!maddr) {
		TC_ERROR("Cannot add multicast IPv6 address %s\n",
		       net_sprint_ipv6_addr(&in6addr_mcast));
		return false;
	}

	/* The semaphore is there to wait the data to be received. */
	k_sem_init(&wait_data, 0, UINT_MAX);

	net_if_register_link_cb(&link_cb, send_link_cb);

	/* Creating a new RPL DAG */
	net_rpl_set_root(iface, NET_RPL_DEFAULT_INSTANCE, &in6addr_my);
	dag = net_rpl_get_any_dag();
	net_rpl_set_prefix(iface, dag, &in6addr_my, 64);

	return true;
}

static bool net_ctx_create(void)
{
	int ret;

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_ctx);
	if (ret != 0) {
		TC_ERROR("Context create IPv6 UDP test failed (%d vs %d)\n",
		       ret, 0);
		return false;
	}

	return true;
}

static bool test_rpl_mcast_addr(void)
{
	struct in6_addr rpl_mcast = { { { 0xff, 0x02, 0, 0, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x1a } } };
	struct in6_addr addr;
	bool ret;

	ret = net_rpl_is_ipv6_addr_mcast(&rpl_mcast);
	if (!ret) {
		TC_ERROR("RPL multicast address check failed.\n");
		return false;
	}

	net_rpl_create_mcast_address(&addr);

	ret = net_rpl_is_ipv6_addr_mcast(&addr);
	if (!ret) {
		TC_ERROR("Generated RPL multicast address check failed.\n");
		return false;
	}

	return true;
}

static bool test_dio_dummy_input(void)
{
	struct net_buf *buf, *frag;
	int ret;

	buf = net_nbuf_get_tx(udp_ctx);
	frag = net_nbuf_get_data(udp_ctx);

	net_buf_frag_add(buf, frag);

	msg_sending = NET_RPL_DODAG_INFO_OBJ;

	set_buf_ll_addr(net_if_get_default()->dev, buf);

	ret = net_icmpv6_input(buf, net_buf_frags_len(buf->frags),
			       NET_ICMPV6_RPL, msg_sending);
	if (!ret) {
		TC_ERROR("%d: Callback in %s not called properly\n", __LINE__,
			 __func__);
		return false;
	}

	data_failure = false;
	k_sem_take(&wait_data, WAIT_TIME);

	if (data_failure) {
		TC_ERROR("%d: Unexpected ICMPv6 code received\n", __LINE__);
		return false;
	}

	data_failure = false;

	return true;
}

static bool test_dis_sending(void)
{
	struct net_if *iface;
	int ret;

	iface = net_if_get_default();

	msg_sending = NET_RPL_DODAG_SOLICIT;

	ret = net_rpl_dis_send(NULL, iface);
	if (ret) {
		TC_ERROR("%d: Cannot send DIS (%d)\n", __LINE__, ret);
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

static bool test_dao_sending_fail(void)
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

	/* The sending should fail at this point because the neighbor
	 * is not suppose to be found in neighbor cache.
	 */
	ret = net_rpl_dao_send(iface, &parent, prefix, 100);
	if (!ret) {
		TC_ERROR("DAO send succeed but should not have\n");
		return false;
	}

	return true;
}

static bool net_test_send_ns(void)
{
	int ret;

	ret = net_ipv6_send_ns(net_if_get_default(),
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

	net_ipv6_nbr_data(nbr)->state = NET_NBR_REACHABLE;

	return true;
}

static bool populate_nbr_cache(void)
{
	msg_sending = NET_ICMPV6_NS;
	feed_data = true;
	data_failure = false;

	if (!net_test_send_ns()) {
		return false;
	}

	k_sem_take(&wait_data, WAIT_TIME);

	feed_data = false;

	if (data_failure) {
		data_failure = false;
		return false;
	}

	data_failure = false;

	if (!net_test_nbr_lookup_ok()) {
		return false;
	}

	return true;
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
#endif

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

static bool test_dio_receive_dest(void)
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

	msg_sending = NET_RPL_DODAG_INFO_OBJ;

	ret = net_rpl_dio_send(iface, &instance, &peer_addr, &in6addr_my);
	if (ret) {
		TC_ERROR("%d: Cannot send DIO (%d)\n", __LINE__, ret);
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

static const struct {
	const char *name;
	bool (*func)(void);
} tests[] = {
	{ "test init", test_init },
	{ "test ctx create", net_ctx_create },
	{ "RPL multicast address test", test_rpl_mcast_addr },
	{ "DIO input handler test", test_dio_dummy_input },
	{ "DIS sending", test_dis_sending },
	{ "DAO sending fail", test_dao_sending_fail },
	{ "Populate neighbor cache", populate_nbr_cache },
	{ "Link cb test", test_link_cb },
	{ "DIO receive dest set", test_dio_receive_dest },
#if 0
	{ "DAO sending ok", test_dao_sending_ok },
	{ "DIO receive dest not set", test_dio_receive },
#endif
};

void main(void)
{
	int count, pass;

	for (count = 0, pass = 0; count < ARRAY_SIZE(tests); count++) {
		TC_START(tests[count].name);
		test_failed = false;
		if (!tests[count].func() || test_failed) {
			TC_END(FAIL, "failed\n");
		} else {
			TC_END(PASS, "passed\n");
			pass++;
		}
	}

	TC_END_REPORT(((pass != ARRAY_SIZE(tests)) ? TC_FAIL : TC_PASS));
}
