/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL LOG_LEVEL_DBG
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mdns_resp_probe_test);

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

/* Why this is a separate test suite/directory instead of living in
 * tests/net/lib/mdns_responder/:
 *
 * That suite's prj.conf does not enable CONFIG_MDNS_RESPONDER_PROBE, and its
 * tests drive dns_read() directly against hand-built packets on a two-iface
 * setup -- synchronous, with no real RFC 6762 probe/announce timing at all.
 *
 * This suite exists to exercise the real, timed probe -> init_listener ->
 * announce sequence (CONFIG_MDNS_RESPONDER_PROBE), on a single iface, with
 * outbound packets captured via the iface's .send callback instead of being
 * fed synchronously to dns_read(). test_announce_includes_dns_sd_records()
 * below needs that real sequence: it registers a DNS-SD record and then
 * waits out PROBE_SEQUENCE_TIMEOUT for a real unsolicited announce to be
 * sent, to confirm the announce includes the service's PTR/SRV/TXT records
 * (RFC 6762 8.3), not just its address records.
 *
 * Folding this into tests/net/lib/mdns_responder/'s existing ZTEST_SUITE
 * would mean bolting this single-iface, real-timing, packet-capture fixture
 * onto that suite's incompatible two-iface, synchronous one in the same
 * main.c, and adding a multi-second real sleep to any scenario that also
 * builds today's fast unit tests unless they're kept in separate tests.yaml
 * scenarios anyway -- at which point little is actually saved by sharing the
 * directory.
 */

/* RFC 6762 8.1 PROBE_TIMEOUT is 1750ms, plus up to a 250ms random initial
 * delay (see mdns_addr_event_handler()'s probe_delay) before the probe even
 * starts. Give a comfortable margin above the worst case for the full
 * probe -> init_listener -> announce sequence to complete.
 */
#define PROBE_SEQUENCE_TIMEOUT (K_SECONDS(4))

static struct net_if *iface1;

static struct net_if_test {
	uint8_t idx; /* not used for anything, just a dummy value */
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
} net_iface1_data;

static uint8_t mdns_server_ipv6_addr[] = {0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfb};

static struct net_in6_addr ll_addr = {
	{{0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0, 0x9f, 0x74, 0x88, 0x9c, 0x1b, 0x44, 0x72, 0x39}}};

static struct net_in6_addr sender_ll_addr = {
	{{0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0x9f, 0x74, 0x88, 0x9c, 0x1b, 0x44, 0x72, 0x39}}};

static bool test_started;
static struct k_sem wait_data;
static struct net_pkt *response_pkts[8];
static size_t responses_count;

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

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr), NET_LINK_ETHERNET);
	net_if_flag_set(iface, NET_IF_IPV6_NO_ND);
}

static int sender_iface(const struct device *dev, struct net_pkt *pkt)
{
	struct net_ipv6_hdr *hdr;

	if (!pkt->buffer) {
		return -ENODATA;
	}

	if (test_started && net_pkt_family(pkt) == NET_AF_INET6) {
		hdr = NET_IPV6_HDR(pkt);

		if (net_ipv6_addr_cmp_raw(hdr->dst, mdns_server_ipv6_addr) &&
		    responses_count < ARRAY_SIZE(response_pkts)) {
			net_pkt_ref(pkt);
			response_pkts[responses_count++] = pkt;
			k_sem_give(&wait_data);
		}
	}

	return 0;
}

static struct dummy_api net_iface_api = {
	.iface_api.init = net_iface_init,
	.send = sender_iface,
};

#define _ETH_L2_LAYER    DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT_INSTANCE(net_iface1_test, "iface1", iface1, NULL, NULL, &net_iface1_data, NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &net_iface_api, _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE, 127);

