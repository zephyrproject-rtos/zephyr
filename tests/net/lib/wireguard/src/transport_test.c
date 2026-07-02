/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/wireguard.h>
#include <zephyr/net/socket.h>

#include "wg_test_helpers.h"

static int test_peer_id;

static struct wireguard_test_session test_session = {
	.recv_key = {
		0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
		0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
		0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
		0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20,
	},
	.send_key = {
		0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
		0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30,
		0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
		0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x40,
	},
	.local_index = 0x12345678,
	.remote_index = 0x87654321,
};

static struct net_sockaddr_storage make_addr(uint32_t addr_be32, uint16_t port)
{
	struct net_sockaddr_storage storage = { 0 };
	struct net_sockaddr *addr = (struct net_sockaddr *)&storage;

	addr->sa_family = NET_AF_INET;
	net_sin(addr)->sin_addr.s_addr = addr_be32;
	net_sin(addr)->sin_port = net_htons(port);

	return storage;
}

static void *transport_setup(void)
{
	int ret;

	test_peer_id = wireguard_test_peer_add(NULL);
	zassert_true(test_peer_id > 0, "peer add failed (%d)", test_peer_id);

	ret = wireguard_test_install_session(test_peer_id, &test_session);
	zassert_equal(ret, 0, "session install failed (%d)", ret);

	return NULL;
}

static void transport_before(void *fixture)
{
	int ret;

	ARG_UNUSED(fixture);

	ret = wireguard_test_install_session(test_peer_id, &test_session);
	zassert_equal(ret, 0, "session reinstall failed (%d)", ret);
}

ZTEST(wireguard_transport, test_replay_does_not_mutate_peer_state)
{
	struct net_stats_vpn stats_before;
	struct net_stats_vpn stats_after;
	struct wireguard_test_keypair_state kp_before;
	struct wireguard_test_keypair_state kp_after;
	struct net_sockaddr_storage endpoint_before;
	struct net_sockaddr_storage endpoint_after;
	uint8_t ciphertext[32];
	size_t ciphertext_len;
	uint32_t last_rx_before;
	uint32_t last_rx_after;
	struct net_sockaddr_storage src_a;
	struct net_sockaddr_storage src_b;
	int ret;

	src_a = make_addr(net_htonl(0xC0000201), 51820); /* 192.0.2.1 */
	src_b = make_addr(net_htonl(0xC0000263), 51820); /* 192.0.2.99 */

	ret = wireguard_test_encrypt(test_session.recv_key, 0, NULL, 0,
				     ciphertext, sizeof(ciphertext),
				     &ciphertext_len);
	zassert_equal(ret, 0);
	zassert_equal(ciphertext_len, 16U);

	ret = wireguard_test_inject_transport(test_peer_id,
					      (struct net_sockaddr *)&src_a,
					      0, ciphertext, ciphertext_len);
	zassert_equal(ret, 0, "first inject failed (%d)", ret);

	last_rx_before = wireguard_test_peer_last_rx(test_peer_id);
	ret = wireguard_test_peer_endpoint(test_peer_id, &endpoint_before);
	zassert_equal(ret, 0);
	ret = wireguard_test_get_stats(test_peer_id, &stats_before);
	zassert_equal(ret, 0);
	ret = wireguard_test_peer_keypair_state(test_peer_id, &kp_before);
	zassert_equal(ret, 0);

	ret = wireguard_test_inject_transport(test_peer_id,
					      (struct net_sockaddr *)&src_b,
					      0, ciphertext, ciphertext_len);
	zassert_true(ret < 0, "replay inject should fail (%d)", ret);

	last_rx_after = wireguard_test_peer_last_rx(test_peer_id);
	ret = wireguard_test_peer_endpoint(test_peer_id, &endpoint_after);
	zassert_equal(ret, 0);
	ret = wireguard_test_get_stats(test_peer_id, &stats_after);
	zassert_equal(ret, 0);
	ret = wireguard_test_peer_keypair_state(test_peer_id, &kp_after);
	zassert_equal(ret, 0);

	zassert_equal(last_rx_after, last_rx_before);
	zassert_mem_equal(&endpoint_before, &endpoint_after, sizeof(endpoint_before));
	zassert_equal(stats_after.replay_error, stats_before.replay_error + 1);
	zassert_equal(kp_after.prev_valid, kp_before.prev_valid);
	zassert_equal(kp_after.current_valid, kp_before.current_valid);
	zassert_equal(kp_after.next_valid, kp_before.next_valid);
}

ZTEST(wireguard_transport, test_forged_keepalive_rejected)
{
	struct net_stats_vpn stats_before;
	struct net_stats_vpn stats_after;
	uint8_t garbage[16];
	struct net_sockaddr_storage src;
	int ret;

	src = make_addr(net_htonl(0xC0000201), 51820);
	memset(garbage, 0x5a, sizeof(garbage));

	ret = wireguard_test_get_stats(test_peer_id, &stats_before);
	zassert_equal(ret, 0);
	zassert_false(wireguard_test_peer_first_valid(test_peer_id));

	ret = wireguard_test_inject_transport(test_peer_id,
					      (struct net_sockaddr *)&src,
					      0, garbage, sizeof(garbage));
	zassert_true(ret < 0, "forged keepalive should fail (%d)", ret);

	ret = wireguard_test_get_stats(test_peer_id, &stats_after);
	zassert_equal(ret, 0);
	zassert_false(wireguard_test_peer_first_valid(test_peer_id));
	zassert_equal(stats_after.decrypt_failed, stats_before.decrypt_failed + 1);
	zassert_equal(stats_after.keepalive_rx, stats_before.keepalive_rx);
}

ZTEST(wireguard_transport, test_valid_keepalive_accepted)
{
	struct net_stats_vpn stats_before;
	struct net_stats_vpn stats_after;
	uint8_t ciphertext[32];
	size_t ciphertext_len;
	struct net_sockaddr_storage src;
	int ret;

	src = make_addr(net_htonl(0xC0000201), 51820);

	ret = wireguard_test_encrypt(test_session.recv_key, 1, NULL, 0,
				     ciphertext, sizeof(ciphertext),
				     &ciphertext_len);
	zassert_equal(ret, 0);
	zassert_equal(ciphertext_len, 16U);

	ret = wireguard_test_get_stats(test_peer_id, &stats_before);
	zassert_equal(ret, 0);

	ret = wireguard_test_inject_transport(test_peer_id,
					      (struct net_sockaddr *)&src,
					      1, ciphertext, ciphertext_len);
	zassert_equal(ret, 0, "valid keepalive failed (%d)", ret);

	ret = wireguard_test_get_stats(test_peer_id, &stats_after);
	zassert_equal(ret, 0);
	zassert_true(wireguard_test_peer_first_valid(test_peer_id));
	zassert_equal(stats_after.keepalive_rx, stats_before.keepalive_rx + 1);
	zassert_equal(stats_after.valid_rx, stats_before.valid_rx);
}

ZTEST_SUITE(wireguard_transport, NULL, transport_setup, transport_before, NULL, NULL);
