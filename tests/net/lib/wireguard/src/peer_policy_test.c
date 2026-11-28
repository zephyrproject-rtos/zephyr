/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/dummy.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/virtual_mgmt.h>
#include "wg_private.h"
#include <zephyr/sys/base64.h>
#include <zephyr/ztest.h>

#include "ipv4.h"
#include "udp_internal.h"
#include "wg_internal.h"
#include "wg_test_helpers.h"

#if defined(CONFIG_WIREGUARD_PEER_POLICY)

struct wireguard_policy_fixture {
	struct net_if *iface;
	struct net_if *underlay;
	struct net_conn_handle *listener;
	struct k_work revoke_work;
	struct net_pkt *retained[CONFIG_WIREGUARD_BUF_COUNT];
	size_t retained_count;
	bool managed;
	bool overlay_address;
	bool work_blocked;
	bool rx_blocked;
	bool handshake_blocked;
	bool revoke_pending;
	int reentrant_result;
	int revoke_result;
};

struct admission {
	uint64_t identity;
	size_t len;
	unsigned int calls;
	bool allow;
};

static const struct net_in_addr underlay_address = {.s_addr = net_htonl(0xc0000204U)};
static const struct net_in_addr inner_address = {.s_addr = net_htonl(0xc6336402U)};
static K_SEM_DEFINE(work_entered, 0, 1);
static K_SEM_DEFINE(work_release, 0, 1);
static K_SEM_DEFINE(native_sent, 0, 4);
static K_SEM_DEFINE(rx_entered, 0, 1);
static K_SEM_DEFINE(rx_release, 0, 1);
static K_SEM_DEFINE(revoked, 0, 1);
static K_SEM_DEFINE(retained_packet, 0, CONFIG_WIREGUARD_BUF_COUNT);
static struct net_in_addr native_destination;
static struct net_in_addr native_source;

#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
static K_SEM_DEFINE(handshake_entered, 0, 1);
static K_SEM_DEFINE(handshake_release, 0, 1);
static atomic_t pause_handshake;
static atomic_t raw_agreements;

psa_status_t __real_psa_raw_key_agreement(psa_algorithm_t alg, psa_key_id_t private_key,
					 const uint8_t *peer_key, size_t peer_key_length,
					 uint8_t *output, size_t output_size,
					 size_t *output_length);

psa_status_t __wrap_psa_raw_key_agreement(psa_algorithm_t alg, psa_key_id_t private_key,
					 const uint8_t *peer_key, size_t peer_key_length,
					 uint8_t *output, size_t output_size,
					 size_t *output_length)
{
	atomic_inc(&raw_agreements);
	if (atomic_cas(&pause_handshake, 1, 0)) {
		k_sem_give(&handshake_entered);
		if (k_sem_take(&handshake_release, K_SECONDS(5)) != 0) {
			return PSA_ERROR_GENERIC_ERROR;
		}
	}
	return __real_psa_raw_key_agreement(alg, private_key, peer_key, peer_key_length,
					   output, output_size, output_length);
}
#endif

static void delay_native_tx(struct k_work *work)
{
	k_sem_give(&work_entered);
	(void)k_sem_take(&work_release, K_SECONDS(5));
}

static K_WORK_DEFINE(tx_blocker, delay_native_tx);

static void policy_underlay_init(struct net_if *iface)
{
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	net_if_flag_set(iface, NET_IF_POINTOPOINT);
	net_if_flag_clear(iface, NET_IF_IPV4);
	net_if_flag_clear(iface, NET_IF_IPV6);
}

static int policy_underlay_send(const struct device *dev, struct net_pkt *pkt)
{
	memcpy(&native_source, NET_IPV4_HDR(pkt)->src, sizeof(native_source));
	memcpy(&native_destination, NET_IPV4_HDR(pkt)->dst, sizeof(native_destination));
	k_sem_give(&native_sent);
	return 0;
}

static DEVICE_API(dummy, policy_underlay_api) = {
	.iface_api.init = policy_underlay_init,
	.send = policy_underlay_send,
};

NET_DEVICE_INIT(wg_policy_underlay, "wg_policy_underlay", NULL, NULL, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &policy_underlay_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 1500);

static bool allow_input(uint64_t identity, const uint8_t *ip, size_t len, void *context)
{
	struct admission *admission = context;

	admission->identity = identity;
	admission->len = len;
	admission->calls++;
	return admission->allow;
}

static bool deny_input(uint64_t identity, const uint8_t *ip, size_t len, void *context)
{
	return false;
}

static void revoke_policy(struct k_work *work)
{
	struct wireguard_policy_fixture *fixture =
		CONTAINER_OF(work, struct wireguard_policy_fixture, revoke_work);

	fixture->revoke_result = wireguard_replace_peers(fixture->iface, NULL, 0, deny_input,
							 NULL);
	k_sem_give(&revoked);
}

static void *setup(void)
{
	static struct wireguard_policy_fixture fixture;

	fixture.iface = net_if_get_by_index(net_if_get_by_name("wg0"));
	zassert_not_null(fixture.iface);
	k_work_init(&fixture.revoke_work, revoke_policy);
	return &fixture;
}

static void provision(void *data)
{
	struct wireguard_policy_fixture *fixture = data;
	uint8_t private_key[32] = {0x74U};
	struct virtual_interface_req_params params = {
		.private_key = {.data = private_key, .len = sizeof(private_key)},
	};

	zassert_ok(net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PRIVATE_KEY, fixture->iface,
			    &params, sizeof(params)));
}

