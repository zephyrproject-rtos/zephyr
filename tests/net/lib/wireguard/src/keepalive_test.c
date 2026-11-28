/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/virtual_mgmt.h>
#include <zephyr/net/wireguard.h>
#include <zephyr/sys/base64.h>

#include "wg_private.h"
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

static void set_test_endpoint(struct wireguard_peer_config *config)
{
	struct net_sockaddr_in *cfg_endpoint = (struct net_sockaddr_in *)&config->endpoint_ip;

	cfg_endpoint->sin_family = NET_AF_INET;
	cfg_endpoint->sin_port = net_htons(51820);
	cfg_endpoint->sin_addr.s_addr = net_htonl(0xC0000201U); /* 192.0.2.1 */
}

K_MSGQ_DEFINE(added_ids, sizeof(int), 2, sizeof(int));

static void record_added_id(struct net_mgmt_event_callback *cb, uint64_t event,
			    struct net_if *iface)
{
	if (event == NET_EVENT_VPN_PEER_ADD &&
	    cb->info_length == sizeof(struct net_event_vpn_peer)) {
		struct net_event_vpn_peer added;
		int id;

		memcpy(&added, cb->info, sizeof(added));
		id = added.id;
		(void)k_msgq_put(&added_ids, &id, K_NO_WAIT);
	}
}

ZTEST(wireguard_keepalive, test_added_peer_configuration_snapshot)
{
	struct wireguard_peer_config config = {.public_key = peer_public_key};
	struct net_mgmt_event_callback cb;
	struct wireguard_peer_info info = {0};
	struct net_if *iface;
	int observed = 0;
	int id;

	set_private_key();
	set_test_endpoint(&config);
	config.keepalive_interval = 30;
	config.allowed_ip[0].addr.family = NET_AF_INET;
	config.allowed_ip[0].addr.in_addr.s_addr = net_htonl(0xc6336402U);
	config.allowed_ip[0].mask_len = 32;
	config.allowed_ip[0].is_valid = true;
	net_mgmt_init_event_callback(&cb, record_added_id, NET_EVENT_VPN_PEER_ADD);
	net_mgmt_add_event_callback(&cb);
	id = wireguard_peer_add(&config, &iface);
	zexpect_true(id > 0);
	zexpect_ok(k_msgq_get(&added_ids, &observed, K_SECONDS(1)));
	net_mgmt_del_event_callback(&cb);
	zexpect_equal(observed, id);
	zexpect_ok(wireguard_peer_get_info(id, &info));
	zexpect_mem_equal(&info.endpoint, &config.endpoint_ip, sizeof(info.endpoint));
	zexpect_equal(info.keepalive_interval, 30);
	zexpect_true(info.allowed_ip[0].is_valid);
	zexpect_equal(info.allowed_ip[0].mask_len, 32);
	zexpect_equal(info.allowed_ip[0].addr.in_addr.s_addr, net_htonl(0xc6336402U));
	zexpect_equal(info.public_key[0], 0x7c);
	zexpect_ok(wireguard_peer_remove(id));
	zexpect_equal(wireguard_peer_get_info(id, &info), -ENOENT);
	zexpect_equal(info.public_key[0], 0x7c);
}

/* With a Wireguard work queue of its own, the periodic timer sends the first
 * initiation there, after the call that started it has returned.
 */
static uint32_t wait_for_init_tx(int peer_id, uint32_t count)
{
	struct net_stats_vpn stats = { 0 };
	k_timepoint_t end = sys_timepoint_calc(K_SECONDS(1));

	do {
		if (wireguard_test_get_stats(peer_id, &stats) != 0) {
			break;
		}

		if (stats.handshake_init_tx >= count) {
			break;
		}

		k_sleep(K_MSEC(10));
	} while (!sys_timepoint_expired(end));

	return stats.handshake_init_tx;
}

