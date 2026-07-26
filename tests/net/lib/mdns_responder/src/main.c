/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdns_resp_test);

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <ipv6.h>

#include <zephyr/net/dns_sd.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/mdns_responder.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#include "dns_pack.h"

#define NULL_CHAR_SIZE 1
#define EXT_RECORDS_NUM 3
#define MAX_RESP_PKTS 8
#define MAX_TXT_SIZE 128
#define RESPONSE_TIMEOUT (K_MSEC(250))

struct service_info {
	bool used;
	char instance[DNS_SD_INSTANCE_MAX_SIZE + NULL_CHAR_SIZE];
	char service[DNS_SD_SERVICE_MAX_SIZE + NULL_CHAR_SIZE];
	char proto[DNS_SD_PROTO_SIZE + NULL_CHAR_SIZE];
	char domain[DNS_SD_DOMAIN_MAX_SIZE + NULL_CHAR_SIZE];
	char text[MAX_TXT_SIZE];
	uint16_t port;
	struct dns_sd_rec *record;
};

static struct net_if *iface1;

static struct net_if_test {
	uint8_t idx; /* not used for anything, just a dummy value */
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
} net_iface1_data;

static const uint8_t ipv6_hdr_start[] = {
0x60, 0x05, 0xe7, 0x00
};

static const uint8_t ipv6_hdr_rest[] = {
0x11, 0xff, 0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9f, 0x74, 0x88,
0x9c, 0x1b, 0x44, 0x72, 0x39, 0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfb
};

static const uint8_t dns_sd_service_enumeration_query[] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09,
0x5f, 0x73, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x73, 0x07, 0x5f, 0x64, 0x6e,
0x73, 0x2d, 0x73, 0x64, 0x04, 0x5f, 0x75, 0x64, 0x70, 0x05, 0x6c, 0x6f, 0x63,
0x61, 0x6c, 0x00, 0x00, 0x0c, 0x00, 0x01
};

static const uint8_t service_enum_start[] = {
0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x09,
0x5f, 0x73, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x73, 0x07, 0x5f, 0x64, 0x6e,
0x73, 0x2d, 0x73, 0x64, 0x04, 0x5f, 0x75, 0x64, 0x70, 0x05, 0x6c, 0x6f, 0x63,
0x61, 0x6c, 0x00, 0x00, 0x0c, 0x00, 0x01, 0x00, 0x00, 0x11, 0x94
};

static const uint8_t payload_bar_udp_local[] = {
0x00, 0x0c, 0x04, 0x5f, 0x62, 0x61, 0x72, 0x04, 0x5f, 0x75, 0x64, 0x70, 0xc0,
0x23
};

static const uint8_t payload_custom_tcp_local[] = {
0x00, 0x0f, 0x07, 0x5f, 0x63, 0x75, 0x73, 0x74, 0x6f, 0x6d, 0x04,
0x5f, 0x74, 0x63, 0x70, 0xc0, 0x23
};

static const uint8_t payload_foo_tcp_local[] = {
0x00, 0x0c, 0x04, 0x5f, 0x66, 0x6f, 0x6f, 0x04, 0x5f, 0x74, 0x63, 0x70, 0xc0,
0x23
};

static const uint8_t payload_foo_udp_local[] = {
0x00, 0x0c, 0x04, 0x5f, 0x66, 0x6f, 0x6f, 0x04, 0x5f, 0x75, 0x64, 0x70, 0xc0,
0x23
};

static uint8_t mdns_server_ipv6_addr[] = {
0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfb
};

static struct net_in6_addr ll_addr = {{{
0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0, 0x9f, 0x74, 0x88, 0x9c, 0x1b, 0x44, 0x72, 0x39
}}};

static struct net_in6_addr sender_ll_addr = {{{
0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0x9f, 0x74, 0x88, 0x9c, 0x1b, 0x44, 0x72, 0x39
}}};

static struct net_in6_addr extra_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					      0, 0, 0, 0, 0, 0, 0, 0x1 } } };

static bool test_started;
static struct k_sem wait_data;
static struct net_pkt *response_pkts[MAX_RESP_PKTS];
static size_t responses_count;

/* Count of IGMP membership reports for 224.0.0.251 seen leaving the interface.
 * This lets us verify that a fresh report is actually put on the wire on
 * recovery (an IGMP-snooping switch needs it), not just that the local maddr
 * is flagged as joined.
 */
static int igmp_report_count;
static struct service_info services[EXT_RECORDS_NUM];
static struct dns_sd_rec records[EXT_RECORDS_NUM];

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
		data->mac_addr[5] = 0x01;
	}

	memcpy(data->ll_addr.addr, data->mac_addr, sizeof(data->mac_addr));
	data->ll_addr.len = 6U;

	return data->mac_addr;
}