static void teardown(void *data)
{
	struct wireguard_policy_fixture *fixture = data;

#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
	if (fixture->handshake_blocked) {
		atomic_clear(&pause_handshake);
		k_sem_give(&handshake_release);
		fixture->handshake_blocked = false;
	}
#endif
	while (fixture->retained_count > 0U) {
		net_pkt_unref(fixture->retained[--fixture->retained_count]);
	}
	if (fixture->rx_blocked || fixture->revoke_pending) {
		struct k_work_sync sync;

		if (fixture->rx_blocked) {
			k_sem_give(&rx_release);
		}
		(void)k_work_flush(&fixture->revoke_work, &sync);
		fixture->rx_blocked = false;
		fixture->revoke_pending = false;
	}
	if (fixture->listener != NULL) {
		zexpect_ok(net_udp_unregister(fixture->listener));
		fixture->listener = NULL;
		zexpect_true(net_if_ipv4_addr_rm(fixture->iface, &inner_address));
	}
	if (fixture->overlay_address) {
		zexpect_true(net_if_ipv4_addr_rm(fixture->iface, &inner_address));
		fixture->overlay_address = false;
	}
	if (fixture->work_blocked) {
		struct k_work_sync sync;

		k_sem_give(&work_release);
		(void)k_work_flush(&tx_blocker, &sync);
		fixture->work_blocked = false;
	}
	if (fixture->managed) {
		zexpect_ok(wireguard_release_peers(fixture->iface));
		fixture->managed = false;
	}
	if (fixture->underlay != NULL) {
		zexpect_true(net_if_ipv4_addr_rm(fixture->underlay, &underlay_address));
		net_if_flag_clear(fixture->underlay, NET_IF_IPV4);
		zexpect_ok(net_if_down(fixture->underlay));
		fixture->underlay = NULL;
	}
}

static void prepare_peer(struct wireguard_policy_peer *peer, uint64_t identity, uint8_t seed,
			 uint32_t ipv4, char encoded[45])
{
	uint8_t key[32] = {seed};
	size_t len;

	zassert_ok(base64_encode(encoded, 45, &len, key, sizeof(key)));
	*peer = (struct wireguard_policy_peer){
		.identity = identity,
		.config = {
			.public_key = encoded,
			.allowed_ip[0] = {
				.addr = {
					.family = NET_AF_INET,
					.in_addr.s_addr = net_htonl(ipv4),
				},
				.mask_len = 32U,
				.is_valid = true,
			},
		},
	};
}

struct peer_match {
	uint64_t identity;
	int id;
};

static void find_policy_peer(struct wg_peer *peer, void *context)
{
	struct peer_match *match = context;

	if (peer->policy_identity == match->identity) {
		match->id = peer->id;
	}
}

static int peer_id(uint64_t identity)
{
	struct peer_match match = {.identity = identity, .id = -ENOENT};

	wireguard_peer_foreach(find_policy_peer, &match);
	return match.id;
}

static bool has_route(struct net_if *iface, uint32_t address)
{
	struct net_in_addr ip = {.s_addr = net_htonl(address)};

	return wireguard_policy_route_owner(iface, NET_AF_INET, &ip) != 0U;
}

ZTEST_F(wireguard_policy, test_staging_slots_are_reserved_across_interfaces)
{
	struct wireguard_policy_peer set[4];
	struct wireguard_policy_peer other_peer;
	struct net_if *other = net_if_get_by_index(net_if_get_by_name("wg1"));
	uint8_t private_key[32] = {0x58};
	struct virtual_interface_req_params params = {
		.private_key = {.data = private_key, .len = sizeof(private_key)},
	};
	char keys[4][45];
	char other_key[45];
	int manual_id;

	zassert_not_null(other);
	zassert_ok(net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PRIVATE_KEY, other, &params,
			    sizeof(params)));
	prepare_peer(&other_peer, 10U, 13U, 0xc0000210U, other_key);
	for (size_t i = 0; i < ARRAY_SIZE(set); i++) {
		prepare_peer(&set[i], i + 1U, 9U + i, 0xc0000201U + i, keys[i]);
	}
	zassert_ok(wireguard_replace_peers(fixture->iface, set, ARRAY_SIZE(set), deny_input, NULL));
	fixture->managed = true;
	zexpect_equal(wireguard_replace_peers(other, &other_peer, 1, deny_input, NULL), -ENOSPC);
	zexpect_equal(wireguard_peer_add_to_interface(other, &other_peer.config, NULL), -ENOMEM);
	zassert_ok(wireguard_replace_peers(fixture->iface, set, 3, deny_input, NULL));
	manual_id = wireguard_peer_add_to_interface(other, &other_peer.config, NULL);
	zassert_true(manual_id > 0);
	zexpect_equal(wireguard_replace_peers(fixture->iface, set, ARRAY_SIZE(set), deny_input,
					     NULL), -ENOSPC);
	zexpect_false(has_route(fixture->iface, 0xc0000204U));
	zassert_ok(wireguard_peer_remove(manual_id));
	zassert_ok(wireguard_replace_peers(fixture->iface, set, ARRAY_SIZE(set), deny_input, NULL));
}

static K_SEM_DEFINE(transport_sent, 0, 1);

static int policy_transport_send(int id, const struct wireguard_datagram_path *reply,
				 const uint8_t *data, size_t len, void *context)
{
	int *seen = context;

	*seen = id;
	k_sem_give(&transport_sent);
	return 0;
}

ZTEST_F(wireguard_policy, test_transport_peer_id_publication)
{
	int installed_id = -1, seen = -1, rejected_id = -1;
	struct wireguard_transport transport = {.send = policy_transport_send, .user_data = &seen};
	struct wireguard_policy_peer set[2];
	struct wireguard_datagram_path relay = {.kind = WIREGUARD_PATH_RELAY, .relay_id = 1};
	uint8_t header[4] = {1};
	char keys[2][45];

	prepare_peer(&set[0], 1, 9, 0xc0000201U, keys[0]);
	set[0].transport = &transport;
	set[0].peer_id = &installed_id;
	zassert_ok(wireguard_replace_peers(fixture->iface, set, 1, deny_input, NULL));
	fixture->managed = true;
	zassert_true(installed_id > 0);
	zassert_ok(wireguard_peer_keepalive(installed_id));
	zassert_ok(k_sem_take(&transport_sent, K_SECONDS(2)));
	zassert_equal(seen, installed_id);

	prepare_peer(&set[1], 2, 13, 0xc0000201U, keys[1]);
	set[1].peer_id = &rejected_id;
	zassert_equal(wireguard_replace_peers(fixture->iface, set, 2, deny_input, NULL),
		      -EADDRINUSE);
	zassert_equal(rejected_id, -1);
	zassert_ok(wireguard_replace_peers(fixture->iface, NULL, 0, deny_input, NULL));
	zassert_equal(wireguard_peer_receive_datagram(installed_id, &relay, header, sizeof(header)),
		      -ENOENT);
}

