/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/virtual_mgmt.h>
#include "wg_private.h"
#include <zephyr/sys/base64.h>
#include <zephyr/ztest.h>

#include "wg_internal.h"
#include "wg_test_helpers.h"

struct wg_peer *peer_lookup_by_id(int id);
int wg_create_cookie_reply(struct wg_iface_context *ctx, struct msg_cookie_reply *dst,
			   const uint8_t *mac1, uint32_t index, uint8_t *source_addr_port,
			   size_t source_length);

#define PUBLIC_KEY_B64_SIZE 45

struct handshake_capture {
	struct k_sem ready;
	struct msg_handshake_init initiation;
};

struct wireguard_index_fixture {
	int ids[3];
	struct handshake_capture captures[2];
};

static void *index_setup(void)
{
	static struct wireguard_index_fixture fixture;

	return &fixture;
}

static void index_cleanup(void *data)
{
	struct wireguard_index_fixture *fixture = data;

	ARRAY_FOR_EACH(fixture->ids, i) {
		if (fixture->ids[i] > 0) {
			zexpect_ok(wireguard_peer_remove(fixture->ids[i]));
			fixture->ids[i] = 0;
		}
	}
}

static void encode_key(const uint8_t *key, char encoded[PUBLIC_KEY_B64_SIZE])
{
	size_t size;

	zassert_ok(base64_encode(encoded, PUBLIC_KEY_B64_SIZE, &size, key, WG_PUBLIC_KEY_LEN));
}

static void configure_key(struct net_if *iface, uint8_t value, char encoded[PUBLIC_KEY_B64_SIZE])
{
	struct virtual_interface_req_params params = {0};
	uint8_t key[WG_PRIVATE_KEY_LEN];

	memset(key, value, sizeof(key));
	params.private_key.data = key;
	params.private_key.len = sizeof(key);
	zassert_ok(net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PRIVATE_KEY, iface, &params,
			    sizeof(params)));
	zassert_ok(net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_GET_PUBLIC_KEY, iface, &params,
			    sizeof(params)));
	encode_key(params.public_key.data, encoded);
}

static int capture_initiation(int peer_id, const struct wireguard_datagram_path *path,
			      const uint8_t *data, size_t len, void *user_data)
{
	struct handshake_capture *capture = user_data;

	ARG_UNUSED(peer_id);
	ARG_UNUSED(path);
	if (len == sizeof(capture->initiation) && data[0] == MESSAGE_HANDSHAKE_INITIATION) {
		memcpy(&capture->initiation, data, len);
		k_sem_give(&capture->ready);
	}
	return 0;
}