static void net_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_iface_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
	net_if_flag_set(iface, NET_IF_IPV6_NO_ND);
}

static int sender_iface(const struct device *dev, struct net_pkt *pkt)
{
	struct net_ipv6_hdr *hdr;

	if (!pkt->buffer) {
		return -ENODATA;
	}

	if (test_started && net_pkt_family(pkt) == NET_AF_INET) {
		struct net_ipv4_hdr *ip4 = NET_IPV4_HDR(pkt);

		/* Count outgoing IGMP membership reports (the mDNS responder
		 * only joins 224.0.0.251 here, so any IGMP report is for it).
		 */
		if (ip4->proto == NET_IPPROTO_IGMP) {
			igmp_report_count++;
		}

		return 0;
	}

	if (test_started) {
		hdr = NET_IPV6_HDR(pkt);

		if (net_ipv6_addr_cmp_raw(hdr->dst, mdns_server_ipv6_addr)) {
			if (responses_count < MAX_RESP_PKTS) {
				net_pkt_ref(pkt);
				response_pkts[responses_count++] = pkt;
				k_sem_give(&wait_data);
			}
		}
	}

	return 0;
}

static struct dummy_api net_iface_api = {
	.iface_api.init = net_iface_init,
	.send = sender_iface,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT_INSTANCE(net_iface1_test,
			 "iface1",
			 iface1,
			 NULL,
			 NULL,
			 &net_iface1_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE,
			 127);

/* A second interface is used to check that the responder recovers its mDNS
 * multicast groups on interfaces other than the first one. This exercises the
 * interface handling that used to index a per-interface array with a value
 * derived from the interface index.
 */
static struct net_if_test net_iface2_data;

static void net_iface_init2(struct net_if *iface)
{
	struct net_if_test *data = net_if_get_device(iface)->data;

	/* Give the second interface a distinct link address. */
	data->mac_addr[0] = 0x00;
	data->mac_addr[1] = 0x00;
	data->mac_addr[2] = 0x5E;
	data->mac_addr[3] = 0x00;
	data->mac_addr[4] = 0x53;
	data->mac_addr[5] = 0x02;

	net_if_set_link_addr(iface, data->mac_addr, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
	net_if_flag_set(iface, NET_IF_IPV6_NO_ND);
}

static struct dummy_api net_iface_api2 = {
	.iface_api.init = net_iface_init2,
	.send = sender_iface,
};

NET_DEVICE_INIT_INSTANCE(net_iface2_test,
			 "iface2",
			 iface2,
			 NULL,
			 NULL,
			 &net_iface2_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api2,
			 _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE,
			 127);

static void *test_setup(void)
{
	struct net_if_addr *ifaddr;
	int idx;

	memset(response_pkts, 0, sizeof(response_pkts));
	memset(services, 0, sizeof(services));
	memset(records, 0, sizeof(records));

	/* Cross assign records and buffers to entries for allocation */
	for (int i = 0; i < EXT_RECORDS_NUM; ++i) {
		services[i].record = &records[i];

		records[i].instance = services[i].instance;
		records[i].service = services[i].service;
		records[i].proto = services[i].proto;
		records[i].domain = services[i].domain;
		records[i].text = services[i].text;
		records[i].port = &services[i].port;
	}

	mdns_responder_set_ext_records(records, EXT_RECORDS_NUM);

	/* The semaphore is there to wait the data to be received. */
	k_sem_init(&wait_data, 0, UINT_MAX);

	iface1 = net_if_get_by_index(1);

	zassert_not_null(iface1, "Iface1 is NULL");

	((struct net_if_test *) net_if_get_device(iface1)->data)->idx =
		net_if_get_by_iface(iface1);

	idx = net_if_get_by_iface(iface1);
	zassert_equal(idx, 1, "Invalid index iface1");

	zassert_not_null(iface1, "Interface 1");

	ifaddr = net_if_ipv6_addr_add(iface1, &ll_addr,
				      NET_ADDR_MANUAL, 0);

	net_ipv6_nbr_add(iface1, &sender_ll_addr, net_if_get_link_addr(iface1), false,
			 NET_IPV6_NBR_STATE_STATIC);

	zassert_not_null(ifaddr, "Failed to add LL-addr");

	/* we need to set the addresses preferred */
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface1, &extra_addr,
				      NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Failed to add second addr");
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_if_up(iface1);

	return NULL;
}

static void free_service(struct service_info *service)
{
	service->used = false;
	service->instance[0] = '\0';
	service->service[0] = '\0';
	service->proto[0] = '\0';
	service->domain[0] = '\0';
	service->port = 0;

}

static void free_ext_record(struct dns_sd_rec *rec)
{
	for (int i = 0; i < EXT_RECORDS_NUM; ++i) {
		if (services[i].record == rec) {
			free_service(&services[i]);
			return;
		}
	}
}

static void before(void *d)
{
	ARG_UNUSED(d);

	responses_count = 0;
	test_started = true;
}

static void cleanup(void *d)
{
	ARG_UNUSED(d);

	test_started = false;

	for (size_t i = 0; i < responses_count; ++i) {
		if (response_pkts[i]) {
			net_pkt_unref(response_pkts[i]);
			response_pkts[i] = NULL;
		}
	}

	/* Clear semaphore counter */
	while (k_sem_take(&wait_data, K_NO_WAIT) == 0) {
		/* NOP */
	}

	for (int i = 0; i < EXT_RECORDS_NUM; ++i) {
		if (services[i].used) {
			free_service(&services[i]);
		}
	}
}

static void send_msg(const uint8_t *data, size_t len)
{
	struct net_pkt *pkt;
	int res;

	pkt = net_pkt_alloc_with_buffer(iface1, NET_IPV6UDPH_LEN + len, NET_AF_UNSPEC,
					0, K_FOREVER);
	zassert_not_null(pkt, "PKT is null");

	res = net_pkt_write(pkt, ipv6_hdr_start, sizeof(ipv6_hdr_start));
	zassert_equal(res, 0, "pkt write for header start failed");

	res = net_pkt_write_be16(pkt, len + NET_UDPH_LEN);
	zassert_equal(res, 0, "pkt write for header length failed");

	res = net_pkt_write(pkt, ipv6_hdr_rest, sizeof(ipv6_hdr_rest));
	zassert_equal(res, 0, "pkt write for rest of the header failed");

	res = net_pkt_write_be16(pkt, 5353);
	zassert_equal(res, 0, "pkt write for UDP src port failed");

	res = net_pkt_write_be16(pkt, 5353);
	zassert_equal(res, 0, "pkt write for UDP dst port failed");

	res = net_pkt_write_be16(pkt, len + NET_UDPH_LEN);
	zassert_equal(res, 0, "pkt write for UDP length failed");

	/* to simplify testing checking of UDP checksum is disabled in prj.conf */
	res = net_pkt_write_be16(pkt, 0);
	zassert_equal(res, 0, "net_pkt_write_be16() for UDP checksum failed");

	res = net_pkt_write(pkt, data, len);
	zassert_equal(res, 0, "net_pkt_write() for data failed");

	res = net_recv_data(iface1, pkt);
	zassert_equal(res, 0, "net_recv_data() failed");
}

static struct dns_sd_rec *alloc_ext_record(const char *instance, const char *service,
					   const char *proto, const char *domain, uint8_t *txt,
					   size_t txt_len, uint16_t port)
{
	for (int i = 0; i < EXT_RECORDS_NUM; ++i) {
		if (!services[i].used) {
			services[i].used = true;

			strcpy(services[i].instance, instance);
			strcpy(services[i].service, service);
			strcpy(services[i].proto, proto);
			strcpy(services[i].domain, domain);

			if (txt && txt_len) {
				memcpy(services[i].text, txt, txt_len);
			}

			services[i].port = net_htons(port);
			services[i].record->text_size = txt_len;

			return services[i].record;
		}
	}

	return NULL;
}

static void check_service_type_enum_resp(struct net_pkt *pkt, const uint8_t *payload, size_t len)
{
	int res;

	net_pkt_cursor_init(pkt);

	net_pkt_set_overwrite(pkt, true);
	net_pkt_skip(pkt, NET_IPV6UDPH_LEN);

	res = net_buf_data_match(pkt->cursor.buf, pkt->cursor.pos - pkt->cursor.buf->data,
				 service_enum_start, sizeof(service_enum_start));

	zassert_equal(res, sizeof(service_enum_start),
		      "mDNS content beginning does not match");

	net_pkt_skip(pkt, sizeof(service_enum_start));

	res = net_pkt_get_len(pkt) - net_pkt_get_current_offset(pkt);
	zassert_equal(res, len, "Remaining packet's length does match payload's length");

	res = net_buf_data_match(pkt->cursor.buf, pkt->cursor.pos - pkt->cursor.buf->data, payload,
				 len);
	zassert_equal(res, len, "Payload does not match");
}

/* mDNS responder can advertise only ports that are bound - reuse its own port */
DNS_SD_REGISTER_UDP_SERVICE(foo, "zephyr", "_foo", "local", DNS_SD_EMPTY_TXT, 5353);

ZTEST(test_mdns_responder, test_external_records)
{
	int res;
	struct dns_sd_rec *records[EXT_RECORDS_NUM];

	records[0] = alloc_ext_record("test_rec", "_custom", "_tcp", "local", NULL, 0, 5353);
	zassert_not_null(records[0], "Failed to alloc the record");

	records[1] = alloc_ext_record("foo", "_bar", "_udp", "local", NULL, 0, 5353);
	zassert_not_null(records[1], "Failed to alloc the record");

	records[2] = alloc_ext_record("bar", "_foo", "_tcp", "local", NULL, 0, 5353);
	zassert_not_null(records[2], "Failed to alloc the record");

	/* Request service type enumeration */
	send_msg(dns_sd_service_enumeration_query, sizeof(dns_sd_service_enumeration_query));

	/* Expect 4 packets */
	for (int i = 0; i < 4; ++i) {
		res = k_sem_take(&wait_data, RESPONSE_TIMEOUT);
		zassert_equal(res, 0, "Did not receive a response number %d", i + 1);
	}

	/* Responder always starts with statically allocated services */
	check_service_type_enum_resp(response_pkts[0], payload_foo_udp_local,
				     sizeof(payload_foo_udp_local));

	/* Responder iterates through external records backwards so check responses in LIFO seq. */
	check_service_type_enum_resp(response_pkts[1], payload_foo_tcp_local,
				     sizeof(payload_foo_tcp_local));
	check_service_type_enum_resp(response_pkts[2], payload_bar_udp_local,
				     sizeof(payload_bar_udp_local));
	check_service_type_enum_resp(response_pkts[3], payload_custom_tcp_local,
				     sizeof(payload_custom_tcp_local));

	/* Remove record from the middle */
	free_ext_record(records[1]);

	/* Repeat service type enumeration */
	send_msg(dns_sd_service_enumeration_query, sizeof(dns_sd_service_enumeration_query));

	/* Expect 3 packets */
	for (int i = 0; i < 3; ++i) {
		res = k_sem_take(&wait_data, RESPONSE_TIMEOUT);
		zassert_equal(res, 0, "Did not receive a response number %d", i + 1);
	}

	/* Repeat checks without the removed record */
	check_service_type_enum_resp(response_pkts[4], payload_foo_udp_local,
				     sizeof(payload_foo_udp_local));
	check_service_type_enum_resp(response_pkts[5], payload_foo_tcp_local,
				     sizeof(payload_foo_tcp_local));
	check_service_type_enum_resp(response_pkts[6], payload_custom_tcp_local,
				     sizeof(payload_custom_tcp_local));
}

static void skip_labels(struct net_pkt *pkt)
{
	uint8_t label_len;

	while (true) {
		zassert_ok(net_pkt_read_u8(pkt, &label_len), "net_pkt read failed");
		if (label_len == 0) {
			break;
		}

		if ((label_len & NS_CMPRSFLGS) == NS_CMPRSFLGS) {
			zassert_ok(net_pkt_skip(pkt, 1), "net_pkt skip failed");
			break;
		}

		zassert_ok(net_pkt_skip(pkt, label_len), "net_pkt skip failed");
	}
}

static void validate_label(struct net_pkt *pkt, const char *label, bool last)
{
	uint8_t temp_buf[32];
	uint8_t label_len;

	zassert_ok(net_pkt_read_u8(pkt, &label_len), "net_pkt read failed");
	zassert_equal(label_len, strlen(label), "Invalid label");

	zassert_ok(net_pkt_read(pkt, &temp_buf, label_len), "net_pkt read failed");
	zassert_mem_equal(temp_buf, label, label_len);

	if (last) {
		zassert_ok(net_pkt_read_u8(pkt, &label_len), "net_pkt read failed");
		zassert_equal(label_len, 0, "Invalid label");
	}
}

static void check_basic_query_resp(struct net_pkt *pkt)
{
	struct dns_header resp_header;
	struct dns_rr resp_record;
	struct net_in6_addr resp_addr[2];

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);

	/* Jump to DNS payload */
	zassert_ok(net_pkt_skip(pkt, NET_IPV6UDPH_LEN), "net_pkt skip failed");

	/* Header */
	zassert_ok(net_pkt_read(pkt, &resp_header, sizeof(resp_header)), "net_pkt read failed");
	zassert_equal(net_ntohs(resp_header.ancount), 2, "Invalid record count");

	validate_label(pkt, "zephyr", false);
	validate_label(pkt, "local", true);

	/* First AAAA record */
	zassert_ok(net_pkt_read(pkt, &resp_record, sizeof(resp_record)), "net_pkt read failed");
	zassert_equal(net_ntohs(resp_record.type), DNS_RR_TYPE_AAAA, "Invalid record type");
	zassert_equal(net_ntohs(resp_record.rdlength), sizeof(struct net_in6_addr),
		      "Invalid record len");
	zassert_ok(net_pkt_read(pkt, &resp_addr[0], sizeof(struct net_in6_addr)),
		   "net_pkt read failed");

	/* Second AAAA record */
	skip_labels(pkt);
	zassert_ok(net_pkt_read(pkt, &resp_record, sizeof(resp_record)), "net_pkt read failed");
	zassert_equal(net_ntohs(resp_record.type), DNS_RR_TYPE_AAAA, "Invalid record type");
	zassert_equal(net_ntohs(resp_record.rdlength), sizeof(struct net_in6_addr),
		      "Invalid record len");
	zassert_ok(net_pkt_read(pkt, &resp_addr[1], sizeof(struct net_in6_addr)),
		   "net_pkt read failed");

	/* Verify both addresses belong to the network interface. */
	zassert_false(net_ipv6_addr_cmp(&resp_addr[0], &resp_addr[1]), "Got same address twice");
	zassert_not_null(net_if_ipv6_addr_lookup_by_iface(iface1, &resp_addr[0]),
			 "Address 1 not found");
	zassert_not_null(net_if_ipv6_addr_lookup_by_iface(iface1, &resp_addr[1]),
			 "Address 2 not found");
}

ZTEST(test_mdns_responder, test_basic_query)
{
	static uint8_t zephyr_local_query[] = {
		/* Header */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		/* zephyr.local */
		0x06, 0x7a, 0x65, 0x70, 0x68, 0x79, 0x72, 0x05, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x00,
		/* AAAA record */
		0x00, 0x1c, 0x00, 0x01
	};
	int res;

	/* Send basic mDNS query for zephyr.local */
	send_msg(zephyr_local_query, sizeof(zephyr_local_query));

	/* Expect response */
	res = k_sem_take(&wait_data, RESPONSE_TIMEOUT);
	zassert_ok(res, "Did not receive a response");

	check_basic_query_resp(response_pkts[0]);
}

static void check_basic_dns_sd_query_resp(struct net_pkt *pkt)
{
	struct dns_header resp_header;
	struct dns_rr resp_record;
	struct net_in6_addr resp_addr[2];

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);