ZTEST_F(wireguard_policy, test_blocked_transport_preserves_peer_state)
{
	const struct wireguard_test_session session = {
		.recv_key = {0xa4},
		.send_key = {0xe3},
		.local_index = 234,
		.remote_index = 567,
	};
	const struct msg_transport_data header = {
		.type = MESSAGE_TRANSPORT_DATA,
		.receiver = session.local_index,
	};
	struct wireguard_datagram_path path = {
		.kind = WIREGUARD_PATH_DIRECT,
		.iface = net_if_get_default(),
	};
	struct wireguard_policy_peer peer;
	struct net_sockaddr_storage endpoint;
	const struct net_sockaddr_in *actual = net_sin(net_sad(&endpoint));
	struct net_sockaddr_in *local = net_sin(net_sad(&path.local));
	struct net_sockaddr_in *remote = net_sin(net_sad(&path.remote));
	uint8_t datagram[sizeof(header) + WG_AUTHTAG_LEN];
	size_t len;
	int id = -1;
	struct wireguard_transport transport = {.send = policy_transport_send, .user_data = &id};
	char key[45];

	*local = (struct net_sockaddr_in){
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(51820),
		.sin_addr = underlay_address,
	};
	*remote = (struct net_sockaddr_in){
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(51821),
		.sin_addr.s_addr = net_htonl(0xc0000205U),
	};
	prepare_peer(&peer, 10U, 9U, 0xc6336401U, key);
	peer.transport = &transport;
	peer.peer_id = &id;
	zassert_ok(wireguard_replace_peers(fixture->iface, &peer, 1U, deny_input, NULL));
	fixture->managed = true;
	zassert_ok(wireguard_test_install_session(id, &session));
	memcpy(datagram, &header, sizeof(header));
	zassert_ok(wireguard_test_encrypt(session.recv_key, 0U, NULL, 0U, datagram + sizeof(header),
					  WG_AUTHTAG_LEN, &len));
	zassert_equal(len, WG_AUTHTAG_LEN);

	wireguard_policy_block(fixture->iface);
	zexpect_equal(wireguard_peer_receive_datagram(id, &path, datagram, sizeof(datagram)),
		      -EACCES);
	zassert_ok(wireguard_test_peer_endpoint(id, &endpoint));
	zexpect_equal(endpoint.ss_family, NET_AF_UNSPEC);

	zassert_ok(wireguard_replace_peers(fixture->iface, &peer, 1U, deny_input, NULL));
	zexpect_ok(wireguard_peer_receive_datagram(id, &path, datagram, sizeof(datagram)));
	zassert_ok(wireguard_test_peer_endpoint(id, &endpoint));
	zassert_equal(endpoint.ss_family, NET_AF_INET);
	zexpect_equal(actual->sin_port, remote->sin_port);
	zexpect_true(net_ipv4_addr_cmp(&actual->sin_addr, &remote->sin_addr));
}

ZTEST_F(wireguard_policy, test_full_capacity_rejection_and_withdrawal)
{
	struct wireguard_policy_peer original[4];
	struct wireguard_policy_peer candidate[4];
	struct wireguard_peer_info info;
	struct admission old_filter = {0};
	struct admission new_filter = {0};
	char keys[4][45];
	char replacement_key[45];
	int ids[4];

	/* An empty policy reserves the interface and retains its provisioned key. */
	zassert_ok(wireguard_replace_peers(fixture->iface, NULL, 0, allow_input, &old_filter));
	fixture->managed = true;
	zassert_true(net_if_is_up(fixture->iface));

	for (size_t i = 0; i < ARRAY_SIZE(original); i++) {
		prepare_peer(&original[i], i + 1U, 9U + i, 0xc0000201U + i, keys[i]);
	}
	zassert_ok(wireguard_replace_peers(fixture->iface, original, ARRAY_SIZE(original),
					 allow_input, &old_filter));
	for (size_t i = 0; i < ARRAY_SIZE(original); i++) {
		ids[i] = peer_id(original[i].identity);
		zassert_true(ids[i] > 0);
	}
	zassert_equal(wireguard_peer_remove(ids[0]), -EBUSY);
	zassert_equal(wireguard_peer_add_to_interface(fixture->iface, &original[0].config, NULL),
		      -EBUSY);

	memcpy(candidate, original, sizeof(candidate));
	/* Two replacements exceed the single spare slot after the first stages. */
	candidate[0].config.allowed_ip[0].addr.in_addr.s_addr = net_htonl(0xc0000210U);
	candidate[2].identity = 30U;
	prepare_peer(&candidate[3], 4U, 13U, 0xc0000204U, replacement_key);
	zassert_equal(wireguard_replace_peers(fixture->iface, candidate, ARRAY_SIZE(candidate),
					     allow_input, &new_filter), -ENOSPC);
	for (size_t i = 0; i < ARRAY_SIZE(original); i++) {
		zexpect_true(has_route(fixture->iface, 0xc0000201U + i));
		zexpect_equal(peer_id(original[i].identity), ids[i]);
	}
	zexpect_false(has_route(fixture->iface, 0xc0000210U));
	zexpect_equal(peer_id(30U), -ENOENT);

	/* Rotate a key at full capacity, then reassign a surviving key. */
	candidate[2].identity = original[2].identity;
	zassert_ok(wireguard_replace_peers(fixture->iface, candidate, ARRAY_SIZE(candidate),
					 allow_input, &new_filter));
	zexpect_equal(wireguard_peer_get_info(ids[3], &info), -ENOENT);
	zexpect_true(peer_id(4U) > ids[3]);
	for (size_t i = 0; i < 3; i++) {
		zexpect_equal(peer_id(original[i].identity), ids[i]);
	}

	candidate[2].identity = 30U;
	zassert_ok(wireguard_replace_peers(fixture->iface, candidate, ARRAY_SIZE(candidate),
					 allow_input, &new_filter));
	zexpect_equal(wireguard_peer_get_info(ids[2], &info), -ENOENT);
	zexpect_equal(peer_id(3U), -ENOENT);
	zexpect_true(peer_id(30U) > ids[2]);
	zexpect_equal(peer_id(1U), ids[0]);
	zexpect_equal(peer_id(2U), ids[1]);

	zassert_ok(wireguard_replace_peers(fixture->iface, candidate, 3, allow_input, &new_filter));
	zexpect_true(has_route(fixture->iface, 0xc0000210U));
	zexpect_false(has_route(fixture->iface, 0xc0000201U));
	zexpect_false(has_route(fixture->iface, 0xc0000204U));
	zexpect_equal(wireguard_peer_get_info(ids[3], &info), -ENOENT);
	zassert_ok(wireguard_peer_get_info(ids[0], &info));
	zexpect_equal(info.allowed_ip[0].addr.in_addr.s_addr, net_htonl(0xc0000210U));

	zassert_ok(wireguard_replace_peers(fixture->iface, NULL, 0, allow_input, &new_filter));
	zexpect_true(net_if_is_up(fixture->iface));
	zexpect_false(has_route(fixture->iface, 0xc0000210U));
	zassert_ok(wireguard_replace_peers(fixture->iface, original, 1, allow_input, &old_filter));
	zexpect_true(peer_id(original[0].identity) > ids[0]);

	zassert_ok(wireguard_release_peers(fixture->iface));
	fixture->managed = false;
	zexpect_false(net_if_is_up(fixture->iface));
	zexpect_equal(wireguard_replace_peers(fixture->iface, original, 1, allow_input,
					     &old_filter), -EINVAL);
}

