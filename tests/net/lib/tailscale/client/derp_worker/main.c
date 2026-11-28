/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/net/socket.h>
#include <zephyr/ztest.h>

#include "transport.h"
#include "control_internal.h"
#include "disco_box.h"
#ifdef CONFIG_TAILSCALE_DERP
#include "derp_conn.h"
#endif

#define LOCAL_PORT 46001
#define PEER_PORT 46002
#define PEER_ID 1
#define RELAY_REGION 901
#define ADDITIONAL_REGION 903

static K_SEM_DEFINE(direct_received, 0, 1);
#ifdef CONFIG_TAILSCALE_DERP
static K_SEM_DEFINE(relay_entered, 0, 1);
static K_SEM_DEFINE(relay_release, 0, 1);
static K_SEM_DEFINE(additional_relay_wrote, 0, 1);
static atomic_t relay_waiting;
static bool simulate_relays;
static int additional_fd = -1;
#endif
static struct net_in_addr replacement_ipv4;
static struct net_in6_addr replacement_ipv6;
static struct tailscale_transport_plan plan;
static struct tailscale_peer_snapshot snapshot;
static struct net_if *loopback;
static int peer_fd = -1;
static bool started;
static const uint8_t private_key[32] = {1};
static const uint8_t inbound[32] = {4, 0, 0, 0, 0x17};
static const uint8_t outbound[32] = {4, 0, 0, 0, 0x29};

#ifdef CONFIG_TAILSCALE_DERP
int derp_conn_open(struct derp_conn *conn, const struct tailscale_derp_node *node,
		   struct net_if *lan, sec_tag_t ca, mbedtls_svc_key_id_t key, bool home,
		   uint8_t scratch[DERP_FRAME_MAX])
{
	ARG_UNUSED(lan);
	ARG_UNUSED(ca);
	ARG_UNUSED(key);
	ARG_UNUSED(home);
	ARG_UNUSED(scratch);
	if (simulate_relays) {
		conn->io.fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
		if (conn->io.fd < 0) {
			return -errno;
		}
		conn->last_rx = k_uptime_get();
		if (node->region == ADDITIONAL_REGION) {
			additional_fd = conn->io.fd;
		}
		return 0;
	}
	atomic_set(&relay_waiting, 1);
	k_sem_give(&relay_entered);
	(void)k_sem_take(&relay_release, K_SECONDS(5));
	atomic_clear(&relay_waiting);
	return -ETIMEDOUT;
}

int derp_conn_read(struct derp_conn *conn, uint8_t *type, uint8_t data[DERP_FRAME_MAX], size_t *len)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(type);
	ARG_UNUSED(data);
	ARG_UNUSED(len);
	return -ENOTCONN;
}

int derp_conn_write(struct derp_conn *conn, uint8_t type, const uint8_t *data, size_t len)
{
	if (simulate_relays) {
		if (conn->io.fd == additional_fd && type == DERP_SEND &&
		    len == 32U + sizeof(outbound) &&
		    memcmp(data + 32U, outbound, sizeof(outbound)) == 0) {
			k_sem_give(&additional_relay_wrote);
		}
		return 0;
	}
	return -ENOTCONN;
}

#endif

int __wrap_wireguard_peer_receive_datagram(int id, const struct wireguard_datagram_path *path,
					  const uint8_t *data, size_t len)
{
	if (id != PEER_ID || path->kind != WIREGUARD_PATH_DIRECT || path->iface != loopback ||
	    path->remote.ss_family != NET_AF_INET || path->local.ss_family != NET_AF_INET ||
	    net_sin(net_sad(&path->remote))->sin_port != net_htons(PEER_PORT) ||
	    net_sin(net_sad(&path->local))->sin_port != net_htons(LOCAL_PORT) ||
	    len != sizeof(inbound) || memcmp(data, inbound, sizeof(inbound)) != 0) {
		return -EINVAL;
	}
	k_sem_give(&direct_received);
	return 0;
}

static void cleanup(void *fixture)
{
	ARG_UNUSED(fixture);
#ifdef CONFIG_TAILSCALE_DERP
	k_sem_give(&relay_release);
#endif
	if (started) {
		tailscale_transport_stop();
		started = false;
	}
	if (loopback != NULL) {
		(void)net_if_ipv4_addr_rm(loopback, &replacement_ipv4);
		(void)net_if_ipv6_addr_rm(loopback, &replacement_ipv6);
	}
#ifdef CONFIG_TAILSCALE_DERP
	simulate_relays = false;
	additional_fd = -1;
#endif
	memset(&plan, 0, sizeof(plan));
	memset(&snapshot, 0, sizeof(snapshot));
	if (peer_fd >= 0) {
		(void)zsock_close(peer_fd);
		peer_fd = -1;
	}
}

