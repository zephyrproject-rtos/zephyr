/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/virtual_mgmt.h>
#include "wg_private.h"
#include <zephyr/random/random.h>
#include <zephyr/sys/base64.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "ipv4.h"
#include "udp_internal.h"
#include "wg_internal.h"
#include "wg_test_helpers.h"

struct datagram {
	uint8_t data[160];
	size_t len;
	int from;
	uint32_t reply_relay;
};

K_MSGQ_DEFINE(datagrams, sizeof(struct datagram), 4, 4);

static int capture(int peer_id, const struct wireguard_datagram_path *path, const uint8_t *data,
		   size_t len, void *user_data)
{
	struct datagram msg = {
		.from = peer_id,
		.len = len,
		.reply_relay = path != NULL ? path->relay_id : 0U,
	};

	atomic_t *refuse_init = user_data;

	if (data[0] == 1U && atomic_cas(refuse_init, 1, 0)) {
		return -ENOBUFS;
	}
	if (len > sizeof(msg.data)) {
		return -EMSGSIZE;
	}
	memcpy(msg.data, data, len);
	return k_msgq_put(&datagrams, &msg, K_NO_WAIT);
}

static void collect_interfaces(struct net_if *iface, void *user_data)
{
	struct net_if **interfaces = user_data;

	if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL) &&
	    net_virtual_get_iface_capabilities(iface) == VIRTUAL_INTERFACE_VPN) {
		if (interfaces[0] == NULL) {
			interfaces[0] = iface;
		} else if (interfaces[1] == NULL) {
			interfaces[1] = iface;
		}
	}
}

static void configure_identity(struct net_if *iface, uint8_t seed, char *pubkey, size_t size)
{
	struct virtual_interface_req_params params = {0};
	uint8_t key[32];
	size_t olen;

	memset(key, seed, sizeof(key));
	params.private_key.data = key;
	params.private_key.len = sizeof(key);
	zassert_ok(net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PRIVATE_KEY, iface, &params,
			    sizeof(params)));
	zassert_ok(net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_GET_PUBLIC_KEY, iface, &params,
			    sizeof(params)));
	zassert_ok(
		base64_encode(pubkey, size, &olen, params.public_key.data, params.public_key.len));
}

struct echo_listener {
	struct k_work remove;
	struct k_sem done;
	int peer_id;
	int remove_result;
	struct net_if *iface;
	uint8_t bytes[16];
	bool received;
};

static void remove_during_delivery(struct k_work *work)
{
	struct echo_listener *listener = CONTAINER_OF(work, struct echo_listener, remove);

	listener->remove_result = wireguard_peer_remove(listener->peer_id);
	k_sem_give(&listener->done);
}

static enum net_verdict echo_input(struct net_conn *conn, struct net_pkt *pkt,
				   union net_ip_header *ip, union net_proto_header *proto,
				   void *user_data)
{
	struct echo_listener *listener = user_data;

	if (net_pkt_iface(pkt) != listener->iface ||
	    net_pkt_read(pkt, listener->bytes, sizeof(listener->bytes)) < 0) {
		return NET_DROP;
	}

	if (k_work_submit(&listener->remove) < 0 ||
	    k_sem_take(&listener->done, K_SECONDS(1)) != 0 || listener->remove_result != -EBUSY) {
		return NET_DROP;
	}
	listener->received = true;
	net_pkt_unref(pkt);
	return NET_OK;
}

static int send_udp(struct net_if *iface, struct net_in_addr *src, struct net_in_addr *dst,
		    const uint8_t *payload, size_t len)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(iface, len, NET_AF_INET, NET_IPPROTO_UDP, K_NO_WAIT);
	zassert_not_null(pkt);
	net_pkt_set_ipv4_ttl(pkt, 64);
	zassert_ok(net_ipv4_create(pkt, src, dst));
	zassert_ok(net_udp_create(pkt, net_htons(9501), net_htons(9500)));
	zassert_ok(net_pkt_write(pkt, payload, len));
	net_pkt_cursor_init(pkt);
	zassert_ok(net_ipv4_finalize(pkt, NET_IPPROTO_UDP));
	ret = net_if_l2(iface)->send(iface, pkt);
	if (ret < 0) {
		net_pkt_unref(pkt);
	}
	return ret;
}