static int inject(int id, const struct wireguard_test_session *session, uint64_t sequence)
{
	const struct net_sockaddr_in outer = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(51820),
		.sin_addr.s_addr = net_htonl(0xc6336401U),
	};
	const uint8_t packet[28] = {
		0x45, 0, 0, 28, 0, 0, 0, 0, 64, NET_IPPROTO_UDP, 0, 0,
		192, 0, 2, 1, 192, 0, 2, 2, 0x41, 0x42, 0x43, 0x44, 0, 8, 0, 0,
	};
	uint8_t ciphertext[48];
	size_t len;

	zassert_ok(wireguard_test_encrypt(session->recv_key, sequence, packet, sizeof(packet),
					  ciphertext, sizeof(ciphertext), &len));
	return wireguard_test_inject_transport(id, (const struct net_sockaddr *)&outer, sequence,
					       ciphertext, len);
}

ZTEST_F(wireguard_policy, test_policy_and_cryptographic_staging)
{
	struct wireguard_policy_peer peers[3];
	struct admission original = {.allow = true};
	struct admission denied = {0};
	struct net_sockaddr_storage endpoint;
	const struct wireguard_test_session session = {
		.recv_key = {0xa4},
		.send_key = {0xe3},
		.local_index = 234,
		.remote_index = 567,
	};
	char keys[3][45];
	int id;

	prepare_peer(&peers[0], 10, 9U, 0xc0000201U, keys[0]);
	prepare_peer(&peers[1], 20, 10U, 0xc0000202U, keys[1]);
	prepare_peer(&peers[2], 30, 0U, 0xc0000203U, keys[2]);
	zassert_ok(wireguard_replace_peers(fixture->iface, peers, 1, allow_input, &original));
	fixture->managed = true;

	id = peer_id(10);
	zassert_true(id > 0);
	zassert_ok(wireguard_test_install_session(id, &session));
	zassert_ok(inject(id, &session, 7));
	zexpect_equal(original.calls, 1);
	zexpect_equal(original.identity, 10);
	zexpect_equal(original.len, 28);

	/* The final low-order public key fails DH after the preceding peer staged. */
	zassert_true(wireguard_replace_peers(fixture->iface, peers, ARRAY_SIZE(peers),
					     allow_input, &denied) < 0);
	zexpect_equal(peer_id(20), -ENOENT);
	zexpect_false(has_route(fixture->iface, 0xc0000202U));
	zassert_ok(inject(id, &session, 8));
	zexpect_equal(original.calls, 2);
	zexpect_equal(denied.calls, 0);

	/* A canonical duplicate prefix also leaves membership and the filter intact. */
	peers[1].config.allowed_ip[0].addr = peers[0].config.allowed_ip[0].addr;
	zassert_equal(wireguard_replace_peers(fixture->iface, peers, 2, allow_input, &denied),
		      -EADDRINUSE);
	peers[1].config.allowed_ip[0].addr.in_addr.s_addr = net_htonl(0xc0000202U);

	zassert_ok(wireguard_replace_peers(fixture->iface, peers, 2, allow_input, &denied));
	zexpect_equal(peer_id(10), id);
	zassert_ok(wireguard_test_peer_endpoint(id, &endpoint));
	zexpect_equal(net_sin(net_sad(&endpoint))->sin_addr.s_addr, net_htonl(0xc6336401U));
	zexpect_equal(inject(id, &session, 8), -EINVAL);
	zexpect_equal(inject(id, &session, 9), -EACCES);
	zexpect_equal(original.calls, 2);
	zexpect_equal(denied.calls, 1);
	zexpect_equal(denied.identity, 10);
}

static struct net_pkt *make_inner(struct net_if *iface, uint32_t address)
{
	const struct net_in_addr source = {.s_addr = net_htonl(0xc6336401U)};
	const struct net_in_addr destination = {.s_addr = net_htonl(address)};
	struct net_pkt *pkt = net_pkt_alloc_with_buffer(iface, 1, NET_AF_INET,
							 NET_IPPROTO_UDP, K_NO_WAIT);