#if defined(CONFIG_TAILSCALE_DERP) && defined(CONFIG_TAILSCALE_DISCO)
ZTEST(derp_worker, test_direct_traffic_during_relay_connect)
{
	struct net_sockaddr_in local = {.sin_family = NET_AF_INET,
					.sin_port = net_htons(LOCAL_PORT)};
	struct net_sockaddr_in peer = {.sin_family = NET_AF_INET, .sin_port = net_htons(PEER_PORT)};
	struct tailscale_control_params params = {.node_private = private_key,
						  .disco_private = private_key};
	struct zsock_pollfd reply = {.events = ZSOCK_POLLIN};
	uint8_t received[sizeof(outbound)];

	k_sem_reset(&relay_entered);
	k_sem_reset(&relay_release);
	k_sem_reset(&direct_received);
	zassert_ok(net_addr_pton(NET_AF_INET, "127.0.0.1", &local.sin_addr));
	peer.sin_addr = local.sin_addr;
	zassert_not_null(net_if_ipv4_addr_lookup(&local.sin_addr, &loopback));
	zassert_equal(psa_crypto_init(), PSA_SUCCESS);
	memcpy(&params.local_endpoint, &local, sizeof(local));
	peer_fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(peer_fd >= 0, "peer socket: %d", errno);
	zassert_ok(zsock_bind(peer_fd, (const void *)&peer, sizeof(peer)), "peer bind: %d", errno);
	zassert_ok(tailscale_transport_start(&params, loopback));
	started = true;

	plan.count = 1;
	plan.peers[0].id = PEER_ID;
	plan.peers[0].region = RELAY_REGION;
	memcpy(&plan.peers[0].endpoint, &peer, sizeof(peer));
	snapshot.derp.count = 1;
	snapshot.derp.nodes[0].region = RELAY_REGION;
	snapshot.derp.nodes[0].port = 443;
	zassert_ok(tailscale_transport_stage(&plan, &snapshot));
	tailscale_transport_publish(&plan);
	zassert_ok(k_sem_take(&relay_entered, K_SECONDS(1)), "relay did not enter connect");

	zassert_equal(zsock_sendto(peer_fd, inbound, sizeof(inbound), 0, (const void *)&local,
				  sizeof(local)), sizeof(inbound));
	zassert_ok(k_sem_take(&direct_received, K_SECONDS(1)), "direct receive stalled");
	zassert_ok(tailscale_transport_send(PEER_ID, NULL, outbound, sizeof(outbound),
				   UINT_TO_POINTER(plan.epoch)));
	reply.fd = peer_fd;
	zassert_equal(zsock_poll(&reply, 1, 1000), 1, "direct send stalled");
	zassert_equal(zsock_recv(peer_fd, received, sizeof(received), ZSOCK_MSG_DONTWAIT),
		      sizeof(received));
	zassert_mem_equal(received, outbound, sizeof(outbound));
	zassert_true(atomic_get(&relay_waiting), "relay connect returned during direct exchange");
}

#endif

static void start_worker(void)
{
	const struct net_sockaddr_in local = {
		.sin_family = NET_AF_INET, .sin_port = net_htons(LOCAL_PORT),
		.sin_addr = NET_INADDR_LOOPBACK_INIT,
	};
	struct tailscale_control_params params = {
		.node_private = private_key, .disco_private = private_key,
	};

	zassert_not_null(net_if_ipv4_addr_lookup(&local.sin_addr, &loopback));
	zassert_equal(psa_crypto_init(), PSA_SUCCESS);
	memcpy(&params.local_endpoint, &local, sizeof(local));
	zassert_ok(tailscale_transport_start(&params, loopback));
	started = true;
}

#ifndef CONFIG_TAILSCALE_DERP
static const uint8_t disco_magic[] = {0x54, 0x53, 0xf0, 0x9f, 0x92, 0xac};

#define DISCO_KEY_OFFSET sizeof(disco_magic)
#define DISCO_NONCE_OFFSET (DISCO_KEY_OFFSET + DISCO_BOX_KEY_SIZE)
#define DISCO_HEADER_SIZE (DISCO_NONCE_OFFSET + DISCO_BOX_NONCE_SIZE)

