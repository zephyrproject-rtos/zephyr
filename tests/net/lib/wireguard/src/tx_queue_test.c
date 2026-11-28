/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/net/dummy.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/virtual_mgmt.h>
#include "wg_private.h"
#include <zephyr/sys/base64.h>
#include <zephyr/ztest.h>

#include "ipv4.h"
#include "udp_internal.h"
#include "wg_test_helpers.h"

struct wireguard_tx_queue_fixture {
	int peers[3];
	struct net_if *gate;
	bool gate_queued;
	bool gate_address;
};

static void *queue_setup(void)
{
	static struct wireguard_tx_queue_fixture fixture;

	return &fixture;
}

#if defined(CONFIG_WIREGUARD_MULTI_PEER)

static struct net_in_addr underlay;
K_SEM_DEFINE(native_datagram, 0, 1);
K_SEM_DEFINE(gate_entered, 0, 1);
K_SEM_DEFINE(gate_release, 0, 1);
K_SEM_DEFINE(gate_exited, 0, 1);
K_MSGQ_DEFINE(transport_sends, sizeof(int), 3, sizeof(int));

static void gate_init(struct net_if *iface)
{
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	net_if_flag_set(iface, NET_IF_POINTOPOINT);
	net_if_flag_clear(iface, NET_IF_IPV4);
	net_if_flag_clear(iface, NET_IF_IPV6);
}

static int gate_send(const struct device *dev, struct net_pkt *pkt)
{
	int ret;

	ARG_UNUSED(dev);
	if (net_pkt_get_len(pkt) > 1U) {
		k_sem_give(&native_datagram);
		return 0;
	}
	k_sem_give(&gate_entered);
	ret = k_sem_take(&gate_release, K_SECONDS(5));
	/* Dummy L2 releases the driver packet on success. */
	k_sem_give(&gate_exited);
	return ret;
}

static DEVICE_API(dummy, gate_api) = {
	.iface_api.init = gate_init,
	.send = gate_send,
};

NET_DEVICE_INIT(wg_tx_gate, "wg_tx_gate", NULL, NULL, NULL, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &gate_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 1500);

static void queue_cleanup(void *data)
{
	struct wireguard_tx_queue_fixture *fixture = data;

	if (fixture->gate_queued) {
		k_sem_give(&gate_release);
		zexpect_ok(k_sem_take(&gate_exited, K_SECONDS(1)));
		fixture->gate_queued = false;
	}
	ARRAY_FOR_EACH(fixture->peers, i) {
		if (fixture->peers[i] > 0) {
			zexpect_ok(wireguard_peer_remove(fixture->peers[i]));
			fixture->peers[i] = 0;
		}
	}
	if (fixture->gate_address) {
		zexpect_true(net_if_ipv4_addr_rm(fixture->gate, &underlay));
		net_if_flag_clear(fixture->gate, NET_IF_IPV4);
		fixture->gate_address = false;
	}
	if (fixture->gate != NULL) {
		zexpect_ok(net_if_down(fixture->gate));
		fixture->gate = NULL;
	}
}

static int capture_send(int peer_id, const struct wireguard_datagram_path *path,
			 const uint8_t *data, size_t len, void *user_data)
{
	ARG_UNUSED(path);
	ARG_UNUSED(user_data);
	if (len < 4U || data[0] != 4U) {
		return -EINVAL;
	}
	return k_msgq_put(&transport_sends, &peer_id, K_NO_WAIT);
}

static void provision(struct net_if *iface)
{
	uint8_t key[32] = {0x73U};
	struct virtual_interface_req_params params = {0};

	params.private_key.data = key;
	params.private_key.len = sizeof(key);
	int ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PRIVATE_KEY, iface, &params,
			   sizeof(params));

	zassert_ok(ret, "setting private key returned %d, interface up %d", ret,
		   net_if_is_up(iface));
}