	zassert_not_null(pkt);
	net_pkt_set_ipv4_ttl(pkt, 64);
	zassert_ok(net_ipv4_create(pkt, &source, &destination));
	zassert_ok(net_udp_create(pkt, net_htons(5900), net_htons(5901)));
	zassert_ok(net_pkt_write_u8(pkt, 1));
	net_pkt_cursor_init(pkt);
	zassert_ok(net_ipv4_finalize(pkt, NET_IPPROTO_UDP));
	return pkt;
}

static void queue_inner(struct net_if *iface, uint32_t address)
{
	struct net_pkt *pkt = make_inner(iface, address);

	zassert_ok(net_if_l2(iface)->send(iface, pkt));
}

static void setup_underlay(struct wireguard_policy_fixture *fixture)
{
	const struct net_in_addr netmask = {.s_addr = net_htonl(0xffffff00U)};

	fixture->underlay = net_if_lookup_by_dev(DEVICE_GET(wg_policy_underlay));
	zassert_not_null(fixture->underlay);
	net_if_flag_set(fixture->underlay, NET_IF_IPV4);
	zassert_not_null(net_if_ipv4_addr_add(fixture->underlay, &underlay_address,
					     NET_ADDR_MANUAL, 0));
	zassert_true(net_if_ipv4_set_netmask_by_addr(fixture->underlay, &underlay_address,
						    &netmask));
	zassert_ok(net_if_up(fixture->underlay));
}

ZTEST_F(wireguard_policy, test_native_ciphertext_stays_on_the_underlay)
{
	const struct wireguard_test_session session = {
		.recv_key = {0xa4},
		.send_key = {0xe3},
		.local_index = 234,
		.remote_index = 567,
	};
	const struct net_in_addr netmask = {.s_addr = net_htonl(0xffffff00U)};
	const uint32_t destination = 0xc6336403U;
	struct wireguard_policy_peer peer;
	struct net_sockaddr_in *endpoint = net_sin(net_sad(&peer.config.endpoint_ip));
	char key[45];
	int id = -1;

	setup_underlay(fixture);
	prepare_peer(&peer, 10U, 9U, destination, key);
	peer.underlay = fixture->underlay;
	peer.peer_id = &id;
	endpoint->sin_family = NET_AF_INET;
	endpoint->sin_addr.s_addr = net_htonl(0xc6336405U);
	endpoint->sin_port = net_htons(51821);
	zassert_not_null(net_if_ipv4_addr_add(fixture->iface, &inner_address, NET_ADDR_MANUAL, 0));
	fixture->overlay_address = true;
	zassert_true(net_if_ipv4_set_netmask_by_addr(fixture->iface, &inner_address, &netmask));
	zassert_ok(wireguard_replace_peers(fixture->iface, &peer, 1U, deny_input, NULL));
	fixture->managed = true;
	zassert_ok(wireguard_test_install_session(id, &session));
	zassert_equal_ptr(net_if_ipv4_select_src_iface(&endpoint->sin_addr), fixture->iface);

	k_sem_reset(&native_sent);
	queue_inner(fixture->iface, destination);
	zassert_ok(k_sem_take(&native_sent, K_SECONDS(1)));
	zexpect_equal(native_source.s_addr, underlay_address.s_addr);
	zexpect_equal(native_destination.s_addr, endpoint->sin_addr.s_addr);
}

ZTEST_F(wireguard_policy, test_native_ciphertext_rechecks_survivor_route)
{
	const struct wireguard_test_session session = {
		.recv_key = {0xa4},
		.send_key = {0xe3},
		.local_index = 234,
		.remote_index = 567,
	};
	struct wireguard_policy_peer peer;
	struct net_sockaddr_in *endpoint;
	struct admission admission = {0};
	char key[45];
	int id;

	setup_underlay(fixture);
	prepare_peer(&peer, 40, 9U, 0xc6336400U, key);
	peer.config.allowed_ip[0].mask_len = 24U;
	endpoint = net_sin(net_sad(&peer.config.endpoint_ip));
	endpoint->sin_family = NET_AF_INET;
	endpoint->sin_addr.s_addr = net_htonl(0xc0000205U);
	endpoint->sin_port = net_htons(51820);
	zassert_ok(wireguard_replace_peers(fixture->iface, &peer, 1, allow_input, &admission));
	fixture->managed = true;
	id = peer_id(40);
	zassert_true(id > 0);
	zassert_ok(wireguard_test_install_session(id, &session));

	k_sem_reset(&native_sent);
	k_sem_reset(&work_entered);
	k_sem_reset(&work_release);
	fixture->work_blocked = true;
	zassert_true(k_work_submit(&tx_blocker) >= 0);
	zassert_ok(k_sem_take(&work_entered, K_SECONDS(1)));

	queue_inner(fixture->iface, 0xc6336402U);
	queue_inner(fixture->iface, 0xc6336403U);
	peer.config.allowed_ip[0].addr.in_addr.s_addr = net_htonl(0xc6336403U);
	peer.config.allowed_ip[0].mask_len = 32U;
	zassert_ok(wireguard_replace_peers(fixture->iface, &peer, 1, allow_input, &admission));
	zexpect_equal(peer_id(40), id);
	k_sem_give(&work_release);

	zassert_ok(k_sem_take(&native_sent, K_SECONDS(1)));
	zexpect_equal(k_sem_take(&native_sent, K_MSEC(100)), -EAGAIN);
}