static void receive(int sender, int receiver, struct wireguard_datagram_path *path, uint8_t type,
		    uint32_t expected_reply)
{
	struct datagram msg;
	int ret;

	zassert_ok(k_msgq_get(&datagrams, &msg, K_SECONDS(1)));
	zassert_equal(msg.from, sender);
	zassert_equal(msg.data[0], type);
	zassert_equal(msg.reply_relay, expected_reply);
	ret = wireguard_peer_receive_datagram(receiver, path, msg.data, msg.len);

	zassert_ok(ret, "peer %d type %u len %zu returned %d", receiver, type, msg.len, ret);
}

struct keepalive_work {
	struct k_work work;
	int id;
	int result;
};

static void send_keepalives(struct k_work *work)
{
	struct keepalive_work *tx = CONTAINER_OF(work, struct keepalive_work, work);

	tx->result = wireguard_peer_keepalive(tx->id);
	if (tx->result == 0) {
		tx->result = wireguard_peer_keepalive(tx->id);
	}
}

struct wireguard_datagram_fixture {
	atomic_t refuse_init;
	struct net_if *interfaces[2];
	struct net_in_addr ips[2];
	int ids[2];
	struct wireguard_datagram_path relay;
	struct echo_listener listener;
	struct net_conn_handle *handle;
};

ZTEST_F(wireguard_datagram, test_concurrent_tx_and_backpressure)
{
	int peer_id = fixture->ids[0];
	struct k_work_sync sync;
	struct keepalive_work tx = {.id = peer_id};
	struct datagram msg;
	uint64_t counter = 0U;

	k_work_init(&tx.work, send_keepalives);
	zassert_true(k_work_submit(&tx.work) >= 0);
	zexpect_ok(wireguard_peer_keepalive(peer_id));
	zexpect_ok(wireguard_peer_keepalive(peer_id));
	(void)k_work_flush(&tx.work, &sync);
	zassert_ok(tx.result);
	zexpect_true(wireguard_peer_keepalive(peer_id) < 0);

	for (int i = 0; i < 4; i++) {
		zassert_ok(k_msgq_get(&datagrams, &msg, K_NO_WAIT));
		zassert_equal(msg.len, 32U);
		if (i > 0) {
			zassert_equal(sys_get_le64(msg.data + 8), counter + 1U);
		}
		counter = sys_get_le64(msg.data + 8);
	}

	zassert_ok(wireguard_peer_keepalive(peer_id));
	zassert_ok(k_msgq_get(&datagrams, &msg, K_NO_WAIT));
	/* The transport refused one encrypted datagram while its queue was full. */
	zassert_equal(sys_get_le64(msg.data + 8), counter + 2U);
}

static void *suite_setup(void)
{
	static struct wireguard_datagram_fixture fixture;

	fixture.ips[0].s_addr = net_htonl(0xC6336401U);
	fixture.ips[1].s_addr = net_htonl(0xC6336402U);
	fixture.relay.kind = WIREGUARD_PATH_RELAY;
	fixture.relay.relay_id = 7U;
	return &fixture;
}