	/* Jump to DNS payload */
	zassert_ok(net_pkt_skip(pkt, NET_IPV6UDPH_LEN), "net_pkt skip failed");

	/* Header */
	zassert_ok(net_pkt_read(pkt, &resp_header, sizeof(resp_header)), "net_pkt read failed");
	zassert_equal(net_ntohs(resp_header.ancount), 1, "Invalid record count");
	zassert_equal(net_ntohs(resp_header.arcount), 4, "Invalid record count");

	validate_label(pkt, "_foo", false);
	validate_label(pkt, "_udp", false);
	validate_label(pkt, "local", true);

	/* PTR answer record */
	zassert_ok(net_pkt_read(pkt, &resp_record, sizeof(resp_record)), "net_pkt read failed");
	zassert_equal(net_ntohs(resp_record.type), DNS_RR_TYPE_PTR, "Invalid record type");
	zassert_ok(net_pkt_skip(pkt, net_ntohs(resp_record.rdlength)), "net_pkt skip failed");

	/* TXT additional record */
	skip_labels(pkt);
	zassert_ok(net_pkt_read(pkt, &resp_record, sizeof(resp_record)), "net_pkt read failed");
	zassert_equal(net_ntohs(resp_record.type), DNS_RR_TYPE_TXT, "Invalid record type");
	zassert_ok(net_pkt_skip(pkt, net_ntohs(resp_record.rdlength)), "net_pkt skip failed");