ZTEST_F(wireguard_policy, test_deadline_blocks_ingress_and_queued_egress_before_cleanup)
{
	struct wireguard_test_session session = {
		.recv_key = {0xa4},
		.send_key = {0xe3},
		.local_index = 234,
		.remote_index = 567,
	};
	struct wireguard_policy_peer peers[2];
	struct wireguard_test_keypair_state survivor;
	struct wireguard_peer_info info;
	struct admission admission = {.allow = true};
	char keys[2][45];
	int ids[2];

	setup_underlay(fixture);
	for (size_t i = 0; i < ARRAY_SIZE(peers); i++) {
		prepare_peer(&peers[i], 10U + i, 9U + i, 0xc0000201U + i, keys[i]);
		struct net_sockaddr_in *endpoint = net_sin(net_sad(&peers[i].config.endpoint_ip));

		endpoint->sin_family = NET_AF_INET;
		endpoint->sin_addr.s_addr = net_htonl(0xc0000205U + i);
		endpoint->sin_port = net_htons(51820);
	}
	peers[0].config.allowed_ip[1] = peers[0].config.allowed_ip[0];
	peers[0].config.allowed_ip[1].addr.in_addr.s_addr = net_htonl(0xc6336402U);
	peers[1].config.allowed_ip[0].addr.in_addr.s_addr = net_htonl(0xc0000200U);
	peers[1].config.allowed_ip[0].mask_len = 24U;
	peers[0].expires_at_ms = k_uptime_get() + 300;
	zassert_ok(wireguard_replace_peers(fixture->iface, peers, ARRAY_SIZE(peers),
					 allow_input, &admission));
	fixture->managed = true;
	for (size_t i = 0; i < ARRAY_SIZE(peers); i++) {
		ids[i] = peer_id(peers[i].identity);
		zassert_true(ids[i] > 0);
		zassert_ok(wireguard_test_install_session(ids[i], &session));
		session.local_index++;
		session.remote_index++;
	}
	zassert_true(has_route(fixture->iface, 0xc0000201U));

	k_sem_reset(&native_sent);
	k_sem_reset(&work_entered);
	k_sem_reset(&work_release);
	fixture->work_blocked = true;
	zassert_true(k_work_submit(&tx_blocker) >= 0);
	zassert_ok(k_sem_take(&work_entered, K_SECONDS(1)));
	queue_inner(fixture->iface, 0xc0000201U);
	queue_inner(fixture->iface, 0xc0000202U);
	zassert_ok(inject(ids[0], &session, 1U));
	zassert_equal(admission.calls, 1U);
	k_sleep(K_TIMEOUT_ABS_MS(peers[0].expires_at_ms));

	/* No replacement or timer cleanup has run. The installed peer is inert. */
	zassert_ok(wireguard_peer_get_info(ids[0], &info));
	zassert_false(has_route(fixture->iface, 0xc6336402U));
	zassert_true(has_route(fixture->iface, 0xc0000201U));
	zassert_true(has_route(fixture->iface, 0xc0000202U));
	zassert_true(inject(ids[0], &session, 2U) < 0);
	zassert_equal(admission.calls, 1U);

	/* Plaintext queued for the old owner must not move to the fallback. */
	struct net_pkt *old_destination = make_inner(fixture->iface, 0xc0000201U);

	net_pkt_set_vpn_peer_id(old_destination, ids[0]);
	zassert_equal(net_if_l2(fixture->iface)->send(fixture->iface, old_destination), -ESTALE);
	net_pkt_unref(old_destination);

	k_sem_give(&work_release);
	zassert_ok(k_sem_take(&native_sent, K_SECONDS(1)));
	zassert_equal(native_destination.s_addr, net_htonl(0xc0000206U));
	zassert_equal(k_sem_take(&native_sent, K_MSEC(100)), -EAGAIN);

	/* A fresh selection can use the surviving owner's broader prefix. */
	queue_inner(fixture->iface, 0xc0000201U);
	zassert_ok(k_sem_take(&native_sent, K_SECONDS(1)));
	zassert_equal(native_destination.s_addr, net_htonl(0xc0000206U));

	zassert_ok(wireguard_replace_peers(fixture->iface, &peers[1], 1, allow_input, &admission));
	zassert_equal(wireguard_peer_get_info(ids[0], &info), -ENOENT);
	zassert_equal(peer_id(peers[1].identity), ids[1]);
	zassert_ok(wireguard_test_peer_keypair_state(ids[1], &survivor));
	zassert_true(survivor.current_valid);
}

static enum net_verdict block_delivery(struct net_conn *conn, struct net_pkt *pkt,
				       union net_ip_header *ip, union net_proto_header *proto,
				       void *context)
{
	struct wireguard_policy_fixture *fixture = context;

	wireguard_policy_block(fixture->iface);
	fixture->reentrant_result = wireguard_replace_peers(fixture->iface, NULL, 0, deny_input,
							    NULL);
	k_sem_give(&rx_entered);
	(void)k_sem_take(&rx_release, K_SECONDS(5));
	net_pkt_unref(pkt);
	return NET_OK;
}

static void send_native_payload(struct wireguard_policy_fixture *fixture,
				const void *header, size_t header_len,
				const void *data, size_t data_len)
{
	const struct net_in_addr outer_source = {.s_addr = net_htonl(0xc0000205U)};
	struct net_pkt *pkt;

	pkt = net_pkt_rx_alloc_with_buffer(fixture->underlay, header_len + data_len, NET_AF_INET,
					   NET_IPPROTO_UDP, K_NO_WAIT);
	zassert_not_null(pkt);
	net_pkt_set_ipv4_ttl(pkt, 64);
	zassert_ok(net_ipv4_create(pkt, &outer_source, &underlay_address));
	zassert_ok(net_udp_create(pkt, net_htons(51821), net_htons(51820)));
	zassert_ok(net_pkt_write(pkt, header, header_len));
	if (data_len > 0U) {
		zassert_ok(net_pkt_write(pkt, data, data_len));
	}
	net_pkt_cursor_init(pkt);
	zassert_ok(net_ipv4_finalize(pkt, NET_IPPROTO_UDP));
	zassert_ok(net_recv_data(fixture->underlay, pkt));
}