static void connect_peers(void *data)
{
	struct wireguard_datagram_fixture *fixture = data;
	const struct wireguard_transport transport = {
		.send = capture,
		.user_data = &fixture->refuse_init,
	};
	const uint8_t first = 0;
	int ret;
	struct wireguard_peer_config config = {0};
	struct net_if *assigned;
	char pubkeys[2][45];

	net_if_foreach(collect_interfaces, fixture->interfaces);
	for (size_t i = 0; i < ARRAY_SIZE(fixture->interfaces); i++) {
		zassert_not_null(fixture->interfaces[i]);
		configure_identity(fixture->interfaces[i], i + 1U, pubkeys[i], sizeof(pubkeys[i]));
	}
	for (size_t i = 0; i < ARRAY_SIZE(fixture->interfaces); i++) {
		config.public_key = pubkeys[1U - i];
		config.allowed_ip[0].is_valid = true;
		config.allowed_ip[0].addr.family = NET_AF_INET;
		config.allowed_ip[0].addr.in_addr = fixture->ips[1U - i];
		config.allowed_ip[0].mask_len = 32U;
		fixture->ids[i] = wireguard_peer_add_with_transport(&config, &assigned, &transport);
		zassert_true(fixture->ids[i] > 0, "peer add failed (%d)", fixture->ids[i]);
		zassert_equal(assigned, fixture->interfaces[i]);
	}

	k_sleep(K_TICKS(1));
	atomic_set(&fixture->refuse_init, 1);
	ret = send_udp(fixture->interfaces[0], &fixture->ips[0], &fixture->ips[1], &first, 1U);
	zassert_true(ret == -ENOBUFS || ret == -EAGAIN);
	receive(fixture->ids[0], fixture->ids[1], &fixture->relay, 1U, 0U);
	receive(fixture->ids[1], fixture->ids[0], &fixture->relay, 2U, fixture->relay.relay_id);
	receive(fixture->ids[0], fixture->ids[1], &fixture->relay, 4U, 0U);
}

static void disconnect_peers(void *data)
{
	struct wireguard_datagram_fixture *fixture = data;

	if (fixture->handle != NULL) {
		zexpect_ok(net_udp_unregister(fixture->handle));
		fixture->handle = NULL;
	}
	for (size_t i = 0; i < ARRAY_SIZE(fixture->ids); i++) {
		if (fixture->ids[i] > 0) {
			zexpect_ok(wireguard_peer_remove(fixture->ids[i]));
			fixture->ids[i] = 0;
		}
		if (fixture->interfaces[i] != NULL) {
			(void)net_if_ipv4_addr_rm(fixture->interfaces[i], &fixture->ips[i]);
			fixture->interfaces[i] = NULL;
		}
	}
	k_msgq_purge(&datagrams);
}

ZTEST_F(wireguard_datagram, test_udp_echo_and_path_change)
{
	struct wireguard_datagram_path direct = {.kind = WIREGUARD_PATH_DIRECT};
	struct net_sockaddr_storage endpoint;
	struct echo_listener *listener = &fixture->listener;
	uint8_t challenge[sizeof(listener->bytes)];
	int *ids = fixture->ids;

	direct.iface = net_if_get_default();
	direct.remote.ss_family = NET_AF_INET;
	net_sin(net_sad(&direct.remote))->sin_addr.s_addr = net_htonl(0xC0000201U);
	net_sin(net_sad(&direct.remote))->sin_port = net_htons(51820);
	direct.local.ss_family = NET_AF_INET;
	net_sin(net_sad(&direct.local))->sin_addr.s_addr = net_htonl(0xC0000202U);
	net_sin(net_sad(&direct.local))->sin_port = net_htons(51820);
	zassert_ok(wireguard_peer_keepalive(ids[0]));
	receive(ids[0], ids[1], &direct, 4U, 0U);
	zassert_ok(wireguard_peer_keepalive(ids[0]));
	receive(ids[0], ids[1], &fixture->relay, 4U, 0U);
	zassert_ok(wireguard_test_peer_endpoint(ids[1], &endpoint));
	zassert_mem_equal(&endpoint, &direct.remote, sizeof(endpoint));

	k_work_init(&listener->remove, remove_during_delivery);
	k_sem_init(&listener->done, 0, 1);
	sys_rand_get(challenge, sizeof(challenge));
	zassert_ok(net_udp_register(NET_AF_INET, NULL, NULL, 0, 9500, NULL, echo_input, listener,
				    &fixture->handle));
	/* Both nodes share a simulated IP stack. Install only the receiving address
	 * so the stack can verify the source as a remote address on each leg.
	 */
	listener->peer_id = ids[1];
	listener->iface = fixture->interfaces[1];
	zassert_not_null(
		net_if_ipv4_addr_add(listener->iface, &fixture->ips[1], NET_ADDR_MANUAL, 0));
	zassert_ok(send_udp(fixture->interfaces[0], &fixture->ips[0], &fixture->ips[1], challenge,
			    sizeof(challenge)));
	receive(ids[0], ids[1], &fixture->relay, 4U, 0U);
	zassert_true(listener->received);
	zassert_mem_equal(listener->bytes, challenge, sizeof(challenge));

	zassert_true(net_if_ipv4_addr_rm(listener->iface, &fixture->ips[1]));
	listener->peer_id = ids[0];
	listener->iface = fixture->interfaces[0];
	zassert_not_null(
		net_if_ipv4_addr_add(listener->iface, &fixture->ips[0], NET_ADDR_MANUAL, 0));
	listener->received = false;
	zassert_ok(send_udp(fixture->interfaces[1], &fixture->ips[1], &fixture->ips[0],
			    listener->bytes, sizeof(listener->bytes)));
	receive(ids[1], ids[0], &direct, 4U, 0U);
	zassert_true(listener->received);
	zassert_mem_equal(listener->bytes, challenge, sizeof(challenge));
	zassert_true(net_if_ipv4_addr_rm(listener->iface, &fixture->ips[0]));
}