	/* SRV additional record */
	skip_labels(pkt);
	zassert_ok(net_pkt_read(pkt, &resp_record, sizeof(resp_record)), "net_pkt read failed");
	zassert_equal(net_ntohs(resp_record.type), DNS_RR_TYPE_SRV, "Invalid record type");
	zassert_ok(net_pkt_skip(pkt, net_ntohs(resp_record.rdlength)), "net_pkt skip failed");

	/* First AAAA additional record */
	skip_labels(pkt);
	zassert_ok(net_pkt_read(pkt, &resp_record, sizeof(resp_record)), "net_pkt read failed");
	zassert_equal(net_ntohs(resp_record.type), DNS_RR_TYPE_AAAA, "Invalid record type");
	zassert_equal(net_ntohs(resp_record.rdlength), sizeof(struct net_in6_addr),
		      "Invalid record len");
	zassert_ok(net_pkt_read(pkt, &resp_addr[0], sizeof(struct net_in6_addr)),
		   "net_pkt read failed");

	/* Second AAAA additional record */
	skip_labels(pkt);
	zassert_ok(net_pkt_read(pkt, &resp_record, sizeof(resp_record)), "net_pkt read failed");
	zassert_equal(net_ntohs(resp_record.type), DNS_RR_TYPE_AAAA, "Invalid record type");
	zassert_equal(net_ntohs(resp_record.rdlength), sizeof(struct net_in6_addr),
		      "Invalid record len");
	zassert_ok(net_pkt_read(pkt, &resp_addr[1], sizeof(struct net_in6_addr)),
		   "net_pkt read failed");

