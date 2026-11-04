/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/virtual_mgmt.h>
#include <zephyr/net/wireguard.h>
#include <zephyr/sys/base64.h>

#include "wg_test_helpers.h"

static void find_vpn_iface(struct net_if *iface, void *user_data)
{
	struct net_if **found = user_data;

	if (*found == NULL && net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL) &&
	    net_virtual_get_iface_capabilities(iface) == VIRTUAL_INTERFACE_VPN) {
		*found = iface;
	}
}

static void set_private_key(void)
{
	static const char *private_key = "lmAIbJR8PQOpgJxmfOydBiDbexTMEKsjglZ1sj3kIjs=";
	struct virtual_interface_req_params params = { 0 };
	struct net_if *vpn_iface = NULL;
	uint8_t key[32];
	size_t olen;
	int ret;

	net_if_foreach(find_vpn_iface, &vpn_iface);
	zassert_not_null(vpn_iface, "No VPN interface");

	ret = base64_decode(key, sizeof(key), &olen, private_key, strlen(private_key));
	zassert_true(ret == 0 && olen == sizeof(key), "Cannot decode the private key");

	params.private_key.data = key;
	params.private_key.len = sizeof(key);

	ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PRIVATE_KEY, vpn_iface, &params,
		       sizeof(params));
	(void)memset(key, 0, sizeof(key));
	zassert_equal(ret, 0, "Cannot set the private key (%d)", ret);
}

/* Each test removes the peer it adds, and checks with zexpect while the peer
 * exists so that it always gets there.
 */

static const char *peer_public_key = "fF2zTE6NVbBEZkwDgaPDkd9Vx7nL6xj/6g9W3nwyyHc=";

/* A peer that was just added has no endpoint learnt from it yet. A keepalive
 * must start the first handshake, to the configured endpoint, so that an
 * application can bring the tunnel up without sending traffic into it.
 */
ZTEST(wireguard_keepalive, test_keepalive_starts_first_handshake)
{
	struct wireguard_peer_config config = {
		.public_key = peer_public_key,
	};
	struct net_sockaddr_in *cfg_endpoint = (struct net_sockaddr_in *)&config.endpoint_ip;
	struct net_sockaddr_storage endpoint;
	struct net_stats_vpn before;
	struct net_stats_vpn after;
	struct net_if *peer_iface;
	int peer_id;
	int ret;

	set_private_key();

	cfg_endpoint->sin_family = NET_AF_INET;
	cfg_endpoint->sin_port = net_htons(51820);
	cfg_endpoint->sin_addr.s_addr = net_htonl(0xC0000201U); /* 192.0.2.1 */

	peer_id = wireguard_peer_add(&config, &peer_iface);
	zassert_true(peer_id > 0, "Peer add failed (%d)", peer_id);

	ret = wireguard_test_peer_endpoint(peer_id, &endpoint);
	zexpect_equal(ret, 0);
	zexpect_equal(endpoint.ss_family, NET_AF_UNSPEC, "New peer has a learnt endpoint");

	ret = wireguard_test_get_stats(peer_id, &before);
	zexpect_equal(ret, 0);

	ret = wireguard_peer_keepalive(peer_id);
	zexpect_equal(ret, 0, "Keepalive failed (%d)", ret);

	ret = wireguard_test_get_stats(peer_id, &after);
	zexpect_equal(ret, 0);
	zexpect_equal(after.handshake_init_tx, before.handshake_init_tx + 1,
		      "No handshake initiation sent");

	ret = wireguard_test_peer_endpoint(peer_id, &endpoint);
	zexpect_equal(ret, 0);
	zexpect_equal(endpoint.ss_family, NET_AF_INET, "Not sent to the configured endpoint");
	zexpect_equal(net_sin(net_sad(&endpoint))->sin_addr.s_addr,
		      cfg_endpoint->sin_addr.s_addr);
	zexpect_equal(net_sin(net_sad(&endpoint))->sin_port, cfg_endpoint->sin_port);

	/* While the handshake is pending, another keepalive is no error and
	 * sends no other initiation before the rekey timeout. Nor does it leave
	 * a handshake request behind, which would rekey the session as soon as
	 * the rekey timeout expires.
	 */
	ret = wireguard_peer_keepalive(peer_id);
	zexpect_equal(ret, 0, "Second keepalive failed (%d)", ret);
	zexpect_false(wireguard_test_peer_send_handshake(peer_id),
		      "Handshake request left pending");

	before = after;
	ret = wireguard_test_get_stats(peer_id, &after);
	zexpect_equal(ret, 0);
	zexpect_equal(after.handshake_init_tx, before.handshake_init_tx);
	zexpect_equal(after.invalid_key, before.invalid_key);
	zexpect_equal(after.invalid_ip_family, before.invalid_ip_family);

	ret = wireguard_peer_remove(peer_id);
	zassert_equal(ret, 0, "Peer remove failed (%d)", ret);
}

/* A peer added without an endpoint can only be reached once it contacts us */
ZTEST(wireguard_keepalive, test_keepalive_without_endpoint)
{
	struct wireguard_peer_config config = {
		.public_key = peer_public_key,
	};
	struct net_stats_vpn before;
	struct net_stats_vpn after;
	struct net_if *peer_iface;
	int peer_id;
	int ret;

	set_private_key();

	peer_id = wireguard_peer_add(&config, &peer_iface);
	zassert_true(peer_id > 0, "Peer add failed (%d)", peer_id);

	ret = wireguard_test_get_stats(peer_id, &before);
	zexpect_equal(ret, 0);

	ret = wireguard_peer_keepalive(peer_id);
	zexpect_equal(ret, -EDESTADDRREQ, "Unexpected result (%d)", ret);

	ret = wireguard_test_get_stats(peer_id, &after);
	zexpect_equal(ret, 0);
	zexpect_equal(after.handshake_init_tx, before.handshake_init_tx);

	ret = wireguard_peer_remove(peer_id);
	zassert_equal(ret, 0, "Peer remove failed (%d)", ret);
}

static void *keepalive_setup(void)
{
	/* The library takes a zero tick count for "never sent", so leave the
	 * first tick behind before sending anything.
	 */
	k_sleep(K_MSEC(10));

	return NULL;
}

ZTEST_SUITE(wireguard_keepalive, NULL, keepalive_setup, NULL, NULL, NULL);
