/* main.c - Application main entry point */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <linker/sections.h>

#include <ztest.h>

#include <net/ethernet.h>
#include <net/buf.h>
#include <net/net_ip.h>
#include <net/net_if.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

#include "ipv6.h"
#include "udp.h"

#if defined(CONFIG_NET_DEBUG_IPV6)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

/* Interface 1 addresses */
static struct in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Interface 2 addresses */
static struct in6_addr my_addr2 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x2 } } };

/* Extra address is assigned to ll_addr */
static struct in6_addr ll_addr = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
				       0x04 } } };

static u8_t mac2_addr[] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x02 };
static struct net_linkaddr ll_addr2 = {
	.addr = mac2_addr,
	.len = 6,
};

/* No extension header */
static const unsigned char ipv6_udp[] = {
/* IPv6 header starts here */
0x60, 0x00, 0x00, 0x00, 0x00, 0x36, 0x11, 0x3f, /* `....6.? */
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
/* UDP header starts here (checksum is "fixed" in this example) */
0xaa, 0xdc, 0xbf, 0xd7, 0x00, 0x2e, 0xa2, 0x55, /* ......M. */
/* User data starts here and is appended in corresponding function */
};

/* IPv6 hop-by-hop option in the message */
static const unsigned char ipv6_hbho[] = {
/* IPv6 header starts here */
0x60, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x3f, /* `....6.? */
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
/* Hop-by-hop option starts here */
0x11, 0x00,
/* RPL sub-option starts here */
0x63, 0x04, 0x80, 0x1e, 0x01, 0x00, /* ..c..... */
/* UDP header starts here (checksum is "fixed" in this example) */
0xaa, 0xdc, 0xbf, 0xd7, 0x00, 0x2e, 0xa2, 0x55, /* ......M. */
/* User data starts here and is appended in corresponding function */
};

/* IPv6 hop-by-hop option in the message */
static const unsigned char ipv6_hbho_frag[] = {
/* IPv6 header starts here */
0x60, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x3f, /* `....6.? */
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
/* Hop-by-hop option starts here */
0x2c, 0x00,
/* RPL sub-option starts here */
0x63, 0x04, 0x80, 0x1e, 0x01, 0x00, /* ..c..... */
/* IPv6 fragment header */
0x11, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
/* UDP header starts here (checksum is "fixed" in this example) */
0xaa, 0xdc, 0xbf, 0xd7, 0x00, 0x2e, 0xa2, 0x55, /* ......M. */
/* User data starts here and is appended in corresponding function */
};

static unsigned char ipv6_first_frag[] = {
/* IPv6 header starts here */
0x60, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x3f, /* `....6.? */
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
/* Hop-by-hop option starts here */
0x2C, 0x00,
/* RPL sub-option starts here */
0x63, 0x04, 0x80, 0x1e, 0x01, 0x00, /* ..c..... */
/* IPv6 fragment header */
0x11, 0x00, 0x00, 0x01, 0x01, 0x02, 0x03, 0x04,
/* UDP header starts here (checksum is "fixed" in this example) */
0xaa, 0xdc, 0xbf, 0xd7, 0x00, 0x2e, 0xa2, 0x55, /* ......M. */
/* User data starts here and is appended in corresponding function */
};

static unsigned char ipv6_second_frag[] = {
/* IPv6 header starts here */
0x60, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x3f, /* `....6.? */
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
/* Hop-by-hop option starts here */
0x2C, 0x00,
/* RPL sub-option starts here */
0x63, 0x04, 0x80, 0x1e, 0x01, 0x00, /* ..c..... */
/* IPv6 fragment header */
0x11, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
};

static int frag_count;

static struct net_if *iface1;
static struct net_if *iface2;

static bool test_failed;
static bool test_started;
static struct k_sem wait_data;

static u16_t pkt_data_len;
static u16_t pkt_recv_data_len;

#define WAIT_TIME K_SECONDS(1)

#define ALLOC_TIMEOUT 500

