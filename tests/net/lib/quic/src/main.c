/*
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_QUIC_LOG_LEVEL);

#include <stdio.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/misc/lorem_ipsum.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/loopback.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/quic.h>

#include "net_private.h"
#include "quic_internal.h"
#include "quic_stats.h"
#include "quic_test.h"
#include "certificate.h"

#if defined(CONFIG_QUIC_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#define LOCAL_ADDR_IPV4 "192.0.2.1"
#define LOCAL_ADDR_IPV6 "2001:db8::1"

#define REMOTE_ADDR_IPV4 "192.0.2.2"
#define REMOTE_ADDR_IPV6 "2001:db8::2"

static struct net_sockaddr_in local_addr_ipv4;
static struct net_sockaddr_in6 local_addr_ipv6;

static struct net_sockaddr_in remote_addr_ipv4;
static struct net_sockaddr_in6 remote_addr_ipv6;

static struct quic_endpoint test_ep_a;
static struct quic_endpoint test_ep_b;

static bool test_started;
static bool test_failure;

/*
 * RFC 9001 Appendix A Test Vectors
 */

/* RFC 9001 Appendix A.1 - Destination Connection ID */
static const uint8_t rfc9001_dcid[] = {
	0x83, 0x94, 0xc8, 0xf0, 0x3e, 0x51, 0x57, 0x08
};

/* RFC 9001 Appendix A.1 - Client Initial Secret */
static const uint8_t rfc9001_client_initial_secret[] = {
	0xc0, 0x0c, 0xf1, 0x51, 0xca, 0x5b, 0xe0, 0x75,
	0xed, 0x0e, 0xbf, 0xb5, 0xc8, 0x03, 0x23, 0xc4,
	0x2d, 0x6b, 0x7d, 0xb6, 0x78, 0x81, 0x28, 0x9a,
	0xf4, 0x00, 0x8f, 0x1f, 0x6c, 0x35, 0x7a, 0xea,
};

/* RFC 9001 Appendix A.1 - Server Initial Secret */
static const uint8_t rfc9001_server_initial_secret[] = {
	0x3c, 0x19, 0x98, 0x28, 0xfd, 0x13, 0x9e, 0xfd,
	0x21, 0x6c, 0x15, 0x5a, 0xd8, 0x44, 0xcc, 0x81,
	0xfb, 0x82, 0xfa, 0x8d, 0x74, 0x46, 0xfa, 0x7d,
	0x78, 0xbe, 0x80, 0x3a, 0xcd, 0xda, 0x95, 0x1b
};

/* RFC 9001 Appendix A.1 - Client Key */
static const uint8_t rfc9001_client_key[] = {
	0x1f, 0x36, 0x96, 0x13, 0xdd, 0x76, 0xd5, 0x46,
	0x77, 0x30, 0xef, 0xcb, 0xe3, 0xb1, 0xa2, 0x2d
};

/* RFC 9001 Appendix A.1 - Client IV */
static const uint8_t rfc9001_client_iv[] = {
	0xfa, 0x04, 0x4b, 0x2f, 0x42, 0xa3, 0xfd, 0x3b,
	0x46, 0xfb, 0x25, 0x5c
};

/* RFC 9001 Appendix A.1 - Client HP Key */
static const uint8_t rfc9001_client_hp[] = {
	0x9f, 0x50, 0x44, 0x9e, 0x04, 0xa0, 0xe8, 0x10,
	0x28, 0x3a, 0x1e, 0x99, 0x33, 0xad, 0xed, 0xd2
};

/* RFC 9001 Appendix A.1 - Server Key */
static const uint8_t rfc9001_server_key[] = {
	0xcf, 0x3a, 0x53, 0x31, 0x65, 0x3c, 0x36, 0x4c,
	0x88, 0xf0, 0xf3, 0x79, 0xb6, 0x06, 0x7e, 0x37
};

/* RFC 9001 Appendix A.1 - Server IV */
static const uint8_t rfc9001_server_iv[] = {
	0x0a, 0xc1, 0x49, 0x3c, 0xa1, 0x90, 0x58, 0x53,
	0xb0, 0xbb, 0xa0, 0x3e
};

/* RFC 9001 Appendix A.1 - Server HP Key */
static const uint8_t rfc9001_server_hp[] = {
	0xc2, 0x06, 0xb8, 0xd9, 0xb9, 0xf0, 0xf3, 0x76,
	0x44, 0x43, 0x0b, 0x49, 0x0e, 0xea, 0xa3, 0x14
};

/*
 * RFC 9001 Appendix A.2 - Client Initial Packet
 *
 * IMPORTANT: The unprotected first byte is 0xc3, which means:
 * - Bits 7-6: 11 = Long header
 * - Bits 5-4: 00 = Initial packet
 * - Bits 3-2: 00 = Reserved
 * - Bits 1-0: 11 = PN Length encoding (3 means 4-byte PN)
 *
 * So the packet number is 4 bytes:  00 00 00 00 (value = 0)
 *
 * Structure:
 * Byte 0: First byte (0xc0 protected / 0xc3 unprotected)
 * Bytes 1-4: Version (00 00 00 01)
 * Byte 5: DCID length (08)
 * Bytes 6-13: DCID (83 94 c8 f0 3e 51 57 08)
 * Byte 14: SCID length (00)
 * Byte 15: Token length (00)
 * Bytes 16-17: Length (44 9e = 1182 as 2-byte varint)
 * Byte 18: PN (7b protected / 00 unprotected)
 * Bytes 19+: Ciphertext
 *
 * pn_offset = 18
 * sample_offset = pn_offset + 4 = 22
 */
static const uint8_t rfc9001_client_initial_protected[] = {
	0xc0, 0x00, 0x00, 0x00, 0x01, 0x08, 0x83, 0x94,
	0xc8, 0xf0, 0x3e, 0x51, 0x57, 0x08, 0x00, 0x00,
	0x44, 0x9e, 0x7b, 0x9a, 0xec, 0x34, 0xd1, 0xb1,
	0xc9, 0x8d, 0xd7, 0x68, 0x9f, 0xb8, 0xec, 0x11,
	0xd2, 0x42, 0xb1, 0x23, 0xdc, 0x9b, 0xd8, 0xba,
	0xb9, 0x36, 0xb4, 0x7d, 0x92, 0xec, 0x35, 0x6c,
	0x0b, 0xab, 0x7d, 0xf5, 0x97, 0x6d, 0x27, 0xcd,
	0x44, 0x9f, 0x63, 0x30, 0x00, 0x99, 0xf3, 0x99
};

struct eth_fake_context {
	struct net_if *iface;
	uint8_t mac_address[6];
};

static struct eth_fake_context eth_fake_data;

static struct quic_endpoint *reset_test_ep(struct quic_endpoint *ep)
{
	memset(ep, 0, sizeof(*ep));
	ep->sock = -1;

	return ep;
}

static void eth_fake_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->data;

	ctx->iface = iface;

	net_if_set_link_addr(iface, ctx->mac_address,
			     sizeof(ctx->mac_address),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int eth_fake_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	if (!test_started) {
		return 0;
	}

	return 0;
}

static struct ethernet_api eth_fake_api_funcs = {
	.iface_api.init = eth_fake_iface_init,
	.send = eth_fake_send,
};

ETH_NET_DEVICE_INIT(eth_fake, "eth_fake", NULL, NULL, &eth_fake_data, NULL,
		    CONFIG_ETH_INIT_PRIORITY, &eth_fake_api_funcs, NET_ETH_MTU);

static struct net_if *eth_iface;
static struct net_if *lo0;

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct net_if **my_iface = user_data;

	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		if (iface == NET_IF_GET(eth_fake, 0)) {
			*my_iface = iface;
		}
	}

	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		lo0 = iface;
		net_if_set_default(iface);
	}
}

static void setup_interfaces(void)
{
	struct net_if_addr *ifaddr;

	net_if_foreach(iface_cb, &eth_iface);
	zassert_not_null(eth_iface, "No ethernet interface found");

	ifaddr = net_if_ipv6_addr_add(eth_iface, &local_addr_ipv6.sin6_addr,
				      NET_ADDR_MANUAL, 0);
	if (ifaddr == NULL) {
		DBG("Cannot add IPv6 address %s\n",
		    net_sprint_ipv6_addr(&local_addr_ipv6.sin6_addr));
		zassert_not_null(ifaddr, "local addr");
	}

	ifaddr = net_if_ipv4_addr_add(eth_iface, &local_addr_ipv4.sin_addr,
				      NET_ADDR_MANUAL, 0);
	if (ifaddr == NULL) {
		DBG("Cannot add IPv4 address %s\n",
		    net_sprint_ipv4_addr(&local_addr_ipv4.sin_addr));
		zassert_not_null(ifaddr, "local addr");
	}

	net_if_up(eth_iface);
}

static void setup_certs(void)
{
	int ret;

	ret = tls_credential_add(CA_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca_certificate,
				 sizeof(ca_certificate));
	if (ret < 0) {
		LOG_ERR("Failed to register CA certificate: %d", ret);
	}

	ret = tls_credential_add(SERVER_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
				 server_certificate,
				 sizeof(server_certificate));
	if (ret < 0) {
		LOG_ERR("Failed to register server public certificate: %d", ret);
	}

	ret = tls_credential_add(SERVER_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_PRIVATE_KEY,
				 server_private_key, sizeof(server_private_key));
	if (ret < 0) {
		LOG_ERR("Failed to register server private key: %d", ret);
	}

	/* Register client certificate (both client/server certs are signed by same CA) */
	ret = tls_credential_add(CLIENT_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_PUBLIC_CERTIFICATE,
				 client_certificate,
				 sizeof(client_certificate));
	if (ret < 0) {
		LOG_ERR("Failed to register client public certificate: %d", ret);
	}

	ret = tls_credential_add(CLIENT_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_PRIVATE_KEY,
				 client_private_key, sizeof(client_private_key));
	if (ret < 0) {
		LOG_ERR("Failed to register client private key: %d", ret);
	}
}

ZTEST(net_socket_quic, test_zzzzzz_must_be_the_last_test)
{
	zassert_false(test_failure, "Test fixture failure");
}

static struct test_config {
	int endpoint_count;
	int stream_count;
	int connection_count;
} test_cfg;

static void *setup(void)
{
	setup_certs();

	return &test_cfg;
}

static void test_endpoint_cb(struct quic_endpoint *ep, void *user_data)
{
	struct test_config *cfg = user_data;

	cfg->endpoint_count++;
}

static void test_stream_cb(struct quic_stream *stream, void *user_data)
{
	struct test_config *cfg = user_data;

	cfg->stream_count++;
}

static void test_stream_close_cb(struct quic_stream *stream, void *user_data)
{
	ARG_UNUSED(user_data);
	(void)zsock_close(stream->sock);
}

static void test_connection_cb(struct quic_context *ctx, void *user_data)
{
	struct test_config *cfg = user_data;

	cfg->connection_count++;
}

static void test_connection_close_cb(struct quic_context *ctx, void *user_data)
{
	ARG_UNUSED(user_data);
	(void)quic_connection_close(ctx->sock);
}

static void before(void *arg)
{
	struct test_config *cfg = arg;
	int ret;

	ret = zsock_inet_pton(NET_AF_INET, LOCAL_ADDR_IPV4, &local_addr_ipv4.sin_addr);
	zassert_equal(ret, 1, "Invalid local IPv4 address");

	ret = zsock_inet_pton(NET_AF_INET6, LOCAL_ADDR_IPV6, &local_addr_ipv6.sin6_addr);
	zassert_equal(ret, 1, "Invalid local IPv6 address");

	ret = zsock_inet_pton(NET_AF_INET, REMOTE_ADDR_IPV4, &remote_addr_ipv4.sin_addr);
	zassert_equal(ret, 1, "Invalid remote IPv4 address");

	ret = zsock_inet_pton(NET_AF_INET6, REMOTE_ADDR_IPV6, &remote_addr_ipv6.sin6_addr);
	zassert_equal(ret, 1, "Invalid remote IPv6 address");

	local_addr_ipv4.sin_family = NET_AF_INET;
	local_addr_ipv4.sin_port = net_htons(4242);

	local_addr_ipv6.sin6_family = NET_AF_INET6;
	local_addr_ipv6.sin6_port = net_htons(4242);

	remote_addr_ipv4.sin_family = NET_AF_INET;
	remote_addr_ipv4.sin_port = net_htons(4242);

	remote_addr_ipv6.sin6_family = NET_AF_INET6;
	remote_addr_ipv6.sin6_port = net_htons(4242);

	setup_interfaces();

	cfg->stream_count = 0;
	cfg->endpoint_count = 0;
	cfg->connection_count = 0;

	quic_stream_foreach(test_stream_cb, cfg);
	if (cfg->stream_count != 0) {
		LOG_ERR("Expected 0 streams at test start (was %d)",
			cfg->stream_count);
		test_failure = true;
	}

	quic_endpoint_foreach(test_endpoint_cb, cfg);
	if (cfg->endpoint_count != 0) {
		LOG_ERR("Expected 0 endpoints at test start (was %d)",
			cfg->endpoint_count);
		test_failure = true;
	}

	quic_context_foreach(test_connection_cb, cfg);
	if (cfg->connection_count != 0) {
		LOG_ERR("Expected 0 connections at test start (was %d)",
			cfg->connection_count);
		test_failure = true;
	}
}

#define MAX_WAIT_COUNT 3
#define WAIT_DELAY_MS 100

static void after(void *arg)
{
	struct test_config *cfg = arg;
	int loop = MAX_WAIT_COUNT;
	bool fail;

	/* Wait a few iterations to allow any pending cleanup to complete */
	while (loop-- > 0) {
		quic_stream_foreach(test_stream_close_cb, cfg);
		if (cfg->stream_count == 0) {
			fail = false;
			break;
		}

		fail = true;
		cfg->stream_count = 0;
		k_sleep(K_MSEC(WAIT_DELAY_MS));
	}

	if (fail) {
		LOG_ERR("Expected 0 streams at test end (was %d)",
			cfg->stream_count);
		test_failure = true;
	}

	loop = MAX_WAIT_COUNT;

	while (loop-- > 0) {
		quic_context_foreach(test_connection_close_cb, cfg);
		if (cfg->connection_count == 0) {
			fail = false;
			break;
		}

		fail = true;
		cfg->connection_count = 0;
		k_sleep(K_MSEC(WAIT_DELAY_MS));
	}

	if (fail) {
		LOG_ERR("Expected 0 connections at test end (was %d)",
			cfg->connection_count);
		test_failure = true;
	}

	loop = MAX_WAIT_COUNT;

	while (loop-- > 0) {
		quic_endpoint_foreach(test_endpoint_cb, cfg);
		if (cfg->endpoint_count == 0) {
			fail = false;
			break;
		}

		fail = true;
		cfg->endpoint_count = 0;
		k_sleep(K_MSEC(WAIT_DELAY_MS));
	}

	if (fail) {
		LOG_ERR("Expected 0 endpoints at test end (was %d)",
			cfg->endpoint_count);
		test_failure = true;
	}

	cfg->stream_count = 0;
	cfg->endpoint_count = 0;
	cfg->connection_count = 0;
}

static void assert_stream_type_and_id(int sock, int expected_type)
{
	net_socklen_t optlen = sizeof(int);
	uint64_t stream_id = UINT64_MAX;
	int stream_type = -1;
	int ret;

	zassert_true(quic_is_stream_socket(sock), "Socket %d must be a QUIC stream", sock);
	zassert_false(quic_is_connection_socket(sock),
		      "Socket %d must not be reported as a QUIC connection", sock);

	ret = zsock_getsockopt(sock, ZSOCK_SOL_QUIC, ZSOCK_QUIC_SO_STREAM_TYPE,
			       &stream_type, &optlen);
	zassert_equal(ret, 0, "Failed to get stream type on socket %d (%d)", sock, -errno);
	zassert_equal(optlen, sizeof(int), "Unexpected stream type size %u", optlen);
	zassert_equal(stream_type, expected_type,
		      "Socket %d stream type mismatch (%d != %d)",
		      sock, stream_type, expected_type);

	ret = quic_stream_get_id(sock, &stream_id);
	zassert_equal(ret, 0, "Failed to get stream ID on socket %d (%d)", sock, ret);
	zassert_equal(stream_id & 0x03, expected_type,
		      "Socket %d stream ID bits mismatch (%" PRIu64 ")", sock, stream_id);
}

static void copy_quic_connection_stats(int sock, struct net_stats_quic *stats)
{
	struct quic_context *ctx;

	zassert_not_null(stats, "Missing stats storage");

	ctx = quic_get_context(sock);
	zassert_not_null(ctx, "Failed to get QUIC context for socket %d", sock);

	memcpy(stats, &ctx->stats, sizeof(*stats));
}

static void copy_quic_global_stats(struct net_stats_quic_global *stats)
{
	zassert_not_null(stats, "Missing global stats storage");
	zassert_not_null(quic_stats, "Missing QUIC global stats");

	memcpy(stats, quic_stats, sizeof(*stats));
}

#if defined(CONFIG_QUIC_STATS_HISTORY)
struct quic_closed_stats_snapshot {
	struct quic_closed_context_stats entries[CONFIG_QUIC_STATS_HISTORY_SIZE];
	size_t count;
};

static void copy_quic_closed_context_stats_cb(const struct quic_closed_context_stats *stats,
					      void *user_data)
{
	struct quic_closed_stats_snapshot *snapshot = user_data;

	zassert_not_null(snapshot, "Missing closed stats snapshot");
	zassert_true(snapshot->count < ARRAY_SIZE(snapshot->entries),
		     "Too many closed stats entries (%zu)", snapshot->count);

	snapshot->entries[snapshot->count++] = *stats;
}

static void copy_quic_closed_context_stats(struct quic_closed_stats_snapshot *snapshot)
{
	zassert_not_null(snapshot, "Missing closed stats snapshot");

	memset(snapshot, 0, sizeof(*snapshot));
	quic_closed_context_stats_foreach(copy_quic_closed_context_stats_cb, snapshot);
}

static int max_quic_closed_context_id(const struct quic_closed_stats_snapshot *snapshot)
{
	int max_id = -1;

	for (size_t i = 0; i < snapshot->count; i++) {
		if (snapshot->entries[i].id > max_id) {
			max_id = snapshot->entries[i].id;
		}
	}

	return max_id;
}

static uint16_t quic_sockaddr_port(const struct net_sockaddr_storage *addr)
{
	if (addr == NULL) {
		return 0U;
	}

	if (addr->ss_family == NET_AF_INET6) {
		return net_ntohs(net_sin6(net_sad(addr))->sin6_port);
	}

	if (addr->ss_family == NET_AF_INET) {
		return net_ntohs(net_sin(net_sad(addr))->sin_port);
	}

	return 0U;
}

static size_t count_new_quic_closed_contexts(const struct quic_closed_stats_snapshot *snapshot,
					     int baseline_id)
{
	size_t count = 0U;

	for (size_t i = 0; i < snapshot->count; i++) {
		if (snapshot->entries[i].id > baseline_id) {
			count++;
		}
	}

	return count;
}

static const struct quic_closed_context_stats *find_new_quic_closed_context_stats(
	const struct quic_closed_stats_snapshot *snapshot, int baseline_id, bool is_server,
	uint16_t local_port, uint16_t remote_port)
{
	for (size_t i = 0; i < snapshot->count; i++) {
		const struct quic_closed_context_stats *stats = &snapshot->entries[i];

		if (stats->id <= baseline_id) {
			continue;
		}

		if (stats->is_server != is_server) {
			continue;
		}

		if (quic_sockaddr_port(&stats->local_addr) != local_port) {
			continue;
		}

		if (quic_sockaddr_port(&stats->remote_addr) != remote_port) {
			continue;
		}

		return stats;
	}

	return NULL;
}
#endif /* CONFIG_QUIC_STATS_HISTORY */