	/* Verify both addresses belong to the network interface. */
	zassert_false(net_ipv6_addr_cmp(&resp_addr[0], &resp_addr[1]), "Got same address twice");
	zassert_not_null(net_if_ipv6_addr_lookup_by_iface(iface1, &resp_addr[0]),
			 "Address 1 not found");
	zassert_not_null(net_if_ipv6_addr_lookup_by_iface(iface1, &resp_addr[1]),
			 "Address 2 not found");
}

ZTEST(test_mdns_responder, test_basic_dns_sd_query)
{
	static uint8_t dns_sd_query[] = {
		/* Header */
		0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		/* _foo._udp.local */
		0x04, 0x5f, 0x66, 0x6f, 0x6f, 0x04, 0x5f, 0x75, 0x64, 0x70, 0x05, 0x6c,
		0x6f, 0x63, 0x61, 0x6c, 0x00,
		/* PTR record */
		0x00, 0x0c, 0x00, 0x01
	};
	int res;

	/* Send basic DNS SD query  */
	send_msg(dns_sd_query, sizeof(dns_sd_query));

	/* Expect response */
	res = k_sem_take(&wait_data, RESPONSE_TIMEOUT);
	zassert_ok(res, "Did not receive a response");

	check_basic_dns_sd_query_resp(response_pkts[0]);
}