struct net_if_test {
	u8_t idx;
	u8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

static int net_iface_dev_init(struct device *dev)
{
	return 0;
}

static u8_t *net_iface_get_mac(struct device *dev)
{
	struct net_if_test *data = dev->driver_data;

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
	data->ll_addr.len = 6;

	return data->mac_addr;
}

static void net_iface_init(struct net_if *iface)
{
	u8_t *mac = net_iface_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static int verify_fragment(struct net_pkt *pkt)
{
	/* The fragment needs to have
	 *  1) IPv6 header
	 *  2) HBH option (if any)
	 *  3) IPv6 fragment header
	 *  4) UDP/ICMPv6/TCP header
	 *  5) data
	 */
	u16_t offset;

	frag_count++;

	if (frag_count == 1) {
		/* First fragment received. Make sure that all the
		 * things are correct before the fragment header.
		 */
		size_t total_len = net_pkt_get_len(pkt);

		total_len -= sizeof(struct net_ipv6_hdr);

		ipv6_first_frag[4] = total_len / 256;
		ipv6_first_frag[5] = total_len -
			ipv6_first_frag[4] * 256;

		if ((total_len / 256) != pkt->frags->data[4]) {
			DBG("Invalid length, 1st byte\n");
			return -EINVAL;
		}

		if ((total_len - pkt->frags->data[4] * 256) !=
		    pkt->frags->data[5]) {
			DBG("Invalid length, 2nd byte\n");
			return -EINVAL;
		}

		offset = pkt->frags->data[6 * 8 + 2] * 256 +
			(pkt->frags->data[6 * 8 + 3] & 0xfe);
		if (offset != 0) {
			DBG("Invalid offset\n");
			return -EINVAL;
		}

		if ((ipv6_first_frag[6 * 8 + 3] & 0x01) != 1) {
			DBG("Invalid MORE flag for first fragment\n");
			return -EINVAL;
		}

		pkt_recv_data_len += total_len - 8 /* HBHO */ - 8 /* UDP */ -
			sizeof(struct net_ipv6_frag_hdr);

		/* Rewrite the fragment id so that the memcmp() will not fail */
		ipv6_first_frag[6 * 8 + 4] = pkt->frags->data[6 * 8 + 4];
		ipv6_first_frag[6 * 8 + 5] = pkt->frags->data[6 * 8 + 5];
		ipv6_first_frag[6 * 8 + 6] = pkt->frags->data[6 * 8 + 6];
		ipv6_first_frag[6 * 8 + 7] = pkt->frags->data[6 * 8 + 7];

		if (memcmp(pkt->frags->data, ipv6_first_frag, 7 * 8) != 0) {
			net_hexdump("received", pkt->frags->data, 7 * 8);
			DBG("\n");
			net_hexdump("expected", ipv6_first_frag, 7 * 8);

			return -EINVAL;
		}
	}

	if (frag_count == 2) {
		/* Second fragment received. */
		size_t total_len = net_pkt_get_len(pkt);

		total_len -= sizeof(struct net_ipv6_hdr);

		ipv6_second_frag[4] = total_len / 256;
		ipv6_second_frag[5] = total_len -
			ipv6_second_frag[4] * 256;

		if ((total_len / 256) != pkt->frags->data[4]) {
			DBG("Invalid length, 1st byte\n");
			return -EINVAL;
		}

		if ((total_len - pkt->frags->data[4] * 256) !=
		    pkt->frags->data[5]) {
			DBG("Invalid length, 2nd byte\n");
			return -EINVAL;
		}

		offset = pkt->frags->data[6 * 8 + 2] * 256 +
			(pkt->frags->data[6 * 8 + 3] & 0xfe);

		if (offset != pkt_recv_data_len) {
			DBG("Invalid offset %d received %d\n",
			    offset, pkt_recv_data_len);
			return -EINVAL;
		}

		/* Make sure the MORE flag is set correctly */
		if ((pkt->frags->data[6 * 8 + 3] & 0x01) != 0) {
			DBG("Invalid MORE flag for second fragment\n");
			return -EINVAL;
		}

		pkt_recv_data_len += total_len - 8 /* HBHO */ - 8 /* UDP */ -
			sizeof(struct net_ipv6_frag_hdr);

		ipv6_second_frag[6 * 8 + 2] = pkt->frags->data[6 * 8 + 2];
		ipv6_second_frag[6 * 8 + 3] = pkt->frags->data[6 * 8 + 3];

		/* Rewrite the fragment id so that the memcmp() will not fail */
		ipv6_second_frag[6 * 8 + 4] = pkt->frags->data[6 * 8 + 4];
		ipv6_second_frag[6 * 8 + 5] = pkt->frags->data[6 * 8 + 5];
		ipv6_second_frag[6 * 8 + 6] = pkt->frags->data[6 * 8 + 6];
		ipv6_second_frag[6 * 8 + 7] = pkt->frags->data[6 * 8 + 7];

		if (memcmp(pkt->frags->data, ipv6_second_frag, 7 * 8) != 0) {
			net_hexdump("received 2", pkt->frags->data, 7 * 8);
			DBG("\n");
			net_hexdump("expected 2", ipv6_second_frag, 7 * 8);

			return -EINVAL;
		}

		if (pkt_data_len != pkt_recv_data_len) {
			DBG("Invalid amount of data received (%d vs %d)\n",
			    pkt_data_len, pkt_recv_data_len);
			return -EINVAL;
		}
	}

	return 0;
}

static int sender_iface(struct net_if *iface, struct net_pkt *pkt)
{
	if (!pkt->frags) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	if (test_started) {
		struct net_if_test *data = iface->dev->driver_data;

		DBG("Sending at iface %d %p\n", net_if_get_by_iface(iface),
		    iface);

		if (net_pkt_iface(pkt) != iface) {
			DBG("Invalid interface %p, expecting %p\n",
				 net_pkt_iface(pkt), iface);
			test_failed = true;
		}

		if (net_if_get_by_iface(iface) != data->idx) {
			DBG("Invalid interface %d index, expecting %d\n",
				 data->idx, net_if_get_by_iface(iface));
			test_failed = true;
		}

		/* Verify the fragments */
		if (verify_fragment(pkt) < 0) {
			DBG("Fragments cannot be verified\n");
			test_failed = true;
		} else {
			k_sem_give(&wait_data);
		}
	}

	zassert_false(test_failed, "Fragment verify failed");

	net_pkt_unref(pkt);

	return 0;
}

struct net_if_test net_iface1_data;
struct net_if_test net_iface2_data;

static struct net_if_api net_iface_api = {
	.init = net_iface_init,
	.send = sender_iface,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT_INSTANCE(net_iface1_test,
			 "iface1",
			 iface1,
			 net_iface_dev_init,
			 &net_iface1_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE,
			 127);

NET_DEVICE_INIT_INSTANCE(net_iface2_test,
			 "iface2",
			 iface2,
			 net_iface_dev_init,
			 &net_iface2_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE,
			 127);

static void add_nbr(struct net_if *iface,
		    struct in6_addr *addr,
		    struct net_linkaddr *lladdr)
{
	struct net_nbr *nbr;

	nbr = net_ipv6_nbr_add(iface, addr, lladdr, false,
			       NET_IPV6_NBR_STATE_REACHABLE);
	zassert_not_null(nbr, "Cannot add neighbor");
}

static enum net_verdict udp_data_received(struct net_conn *conn,
					  struct net_pkt *pkt,
					  void *user_data)
{
	DBG("Data %p received\n", pkt);

	net_pkt_unref(pkt);

	return NET_OK;
}

static void setup_udp_handler(const struct in6_addr *raddr,
			      const struct in6_addr *laddr,
			      u16_t remote_port,
			      u16_t local_port)
{
	static struct net_conn_handle *handle;
	struct sockaddr remote_addr = { 0 };
	struct sockaddr local_addr = { 0 };
	int ret;

	net_ipaddr_copy(&net_sin6(&local_addr)->sin6_addr, laddr);
	local_addr.family = AF_INET6;

	net_ipaddr_copy(&net_sin6(&remote_addr)->sin6_addr, raddr);
	remote_addr.family = AF_INET6;

	ret = net_udp_register(&remote_addr, &local_addr, remote_port,
			       local_port, udp_data_received,
			       NULL, &handle);
	zassert_equal(ret, 0, "Cannot register UDP handler");
}

static void setup(void)
{
	struct net_if_addr *ifaddr;
	int idx;

	/* The semaphore is there to wait the data to be received. */
	k_sem_init(&wait_data, 0, UINT_MAX);

	iface1 = net_if_get_by_index(0);
	iface2 = net_if_get_by_index(1);

	((struct net_if_test *)iface1->dev->driver_data)->idx = 0;
	((struct net_if_test *)iface2->dev->driver_data)->idx = 1;

	idx = net_if_get_by_iface(iface1);
	zassert_equal(idx, 0, "Invalid index iface1");

	idx = net_if_get_by_iface(iface2);
	zassert_equal(idx, 1, "Invalid index iface2");

	zassert_not_null(iface1, "Interface 1");
	zassert_not_null(iface2, "Interface 2");

	ifaddr = net_if_ipv6_addr_add(iface1, &my_addr1,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr1));
		zassert_not_null(ifaddr, "addr1");
	}

	/* For testing purposes we need to set the adddresses preferred */
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface1, &ll_addr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&ll_addr));
		zassert_not_null(ifaddr, "ll_addr");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_if_up(iface1);
	net_if_up(iface2);

	add_nbr(iface1, &my_addr2, &ll_addr2);

	/* Remote and local are swapped so that we can receive the sent
	 * packet.
	 */
	setup_udp_handler(&my_addr1, &my_addr2, 4352, 25348);

	/* The interface might receive data which might fail the checks
	 * in the iface sending function, so we need to reset the failure
	 * flag.
	 */
	test_failed = false;

	test_started = true;
}