static void assert_quic_stats_zero(const struct net_stats_quic *stats, const char *who)
{
	zassert_equal(stats->handshake_init_rx, 0, "%s handshake_init_rx", who);
	zassert_equal(stats->handshake_init_tx, 0, "%s handshake_init_tx", who);
	zassert_equal(stats->handshake_resp_rx, 0, "%s handshake_resp_rx", who);
	zassert_equal(stats->handshake_resp_tx, 0, "%s handshake_resp_tx", who);
	zassert_equal(stats->invalid_handshake, 0, "%s invalid_handshake", who);
	zassert_equal(stats->peer_not_found, 0, "%s peer_not_found", who);
	zassert_equal(stats->invalid_packet, 0, "%s invalid_packet", who);
	zassert_equal(stats->invalid_key, 0, "%s invalid_key", who);
	zassert_equal(stats->invalid_packet_len, 0, "%s invalid_packet_len", who);
	zassert_equal(stats->decrypt_failed, 0, "%s decrypt_failed", who);
	zassert_equal(stats->drop_rx, 0, "%s drop_rx", who);
	zassert_equal(stats->drop_tx, 0, "%s drop_tx", who);
	zassert_equal(stats->alloc_failed, 0, "%s alloc_failed", who);
	zassert_equal(stats->valid_rx, 0, "%s valid_rx", who);
	zassert_equal(stats->valid_tx, 0, "%s valid_tx", who);
}

static void assert_quic_stats_no_errors(const struct net_stats_quic *stats, const char *who)
{
	zassert_equal(stats->invalid_handshake, 0, "%s invalid_handshake", who);
	zassert_equal(stats->peer_not_found, 0, "%s peer_not_found", who);
	zassert_equal(stats->invalid_packet, 0, "%s invalid_packet", who);
	zassert_equal(stats->invalid_key, 0, "%s invalid_key", who);
	zassert_equal(stats->invalid_packet_len, 0, "%s invalid_packet_len", who);
	zassert_equal(stats->decrypt_failed, 0, "%s decrypt_failed", who);
	zassert_equal(stats->drop_rx, 0, "%s drop_rx", who);
	zassert_equal(stats->drop_tx, 0, "%s drop_tx", who);
	zassert_equal(stats->alloc_failed, 0, "%s alloc_failed", who);
}

/* Test 010: Basic connection open/close */
ZTEST(net_socket_quic, test_010_open_connection_and_close)
{
	struct quic_context *ctx;
	struct quic_endpoint *ep;
	int ret;

	ret = quic_connection_open((struct net_sockaddr *)&remote_addr_ipv4,
				   (struct net_sockaddr *)&local_addr_ipv4);
	zassert_true(ret >= 0, "Failed to open QUIC connection (%d)", ret);

	ctx = quic_get_context(ret);
	zassert_not_null(ctx, "Failed to get QUIC context for socket %d", ret);
	ep = SYS_SLIST_PEEK_HEAD_CONTAINER(&ctx->endpoints, ep, node);
	zassert_not_null(ep, "Failed to get QUIC endpoint for socket %d", ret);

	zassert_equal(1, atomic_get(&ctx->refcount),
		      "Invalid refcount %d", (int)atomic_get(&ctx->refcount));
	zassert_equal(ep->crypto.tls.ks.key_exchange_group,
		      MBEDTLS_SSL_IANA_TLS_GROUP_SECP256R1,
		      "Unexpected default key exchange group 0x%04x",
		      ep->crypto.tls.ks.key_exchange_group);

	ret = quic_connection_close(ret);
	zassert_equal(0, ret, "Failed to close QUIC connection (%d)", ret);

	zassert_equal(0, atomic_get(&ctx->refcount),
		      "Invalid refcount %d after close", (int)atomic_get(&ctx->refcount));
}

/* Test 020: Stream open/close */
ZTEST(net_socket_quic, test_020_open_stream_and_close)
{
	struct quic_context *ctx;
	int ret, conn_sock, stream_sock;

	/* Use server-side connection (no remote_addr) to avoid handshake */
	ret = quic_connection_open(NULL,
				   (struct net_sockaddr *)&local_addr_ipv4);
	zassert_true(ret >= 0, "Failed to open QUIC connection (%d)", ret);

	conn_sock = ret;

	ctx = quic_get_context(ret);
	zassert_not_null(ctx, "Failed to get QUIC context for socket %d", ret);

	zassert_equal(1, atomic_get(&ctx->refcount),
		      "Invalid refcount %d", (int)atomic_get(&ctx->refcount));

	ret = quic_stream_open(conn_sock, QUIC_STREAM_SERVER,
			       QUIC_STREAM_BIDIRECTIONAL, 0);
	zassert_true(ret >= 0, "Failed to open QUIC stream (%d)", ret);

	stream_sock = ret;

	/* Stream will keep connection alive */
	zassert_equal(2, atomic_get(&ctx->refcount),
		      "Invalid refcount %d after close", (int)atomic_get(&ctx->refcount));

	ret = quic_stream_close(stream_sock);
	zassert_equal(0, ret, "Failed to close QUIC stream (%d)", ret);

	zassert_equal(1, atomic_get(&ctx->refcount),
		      "Invalid refcount %d after close", (int)atomic_get(&ctx->refcount));

	ret = quic_connection_close(conn_sock);
	zassert_equal(0, ret, "Failed to close QUIC connection (%d)", ret);

	zassert_equal(0, atomic_get(&ctx->refcount),
		      "Invalid refcount %d after close", (int)atomic_get(&ctx->refcount));
}

ZTEST(net_socket_quic, test_030_socket_kind_helpers)
{
	uint64_t stream_id = UINT64_MAX;
	int ret, conn_sock, stream_sock, udp_sock;

	ret = quic_connection_open(NULL,
				   (struct net_sockaddr *)&local_addr_ipv4);
	zassert_true(ret >= 0, "Failed to open QUIC connection (%d)", ret);
	conn_sock = ret;

	ret = quic_stream_open(conn_sock, QUIC_STREAM_SERVER,
			       QUIC_STREAM_BIDIRECTIONAL, 0);
	zassert_true(ret >= 0, "Failed to open QUIC stream (%d)", ret);
	stream_sock = ret;

	udp_sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(udp_sock >= 0, "Failed to open UDP socket (%d)", -errno);

	zassert_true(quic_is_connection_socket(conn_sock),
		     "Connection socket %d must be recognized", conn_sock);
	zassert_false(quic_is_stream_socket(conn_sock),
		      "Connection socket %d must not be reported as a stream", conn_sock);

	zassert_true(quic_is_stream_socket(stream_sock),
		     "Stream socket %d must be recognized", stream_sock);
	zassert_false(quic_is_connection_socket(stream_sock),
		      "Stream socket %d must not be reported as a connection", stream_sock);

	zassert_false(quic_is_stream_socket(udp_sock),
		      "UDP socket %d must not be reported as a QUIC stream", udp_sock);
	zassert_false(quic_is_connection_socket(udp_sock),
		      "UDP socket %d must not be reported as a QUIC connection", udp_sock);

	ret = quic_stream_get_id(conn_sock, &stream_id);
	zassert_equal(ret, -ENOENT, "Connection socket must reject quic_stream_get_id (%d)", ret);

	ret = quic_stream_get_id(udp_sock, &stream_id);
	zassert_equal(ret, -ENOENT, "UDP socket must reject quic_stream_get_id (%d)", ret);

	ret = zsock_close(udp_sock);
	zassert_equal(ret, 0, "Failed to close UDP socket %d (%d)", udp_sock, -errno);

	ret = quic_stream_close(stream_sock);
	zassert_equal(ret, 0, "Failed to close QUIC stream (%d)", ret);

	ret = quic_connection_close(conn_sock);
	zassert_equal(ret, 0, "Failed to close QUIC connection (%d)", ret);
}

ZTEST(net_socket_quic, test_040_stream_type_helpers_for_local_streams)
{
	int ret, conn_sock, bidi_stream_sock, uni_stream_sock;

	ret = quic_connection_open(NULL,
				   (struct net_sockaddr *)&local_addr_ipv4);
	zassert_true(ret >= 0, "Failed to open QUIC connection (%d)", ret);
	conn_sock = ret;

	ret = quic_stream_open(conn_sock, QUIC_STREAM_SERVER,
			       QUIC_STREAM_BIDIRECTIONAL, 0);
	zassert_true(ret >= 0, "Failed to open bidirectional QUIC stream (%d)", ret);
	bidi_stream_sock = ret;

	ret = quic_stream_open(conn_sock, QUIC_STREAM_SERVER,
			       QUIC_STREAM_UNIDIRECTIONAL, 0);
	zassert_true(ret >= 0, "Failed to open unidirectional QUIC stream (%d)", ret);
	uni_stream_sock = ret;

	assert_stream_type_and_id(bidi_stream_sock,
				  QUIC_STREAM_SERVER | QUIC_STREAM_BIDIRECTIONAL);
	assert_stream_type_and_id(uni_stream_sock,
				  QUIC_STREAM_SERVER | QUIC_STREAM_UNIDIRECTIONAL);

	ret = quic_stream_close(uni_stream_sock);
	zassert_equal(ret, 0, "Failed to close unidirectional stream (%d)", ret);

	ret = quic_stream_close(bidi_stream_sock);
	zassert_equal(ret, 0, "Failed to close bidirectional stream (%d)", ret);

	ret = quic_connection_close(conn_sock);
	zassert_equal(ret, 0, "Failed to close QUIC connection (%d)", ret);
}

/* Test 050: Variable-length integer encoding/decoding */
ZTEST(net_socket_quic, test_050_len_encode_decode)
{
	/* The values here are taken from RFC9000 Appending A.1 */
	uint8_t buf8[] = { 0xc2, 0x19, 0x7c, 0x5e, 0xff, 0x14, 0xe8, 0x8c };
	uint8_t buf4[] = { 0x9d, 0x7f, 0x3e, 0x7d };
	uint8_t buf2[] = { 0x7b, 0xbd };
	uint8_t buf1[] = { 0x25 };
	uint8_t encoded[8];
	uint64_t val;
	int ret;

	ret = quic_get_len(buf8, sizeof(buf8), &val);
	zassert_equal(ret, 8, "Failed to decode length from 8-byte buffer (%d)", ret);
	ret = quic_put_len(encoded, sizeof(encoded), val);
	zassert_ok(ret, "Failed to encode length to 8-byte buffer (%d)", ret);
	zassert_mem_equal(buf8, encoded, sizeof(buf8),
			  "Encoded 8-byte buffer does not match original");
	memset(encoded, 0, sizeof(encoded));

	ret = quic_get_len(buf4, sizeof(buf4), &val);
	zassert_equal(ret, 4, "Failed to decode length from 4-byte buffer (%d)", ret);
	ret = quic_put_len(encoded, sizeof(encoded), val);
	zassert_ok(ret, "Failed to encode length to 4-byte buffer (%d)", ret);
	zassert_mem_equal(buf4, encoded, sizeof(buf4),
			  "Encoded 4-byte buffer does not match original");
	memset(encoded, 0, sizeof(encoded));

	ret = quic_get_len(buf2, sizeof(buf2), &val);
	zassert_equal(ret, 2, "Failed to decode length from 2-byte buffer (%d)", ret);
	ret = quic_put_len(encoded, sizeof(encoded), val);
	zassert_ok(ret, "Failed to encode length to 2-byte buffer (%d)", ret);
	zassert_mem_equal(buf2, encoded, sizeof(buf2),
			  "Encoded 2-byte buffer does not match original");
	memset(encoded, 0, sizeof(encoded));

	ret = quic_get_len(buf1, sizeof(buf1), &val);
	zassert_equal(ret, 1, "Failed to decode length from 1-byte buffer (%d)", ret);
	ret = quic_put_len(encoded, sizeof(encoded), val);
	zassert_ok(ret, "Failed to encode length to 1-byte buffer (%d)", ret);
	zassert_mem_equal(buf1, encoded, sizeof(buf1),
			  "Encoded 1-byte buffer does not match original");
}

ZTEST(net_socket_quic, test_060_varint_helpers_reject_invalid_buffers)
{
	uint8_t buf[1] = { 0 };
	uint64_t val = 0;
	int ret;

	ret = quic_get_len(buf, 0, &val);
	zassert_equal(ret, -EINVAL, "Zero-length varint input must fail (%d)", ret);

	ret = quic_put_varint(buf, sizeof(buf), 0x40);
	zassert_equal(ret, -EINVAL, "Too-small varint output buffer must fail (%d)", ret);
}

ZTEST(net_socket_quic, test_070_version_negotiation_packet)
{
	static const uint8_t peer_scid[] = { 0xaa, 0xbb, 0xcc, 0xdd };
	static const uint8_t peer_dcid[] = { 0x01, 0x02, 0x03, 0x04,
					     0x05, 0x06, 0x07, 0x08 };
	static const uint8_t version0[] = { 0x00, 0x00, 0x00, 0x00 };
	static const uint8_t version1[] = {
		(QUIC_VERSION_1 >> 24) & 0xff,
		(QUIC_VERSION_1 >> 16) & 0xff,
		(QUIC_VERSION_1 >> 8) & 0xff,
		QUIC_VERSION_1 & 0xff
	};
	uint8_t packet[32];
	size_t pos = 0;
	int ret;

	ret = quic_build_version_negotiation_packet(packet, sizeof(packet),
						    peer_scid, sizeof(peer_scid),
						    peer_dcid, sizeof(peer_dcid));
	zassert_true(ret > 0, "Failed to build Version Negotiation packet (%d)", ret);

	zassert_equal(packet[pos++] & 0xc0, 0xc0, "Invalid VN first byte");
	zassert_mem_equal(&packet[pos], version0, 4, "VN packet must use version 0");
	pos += 4;

	zassert_equal(packet[pos++], sizeof(peer_scid), "Unexpected DCID length");
	zassert_mem_equal(&packet[pos], peer_scid, sizeof(peer_scid),
			  "VN DCID must echo peer SCID");
	pos += sizeof(peer_scid);

	zassert_equal(packet[pos++], sizeof(peer_dcid), "Unexpected SCID length");
	zassert_mem_equal(&packet[pos], peer_dcid, sizeof(peer_dcid),
			  "VN SCID must echo peer DCID");
	pos += sizeof(peer_dcid);

	zassert_mem_equal(&packet[pos],
			  version1,
			  4,
			  "VN packet must advertise QUIC v1");
}

ZTEST(net_socket_quic, test_080_frame_type_level_validation)
{
	int ret;

	ret = quic_validate_frame_type(QUIC_FRAME_TYPE_STREAM_BASE,
					 QUIC_SECRET_LEVEL_INITIAL);
	zassert_equal(ret, -EPROTO, "STREAM must be forbidden in Initial (%d)", ret);

	ret = quic_validate_frame_type(QUIC_FRAME_TYPE_NEW_TOKEN,
					 QUIC_SECRET_LEVEL_HANDSHAKE);
	zassert_equal(ret, -EPROTO, "NEW_TOKEN must be forbidden in Handshake (%d)", ret);

	ret = quic_validate_frame_type(QUIC_FRAME_TYPE_PATH_CHALLENGE,
					 QUIC_SECRET_LEVEL_INITIAL);
	zassert_equal(ret, -EPROTO, "PATH_CHALLENGE must be forbidden in Initial (%d)", ret);

	ret = quic_validate_frame_type(QUIC_FRAME_TYPE_HANDSHAKE_DONE,
					 QUIC_SECRET_LEVEL_APPLICATION);
	zassert_equal(ret, 0, "HANDSHAKE_DONE must be allowed in Application (%d)", ret);

	ret = quic_validate_frame_type(QUIC_FRAME_TYPE_CRYPTO,
					 QUIC_SECRET_LEVEL_HANDSHAKE);
	zassert_equal(ret, 0, "CRYPTO must be allowed in Handshake (%d)", ret);

	ret = quic_validate_frame_type(0x30, QUIC_SECRET_LEVEL_APPLICATION);
	zassert_equal(ret, -ENOTSUP, "Unknown frame type must be rejected (%d)", ret);
}

ZTEST(net_socket_quic, test_090_recovery_shutdown_stops_tracking)
{
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);

	quic_recovery_init(ep);
	quic_recovery_begin_shutdown(ep);
	quic_recovery_on_packet_sent(ep, QUIC_SECRET_LEVEL_APPLICATION, 1, 123, true);

	zassert_true(ep->recovery.closing, "Recovery must stay in shutdown state");
	zassert_equal(ep->recovery.bytes_in_flight, 0,
		      "Shutdown recovery must not track new in-flight bytes");
}

static void init_test_crypto_endpoint(struct quic_endpoint *ep,
				      enum quic_secret_level level)
{
	memset(ep, 0, sizeof(*ep));
	ep->crypto.initial.initialized = true;
	ep->crypto.tls.is_initialized = true;
	ep->crypto.rx_buf_level = level;
}

ZTEST(net_socket_quic, test_100_crypto_frame_duplicate_identical)
{
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	uint8_t frame[] = { QUIC_FRAME_TYPE_CRYPTO, 0x00, 0x04, 0x11, 0x22, 0x33, 0x44 };
	size_t consumed = 0;
	int ret;

	init_test_crypto_endpoint(ep, QUIC_SECRET_LEVEL_INITIAL);
	memcpy(ep->crypto.rx_buffer, &frame[3], 4);
	ep->crypto.rx_buf_len = 4;
	ep->crypto.stream[QUIC_SECRET_LEVEL_INITIAL].rx_offset = 4;
	ep->crypto.stream[QUIC_SECRET_LEVEL_INITIAL].tls_offset = 4;

	ret = handle_crypto_frame(ep, QUIC_SECRET_LEVEL_INITIAL, frame, sizeof(frame), &consumed);
	zassert_equal(ret, 0, "Identical duplicate CRYPTO data must be accepted (%d)", ret);
	zassert_equal(consumed, sizeof(frame), "Duplicate CRYPTO frame not fully consumed");
	zassert_mem_equal(ep->crypto.rx_buffer, &frame[3], 4,
			  "Duplicate CRYPTO frame must not corrupt buffer");
}

ZTEST(net_socket_quic, test_110_crypto_frame_overlap_mismatch)
{
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	uint8_t frame[] = { QUIC_FRAME_TYPE_CRYPTO, 0x02, 0x04, 0x33, 0xff, 0x55, 0x66 };
	static const uint8_t orig_buffer[] = { 0x11, 0x22, 0x33, 0x44 };
	size_t consumed = 0;
	int ret;

	init_test_crypto_endpoint(ep, QUIC_SECRET_LEVEL_INITIAL);
	ep->crypto.rx_buffer[0] = 0x11;
	ep->crypto.rx_buffer[1] = 0x22;
	ep->crypto.rx_buffer[2] = 0x33;
	ep->crypto.rx_buffer[3] = 0x44;
	ep->crypto.rx_buf_len = 4;
	ep->crypto.stream[QUIC_SECRET_LEVEL_INITIAL].rx_offset = 4;
	ep->crypto.stream[QUIC_SECRET_LEVEL_INITIAL].tls_offset = 4;

	ret = handle_crypto_frame(ep, QUIC_SECRET_LEVEL_INITIAL, frame, sizeof(frame), &consumed);
	zassert_equal(ret, -EPROTO, "Mismatched overlapping CRYPTO data must fail (%d)", ret);
	zassert_mem_equal(ep->crypto.rx_buffer,
			  orig_buffer,
			  4,
			  "Rejected overlap must leave buffered CRYPTO data unchanged");
}