static void send_native_rx(struct wireguard_policy_fixture *fixture,
			    const struct wireguard_test_session *session, uint64_t counter)
{
	const struct net_in_addr inner_source = {.s_addr = net_htonl(0xc6336401U)};
	struct msg_transport_data header = {
		.type = MESSAGE_TRANSPORT_DATA,
		.receiver = session->local_index,
	};
	struct net_pkt *pkt;
	uint8_t plaintext[29];
	uint8_t ciphertext[48];
	size_t len;

	sys_put_le64(counter, (uint8_t *)&header.counter);
	pkt = net_pkt_alloc_with_buffer(fixture->iface, 1, NET_AF_INET, NET_IPPROTO_UDP,
					K_NO_WAIT);
	zassert_not_null(pkt);
	net_pkt_set_ipv4_ttl(pkt, 64);
	zassert_ok(net_ipv4_create(pkt, &inner_source, &inner_address));
	zassert_ok(net_udp_create(pkt, net_htons(5900), net_htons(5901)));
	zassert_ok(net_pkt_write_u8(pkt, (uint8_t)counter));
	net_pkt_cursor_init(pkt);
	zassert_ok(net_ipv4_finalize(pkt, NET_IPPROTO_UDP));
	net_pkt_cursor_init(pkt);
	zassert_ok(net_pkt_read(pkt, plaintext, sizeof(plaintext)));
	net_pkt_unref(pkt);

	zassert_ok(wireguard_test_encrypt(session->recv_key, counter, plaintext, sizeof(plaintext),
					  ciphertext, sizeof(ciphertext), &len));
	send_native_payload(fixture, &header, sizeof(header), ciphertext, len);
}

static enum net_verdict hold_plaintext(struct net_conn *conn, struct net_pkt *pkt,
				       union net_ip_header *ip, union net_proto_header *proto,
				       void *context)
{
	struct wireguard_policy_fixture *fixture = context;

	if (fixture->retained_count == ARRAY_SIZE(fixture->retained)) {
		return NET_DROP;
	}
	fixture->retained[fixture->retained_count++] = pkt;
	k_sem_give(&retained_packet);
	return NET_OK;
}

#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
ZTEST_F(wireguard_policy, test_native_handshake_isolates_traffic_and_policy_cutover)
{
	const struct msg_handshake_init handshake = {
		.type = MESSAGE_HANDSHAKE_INITIATION,
		.sender = sys_cpu_to_le32(0x123U),
		.ephemeral = {9U},
		/* MAC1 for provision()'s local public key. */
		.mac1 = {0xe0, 0x9e, 0x5c, 0x61, 0xcb, 0x4a, 0x77, 0xfa,
			 0x4d, 0xa5, 0x7e, 0xa8, 0x5e, 0x02, 0xa9, 0x04},
	};
	const struct wireguard_test_session session = {
		.recv_key = {0xa4},
		.send_key = {0xe3},
		.local_index = 234,
		.remote_index = 567,
	};
	const uint32_t destination = 0xc6336401U;
	struct admission admission = {.allow = true};
	struct wireguard_policy_peer peer;
	struct net_sockaddr_in *endpoint;
	struct net_pkt *packet;
	char key[45];
	int ret;

	setup_underlay(fixture);
	zassert_not_null(net_if_ipv4_addr_add(fixture->iface, &inner_address, NET_ADDR_MANUAL, 0));
	zassert_ok(net_udp_register(NET_AF_INET, NULL, NULL, 0, 5901, NULL, hold_plaintext,
				    fixture, &fixture->listener));
	prepare_peer(&peer, 10U, 9U, destination, key);
	endpoint = net_sin(net_sad(&peer.config.endpoint_ip));
	endpoint->sin_family = NET_AF_INET;
	endpoint->sin_addr.s_addr = net_htonl(0xc0000205U);
	endpoint->sin_port = net_htons(51821);
	zassert_ok(wireguard_replace_peers(fixture->iface, &peer, 1U, allow_input, &admission));
	fixture->managed = true;
	zassert_ok(wireguard_test_install_session(peer_id(10U), &session));
	k_sem_reset(&native_sent);
	k_sem_reset(&retained_packet);
	k_sem_reset(&handshake_entered);
	k_sem_reset(&handshake_release);
	k_sem_reset(&revoked);
	atomic_clear(&raw_agreements);
	fixture->handshake_blocked = true;
	atomic_set(&pause_handshake, 1);
	send_native_payload(fixture, &handshake, sizeof(handshake), NULL, 0);
	zassert_ok(k_sem_take(&handshake_entered, K_SECONDS(1)));

	send_native_rx(fixture, &session, 0);
	zassert_ok(k_sem_take(&retained_packet, K_SECONDS(1)));
	packet = make_inner(fixture->iface, destination);
	ret = net_if_l2(fixture->iface)->send(fixture->iface, packet);
	if (ret != 0) {
		net_pkt_unref(packet);
	}
	zassert_ok(ret);
	zassert_ok(k_sem_take(&native_sent, K_SECONDS(1)));
	zassert_equal(native_destination.s_addr, endpoint->sin_addr.s_addr);

	/* The second initiation is queued before the policy withdraws its peer. */
	send_native_payload(fixture, &handshake, sizeof(handshake), NULL, 0);
	send_native_rx(fixture, &session, 1U);
	zassert_ok(k_sem_take(&retained_packet, K_SECONDS(1)));
	fixture->revoke_pending = true;
	zassert_true(k_work_submit(&fixture->revoke_work) >= 0);
	for (int i = 0; i < 100 && peer_id(10U) != -ENOENT; i++) {
		k_sleep(K_MSEC(1));
	}
	zassert_equal(peer_id(10U), -ENOENT);
	zassert_equal(k_sem_take(&revoked, K_NO_WAIT), -EBUSY);
	k_sem_give(&handshake_release);
	fixture->handshake_blocked = false;
	zassert_ok(k_sem_take(&revoked, K_SECONDS(1)));
	zassert_ok(fixture->revoke_result);

	/* Staging on the same queue follows the stale datagram. */
	zassert_ok(wireguard_replace_peers(fixture->iface, &peer, 1U, allow_input, &admission));
	zassert_equal(atomic_get(&raw_agreements), 2, "retired datagram reached X25519");
}
#endif