ZTEST_F(wireguard_index, test_cookie_and_transport_index_routing)
{
	const uint8_t base_point[WG_PUBLIC_KEY_LEN] = {9U};
	const struct msg_handshake_init *first_init = &fixture->captures[0].initiation;
	const struct msg_handshake_init *second_init = &fixture->captures[1].initiation;
	char keys[3][PUBLIC_KEY_B64_SIZE];
	struct wireguard_peer_config config = {0};
	struct wireguard_transport transport = {.send = capture_initiation};
	struct wireguard_datagram_path path = {.kind = WIREGUARD_PATH_DIRECT};
	struct net_sockaddr_in *local = net_sin(net_sad(&path.local));
	struct net_sockaddr_in *remote = net_sin(net_sad(&path.remote));
	struct msg_cookie_reply cookie;
	struct wireguard_test_session session = {.local_index = 0x5432U};
	struct msg_transport_data header = {
		.type = MESSAGE_TRANSPORT_DATA,
		.receiver = session.local_index,
	};
	uint8_t datagram[sizeof(header) + WG_AUTHTAG_LEN] = {0};
	size_t ciphertext_len;
	struct wg_peer *first;
	struct net_if *hub;
	struct net_if *responder;

	if (!IS_ENABLED(CONFIG_WIREGUARD_MULTI_PEER) || CONFIG_WIREGUARD_MAX_PEER < 3 ||
	    net_if_get_by_name("wg1") < 0) {
		ztest_test_skip();
		return;
	}

	hub = net_if_get_by_index(net_if_get_by_name("wg0"));
	responder = net_if_get_by_index(net_if_get_by_name("wg1"));
	configure_key(hub, 5U, keys[0]);
	encode_key(base_point, keys[1]);
	configure_key(responder, 7U, keys[2]);
	ARRAY_FOR_EACH(fixture->captures, i) {
		struct handshake_capture *capture = &fixture->captures[i];

		k_sem_init(&capture->ready, 0, 1);
		transport.user_data = capture;
		config.public_key = keys[i + 1U];
		fixture->ids[i] = wireguard_peer_add_to_interface(hub, &config, &transport);
		zassert_true(fixture->ids[i] > 0, "add returned %d", fixture->ids[i]);
		zassert_ok(wireguard_peer_keepalive(fixture->ids[i]));
		zassert_ok(k_sem_take(&capture->ready, K_SECONDS(1)));
	}
	config.public_key = keys[0];
	fixture->ids[2] = wireguard_peer_add_to_interface(responder, &config, &transport);
	zassert_true(fixture->ids[2] > 0, "add returned %d", fixture->ids[2]);
	zassert_not_equal(first_init->sender, second_init->sender);

	path.iface = net_if_get_default();
	local->sin_family = NET_AF_INET;
	local->sin_port = net_htons(51820);
	local->sin_addr.s_addr = net_htonl(0xC0000201U);
	remote->sin_family = NET_AF_INET;
	remote->sin_port = net_htons(51821);
	remote->sin_addr.s_addr = net_htonl(0xC0000202U);

	zassert_ok(wg_create_cookie_reply(net_if_get_device(responder)->data, &cookie,
					  second_init->mac1, second_init->sender,
					  (uint8_t *)&local->sin_addr, sizeof(local->sin_addr)));
	zassert_equal(wireguard_peer_receive_datagram(fixture->ids[0], &path,
						      (const uint8_t *)&cookie, sizeof(cookie)),
		      -ENOENT);
	first = peer_lookup_by_id(fixture->ids[0]);
	zassert_not_null(first);
	first->handshake.local_index = second_init->sender;
	zassert_equal(wireguard_interface_receive_datagram(hub, &path, (const uint8_t *)&cookie,
							   sizeof(cookie)), -ENOENT);
	first->handshake.local_index = first_init->sender;
	zassert_ok(wireguard_interface_receive_datagram(hub, &path, (const uint8_t *)&cookie,
							sizeof(cookie)));

	memset(session.recv_key, 0x5a, sizeof(session.recv_key));
	zassert_ok(wireguard_test_install_session(fixture->ids[0], &session));
	zassert_ok(wireguard_test_install_session(fixture->ids[1], &session));
	memcpy(datagram, &header, sizeof(header));
	zassert_ok(wireguard_test_encrypt(session.recv_key, 0U, NULL, 0U, datagram + sizeof(header),
					  WG_AUTHTAG_LEN, &ciphertext_len));
	zassert_equal(ciphertext_len, WG_AUTHTAG_LEN);
	zassert_equal(wireguard_interface_receive_datagram(hub, &path, datagram, sizeof(datagram)),
		      -ENOENT);
	zassert_equal(wireguard_peer_receive_datagram(fixture->ids[1], &path, datagram,
						      sizeof(datagram)), -ENOENT);
	zassert_ok(wireguard_peer_remove(fixture->ids[0]));
	fixture->ids[0] = 0;
	zassert_ok(wireguard_interface_receive_datagram(hub, &path, datagram, sizeof(datagram)));
}

ZTEST_F(wireguard_index, test_native_udp_rejects_duplicate_local_identity)
{
	const uint8_t base_point[WG_PUBLIC_KEY_LEN] = {9U};
	char local[2][PUBLIC_KEY_B64_SIZE];
	char remote[PUBLIC_KEY_B64_SIZE];
	struct wireguard_peer_config config = {.public_key = remote};
	struct net_if *left;
	struct net_if *right;

	if (!IS_ENABLED(CONFIG_WIREGUARD_MULTI_PEER) || net_if_get_by_name("wg1") < 0) {
		ztest_test_skip();
		return;
	}
	left = net_if_get_by_index(net_if_get_by_name("wg0"));
	right = net_if_get_by_index(net_if_get_by_name("wg1"));
	configure_key(left, 9U, local[0]);
	configure_key(right, 9U, local[1]);
	zassert_mem_equal(local[0], local[1], sizeof(local[0]));
	encode_key(base_point, remote);

	fixture->ids[0] = wireguard_peer_add_to_interface(left, &config, NULL);
	zassert_true(fixture->ids[0] > 0, "add returned %d", fixture->ids[0]);
	fixture->ids[1] = wireguard_peer_add_to_interface(right, &config, NULL);
	zassert_equal(fixture->ids[1], -EADDRINUSE);
	zassert_false(net_if_is_up(right));

	zassert_ok(wireguard_peer_remove(fixture->ids[0]));
	fixture->ids[0] = 0;
	fixture->ids[1] = wireguard_peer_add_to_interface(right, &config, NULL);
	zassert_true(fixture->ids[1] > 0, "add returned %d", fixture->ids[1]);
}