static int add_peer(struct net_if *iface, const struct net_in_addr *allowed, uint8_t key)
{
	const struct wireguard_transport transport = {.send = capture_send};
	struct wireguard_peer_config config = {0};
	struct wireguard_test_session session = {.local_index = key, .remote_index = key + 100U};
	uint8_t public_key[32] = {key};
	char encoded[45];
	size_t encoded_len;
	int id;

	zassert_ok(base64_encode(encoded, sizeof(encoded), &encoded_len,
				 public_key, sizeof(public_key)));
	config.public_key = encoded;
	config.allowed_ip[0].is_valid = true;
	config.allowed_ip[0].addr.family = NET_AF_INET;
	config.allowed_ip[0].addr.in_addr = *allowed;
	config.allowed_ip[0].mask_len = 32U;
	id = wireguard_peer_add_to_interface(iface, &config, &transport);
	zassert_true(id > 0, "add peer returned %d", id);
	memset(session.send_key, key, sizeof(session.send_key));
	zassert_ok(wireguard_test_install_session(id, &session));
	return id;
}

static struct net_pkt *udp_packet(struct net_if *iface, const struct net_in_addr *destination)
{
	struct net_in_addr source = {.s_addr = net_htonl(0xC0000201U)};
	struct net_pkt *pkt = net_pkt_alloc_with_buffer(iface, 1U, NET_AF_INET,
							NET_IPPROTO_UDP, K_NO_WAIT);

	zassert_not_null(pkt);
	net_pkt_set_ipv4_ttl(pkt, 64);
	zassert_ok(net_ipv4_create(pkt, &source, destination));
	zassert_ok(net_udp_create(pkt, net_htons(46000), net_htons(46001)));
	zassert_ok(net_pkt_write_u8(pkt, 0x73));
	net_pkt_cursor_init(pkt);
	zassert_ok(net_ipv4_finalize(pkt, NET_IPPROTO_UDP));
	return pkt;
}

struct tx_completion {
	int status;
	bool link;
};

K_MSGQ_DEFINE(tx_completions, sizeof(struct tx_completion), 4, sizeof(int));
static struct net_if_link_cb link_cb;
static struct net_if *completion_iface;

static void context_complete(struct net_context *ctx, int status, void *user_data)
{
	struct tx_completion completion = {.status = status};

	(void)k_msgq_put(&tx_completions, &completion, K_NO_WAIT);
}

static void link_complete(struct net_if *iface, struct net_linkaddr *dst, int status)
{
	struct tx_completion completion = {.status = status, .link = true};

	if (iface == completion_iface) {
		(void)k_msgq_put(&tx_completions, &completion, K_NO_WAIT);
	}
}

static __maybe_unused void block_tx_queue(struct wireguard_tx_queue_fixture *fixture)
{
	struct net_pkt *blocker;

	fixture->gate = net_if_lookup_by_dev(DEVICE_GET(wg_tx_gate));
	zassert_not_null(fixture->gate);
	if (!net_if_is_up(fixture->gate)) {
		zassert_ok(net_if_up(fixture->gate));
	}
	blocker = net_pkt_alloc_with_buffer(fixture->gate, 1U, NET_AF_UNSPEC, 0, K_NO_WAIT);
	zassert_not_null(blocker);
	zassert_ok(net_pkt_write_u8(blocker, 0));
	fixture->gate_queued = true;
	net_if_try_queue_tx(fixture->gate, blocker, K_NO_WAIT);
	zassert_ok(k_sem_take(&gate_entered, K_SECONDS(1)));
}

#else

static void queue_cleanup(void *data)
{
	ARG_UNUSED(data);
}

#endif