/* Basic mDNS query for zephyr.local (AAAA), used to probe whether the
 * responder is currently reachable.
 */
static const uint8_t recovery_probe_query[] = {
	/* Header */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	/* zephyr.local */
	0x06, 0x7a, 0x65, 0x70, 0x68, 0x79, 0x72, 0x05, 0x6c, 0x6f, 0x63, 0x61, 0x6c, 0x00,
	/* AAAA record */
	0x00, 0x1c, 0x00, 0x01
};

/* Send a zephyr.local query and tell whether the responder answered. The
 * query is sent to the ff02::fb multicast address, so it is only delivered to
 * the responder's socket if the interface is still a member of that group.
 */
static bool responder_answers_query(void)
{
	/* Start from a clean slate so we only observe the response to the
	 * query we are about to send.
	 */
	responses_count = 0;
	while (k_sem_take(&wait_data, K_NO_WAIT) == 0) {
		/* drain stale responses */
	}

	send_msg(recovery_probe_query, sizeof(recovery_probe_query));

	return k_sem_take(&wait_data, RESPONSE_TIMEOUT) == 0;
}

/* Regression test for GitHub issue #112266: the mDNS responder must keep
 * answering queries after the network interface goes down and comes back up
 * (e.g. Ethernet cable removed and reattached). Bringing the interface down
 * makes the stack leave the ff02::fb multicast group; if the responder does
 * not rejoin it when the interface comes back up, subsequent queries are
 * dropped at the IPv6 input as "packet for unjoined multicast address".
 */
ZTEST(test_mdns_responder, test_recovery_after_iface_down_up)
{
	zassert_true(responder_answers_query(),
		     "Responder did not answer before the link went down");

	/* Simulate the Ethernet cable being removed and reattached. */
	zassert_ok(net_if_down(iface1), "Cannot bring the interface down");
	zassert_ok(net_if_up(iface1), "Cannot bring the interface back up");

	/* Let the NET_EVENT_IF_UP handler run so it can rejoin the mDNS
	 * multicast groups.
	 */
	k_sleep(K_MSEC(100));

	zassert_true(responder_answers_query(),
		     "Responder did not recover after the interface came back up");
}

/* Whether the interface is currently a member of the mDNS multicast groups.
 * A query is only delivered to the responder's socket while the group is
 * joined, so these mirror what the responder needs to keep working.
 */
static bool ipv4_group_joined(struct net_if *iface)
{
	const struct net_in_addr mcast = { { { 224, 0, 0, 251 } } };
	struct net_if_mcast_addr *maddr;

	maddr = net_if_ipv4_maddr_lookup(&mcast, &iface);

	return maddr != NULL && net_if_ipv4_maddr_is_joined(maddr);
}

static bool ipv6_group_joined(struct net_if *iface)
{
	struct net_in6_addr mcast;
	struct net_if_mcast_addr *maddr;

	net_ipv6_addr_create(&mcast, 0xff02, 0, 0, 0, 0, 0, 0, 0x00fb);
	maddr = net_if_ipv6_maddr_lookup(&mcast, &iface);

	return maddr != NULL && net_if_ipv6_maddr_is_joined(maddr);
}