#if defined(CONFIG_WIREGUARD_MULTI_PEER) && defined(CONFIG_NET_MGMT_EVENT_INFO)
struct event_sample {
	uint64_t event;
	int id;
	char public_key[PUBLIC_KEY_B64_SIZE];
	uint32_t endpoint;
	uint32_t prefix;
	int keepalive;
};

K_MSGQ_DEFINE(vpn_events, sizeof(struct event_sample), 16, 8);
static K_SEM_DEFINE(event_entered, 0, 1);
static K_SEM_DEFINE(event_release, 0, 1);
static atomic_t pause_add;
static struct net_mgmt_event_callback event_cb;
static struct net_if *event_iface;

static void record_event(struct net_mgmt_event_callback *cb, uint64_t event, struct net_if *iface)
{
	struct event_sample sample = {.event = event};

	if (iface != event_iface) {
		return;
	}
	switch (event) {
	case NET_EVENT_VPN_PEER_ADD:
		if (atomic_cas(&pause_add, 1, 0)) {
			k_sem_give(&event_entered);
			(void)k_sem_take(&event_release, K_SECONDS(2));
		}
		if (cb->info_length == sizeof(struct net_event_vpn_peer)) {
			struct net_event_vpn_peer added;

			memcpy(&added, cb->info, sizeof(added));
			sample.id = added.id;
			if (added.public_key != NULL) {
				memcpy(sample.public_key, added.public_key,
				       sizeof(sample.public_key));
			}
			if (added.endpoint != NULL && added.endpoint->sa_family == NET_AF_INET) {
				sample.endpoint = net_sin(added.endpoint)->sin_addr.s_addr;
			}
			if (added.allowed_ip[0] != NULL && added.allowed_ip[0]->is_valid) {
				sample.prefix = added.allowed_ip[0]->addr.in_addr.s_addr;
			}
			sample.keepalive = added.keepalive_interval;
		}
		break;
	case NET_EVENT_VPN_PEER_DEL:
		memcpy(&sample.id, cb->info, sizeof(sample.id));
		break;
	case NET_EVENT_VPN_CONNECTED:
	case NET_EVENT_VPN_DISCONNECTED:
		break;
	default:
		return;
	}
	(void)k_msgq_put(&vpn_events, &sample, K_NO_WAIT);
}

static struct event_sample expect_event(uint64_t event, int id)
{
	struct event_sample sample;

	zassert_ok(k_msgq_get(&vpn_events, &sample, K_SECONDS(1)));
	zassert_equal(sample.event, event);
	zassert_equal(sample.id, id);
	return sample;
}

static int add_event_peer(struct net_if *iface, uint8_t value)
{
	uint8_t key[WG_PUBLIC_KEY_LEN] = {value};
	char encoded[PUBLIC_KEY_B64_SIZE];
	struct wireguard_peer_config config = {.public_key = encoded};
	struct wireguard_transport transport = {.send = capture_initiation};
	int id;

	encode_key(key, encoded);
	net_sin(net_sad(&config.endpoint_ip))->sin_family = NET_AF_INET;
	net_sin(net_sad(&config.endpoint_ip))->sin_addr.s_addr = net_htonl(0xc0000200U | value);
	config.allowed_ip[0] = (struct wireguard_allowed_ip){
		.addr.family = NET_AF_INET,
		.addr.in_addr.s_addr = net_htonl(0xc6336400U | value),
		.mask_len = 32,
		.is_valid = true,
	};
	config.keepalive_interval = value;
	id = wireguard_peer_add_to_interface(iface, &config, &transport);
	zassert_true(id > 0, "add returned %d", id);
	return id;
}

