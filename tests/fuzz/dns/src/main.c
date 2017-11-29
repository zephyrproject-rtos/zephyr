/* main.c - DNS Fuzz testing client
 *
 * Boots up and starts making DNS requests for www.zephyrproject.org until it
 * receives a TCP connection on port 4242, which is its signal to shutdown.
 *
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <linker/sections.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <device.h>
#include <init.h>
#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_ip.h>
#include <net/dns.h>
#include <net/ethernet.h>
#include <net/net_mgmt.h>
#include <net/udp.h>

/* Headers need for DNS fuzzing */
#include <net/net_core.h>
#include <net/net_if.h>
#include <net/net_mgmt.h>
#include <net/dns_resolve.h>

#include <tc_util.h>
#include <ztest.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

static const struct net_eth_addr src_addr = {
	{ 0x00, 0x00, 0x5E, 0x00, 0x53, 0x01 } };
static const struct net_eth_addr dst_addr = {
	{ 0x00, 0x00, 0x5E, 0x00, 0x53, 0x02 } };
static const struct in_addr server_addr = { { { 192, 0, 2, 1 } } };
static const struct in_addr client_addr = { { { 255, 255, 255, 255 } } };

/* FDQN to ask for in the DNS request */
static const char *dns_fdqn = "www.zephyrproject.org";

#define SHUTDOWN_SERVER_PORT	4242

static void test_result(bool pass)
{
	if (pass) {
		TC_END_REPORT(TC_PASS);
	} else {
		TC_END_REPORT(TC_FAIL);
	}
}

struct net_dns_context {
	u8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

static int net_dns_dev_init(struct device *dev)
{
	struct net_dns_context *net_dns_context = dev->driver_data;

	net_dns_context = net_dns_context;

	return 0;
}

static u8_t *net_dns_get_mac(struct device *dev)
{
	struct net_dns_context *context = dev->driver_data;

	if (context->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx RFC 7042 defines 00-00-5E-00-53-00 through
		 * 00-00-5E-00-53-FF for unicast per section 2.1.2 */
		context->mac_addr[0] = 0x00;
		context->mac_addr[1] = 0x00;
		context->mac_addr[2] = 0x5E;
		context->mac_addr[3] = 0x00;
		context->mac_addr[4] = 0x53;
		context->mac_addr[5] = 0x01;
	}

	return context->mac_addr;
}

static void net_dns_iface_init(struct net_if *iface)
{
	u8_t *mac = net_dns_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);
}

static struct net_buf *pkt_get_data(struct net_pkt *pkt, struct net_if *iface)
{
	struct net_buf *frag;
	struct net_eth_hdr *hdr;

	net_pkt_set_ll_reserve(pkt, net_if_get_ll_reserve(iface, NULL));

	frag = net_pkt_get_frag(pkt, K_FOREVER);
	if (!frag) {
		return NULL;
	}

	hdr = (struct net_eth_hdr *)(frag->data - net_pkt_ll_reserve(pkt));
	hdr->type = htons(NET_ETH_PTYPE_IP);

	net_ipaddr_copy(&hdr->dst, &src_addr);
	net_ipaddr_copy(&hdr->src, &dst_addr);

	return frag;
}

static void set_ipv4_header(struct net_pkt *pkt)
{
	struct net_ipv4_hdr *ipv4;
	u16_t length;

	ipv4 = NET_IPV4_HDR(pkt);

	ipv4->vhl = 0x45; /* IP version and header length */
	ipv4->tos = 0x00;

	length = sizeof(offer) + sizeof(struct net_ipv4_hdr) +
		 sizeof(struct net_udp_hdr);

	ipv4->len[1] = length;
	ipv4->len[0] = length >> 8;

	memset(ipv4->id, 0, 4); /* id and offset */

	ipv4->ttl = 0xFF;
	ipv4->proto = IPPROTO_UDP;

	net_ipaddr_copy(&ipv4->src, &server_addr);
	net_ipaddr_copy(&ipv4->dst, &client_addr);
}

static void set_udp_header(struct net_pkt *pkt)
{
	struct net_udp_hdr *udp;
	u16_t length;

	udp = (struct net_udp_hdr *)((u8_t *)(NET_IPV4_HDR(pkt)) +
				     sizeof(struct net_ipv4_hdr));

	udp->src_port = htons(SERVER_PORT);
	udp->dst_port = htons(CLIENT_PORT);

	length = sizeof(offer) + sizeof(struct net_udp_hdr);
	udp->len = htons(length);
	udp->chksum = 0;
}