static void *test_setup(void)
{
	struct net_if_addr *ifaddr;

	memset(response_pkts, 0, sizeof(response_pkts));

	k_sem_init(&wait_data, 0, UINT_MAX);

	iface1 = net_if_get_by_index(1);
	zassert_not_null(iface1, "Iface1 is NULL");

	((struct net_if_test *)net_if_get_device(iface1)->data)->idx = net_if_get_by_iface(iface1);

	/* This IPv6 address is enough on its own to drive a full,
	 * non-racing probe -> init_listener -> announce cycle -- no need
	 * for IPv4/DHCP involvement in this test.
	 */
	ifaddr = net_if_ipv6_addr_add(iface1, &ll_addr, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Failed to add LL-addr");
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_ipv6_nbr_add(iface1, &sender_ll_addr, net_if_get_link_addr(iface1), false,
			 NET_IPV6_NBR_STATE_STATIC);

	net_if_up(iface1);

	return NULL;
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

	while (k_sem_take(&wait_data, K_NO_WAIT) == 0) {
		/* NOP */
	}
}

/* Peeks at a captured packet's first Answer record without any fatal
 * assertions (unlike validate_label()/check_*_resp() elsewhere in this
 * codebase) -- needed here because this test doesn't know in advance which of
 * several captured announce packets (address-only vs. DNS-SD) it's looking
 * at, and a non-matching one must not abort the test.
 */
static bool packet_has_ptr_answer(struct net_pkt *pkt, const char *service, const char *proto,
				  const char *domain)
{
	const char *expect[3] = {service, proto, domain};
	uint8_t label_len;
	char buf[DNS_SD_INSTANCE_MAX_SIZE + 1];

	net_pkt_cursor_init(pkt);
	net_pkt_set_overwrite(pkt, true);

	if (net_pkt_skip(pkt, NET_IPV6UDPH_LEN) || net_pkt_skip(pkt, sizeof(struct dns_header))) {
		return false;
	}

	for (int i = 0; i < 3; i++) {
		if (net_pkt_read_u8(pkt, &label_len) != 0 || label_len == 0 ||
		    label_len != strlen(expect[i])) {
			return false;
		}
		if (net_pkt_read(pkt, buf, label_len) != 0) {
			return false;
		}
		buf[label_len] = '\0';
		if (strcmp(buf, expect[i]) != 0) {
			return false;
		}
	}

	/* terminating root label */
	if (net_pkt_read_u8(pkt, &label_len) != 0 || label_len != 0) {
		return false;
	}

	return true;
}

/* Regression test for the RFC 6762 8.3 announce-completeness feature:
 * send_announce() used to only ever announce address (A/AAAA) records on
 * startup/re-announce, never a registered DNS-SD service's PTR/SRV/TXT
 * records, even though RFC 6762 8.3 requires announcing *all* of a
 * responder's newly registered records. This registers one external DNS-SD
 * record, drives the responder through a normal (non-racing) bring-up, and
 * checks that at least one of the announce packets sent to the mDNS group
 * is a PTR answer for that service -- not just the address-only announce.
 */
ZTEST(test_mdns_responder_probe, test_announce_includes_dns_sd_records)
{
	static const uint16_t svc_port = sys_cpu_to_be16(5353);
	static const char instance[] = "zephyr";
	static const char service[] = "_zephyr";
	static const char proto[] = "_tcp";
	static const char domain[] = "local";
	static const struct dns_sd_rec record = {
		.instance = instance,
		.service = service,
		.proto = proto,
		.domain = domain,
		.text = DNS_SD_EMPTY_TXT,
		.text_size = sizeof(dns_sd_empty_txt),
		.port = &svc_port,
	};
	bool found_dns_sd_announce = false;

	zassert_ok(mdns_responder_set_ext_records(&record, 1),
		   "Failed to register the external DNS-SD record");

	k_sleep(PROBE_SEQUENCE_TIMEOUT);

	zassert_true(responses_count > 0, "No announce packets were sent at all");

	for (size_t i = 0; i < responses_count; i++) {
		if (packet_has_ptr_answer(response_pkts[i], service, proto, domain)) {
			found_dns_sd_announce = true;
			break;
		}
	}

	zassert_true(found_dns_sd_announce,
		     "None of the %zu announce packets included a PTR answer for the "
		     "registered DNS-SD service -- RFC 6762 8.3 announce-completeness "
		     "not honored",
		     responses_count);
}

ZTEST_SUITE(test_mdns_responder_probe, NULL, test_setup, before, cleanup, NULL);