/* Same scenario as test_recovery_after_iface_down_up, but checking the IPv4
 * IGMP group. Bringing the interface fully down leaves the group; the
 * responder must rejoin it (224.0.0.251) when the interface comes back up.
 */
ZTEST(test_mdns_responder, test_ipv4_group_recovery_after_iface_down_up)
{
	zassert_true(ipv4_group_joined(iface1),
		     "IPv4 mDNS group not joined before the link went down");

	zassert_ok(net_if_down(iface1), "Cannot bring the interface down");
	zassert_ok(net_if_up(iface1), "Cannot bring the interface back up");

	k_sleep(K_MSEC(100));

	zassert_true(ipv4_group_joined(iface1),
		     "IPv4 mDNS group not rejoined after the interface came back up");
}

/* Recovery must put a fresh IGMP membership report on the wire, not just flip
 * the local is_joined flag. An IGMP-snooping switch that dropped its state
 * during the outage needs that report to resume forwarding the mDNS multicast.
 * This is the case that the connection-manager AUTO_IF_DOWN reporter hit:
 * net_if_down() followed by net_if_up().
 */
ZTEST(test_mdns_responder, test_ipv4_igmp_report_reemitted_on_iface_down_up)
{
	zassert_true(ipv4_group_joined(iface1),
		     "IPv4 mDNS group not joined before the link went down");

	zassert_ok(net_if_down(iface1), "Cannot bring the interface down");
	k_sleep(K_MSEC(50));

	/* Count only what is emitted from here on (i.e. on the way back up). */
	igmp_report_count = 0;

	zassert_ok(net_if_up(iface1), "Cannot bring the interface back up");
	k_sleep(K_MSEC(200));

	zassert_true(ipv4_group_joined(iface1),
		     "IPv4 mDNS group not rejoined after the interface came back up");
	zassert_true(igmp_report_count > 0,
		     "No IGMP report was re-emitted for 224.0.0.251 on recovery");
}

/* Multicast addresses are reference counted. Recovery on NET_EVENT_IF_UP must
 * not take a new reference on every cycle, otherwise repeated link bounces
 * would leak references and the group could never actually be left. Cycle the
 * interface many times and verify the reference count stays bounded.
 */
static long ipv4_group_refcount(struct net_if *iface)
{
	const struct net_in_addr mcast = { { { 224, 0, 0, 251 } } };
	struct net_if_mcast_addr *maddr;

	maddr = net_if_ipv4_maddr_lookup(&mcast, &iface);
	if (maddr == NULL) {
		return -1;
	}

	return atomic_get(&maddr->atomic_ref);
}

ZTEST(test_mdns_responder, test_ipv4_group_ref_not_leaked_on_cycles)
{
	long ref_before, ref_after;

	ref_before = ipv4_group_refcount(iface1);
	zassert_true(ref_before > 0, "IPv4 mDNS group not joined at start");

	for (int i = 0; i < 5; i++) {
		zassert_ok(net_if_down(iface1), "Cannot bring the interface down");
		zassert_ok(net_if_up(iface1), "Cannot bring the interface back up");
		k_sleep(K_MSEC(50));
	}

	for (int i = 0; i < 5; i++) {
		net_if_carrier_off(iface1);
		net_if_carrier_on(iface1);
		k_sleep(K_MSEC(50));
	}

	ref_after = ipv4_group_refcount(iface1);
	zassert_true(ref_after > 0, "IPv4 mDNS group not joined after cycling");
	zassert_equal(ref_after, ref_before,
		      "IPv4 mDNS group reference count leaked across cycles "
		      "(%ld -> %ld)", ref_before, ref_after);
}

static long ipv6_group_refcount(struct net_if *iface)
{
	struct net_in6_addr mcast;
	struct net_if_mcast_addr *maddr;

	net_ipv6_addr_create(&mcast, 0xff02, 0, 0, 0, 0, 0, 0, 0x00fb);

	maddr = net_if_ipv6_maddr_lookup(&mcast, &iface);
	if (maddr == NULL) {
		return -1;
	}

	return atomic_get(&maddr->atomic_ref);
}

ZTEST(test_mdns_responder, test_ipv6_group_ref_not_leaked_on_cycles)
{
	long ref_before, ref_after;

	ref_before = ipv6_group_refcount(iface1);
	zassert_true(ref_before > 0, "IPv6 mDNS group not joined at start");

	for (int i = 0; i < 5; i++) {
		zassert_ok(net_if_down(iface1), "Cannot bring the interface down");
		zassert_ok(net_if_up(iface1), "Cannot bring the interface back up");
		k_sleep(K_MSEC(50));
	}

	for (int i = 0; i < 5; i++) {
		net_if_carrier_off(iface1);
		net_if_carrier_on(iface1);
		k_sleep(K_MSEC(50));
	}

	ref_after = ipv6_group_refcount(iface1);
	zassert_true(ref_after > 0, "IPv6 mDNS group not joined after cycling");
	zassert_equal(ref_after, ref_before,
		      "IPv6 mDNS group reference count leaked across cycles "
		      "(%ld -> %ld)", ref_before, ref_after);
}

