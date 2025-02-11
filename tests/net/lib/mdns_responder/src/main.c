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
#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

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

static struct in6_addr ll_addr = {{{
0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0, 0x9f, 0x74, 0x88, 0x9c, 0x1b, 0x44, 0x72, 0x39
}}};

static struct in6_addr sender_ll_addr = {{{
0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0x9f, 0x74, 0x88, 0x9c, 0x1b, 0x44, 0x72, 0x39
}}};

static bool test_started;
static struct k_sem wait_data;
static struct net_pkt *response_pkts[MAX_RESP_PKTS];
static size_t responses_count;
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
	struct net_ipv6_hdr *hdr;

	if (!pkt->buffer) {
		return -ENODATA;
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

static void *test_setup(void)
{
	struct net_if_addr *ifaddr;
	int idx;

	memset(response_pkts, 0, sizeof(response_pkts));
	memset(services, 0, sizeof(services));
	memset(records, 0, sizeof(records));

	responses_count = 0;

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

	pkt = net_pkt_alloc_with_buffer(iface1, NET_IPV6UDPH_LEN + len, AF_UNSPEC,
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

			services[i].port = htons(port);
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

ZTEST(test_mdns_responder, test_external_records)
{
	int res;
	struct dns_sd_rec *records[EXT_RECORDS_NUM];

	/* mDNS responder can advertise only ports that are bound - reuse its own port */
	DNS_SD_REGISTER_UDP_SERVICE(foo, "zephyr", "_foo", "local", DNS_SD_EMPTY_TXT, 5353);

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

ZTEST_SUITE(test_mdns_responder, NULL, test_setup, before, cleanup, NULL);