ZTEST(wireguard_keepalive, test_readd_uses_provisioned_private_key)
{
	struct wireguard_peer_config config = {.public_key = peer_public_key};
	struct net_stats_vpn before = {0};
	struct net_if *iface;
	struct net_if *reattached;
	int id;
	int new_id;

	set_private_key();
	set_test_endpoint(&config);
	id = wireguard_peer_add(&config, &iface);
	zassert_true(id > 0, "Peer add failed (%d)", id);
	zassert_ok(wireguard_peer_remove(id));

	new_id = wireguard_peer_add(&config, &reattached);
	zassert_true(new_id > 0, "Peer re-add failed (%d)", new_id);
	zexpect_not_equal(new_id, id);
	zexpect_equal_ptr(reattached, iface);
	zexpect_ok(wireguard_test_get_stats(new_id, &before));
	zexpect_ok(wireguard_peer_keepalive(new_id));
	zexpect_equal(wait_for_init_tx(new_id, before.handshake_init_tx + 1U),
		      before.handshake_init_tx + 1U);
	zexpect_ok(wireguard_peer_remove(new_id));
}

ZTEST(wireguard_keepalive, test_keepalive_drops_on_encryption_failure)
{
	const struct wireguard_test_session session = {
		.recv_key = { 1U },
		.send_key = { 2U },
		.local_index = 1U,
		.remote_index = 2U,
	};
	struct wireguard_peer_config config = { .public_key = peer_public_key };
	struct net_stats_vpn before = { 0 };
	struct net_stats_vpn after = { 0 };
	struct net_if *iface;
	int id;

	set_private_key();
	set_test_endpoint(&config);
	id = wireguard_peer_add(&config, &iface);
	zassert_true(id > 0, "Peer add failed (%d)", id);

	zexpect_ok(wireguard_test_get_stats(id, &before));
	zexpect_ok(wireguard_peer_keepalive(id));
	zexpect_equal(wait_for_init_tx(id, before.handshake_init_tx + 1U),
		      before.handshake_init_tx + 1U);
	zexpect_ok(wireguard_test_install_session(id, &session));
	zexpect_ok(wireguard_peer_keepalive(id));
	zexpect_ok(wireguard_test_get_stats(id, &before));

	zexpect_ok(wireguard_test_destroy_sending_key(id));
	zexpect_true(wireguard_peer_keepalive(id) < 0);
	zexpect_ok(wireguard_test_get_stats(id, &after));
	zexpect_equal(after.valid_tx, before.valid_tx);
	zexpect_equal(after.keepalive_tx, before.keepalive_tx);
	zexpect_ok(wireguard_peer_remove(id));
}

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
	set_test_endpoint(&config);

	peer_id = wireguard_peer_add(&config, &peer_iface);
	zassert_true(peer_id > 0, "Peer add failed (%d)", peer_id);

	ret = wireguard_test_peer_endpoint(peer_id, &endpoint);
	zexpect_equal(ret, 0);
	zexpect_equal(endpoint.ss_family, NET_AF_UNSPEC, "New peer has a learnt endpoint");

	ret = wireguard_test_get_stats(peer_id, &before);
	zexpect_equal(ret, 0);

	ret = wireguard_peer_keepalive(peer_id);
	zexpect_equal(ret, 0, "Keepalive failed (%d)", ret);

	zexpect_equal(wait_for_init_tx(peer_id, before.handshake_init_tx + 1),
		      before.handshake_init_tx + 1, "No handshake initiation sent");

	ret = wireguard_test_get_stats(peer_id, &after);
	zexpect_equal(ret, 0);

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

/* With a Wireguard work queue of its own, the initiation started by a
 * keepalive is not computed on the stack of the calling thread: with the
 * scheduler locked nothing is sent before the call returns, and the work
 * queue sends the initiation once the scheduler is unlocked.
 */