ZTEST(net_socket_quic, test_120_crypto_frame_buffer_exceeded)
{
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	uint8_t frame[] = {
		QUIC_FRAME_TYPE_CRYPTO, 0x7f, 0xfd, 0x02, 0xaa, 0xbb
	};
	size_t consumed = 0;
	int ret;

	init_test_crypto_endpoint(ep, QUIC_SECRET_LEVEL_INITIAL);

	ret = handle_crypto_frame(ep, QUIC_SECRET_LEVEL_INITIAL, frame, sizeof(frame), &consumed);
	zassert_equal(ret, -ENOMEM, "Oversized CRYPTO frame must fail with ENOMEM (%d)", ret);
}

ZTEST(net_socket_quic, test_130_anti_amplification_budget)
{
	struct quic_endpoint *listen_ep = reset_test_ep(&test_ep_a);
	struct quic_endpoint *child_ep = reset_test_ep(&test_ep_b);

	child_ep->is_server = true;
	child_ep->parent = listen_ep;

	quic_endpoint_note_unvalidated_rx(child_ep, 1200);

	zassert_true(quic_endpoint_can_send_unvalidated(child_ep, 0),
		     "Zero-byte send should always fit");

#if defined(CONFIG_QUIC_SERVER_ANTI_AMPLIFICATION_LIMIT)
	zassert_equal(child_ep->anti_amplification.bytes_received, 1200,
		      "Unexpected RX credit");
	zassert_true(quic_endpoint_can_send_unvalidated(child_ep, 3600),
		     "3x budget should allow exactly 3600 bytes");
	zassert_equal((uint64_t)child_ep->anti_amplification.bytes_received * 3U, 3600ULL,
		      "Unexpected anti-amplification budget");

	child_ep->anti_amplification.bytes_sent = 3590;
	zassert_true(quic_endpoint_can_send_unvalidated(child_ep, 10),
		     "Remaining budget should allow final bytes");
	zassert_false(quic_endpoint_can_send_unvalidated(child_ep, 11),
		      "Send beyond remaining budget must fail");
	child_ep->anti_amplification.bytes_sent = 1;
	zassert_false(quic_endpoint_can_send_unvalidated(child_ep, SIZE_MAX),
		      "Huge send requests must not wrap anti-amplification accounting");
#else
	zassert_true(quic_endpoint_can_send_unvalidated(child_ep, SIZE_MAX),
		     "Budget check should be disabled");
#endif
}

static void init_test_rx_stream(struct quic_stream *stream,
				struct quic_endpoint *ep,
				uint64_t stream_max_data,
				uint64_t conn_max_data,
				uint64_t stream_id)
{
	memset(stream, 0, sizeof(*stream));
	memset(ep, 0, sizeof(*ep));

	stream->id = stream_id;
	stream->ep = ep;
	stream->local_max_data = stream_max_data;
	stream->local_max_data_sent = stream_max_data;
	stream->rx_buf.size = sizeof(stream->rx_buf.data);
	stream->stop_sending_error_code = QUIC_ERROR_NO_ERROR;

	k_mutex_init(&stream->cond.data_available);
	k_condvar_init(&stream->cond.recv);
	k_poll_signal_init(&stream->recv.signal);

	ep->sock = -1;
	ep->rx_fc.max_data = conn_max_data;
	ep->rx_fc.max_data_sent = conn_max_data;
}

ZTEST(net_socket_quic, test_140_stream_local_rx_limit_by_type)
{
	struct quic_endpoint *client_ep = reset_test_ep(&test_ep_a);
	struct quic_endpoint *server_ep = reset_test_ep(&test_ep_b);

	server_ep->is_server = true;

	zassert_equal(quic_stream_local_rx_limit(client_ep,
						 QUIC_STREAM_CLIENT |
						 QUIC_STREAM_BIDIRECTIONAL),
		      CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL,
		      "Wrong local bidi limit for client");
	zassert_equal(quic_stream_local_rx_limit(client_ep,
						 QUIC_STREAM_SERVER |
						 QUIC_STREAM_BIDIRECTIONAL),
		      CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE,
		      "Wrong remote bidi limit for client");
	zassert_equal(quic_stream_local_rx_limit(client_ep,
						 QUIC_STREAM_SERVER |
						 QUIC_STREAM_UNIDIRECTIONAL),
		      CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_UNI,
		      "Wrong remote uni limit for client");
	zassert_equal(quic_stream_local_rx_limit(client_ep,
						 QUIC_STREAM_CLIENT |
						 QUIC_STREAM_UNIDIRECTIONAL),
		      0, "Local uni stream should not accept peer data");

	zassert_equal(quic_stream_local_rx_limit(server_ep,
						 QUIC_STREAM_SERVER |
						 QUIC_STREAM_BIDIRECTIONAL),
		      CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_LOCAL,
		      "Wrong local bidi limit for server");
	zassert_equal(quic_stream_local_rx_limit(server_ep,
						 QUIC_STREAM_CLIENT |
						 QUIC_STREAM_BIDIRECTIONAL),
		      CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_BIDI_REMOTE,
		      "Wrong remote bidi limit for server");
	zassert_equal(quic_stream_local_rx_limit(server_ep,
						 QUIC_STREAM_CLIENT |
						 QUIC_STREAM_UNIDIRECTIONAL),
		      CONFIG_QUIC_INITIAL_MAX_STREAM_DATA_UNI,
		      "Wrong remote uni limit for server");
	zassert_equal(quic_stream_local_rx_limit(server_ep,
						 QUIC_STREAM_SERVER |
						 QUIC_STREAM_UNIDIRECTIONAL),
		      0, "Local uni stream should not accept peer data");
}

ZTEST(net_socket_quic, test_150_stream_rx_flow_control_limit)
{
	static struct quic_stream stream;
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	uint8_t data[9] = { 0 };
	int ret;

	init_test_rx_stream(&stream, ep, 8, 64, 1);

	ret = quic_stream_receive_data(&stream, 0, data, sizeof(data), false);
	zassert_equal(ret, -EPROTO, "Expected MAX_STREAM_DATA violation (%d)", ret);
	zassert_equal(stream.fc_bytes_received, 0,
		      "Stream RX accounting changed unexpectedly");
	zassert_equal(stream.highest_offset_received, 0,
		      "Highest offset changed on rejected data");
	zassert_equal(ep->rx_fc.bytes_received, 0,
		      "Connection RX accounting changed on rejected data");
	zassert_false(ep->recovery.closing,
		      "Synthetic endpoints must not enter recovery shutdown");
}

ZTEST(net_socket_quic, test_155_stream_rx_buffer_overflow_skips_close_on_test_endpoint)
{
	static struct quic_stream stream;
	static uint8_t oversize[CONFIG_QUIC_STREAM_RX_BUFFER_SIZE + 1];
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	int ret;

	init_test_rx_stream(&stream, ep, sizeof(oversize), sizeof(oversize), 1);

	ret = quic_stream_receive_data(&stream, 0, oversize, sizeof(oversize), false);
	zassert_equal(ret, -EPROTO,
		      "Expected RX buffer overflow to be fatal (%d)", ret);
	zassert_equal(stream.fc_bytes_received, 0,
		      "Overflow must not change stream RX accounting");
	zassert_equal(ep->rx_fc.bytes_received, 0,
		      "Overflow must not change connection RX accounting");
	zassert_false(ep->recovery.closing,
		      "Synthetic endpoints must not enter recovery shutdown");
}

ZTEST(net_socket_quic, test_160_connection_rx_flow_control_limit)
{
	static struct quic_stream stream1;
	static struct quic_stream stream2;
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	uint8_t data4[4] = { 0 };
	uint8_t data1[1] = { 0 };
	int ret;

	init_test_rx_stream(&stream1, ep, 16, 8, 0);
	init_test_rx_stream(&stream2, ep, 16, 8, 4);
	stream2.ep = ep;

	ret = quic_stream_receive_data(&stream1, 0, data4, sizeof(data4), false);
	zassert_equal(ret, 0, "Unexpected receive failure on stream1 (%d)", ret);

	ret = quic_stream_receive_data(&stream2, 0, data4, sizeof(data4), false);
	zassert_equal(ret, 0, "Unexpected receive failure on stream2 (%d)", ret);
	zassert_equal(ep->rx_fc.bytes_received, 8,
		      "Connection RX accounting should reach MAX_DATA");

	ret = quic_stream_receive_data(&stream2, 4, data1, sizeof(data1), false);
	zassert_equal(ret, -EPROTO, "Expected MAX_DATA violation (%d)", ret);
	zassert_equal(ep->rx_fc.bytes_received, 8,
		      "Connection RX accounting should not grow after violation");
}

ZTEST(net_socket_quic, test_165_connection_rx_flow_control_gap_counts_offsets)
{
	static struct quic_stream stream;
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	uint8_t data4[4] = { 0 };
	uint8_t data1[1] = { 0 };
	int ret;

	init_test_rx_stream(&stream, ep, 8, 8, 1);

	ret = quic_stream_receive_data(&stream, 4, data4, sizeof(data4), false);
	zassert_equal(ret, -EAGAIN, "Expected gap data to be queued (%d)", ret);
	zassert_equal(stream.highest_offset_received, 8,
		      "Gap data should advance the highest received end");
	zassert_equal(stream.fc_bytes_received, 8,
		      "Gap data should consume stream flow-control credit");
	zassert_equal(ep->rx_fc.bytes_received, 8,
		      "Gap data should consume connection flow-control credit");

	ret = quic_stream_receive_data(&stream, 0, data4, sizeof(data4), false);
	zassert_equal(ret, 0, "Expected missing prefix to succeed (%d)", ret);
	zassert_equal(stream.rx_buf.ooo_count, 0, "Gap replay should drain the OOO queue");
	zassert_equal(stream.rx_buf.tail - stream.rx_buf.head, 8,
		      "Gap replay should make 8 bytes readable");
	zassert_equal(stream.fc_bytes_received, 8,
		      "Filling the gap must not double-count stream credit");
	zassert_equal(ep->rx_fc.bytes_received, 8,
		      "Filling the gap must not double-count connection credit");

	ret = quic_stream_receive_data(&stream, 8, data1, sizeof(data1), false);
	zassert_equal(ret, -EPROTO, "Expected MAX_DATA violation after gap (%d)", ret);
	zassert_equal(stream.fc_bytes_received, 8,
		      "Violation must not change stream flow-control credit");
	zassert_equal(ep->rx_fc.bytes_received, 8,
		      "Violation must not change connection flow-control credit");
}

ZTEST(net_socket_quic, test_170_rx_flow_control_ooo_no_double_count)
{
	static struct quic_stream stream;
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	uint8_t data4[4] = { 0 };
	int ret;

	init_test_rx_stream(&stream, ep, 16, 8, 1);

	ret = quic_stream_receive_data(&stream, 4, data4, sizeof(data4), false);
	zassert_equal(ret, -EAGAIN, "Expected out-of-order data to be queued (%d)", ret);
	zassert_equal(stream.rx_buf.ooo_count, 1, "Expected one OOO segment");
	zassert_equal(stream.highest_offset_received, 8,
		      "OOO segment should advance highest received end");
	zassert_equal(stream.fc_bytes_received, 8,
		      "OOO segment should count against stream flow control");
	zassert_equal(ep->rx_fc.bytes_received, 8,
		      "OOO segment should count against connection flow control");

	ret = quic_stream_receive_data(&stream, 4, data4, sizeof(data4), false);
	zassert_equal(ret, -EAGAIN, "Expected duplicate OOO data to remain non-fatal (%d)", ret);
	zassert_equal(stream.rx_buf.ooo_count, 1, "Duplicate OOO data should not add slots");
	zassert_equal(stream.fc_bytes_received, 8,
		      "Duplicate OOO data must not double-count stream bytes");
	zassert_equal(ep->rx_fc.bytes_received, 8,
		      "Duplicate OOO data must not double-count connection bytes");

	ret = quic_stream_receive_data(&stream, 0, data4, sizeof(data4), false);
	zassert_equal(ret, 0, "Expected in-order data to succeed (%d)", ret);
	zassert_equal(stream.rx_buf.ooo_count, 0, "OOO queue should be replayed");
	zassert_equal(stream.rx_buf.tail - stream.rx_buf.head, 8,
		      "In-order replay should make 8 bytes readable");
	zassert_equal(stream.fc_bytes_received, 8,
		      "OOO replay must not double-count stream bytes");
	zassert_equal(ep->rx_fc.bytes_received, 8,
		      "OOO replay must not double-count connection bytes");
}

ZTEST(net_socket_quic, test_175_rx_flow_control_ooo_unbufferable_is_fatal)
{
	static struct quic_stream stream;
	static uint8_t oversize[CONFIG_QUIC_STREAM_OOO_SEG_SIZE + 1];
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	uint8_t data4[4] = { 0 };
	size_t slots = ARRAY_SIZE(stream.rx_buf.ooo);
	uint64_t queued_bytes = 4U * (slots + 1U);
	int ret;

	init_test_rx_stream(&stream, ep, 1024, 1024, 1);

	for (size_t i = 0; i < slots; i++) {
		ret = quic_stream_receive_data(&stream, 4U * (i + 1U),
					       data4, sizeof(data4), false);
		zassert_equal(ret, -EAGAIN,
			      "Expected OOO data %zu to be queued (%d)", i, ret);
	}

	zassert_equal(stream.rx_buf.ooo_count, slots, "Expected OOO queue to be full");
	zassert_equal(stream.highest_offset_received, queued_bytes,
		      "Unexpected highest received end after filling OOO queue");
	zassert_equal(stream.fc_bytes_received, queued_bytes,
		      "Unexpected stream RX accounting after filling OOO queue");
	zassert_equal(ep->rx_fc.bytes_received, queued_bytes,
		      "Unexpected connection RX accounting after filling OOO queue");

	ret = quic_stream_receive_data(&stream, 4, data4, sizeof(data4), false);
	zassert_equal(ret, -EAGAIN,
		      "Expected duplicate OOO data to remain non-fatal when full (%d)",
		      ret);
	zassert_equal(stream.rx_buf.ooo_count, slots,
		      "Duplicate OOO data should not consume extra slots");
	zassert_equal(stream.fc_bytes_received, queued_bytes,
		      "Duplicate OOO data must not change stream accounting");
	zassert_equal(ep->rx_fc.bytes_received, queued_bytes,
		      "Duplicate OOO data must not change connection accounting");

	ret = quic_stream_receive_data(&stream, queued_bytes,
				       data4, sizeof(data4), false);
	zassert_equal(ret, -EPROTO,
		      "Expected unbufferable OOO data to be fatal (%d)", ret);
	zassert_equal(stream.rx_buf.ooo_count, slots,
		      "Fatal OOO data should not change the queued segment count");
	zassert_equal(stream.highest_offset_received, queued_bytes,
		      "Fatal OOO data must not advance highest received end");
	zassert_equal(stream.fc_bytes_received, queued_bytes,
		      "Fatal OOO data must not change stream accounting");
	zassert_equal(ep->rx_fc.bytes_received, queued_bytes,
		      "Fatal OOO data must not change connection accounting");
	zassert_false(ep->recovery.closing,
		      "Synthetic endpoints must not enter recovery shutdown");

	init_test_rx_stream(&stream, ep, 4096, 4096, 1);

	ret = quic_stream_receive_data(&stream, 4, oversize, sizeof(oversize), false);
	zassert_equal(ret, -EPROTO,
		      "Expected oversized OOO data to be fatal (%d)", ret);
	zassert_equal(stream.rx_buf.ooo_count, 0, "Oversized OOO data must not be queued");
	zassert_equal(stream.highest_offset_received, 0,
		      "Oversized OOO data must not advance highest received end");
	zassert_equal(stream.fc_bytes_received, 0,
		      "Oversized OOO data must not change stream accounting");
	zassert_equal(ep->rx_fc.bytes_received, 0,
		      "Oversized OOO data must not change connection accounting");
	zassert_false(ep->recovery.closing,
		      "Synthetic endpoints must not enter recovery shutdown");
}

/*
 * Test 04: RFC 9001 A.1 - Initial secret derivation
 *
 * This test verifies the HKDF functions by deriving secrets directly
 * and comparing against RFC test vectors.
 */
ZTEST(net_socket_quic, test_180_rfc9001_initial_secret_derivation)
{
	uint8_t client_secret[QUIC_HASH_SHA2_256_LEN];
	uint8_t server_secret[QUIC_HASH_SHA2_256_LEN];
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	bool ret;

	ret = quic_setup_initial_secrets(ep,
					 rfc9001_dcid, sizeof(rfc9001_dcid),
					 client_secret, server_secret);
	zassert_true(ret, "Failed to derive initial secrets");

	DBG("Client secret derived:\n");
	for (int i = 0; i < 32; i++) {
		DBG("%02x ", client_secret[i]);
		if ((i + 1) % 16 == 0) {
			DBG("\n");
		}
	}

	DBG("Expected client secret:\n");
	for (int i = 0; i < 32; i++) {
		DBG("%02x ", rfc9001_client_initial_secret[i]);
		if ((i + 1) % 16 == 0) {
			DBG("\n");
		}
	}

	zassert_mem_equal(client_secret, rfc9001_client_initial_secret,
			  QUIC_HASH_SHA2_256_LEN,
			  "Client initial secret mismatch");

	zassert_mem_equal(server_secret, rfc9001_server_initial_secret,
			  QUIC_HASH_SHA2_256_LEN,
			  "Server initial secret mismatch");
}

/* Test 190: RFC 9001 A.1 - Client key derivation */
ZTEST(net_socket_quic, test_190_rfc9001_client_key_derivation)
{
	uint8_t key[QUIC_HASH_SHA2_128_LEN];
	uint8_t iv[TLS13_AEAD_NONCE_LENGTH];
	uint8_t hp[QUIC_HP_SAMPLE_LEN];
	int ret;

	/* Derive key */
	ret = quic_hkdf_expand_label(rfc9001_client_initial_secret,
				     QUIC_HASH_SHA2_256_LEN,
				     (const uint8_t *)"quic key", 8,
				     key, sizeof(key));
	zassert_ok(ret, "Failed to derive client key");
	zassert_mem_equal(key, rfc9001_client_key, sizeof(key),
			  "Client key mismatch");

	/* Derive IV */
	ret = quic_hkdf_expand_label(rfc9001_client_initial_secret,
				     QUIC_HASH_SHA2_256_LEN,
				     (const uint8_t *)"quic iv", 7,
				     iv, sizeof(iv));
	zassert_ok(ret, "Failed to derive client IV");
	zassert_mem_equal(iv, rfc9001_client_iv, sizeof(iv),
			  "Client IV mismatch");

	/* Derive HP key */
	ret = quic_hkdf_expand_label(rfc9001_client_initial_secret,
				     QUIC_HASH_SHA2_256_LEN,
				     (const uint8_t *)"quic hp", 7,
				     hp, sizeof(hp));
	zassert_ok(ret, "Failed to derive client HP key");
	zassert_mem_equal(hp, rfc9001_client_hp, sizeof(hp),
			  "Client HP key mismatch");
}