ZTEST_F(wireguard_tx_queue, test_replaced_peer_drops_queued_plaintext)
{
#if defined(CONFIG_WIREGUARD_MULTI_PEER) && CONFIG_NET_TC_TX_COUNT == 1
	struct net_in_addr replaced = {.s_addr = net_htonl(0xC0000202U)};
	struct net_in_addr survivor = {.s_addr = net_htonl(0xC0000203U)};
	struct net_if *iface = net_if_get_by_index(net_if_get_by_name("wg0"));
	struct net_pkt *original;
	struct net_pkt *clone;
	int old_id;
	int observed;

	zassert_not_null(iface);
	provision(iface);
	fixture->peers[0] = add_peer(iface, &replaced, 9U);
	fixture->peers[1] = add_peer(iface, &survivor, 10U);
	old_id = fixture->peers[0];
	block_tx_queue(fixture);

	net_if_try_queue_tx(iface, udp_packet(iface, &replaced), K_NO_WAIT);
	original = udp_packet(iface, &survivor);
	clone = net_pkt_clone(original, K_NO_WAIT);
	zassert_not_null(clone);
	net_pkt_unref(original);
	net_pkt_set_vpn_peer_id(clone, old_id);
	net_if_try_queue_tx(iface, clone, K_NO_WAIT);

	zassert_ok(wireguard_peer_remove(old_id));
	fixture->peers[0] = 0;
	fixture->peers[2] = add_peer(iface, &replaced, 9U);
	zassert_not_equal(fixture->peers[2], old_id);
	net_if_try_queue_tx(iface, udp_packet(iface, &replaced), K_NO_WAIT);

	k_sem_give(&gate_release);
	zassert_ok(k_sem_take(&gate_exited, K_SECONDS(1)));
	fixture->gate_queued = false;
	zassert_ok(k_msgq_get(&transport_sends, &observed, K_SECONDS(1)));
	zassert_equal(observed, fixture->peers[1], "stale datagram reached replacement");
	zassert_ok(k_msgq_get(&transport_sends, &observed, K_SECONDS(1)));
	zassert_equal(observed, fixture->peers[2], "fresh datagram missed replacement");
	zassert_equal(k_msgq_num_used_get(&transport_sends), 0);
#else
	ztest_test_skip();
#endif
}

ZTEST_F(wireguard_tx_queue, test_native_send_with_congested_traffic_class)
{
#if defined(CONFIG_WIREGUARD_MULTI_PEER) && CONFIG_NET_TC_TX_COUNT == 2
	struct net_if *iface = net_if_get_by_index(net_if_get_by_name("wg0"));
	struct net_in_addr allowed = {.s_addr = net_htonl(0xc6336402U)};
	struct wireguard_peer_config config = {0};
	struct net_sockaddr_in *endpoint = net_sin(net_sad(&config.endpoint_ip));
	struct wireguard_test_session session = {.local_index = 123, .remote_index = 321};
	struct net_in_addr mask = {.s_addr = net_htonl(0xffffff00U)};
	uint8_t key[32] = {11};
	char encoded[45];
	size_t len;
	int observed;
	net_stats_t dropped;

	underlay.s_addr = net_htonl(0xc0000201U);
	provision(iface);
	fixture->peers[0] = add_peer(iface, &allowed, 9);
	fixture->gate = net_if_lookup_by_dev(DEVICE_GET(wg_tx_gate));
	net_if_flag_set(fixture->gate, NET_IF_IPV4);
	zassert_not_null(net_if_ipv4_addr_add(fixture->gate, &underlay, NET_ADDR_MANUAL, 0));
	fixture->gate_address = true;
	zassert_true(net_if_ipv4_set_netmask_by_addr(fixture->gate, &underlay, &mask));
	zassert_ok(net_if_up(fixture->gate));
	zassert_ok(base64_encode(encoded, sizeof(encoded), &len, key, sizeof(key)));
	config.public_key = encoded;
	endpoint->sin_family = NET_AF_INET;
	endpoint->sin_port = net_htons(51820);
	endpoint->sin_addr.s_addr = net_htonl(0xc0000202U);
	fixture->peers[1] = wireguard_peer_add_to_interface(iface, &config, NULL);
	zassert_true(fixture->peers[1] > 0);
	zassert_ok(wireguard_peer_keepalive(fixture->peers[1]));
	zassert_ok(k_sem_take(&native_datagram, K_SECONDS(1)));
	zassert_ok(wireguard_test_install_session(fixture->peers[1], &session));

	block_tx_queue(fixture);
	/* Fill the ordinary class with plaintext waiting for the WireGuard TX path. */
	for (size_t i = 0; i < CONFIG_NET_PKT_TX_COUNT / CONFIG_NET_TC_TX_COUNT; ++i) {
		net_if_try_queue_tx(iface, udp_packet(iface, &underlay), K_NO_WAIT);
	}
	dropped = fixture->gate->stats.tc.sent[0].dropped;
	zassert_ok(wireguard_peer_keepalive(fixture->peers[1]));
	for (int i = 0; i < 100 && fixture->gate->stats.tc.sent[0].dropped == dropped; ++i) {
		k_sleep(K_MSEC(1));
	}
	zexpect_equal(fixture->gate->stats.tc.sent[0].dropped, dropped + 1);
	zassert_ok(wireguard_peer_keepalive(fixture->peers[0]));
	zassert_ok(k_msgq_get(&transport_sends, &observed, K_SECONDS(1)));
	zassert_equal(observed, fixture->peers[0]);
	zassert_ok(wireguard_peer_remove(fixture->peers[0]));
	fixture->peers[0] = 0;

	k_sem_give(&gate_release);
	zassert_ok(k_sem_take(&gate_exited, K_SECONDS(1)));
	fixture->gate_queued = false;
	zassert_ok(wireguard_peer_keepalive(fixture->peers[1]));
	zassert_ok(k_sem_take(&native_datagram, K_SECONDS(1)));
#else
	ztest_test_skip();
#endif
}