ZTEST_F(wireguard_datagram, test_receive_across_rekey)
{
	struct datagram pending;
	struct datagram renewed;
	int *ids = fixture->ids;

	zassert_ok(wireguard_peer_keepalive(ids[0]));
	zassert_ok(k_msgq_get(&datagrams, &pending, K_SECONDS(1)));

	k_sleep(K_SECONDS(REKEY_AFTER_TIME + 1U));
	zassert_ok(wireguard_peer_keepalive(ids[1]));
	receive(ids[1], ids[0], &fixture->relay, MESSAGE_TRANSPORT_DATA, 0U);
	receive(ids[0], ids[1], &fixture->relay, MESSAGE_HANDSHAKE_INITIATION, 0U);
	receive(ids[1], ids[0], &fixture->relay, MESSAGE_HANDSHAKE_RESPONSE,
		fixture->relay.relay_id);

	zassert_ok(k_msgq_get(&datagrams, &renewed, K_SECONDS(1)));
	zassert_equal(renewed.from, ids[0]);
	zassert_equal(renewed.data[0], MESSAGE_TRANSPORT_DATA);
	zassert_not_equal(sys_get_le32(renewed.data + 4), sys_get_le32(pending.data + 4));
	zassert_ok(wireguard_peer_receive_datagram(ids[1], &fixture->relay, renewed.data,
						   renewed.len));
	zassert_ok(wireguard_peer_receive_datagram(ids[1], &fixture->relay, pending.data,
						   pending.len));
}

ZTEST_F(wireguard_datagram, test_key_failure_and_late_receive)
{
	int *ids = fixture->ids;
	int retired = ids[1];
	struct datagram msg;

	zassert_ok(wireguard_peer_keepalive(ids[0]));
	zassert_ok(k_msgq_get(&datagrams, &msg, K_SECONDS(1)));
	zassert_ok(net_if_down(fixture->interfaces[1]));
	zexpect_equal(wireguard_peer_keepalive(ids[1]), -ENETDOWN);
	zexpect_equal(wireguard_peer_receive_datagram(ids[1], &fixture->relay, msg.data, msg.len),
		      -ENETDOWN);
	zassert_ok(net_if_up(fixture->interfaces[1]));
	zassert_ok(wireguard_peer_receive_datagram(ids[1], &fixture->relay, msg.data, msg.len));
	zassert_ok(wireguard_test_destroy_sending_key(ids[0]));
	zexpect_true(wireguard_peer_keepalive(ids[0]) < 0);
	zexpect_equal(k_msgq_num_used_get(&datagrams), 0U);
	zassert_ok(wireguard_peer_remove(ids[1]));
	ids[1] = 0;
	zexpect_equal(wireguard_peer_receive_datagram(retired, &fixture->relay, msg.data, msg.len),
		      -ENOENT);
}