/* Test 200: RFC 9001 A.1 - Server key derivation */
ZTEST(net_socket_quic, test_200_rfc9001_server_key_derivation)
{
	uint8_t key[QUIC_HASH_SHA2_128_LEN];
	uint8_t iv[TLS13_AEAD_NONCE_LENGTH];
	uint8_t hp[QUIC_HP_SAMPLE_LEN];
	int ret;

	/* Derive key */
	ret = quic_hkdf_expand_label(rfc9001_server_initial_secret,
				     QUIC_HASH_SHA2_256_LEN,
				     (const uint8_t *)"quic key", 8,
				     key, sizeof(key));
	zassert_ok(ret, "Failed to derive server key");
	zassert_mem_equal(key, rfc9001_server_key, sizeof(key),
			  "Server key mismatch");

	/* Derive IV */
	ret = quic_hkdf_expand_label(rfc9001_server_initial_secret,
				     QUIC_HASH_SHA2_256_LEN,
				     (const uint8_t *)"quic iv", 7,
				     iv, sizeof(iv));
	zassert_ok(ret, "Failed to derive server IV");
	zassert_mem_equal(iv, rfc9001_server_iv, sizeof(iv),
			  "Server IV mismatch");

	/* Derive HP key */
	ret = quic_hkdf_expand_label(rfc9001_server_initial_secret,
				     QUIC_HASH_SHA2_256_LEN,
				     (const uint8_t *)"quic hp", 7,
				     hp, sizeof(hp));
	zassert_ok(ret, "Failed to derive server HP key");
	zassert_mem_equal(hp, rfc9001_server_hp, sizeof(hp),
			  "Server HP key mismatch");
}

/* Test 210: RFC 9001 A. 2 - Header protection mask generation */
ZTEST(net_socket_quic, test_210_rfc9001_hp_mask)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t hp_key_id;
	psa_status_t status;
	uint8_t mask[QUIC_HP_MASK_LEN];
	int ret;

	/*
	 * From RFC 9001 A.2, sample is taken from bytes 17-32 of the
	 * ciphertext portion of the packet (after pn_offset + 4).
	 *
	 * For the client Initial packet with pn_offset = 17:
	 * sample = ciphertext[4:20] starting from encrypted payload
	 */
	const uint8_t sample[] = {
		0xd1, 0xb1, 0xc9, 0x8d, 0xd7, 0x68, 0x9f, 0xb8,
		0xec, 0x11, 0xd2, 0x42, 0xb1, 0x23, 0xdc, 0x9b
	};

	/* Expected mask from RFC 9001 */
	const uint8_t expected_mask[] = {
		0x43, 0x7b, 0x9a, 0xec, 0x36
	};

	/* Import HP key */
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECB_NO_PADDING);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, 128);

	status = psa_import_key(&attributes, rfc9001_client_hp,
				sizeof(rfc9001_client_hp), &hp_key_id);
	zassert_equal(status, PSA_SUCCESS, "Failed to import HP key");

	/* Generate mask */
	ret = quic_hp_mask(hp_key_id, QUIC_CIPHER_AES_128_GCM, sample, mask);
	zassert_ok(ret, "Failed to generate HP mask");

	zassert_mem_equal(mask, expected_mask, sizeof(expected_mask),
			  "HP mask mismatch");

	psa_destroy_key(hp_key_id);
}

/* Test 220: RFC 9001 A.2 - Header decryption */
ZTEST(net_socket_quic, test_220_rfc9001_header_decryption)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t hp_key_id;
	psa_status_t status;
	uint8_t first_byte;
	uint32_t pn;
	size_t pn_len;
	int ret;

	/*
	 * RFC 9001 A.2 Client Initial:
	 *
	 * Packet structure:
	 * Offset 0: First byte (0xc0 protected)
	 * Offset 1-4: Version (00 00 00 01)
	 * Offset 5: DCID length (08)
	 * Offset 6-13: DCID
	 * Offset 14: SCID length (00)
	 * Offset 15: Token length (00)
	 * Offset 16-17: Length (44 9e)
	 * Offset 18: PN (0x7b protected, 0x00 unprotected)
	 * Offset 19+: Ciphertext
	 */

	/* Import HP key */
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECB_NO_PADDING);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, 128);

	status = psa_import_key(&attributes, rfc9001_client_hp,
				sizeof(rfc9001_client_hp), &hp_key_id);
	zassert_equal(status, PSA_SUCCESS, "Failed to import HP key");

	DBG("Testing header decryption\n");
	DBG("Protected packet (first 20 bytes):\n");
	for (int i = 0; i < 20 && i < sizeof(rfc9001_client_initial_protected); i++) {
		DBG("%02x ", rfc9001_client_initial_protected[i]);
	}
	DBG("\n");

	/*
	 * The pn_offset for the client Initial packet is 18 (after the
	 * 2-byte Length field at offset 16).
	 */
	ret = quic_decrypt_header(rfc9001_client_initial_protected,
				  sizeof(rfc9001_client_initial_protected),
				  18,  /* pn_offset */
				  hp_key_id,
				  QUIC_CIPHER_AES_128_GCM,
				  &first_byte,
				  &pn,
				  &pn_len);
	zassert_ok(ret, "Failed to decrypt header (%d)", ret);

	DBG("Decrypted:  first_byte=0x%02x, pn=%u, pn_len=%zu\n",
	    first_byte, pn, pn_len);

	/* Verify unprotected first byte (0xc3 for Initial with PN len 1) */
	zassert_equal(first_byte, 0xc3, "First byte mismatch: got 0x%02x", first_byte);

	/* Verify packet number length */
	zassert_equal(pn_len, 4, "PN length mismatch (got %zu)", pn_len);

	/* Verify packet number value */
	zassert_equal(pn, 2, "PN value mismatch:  got %u", pn);

	psa_destroy_key(hp_key_id);
}

/* Test 230: Full cipher setup from connection ID */
ZTEST(net_socket_quic, test_230_rfc9001_full_cipher_setup)
{
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	bool ret;

	ep->is_server = true;

	ret = quic_conn_init_setup(ep, rfc9001_dcid, sizeof(rfc9001_dcid));
	zassert_true(ret, "Failed to setup initial ciphers");

	/* Verify TX (server) ciphers are initialized */
	zassert_true(ep->crypto.initial.tx.hp.initialized, "TX HP not initialized");
	zassert_true(ep->crypto.initial.tx.pp.initialized, "TX PP not initialized");

	/* Verify RX (client) ciphers are initialized */
	zassert_true(ep->crypto.initial.rx.hp.initialized, "RX HP not initialized");
	zassert_true(ep->crypto.initial.rx.pp.initialized, "RX PP not initialized");

	/* Verify IV matches expected client IV (since we're server, RX = client) */
	zassert_mem_equal(ep->crypto.initial.rx.pp.iv, rfc9001_client_iv,
			  sizeof(rfc9001_client_iv), "RX IV mismatch");

	/* Verify TX IV matches expected server IV */
	zassert_mem_equal(ep->crypto.initial.tx.pp.iv, rfc9001_server_iv,
			  sizeof(rfc9001_server_iv), "TX IV mismatch");

	/* Clean up */
	quic_crypto_context_destroy(&ep->crypto.initial);
}

/* Test 240: Nonce construction */
ZTEST(net_socket_quic, test_240_nonce_construction)
{
	uint8_t nonce[12];
	uint8_t expected_last;

	/*
	 * RFC 9001 Section 5.3:
	 * nonce = iv XOR packet_number (right-aligned)
	 *
	 * For packet number 0 with client IV, nonce should equal IV.
	 */
	quic_construct_nonce(rfc9001_client_iv, sizeof(rfc9001_client_iv), 0, nonce);
	zassert_mem_equal(nonce, rfc9001_client_iv, sizeof(nonce),
			  "Nonce for PN=0 should equal IV");

	/* For packet number 1 */
	quic_construct_nonce(rfc9001_client_iv, sizeof(rfc9001_client_iv), 1, nonce);

	/* Last byte should be IV[11] XOR 1 */
	expected_last = rfc9001_client_iv[11] ^ 0x01;
	zassert_equal(nonce[11], expected_last, "Nonce last byte mismatch for PN=1");

	/* First 11 bytes should still match IV */
	zassert_mem_equal(nonce, rfc9001_client_iv, 11, "Nonce prefix mismatch");
}

/* Test 250: Packet number reconstruction */
ZTEST(net_socket_quic, test_250_pn_reconstruction)
{
	uint64_t full_pn;

	/*
	 * RFC 9000 Appendix A - Sample Packet Number Decoding
	 */

	/* Case 1: truncated=0, largest=0, bits=8 -> full=0 */
	full_pn = quic_reconstruct_pn(0, 8, 0);
	zassert_equal(full_pn, 0, "PN reconstruction failed for case 1");

	/* Case 2: truncated=1, largest=0, bits=8 -> full=1 */
	full_pn = quic_reconstruct_pn(1, 8, 0);
	zassert_equal(full_pn, 1, "PN reconstruction failed for case 2");

	/* Case 3: truncated=0, largest=255, bits=8 -> full=256 */
	full_pn = quic_reconstruct_pn(0, 8, 255);
	zassert_equal(full_pn, 256, "PN reconstruction failed for case 3");

	/* Case 4: truncated=0xa82f, largest=0xa82e, bits=16 -> full=0xa82f */
	full_pn = quic_reconstruct_pn(0xa82f, 16, 0xa82e);
	zassert_equal(full_pn, 0xa82f, "PN reconstruction failed for case 4");

	/* Case 5: truncated=0x9b32, largest=0xa82f30ea, bits=16 -> full=0xa82f9b32 */
	full_pn = quic_reconstruct_pn(0x9b32, 16, 0xa82f30eaULL);
	zassert_equal(full_pn, 0xa82f9b32ULL, "PN reconstruction failed for case 5");
}

/* Test 260: AEAD encryption/decryption round-trip */
ZTEST(net_socket_quic, test_260_aead_roundtrip)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	struct quic_pp_cipher pp = {0};
	psa_status_t status;
	uint8_t plaintext[] = "Hello, QUIC!";
	uint8_t header[] = {0xc3, 0x00, 0x00, 0x00, 0x01};
	uint8_t ciphertext[64];
	uint8_t decrypted[64];
	size_t ciphertext_len;
	size_t decrypted_len;
	int ret;

	/* Setup PP cipher with client key and IV */
	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT | PSA_KEY_USAGE_DECRYPT);
	psa_set_key_algorithm(&attributes, PSA_ALG_GCM);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, 128);

	status = psa_import_key(&attributes, rfc9001_client_key,
				sizeof(rfc9001_client_key), &pp.key_id);
	zassert_equal(status, PSA_SUCCESS, "Failed to import PP key");

	memcpy(pp.iv, rfc9001_client_iv, sizeof(rfc9001_client_iv));
	pp.iv_len = sizeof(rfc9001_client_iv);
	pp.cipher_algo = QUIC_CIPHER_AES_128_GCM;
	pp.initialized = true;

	ret = quic_encrypt_payload(&pp, 0, header, sizeof(header),
				   plaintext, sizeof(plaintext),
				   ciphertext, sizeof(ciphertext),
				   &ciphertext_len);
	zassert_ok(ret, "Encryption failed");
	zassert_equal(ciphertext_len, sizeof(plaintext) + 16, "Ciphertext length wrong");

	ret = quic_decrypt_payload(&pp, 0, header, sizeof(header),
				   ciphertext, ciphertext_len,
				   decrypted, sizeof(decrypted),
				   &decrypted_len);
	zassert_ok(ret, "Decryption failed");
	zassert_equal(decrypted_len, sizeof(plaintext), "Decrypted length wrong");
	zassert_mem_equal(decrypted, plaintext, sizeof(plaintext), "Decrypted mismatch");

	psa_destroy_key(pp.key_id);
}

/* Test 270: Header protection round-trip */
ZTEST(net_socket_quic, test_270_header_protection_roundtrip)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t hp_key_id;
	psa_status_t status;
#define MAX_PACKET_SIZE 64
	uint8_t packet[MAX_PACKET_SIZE];
	uint8_t original_first_byte;
	uint8_t original_pn[4];
	uint8_t first_byte;
	uint32_t pn;
	size_t pn_len;
	int ret;

	/*
	 * Create a minimal test packet:
	 * - First byte: 0xc3 (Initial, PN len = 1)
	 * - Header: some bytes
	 * - PN at offset 18
	 * - Fake ciphertext for sample at offset 14
	 */
	memset(packet, 0xAA, sizeof(packet));

	packet[0] = 0xc3;  /* Long header, Initial, PN len = 1 */
	packet[1] = 0x00;  /* Version */
	packet[2] = 0x00;
	packet[3] = 0x00;
	packet[4] = 0x01;
	packet[5] = 0x08;  /* DCID len */
	/* DCID at 6-13 */
	packet[14] = 0x00; /* SCID len */
	packet[15] = 0x00; /* Token len */
	packet[16] = 0x40; /* Length (2-byte varint) */
	packet[17] = 0x20;
	/* 4-byte PN at offset 18-21 */
	packet[18] = 0x00;
	packet[19] = 0x00;
	packet[20] = 0x00;
	packet[21] = 0x05; /* PN = 5 */
	/* Ciphertext from byte 22 onwards (filled with 0xAA) */

	original_first_byte = packet[0];
	memcpy(original_pn, &packet[18], 4);

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECB_NO_PADDING);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, 128);

	status = psa_import_key(&attributes, rfc9001_client_hp,
				sizeof(rfc9001_client_hp), &hp_key_id);
	zassert_equal(status, PSA_SUCCESS, "Failed to import HP key");

	/* Apply header protection */
	ret = quic_encrypt_header(packet, sizeof(packet),
				  18,  /* pn_offset */
				  4,   /* pn_length */
				  hp_key_id,
				  QUIC_CIPHER_AES_128_GCM);
	zassert_ok(ret, "Header protection failed");

	/* Verify modifications */
	zassert_not_equal(packet[0], original_first_byte, "First byte not modified");
	zassert_not_equal(packet[18], original_pn[0], "PN not modified");

	DBG("After HP:  first=0x%02x, pn=0x%02x\n", packet[0], packet[18]);

	/* Remove header protection */
	ret = quic_decrypt_header(packet, sizeof(packet),
				  18,  /* pn_offset */
				  hp_key_id,
				  QUIC_CIPHER_AES_128_GCM,
				  &first_byte, &pn, &pn_len);
	zassert_ok(ret, "Header unprotection failed");

	/* Verify recovered values */
	DBG("Recovered: first=0x%02x, pn=%u, pn_len=%zu\n", first_byte, pn, pn_len);

	zassert_equal(first_byte, original_first_byte,
		      "First byte mismatch:  got 0x%02x, expected 0x%02x",
		      first_byte, original_first_byte);
	zassert_equal(pn, 5, "PN mismatch: got %u", pn);
	zassert_equal(pn_len, 4, "PN length mismatch: got %zu", pn_len);

	psa_destroy_key(hp_key_id);
}

/* Test 280: Full packet decryption simulation */
ZTEST(net_socket_quic, test_280_full_packet_decrypt_setup)
{
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	bool ret;

	/*
	 * Setup endpoint as server to decrypt client Initial packet.
	 * Server RX keys = Client TX keys
	 */
	ep->is_server = true;

	ret = quic_conn_init_setup(ep, rfc9001_dcid, sizeof(rfc9001_dcid));
	zassert_true(ret, "Failed to setup ciphers");

	/* Verify we can access the decryption keys */
	zassert_true(ep->crypto.initial.rx.hp.initialized, "RX HP not ready");
	zassert_true(ep->crypto.initial.rx.pp.initialized, "RX PP not ready");

	/*
	 * At this point, ep->crypto.initial.rx contains the client's
	 * keys (HP and PP), which can be used to decrypt client Initial
	 * packets like rfc9001_client_initial_packet.
	 *
	 * The actual decryption would require parsing the full packet
	 * structure and calling quic_decrypt_packet().
	 */

	/* TBD: Implement full packet decryption test */

	/* Clean up */
	quic_crypto_context_destroy(&ep->crypto.initial);
	zassert_true(ep->crypto.initial.rx.hp.initialized == false, "RX HP not destroyed");
	zassert_true(ep->crypto.initial.rx.pp.initialized == false, "RX PP not destroyed");
	zassert_true(ep->crypto.initial.tx.hp.initialized == false, "TX HP not destroyed");
	zassert_true(ep->crypto.initial.tx.pp.initialized == false, "TX PP not destroyed");
	zassert_true(ep->crypto.initial.initialized == false, "Initial context not cleared");
}

/*
 * Test 15: Header protection with RFC 9001 A.2 vectors
 *
 * Uses the exact packet structure from RFC 9001 Appendix A.2
 */
ZTEST(net_socket_quic, test_290_header_protection_rfc9001)
{
	psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
	psa_key_id_t hp_key_id;
	psa_status_t status;
	int ret;
	uint8_t first_byte;
	uint32_t pn;
	size_t pn_len;

	/* RFC 9001 A.2 protected packet */
	uint8_t protected_pkt[] = {
		0xc0, 0x00, 0x00, 0x00, 0x01, 0x08, 0x83, 0x94,  /* 0-7 */
		0xc8, 0xf0, 0x3e, 0x51, 0x57, 0x08, 0x00, 0x00,  /* 8-15 */
		0x44, 0x9e, 0x7b, 0x9a, 0xec, 0x34, 0xd1, 0xb1,  /* 16-23 */
		0xc9, 0x8d, 0xd7, 0x68, 0x9f, 0xb8, 0xec, 0x11,  /* 24-31 */
		0xd2, 0x42, 0xb1, 0x23, 0xdc, 0x9b, 0xd8, 0xba   /* 32-39 */
	};

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_ENCRYPT);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECB_NO_PADDING);
	psa_set_key_type(&attributes, PSA_KEY_TYPE_AES);
	psa_set_key_bits(&attributes, 128);

	status = psa_import_key(&attributes, rfc9001_client_hp,
				sizeof(rfc9001_client_hp), &hp_key_id);
	zassert_equal(status, PSA_SUCCESS, "Failed to import HP key");

	/* Decrypt header */
	ret = quic_decrypt_header(protected_pkt, sizeof(protected_pkt),
				  18,  /* pn_offset */
				  hp_key_id,
				  QUIC_CIPHER_AES_128_GCM,
				  &first_byte, &pn, &pn_len);
	zassert_ok(ret, "Header decryption failed (%d)", ret);

	DBG("RFC 9001 A.2 decryption:\n");
	DBG("  first_byte = 0x%02x (expected 0xc3)\n", first_byte);
	DBG("  pn = %u (expected 2)\n", pn);
	DBG("  pn_len = %zu (expected 4)\n", pn_len);

	zassert_equal(first_byte, 0xc3, "First byte wrong: 0x%02x", first_byte);
	zassert_equal(pn_len, 4, "PN length wrong: %zu", pn_len);
	zassert_equal(pn, 2, "PN wrong: %u", pn);

	/* Now test encryption:  unprotected -> protected */
	uint8_t unprotected_pkt[] = {
		0xc3, 0x00, 0x00, 0x00, 0x01, 0x08, 0x83, 0x94,  /* 0-7 */
		0xc8, 0xf0, 0x3e, 0x51, 0x57, 0x08, 0x00, 0x00,  /* 8-15 */
		0x44, 0x9e, 0x00, 0x00, 0x00, 0x02, 0xd1, 0xb1,  /* 16-23: PN=0x00000002 at 18-21 */
		0xc9, 0x8d, 0xd7, 0x68, 0x9f, 0xb8, 0xec, 0x11,  /* 24-31 */
		0xd2, 0x42, 0xb1, 0x23, 0xdc, 0x9b, 0xd8, 0xba   /* 32-39 */
	};

	ret = quic_encrypt_header(unprotected_pkt, sizeof(unprotected_pkt),
				  18,  /* pn_offset */
				  4,   /* pn_length */
				  hp_key_id,
				  QUIC_CIPHER_AES_128_GCM);
	zassert_ok(ret, "Header encryption failed (%d)", ret);

	DBG("After HP encryption:\n");
	DBG("  first_byte = 0x%02x (expected 0xc0)\n", unprotected_pkt[0]);
	DBG("  pn bytes = %02x %02x %02x %02x (expected 7b 9a ec 34)\n",
	    unprotected_pkt[18], unprotected_pkt[19],
	    unprotected_pkt[20], unprotected_pkt[21]);

	zassert_equal(unprotected_pkt[0], 0xc0,
		      "Protected first byte wrong: 0x%02x", unprotected_pkt[0]);

	/* PN bytes:  00 00 00 02 XOR 7b 9a ec 36 = 7b 9a ec 34 */
	zassert_equal(unprotected_pkt[18], 0x7b, "Protected PN[0] wrong");
	zassert_equal(unprotected_pkt[19], 0x9a, "Protected PN[1] wrong");
	zassert_equal(unprotected_pkt[20], 0xec, "Protected PN[2] wrong");
	zassert_equal(unprotected_pkt[21], 0x34, "Protected PN[3] wrong");

	psa_destroy_key(hp_key_id);
}