struct net_pkt *prepare_dns_offer(struct net_if *iface, u32_t xid)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	int bytes, remaining = sizeof(offer), pos = 0;
	u16_t offset;

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	if (!pkt) {
		return NULL;
	}

	frag = pkt_get_data(pkt, iface);
	if (!frag) {
		net_pkt_unref(pkt);
		return NULL;
	}

	net_pkt_set_iface(pkt, iface);
	net_pkt_set_ll_reserve(pkt, net_buf_headroom(frag));
	net_pkt_set_family(pkt, AF_INET);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	net_pkt_frag_add(pkt, frag);

	/* Place the IPv4 header */
	set_ipv4_header(pkt);

	/* Place the UDP header */
	set_udp_header(pkt);

	net_buf_add(frag, NET_IPV4UDPH_LEN);
	offset = NET_IPV4UDPH_LEN;

	while (remaining > 0) {
		int copy;

		bytes = net_buf_tailroom(frag);
		copy = remaining > bytes ? bytes : remaining;

		memcpy(frag->data + offset, &offer[pos], copy);

		net_buf_add(frag, copy);

		pos += bytes;
		remaining -= bytes;

		if (remaining > 0) {
			frag = pkt_get_data(pkt, iface);
			if (!frag) {
				goto fail;
			}

			offset = 0;
			net_pkt_frag_add(pkt, frag);
		}
	}

	/* Now fixup the expect XID */
	frag = net_pkt_write_be32(pkt, pkt->frags,
				   (sizeof(struct net_ipv4_hdr) +
				    sizeof(struct net_udp_hdr)) + 4,
				   &offset, xid);
	return pkt;

fail:
	net_pkt_unref(pkt);
	return NULL;
}

struct net_pkt *prepare_dns_ack(struct net_if *iface, u32_t xid)
{
	struct net_pkt *pkt;
	struct net_buf *frag;
	int bytes, remaining = sizeof(ack), pos = 0;
	u16_t offset;

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	if (!pkt) {
		return NULL;
	}

	frag = pkt_get_data(pkt, iface);
	if (!frag) {
		net_pkt_unref(pkt);
		return NULL;
	}

	net_pkt_set_iface(pkt, iface);
	net_pkt_set_ll_reserve(pkt, net_buf_headroom(frag));
	net_pkt_set_family(pkt, AF_INET);
	net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));

	net_pkt_frag_add(pkt, frag);

	/* Place the IPv4 header */
	set_ipv4_header(pkt);

	/* Place the UDP header */
	set_udp_header(pkt);

	net_buf_add(frag, NET_IPV4UDPH_LEN);
	offset = NET_IPV4UDPH_LEN;

	while (remaining > 0) {
		int copy;

		bytes = net_buf_tailroom(frag);
		copy = remaining > bytes ? bytes : remaining;

		memcpy(frag->data + offset, &ack[pos], copy);

		net_buf_add(frag, copy);

		pos += bytes;
		remaining -= bytes;

		if (remaining > 0) {
			frag = pkt_get_data(pkt, iface);
			if (!frag) {
				goto fail;
			}

			offset = 0;
			net_pkt_frag_add(pkt, frag);
		}
	}

	/* Now fixup the expect XID */
	frag = net_pkt_write_be32(pkt, pkt->frags,
				   (sizeof(struct net_ipv4_hdr) +
				    sizeof(struct net_udp_hdr)) + 4,
				   &offset, xid);
	return pkt;

fail:
	net_pkt_unref(pkt);
	return NULL;
}

static int parse_dns_message(struct net_pkt *pkt, struct dns_msg *msg)
{
	struct net_buf *frag = pkt->frags;
	u8_t type;
	u16_t offset;

	frag = net_frag_skip(frag, 0, &offset,
			   /* size of op, htype, hlen, hops */
			   (sizeof(struct net_ipv4_hdr) +
			    sizeof(struct net_udp_hdr)) + 4);
	if (!frag) {
		return 0;
	}

	frag = net_frag_read_be32(frag, offset, &offset, &msg->xid);
	if (!frag) {
		return 0;
	}

	frag = net_frag_skip(frag, offset, &offset,
			   /* size of op, htype ... cookie */
			   (36 + 64 + 128 + 4));
	if (!frag) {
		return 0;
	}

	while (frag) {
		u8_t length;

		frag = net_frag_read_u8(frag, offset, &offset, &type);
		if (!frag) {
			return 0;
		}

		if (type == MSG_TYPE) {
			frag = net_frag_skip(frag, offset, &offset, 1);
			if (!frag) {
				return 0;
			}

			frag = net_frag_read_u8(frag, offset, &offset,
						&msg->type);
			if (!frag) {
				return 0;
			}

			return 1;
		}

		frag = net_frag_read_u8(frag, offset, &offset, &length);
		if (frag) {
			frag = net_frag_skip(frag, offset, &offset, length);
			if (!frag) {
				return 0;
			}
		}
	}

	return 0;
}