static void find_last_ipv6_fragment_udp(void)
{
	u16_t next_hdr_idx = 0;
	u16_t last_hdr_pos = 0;
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_get_reserve_tx(0, ALLOC_TIMEOUT);
	zassert_not_null(pkt, "packet");

	net_pkt_set_iface(pkt, iface1);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_ipv6_ext_len(pkt, sizeof(ipv6_udp) -
				 sizeof(struct net_ipv6_hdr));
	net_pkt_ll_clear(pkt);

	/* Add IPv6 header + UDP */
	ret = net_pkt_append_all(pkt, sizeof(ipv6_udp), ipv6_udp,
				 ALLOC_TIMEOUT);
	zassert_true(ret, "IPv6 header append failed");

	ret = net_ipv6_find_last_ext_hdr(pkt, &next_hdr_idx, &last_hdr_pos);
	zassert_equal(ret, 0, "Cannot find last header");

	zassert_equal(next_hdr_idx, 6, "Next header index wrong");
	zassert_equal(last_hdr_pos, sizeof(struct net_ipv6_hdr),
		      "Last header position wrong");

	zassert_equal(NET_IPV6_HDR(pkt)->nexthdr, 0x11, "Invalid next header");
	zassert_equal(pkt->frags->data[next_hdr_idx], 0x11, "Invalid next "
		      "header");

	net_pkt_unref(pkt);
}