#define MAX_BUF_SIZE 1024
#define POLL_TIMEOUT_MS 1000
#define HELPER_POLL_TIMEOUT_MS 50

struct config {
	int sock;             /* Server listening socket */
	int connected_sock;   /* Server accepted connection socket */
	int stream_recv_sock; /* Server accepted stream socket for receiving data */
	struct k_sem sem;
	int error;
	int counter;
	volatile bool test_done;
};

static int wait_for_pollin(struct config *data, struct zsock_pollfd *pfd)
{
	int ret;

	while (!data->test_done) {
		pfd->revents = 0;

		ret = zsock_poll(pfd, 1, HELPER_POLL_TIMEOUT_MS);
		if (ret < 0) {
			return -errno;
		}

		if (ret == 0) {
			continue;
		}

		if (pfd->revents & ZSOCK_POLLIN) {
			return 0;
		}

		if (pfd->revents & ZSOCK_POLLNVAL) {
			return -EBADF;
		}

		if (pfd->revents & (ZSOCK_POLLERR | ZSOCK_POLLHUP)) {
			return -EIO;
		}
	}

	return -ECANCELED;
}

static void server_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct config *data = (struct config *)p1;
	int server_sock = data->sock;
	int connected_sock;
	struct net_sockaddr_storage client_addr;
	net_socklen_t client_addr_len = sizeof(client_addr);
	static uint8_t buf[MAX_BUF_SIZE];
	ssize_t len = 0;
	int stream, ret;

	k_sem_give(&data->sem);

	struct zsock_pollfd pfd = {
		.fd = server_sock,
		.events = ZSOCK_POLLIN,
	};

	ret = wait_for_pollin(data, &pfd);
	if (ret == -ECANCELED) {
		LOG_DBG("Poll cancelled while waiting for client connection");
		return;
	}

	if (ret < 0) {
		data->error = ret;
		LOG_DBG("Poll error while waiting for client connection");
		return;
	}

	connected_sock = zsock_accept(server_sock,
				      (struct net_sockaddr *)&client_addr,
				      &client_addr_len);
	if (connected_sock < 0) {
		data->error = errno;
		LOG_DBG("Connection accept failed (%d)", -data->error);
		return;
	}

	data->connected_sock = connected_sock;

	/* Wait that the client creates a stream */
	pfd.fd = connected_sock;
	pfd.events = ZSOCK_POLLIN;
	pfd.revents = 0;

	ret = wait_for_pollin(data, &pfd);
	if (ret == -ECANCELED) {
		LOG_DBG("Poll cancelled while waiting for client stream");
		return;
	}

	if (ret < 0) {
		data->error = ret;
		LOG_DBG("Poll error while waiting for client stream");
		return;
	}

	stream = zsock_accept(connected_sock,
			      (struct net_sockaddr *)&client_addr,
			      &client_addr_len);
	if (stream < 0) {
		data->error = errno;
		LOG_DBG("Stream accept failed (%d)", -data->error);
		return;
	}

	data->stream_recv_sock = stream;

	pfd.fd = stream;
	pfd.events = ZSOCK_POLLIN;
	pfd.revents = 0;

	while (true) {
		ret = wait_for_pollin(data, &pfd);
		if (ret == -ECANCELED) {
			break;
		}

		if (ret < 0) {
			data->error = ret;
			LOG_DBG("Poll error on stream (%d)", data->error);
			break;
		}

		len = zsock_recv(stream, buf, sizeof(buf), 0);
		if (len == 0) {
			/* FIN received — client closed its send side cleanly */
			break;
		}

		if (len < 0) {
			data->error = -errno;
			LOG_DBG("Stream recv failed (%d)", data->error);
			break;
		}

		ret = zsock_send(stream, buf, len, 0);
		if (ret < 0) {
			data->error = -errno;
			LOG_DBG("Stream send failed (%d)", data->error);
			break;
		}
	}

}

static void quic_server_and_client_with_stats(const char *server, const char *client,
					      char *tx_buf, size_t tx_buf_len,
					      char *rx_buf, size_t rx_buf_len,
					      size_t batch_size,
					      struct net_stats_quic *client_stats,
					      struct net_stats_quic *server_stats)
{
	/* Implement a test that sets up a QUIC server and client
	 * using the socket API, performs a handshake, and exchanges
	 * some data. This would be an end-to-end test of the QUIC
	 * implementation.
	 */
	struct net_sockaddr_storage server_addr;
	struct net_sockaddr_storage client_addr;
	sec_tag_t server_sec_tags[] = {
		SERVER_CERTIFICATE_TAG,
	};
	sec_tag_t client_sec_tags[] = {
		CA_CERTIFICATE_TAG,
	};
	static const char * const alpn_list[] = {
		"test-quic",
		NULL
	};

	int server_sock, client_sock, client_stream_sock, server_stream_sock;
	int server_connected_sock = -1;
	k_tid_t tid;
	int counter = 5;
	int ret;

#define STACK_SIZE 2048
	static K_THREAD_STACK_DEFINE(server_thread_stack, STACK_SIZE);
	static struct k_thread server_thread_data;

	static struct config data;

	ret = k_sem_init(&data.sem, 0, 1);
	zassert_ok(ret, "Failed to initialize semaphore (%d)", ret);

	ret = net_ipaddr_parse(server, strlen(server),
			       (struct net_sockaddr *)&server_addr);
	zassert_true(ret, "Failed to parse server IP address %s (%d)",
		     server, ret);

	ret = net_ipaddr_parse(client, strlen(client),
			       (struct net_sockaddr *)&client_addr);
	zassert_true(ret, "Failed to parse client IP address %s (%d)",
		     client, ret);

	prepare_quic_socket(&server_sock,
			    NULL,
			    (const struct net_sockaddr *)&server_addr);
	prepare_quic_socket(&client_sock,
			    (const struct net_sockaddr *)&server_addr,
			    (const struct net_sockaddr *)&client_addr);

	zassert_true(server_sock >= 0, "Failed to create server socket");
	zassert_true(client_sock >= 0, "Failed to create client socket");

	setup_quic_certs(server_sock, server_sec_tags, ARRAY_SIZE(server_sec_tags));
	setup_alpn(server_sock, alpn_list, ARRAY_SIZE(alpn_list));

	data.sock = server_sock;
	data.counter = counter * ((tx_buf_len + batch_size - 1) / batch_size);
	data.error = 0;
	data.connected_sock = -1;
	data.stream_recv_sock = -1;
	data.test_done = false;

	/* Start listening on the server socket in a separate thread */
	tid = k_thread_create(&server_thread_data, server_thread_stack,
			      K_THREAD_STACK_SIZEOF(server_thread_stack),
			      server_thread, &data, NULL, NULL,
			      K_PRIO_PREEMPT(1), 0, K_FOREVER);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
#define MAX_NAME_LEN sizeof("quicX_test[xxx]")
		char name[MAX_NAME_LEN];

		snprintk(name, sizeof(name), "quic%d_test[%d]",
			 server_addr.ss_family == NET_AF_INET6 ? 6 : 4,
			 server_sock);
		k_thread_name_set(&server_thread_data, name);
	}

	k_thread_start(tid);

	ret = k_sem_take(&data.sem, K_FOREVER);
	zassert_ok(ret, "Failed to take semaphore (%d)", ret);

	setup_quic_certs(client_sock, client_sec_tags, ARRAY_SIZE(client_sec_tags));
	setup_alpn(client_sock, alpn_list, ARRAY_SIZE(alpn_list));

	/* Create a stream on the client and send some data to server */
	client_stream_sock = quic_stream_open(client_sock, QUIC_STREAM_CLIENT,
					      QUIC_STREAM_BIDIRECTIONAL, 0);
	zassert_true(client_stream_sock >= 0, "Failed to open client stream");

	for (int i = 0; i < counter; i++) {
		int sent = tx_buf_len;
		int idx = 0;
		int total_drained = 0;  /* Track bytes received during send retries */

		do {
			int to_send = MIN((int)batch_size, sent);
			int actually_sent;
			int recvd;

			/* Send with EAGAIN handling, poll and retry if flow control blocks */
			while (true) {
				ret = zsock_send(client_stream_sock, tx_buf + idx, to_send, 0);
				if (ret > 0) {
					break;
				}
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					struct zsock_pollfd pfd = {
						.fd = client_stream_sock,
						.events = ZSOCK_POLLOUT | ZSOCK_POLLIN,
					};
					int poll_ret;
					int drain_ret;

					poll_ret = zsock_poll(&pfd, 1, POLL_TIMEOUT_MS);
					zassert_true(poll_ret >= 0, "poll failed (%d)", -errno);

					/* Drain any incoming data while waiting to send.
					 * This is critical for flow control, the server may be
					 * waiting to send us MAX_DATA or echo data.
					 */
					if (!(pfd.revents & ZSOCK_POLLIN)) {
						continue;
					}

					drain_ret = zsock_recv(client_stream_sock,
							       rx_buf + total_drained,
							       rx_buf_len - total_drained,
							       ZSOCK_MSG_DONTWAIT);
					if (drain_ret > 0) {
						total_drained += drain_ret;
					}

					continue;
				}
				zassert_true(false, "Failed to send data on client stream %d (%d)",
					     client_stream_sock, -errno);
			}

			/* Don't assert ret == to_send; partial sends are valid */
			actually_sent = ret;

			/* Receive what was sent minus what we already drained */
			recvd = (total_drained > idx) ? (total_drained - idx) : 0;
			if (recvd > actually_sent) {
				recvd = actually_sent;  /* Don't count more than this chunk */
			}
			while (recvd < actually_sent) {
				ret = zsock_recv(client_stream_sock, rx_buf + idx + recvd,
						 actually_sent - recvd, 0);
				if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
					struct zsock_pollfd pfd = {
						.fd = client_stream_sock,
						.events = ZSOCK_POLLIN,
					};

					ret = zsock_poll(&pfd, 1, POLL_TIMEOUT_MS);
					zassert_true(ret >= 0, "poll failed (%d)", -errno);
					continue;
				}
				zassert_true(ret > 0,
					     "Failed to recv data on client stream %d (%d)",
					     client_stream_sock, -errno);
				recvd += ret;
			}

			zassert_mem_equal(rx_buf + idx, tx_buf + idx, actually_sent,
					  "Data mismatch on client stream %d",
					  client_stream_sock);

			idx  += actually_sent;
			sent -= actually_sent;

			/* Update total_drained to reflect idx advancement */
			if (total_drained < idx) {
				total_drained = idx;
			}

			k_msleep(10);
		} while (sent > 0);

		zassert_mem_equal(rx_buf, tx_buf, MIN(tx_buf_len, rx_buf_len),
				  "Received data does not match sent data on "
				  "client stream %d",
				  client_stream_sock);
	}

	data.test_done = true;

	memset(rx_buf, 0, rx_buf_len);
	memset(tx_buf, 0, tx_buf_len);

	/* We could also use zsock_close() here to close the stream and
	 * the connection.
	 */
	ret = quic_stream_close(client_stream_sock);
	zassert_equal(ret, 0, "Failed to close client stream %d (%d)",
		      client_stream_sock, ret);

	ret = k_thread_join(tid, K_MSEC(500));
	zassert_equal(ret, 0, "Cannot join thread (%d)", ret);

	server_stream_sock = data.stream_recv_sock;
	server_connected_sock = data.connected_sock;

	zassert_equal(data.error, 0, "Server thread reported error (%d)", data.error);

	if (client_stats != NULL) {
		copy_quic_connection_stats(client_sock, client_stats);
	}

	if (server_stats != NULL) {
		zassert_true(server_connected_sock >= 0, "Server did not accept a connection");
		copy_quic_connection_stats(server_connected_sock, server_stats);
	}

	ret = quic_stream_close(server_stream_sock);
	zassert_equal(ret, 0, "Failed to close server stream %d (%d)",
		      server_stream_sock, ret);

	ret = quic_connection_close(server_connected_sock);
	zassert_equal(ret, 0, "Failed to close server connection (%d)", ret);

	ret = quic_connection_close(client_sock);
	zassert_equal(ret, 0, "Failed to close client connection (%d)", ret);

	ret = quic_connection_close(server_sock);
	zassert_equal(ret, 0, "Failed to close server connection (%d)", ret);
}

static void quic_server_and_client(const char *server, const char *client,
				   char *tx_buf, size_t tx_buf_len,
				   char *rx_buf, size_t rx_buf_len,
				   size_t batch_size)
{
	quic_server_and_client_with_stats(server, client, tx_buf, tx_buf_len,
					  rx_buf, rx_buf_len, batch_size,
					  NULL, NULL);
}

#define LOCAL_ADDR_IPV6_STR "[::1]:12345"
#define REMOTE_ADDR_IPV6_STR "[::1]:9999"

#define SMALL_BUF_SIZE 128

ZTEST(net_socket_quic, test_300_quic_ipv6_server_and_client)
{
	static uint8_t tx_buf[SMALL_BUF_SIZE];
	static uint8_t rx_buf[SMALL_BUF_SIZE];
	int count;
	int ret;

	ret = loopback_set_packet_drop_ratio(0.0); /* No drops for this test */
	zassert_ok(ret, "Failed to set loopback packet drop ratio (%d)", ret);

	count = sys_rand16_get() % sizeof(tx_buf);
	sys_rand_get(tx_buf, count);

	quic_server_and_client(LOCAL_ADDR_IPV6_STR, REMOTE_ADDR_IPV6_STR,
			       tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf),
			       sizeof(tx_buf));
}

#define LOCAL_ADDR_IPV4_STR "127.0.0.1:54321"
#define REMOTE_ADDR_IPV4_STR "127.0.0.1:19999"

ZTEST(net_socket_quic, test_310_quic_ipv4_server_and_client)
{
	static uint8_t tx_buf[SMALL_BUF_SIZE];
	static uint8_t rx_buf[SMALL_BUF_SIZE];
	int count;
	int ret;

	ret = loopback_set_packet_drop_ratio(0.0); /* No drops for this test */
	zassert_ok(ret, "Failed to set loopback packet drop ratio (%d)", ret);

	count = sys_rand16_get() % sizeof(tx_buf);
	sys_rand_get(tx_buf, count);

	quic_server_and_client(LOCAL_ADDR_IPV4_STR, REMOTE_ADDR_IPV4_STR,
			       tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf),
			       sizeof(tx_buf));
}

/* IPv4 UDP packet with QUIC Initial header (too short payload)
 * QUIC: Initial, DCID=fb2d975016f72831, SCID=c0e644e9be5f36dc, PKN: 0, CRYPTO, PADDING
 */
static const uint8_t too_short_initial_msg[] =
	/* QUIC header and payload (partial and too short) */
"\xcf\x00\x00\x00\x01\x08"
"\xfb\x2d\x97\x50\x16\xf7\x28\x31\x08\xc0\xe6\x44\xe9\xbe\x5f\x36"
"\xdc\x00\x44\x96\x98\x17\xe1\x48\xa3\x61\xed\x5a\xa6\x85\x82\x57"
"\xba\xa5\xa3\x0b\x55\x75\xfe\x6b\x9a\xd6\xce\xa9\x51\xb9\x39\xe6"
"\x54\x12\x91\xce\x40\x2f\xa2\xcc\x29\xdb\xf4\xd2\x41\x7c\x51\x24"
"\xdf\x60\x11\x80\x4c\xc5\x0e\x4a\x37\x30\x4e\x94\x13\x9d\x24\x8e"
"\x5f\x31\xf8\x0f\xdc\x2c\xdd\xf0\xef\x2c\x9d\x42\x12\xfd\x89\x49"
"\x2f\x3c\xac\x57\x6b\x79\x07\x9f\x87\xcd\x9c\xcb\x70\x18\xb5\xb1"
"\x3a\x27\x31\x42\xf6\xa6\xf2\x15\xd8\x9a\x4c\xeb\x2d\xe5\x8f\x82"
"\xcc\x94\x78\x60\xd7\xfe\xaf\x94\x3c\x59\x38\xc4\xd0\x79\x8a\x08"
"\xb5\xcf\x15\x01\x1f\xa9\xf7\xf5\x5f\xcc\xdd\x2e\xd0\x4c\x25\x2f"
"\xbf\x16\x4b\x2e\x40\xaa\xf1\x1f\x8a\x16\xb0\x93\x0c\xf5\xa9\x8f"
"\xdc\x66\x8e\xdb\xb3\x53\x5b\x74\x7e\x53\x02\x03\xc9\xd8\x3e\x2a"
"\x4f\xd5\x55\xa0\xd3\x2b\xc1\xef\x32\x14\x81\xc9\xed\x7d\xd5\x05"
"\x49\x5d\xf2\x0f\xc2\x4a\xf0\x86\xb1\xad\xb9\x2d\x77\x41\x98\x94"
"\x2e\x74\x4b\x52\x65\x2a\x0f\xcf\xe8\x85\xd9\xe5\x25\xda\x26\x10"
"\x94\xf8\xbf\x2f\xa4\x77\xa1\xe4\x04\x13\x15\x2e\xac\xc8\x56\xa4"
"\x8a\x73\x41\x66\x63\x1b\x24\x32\x37\xc2\x4a\x05\x08\xf0\x76\x24"
"\x57\x13\x42\x98\xbe\xa4\x5d\xda\x50\xc4\xa4\xab\x58\x99\x18\x19"
"\xe1\x0d\x8f\xc3\xef\xa3\xe1\x76\xe9\x3c\x45\xf1\xa3\x1e\x60\x1c"
"\xf1\x5b\xa6\x33\x3d\x28\x4f\xcd\x9c\xa6\x0a\xc1\xb4\x2c\xce\xc8"
"\x85\x01\xad\xe1\x73\x4a\x9d\xba\x1c\xf5\xe4\xee\xb4\xf7\x33\xb1"
"\x4a\x64\xfa\xa0\x6e\xf7\x63\xf8\x6f\x26\x43\x39\xf3\x88\x40\x10"
"\x9c\x60\xb3\x22\x7b\x67\x3f\x2e\x0c\x14\xa7\xe8\x7f\x17\xf5\x7f"
"\x92\x18\x92\x4c\xe7\xc1\x96\x31\xa0\xd7\xe4\x1d\xac\x79\x8d\xb4"
"\xf3\xd1\x90\x8e\xd3\x1d\x6e\x4b\x46\x49\x6b\x55\x3e\x14\x7d\x3c"
"\xec\x1f\x41\xb9\xbe\x95\xfd\xef\xe7\xe3\x6b\xc4\xc9\xca\x6d\xa8"
"\xd8\x66\x19\xd8\x5f\x26\x69\x8a\xe3\xe5\xf7\xc4\x73\xfc\x0e\xa1"
"\xc7\x57\x61\x38\x60\xfa\x27\xeb\xc3\x4e\xd8\xb8\x43\x72\x1e\x95"
"\x4c\x15\x61\x1d\x6e\x01\x22\x4d\x57\x78\xfa\x6a\xb7\x1e\x56\xd0"
"\x95\xe0\x8d\x7e\x31\x0a\x14\x3e\x03\x63\x59\xb3\x12\xd8\x74\x66"
"\x56\xbd\xc9\xfc\xf4\x06\x6f\x60\x5f\x08\xa9\x95\x71\xc2\x77\x64"
"\x60\x9b\xcb\xd9\x20\x08\x07\x6b\xe8\x69\xc1\x2f\x01\x34\x84\x34"
"\xec\x45\x72\x36\xd6\xc4\x67\x24\x82\xfb\x6d\x9e\x83\xa4\x59\x4a"
"\x2d\xaa\x56\x61\x6d\xb0\x07\xe1\xf5\x51\x1a\x22\xe8\x95\xdc\xf0"
"\xdb\x66\x5c\x31\x03\x6b\x12\x00\x78\x50\x9c\x0f\xf9\xf0\xd6\x79"
"\x7c\xa2\x30\x58\x81\x8f\x3d\xa3\xfd\x30\x64\xfb\xc2\xaf\x93\x7c"
"\x9c\x10\x2f\xaa\x76\x3b\xd9\xab\x57\x53\x14\x81\xd4\x55\x1b\xd2"
"\xc8\x52\xd0\x4e\x5c\x37\xb8\xd8\x89\xfe\xae\x70\xe4\x52\x54\x53"
"\x5e\x6f\x97\x98\x9f\x0d\xa0\xf0\xf6\x6a\xb0\x68\x78\x11\xa4\xc2";