/* Reproduces the state the connection-manager reporter observed: the mDNS
 * IPv4 group is still marked "joined" when NET_EVENT_IF_UP is delivered (the
 * link bounce did not clear it). A plain net_ipv4_igmp_join() is then a no-op
 * (igmp.c returns early when the address is already joined), so no membership
 * report reaches an IGMP-snooping switch and the responder stops receiving
 * queries even though the local state looks fine. Recovery must force a fresh
 * report in this case.
 */
ZTEST(test_mdns_responder, test_ipv4_igmp_report_when_already_joined_on_if_up)
{
	zassert_true(ipv4_group_joined(iface1),
		     "IPv4 mDNS group should be joined at start");

	/* Group stays joined; only an IF_UP event is delivered (no down). */
	igmp_report_count = 0;

	net_mgmt_event_notify(NET_EVENT_IF_UP, iface1);
	k_sleep(K_MSEC(200));

	zassert_true(igmp_report_count > 0,
		     "No IGMP report was re-emitted when the group was already joined "
		     "on interface up");
}

/* Same, but for a carrier loss (Ethernet cable unplugged/replugged) without an
 * administrative down.
 */
ZTEST(test_mdns_responder, test_ipv4_igmp_report_reemitted_on_carrier_off_on)
{
	zassert_true(ipv4_group_joined(iface1),
		     "IPv4 mDNS group not joined before carrier off");

	net_if_carrier_off(iface1);
	k_sleep(K_MSEC(50));

	igmp_report_count = 0;

	net_if_carrier_on(iface1);
	k_sleep(K_MSEC(200));

	zassert_true(ipv4_group_joined(iface1),
		     "IPv4 mDNS group not rejoined after carrier came back");
	zassert_true(igmp_report_count > 0,
		     "No IGMP report was re-emitted for 224.0.0.251 after carrier came back");
}

/* A carrier loss (Ethernet cable unplugged) without an administrative down
 * only marks the multicast groups as not joined; the responder must still be
 * a member of both the IPv4 and IPv6 mDNS groups once the carrier returns.
 */
ZTEST(test_mdns_responder, test_group_recovery_after_carrier_off_on)
{
	zassert_true(ipv4_group_joined(iface1),
		     "IPv4 mDNS group not joined before carrier off");
	zassert_true(ipv6_group_joined(iface1),
		     "IPv6 mDNS group not joined before carrier off");

	net_if_carrier_off(iface1);
	net_if_carrier_on(iface1);

	k_sleep(K_MSEC(100));

	zassert_true(ipv4_group_joined(iface1),
		     "IPv4 mDNS group not rejoined after carrier came back");
	zassert_true(ipv6_group_joined(iface1),
		     "IPv6 mDNS group not rejoined after carrier came back");

	/* The responder must still answer queries after a carrier cycle. */
	zassert_true(responder_answers_query(),
		     "Responder did not recover after carrier came back");
}

/* Recovery must also work on interfaces other than the first one. The old
 * NET_EVENT_IF_UP handler never rejoined the IPv6 MLD group (ff02::fb) for
 * any interface, so a second interface lost mDNS after a down/up cycle. (Its
 * IPv4 handling worked for in-bounds interface indices, so the IPv4 checks
 * here mainly guard against regressions in the reworked handler.) Verify both
 * groups are restored on the second interface after a down/up cycle.
 */
ZTEST(test_mdns_responder, test_second_iface_group_recovery_after_down_up)
{
	struct net_if *iface2 = net_if_get_by_index(2);

	zassert_not_null(iface2, "Second interface is NULL");

	zassert_true(ipv4_group_joined(iface2),
		     "iface2 IPv4 mDNS group not joined before the link went down");
	zassert_true(ipv6_group_joined(iface2),
		     "iface2 IPv6 mDNS group not joined before the link went down");

	zassert_ok(net_if_down(iface2), "Cannot bring the second interface down");
	zassert_ok(net_if_up(iface2), "Cannot bring the second interface back up");

	k_sleep(K_MSEC(100));

	zassert_true(ipv4_group_joined(iface2),
		     "iface2 IPv4 mDNS group not rejoined after the interface came back up");
	zassert_true(ipv6_group_joined(iface2),
		     "iface2 IPv6 mDNS group not rejoined after the interface came back up");
}

ZTEST_SUITE(test_mdns_responder, NULL, test_setup, before, cleanup, NULL);