static void send_disco(const struct net_sockaddr_in *local, const uint8_t public_key[32],
		       const uint8_t box[32], const uint8_t *payload, size_t len)
{
	uint8_t packet[DISCO_HEADER_SIZE + DISCO_BOX_TAG_SIZE + 32];
	uint8_t *nonce = packet + DISCO_NONCE_OFFSET;

	zassert_true(len <= 32U);
	memcpy(packet, disco_magic, sizeof(disco_magic));
	memcpy(packet + DISCO_KEY_OFFSET, public_key, DISCO_BOX_KEY_SIZE);
	memset(nonce, payload[0], DISCO_BOX_NONCE_SIZE);
	zassert_ok(disco_box_seal(box, nonce, payload, len, packet + DISCO_HEADER_SIZE));
	len += DISCO_HEADER_SIZE + DISCO_BOX_TAG_SIZE;
	zassert_equal(zsock_sendto(peer_fd, packet, len, 0, (const void *)local, sizeof(*local)),
		      len);
}

static void read_disco(const uint8_t box[32], uint8_t *payload, size_t len)
{
	struct zsock_pollfd poll = {.fd = peer_fd, .events = ZSOCK_POLLIN};
	uint8_t packet[DISCO_HEADER_SIZE + DISCO_BOX_TAG_SIZE + 32];
	ssize_t n;

	zassert_equal(zsock_poll(&poll, 1, 1000), 1);
	n = zsock_recv(peer_fd, packet, sizeof(packet), ZSOCK_MSG_DONTWAIT);
	zassert_equal(n, DISCO_HEADER_SIZE + DISCO_BOX_TAG_SIZE + len);
	zassert_mem_equal(packet, disco_magic, sizeof(disco_magic));
	zassert_ok(disco_box_open(box, packet + DISCO_NONCE_OFFSET, packet + DISCO_HEADER_SIZE,
				  len + DISCO_BOX_TAG_SIZE, payload));
}

ZTEST(derp_worker, test_disco_establishes_direct_path_without_relay)
{
	const struct net_sockaddr_in local = {
		.sin_family = NET_AF_INET, .sin_port = net_htons(LOCAL_PORT),
		.sin_addr = NET_INADDR_LOOPBACK_INIT,
	};
	struct net_sockaddr_in peer = local;
	const uint8_t peer_private[32] = {[1] = 7};
	mbedtls_svc_key_id_t local_key, peer_key;
	uint8_t local_public[32], shared[32], box[32];
	uint8_t ping[14] = {1, 0, 0x73};
	uint8_t pong[32] = {2, 0};
	uint8_t received[32];

	start_worker();
	peer.sin_port = net_htons(PEER_PORT);
	peer_fd = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(peer_fd >= 0);
	zassert_ok(zsock_bind(peer_fd, (const void *)&peer, sizeof(peer)));
	zassert_ok(control_x25519_import(private_key, &local_key));
	zassert_ok(control_x25519_public(local_key, local_public));
	zassert_equal(psa_destroy_key(local_key), PSA_SUCCESS);
	zassert_ok(control_x25519_import(peer_private, &peer_key));
	zassert_ok(control_x25519_public(peer_key, plan.peers[0].disco));
	zassert_ok(control_x25519_agree(peer_key, local_public, shared));
	zassert_equal(psa_destroy_key(peer_key), PSA_SUCCESS);
	disco_box_precompute(box, shared);

	plan.count = 1;
	plan.peers[0].id = PEER_ID;
	zassert_ok(tailscale_transport_stage(&plan, &snapshot));
	tailscale_transport_publish(&plan);
	k_sleep(K_MSEC(25));
	send_disco(&local, plan.peers[0].disco, box, ping, sizeof(ping));
	read_disco(box, received, sizeof(pong));
	zassert_equal(received[0], 2);
	zassert_mem_equal(received + 2, ping + 2, 12);
	read_disco(box, received, sizeof(ping));
	zassert_equal(received[0], 1);
	memcpy(pong + 2, received + 2, 12);
	send_disco(&local, plan.peers[0].disco, box, pong, sizeof(pong));

	k_sem_reset(&direct_received);
	zassert_equal(zsock_sendto(peer_fd, inbound, sizeof(inbound), 0, (const void *)&local,
				  sizeof(local)), sizeof(inbound));
	zassert_ok(k_sem_take(&direct_received, K_SECONDS(1)));
	zassert_ok(tailscale_transport_send(PEER_ID, NULL, outbound, sizeof(outbound),
					   UINT_TO_POINTER(plan.epoch)));
	struct zsock_pollfd poll = {.fd = peer_fd, .events = ZSOCK_POLLIN};

	zassert_equal(zsock_poll(&poll, 1, 1000), 1);
	zassert_equal(zsock_recv(peer_fd, received, sizeof(received), ZSOCK_MSG_DONTWAIT),
		      sizeof(outbound));
	zassert_mem_equal(received, outbound, sizeof(outbound));
	zassert_equal(tailscale_transport_home(), 0);
}
#endif