ZTEST_F(wireguard_policy, test_retained_plaintext_does_not_block_unrelated_tx)
{
	const struct wireguard_test_session rx = {
		.recv_key = {0xa4},
		.send_key = {0xe3},
		.local_index = 234,
		.remote_index = 567,
	};
	const struct wireguard_test_session tx = {
		.recv_key = {0xe7},
		.send_key = {0x41},
		.local_index = 235,
		.remote_index = 568,
	};
	const uint32_t tx_destination = 0xc6336403U;
	struct wireguard_policy_peer peers[2];
	struct net_sockaddr_in *endpoint;
	struct admission admission = {.allow = true};
	char keys[2][45];

	setup_underlay(fixture);
	zassert_not_null(net_if_ipv4_addr_add(fixture->iface, &inner_address, NET_ADDR_MANUAL, 0));
	zassert_ok(net_udp_register(NET_AF_INET, NULL, NULL, 0, 5901, NULL, hold_plaintext,
				    fixture, &fixture->listener));
	prepare_peer(&peers[0], 10, 9U, 0xc6336401U, keys[0]);
	prepare_peer(&peers[1], 20, 10U, tx_destination, keys[1]);
	endpoint = net_sin(net_sad(&peers[1].config.endpoint_ip));
	endpoint->sin_family = NET_AF_INET;
	endpoint->sin_addr.s_addr = net_htonl(0xc0000206U);
	endpoint->sin_port = net_htons(51820);
	zassert_ok(wireguard_replace_peers(fixture->iface, peers, ARRAY_SIZE(peers),
					 allow_input, &admission));
	fixture->managed = true;
	zassert_ok(wireguard_test_install_session(peer_id(10), &rx));
	zassert_ok(wireguard_test_install_session(peer_id(20), &tx));

	k_sem_reset(&native_sent);
	k_sem_reset(&retained_packet);
	for (size_t i = 0; i < ARRAY_SIZE(fixture->retained); i++) {
		send_native_rx(fixture, &rx, i);
		zassert_ok(k_sem_take(&retained_packet, K_SECONDS(1)), "retaining packet %zu", i);
	}

	/* Saturated RX must neither delay the WG mutex nor deny the other peer's TX. */
	int64_t started = k_uptime_get();
	struct net_pkt *packet = make_inner(fixture->iface, tx_destination);
	int rc;

	send_native_rx(fixture, &rx, ARRAY_SIZE(fixture->retained));
	rc = net_if_l2(fixture->iface)->send(fixture->iface, packet);
	if (rc != 0) {
		net_pkt_unref(packet);
	}
	zassert_ok(rc, "TX failed with %zu retained RX packets", fixture->retained_count);
	zassert_ok(k_sem_take(&native_sent, K_MSEC(50)), "ciphertext did not reach the underlay");
	zassert_true(k_uptime_get() - started < 100, "full RX delayed the transmit path");
	zassert_equal(native_destination.s_addr, net_htonl(0xc0000206U));
	zassert_equal(admission.calls, ARRAY_SIZE(fixture->retained));

	for (size_t i = 0; i < fixture->retained_count; i++) {
		uint8_t nonce;

		net_pkt_cursor_init(fixture->retained[i]);
		zassert_ok(net_pkt_skip(fixture->retained[i], 28U));
		zassert_ok(net_pkt_read_u8(fixture->retained[i], &nonce));
		zassert_equal(nonce, i);
	}
	net_pkt_unref(fixture->retained[--fixture->retained_count]);
	send_native_rx(fixture, &rx, ARRAY_SIZE(fixture->retained) + 1U);
	zassert_ok(k_sem_take(&retained_packet, K_SECONDS(1)));
}

ZTEST_F(wireguard_policy, test_replacement_waits_for_admitted_delivery)
{
	const struct wireguard_test_session session = {
		.recv_key = {0xa4},
		.send_key = {0xe3},
		.local_index = 234,
		.remote_index = 567,
	};
	struct wireguard_policy_peer peer;
	struct admission admission = {.allow = true};
	uint32_t remote = 0xc6336401U;
	char key[45];
	int id;

	setup_underlay(fixture);
	zassert_not_null(net_if_ipv4_addr_add(fixture->iface, &inner_address, NET_ADDR_MANUAL, 0));
	zassert_ok(net_udp_register(NET_AF_INET, NULL, NULL, 0, 5901, NULL, block_delivery,
				    fixture, &fixture->listener));
	prepare_peer(&peer, 40, 9U, remote, key);
	zassert_ok(wireguard_replace_peers(fixture->iface, &peer, 1, allow_input, &admission));
	fixture->managed = true;
	id = peer_id(40);
	zassert_true(id > 0);
	zassert_ok(wireguard_test_install_session(id, &session));
	zassert_true(has_route(fixture->iface, remote));

	k_sem_reset(&rx_entered);
	k_sem_reset(&rx_release);
	k_sem_reset(&revoked);
	fixture->rx_blocked = true;
	send_native_rx(fixture, &session, 0U);
	zassert_ok(k_sem_take(&rx_entered, K_SECONDS(1)));
	zexpect_equal(fixture->reentrant_result, -EBUSY);
	zassert_equal(peer_id(40), id);
	zassert_false(has_route(fixture->iface, remote));

	struct net_pkt *packet = make_inner(fixture->iface, remote);

	zassert_equal(net_if_l2(fixture->iface)->send(fixture->iface, packet), -ENETUNREACH);
	net_pkt_unref(packet);

	zassert_true(k_work_submit(&fixture->revoke_work) >= 0);
	for (int i = 0; i < 100 && peer_id(40) != -ENOENT; i++) {
		k_sleep(K_MSEC(1));
	}
	zassert_equal(peer_id(40), -ENOENT);
	zexpect_false(has_route(fixture->iface, remote));
	zexpect_equal(k_sem_take(&revoked, K_NO_WAIT), -EBUSY);
	k_sem_give(&rx_release);
	zassert_ok(k_sem_take(&revoked, K_SECONDS(1)));
	zexpect_ok(fixture->revoke_result);
	zexpect_equal(peer_id(40), -ENOENT);
	zassert_ok(wireguard_replace_peers(fixture->iface, &peer, 1, allow_input, &admission));
	zassert_true(has_route(fixture->iface, remote));
}

ZTEST_SUITE(wireguard_policy, NULL, setup, provision, teardown, NULL);

#endif /* CONFIG_WIREGUARD_PEER_POLICY */