ZTEST_SUITE(wireguard_datagram, NULL, suite_setup, connect_peers, disconnect_peers, NULL);

#if defined(CONFIG_WIREGUARD_MULTI_PEER)
static int add_shared_peer(struct net_if *iface, const char *key, uint32_t address, uint8_t prefix,
			   const struct wireguard_transport *transport)
{
	struct wireguard_peer_config config = {.public_key = key};

	config.allowed_ip[0].addr.family = NET_AF_INET;
	config.allowed_ip[0].addr.in_addr.s_addr = net_htonl(address);
	config.allowed_ip[0].is_valid = true;
	config.allowed_ip[0].mask_len = prefix;
	return wireguard_peer_add_to_interface(iface, &config, transport);
}

static void shared_path(struct wireguard_datagram_path *path)
{
	*path = (struct wireguard_datagram_path){
		.kind = WIREGUARD_PATH_DIRECT,
		.iface = net_if_get_default(),
	};
	path->local.ss_family = NET_AF_INET;
	net_sin(net_sad(&path->local))->sin_port = net_htons(51820);
	net_sin(net_sad(&path->local))->sin_addr.s_addr = net_htonl(0xc0000260U);
	path->remote.ss_family = NET_AF_INET;
	net_sin(net_sad(&path->remote))->sin_port = net_htons(51821);
	net_sin(net_sad(&path->remote))->sin_addr.s_addr = net_htonl(0xc0000261U);
}

struct wireguard_shared_fixture {
	int hub[2];
	int leaves[2];
	atomic_t refuse;
};

static void *shared_setup(void)
{
	static struct wireguard_shared_fixture fixture;

	return &fixture;
}

static void shared_cleanup(void *data)
{
	struct wireguard_shared_fixture *fixture = data;

	ARRAY_FOR_EACH(fixture->leaves, i) {
		if (fixture->leaves[i] > 0) {
			zexpect_ok(wireguard_peer_remove(fixture->leaves[i]));
			fixture->leaves[i] = 0;
		}
		if (fixture->hub[i] > 0) {
			zexpect_ok(wireguard_peer_remove(fixture->hub[i]));
			fixture->hub[i] = 0;
		}
	}
}

static void receive_pair(const int senders[2], uint8_t type, struct datagram pair[2])
{
	bool seen[2] = {false};

	ARRAY_FOR_EACH(seen, i) {
		struct datagram msg;
		size_t slot;

		zassert_ok(k_msgq_get(&datagrams, &msg, K_SECONDS(1)));
		zassert_equal(msg.data[0], type);
		slot = msg.from == senders[0] ? 0U : 1U;
		zassert_equal(msg.from, senders[slot]);
		zassert_false(seen[slot], "duplicate datagram from peer %d", msg.from);
		seen[slot] = true;
		pair[slot] = msg;
	}
}