static int tester_send(struct net_if *iface, struct net_pkt *pkt)
{
	struct net_pkt *rpkt;
	struct dns_msg msg;

	memset(&msg, 0, sizeof(msg));

	if (!pkt->frags) {
		TC_PRINT("No data to send!\n");

		return -ENODATA;
	}

	parse_dns_message(pkt, &msg);

	if (msg.type == DISCOVER) {
		/* Reply with DNS offer message */
		rpkt = prepare_dns_offer(iface, msg.xid);
		if (!rpkt) {
			return -EINVAL;
		}
	} else if (msg.type == REQUEST) {
		/* Reply with DNS ACK message */
		rpkt = prepare_dns_ack(iface, msg.xid);
		if (!rpkt) {
			return -EINVAL;
		}

	} else {
		/* Invalid message type received */
		return -EINVAL;
	}

	if (net_recv_data(iface, rpkt)) {
		net_pkt_unref(rpkt);

		return -EINVAL;
	}

	net_pkt_unref(pkt);
	return NET_OK;
}

struct net_dns_context net_dns_context_data;

static struct net_if_api net_dns_if_api = {
	.init = net_dns_iface_init,
	.send = tester_send,
};

NET_DEVICE_INIT(net_dns_test, "net_dns_test",
		net_dns_dev_init, &net_dns_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_dns_if_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

static struct net_mgmt_event_callback rx_cb;

static void receiver_cb(struct net_mgmt_event_callback *cb,
			u32_t nm_event, struct net_if *iface)
{
	TC_PRINT("Calling test_result...");
	test_result(true);
	TC_PRINT("test_result called with true");
}

void test_dns(void)
{
	struct net_if *iface;

	k_thread_priority_set(k_current_get(), K_PRIO_COOP(7));
	net_mgmt_init_event_callback(&rx_cb, receiver_cb,
				     NET_EVENT_IPV4_ADDR_ADD);

	net_mgmt_add_event_callback(&rx_cb);

	iface = net_if_get_default();
	if (!iface) {
		TC_PRINT("Interface not available n");
		return;
	}

	TC_PRINT("Starting DNS resolve fuzz client");

	setup_ipv4(iface);

	k_yield();
}

/**test case main entry */
void test_main(void)
{
	ztest_test_suite(test_dns,
			ztest_unit_test(test_dns));
	ztest_run_test_suite(test_dns);
}

/* DNS fuzzing client code */
#include <net/net_core.h>
#include <net/net_if.h>
#include <net/net_mgmt.h>
#include <net/dns_resolve.h>

static void do_ipv4_lookup();
void dns_result_cb(enum dns_resolve_status status,
		   struct dns_addrinfo *info,
		   void *user_data);

#define DNS_TIMEOUT K_SECONDS(2)

static void do_ipv4_lookup()
{
	u16_t dns_id;
	int ret;

	ret = dns_get_addr_info(dns_fdqn,
				DNS_QUERY_TYPE_A,
				&dns_id,
				dns_result_cb,
				(void *)dns_fdqn,
				DNS_TIMEOUT);
	if (ret < 0) {
		NET_ERR("Cannot resolve IPv4 address (%d)", ret);
		return;
	}

	NET_DBG("DNS id %u", dns_id);
}

void dns_result_cb(enum dns_resolve_status status,
		   struct dns_addrinfo *info,
		   void *user_data)
{
	char hr_addr[NET_IPV6_ADDR_LEN];
	char *queried = user_data ? (char *)user_data : "<address unknown>";
	char *hr_family;
	void *addr;

	switch (status) {
	case DNS_EAI_CANCELED:
		TC_PRINT("DNS query was canceled: %s", queried);
		return;
	case DNS_EAI_FAIL:
		TC_PRINT("DNS resolve failed: %s", queried);
		return;
	case DNS_EAI_NODATA:
		TC_PRINT("Cannot resolve address: %s", queried);
		return;
	case DNS_EAI_ALLDONE:
		TC_PRINT("DNS resolving finished: %s", queried);
		return;
	case DNS_EAI_INPROGRESS:
		break;
	default:
		TC_PRINT("DNS resolving error (%d): %s", status, queried);
		return;
	}

	if (!info) {
		return;
	}

	if (info->ai_family == AF_INET) {
		hr_family = "IPv4";
		addr = &net_sin(&info->ai_addr)->sin_addr;
	} else if (info->ai_family == AF_INET6) {
		hr_family = "IPv6";
		addr = &net_sin6(&info->ai_addr)->sin6_addr;
	} else {
		NET_ERR("Invalid IP address family %d: %s", (char *)user_data,
			info->ai_family);
		return;
	}

	TC_PRINT("DNS result for \"%s\": %s address: %s",
		 queried,
		 hr_family,
		 net_addr_ntop(info->ai_family, addr,
			       hr_addr, sizeof(hr_addr)));

	do_ipv4_lookup();
}

static void setup_ipv4(struct net_if *iface)
{
	char hr_addr[NET_IPV4_ADDR_LEN];
	struct in_addr addr;

	if (net_addr_pton(AF_INET, CONFIG_NET_APP_MY_IPV4_ADDR, &addr)) {
		NET_ERR("Invalid address: %s", CONFIG_NET_APP_MY_IPV4_ADDR);
		return;
	}

	net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);

	TC_PRINT("IPv4 address: %s",
		 net_addr_ntop(AF_INET, &addr, hr_addr, NET_IPV4_ADDR_LEN));

	do_ipv4_lookup();
}