ZTEST(wireguard_keepalive, test_keepalive_initiation_on_workq)
{
	struct wireguard_peer_config config = {
		.public_key = peer_public_key,
	};
	struct net_stats_vpn before;
	struct net_stats_vpn locked;
	struct net_if *peer_iface;
	bool pending;
	int peer_id;
	int ret;

	if (CONFIG_WIREGUARD_WORKQ_STACK_SIZE == 0) {
		ztest_test_skip();
	}

	set_private_key();
	set_test_endpoint(&config);

	peer_id = wireguard_peer_add(&config, &peer_iface);
	zassert_true(peer_id > 0, "Peer add failed (%d)", peer_id);

	ret = wireguard_test_get_stats(peer_id, &before);
	zexpect_equal(ret, 0);

	k_sched_lock();
	ret = wireguard_peer_keepalive(peer_id);
	pending = wireguard_test_peer_send_handshake(peer_id);
	(void)wireguard_test_get_stats(peer_id, &locked);
	k_sched_unlock();

	zexpect_equal(ret, 0, "Keepalive failed (%d)", ret);
	zexpect_true(pending, "Initiation not left to the work queue");
	zexpect_equal(locked.handshake_init_tx, before.handshake_init_tx,
		      "Initiation sent from the calling thread");

	zexpect_equal(wait_for_init_tx(peer_id, before.handshake_init_tx + 1),
		      before.handshake_init_tx + 1, "No handshake initiation sent");
	zexpect_false(wireguard_test_peer_send_handshake(peer_id),
		      "Handshake request left pending");

	ret = wireguard_peer_remove(peer_id);
	zassert_equal(ret, 0, "Peer remove failed (%d)", ret);
}

/* Nor is the initiation started by the first packet sent to a peer: the
 * packet is dropped with -EAGAIN, the handshake left due, and the work queue
 * sends the initiation once the scheduler is unlocked.
 */
ZTEST(wireguard_keepalive, test_first_packet_initiation_on_workq)
{
	struct wireguard_peer_config config = {
		.public_key = peer_public_key,
	};
	const struct virtual_interface_driver_api *api;
	struct net_stats_vpn before;
	struct net_stats_vpn locked;
	struct net_if *peer_iface;
	struct net_pkt *pkt;
	bool pending;
	int peer_id;
	int ret;

	if (CONFIG_WIREGUARD_WORKQ_STACK_SIZE == 0) {
		ztest_test_skip();
	}

	set_private_key();
	set_test_endpoint(&config);

	peer_id = wireguard_peer_add(&config, &peer_iface);
	zassert_true(peer_id > 0, "Peer add failed (%d)", peer_id);

	ret = wireguard_test_get_stats(peer_id, &before);
	zexpect_equal(ret, 0);

	/* An empty packet passes the allowed IP check */
	pkt = net_pkt_alloc_with_buffer(peer_iface, 0, NET_AF_INET, 0, K_NO_WAIT);
	zexpect_not_null(pkt, "Cannot allocate a packet");

	if (pkt != NULL) {
		api = net_if_get_device(peer_iface)->api;

		k_sched_lock();
		ret = api->send(peer_iface, pkt);
		pending = wireguard_test_peer_send_handshake(peer_id);
		(void)wireguard_test_get_stats(peer_id, &locked);
		k_sched_unlock();

		/* A packet that is not sent is left to the caller */
		net_pkt_unref(pkt);

		zexpect_equal(ret, -EAGAIN, "Unexpected result (%d)", ret);
		zexpect_true(pending, "Initiation not left to the work queue");
		zexpect_equal(locked.handshake_init_tx, before.handshake_init_tx,
			      "Initiation sent from the calling thread");

		zexpect_equal(wait_for_init_tx(peer_id, before.handshake_init_tx + 1),
			      before.handshake_init_tx + 1, "No handshake initiation sent");
		zexpect_false(wireguard_test_peer_send_handshake(peer_id),
			      "Handshake request left pending");
	}

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