ZTEST_F(wireguard_shared, test_concurrent_initiation_and_ownership)
{
	struct net_if *iface[] = {
		net_if_get_by_index(net_if_get_by_name("wg0")),
		net_if_get_by_index(net_if_get_by_name("wg1")),
		net_if_get_by_index(net_if_get_by_name("wg2")),
	};
	uint32_t addresses[] = {0xc6336401U, 0xc6336402U, 0xc6336403U};
	struct wireguard_datagram_path relay = {.kind = WIREGUARD_PATH_RELAY, .relay_id = 7};
	struct wireguard_datagram_path direct;
	struct wireguard_transport transport = {.send = capture, .user_data = &fixture->refuse};
	struct datagram pair[2], msg;
	char keys[3][45];

	k_msgq_purge(&datagrams);
	shared_path(&direct);
	for (size_t i = 0; i < ARRAY_SIZE(iface); i++) {
		zassert_not_null(iface[i]);
		configure_identity(iface[i], 20 + i * 8, keys[i], sizeof(keys[i]));
	}

	fixture->hub[0] = add_shared_peer(iface[0], keys[1], addresses[1], 24, &transport);
	zassert_true(fixture->hub[0] > 0);
	zassert_equal(add_shared_peer(iface[0], keys[1], addresses[2], 32, &transport), -EALREADY);
	zassert_equal(add_shared_peer(iface[0], keys[2], addresses[2], 24, &transport),
		      -EADDRINUSE);
	fixture->hub[1] = add_shared_peer(iface[0], keys[2], addresses[2], 32, &transport);
	zassert_true(fixture->hub[1] > 0);

	for (size_t i = 0; i < ARRAY_SIZE(fixture->leaves); i++) {
		fixture->leaves[i] =
			add_shared_peer(iface[i + 1], keys[0], addresses[0], 32, &transport);
		zassert_true(fixture->leaves[i] > 0);
		zassert_ok(wireguard_peer_keepalive(fixture->leaves[i]));
	}
	receive_pair(fixture->leaves, MESSAGE_HANDSHAKE_INITIATION, pair);
	for (size_t i = 0; i < ARRAY_SIZE(pair); i++) {
		/* Relay metadata binds a specific peer; the direct socket knows only the device. */
		zassert_true(wireguard_peer_receive_datagram(fixture->hub[1 - i], &relay,
							     pair[i].data, pair[i].len) < 0);
		zassert_true(wireguard_interface_receive_datagram(iface[0], &relay, pair[i].data,
								  pair[i].len) < 0);
		zassert_ok(wireguard_interface_receive_datagram(iface[0], &direct, pair[i].data,
								pair[i].len));
	}
	receive_pair(fixture->hub, MESSAGE_HANDSHAKE_RESPONSE, pair);
	for (size_t i = 0; i < ARRAY_SIZE(pair); i++) {
		zassert_ok(wireguard_interface_receive_datagram(iface[i + 1], &direct,
								pair[i].data, pair[i].len));
	}
	receive_pair(fixture->leaves, MESSAGE_TRANSPORT_DATA, pair);
	for (size_t i = 0; i < ARRAY_SIZE(pair); i++) {
		zassert_ok(wireguard_interface_receive_datagram(iface[0], &direct, pair[i].data,
								pair[i].len));
	}
	zassert_not_equal(sys_get_le32(&pair[0].data[4]), sys_get_le32(&pair[1].data[4]));

	struct net_in_addr narrow = {.s_addr = net_htonl(addresses[2])};
	struct net_in_addr local = {.s_addr = net_htonl(addresses[0])};
	uint8_t payload = 123U;

	zassert_ok(send_udp(iface[0], &local, &narrow, &payload, sizeof(payload)));
	zassert_ok(k_msgq_get(&datagrams, &msg, K_SECONDS(1)));
	zassert_equal(msg.from, fixture->hub[1], "prefer the host route to the covering prefix");

	/* The covering peer cannot claim a source owned by the more specific peer. */
	zassert_ok(send_udp(iface[1], &narrow, &local, &payload, sizeof(payload)));
	zassert_ok(k_msgq_get(&datagrams, &msg, K_SECONDS(1)));
	zassert_equal(msg.from, fixture->leaves[0]);
	struct net_stats_vpn before, after;

	zassert_ok(wireguard_test_get_stats(fixture->hub[0], &before));
	zassert_equal(wireguard_interface_receive_datagram(iface[0], &direct, msg.data, msg.len),
		      -ENOENT);
	zassert_ok(wireguard_test_get_stats(fixture->hub[0], &after));
	zassert_equal(after.denied_ip, before.denied_ip + 1);
	zassert_ok(wireguard_peer_remove(fixture->hub[1]));
	fixture->hub[1] = 0;
	zassert_ok(send_udp(iface[0], &local, &narrow, &payload, sizeof(payload)));
	zassert_ok(k_msgq_get(&datagrams, &msg, K_SECONDS(1)));
	zassert_equal(msg.from, fixture->hub[0], "covering prefix is now the longest match");
}

ZTEST_SUITE(wireguard_shared, NULL, shared_setup, NULL, shared_cleanup, NULL);
#endif /* CONFIG_WIREGUARD_MULTI_PEER */