ZTEST_F(wireguard_tx_queue, test_no_route_completes_send)
{
#if defined(CONFIG_WIREGUARD_MULTI_PEER)
	struct net_if *iface = net_if_get_by_index(net_if_get_by_name("wg0"));
	struct net_in_addr allowed = {.s_addr = net_htonl(0xc0000202U)};
	struct net_in_addr missing = {.s_addr = net_htonl(0xc0000203U)};
	struct net_context *context;
	const uint8_t dst[] = {0x02, 0, 0, 0, 0, 1};
	struct net_pkt *pkt;
	struct tx_completion completion = {0};

	provision(iface);
	fixture->peers[0] = add_peer(iface, &allowed, 9U);
	zassert_ok(net_context_get(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP, &context));
	net_context_set_iface(context, iface);
	context->send_cb = context_complete;
	pkt = udp_packet(iface, &missing);
	net_pkt_set_context(pkt, context);
	zassert_ok(net_linkaddr_set(net_pkt_lladdr_dst(pkt), dst, sizeof(dst)));
	/* Retain one independent reference to observe the stack's release. */
	net_pkt_ref(pkt);
	completion_iface = iface;
	k_msgq_purge(&tx_completions);
	net_if_register_link_cb(&link_cb, link_complete);
	net_if_try_queue_tx(iface, pkt, K_NO_WAIT);
	for (int i = 0; i < 2; ++i) {
		zexpect_ok(k_msgq_get(&tx_completions, &completion, K_SECONDS(1)));
		zexpect_equal(completion.status, -ENETUNREACH);
		zexpect_equal(completion.link, i == 1);
	}
	net_if_unregister_link_cb(&link_cb);
	completion_iface = NULL;
	zexpect_equal(k_msgq_num_used_get(&tx_completions), 0);
	zexpect_equal(atomic_get(&pkt->atomic_ref), 1);
	net_pkt_unref(pkt);
	zexpect_ok(net_context_put(context));
#else
	ztest_test_skip();
#endif
}

ZTEST_SUITE(wireguard_tx_queue, NULL, queue_setup, NULL, queue_cleanup, NULL);