#ifdef CONFIG_TAILSCALE_DERP
ZTEST(derp_worker, test_transmit_to_additional_relay_region)
{
	simulate_relays = true;
	k_sem_reset(&additional_relay_wrote);
	start_worker();
	if (!IS_ENABLED(CONFIG_TAILSCALE_DISCO)) {
		struct tailscale_transport_endpoints endpoints;

		tailscale_transport_get_endpoints(&endpoints);
		zassert_equal(endpoints.local_ipv4.ss_family, NET_AF_UNSPEC);
		zassert_equal(endpoints.local_ipv6.ss_family, NET_AF_UNSPEC);
	}

	snapshot.self.derp_region = RELAY_REGION;
	snapshot.derp.count = 3U;
	plan.count = 4U;
	for (size_t i = 0U; i < snapshot.derp.count; i++) {
		snapshot.derp.nodes[i].region = RELAY_REGION + i;
		plan.peers[i].id = PEER_ID + i;
		plan.peers[i].region = RELAY_REGION + i;
	}
	plan.peers[3].id = PEER_ID + 3;
	plan.peers[3].region = 999;
	zassert_ok(tailscale_transport_stage(&plan, &snapshot));
	zassert_equal(plan.home, RELAY_REGION);
	zassert_equal(plan.peers[2].region, ADDITIONAL_REGION);
	zassert_equal(plan.peers[3].region, RELAY_REGION);
	tailscale_transport_publish(&plan);

	zassert_ok(tailscale_transport_send(PEER_ID + 2, NULL, outbound, sizeof(outbound),
				      UINT_TO_POINTER(plan.epoch)));
	zassert_ok(k_sem_take(&additional_relay_wrote, K_SECONDS(1)));
	zassert_equal(tailscale_transport_home(), RELAY_REGION);
}

#endif

#ifdef CONFIG_TAILSCALE_DISCO
ZTEST(derp_worker, test_endpoints_follow_underlay_addresses)
{
	struct tailscale_transport_endpoints endpoints;
	const struct net_in_addr loopback_addr = NET_INADDR_LOOPBACK_INIT;

	start_worker();
	tailscale_transport_get_endpoints(&endpoints);
	zassert_true(net_ipv4_addr_cmp(&net_sin(net_sad(&endpoints.local_ipv4))->sin_addr,
				       &loopback_addr));
	zassert_equal(endpoints.local_ipv6.ss_family, NET_AF_UNSPEC);

	zassert_ok(net_addr_pton(NET_AF_INET, "192.0.2.1", &replacement_ipv4));
	zassert_ok(net_addr_pton(NET_AF_INET6, "2001:db8::1", &replacement_ipv6));
	zassert_not_null(net_if_ipv4_addr_add(loopback, &replacement_ipv4, NET_ADDR_MANUAL, 0));
	zassert_not_null(net_if_ipv6_addr_add(loopback, &replacement_ipv6, NET_ADDR_MANUAL, 0));
	for (size_t i = 0; i < 40U; i++) {
		k_sleep(K_MSEC(25));
		tailscale_transport_get_endpoints(&endpoints);
		if (endpoints.local_ipv6.ss_family == NET_AF_INET6 &&
		    net_ipv4_addr_cmp(&net_sin(net_sad(&endpoints.local_ipv4))->sin_addr,
				      &replacement_ipv4)) {
			break;
		}
	}
	zassert_true(net_ipv4_addr_cmp(&net_sin(net_sad(&endpoints.local_ipv4))->sin_addr,
				       &replacement_ipv4));
	zassert_equal(endpoints.local_ipv6.ss_family, NET_AF_INET6);
	zassert_true(net_ipv6_addr_cmp(&net_sin6(net_sad(&endpoints.local_ipv6))->sin6_addr,
				       &replacement_ipv6));

	(void)net_if_ipv4_addr_rm(loopback, &replacement_ipv4);
	(void)net_if_ipv6_addr_rm(loopback, &replacement_ipv6);
	for (size_t i = 0; i < 40U; i++) {
		k_sleep(K_MSEC(25));
		tailscale_transport_get_endpoints(&endpoints);
		if (endpoints.local_ipv6.ss_family == NET_AF_UNSPEC &&
		    net_ipv4_addr_cmp(&net_sin(net_sad(&endpoints.local_ipv4))->sin_addr,
				      &loopback_addr)) {
			break;
		}
	}
	zassert_true(net_ipv4_addr_cmp(&net_sin(net_sad(&endpoints.local_ipv4))->sin_addr,
				       &loopback_addr));
	zassert_equal(endpoints.local_ipv6.ss_family, NET_AF_UNSPEC);
}
#endif

ZTEST_SUITE(derp_worker, NULL, NULL, NULL, cleanup, NULL);