static void authenticate_peer(int id)
{
	struct wireguard_test_session session = {.local_index = id};
	const struct wireguard_datagram_path path = {.kind = WIREGUARD_PATH_RELAY};
	struct msg_transport_data header = {.type = MESSAGE_TRANSPORT_DATA, .receiver = id};
	uint8_t datagram[sizeof(header) + WG_AUTHTAG_LEN];
	size_t len;

	memcpy(datagram, &header, sizeof(header));
	zassert_ok(wireguard_test_install_session(id, &session));
	zassert_ok(wireguard_test_encrypt(session.recv_key, 0, NULL, 0, datagram + sizeof(header),
					  WG_AUTHTAG_LEN, &len));
	zassert_ok(wireguard_peer_receive_datagram(id, &path, datagram, sizeof(header) + len));
}

ZTEST_F(wireguard_index, test_peer_events_and_interface_connectivity)
{
	struct wireguard_peer_info info;
	char encoded[PUBLIC_KEY_B64_SIZE];
	struct net_if *iface = net_if_get_by_index(net_if_get_by_name("wg0"));
	int retired;

	configure_key(iface, 41, encoded);
	event_iface = iface;
	k_msgq_purge(&vpn_events);
	net_mgmt_init_event_callback(&event_cb, record_event,
		NET_EVENT_VPN_PEER_ADD | NET_EVENT_VPN_PEER_DEL |
		NET_EVENT_VPN_CONNECTED | NET_EVENT_VPN_DISCONNECTED);
	net_mgmt_add_event_callback(&event_cb);

	fixture->ids[0] = add_event_peer(iface, 9);
	expect_event(NET_EVENT_VPN_PEER_ADD, fixture->ids[0]);
	if (IS_ENABLED(CONFIG_NET_MGMT_EVENT_QUEUE)) {
		atomic_set(&pause_add, 1);
	}
	retired = add_event_peer(iface, 10);
	if (IS_ENABLED(CONFIG_NET_MGMT_EVENT_QUEUE)) {
		zassert_ok(k_sem_take(&event_entered, K_SECONDS(1)));
	}
	zassert_ok(wireguard_peer_remove(retired));
	fixture->ids[1] = add_event_peer(iface, 11);
	k_sem_give(&event_release);
	struct event_sample retired_event = expect_event(NET_EVENT_VPN_PEER_ADD, retired);
	uint8_t retired_key[WG_PUBLIC_KEY_LEN] = {10};

	encode_key(retired_key, encoded);
	zexpect_str_equal(retired_event.public_key, encoded);
	zexpect_equal(retired_event.endpoint, net_htonl(0xc000020aU));
	zexpect_equal(retired_event.prefix, net_htonl(0xc633640aU));
	zexpect_equal(retired_event.keepalive, 10);
	expect_event(NET_EVENT_VPN_PEER_DEL, retired);
	expect_event(NET_EVENT_VPN_PEER_ADD, fixture->ids[1]);
	zassert_equal(wireguard_peer_get_info(retired, &info), -ENOENT);
	zassert_ok(wireguard_peer_get_info(fixture->ids[1], &info));
	zassert_equal(info.public_key[0], 11);

	fixture->ids[2] = add_event_peer(iface, 12);
	expect_event(NET_EVENT_VPN_PEER_ADD, fixture->ids[2]);
	authenticate_peer(fixture->ids[0]);
	expect_event(NET_EVENT_VPN_CONNECTED, 0);
	authenticate_peer(fixture->ids[1]);
	for (size_t i = 0; i < ARRAY_SIZE(fixture->ids); i++) {
		zassert_ok(wireguard_peer_remove(fixture->ids[i]));
		expect_event(NET_EVENT_VPN_PEER_DEL, fixture->ids[i]);
		fixture->ids[i] = 0;
		if (i == 1) {
			/* Only the peer which has never authenticated remains. */
			expect_event(NET_EVENT_VPN_DISCONNECTED, 0);
		}
	}
	struct event_sample extra;

	zexpect_not_equal(k_msgq_get(&vpn_events, &extra, K_MSEC(20)), 0);
	net_mgmt_del_event_callback(&event_cb);
	event_iface = NULL;
}
#endif

ZTEST_SUITE(wireguard_index, NULL, index_setup, NULL, index_cleanup, NULL);