static void find_last_ipv6_fragment_hbho_udp(void)
{
	u16_t next_hdr_idx = 0;
	u16_t last_hdr_pos = 0;
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_get_reserve_tx(0, ALLOC_TIMEOUT);
	zassert_not_null(pkt, "packet");

	net_pkt_set_iface(pkt, iface1);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_ipv6_ext_len(pkt, sizeof(ipv6_hbho) -
				 sizeof(struct net_ipv6_hdr));
	net_pkt_ll_clear(pkt);

	/* Add IPv6 header + HBH option */
	ret = net_pkt_append_all(pkt, sizeof(ipv6_hbho), ipv6_hbho,
				 ALLOC_TIMEOUT);
	zassert_true(ret, "IPv6 header append failed");

	ret = net_ipv6_find_last_ext_hdr(pkt, &next_hdr_idx, &last_hdr_pos);
	zassert_equal(ret, 0, "Cannot find last header");

	zassert_equal(next_hdr_idx, sizeof(struct net_ipv6_hdr),
		      "Next header index wrong");
	zassert_equal(last_hdr_pos, sizeof(struct net_ipv6_hdr) + 8,
		      "Last header position wrong");

	zassert_equal(NET_IPV6_HDR(pkt)->nexthdr, 0, "Invalid next header");
	zassert_equal(pkt->frags->data[next_hdr_idx], 0x11, "Invalid next "
		      "header");

	net_pkt_unref(pkt);
}