ZTEST(net_socket_quic, test_320_quic_initial_too_short)
{
	/* Make sure that if we receive an Initial packet that is too short to contain the
	 * full header, we drop the packet.
	 */
	struct quic_context *server_ctx;
	struct net_stats_quic_global stats_before;
	struct net_stats_quic_global stats_after;
	struct net_sockaddr_storage server_addr;
	struct net_sockaddr_storage client_addr;
	int server_sock, client_sock;
	int ret;
	const char *server;
	const char *client;

#define LOCAL_ADDR_IPV4_STR2 "127.0.0.1:54322"
#define REMOTE_ADDR_IPV4_STR2 "127.0.0.1:19992"
	server = REMOTE_ADDR_IPV4_STR;
	client = LOCAL_ADDR_IPV4_STR;

	ret = net_ipaddr_parse(server, strlen(server),
			       (struct net_sockaddr *)&server_addr);
	zassert_true(ret, "Failed to parse server IP address %s (%d)",
		     server, ret);

	prepare_quic_socket(&server_sock,
			    NULL,
			    (const struct net_sockaddr *)&server_addr);
	zassert_true(server_sock >= 0, "Could not create server socket (%d)",
		     -errno);

	ret = net_ipaddr_parse(client, strlen(client),
			       (struct net_sockaddr *)&client_addr);
	zassert_true(ret, "Failed to parse client IP address %s (%d)",
		     client, ret);

	client_sock = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(client_sock >= 0, "Could not create client socket (%d)",
		     -errno);

	ret = zsock_bind(client_sock, (const struct net_sockaddr *)&client_addr,
			 sizeof(client_addr));
	zassert_ok(ret, "Could not bind client socket (%d)", client_sock);

	server_ctx = quic_get_context(server_sock);
	zassert_not_null(server_ctx, "Could not get server QUIC context");
	assert_quic_stats_zero(&server_ctx->stats, "listener");
	copy_quic_global_stats(&stats_before);

	/* Send the short Initial packet to the server socket */
	ret = zsock_sendto(client_sock, too_short_initial_msg,
			    sizeof(too_short_initial_msg), 0,
			    (const struct net_sockaddr *)&server_addr,
			    sizeof(server_addr));
	zassert_equal(ret, sizeof(too_short_initial_msg),
		      "Failed to send full Initial packet (%d)", -errno);

	/* Let the socket service to forward the packet to the QUIC server */
	for (int i = 0; i < 10 && server_ctx->stats.drop_rx == 0; i++) {
		k_msleep(10);
	}

	zassert_equal(server_ctx->stats.invalid_packet_len, 1,
		      "Expected one invalid_packet_len update, got %u",
		      server_ctx->stats.invalid_packet_len);
	zassert_equal(server_ctx->stats.drop_rx, 1,
		      "Expected one drop_rx update, got %u",
		      server_ctx->stats.drop_rx);
	zassert_equal(server_ctx->stats.handshake_init_rx, 0,
		      "Too-short Initial must not be counted as valid handshake RX");
	zassert_equal(server_ctx->stats.valid_rx, 0,
		      "Too-short Initial must not be counted as valid RX");
	zassert_equal(server_ctx->stats.valid_tx, 0,
		      "Server must not send application packets for malformed Initial");
	copy_quic_global_stats(&stats_after);
	zassert_equal(stats_after.packets_rx - stats_before.packets_rx, 1U,
		      "Malformed Initial must still count as one received packet");

	ret = zsock_close(client_sock);
	zassert_ok(ret, "Cannot close client socket %d (%d)", client_sock, -errno);

	ret = quic_connection_close(server_sock);
	zassert_ok(ret, "Cannot close server socket %d (%d)", server_sock, -errno);
}

ZTEST(net_socket_quic, test_330_quic_initial_dcid_too_short)
{
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	struct net_sockaddr_in src_addr = { 0 };
	uint8_t packet[] = {
		0xc0,
		0x00, 0x00, 0x00, 0x01,
		0x07, 0x83, 0x94, 0xc8, 0xf0, 0x3e, 0x51, 0x57,
		0x08, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x00,
		0x01,
		0x00,
	};
	int ret;

	src_addr.sin_family = NET_AF_INET;
	src_addr.sin_port = net_htons(4242);
	net_addr_pton(NET_AF_INET, REMOTE_ADDR_IPV4, &src_addr.sin_addr);

	ep->local_addr.ss_family = NET_AF_INET;

	ret = process_long_header(ep, (struct net_sockaddr *)&src_addr,
				  sizeof(src_addr), packet,
				  1, 0, sizeof(packet), sizeof(packet) - 1, 1200);
	zassert_equal(ret, -EINVAL, "Expected too-short DCID to be rejected (%d)", ret);
}

ZTEST(net_socket_quic, test_335_quic_vn_ignores_initial_dcid_check)
{
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	struct net_sockaddr_in src_addr = { 0 };
	uint8_t packet[] = {
		0xc0,
		0x00, 0x00, 0x00, 0x00,
		0x07, 0x83, 0x94, 0xc8, 0xf0, 0x3e, 0x51, 0x57,
		0x08, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x00,
	};
	int ret;

	src_addr.sin_family = NET_AF_INET;
	src_addr.sin_port = net_htons(4242);
	net_addr_pton(NET_AF_INET, REMOTE_ADDR_IPV4, &src_addr.sin_addr);

	ep->local_addr.ss_family = NET_AF_INET;

	ret = process_long_header(ep, (struct net_sockaddr *)&src_addr,
				  sizeof(src_addr), packet,
				  0, 0, sizeof(packet), 0, 1200);
	zassert_equal(ret, 1,
		      "Expected Version Negotiation packet to be ignored (%d)", ret);
}

ZTEST(net_socket_quic, test_336_quic_unsupported_version_before_initial_validation)
{
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	struct net_sockaddr_in src_addr = { 0 };
	uint8_t packet[] = {
		0xc0,
		0x00, 0x00, 0x00, 0x02,
		0x07, 0x83, 0x94, 0xc8, 0xf0, 0x3e, 0x51, 0x57,
		0x08, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		0x00,
	};
	int ret;

	src_addr.sin_family = NET_AF_INET;
	src_addr.sin_port = net_htons(4242);
	net_addr_pton(NET_AF_INET, REMOTE_ADDR_IPV4, &src_addr.sin_addr);

	ep->local_addr.ss_family = NET_AF_INET;

	ret = process_long_header(ep, (struct net_sockaddr *)&src_addr,
				  sizeof(src_addr), packet,
				  0, 0, sizeof(packet), 0, 1200);
	zassert_equal(ret, 1,
		      "Expected unsupported version path before Initial validation (%d)",
		      ret);
}

ZTEST(net_socket_quic, test_340_quic_check_retransmissions)
{
	static uint8_t tx_buf[sizeof(LOREM_IPSUM_LONG)];
	static uint8_t rx_buf[sizeof(LOREM_IPSUM_LONG)];
	int ret;

	ret = loopback_set_packet_drop_ratio(0.1); /* drop every 10th packet */
	zassert_ok(ret, "Failed to set packet drop ratio (%d)", ret);

	memcpy(tx_buf, LOREM_IPSUM_LONG, sizeof(LOREM_IPSUM_LONG));

	quic_server_and_client(LOCAL_ADDR_IPV4_STR, REMOTE_ADDR_IPV4_STR,
			       tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf),
			       MAX_BUF_SIZE);
}

static void server_uni_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct config *data = (struct config *)p1;
	int server_sock = data->sock;
	int connected_sock;
	struct net_sockaddr_storage client_addr;
	net_socklen_t client_addr_len = sizeof(client_addr);
	static uint8_t buf[MAX_BUF_SIZE];
	ssize_t len = 0;
	int stream_recv_sock, stream_send_sock = -1, ret;

	k_sem_give(&data->sem);

	struct zsock_pollfd pfd = {
		.fd = server_sock,
		.events = ZSOCK_POLLIN,
	};

	/* First accept the incoming client connection */
	ret = wait_for_pollin(data, &pfd);
	if (ret == -ECANCELED) {
		return;
	}
	if (ret < 0) {
		data->error = ret;
		LOG_DBG("Poll error while waiting for client connection");
		return;
	}

	connected_sock = zsock_accept(server_sock,
				      (struct net_sockaddr *)&client_addr,
				      &client_addr_len);
	if (connected_sock < 0) {
		data->error = errno;
		LOG_DBG("Connection accept failed (%d)", -data->error);
		goto out;
	}

	data->connected_sock = connected_sock;

	/* Wait that the client creates a stream */
	pfd.fd = connected_sock;
	pfd.events = ZSOCK_POLLIN;
	pfd.revents = 0;

	ret = wait_for_pollin(data, &pfd);
	if (ret == -ECANCELED) {
		goto out;
	}
	if (ret < 0) {
		data->error = ret;
		LOG_DBG("Poll error while waiting for client stream");
		goto out;
	}

	/* Then accept the incoming client stream */
	stream_recv_sock = zsock_accept(connected_sock,
					(struct net_sockaddr *)&client_addr,
					&client_addr_len);
	if (stream_recv_sock < 0) {
		data->error = errno;
		LOG_DBG("Stream accept failed (%d)", -data->error);
		goto out;
	}

	data->stream_recv_sock = stream_recv_sock;

	/* Now open a server unidirectional stream on the same connection
	 * by using the accepted stream socket to derive the endpoint
	 */
	stream_send_sock = quic_stream_open(connected_sock, QUIC_STREAM_SERVER,
					    QUIC_STREAM_UNIDIRECTIONAL, 0);
	if (stream_send_sock < 0) {
		LOG_ERR("Failed to open server stream send sock (%d)",
			stream_send_sock);
		data->error = stream_send_sock;
		goto out;
	}

	while (true) {
		len = zsock_recv(stream_recv_sock, buf, sizeof(buf), 0);
		if (len == 0) {
			/* FIN received — client closed its send side cleanly */
			break;
		}

		if (len < 0) {
			data->error = -errno;
			LOG_DBG("Stream recv failed (%d)", data->error);
			break;
		}

		ret = zsock_send(stream_send_sock, buf, len, 0);
		if (ret < 0) {
			data->error = -errno;
			LOG_DBG("Stream send failed (%d)", data->error);
			break;
		}

		k_msleep(10);

		if (data->test_done) {
			break;
		}
	}

out:
	if (stream_send_sock >= 0) {
		ret = zsock_close(stream_send_sock);
		if (ret < 0) {
			ret = -errno;
			LOG_ERR("Failed to close server stream send sock (%d)", ret);
			if (data->error == 0) {
				data->error = ret;
			}
		}
	}
}

static void quic_server_and_client_uni(const char *server, const char *client,
				       char *tx_buf, size_t tx_buf_len,
				       char *rx_buf, size_t rx_buf_len,
				       size_t batch_size)
{
	/* Implement a test that sets up a QUIC server and client
	 * using the socket API, performs a handshake, and exchanges
	 * some data via unidirectional streams. This would be an
	 * end-to-end test of the QUIC implementation.
	 */
	struct net_sockaddr_storage server_addr;
	struct net_sockaddr_storage client_addr;
	struct net_sockaddr_storage peer_addr;
	sec_tag_t server_sec_tags[] = {
		SERVER_CERTIFICATE_TAG,
	};
	sec_tag_t client_sec_tags[] = {
		CA_CERTIFICATE_TAG,
	};
	static const char * const alpn_list[] = {
		"test-quic",
		NULL
	};

	int server_sock, client_sock, server_connected_sock;
	int client_stream_send_sock, client_stream_recv_sock;
	int server_stream_recv_sock;
	net_socklen_t peer_addr_len = sizeof(peer_addr);
	k_tid_t tid;
	int counter = 5;
	int ret;

	static K_THREAD_STACK_DEFINE(server_thread_stack, STACK_SIZE);
	static struct k_thread server_thread_data;

	static struct config data;

	ret = k_sem_init(&data.sem, 0, 1);
	zassert_ok(ret, "Failed to initialize semaphore (%d)", ret);

	ret = net_ipaddr_parse(server, strlen(server),
			       (struct net_sockaddr *)&server_addr);
	zassert_true(ret, "Failed to parse server IP address %s (%d)",
		     server, ret);

	ret = net_ipaddr_parse(client, strlen(client),
			       (struct net_sockaddr *)&client_addr);
	zassert_true(ret, "Failed to parse client IP address %s (%d)",
		     client, ret);

	prepare_quic_socket(&server_sock,
			    NULL,
			    (const struct net_sockaddr *)&server_addr);
	prepare_quic_socket(&client_sock,
			    (const struct net_sockaddr *)&server_addr,
			    (const struct net_sockaddr *)&client_addr);

	zassert_true(server_sock >= 0, "Failed to create server socket");
	zassert_true(client_sock >= 0, "Failed to create client socket");

	setup_quic_certs(server_sock, server_sec_tags, ARRAY_SIZE(server_sec_tags));
	setup_alpn(server_sock, alpn_list, ARRAY_SIZE(alpn_list));

	data.sock = server_sock;
	data.counter = counter * ((tx_buf_len + batch_size - 1) / batch_size);
	data.error = 0;
	data.stream_recv_sock = -1;
	data.connected_sock = -1;
	data.test_done = false;

	/* Start listening on the server socket in a separate thread */
	tid = k_thread_create(&server_thread_data, server_thread_stack,
			      K_THREAD_STACK_SIZEOF(server_thread_stack),
			      server_uni_thread, &data, NULL, NULL,
			      K_PRIO_PREEMPT(1), 0, K_FOREVER);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		char name[MAX_NAME_LEN];

		snprintk(name, sizeof(name), "quic%d_uni[%d]",
			 server_addr.ss_family == NET_AF_INET6 ? 6 : 4,
			 server_sock);
		k_thread_name_set(&server_thread_data, name);
	}

	k_thread_start(tid);

	ret = k_sem_take(&data.sem, K_FOREVER);
	zassert_ok(ret, "Failed to take semaphore (%d)", ret);

	setup_quic_certs(client_sock, client_sec_tags, ARRAY_SIZE(client_sec_tags));
	setup_alpn(client_sock, alpn_list, ARRAY_SIZE(alpn_list));

	/* Create a stream on the client and send some data to server */
	client_stream_send_sock = quic_stream_open(client_sock, QUIC_STREAM_CLIENT,
						   QUIC_STREAM_UNIDIRECTIONAL, 0);
	zassert_true(client_stream_send_sock >= 0, "Failed to open client stream send sock");

	/* Client recv socket will be set once we accept the server's response stream */
	client_stream_recv_sock = -1;

	for (int i = 0; i < counter; i++) {
		int sent = tx_buf_len;
		int idx = 0;

		do {
			int to_send = MIN((int)batch_size, sent);
			int actually_sent;
			int recvd;

			/* Send data on client's unidirectional stream */
			while (true) {
				ret = zsock_send(client_stream_send_sock, tx_buf + idx, to_send, 0);
				if (ret > 0) {
					break;
				}
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					struct zsock_pollfd pfd = {
						.fd = client_sock,
						.events = ZSOCK_POLLIN,
					};
					int poll_ret;

					poll_ret = zsock_poll(&pfd, 1, POLL_TIMEOUT_MS);
					zassert_true(poll_ret >= 0, "poll failed (%d)", -errno);

					/* Try to accept server's response stream if not yet */
					if (client_stream_recv_sock < 0) {
						client_stream_recv_sock = zsock_accept(client_sock,
							(struct net_sockaddr *)&peer_addr,
							&peer_addr_len);
					}
					continue;
				}

				zassert_true(false, "Failed to send data on client stream %d (%d)",
					     client_stream_send_sock, -errno);
			}

			actually_sent = ret;

			/* Now try to accept server's response stream if not yet done */
			if (client_stream_recv_sock < 0) {
				client_stream_recv_sock = zsock_accept(client_sock,
					(struct net_sockaddr *)&peer_addr,
					&peer_addr_len);
				zassert_true(client_stream_recv_sock >= 0,
					     "Failed to accept client stream recv sock");
			}

			/* Receive echoed data from server's unidirectional stream */
			recvd = 0;
			while (recvd < actually_sent) {
				ret = zsock_recv(client_stream_recv_sock, rx_buf + idx + recvd,
						 actually_sent - recvd, 0);
				if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
					struct zsock_pollfd pfd = {
						.fd = client_stream_recv_sock,
						.events = ZSOCK_POLLIN,
					};

					ret = zsock_poll(&pfd, 1, POLL_TIMEOUT_MS);
					zassert_true(ret >= 0, "poll failed (%d)", -errno);
					continue;
				}
				zassert_true(ret > 0,
					     "Failed to recv data on client stream %d (%d)",
					     client_stream_recv_sock, -errno);
				recvd += ret;
			}

			zassert_mem_equal(rx_buf + idx, tx_buf + idx, actually_sent,
					  "Data mismatch on client stream %d",
					  client_stream_recv_sock);

			idx  += actually_sent;
			sent -= actually_sent;

			k_msleep(10);
		} while (sent > 0);

		zassert_mem_equal(rx_buf, tx_buf, MIN(tx_buf_len, rx_buf_len),
				  "Received data does not match sent data on "
				  "client stream %d",
				  client_stream_recv_sock);
	}

	data.test_done = true;

	memset(rx_buf, 0, rx_buf_len);
	memset(tx_buf, 0, tx_buf_len);

	/* We could also use zsock_close() here to close the stream and
	 * the connection.
	 */
	ret = quic_stream_close(client_stream_recv_sock);
	zassert_equal(ret, 0, "Failed to close client recv stream %d (%d)",
		      client_stream_recv_sock, ret);

	ret = quic_stream_close(client_stream_send_sock);
	zassert_equal(ret, 0, "Failed to close client send stream %d (%d)",
		      client_stream_send_sock, ret);

	ret = k_thread_join(tid, K_MSEC(5000));
	zassert_equal(ret, 0, "Cannot join thread (%d)", ret);

	server_stream_recv_sock = data.stream_recv_sock;
	server_connected_sock = data.connected_sock;

	zassert_equal(data.error, 0, "Server thread reported error (%d)", data.error);

	ret = quic_stream_close(server_stream_recv_sock);
	zassert_equal(ret, 0, "Failed to close server stream %d (%d)",
		      server_stream_recv_sock, ret);

	ret = quic_connection_close(server_connected_sock);
	zassert_equal(ret, 0, "Failed to close server connection (%d)", ret);

	ret = quic_connection_close(client_sock);
	zassert_equal(ret, 0, "Failed to close client connection (%d)", ret);

	ret = quic_connection_close(server_sock);
	zassert_equal(ret, 0, "Failed to close server connection (%d)", ret);
}