static void find_last_ipv6_fragment_hbho_frag(void)
{
	u16_t next_hdr_idx = 0;
	u16_t last_hdr_pos = 0;
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_get_reserve_tx(0, ALLOC_TIMEOUT);
	zassert_not_null(pkt, "packet");

	net_pkt_set_iface(pkt, iface1);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_ipv6_ext_len(pkt, sizeof(ipv6_hbho_frag) -
				 sizeof(struct net_ipv6_hdr));
	net_pkt_ll_clear(pkt);

	/* Add IPv6 header + HBH option + fragment header */
	ret = net_pkt_append_all(pkt, sizeof(ipv6_hbho_frag), ipv6_hbho_frag,
				 ALLOC_TIMEOUT);
	zassert_true(ret, "IPv6 header append failed");

	ret = net_ipv6_find_last_ext_hdr(pkt, &next_hdr_idx, &last_hdr_pos);
	zassert_equal(ret, 0, "Cannot find last header");

	zassert_equal(next_hdr_idx, sizeof(struct net_ipv6_hdr) + 8,
		      "Next header index wrong");
	zassert_equal(last_hdr_pos, sizeof(struct net_ipv6_hdr) + 8 + 8,
		      "Last header position wrong");

	zassert_equal(NET_IPV6_HDR(pkt)->nexthdr, 0, "Invalid next header");
	zassert_equal(pkt->frags->data[next_hdr_idx], 0x11, "Invalid next "
		      "header");

	net_pkt_unref(pkt);
}

static void send_ipv6_fragment(void)
{
#define MAX_LEN 1600
	static char data[] = "123456789.";
	int data_len = sizeof(data) - 1;
	int count = MAX_LEN / data_len;
	struct net_pkt *pkt;
	size_t total_len;
	int i, ret;

	pkt_data_len = 0;

	pkt = net_pkt_get_reserve_tx(0, ALLOC_TIMEOUT);
	zassert_not_null(pkt, "packet");

	net_pkt_set_iface(pkt, iface1);
	net_pkt_set_family(pkt, AF_INET6);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
	net_pkt_set_ipv6_ext_len(pkt, 8 + 8); /* hbho + udp */
	net_pkt_ll_clear(pkt);

	/* Add IPv6 header + HBH option */
	ret = net_pkt_append_all(pkt, sizeof(ipv6_hbho), ipv6_hbho,
				 ALLOC_TIMEOUT);
	zassert_true(ret, "IPv6 header append failed");

	/* Then add some data that is over 1280 bytes long */
	for (i = 0; i < count; i++) {
		bool written = net_pkt_append_all(pkt, data_len, (u8_t *)data,
						  ALLOC_TIMEOUT);
		zassert_true(written, "Cannot append data");

		pkt_data_len += data_len;
	}

	zassert_equal(pkt_data_len, count * data_len, "Data size mismatch");

	total_len = net_pkt_get_len(pkt);
	total_len -= sizeof(struct net_ipv6_hdr);

	DBG("Sending %zd bytes of which ext %d and data %d bytes\n",
	    total_len, net_pkt_ipv6_ext_len(pkt), pkt_data_len);

	zassert_equal(total_len - net_pkt_ipv6_ext_len(pkt), pkt_data_len,
		      "Packet size invalid");

	NET_IPV6_HDR(pkt)->len[0] = total_len / 256;
	NET_IPV6_HDR(pkt)->len[1] = total_len -
		NET_IPV6_HDR(pkt)->len[0] * 256;

	NET_UDP_HDR(pkt)->chksum = 0;
	NET_UDP_HDR(pkt)->chksum = ~net_calc_chksum_udp(pkt);

	test_failed = false;

	ret = net_send_data(pkt);
	if (ret < 0) {
		DBG("Cannot send test packet (%d)\n", ret);
		zassert_equal(ret, 0, "Cannot send");
	}

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_equal(ret, 0, "Timeout");
	}
}

static void recv_ipv6_fragment(void)
{
	/* TODO: Verify that we can receive individual fragments and
	 * then reassemble them back.
	 */
}

void test_main(void)
{
	ztest_test_suite(net_ipv6_fragment_test,
			 ztest_unit_test(setup),
			 ztest_unit_test(find_last_ipv6_fragment_udp),
			 ztest_unit_test(find_last_ipv6_fragment_hbho_udp),
			 ztest_unit_test(find_last_ipv6_fragment_hbho_frag),
			 ztest_unit_test(send_ipv6_fragment),
			 ztest_unit_test(recv_ipv6_fragment)
			 );

	ztest_run_test_suite(net_ipv6_fragment_test);
}