static void quic_stream_type_roundtrip_uni(const char *server, const char *client)
{
	struct net_sockaddr_storage server_addr;
	struct net_sockaddr_storage client_addr;
	struct net_sockaddr_storage peer_addr;
	sec_tag_t server_sec_tags[] = {
		SERVER_CERTIFICATE_TAG,
	};
	sec_tag_t client_sec_tags[] = {
		CA_CERTIFICATE_TAG,
	};
	static const char * const alpn_list[] = {
		"test-quic",
		NULL
	};
	static K_THREAD_STACK_DEFINE(server_thread_stack, STACK_SIZE);
	static struct k_thread server_thread_data;
	static struct config data;
	net_socklen_t peer_addr_len = sizeof(peer_addr);
	k_tid_t tid;
	uint8_t tx_byte = 0x5a;
	uint8_t rx_byte = 0;
	int server_sock, client_sock, server_connected_sock;
	int client_stream_send_sock, client_stream_recv_sock = -1;
	int server_stream_recv_sock;
	int ret;

	ret = k_sem_init(&data.sem, 0, 1);
	zassert_ok(ret, "Failed to initialize semaphore (%d)", ret);

	ret = net_ipaddr_parse(server, strlen(server),
			       (struct net_sockaddr *)&server_addr);
	zassert_true(ret, "Failed to parse server IP address %s (%d)",
		     server, ret);

	ret = net_ipaddr_parse(client, strlen(client),
			       (struct net_sockaddr *)&client_addr);
	zassert_true(ret, "Failed to parse client IP address %s (%d)",
		     client, ret);

	prepare_quic_socket(&server_sock,
			    NULL,
			    (const struct net_sockaddr *)&server_addr);
	prepare_quic_socket(&client_sock,
			    (const struct net_sockaddr *)&server_addr,
			    (const struct net_sockaddr *)&client_addr);

	setup_quic_certs(server_sock, server_sec_tags, ARRAY_SIZE(server_sec_tags));
	setup_alpn(server_sock, alpn_list, ARRAY_SIZE(alpn_list));

	data.sock = server_sock;
	data.error = 0;
	data.stream_recv_sock = -1;
	data.connected_sock = -1;
	data.test_done = false;

	tid = k_thread_create(&server_thread_data, server_thread_stack,
			      K_THREAD_STACK_SIZEOF(server_thread_stack),
			      server_uni_thread, &data, NULL, NULL,
			      K_PRIO_PREEMPT(1), 0, K_FOREVER);

	k_thread_start(tid);

	ret = k_sem_take(&data.sem, K_FOREVER);
	zassert_ok(ret, "Failed to take semaphore (%d)", ret);

	setup_quic_certs(client_sock, client_sec_tags, ARRAY_SIZE(client_sec_tags));
	setup_alpn(client_sock, alpn_list, ARRAY_SIZE(alpn_list));

	client_stream_send_sock = quic_stream_open(client_sock, QUIC_STREAM_CLIENT,
						   QUIC_STREAM_UNIDIRECTIONAL, 0);
	zassert_true(client_stream_send_sock >= 0,
		     "Failed to open client send stream (%d)", client_stream_send_sock);

	assert_stream_type_and_id(client_stream_send_sock,
				  QUIC_STREAM_CLIENT | QUIC_STREAM_UNIDIRECTIONAL);
	zassert_true(quic_is_connection_socket(client_sock),
		     "Client connection socket %d must be recognized", client_sock);

	ret = zsock_send(client_stream_send_sock, &tx_byte, sizeof(tx_byte), 0);
	zassert_equal(ret, sizeof(tx_byte), "Failed to send probe byte (%d)", -errno);

	for (int attempt = 0; attempt < 20 && data.stream_recv_sock < 0; attempt++) {
		k_msleep(10);
	}

	server_stream_recv_sock = data.stream_recv_sock;
	zassert_true(server_stream_recv_sock >= 0, "Server did not accept client stream");
	assert_stream_type_and_id(server_stream_recv_sock,
				  QUIC_STREAM_CLIENT | QUIC_STREAM_UNIDIRECTIONAL);

	for (int attempt = 0; attempt < 20 && client_stream_recv_sock < 0; attempt++) {
		struct zsock_pollfd pfd = {
			.fd = client_sock,
			.events = ZSOCK_POLLIN,
		};

		ret = zsock_poll(&pfd, 1, POLL_TIMEOUT_MS);
		zassert_true(ret >= 0, "poll failed (%d)", -errno);

		if (pfd.revents & ZSOCK_POLLIN) {
			client_stream_recv_sock = zsock_accept(client_sock,
							(struct net_sockaddr *)&peer_addr,
							&peer_addr_len);
		}
	}

	zassert_true(client_stream_recv_sock >= 0, "Client did not accept server stream");
	assert_stream_type_and_id(client_stream_recv_sock,
				  QUIC_STREAM_SERVER | QUIC_STREAM_UNIDIRECTIONAL);

	ret = zsock_recv(client_stream_recv_sock, &rx_byte, sizeof(rx_byte), 0);
	zassert_equal(ret, sizeof(rx_byte), "Failed to receive echoed byte (%d)", -errno);
	zassert_equal(rx_byte, tx_byte, "Echoed byte mismatch");

	data.test_done = true;

	ret = quic_stream_close(client_stream_send_sock);
	zassert_equal(ret, 0, "Failed to close client send stream (%d)", ret);

	ret = k_thread_join(tid, K_MSEC(5000));
	zassert_equal(ret, 0, "Cannot join thread (%d)", ret);

	server_connected_sock = data.connected_sock;
	zassert_true(server_connected_sock >= 0, "Invalid server connected socket (%d)",
		     server_connected_sock);
	zassert_equal(data.error, 0, "Server thread reported error (%d)", data.error);

	ret = quic_stream_close(client_stream_recv_sock);
	zassert_equal(ret, 0, "Failed to close client recv stream (%d)", ret);

	ret = quic_stream_close(server_stream_recv_sock);
	zassert_equal(ret, 0, "Failed to close server recv stream (%d)", ret);

	ret = quic_connection_close(server_connected_sock);
	zassert_equal(ret, 0, "Failed to close server connection (%d)", ret);

	ret = quic_connection_close(client_sock);
	zassert_equal(ret, 0, "Failed to close client connection (%d)", ret);

	ret = quic_connection_close(server_sock);
	zassert_equal(ret, 0, "Failed to close server connection (%d)", ret);
}

ZTEST(net_socket_quic, test_350_quic_unidirectional_streams)
{
	static uint8_t tx_buf[sizeof(LOREM_IPSUM_LONG)];
	static uint8_t rx_buf[sizeof(LOREM_IPSUM_LONG)];
	int ret;

	/* Disable packet drop for this test. We focus is on unidirectional stream
	 * functionality, not retransmission handling under packet loss.
	 */
	ret = loopback_set_packet_drop_ratio(0.0);
	zassert_ok(ret, "Failed to set packet drop ratio (%d)", ret);

	memcpy(tx_buf, LOREM_IPSUM_LONG, sizeof(LOREM_IPSUM_LONG));

	quic_server_and_client_uni(LOCAL_ADDR_IPV4_STR, REMOTE_ADDR_IPV4_STR,
				   tx_buf, sizeof(tx_buf), rx_buf, sizeof(rx_buf),
				   MAX_BUF_SIZE);
}

ZTEST(net_socket_quic, test_360_quic_stream_type_bits_end_to_end)
{
	int ret;

	ret = loopback_set_packet_drop_ratio(0.0);
	zassert_ok(ret, "Failed to set packet drop ratio (%d)", ret);

	quic_stream_type_roundtrip_uni(LOCAL_ADDR_IPV4_STR, REMOTE_ADDR_IPV4_STR);
}

ZTEST(net_socket_quic, test_370_client_cert_request_option)
{
	int sock;
	int ret;
	int mode;

	/* Create a server socket */
	ret = quic_connection_open(NULL,
				   (struct net_sockaddr *)&local_addr_ipv4);
	zassert_true(ret >= 0, "Failed to open QUIC connection (%d)", ret);
	sock = ret;

	/* Test setting client cert request to disabled (0) */
	mode = MBEDTLS_SSL_VERIFY_NONE;
	ret = zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY,
			       &mode, sizeof(mode));
	zassert_equal(ret, 0, "Failed to set client cert request disabled (%d)", -errno);

	/* Test setting client cert request to optional (1) */
	mode = MBEDTLS_SSL_VERIFY_OPTIONAL;
	ret = zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY,
			       &mode, sizeof(mode));
	zassert_equal(ret, 0, "Failed to set client cert request optional (%d)", -errno);

	/* Test setting client cert request to required (2) */
	mode = MBEDTLS_SSL_VERIFY_REQUIRED;
	ret = zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY,
			       &mode, sizeof(mode));
	zassert_equal(ret, 0, "Failed to set client cert request required (%d)", -errno);

	/* Test with invalid optlen */
	mode = 1;
	ret = zsock_setsockopt(sock, ZSOCK_SOL_TLS, ZSOCK_TLS_PEER_VERIFY,
			       &mode, sizeof(char));  /* Wrong size */
	zassert_equal(ret, -1, "Should fail with invalid optlen");
	zassert_equal(errno, EINVAL, "Expected EINVAL, got %d", errno);

	ret = quic_connection_close(sock);
	zassert_equal(ret, 0, "Failed to close connection (%d)", ret);
}

ZTEST(net_socket_quic, test_380_cert_chain_add_option)
{
	int sock;
	int ret;
	sec_tag_t tag = CA_CERTIFICATE_TAG;

	/* Create a server socket */
	ret = quic_connection_open(NULL,
				   (struct net_sockaddr *)&local_addr_ipv4);
	zassert_true(ret >= 0, "Failed to open QUIC connection (%d)", ret);
	sock = ret;

	/* Add CA certificate as intermediate cert via sec_tag */
	ret = zsock_setsockopt(sock, ZSOCK_SOL_QUIC, ZSOCK_QUIC_SO_CERT_CHAIN_ADD,
			       &tag, sizeof(tag));
	zassert_equal(ret, 0, "Failed to add intermediate cert (%d)", -errno);

	/* Add same tag again (should succeed, adds to chain) */
	ret = zsock_setsockopt(sock, ZSOCK_SOL_QUIC, ZSOCK_QUIC_SO_CERT_CHAIN_ADD,
			       &tag, sizeof(tag));
	zassert_equal(ret, 0, "Failed to add second intermediate cert (%d)", -errno);

	/* Test with NULL (should fail) */
	ret = zsock_setsockopt(sock, ZSOCK_SOL_QUIC, ZSOCK_QUIC_SO_CERT_CHAIN_ADD,
			       NULL, 0);
	zassert_equal(ret, -1, "Should fail with NULL");

	/* Test with wrong optlen (should fail) */
	ret = zsock_setsockopt(sock, ZSOCK_SOL_QUIC, ZSOCK_QUIC_SO_CERT_CHAIN_ADD,
			       &tag, 1);
	zassert_equal(ret, -1, "Should fail with wrong optlen");

	ret = quic_connection_close(sock);
	zassert_equal(ret, 0, "Failed to close connection (%d)", ret);
}

ZTEST(net_socket_quic, test_390_cert_chain_del_option)
{
	int sock;
	int ret;
	sec_tag_t tag = CA_CERTIFICATE_TAG;

	/* Create a server socket */
	ret = quic_connection_open(NULL,
				   (struct net_sockaddr *)&local_addr_ipv4);
	zassert_true(ret >= 0, "Failed to open QUIC connection (%d)", ret);
	sock = ret;

	/* Add CA certificate as intermediate cert via sec_tag */
	ret = zsock_setsockopt(sock, ZSOCK_SOL_QUIC, ZSOCK_QUIC_SO_CERT_CHAIN_ADD,
			       &tag, sizeof(tag));
	zassert_equal(ret, 0, "Failed to add intermediate cert (%d)", -errno);

	/* Delete the tag (should succeed, deletes the chain) */
	ret = zsock_setsockopt(sock, ZSOCK_SOL_QUIC, ZSOCK_QUIC_SO_CERT_CHAIN_DEL,
			       &tag, sizeof(tag));
	zassert_equal(ret, 0, "Failed to delete intermediate cert (%d)", -errno);

	/* Delete same tag again (should fail) */
	ret = zsock_setsockopt(sock, ZSOCK_SOL_QUIC, ZSOCK_QUIC_SO_CERT_CHAIN_DEL,
			       &tag, sizeof(tag));
	zassert_true(ret == -1 && errno == ENOENT,
		     "Succeed to remove intermediate cert (%d)", -errno);

	/* Add again */
	ret = zsock_setsockopt(sock, ZSOCK_SOL_QUIC, ZSOCK_QUIC_SO_CERT_CHAIN_ADD,
			       &tag, sizeof(tag));
	zassert_equal(ret, 0, "Failed to add intermediate cert (%d)", -errno);

	/* Test with NULL (should remove all) */
	ret = zsock_setsockopt(sock, ZSOCK_SOL_QUIC, ZSOCK_QUIC_SO_CERT_CHAIN_DEL,
			       NULL, 0);
	zassert_equal(ret, 0, "Failed with NULL");

	/* Delete same tag again (should fail) */
	ret = zsock_setsockopt(sock, ZSOCK_SOL_QUIC, ZSOCK_QUIC_SO_CERT_CHAIN_DEL,
			       &tag, sizeof(tag));
	zassert_true(ret == -1 && errno == ENOENT,
		     "Succeed to remove intermediate cert (%d)", -errno);

	/* Test with wrong optlen (should fail) */
	ret = zsock_setsockopt(sock, ZSOCK_SOL_QUIC, ZSOCK_QUIC_SO_CERT_CHAIN_DEL,
			       NULL, sizeof(tag));
	zassert_true(ret == -1 && errno == EINVAL, "Should fail with wrong optlen");

	/* If length is 0, then tag is ignored and all certs are removed, so this should succeed */
	ret = zsock_setsockopt(sock, ZSOCK_SOL_QUIC, ZSOCK_QUIC_SO_CERT_CHAIN_DEL,
			       &tag, 0);
	zassert_equal(ret, 0, "Failed with 0 optlen");

	ret = quic_connection_close(sock);
	zassert_equal(ret, 0, "Failed to close connection (%d)", ret);
}

ZTEST(net_socket_quic, test_400_invalid_quic_sockopt)
{
	int sock;
	int ret;
	int val = 0;

	/* Create a server socket */
	ret = quic_connection_open(NULL,
				   (struct net_sockaddr *)&local_addr_ipv4);
	zassert_true(ret >= 0, "Failed to open QUIC connection (%d)", ret);
	sock = ret;

	/* Test with invalid option number */
	ret = zsock_setsockopt(sock, ZSOCK_SOL_QUIC, 999,
			       &val, sizeof(val));
	zassert_equal(ret, -1, "Should fail with invalid option");
	zassert_equal(errno, ENOPROTOOPT, "Expected ENOPROTOOPT, got %d", errno);

	ret = quic_connection_close(sock);
	zassert_equal(ret, 0, "Failed to close connection (%d)", ret);
}

/*
 * Server requests client certificate, client provides it, handshake succeeds,
 * and data is exchanged successfully.
 */
#define LOCAL_ADDR_IPV4_STR3 "127.0.0.1:54323"
#define REMOTE_ADDR_IPV4_STR3 "127.0.0.1:19993"

static void quic_server_and_client_with_client_cert(const char *server,
						    const char *client,
						    int client_cert_mode,
						    bool setup_client_cert)
{
	struct net_sockaddr_storage server_addr;
	struct net_sockaddr_storage client_addr;
	/* Server needs its own cert + CA to verify client */
	sec_tag_t server_sec_tags[] = {
		SERVER_CERTIFICATE_TAG,
		CA_CERTIFICATE_TAG,
	};
	/* Client needs CA to verify server + its own cert for authentication */
	sec_tag_t client_sec_tags[] = {
		CLIENT_CERTIFICATE_TAG,
		CA_CERTIFICATE_TAG,
	};
	static const char * const alpn_list[] = {
		"test-quic-cert",
		NULL
	};

	int server_sock, client_sock, client_stream_sock, server_stream_sock;
	int server_connected_sock = -1;
	struct zsock_pollfd pfd;
	k_tid_t tid;
	int ret;
	static uint8_t tx_buf[] = "Hello with client cert!";
	static uint8_t rx_buf[64];

#define CERT_STACK_SIZE 2048
	static K_THREAD_STACK_DEFINE(cert_server_thread_stack, CERT_STACK_SIZE);
	static struct k_thread cert_server_thread_data;

	static struct config cert_data;

	ret = k_sem_init(&cert_data.sem, 0, 1);
	zassert_ok(ret, "Failed to initialize semaphore (%d)", ret);

	ret = net_ipaddr_parse(server, strlen(server),
			       (struct net_sockaddr *)&server_addr);
	zassert_true(ret, "Failed to parse server IP address %s", server);

	ret = net_ipaddr_parse(client, strlen(client),
			       (struct net_sockaddr *)&client_addr);
	zassert_true(ret, "Failed to parse client IP address %s", client);

	prepare_quic_socket(&server_sock, NULL,
			    (const struct net_sockaddr *)&server_addr);
	prepare_quic_socket(&client_sock,
			    (const struct net_sockaddr *)&server_addr,
			    (const struct net_sockaddr *)&client_addr);

	zassert_true(server_sock >= 0, "Failed to create server socket");
	zassert_true(client_sock >= 0, "Failed to create client socket");

	/* Set up server to request client certificate */
	ret = zsock_setsockopt(server_sock, ZSOCK_SOL_TLS,
			       ZSOCK_TLS_PEER_VERIFY,
			       &client_cert_mode, sizeof(client_cert_mode));
	zassert_equal(ret, 0, "Failed to set client cert request mode (%d)", -errno);

	/* Set up TLS credentials */
	ret = zsock_setsockopt(server_sock, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST,
			       server_sec_tags, sizeof(server_sec_tags));
	zassert_equal(ret, 0, "Failed to set server sec tags (%d)", -errno);

	setup_alpn(server_sock, alpn_list, ARRAY_SIZE(alpn_list));

	cert_data.sock = server_sock;
	cert_data.counter = 1;
	cert_data.error = 0;
	cert_data.connected_sock = -1;
	cert_data.stream_recv_sock = -1;
	cert_data.test_done = false;

	/* Start server thread */
	tid = k_thread_create(&cert_server_thread_data, cert_server_thread_stack,
			      K_THREAD_STACK_SIZEOF(cert_server_thread_stack),
			      server_thread, &cert_data, NULL, NULL,
			      K_PRIO_PREEMPT(1), 0, K_FOREVER);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		k_thread_name_set(&cert_server_thread_data, "quic_cert_srv");
	}

	k_thread_start(tid);

	ret = k_sem_take(&cert_data.sem, K_FOREVER);
	zassert_ok(ret, "Failed to take semaphore (%d)", ret);

	if (setup_client_cert) {
		/* Set up client with its certificate */
		ret = zsock_setsockopt(client_sock, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST,
				       client_sec_tags, sizeof(client_sec_tags));
		zassert_equal(ret, 0, "Failed to set client sec tags (%d)", -errno);
	}

	setup_alpn(client_sock, alpn_list, ARRAY_SIZE(alpn_list));

	/* Open stream which triggers handshake */
	client_stream_sock = quic_stream_open(client_sock, QUIC_STREAM_CLIENT,
					      QUIC_STREAM_BIDIRECTIONAL, 0);
	if (client_cert_mode == MBEDTLS_SSL_VERIFY_REQUIRED && !setup_client_cert &&
	    client_stream_sock < 0) {
		zassert_equal(client_stream_sock, -ECONNREFUSED,
			      "Unexpected stream open failure without client cert (%d)",
			      client_stream_sock);
	} else {
		zassert_true(client_stream_sock >= 0,
			     "Failed to open client stream with client cert (%d)",
			     client_stream_sock);
	}

	if (client_stream_sock >= 0) {
		/* Send data */
		ret = zsock_send(client_stream_sock, tx_buf, sizeof(tx_buf), 0);
		zassert_equal(ret, sizeof(tx_buf), "Failed to send data (%d)", -errno);

		pfd.fd = client_stream_sock;
		pfd.events = ZSOCK_POLLIN | ZSOCK_POLLHUP;
		pfd.revents = 0;

		ret = zsock_poll(&pfd, 1,
				 (client_cert_mode == MBEDTLS_SSL_VERIFY_REQUIRED &&
				  !setup_client_cert) ? 5000 : SYS_FOREVER_MS);
		zassert_true(ret >= 0, "Poll failed (%d)", -errno);

		if (client_cert_mode == MBEDTLS_SSL_VERIFY_REQUIRED && !setup_client_cert) {
			/* If client cert is required but not set up, handshake should fail and
			 * recv should return error
			 */
			zassert_true(pfd.revents & ZSOCK_POLLHUP,
				     "Expected poll to indicate connection closed (events 0x%02x)",
				     pfd.revents);
		} else {
			/* Receive echo */
			ret = zsock_recv(client_stream_sock, rx_buf, sizeof(rx_buf), 0);

			zassert_true(ret > 0, "Failed to receive data (%d)", -errno);
			zassert_mem_equal(tx_buf, rx_buf, sizeof(tx_buf),
					  "Received data mismatch");
		}
	}

	/* Clean up */
	cert_data.test_done = true;

	if (client_stream_sock >= 0) {
		ret = zsock_close(client_stream_sock);
		zassert_equal(ret, 0, "Failed to close client stream (%d)", ret);
	}

	ret = k_thread_join(&cert_server_thread_data, K_MSEC(500));
	zassert_equal(ret, 0, "Cannot join thread (%d)", ret);

	if (setup_client_cert) {
		server_stream_sock = cert_data.stream_recv_sock;
		zassert_equal(cert_data.error, 0, "Server thread reported error (%d)",
			      cert_data.error);

		ret = quic_stream_close(server_stream_sock);
		zassert_equal(ret, 0, "Failed to close server stream %d (%d)",
		      server_stream_sock, ret);
	}

	if (client_stream_sock >= 0 &&
	    !(client_cert_mode == MBEDTLS_SSL_VERIFY_REQUIRED && !setup_client_cert)) {
		server_connected_sock = cert_data.connected_sock;
		zassert_true(server_connected_sock >= 0, "Invalid connected socket (%d)",
			     server_connected_sock);

		ret = quic_connection_close(server_connected_sock);
		zassert_equal(ret, 0, "Failed to close server connection %d (%d)",
			      server_connected_sock, ret);
	} else if (cert_data.connected_sock >= 0) {
		ret = quic_connection_close(cert_data.connected_sock);
		zassert_equal(ret, 0, "Failed to close rejected server connection %d (%d)",
			      cert_data.connected_sock, ret);
	}

	ret = quic_connection_close(client_sock);
	zassert_equal(ret, 0, "Failed to close client connection (%d)", ret);

	ret = quic_connection_close(server_sock);
	zassert_equal(ret, 0, "Failed to close server connection (%d)", ret);
}

static void quic_server_and_client_with_server_cert(const char *server,
						    const char *client,
						    int client_verify_mode,
						    bool setup_client_ca,
						    bool expect_success)
{
	struct net_sockaddr_storage server_addr;
	struct net_sockaddr_storage client_addr;
	sec_tag_t server_sec_tags[] = {
		SERVER_CERTIFICATE_TAG,
	};
	sec_tag_t client_sec_tags[] = {
		CA_CERTIFICATE_TAG,
	};
	static const char * const alpn_list[] = {
		"test-quic-server-auth",
		NULL
	};
	int server_sock, client_sock, client_stream_sock = -1, server_stream_sock;
	int server_connected_sock = -1;
	struct zsock_pollfd pfd;
	k_tid_t tid;
	int ret;
	static uint8_t tx_buf[] = "Hello with server cert!";
	static uint8_t rx_buf[64];

#define SERVER_AUTH_STACK_SIZE 2048
	static K_THREAD_STACK_DEFINE(server_auth_thread_stack, SERVER_AUTH_STACK_SIZE);
	static struct k_thread server_auth_thread_data;
	static struct config server_auth_data;

	ret = k_sem_init(&server_auth_data.sem, 0, 1);
	zassert_ok(ret, "Failed to initialize semaphore (%d)", ret);

	ret = net_ipaddr_parse(server, strlen(server),
			       (struct net_sockaddr *)&server_addr);
	zassert_true(ret, "Failed to parse server IP address %s", server);

	ret = net_ipaddr_parse(client, strlen(client),
			       (struct net_sockaddr *)&client_addr);
	zassert_true(ret, "Failed to parse client IP address %s", client);

	prepare_quic_socket(&server_sock, NULL,
			    (const struct net_sockaddr *)&server_addr);
	prepare_quic_socket(&client_sock,
			    (const struct net_sockaddr *)&server_addr,
			    (const struct net_sockaddr *)&client_addr);

	zassert_true(server_sock >= 0, "Failed to create server socket");
	zassert_true(client_sock >= 0, "Failed to create client socket");

	ret = zsock_setsockopt(server_sock, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST,
			       server_sec_tags, sizeof(server_sec_tags));
	zassert_equal(ret, 0, "Failed to set server sec tags (%d)", -errno);

	setup_alpn(server_sock, alpn_list, ARRAY_SIZE(alpn_list));

	server_auth_data.sock = server_sock;
	server_auth_data.counter = 1;
	server_auth_data.error = 0;
	server_auth_data.connected_sock = -1;
	server_auth_data.stream_recv_sock = -1;
	server_auth_data.test_done = false;

	tid = k_thread_create(&server_auth_thread_data, server_auth_thread_stack,
			      K_THREAD_STACK_SIZEOF(server_auth_thread_stack),
			      server_thread, &server_auth_data, NULL, NULL,
			      K_PRIO_PREEMPT(1), 0, K_FOREVER);

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		k_thread_name_set(&server_auth_thread_data, "quic_srv_auth");
	}

	k_thread_start(tid);

	ret = k_sem_take(&server_auth_data.sem, K_FOREVER);
	zassert_ok(ret, "Failed to take semaphore (%d)", ret);

	if (setup_client_ca) {
		ret = zsock_setsockopt(client_sock, ZSOCK_SOL_TLS, ZSOCK_TLS_SEC_TAG_LIST,
				       client_sec_tags, sizeof(client_sec_tags));
		zassert_equal(ret, 0, "Failed to set client sec tags (%d)", -errno);
	}

	if (client_verify_mode >= 0) {
		ret = zsock_setsockopt(client_sock, ZSOCK_SOL_TLS,
				       ZSOCK_TLS_PEER_VERIFY,
				       &client_verify_mode, sizeof(client_verify_mode));
		zassert_equal(ret, 0, "Failed to set client verify mode (%d)", -errno);
	}

	setup_alpn(client_sock, alpn_list, ARRAY_SIZE(alpn_list));

	client_stream_sock = quic_stream_open(client_sock, QUIC_STREAM_CLIENT,
					      QUIC_STREAM_BIDIRECTIONAL, 0);

	if (!expect_success && client_stream_sock < 0) {
		zassert_equal(client_stream_sock, -ECONNREFUSED,
			      "Unexpected stream open failure without CA (%d)",
			      client_stream_sock);
		goto cleanup;
	}

	zassert_true(client_stream_sock >= 0, "Failed to open client stream (%d)",
		     client_stream_sock);

	ret = zsock_send(client_stream_sock, tx_buf, sizeof(tx_buf), 0);
	zassert_equal(ret, sizeof(tx_buf), "Failed to send data (%d)", -errno);

	pfd.fd = client_stream_sock;
	pfd.events = ZSOCK_POLLIN | ZSOCK_POLLHUP;
	pfd.revents = 0;

	ret = zsock_poll(&pfd, 1, expect_success ? SYS_FOREVER_MS : 5000);
	zassert_true(ret >= 0, "Poll failed (%d)", -errno);

	if (!expect_success) {
		zassert_true(pfd.revents & ZSOCK_POLLHUP,
			     "Expected poll to indicate connection closed (events 0x%02x)",
			     pfd.revents);
		goto cleanup;
	}

	ret = zsock_recv(client_stream_sock, rx_buf, sizeof(rx_buf), 0);
	zassert_true(ret > 0, "Failed to receive data (%d)", -errno);
	zassert_mem_equal(tx_buf, rx_buf, sizeof(tx_buf), "Received data mismatch");

cleanup:
	server_auth_data.test_done = true;

	if (client_stream_sock >= 0) {
		ret = zsock_close(client_stream_sock);
		zassert_equal(ret, 0, "Failed to close client stream (%d)", ret);
	}

	ret = k_thread_join(&server_auth_thread_data, K_MSEC(500));
	zassert_equal(ret, 0, "Cannot join thread (%d)", ret);

	if (expect_success) {
		server_stream_sock = server_auth_data.stream_recv_sock;
		zassert_equal(server_auth_data.error, 0, "Server thread reported error (%d)",
			      server_auth_data.error);

		ret = quic_stream_close(server_stream_sock);
		zassert_equal(ret, 0, "Failed to close server stream %d (%d)",
			      server_stream_sock, ret);

		server_connected_sock = server_auth_data.connected_sock;
		zassert_true(server_connected_sock >= 0, "Invalid connected socket (%d)",
			     server_connected_sock);

		ret = quic_connection_close(server_connected_sock);
		zassert_equal(ret, 0, "Failed to close server connection %d (%d)",
			      server_connected_sock, ret);
	} else if (server_auth_data.connected_sock >= 0) {
		ret = quic_connection_close(server_auth_data.connected_sock);
		zassert_equal(ret, 0, "Failed to close rejected server connection %d (%d)",
			      server_auth_data.connected_sock, ret);
	}

	ret = quic_connection_close(client_sock);
	zassert_equal(ret, 0, "Failed to close client connection (%d)", ret);

	ret = quic_connection_close(server_sock);
	zassert_equal(ret, 0, "Failed to close server connection (%d)", ret);
}

ZTEST(net_socket_quic, test_410_client_cert_optional)
{
	int ret;

	/* Disable packet drop for this test */
	ret = loopback_set_packet_drop_ratio(0.0);
	zassert_ok(ret, "Failed to set packet drop ratio (%d)", ret);

	/* Test with optional client cert (mode=1) */
	quic_server_and_client_with_client_cert(LOCAL_ADDR_IPV4_STR3,
						REMOTE_ADDR_IPV4_STR3,
						MBEDTLS_SSL_VERIFY_OPTIONAL,
						true /* Set up client cert */);
}

ZTEST(net_socket_quic, test_420_client_cert_mandatory)
{
	int ret;

	/* Disable packet drop for this test */
	ret = loopback_set_packet_drop_ratio(0.0);
	zassert_ok(ret, "Failed to set packet drop ratio (%d)", ret);

	/* Test with optional client cert (mode=1) */
	quic_server_and_client_with_client_cert(LOCAL_ADDR_IPV4_STR3,
						REMOTE_ADDR_IPV4_STR3,
						MBEDTLS_SSL_VERIFY_REQUIRED,
						true /* Set up client cert */);
}

ZTEST(net_socket_quic, test_430_client_cert_mandatory_and_failing)
{
	int ret;

	/* Disable packet drop for this test */
	ret = loopback_set_packet_drop_ratio(0.0);
	zassert_ok(ret, "Failed to set packet drop ratio (%d)", ret);

	/* Test with optional client cert (mode=1) */
	quic_server_and_client_with_client_cert(LOCAL_ADDR_IPV4_STR3,
						REMOTE_ADDR_IPV4_STR3,
						MBEDTLS_SSL_VERIFY_REQUIRED,
						false /* Don't set up client cert */);
}

ZTEST(net_socket_quic, test_440_client_requires_server_ca_by_default)
{
	int ret;

	ret = loopback_set_packet_drop_ratio(0.0);
	zassert_ok(ret, "Failed to set packet drop ratio (%d)", ret);

	quic_server_and_client_with_server_cert(LOCAL_ADDR_IPV4_STR3,
						REMOTE_ADDR_IPV4_STR3,
						-1, false, false);
}

ZTEST(net_socket_quic, test_450_client_can_disable_server_verification)
{
	int ret;

	ret = loopback_set_packet_drop_ratio(0.0);
	zassert_ok(ret, "Failed to set packet drop ratio (%d)", ret);

	quic_server_and_client_with_server_cert(LOCAL_ADDR_IPV4_STR3,
						REMOTE_ADDR_IPV4_STR3,
						MBEDTLS_SSL_VERIFY_NONE,
						false, true);
}

ZTEST(net_socket_quic, test_455_required_peer_verification_rejects_empty_certificate)
{
	static const uint8_t empty_certificate[] = { 0, 0, 0, 0 };
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	struct quic_tls_context ctx = { 0 };
	int ret;

	ep->is_server = false;
	ctx.ep = ep;
	ctx.options.verify_level = -1;

	ret = parse_certificate(&ctx, empty_certificate, sizeof(empty_certificate));
	zassert_equal(ret, -EACCES,
		      "Expected empty required peer certificate to fail (%d)", ret);
	zassert_equal(ctx.peer_cert_len, 0,
		      "Empty peer certificate must not populate peer_cert_len");
}

ZTEST(net_socket_quic, test_460_required_peer_verification_rejects_finished_without_certificate)
{
	struct quic_endpoint *ep = reset_test_ep(&test_ep_a);
	struct quic_tls_context ctx = { 0 };
	static const uint8_t finished_body[] = { 0 };
	static const uint8_t finished_msg[] = { 0 };
	int ret;

	ep->is_server = false;
	ctx.ep = ep;
	ctx.options.verify_level = -1;
	ctx.peer_cert_len = 0;

	ret = process_handshake_message(&ctx, TLS_HS_FINISHED,
					finished_body, sizeof(finished_body),
					finished_msg, sizeof(finished_msg));
	zassert_equal(ret, -EACCES,
		      "Expected Finished without required peer certificate to fail (%d)",
		      ret);
	zassert_not_equal(ctx.state, QUIC_TLS_STATE_CONNECTED,
			  "Handshake must not reach CONNECTED without peer certificate");
}

#define LOCAL_ADDR_IPV4_STR4 "127.0.0.1:54324"
#define REMOTE_ADDR_IPV4_STR4 "127.0.0.1:19996"

/* Test 470: Connection statistics are updated for successful traffic */
ZTEST(net_socket_quic, test_470_connection_statistics_track_traffic)
{
	static uint8_t tx_buf[SMALL_BUF_SIZE];
	static uint8_t rx_buf[SMALL_BUF_SIZE];
	struct net_stats_quic client_stats;
	struct net_stats_quic server_stats;
	struct net_stats_quic_global stats_before;
	struct net_stats_quic_global stats_after;
#if defined(CONFIG_QUIC_STATS_HISTORY)
	struct quic_closed_stats_snapshot closed_before;
	struct quic_closed_stats_snapshot closed_after;
	const struct quic_closed_context_stats *closed_client;
	const struct quic_closed_context_stats *closed_server;
	int closed_baseline_id;
#endif
	int ret;

	memset(tx_buf, 0x5a, sizeof(tx_buf));
	memset(rx_buf, 0, sizeof(rx_buf));
	memset(&client_stats, 0, sizeof(client_stats));
	memset(&server_stats, 0, sizeof(server_stats));
	copy_quic_global_stats(&stats_before);
#if defined(CONFIG_QUIC_STATS_HISTORY)
	copy_quic_closed_context_stats(&closed_before);
	closed_baseline_id = max_quic_closed_context_id(&closed_before);
#endif

	ret = loopback_set_packet_drop_ratio(0.0);
	zassert_ok(ret, "Failed to set packet drop ratio (%d)", ret);

	quic_server_and_client_with_stats(LOCAL_ADDR_IPV4_STR4, REMOTE_ADDR_IPV4_STR4,
					  tx_buf, sizeof(tx_buf),
					  rx_buf, sizeof(rx_buf),
					  sizeof(tx_buf),
					  &client_stats, &server_stats);
	copy_quic_global_stats(&stats_after);
#if defined(CONFIG_QUIC_STATS_HISTORY)
	copy_quic_closed_context_stats(&closed_after);
#endif

	zassert_true(client_stats.handshake_init_tx > 0,
		     "Client handshake_init_tx not updated");
	zassert_true(client_stats.handshake_resp_rx > 0,
		     "Client handshake_resp_rx not updated");
	zassert_true(client_stats.valid_tx > 0,
		     "Client valid_tx not updated");
	zassert_true(client_stats.valid_rx > 0,
		     "Client valid_rx not updated");
	assert_quic_stats_no_errors(&client_stats, "client");

	zassert_true(server_stats.handshake_init_rx > 0,
		     "Server handshake_init_rx not updated");
	zassert_true(server_stats.handshake_resp_tx > 0,
		     "Server handshake_resp_tx not updated");
	zassert_true(server_stats.valid_tx > 0,
		     "Server valid_tx not updated");
	zassert_true(server_stats.valid_rx > 0,
		     "Server valid_rx not updated");
	assert_quic_stats_no_errors(&server_stats, "server");
	zassert_equal(stats_after.connections_opened - stats_before.connections_opened, 3U,
		      "Expected listener, client, and accepted server connection opens");
#if defined(CONFIG_QUIC_STATS_HISTORY)
	zassert_equal(count_new_quic_closed_contexts(&closed_after, closed_baseline_id), 2U,
		      "Expected only client and accepted server contexts in closed stats history");

	closed_client = find_new_quic_closed_context_stats(&closed_after, closed_baseline_id, false,
							       19996U, 54324U);
	zassert_not_null(closed_client, "Missing closed client stats entry");
	zassert_true(closed_client->duration_ms > 0U, "Closed client lifetime was not tracked");
	zassert_true(closed_client->stats.valid_tx > 0U, "Closed client valid_tx missing");
	zassert_true(closed_client->stats.valid_rx > 0U, "Closed client valid_rx missing");
	assert_quic_stats_no_errors(&closed_client->stats, "closed client");

	closed_server = find_new_quic_closed_context_stats(&closed_after, closed_baseline_id, true,
							       54324U, 19996U);
	zassert_not_null(closed_server, "Missing closed server stats entry");
	zassert_true(closed_server->duration_ms > 0U, "Closed server lifetime was not tracked");
	zassert_true(closed_server->stats.valid_tx > 0U, "Closed server valid_tx missing");
	zassert_true(closed_server->stats.valid_rx > 0U, "Closed server valid_rx missing");
	assert_quic_stats_no_errors(&closed_server->stats, "closed server");
#endif
}

ZTEST_SUITE(net_socket_quic, NULL, setup, before, after, NULL);
