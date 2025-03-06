/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Wireguard VPN implementation
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wireguard, CONFIG_WIREGUARD_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/base64.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/virtual_mgmt.h>
#include <zephyr/net/wireguard.h>
#include <zephyr/net/ethernet.h>

#include "net_private.h"
#include "udp_internal.h"
#include "ipv4.h"
#include "ipv6.h"
#include "net_stats.h"

/* Constants */
/* The UTF-8 string literal "Noise_IKpsk2_25519_ChaChaPoly_BLAKE2s", 37 bytes of output */
static const uint8_t CONSTRUCTION[37] = "Noise_IKpsk2_25519_ChaChaPoly_BLAKE2s";
/* The UTF-8 string literal "WireGuard v1 zx2c4 Jason@zx2c4.com", 34 bytes of output */
static const uint8_t IDENTIFIER[34] = "WireGuard v1 zx2c4 Jason@zx2c4.com";
/* Label-Mac1 The UTF-8 string literal "mac1----", 8 bytes of output. */
static const uint8_t LABEL_MAC1[8] = "mac1----";
/* Label-Cookie The UTF-8 string literal "cookie--", 8 bytes of output */
static const uint8_t LABEL_COOKIE[8] = "cookie--";

#define WG_FUNCTION_PROTOTYPES
#include "wg_internal.h"

static int interface_send(struct net_if *iface, struct net_pkt *pkt);

#if defined(CONFIG_WIREGUARD_TXRX_DEBUG)
#define DEBUG_TX 1
#define DEBUG_RX 1
#else
#define DEBUG_TX 0
#define DEBUG_RX 0
#endif

#define WG_TIMER_PERIOD 500 /* ms */

#define WG_BUF_COUNT CONFIG_WIREGUARD_BUF_COUNT
#define WG_MAX_BUF_SIZE CONFIG_WIREGUARD_BUF_LEN

#define WG_DEFAULT_PORT 51820

NET_PKT_SLAB_DEFINE(decrypted_pkts, WG_BUF_COUNT);

#if defined(CONFIG_NET_BUF_FIXED_DATA_SIZE)
NET_BUF_POOL_FIXED_DEFINE(msg_pool, WG_BUF_COUNT,
			  WG_MAX_BUF_SIZE, 0, NULL);
#else
NET_BUF_POOL_VAR_DEFINE(msg_pool, WG_BUF_COUNT,
			WG_MAX_BUF_SIZE, 0, NULL);
#endif

static const uint8_t zero_key[WG_PUBLIC_KEY_LEN];

static K_MUTEX_DEFINE(lock);

static struct wg_peer peers[CONFIG_WIREGUARD_MAX_PEER];
static sys_slist_t peer_list;
static sys_slist_t active_peers;

static struct wg_context {
	struct k_work_delayable wg_periodic_timer;
	struct net_mgmt_event_callback wg_mgmt_cb;
	struct net_if *iface; /* control interface */
	int ifindex;          /* output network interface if set */
	uint8_t construction_hash[WG_HASH_LEN];
	uint8_t identifier_hash[WG_HASH_LEN];

	/* Local Wireguard port */
	uint16_t port;

	bool status;
} wg_ctx;

/* Each Wireguard virtual interface is tied to one specific wg_iface_context */
struct wg_iface_context {
	const char *name;
	struct net_if *iface;
	struct net_if *attached_to;
	struct wg_context *wg_ctx;
	struct wg_peer *peer;

#if defined(CONFIG_NET_STATISTICS_VPN)
	struct net_stats_vpn stats;
#endif /* CONFIG_NET_STATISTICS_VPN */

	uint8_t public_key[WG_PUBLIC_KEY_LEN];
	uint8_t private_key[WG_PRIVATE_KEY_LEN];

	uint8_t cookie_secret[WG_HASH_LEN];
	k_timepoint_t cookie_secret_expires;

	uint8_t label_cookie_key[WG_SESSION_KEY_LEN];
	uint8_t label_mac1_key[WG_SESSION_KEY_LEN];

	bool is_used : 1;
	bool status : 1;
	bool init_done : 1;
};

#include "wg_stats.h"

static int create_packet(struct net_if *iface,
			 struct net_sockaddr *src,
			 struct net_sockaddr *dst,
			 uint8_t *packet,
			 size_t packet_len,
			 uint8_t dscp,
			 uint8_t ecn,
			 struct net_pkt **pkt);

static enum net_verdict wg_input(struct net_conn *conn,
				 struct net_pkt *pkt,
				 union net_ip_header *ip_hdr,
				 union net_proto_header *proto_hdr,
				 void *user_data)
{
	struct wg_context *ctx = user_data;

	ARG_UNUSED(conn);

	net_pkt_set_vpn_iface(pkt, net_pkt_iface(pkt));
	net_pkt_set_vpn_ip_hdr(pkt, ip_hdr);
	net_pkt_set_vpn_udp_hdr(pkt, proto_hdr);

	if (DEBUG_RX) {
		char str[sizeof("RX iface xx")];

		snprintk(str, sizeof(str), "RX iface %d",
			 net_if_get_by_iface(net_pkt_iface(pkt)));

		net_pkt_hexdump(pkt, str);
	}

	/* Feed the data to Wireguard control interface which
	 * will decrypt it and then pass it to the virtual interface
	 * that is handling that connection if such connection is found.
	 */
	return net_if_recv_data(ctx->iface, pkt);
}

static int select_target_iface(struct wg_peer *peer,
			       struct net_sockaddr *addr,
			       struct net_sockaddr_storage *dst,
			       struct net_if **iface)
{
	if (IS_ENABLED(CONFIG_NET_IPV6) && dst->ss_family == NET_AF_INET6) {
		const struct net_in6_addr *src;

		*iface = net_if_ipv6_select_src_iface_addr(
			&net_sin6(net_sad(dst))->sin6_addr, &src);

		net_ipv6_addr_copy_raw((uint8_t *)(&net_sin6(addr)->sin6_addr),
				       (const uint8_t *)src);
		addr->sa_family = NET_AF_INET6;

	} else if (IS_ENABLED(CONFIG_NET_IPV4) && dst->ss_family == NET_AF_INET) {
		const struct net_in_addr *src;

		*iface = net_if_ipv4_select_src_iface_addr(
			&net_sin(net_sad(dst))->sin_addr, &src);

		net_ipv4_addr_copy_raw((uint8_t *)&net_sin(addr)->sin_addr,
				       (const uint8_t *)src);
		addr->sa_family = NET_AF_INET;

	} else {
		return -EAFNOSUPPORT;
	}

	net_sin(addr)->sin_port = net_htons(peer->ctx->wg_ctx->port);

	return 0;
}

static void wg_iface_event_handler(struct net_mgmt_event_callback *cb,
				   uint64_t mgmt_event, struct net_if *iface)
{
	struct wg_context *context =
		CONTAINER_OF(cb, struct wg_context, wg_mgmt_cb);

	if (mgmt_event != NET_EVENT_IF_DOWN && mgmt_event != NET_EVENT_IF_UP) {
		return;
	}

	if (context->ifindex > 0 &&
	    context->ifindex != net_if_get_by_iface(iface)) {
		return;
	}

	if (mgmt_event == NET_EVENT_IF_DOWN) {
		NET_DBG("Interface %d going down", net_if_get_by_iface(iface));
	} else if (mgmt_event == NET_EVENT_IF_UP) {
		NET_DBG("Interface %d coming up", net_if_get_by_iface(iface));
	}
}

static bool should_reset_keypair(struct wg_keypair *keypair)
{
	if (keypair->is_valid && sys_timepoint_expired(keypair->rejected)) {
		return true;
	}

	return false;
}

static bool should_destroy_keypair(struct wg_keypair *keypair)
{
	if (keypair->is_valid &&
	    (sys_timepoint_expired(keypair->expires) ||
	     keypair->sending_counter >= REJECT_AFTER_MESSAGES)) {
		return true;
	}

	return false;
}

static bool should_send_keepalive(struct wg_peer *peer)
{
	if (peer->keepalive_interval > 0) {
		if (peer->session.keypair.current.is_valid ||
		    peer->session.keypair.prev.is_valid) {
			if (sys_timepoint_expired(peer->keepalive_expires)) {
				return true;
			}
		}
	}

	return false;
}

static bool can_send_init(struct wg_peer *peer)
{
	return ((peer->last_initiation_tx == 0) ||
		sys_timepoint_expired(peer->rekey_expires));
}

static bool should_send_init(struct wg_peer *peer)
{
	if (can_send_init(peer)) {
		if (peer->send_handshake) {
			return true;
		}

		if (peer->session.keypair.current.is_valid &&
		    !peer->session.keypair.current.is_initiator) {
			k_timepoint_t expires;

			expires = sys_timepoint_calc(
				K_SECONDS(REJECT_AFTER_TIME - peer->keepalive_interval));

			if (sys_timepoint_cmp(peer->session.keypair.current.expires,
					      expires) > 0) {
				return true;
			}
		}

		if (!peer->session.keypair.current.is_valid) {
			return true;
		}
	}

	return false;
}

static int start_handshake(struct wg_iface_context *ctx,
			   struct wg_peer *peer)
{
	struct msg_handshake_init msg;
	int ret;

	if (!wg_create_handshake_init(ctx, peer, &msg)) {
		return -EINVAL;
	}

	ret = wg_send_handshake_init(ctx, ctx->iface, peer, &msg);
	if (ret < 0) {
		NET_DBG("Cannot send handshake initiation (%d)", ret);
		return ret;
	}

	peer->send_handshake = false;
	peer->last_initiation_tx = sys_clock_tick_get_32();
	peer->rekey_expires = sys_timepoint_calc(K_SECONDS(REKEY_TIMEOUT));
	memcpy(peer->handshake_mac1, msg.mac1, WG_COOKIE_LEN);
	peer->handshake_mac1_valid = true;

	ret = k_work_schedule(&ctx->wg_ctx->wg_periodic_timer, K_MSEC(WG_TIMER_PERIOD));
	if (ret < 0) {
		NET_DBG("Cannot schedule %s work (%d)", "periodic", ret);
	}

	return 0;
}

static int wg_send_keepalive(struct wg_iface_context *ctx,
			     struct wg_peer *peer)
{
	struct net_sockaddr_storage my_addr = { 0 };
	struct net_sockaddr *addr = (struct net_sockaddr *)&my_addr;
	struct net_if *target_iface;
	struct net_pkt *pkt;
	int ret;

	ret = select_target_iface(peer, addr, &peer->endpoint, &target_iface);
	if (ret < 0) {
		NET_DBG("Unknown address family %d", peer->endpoint.ss_family);
		vpn_stats_update_invalid_ip_family(ctx);
		return -EAFNOSUPPORT;
	}

	if (target_iface == NULL) {
		target_iface = net_if_get_default();
	}

	pkt = net_pkt_alloc_with_buffer(target_iface,
					0,
					peer->endpoint.ss_family,
					0,
					PKT_ALLOC_WAIT_TIME);
	if (pkt == NULL) {
		NET_DBG("Packet creation failed (%d)", ret);
		vpn_stats_update_alloc_failed(ctx);
		return -ENOMEM;
	}

	net_pkt_set_vpn_peer_id(pkt, peer->id);
	net_pkt_set_vpn_iface(pkt, target_iface);
	net_pkt_set_iface(pkt, target_iface);

	/* Send empty keepalive packet directly */
	ret = interface_send(ctx->iface, pkt);

	NET_DBG("Sending keepalive to %s via iface %d (%d)",
		net_sprint_addr(peer->endpoint.ss_family,
				&net_sin(net_sad(&peer->endpoint))->sin_addr),
		net_if_get_by_iface(ctx->iface),
		ret);

	if (ret == 0) {
		vpn_stats_update_keepalive_tx(ctx);
	} else {
		net_pkt_unref(pkt);
	}

	return ret;
}

int wireguard_peer_keepalive(int peer_id)
{
	struct wg_peer *peer, *next;
	int ret = -ENOENT;

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_peers, peer, next, node) {

		/* Check if we need to do a rekey or send a handshake */
		if (peer->id == peer_id) {
			ret = wg_send_keepalive(peer->ctx, peer);
			if (ret < 0) {
				NET_DBG("Cannot send keepalive (%d)", ret);
			}

			break;
		}
	}

	k_mutex_unlock(&lock);

	return ret;
}

static void wg_periodic_timer(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct wg_peer *peer, *next;
	int ret;

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_peers, peer, next, node) {
		if (!net_if_is_up(peer->ctx->iface)) {
			continue;
		}

		/* Check if we need to do a rekey or send a handshake */
		if (should_reset_keypair(&peer->session.keypair.current)) {
			keypair_destroy(&peer->session.keypair.current);
			keypair_destroy(&peer->session.keypair.next);
			keypair_destroy(&peer->session.keypair.prev);
		}

		if (should_destroy_keypair(&peer->session.keypair.current)) {
			keypair_destroy(&peer->session.keypair.current);
		}

		if (should_send_keepalive(peer)) {
			(void)wg_send_keepalive(peer->ctx, peer);
		}

		if (should_send_init(peer)) {
			start_handshake(peer->ctx, peer);
		}
	}

	k_mutex_unlock(&lock);

	ret = k_work_reschedule(dwork, K_MSEC(WG_TIMER_PERIOD));
	if (ret < 0) {
		NET_DBG("Cannot schedule %s work (%d)", "periodic", ret);
	}
}

static uint16_t get_port(struct net_sockaddr *addr)
{
	uint16_t local_port;
	int max_count = 10;

	do {
		local_port = sys_rand16_get() | 0x8000;

		if (--max_count < 0) {
			NET_ERR("Cannot get Wireguard service port");
			local_port = 0;
			break;
		}
	} while (net_context_port_in_use(NET_IPPROTO_UDP, local_port, addr));

	return local_port;
}

static void crypto_init(struct wg_context *ctx)
{
	blake2s_ctx bl_ctx;

	wireguard_blake2s_init(&bl_ctx, WG_HASH_LEN, NULL, 0);
	wireguard_blake2s_update(&bl_ctx, CONSTRUCTION, sizeof(CONSTRUCTION));
	wireguard_blake2s_final(&bl_ctx, ctx->construction_hash);

	wireguard_blake2s_init(&bl_ctx, WG_HASH_LEN, NULL, 0);
	wireguard_blake2s_update(&bl_ctx, ctx->construction_hash,
				 sizeof(ctx->construction_hash));
	wireguard_blake2s_update(&bl_ctx, IDENTIFIER, sizeof(IDENTIFIER));
	wireguard_blake2s_final(&bl_ctx, ctx->identifier_hash);
}

static int wireguard_init(void)
{
	struct net_sockaddr local_addr = { 0 };
	const struct device *dev;
	struct wg_context *ctx;
	uint16_t port;
	int ret;

	dev = device_get_binding(WIREGUARD_CTRL_DEVICE);
	if (dev == NULL) {
		NET_DBG("No such device %s found, Wireguard is disabled!",
			WIREGUARD_CTRL_DEVICE);
		return -ENOENT;
	}

	ctx = dev->data;

	for (int i = 0; i < ARRAY_SIZE(peers); i++) {
		sys_slist_prepend(&peer_list, &peers[i].node);
	}

	if (WIREGUARD_INTERFACE[0] != '\0') {
		ret = net_if_get_by_name(WIREGUARD_INTERFACE);
		if (ret < 0) {
			NET_ERR("Cannot find interface \"%s\" (%d)",
				WIREGUARD_INTERFACE, ret);
			return -ENOENT;
		}

		ctx->ifindex = ret;
	}

	crypto_init(ctx);

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		local_addr.sa_family = NET_AF_INET6;

		/* Note that if IPv4 is enabled, then v4-to-v6-mapping option
		 * is set and the system will use the IPv6 socket to provide
		 * IPv4 connectivity.
		 */
	} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
		local_addr.sa_family = NET_AF_INET;
	}

	if (CONFIG_WIREGUARD_PORT > 0) {
		port = CONFIG_WIREGUARD_PORT;
	} else {
		port = get_port(&local_addr);
		if (port == 0) {
			port = WG_DEFAULT_PORT;
		}

		NET_INFO("Wireguard service port %d", port);
	}

	ret = net_udp_register(local_addr.sa_family,
			       NULL,
			       &local_addr,
			       0,
			       port,
			       NULL,
			       wg_input,
			       ctx,
			       NULL);
	if (ret < 0) {
		NET_ERR("Cannot register Wireguard service handler (%d)", ret);
		return ret;
	}

	ctx->port = port;

	net_mgmt_init_event_callback(&ctx->wg_mgmt_cb, wg_iface_event_handler,
				     NET_EVENT_IF_DOWN | NET_EVENT_IF_UP);
	net_mgmt_add_event_callback(&ctx->wg_mgmt_cb);

	k_work_init_delayable(&ctx->wg_periodic_timer, wg_periodic_timer);

	return 0;
}

static void wg_ctrl_iface_init(struct net_if *iface)
{
	struct wg_context *ctx = net_if_get_device(iface)->data;
	int ret;

	ctx->iface = iface;

	ret = net_if_set_name(iface, "wg_ctrl");
	if (ret < 0) {
		NET_DBG("Cannot set interface name (%d)", ret);
	}

	/* The control interface is turned off by default, and it will
	 * turn on after the first VPN connection. User is not able to
	 * manually turn the control interface up if there are no VPN
	 * connections configured.
	 */
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	net_if_flag_set(iface, NET_IF_IPV6_NO_MLD);
	net_if_flag_clear(iface, NET_IF_IPV6);
	net_if_flag_clear(iface, NET_IF_IPV4);
}

static int handle_handshake_init(struct wg_peer *peer,
				 struct net_sockaddr *peer_addr,
				 struct net_sockaddr *my_addr,
				 struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_DEFINE(access, struct msg_handshake_init);
	struct msg_handshake_init *msg;
	bool status;
	int ret;

	msg = (struct msg_handshake_init *)net_pkt_get_data(pkt, &access);
	if (msg == NULL) {
		return -EINVAL;
	}

	NET_DBG("Received handshake initiation from %s",
		net_sprint_addr(peer_addr->sa_family,
				(const void *)&net_sin(peer_addr)->sin_addr));

	status = wg_check_initiation_message(peer->ctx, msg, peer_addr);
	if (status) {
		vpn_stats_update_handshake_init_rx(peer->ctx);

		peer = wg_process_initiation_message(peer->ctx, msg);
		if (peer) {
			memcpy(&peer->endpoint, peer_addr, sizeof(peer->endpoint));
			peer->ctx->peer = peer;

			wg_send_handshake_response(peer->ctx,
						   net_pkt_vpn_iface(pkt),
						   peer,
						   my_addr);
			ret = 0;
		} else {
			NET_DBG("Peer not found for handshake initiation");
			ret = -ENOENT;
		}
	} else {
		NET_DBG("Invalid handshake initiation message");
		ret = -EINVAL;
	}

	return ret;
}

static int handle_handshake_response(struct wg_peer *peer,
				     struct net_sockaddr *peer_addr,
				     struct net_sockaddr *my_addr,
				     struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_DEFINE(access, struct msg_handshake_response);
	struct msg_handshake_response *msg;
	bool ret;

	msg = (struct msg_handshake_response *)net_pkt_get_data(pkt, &access);
	if (msg == NULL) {
		return -EINVAL;
	}

	ret = wg_check_response_message(peer->ctx, msg, peer_addr);
	if (!ret) {
		return -EINVAL;
	}

	if (peer != peer_lookup_by_handshake(peer->ctx, msg->receiver)) {
		return -ENOENT;
	}

	wg_process_response_message(peer->ctx, peer, msg, peer_addr);

	vpn_stats_update_handshake_resp_rx(peer->ctx);

	return 0;
}

static void update_peer_addr(struct wg_peer *peer, struct net_sockaddr *peer_addr)
{
	memcpy(&peer->endpoint, peer_addr, sizeof(peer->endpoint));
}

static int handle_cookie_reply(struct wg_peer *peer,
			       struct net_sockaddr *peer_addr,
			       struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_DEFINE(access, struct msg_cookie_reply);
	struct msg_cookie_reply *msg;
	bool ret;

	msg = (struct msg_cookie_reply *)net_pkt_get_data(pkt, &access);
	if (msg == NULL) {
		return -EINVAL;
	}

	if (peer != peer_lookup_by_handshake(peer->ctx, msg->receiver)) {
		return -ENOENT;
	}

	ret = wg_process_cookie_message(peer->ctx, peer, msg);
	if (!ret) {
		return -EINVAL;
	}

	update_peer_addr(peer, peer_addr);

	return 0;
}

static int handle_transport_data(struct wg_peer *peer,
				 struct net_sockaddr *peer_addr,
				 struct net_pkt *pkt,
				 size_t ip_udp_hdr_len,
				 size_t data_len)
{
	NET_PKT_DATA_ACCESS_DEFINE(access, struct msg_transport_data);
	struct msg_transport_data *msg;

	msg = (struct msg_transport_data *)net_pkt_get_data(pkt, &access);
	if (msg == NULL) {
		return -EINVAL;
	}

	peer = peer_lookup_by_receiver(peer->ctx, msg->receiver);
	if (peer == NULL) {
		return -ENOENT;
	}

	return wg_process_data_message(peer->ctx, peer, msg, pkt,
				       ip_udp_hdr_len, peer_addr);
}

static struct wg_peer *peer_lookup_by_iface(struct net_if *iface)
{
	struct wg_peer *peer, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_peers, peer, next, node) {
		if (peer->ctx->iface == iface) {
			return peer;
		}
	}

	return NULL;
}

static struct wg_peer *peer_lookup_by_virtual_iface(struct net_if *iface)
{
	struct virtual_interface_context *ctx, *tmp;
	const struct virtual_interface_api *api;
	struct wg_peer *peer, *next;
	sys_slist_t *interfaces;

	interfaces = &iface->config.virtual_interfaces;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(interfaces, ctx, tmp, node) {
		if (ctx->virtual_iface == NULL) {
			continue;
		}

		api = net_if_get_device(ctx->virtual_iface)->api;
		if (api == NULL || api->recv == NULL) {
			continue;
		}

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_peers, peer, next, node) {
			if (peer->ctx->iface == ctx->virtual_iface) {
				return peer;
			}
		}
	}

	return NULL;
}

static enum net_verdict wg_ctrl_recv(struct net_if *iface, struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_DEFINE(wg_access, struct wg_msg_hdr);
	enum net_verdict verdict = NET_DROP;
	struct net_sockaddr_storage my_addr_s = { 0 };
	struct net_sockaddr *my_addr = (struct net_sockaddr *)&my_addr_s;
	struct net_sockaddr_storage addr_s = { 0 };
	struct net_sockaddr *addr = (struct net_sockaddr *)&addr_s;
	union net_proto_header *udp_hdr;
	union net_ip_header *ip_hdr;
	struct wg_msg_hdr *hdr;
	struct wg_peer *peer;
	size_t len, hdr_len;
	int ret;

	if (pkt->buffer == NULL) {
		goto drop;
	}

	len = net_pkt_get_len(pkt);

	ip_hdr = net_pkt_vpn_ip_hdr(pkt);
	udp_hdr = net_pkt_vpn_udp_hdr(pkt);

	if (net_pkt_family(pkt) == NET_AF_INET) {
		if (len < NET_IPV4UDPH_LEN + sizeof(struct wg_msg_hdr)) {
			NET_DBG("DROP: Too short Wireguard header");
			goto drop;
		}

		net_pkt_cursor_init(pkt);

		hdr_len = net_pkt_ip_hdr_len(pkt) + NET_UDPH_LEN;

		if (net_pkt_skip(pkt, hdr_len)) {
			NET_DBG("DROP: Too short %s packet", "IPv4");
			goto drop;
		}

		memcpy(&net_sin(addr)->sin_addr, &ip_hdr->ipv4->src,
		       sizeof(struct net_in_addr));
		net_sin(addr)->sin_port = udp_hdr->udp->src_port;
		addr->sa_family = NET_AF_INET;

		memcpy(&net_sin(my_addr)->sin_addr, &ip_hdr->ipv4->dst,
		       sizeof(struct net_in_addr));
		net_sin(my_addr)->sin_port = udp_hdr->udp->dst_port;
		net_sin(my_addr)->sin_family = NET_AF_INET;

	} else if (net_pkt_family(pkt) == NET_AF_INET6) {
		if (len < NET_IPV6UDPH_LEN + sizeof(struct wg_msg_hdr)) {
			NET_DBG("DROP: Too short Wireguard header");
			goto drop;
		}

		net_pkt_cursor_init(pkt);

		hdr_len = net_pkt_ip_hdr_len(pkt) + NET_UDPH_LEN;

		if (net_pkt_skip(pkt, hdr_len)) {
			NET_DBG("DROP: Too short %s packet", "IPv6");
			goto drop;
		}

		memcpy(&net_sin6(addr)->sin6_addr, &ip_hdr->ipv6->src,
		       sizeof(struct net_in6_addr));
		net_sin6(addr)->sin6_port = udp_hdr->udp->src_port;
		addr->sa_family = NET_AF_INET6;

		memcpy(&net_sin6(my_addr)->sin6_addr, &ip_hdr->ipv6->dst,
		       sizeof(struct net_in6_addr));
		net_sin6(my_addr)->sin6_port = udp_hdr->udp->dst_port;
		net_sin6(my_addr)->sin6_family = NET_AF_INET6;
	} else {
		return -EAFNOSUPPORT;
	}

	hdr = (struct wg_msg_hdr *)net_pkt_get_data(pkt, &wg_access);
	if (hdr == NULL) {
		NET_DBG("DROP: NULL Wireguard header");
		goto drop;
	}

	if (!(hdr->reserved[0] == 0 && hdr->reserved[1] == 0 &&
	      hdr->reserved[2] == 0)) {
		NET_DBG("DROP: Invalid Wireguard header");
		goto drop;
	}

	/* At this point we don't know if this came from a valid peer */
	peer = peer_lookup_by_virtual_iface(iface);
	if (peer == NULL) {
		NET_DBG("DROP: Peer not found for interface %d",
			net_if_get_by_iface(iface));
		goto drop;
	}

	if (peer->ctx == NULL) {
		NET_DBG("Invalid configuration");
		goto drop;
	}

	if (hdr->type == MESSAGE_HANDSHAKE_INITIATION) {
		ret = handle_handshake_init(peer, addr, my_addr, pkt);
		if (ret < 0) {
			NET_DBG("DROP: Invalid %s Wireguard header (%d)",
				"handshake init", ret);
			goto drop;
		}

		ret = k_work_schedule(&peer->ctx->wg_ctx->wg_periodic_timer,
				      K_MSEC(WG_TIMER_PERIOD));
		if (ret < 0) {
			NET_DBG("Cannot schedule %s work (%d)", "periodic", ret);
		}

	} else if (hdr->type == MESSAGE_HANDSHAKE_RESPONSE) {
		ret = handle_handshake_response(peer, addr, my_addr, pkt);
		if (ret < 0) {
			NET_DBG("DROP: Invalid %s Wireguard header (%d)",
				"handshake response", ret);
			goto drop;
		}
	} else if (hdr->type == MESSAGE_COOKIE_REPLY) {
		ret = handle_cookie_reply(peer, addr, pkt);
		if (ret < 0) {
			NET_DBG("DROP: Invalid %s Wireguard header (%d)",
				"cookie reply", ret);
			goto drop;
		}
	} else if (hdr->type == MESSAGE_TRANSPORT_DATA) {
		ret = handle_transport_data(peer, addr, pkt, hdr_len, len - hdr_len);
		if (ret < 0) {
			NET_DBG("DROP: Invalid %s Wireguard header (%d)",
				"transport data", ret);
			goto drop;
		}
	} else {
		NET_DBG("DROP: Invalid %s Wireguard header", "message type");
		goto drop;
	}

	verdict = NET_OK;
	net_pkt_unref(pkt);
drop:
	return verdict;
}

static int wg_ctrl_send(const struct device *dev, struct net_pkt *pkt)
{
	struct wg_context *ctx = dev->data;

	net_stats_update_bytes_sent(ctx->iface, net_pkt_get_len(pkt));

	net_pkt_set_iface(pkt, net_pkt_vpn_iface(pkt));

	if (DEBUG_TX) {
		char str[sizeof("TX ctrl iface xx to xx")];

		snprintk(str, sizeof(str), "TX ctrl iface %d to %d",
			 net_if_get_by_iface(net_if_lookup_by_dev(dev)),
			 net_if_get_by_iface(net_pkt_iface(pkt)));

		net_pkt_hexdump(pkt, str);
	}

	return net_send_data(pkt);
}

static int wg_ctrl_start(const struct device *dev)
{
	struct wg_context *ctx = dev->data;
	int ret = 0;

	if (ctx->status) {
		return -EALREADY;
	}

	if (sys_slist_is_empty(&active_peers)) {
		NET_DBG("No active peers found. Interface stays disabled.");
		return -ENODATA;
	}

	ctx->status = true;

	NET_DBG("Starting iface %d", net_if_get_by_iface(ctx->iface));

	return ret;
}

static int wg_ctrl_stop(const struct device *dev)
{
	struct wg_context *ctx = dev->data;

	if (!ctx->status) {
		return -EALREADY;
	}

	ctx->status = false;

	NET_DBG("Stopping iface %d", net_if_get_by_iface(ctx->iface));

	return 0;
}

static struct dummy_api wg_api = {
	.iface_api.init = wg_ctrl_iface_init,
	.recv = wg_ctrl_recv,
	.send = wg_ctrl_send,
	.start = wg_ctrl_start,
	.stop = wg_ctrl_stop,
};

/* Wireguard control interface is a dummy network interface that just
 * encrypts sent data and decrypts received data, and acts as a middle
 * man between the real network interface and the virtual network
 * interface (like wg0) where the application reads/writes the data.
 */
NET_DEVICE_INIT(wireguard,
		WIREGUARD_CTRL_DEVICE,
		NULL,    /* init fn */
		NULL,    /* pm */
		&wg_ctx, /* data */
		NULL,    /* config */
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&wg_api,
		DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2),
		WG_MTU);

static void iface_init(struct net_if *iface)
{
	struct wg_iface_context *ctx = net_if_get_device(iface)->data;

	if (ctx->init_done) {
		return;
	}

	ctx->iface = iface;

	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
	net_if_flag_set(iface, NET_IF_POINTOPOINT);
	(void)net_if_set_name(iface, ctx->name);

	(void)net_virtual_set_name(iface, "Wireguard VPN");
	(void)net_virtual_set_flags(iface, NET_L2_POINT_TO_POINT);

	ctx->init_done = true;
}

static enum virtual_interface_caps get_capabilities(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return VIRTUAL_INTERFACE_VPN;
}

static int interface_start(const struct device *dev)
{
	struct wg_iface_context *ctx = dev->data;
	int ret = 0;

	if (ctx->status) {
		return -EALREADY;
	}

	ctx->status = true;

	NET_DBG("Starting iface %d", net_if_get_by_iface(ctx->iface));

	return ret;
}

static int interface_stop(const struct device *dev)
{
	struct wg_iface_context *ctx = dev->data;

	if (!ctx->status) {
		return -EALREADY;
	}

	ctx->status = false;

	NET_DBG("Stopping iface %d", net_if_get_by_iface(ctx->iface));

	return 0;
}

static int interface_attach(struct net_if *iface, struct net_if *lower_iface)
{
	struct wg_iface_context *ctx;

	if (net_if_get_by_iface(iface) < 0) {
		return -ENOENT;
	}

	ctx = net_if_get_device(iface)->data;
	ctx->attached_to = lower_iface;

	return 0;
}

static int interface_send(struct net_if *iface, struct net_pkt *pkt)
{
	struct wg_iface_context *ctx = net_if_get_device(iface)->data;
	struct msg_transport_data hdr = { 0 };
	struct net_sockaddr_storage src_addr;
	struct net_pkt *pkt_encrypted;
	struct net_if *target_iface;
	struct net_buf *buf;
	const struct dummy_api *api;
	struct wg_keypair *keypair;
	struct wg_peer *peer;
	size_t pkt_len, copied, padded_len;
	int ret;

	if (ctx->attached_to == NULL) {
		return -ENOENT;
	}

	if (net_pkt_family(pkt) != NET_AF_INET &&
	    net_pkt_family(pkt) != NET_AF_INET6) {
		return -EINVAL;
	}

	if (DEBUG_TX) {
		char str[sizeof("TX iface xx")];

		snprintk(str, sizeof(str), "TX iface %d",
			 net_if_get_by_iface(net_pkt_iface(pkt)));

		net_pkt_hexdump(pkt, str);
	}

	/* Encrypt the packet and send it to net via control interface */

	peer = ctx->peer;
	if (peer == NULL) {
		peer = peer_lookup_by_iface(iface);
		if (peer == NULL) {
			NET_DBG("No peer found for iface %d", net_if_get_by_iface(iface));
			return -ENOENT;
		}

		NET_DBG("Peer %d found for iface %d", peer->id, net_if_get_by_iface(iface));

		/* If the peer is not yet active, then we need to start the
		 * handshake with the peer.
		 */
		if (peer->last_tx == 0) {
			peer->send_handshake = true;
			memcpy(&peer->endpoint, &peer->cfg_endpoint, sizeof(peer->endpoint));
			start_handshake(ctx, peer);

			peer->last_tx = sys_clock_tick_get_32();

			return -EAGAIN;
		}
	}

	keypair = &peer->session.keypair.current;

	if (keypair->is_valid && !keypair->is_initiator && keypair->last_rx == 0) {
		keypair = &peer->session.keypair.prev;
	}

	if (!(keypair->is_valid && (keypair->is_initiator || keypair->last_rx != 0))) {
		/* Keys are invalid. */
		vpn_stats_update_invalid_key(ctx);
		return -ENOTCONN;
	}

	if (sys_timepoint_expired(keypair->expires) ||
	    keypair->sending_counter >= REJECT_AFTER_MESSAGES) {
		/* Keys are expired. */
		keypair_destroy(keypair);
		vpn_stats_update_key_expired(ctx);
		return -EKEYEXPIRED;
	}

	pkt_len = net_pkt_get_len(pkt);
	padded_len = ROUND_UP(pkt_len, 16U);

	buf = net_buf_alloc(&msg_pool, BUF_ALLOC_TIMEOUT);
	if (buf == NULL) {
		NET_DBG("Failed to allocate %s buffer", "encrypt");
		vpn_stats_update_alloc_failed(ctx);
		return -ENOMEM;
	}

	memset(buf->data, 0, buf->size);

	hdr.type = MESSAGE_TRANSPORT_DATA;
	hdr.receiver = keypair->remote_index;
	sys_put_le64(keypair->sending_counter, (uint8_t *)&hdr.counter);

	net_buf_add_mem(buf, &hdr, sizeof(hdr));

	/* Linearize the packet and place it after the header */
	copied = net_buf_linearize(buf->data + sizeof(hdr), pkt_len,
				   pkt->buffer, 0, pkt_len);
	if (copied != pkt_len) {
		net_buf_unref(buf);
		vpn_stats_update_alloc_failed(ctx);
		return -EMSGSIZE;
	}

	wg_encrypt_packet(buf->data + sizeof(hdr), buf->data + sizeof(hdr),
			  padded_len, keypair);
	net_buf_add(buf, padded_len + WG_AUTHTAG_LEN);

	/* Figure out the network interface where the packet is sent */
	if (ctx->wg_ctx->ifindex == 0) {
		/* If the interface is not set, then we figure out the
		 * interface based on the packet destination address.
		 */
		if (IS_ENABLED(CONFIG_NET_IPV6) && peer->endpoint.ss_family == NET_AF_INET6) {
			const struct net_in6_addr *src;

			target_iface = net_if_ipv6_select_src_iface_addr(
				&net_sin6(net_sad(&peer->endpoint))->sin6_addr, &src);

			net_ipv6_addr_copy_raw(
				(uint8_t *)(&net_sin6(net_sad(&src_addr))->sin6_addr),
				(const uint8_t *)src);
			src_addr.ss_family = NET_AF_INET6;

		} else if (IS_ENABLED(CONFIG_NET_IPV4) && peer->endpoint.ss_family == NET_AF_INET) {
			const struct net_in_addr *src;

			target_iface = net_if_ipv4_select_src_iface_addr(
				&net_sin(net_sad(&peer->endpoint))->sin_addr, &src);

			net_ipv4_addr_copy_raw(
				(uint8_t *)(&net_sin(net_sad(&src_addr))->sin_addr),
				(const uint8_t *)src);
			src_addr.ss_family = NET_AF_INET;

		} else {
			NET_DBG("Unknown address family %d", peer->endpoint.ss_family);
			net_buf_unref(buf);
			return -EAFNOSUPPORT;
		}

		iface = target_iface;
	} else {
		iface = net_if_get_by_index(ctx->wg_ctx->ifindex);

		if (IS_ENABLED(CONFIG_NET_IPV6) && peer->endpoint.ss_family == NET_AF_INET6) {
			const struct net_in6_addr *src;

			src = net_if_ipv6_select_src_addr(iface,
					&net_sin6(net_sad(&peer->endpoint))->sin6_addr);

			net_ipv6_addr_copy_raw(
				(uint8_t *)(&net_sin6(net_sad(&src_addr))->sin6_addr),
				(const uint8_t *)src);
			src_addr.ss_family = NET_AF_INET6;

		} else if (IS_ENABLED(CONFIG_NET_IPV4) && peer->endpoint.ss_family == NET_AF_INET) {
			const struct net_in_addr *src;

			src = net_if_ipv4_select_src_addr(iface,
					&net_sin(net_sad(&peer->endpoint))->sin_addr);

			net_ipv4_addr_copy_raw(
				(uint8_t *)(&net_sin(net_sad(&src_addr))->sin_addr),
				(const uint8_t *)src);
			src_addr.ss_family = NET_AF_INET;
		}
	}

	net_sin(net_sad(&src_addr))->sin_port = net_htons(ctx->wg_ctx->port);

	/* Create a new packet and send it to the control interface */
	ret = create_packet(iface, net_sad(&src_addr), net_sad(&peer->endpoint),
			    buf->data, sizeof(hdr) + padded_len + WG_AUTHTAG_LEN,
			    0, 0, &pkt_encrypted);

	/* We do not need the temp buffer any more */
	net_buf_unref(buf);

	if (ret < 0) {
		vpn_stats_update_alloc_failed(ctx);
		return ret;
	}

	net_pkt_set_vpn_iface(pkt_encrypted, iface);

	api = net_if_get_device(ctx->attached_to)->api;

	ret = net_l2_send(api->send, net_if_get_device(ctx->attached_to),
			  iface, pkt_encrypted);
	if (ret < 0) {
		net_pkt_unref(pkt_encrypted);
		vpn_stats_update_drop_tx(ctx);
		goto out;
	}

	vpn_stats_update_valid_tx(ctx);

	peer->last_tx = keypair->last_tx = sys_clock_tick_get_32();

	if (peer->keepalive_interval > 0) {
		peer->keepalive_expires =
			sys_timepoint_calc(K_SECONDS(peer->keepalive_interval));
	}

	/* Rekey if needed */
	if (keypair->sending_counter >= REKEY_AFTER_MESSAGES) {
		peer->send_handshake = true;
	} else if (keypair->is_initiator && sys_timepoint_expired(keypair->expires)) {
		peer->send_handshake = true;
	}

	ret = 0;

	net_pkt_unref(pkt);
out:
	return ret;
}

static enum net_verdict interface_recv(struct net_if *iface,
				       struct net_pkt *pkt)
{
	uint8_t vtc_vhl;

	if (DEBUG_RX) {
		char str[sizeof("RX iface xx")];

		snprintk(str, sizeof(str), "RX iface %d",
			 net_if_get_by_iface(iface));

		net_pkt_hexdump(pkt, str);
	}

	net_pkt_set_l2_processed(pkt, true);
	net_pkt_set_loopback(pkt, false);

	/* IP version and header length. */
	vtc_vhl = NET_IPV6_HDR(pkt)->vtc & 0xf0;

	if (IS_ENABLED(CONFIG_NET_IPV6) && vtc_vhl == 0x60) {
		net_pkt_set_family(pkt, NET_AF_INET6);
		return net_ipv6_input(pkt);

	} else if (IS_ENABLED(CONFIG_NET_IPV4) && vtc_vhl == 0x40) {
		net_pkt_set_family(pkt, NET_AF_INET);
		return net_ipv4_input(pkt);
	}

	NET_DBG("Unknown IP family packet (0x%x)", vtc_vhl);

	net_stats_update_ip_errors_protoerr(iface);
	net_stats_update_ip_errors_vhlerr(iface);

	return NET_DROP;
}

static int init_iface_context(struct wg_iface_context *ctx,
			      const struct virtual_interface_config *config)
{
	bool ret;

	if (config->private_key.len != WG_PRIVATE_KEY_LEN) {
		NET_DBG("Invalid private key length, was %zu expected %zu",
			config->private_key.len, WG_PRIVATE_KEY_LEN);
		return -EINVAL;
	}

	/* Generate public key from the private key */
	memcpy(ctx->private_key, config->private_key.data, WG_PRIVATE_KEY_LEN);

	/* Private key needs to be clamped */
	wg_clamp_private_key(ctx->private_key);

	ret = wg_generate_public_key(ctx->public_key, ctx->private_key);
	if (!ret) {
		crypto_zero(ctx->private_key, WG_PRIVATE_KEY_LEN);

		NET_DBG("Public key generation failed");

		return 0;
	}

	wg_generate_cookie_secret(ctx, COOKIE_SECRET_MAX_AGE_MSEC);

	/* 5.4.4 Cookie MACs - The value Hash(Label-Mac1 || Spubm' )
	 * above can be pre-computed.
	 */
	wg_mac_key(ctx->label_mac1_key, ctx->public_key, LABEL_MAC1,
		   sizeof(LABEL_MAC1));

	/* 5.4.7 Under Load: Cookie Reply Message - The value
	 * Hash(Label-Cookie || Spubm) above can be pre-computed.
	 */
	wg_mac_key(ctx->label_cookie_key, ctx->public_key, LABEL_COOKIE,
		   sizeof(LABEL_COOKIE));

	return 0;
}

static int interface_set_config(struct net_if *iface,
				enum virtual_interface_config_type type,
				const struct virtual_interface_config *config)
{
	struct wg_iface_context *ctx = net_if_get_device(iface)->data;

	switch (type) {
	case VIRTUAL_INTERFACE_CONFIG_TYPE_PRIVATE_KEY:
		return init_iface_context(ctx, config);

	case VIRTUAL_INTERFACE_CONFIG_TYPE_MTU:
		NET_DBG("Interface %d MTU set to %d",
			net_if_get_by_iface(iface), config->mtu);
		net_if_set_mtu(iface, config->mtu);
		return 0;

	default:
		break;
	}

	return -ENOTSUP;
}

BUILD_ASSERT(NET_VIRTUAL_MAX_PUBLIC_KEY_LEN >= WG_PUBLIC_KEY_LEN,
	     "Public key length is too small");

static int interface_get_config(struct net_if *iface,
				enum virtual_interface_config_type type,
				struct virtual_interface_config *config)
{
	struct wg_iface_context *ctx = net_if_get_device(iface)->data;

	switch (type) {
	case VIRTUAL_INTERFACE_CONFIG_TYPE_PUBLIC_KEY:
		memcpy(config->public_key.data, ctx->public_key,
		       sizeof(ctx->public_key));
		config->public_key.len = sizeof(ctx->public_key);
		return 0;

	default:
		break;
	}

	return -ENOTSUP;
}

static const struct virtual_interface_api wg_iface_api = {
	.iface_api.init = iface_init,

	.get_capabilities = get_capabilities,
	.start = interface_start,
	.stop = interface_stop,
	.attach = interface_attach,
	.send = interface_send,
	.recv = interface_recv,
	.set_config = interface_set_config,
	.get_config = interface_get_config,
};

#define WG_INTERFACE_INIT(x, _)						\
	static struct wg_iface_context wg_iface_context_data_##x = {	\
		.name =	"wg" #x,					\
		.wg_ctx = &wg_ctx,					\
	};								\
									\
	NET_VIRTUAL_INTERFACE_INIT_INSTANCE(wg_##x,			\
					    "WIREGUARD" #x,		\
					    x,				\
					    NULL,			\
					    NULL,			\
					    &wg_iface_context_data_##x,	\
					    NULL, /* config */		\
					    CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
					    &wg_iface_api,		\
					    WG_MTU)

LISTIFY(CONFIG_WIREGUARD_MAX_PEER, WG_INTERFACE_INIT, (;), _);

static int create_ipv4_packet(struct net_if *iface,
			      struct net_sockaddr *src,
			      struct net_sockaddr *dst,
			      uint8_t *packet,
			      size_t packet_len,
			      uint8_t dscp,
			      uint8_t ecn,
			      struct net_pkt **reply_pkt)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(iface,
					NET_UDPH_LEN + packet_len,
					NET_AF_INET, NET_IPPROTO_UDP,
					PKT_ALLOC_WAIT_TIME);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	net_pkt_set_ip_dscp(pkt, dscp);
	net_pkt_set_ip_ecn(pkt, ecn);

	ret = net_ipv4_create(pkt, &net_sin(src)->sin_addr,
			      &net_sin(dst)->sin_addr);
	if (ret < 0) {
		goto drop;
	}

	*reply_pkt = pkt;

	return 0;

drop:
	net_pkt_unref(pkt);
	return ret;
}

static int create_ipv6_packet(struct net_if *iface,
			      struct net_sockaddr *src,
			      struct net_sockaddr *dst,
			      uint8_t *packet,
			      size_t packet_len,
			      struct net_pkt **reply_pkt)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(iface,
					NET_UDPH_LEN + packet_len,
					NET_AF_INET6, NET_IPPROTO_UDP,
					PKT_ALLOC_WAIT_TIME);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	ret = net_ipv6_create(pkt, &net_sin6(src)->sin6_addr,
			      &net_sin6(dst)->sin6_addr);
	if (ret < 0) {
		goto drop;
	}

	*reply_pkt = pkt;

	return 0;
drop:
	net_pkt_unref(pkt);
	return ret;
}

static int create_packet(struct net_if *iface,
			 struct net_sockaddr *src,
			 struct net_sockaddr *dst,
			 uint8_t *packet,
			 size_t packet_len,
			 uint8_t dscp,
			 uint8_t ecn,
			 struct net_pkt **pkt)
{
	int ret;

	if (IS_ENABLED(CONFIG_NET_IPV4) && dst->sa_family == NET_AF_INET) {
		ret = create_ipv4_packet(iface,
					 src,
					 dst,
					 packet,
					 packet_len,
					 dscp,
					 ecn,
					 pkt);

	} else if (IS_ENABLED(CONFIG_NET_IPV6) && dst->sa_family == NET_AF_INET6) {
		ret = create_ipv6_packet(iface,
					 src,
					 dst,
					 packet,
					 packet_len,
					 pkt);
	} else {
		ret = -ENOTSUP;
		goto out;
	}

	if (ret < 0) {
		NET_DBG("Cannot create packet (%d)", ret);
		goto out;
	}

	ret = net_udp_create(*pkt, net_sin(src)->sin_port,
			     net_sin(dst)->sin_port);
	if (ret < 0) {
		NET_DBG("Cannot create UDP header");
		goto out;
	}

	if (packet_len > 0) {
		net_pkt_write(*pkt, packet, packet_len);
	}

	net_pkt_cursor_init(*pkt);

	net_pkt_set_iface(*pkt, iface);

	if (IS_ENABLED(CONFIG_NET_IPV4) && dst->sa_family == NET_AF_INET) {
		net_ipv4_finalize(*pkt, NET_IPPROTO_UDP);
	} else if (IS_ENABLED(CONFIG_NET_IPV6) && dst->sa_family == NET_AF_INET6) {
		net_ipv6_finalize(*pkt, NET_IPPROTO_UDP);
	}

	ret = 0;
out:
	return ret;
}

/* Send data directly to wg control interface and not through net stack.
 * This is because the control interface does not have IPv4/IPv6 configured,
 * so if we send it through the net stack, it will try do extra checks like
 * TTL, which is 0 for the control interface and packet gets dropped.
 */
static int send_data(struct wg_iface_context *ctx, struct net_pkt *pkt)
{
	const struct dummy_api *api = net_if_get_device(ctx->attached_to)->api;

	return net_l2_send(api->send, net_if_get_device(ctx->attached_to),
			   ctx->attached_to, pkt);
}

static int wg_send_handshake_init(struct wg_iface_context *ctx,
				  struct net_if *iface,
				  struct wg_peer *peer,
				  struct msg_handshake_init *packet)
{
	struct net_sockaddr_storage my_addr = { 0 };
	struct net_sockaddr *addr = (struct net_sockaddr *)&my_addr;
	struct net_pkt *pkt = NULL;
	struct net_if *target_iface;
	int ret;

	ret = select_target_iface(peer, addr, &peer->endpoint, &target_iface);
	if (ret < 0) {
		NET_DBG("Unknown address family %d", peer->endpoint.ss_family);
		vpn_stats_update_invalid_ip_family(ctx);
		return -EAFNOSUPPORT;
	}

	/* No point in sending the packet to the same interface */
	if (iface == target_iface) {
		target_iface = net_if_get_default();
	}

	ret = create_packet(target_iface, addr, net_sad(&peer->endpoint), (uint8_t *)packet,
			    sizeof(*packet), NET_IPV4_DSCP_AF41, 0, &pkt);
	if (ret < 0) {
		NET_DBG("Packet creation failed (%d)", ret);
		vpn_stats_update_alloc_failed(ctx);
		return -ENOMEM;
	}

	net_pkt_set_vpn_iface(pkt, target_iface);
	net_pkt_set_iface(pkt, target_iface);

	NET_DBG("Sending handshake %s from %s:%d to %s:%d", "init",
		net_sprint_addr(my_addr.ss_family,
				(const void *)(&net_sin(addr)->sin_addr)),
		net_ntohs(net_sin(addr)->sin_port),
		net_sprint_addr(peer->endpoint.ss_family,
				(const void *)(&net_sin(net_sad(&peer->endpoint))->sin_addr)),
		net_ntohs(net_sin(net_sad(&peer->endpoint))->sin_port));

	if (pkt == NULL) {
		ret = -ENOMEM;
		goto drop;
	}

	ret = send_data(ctx, pkt);
	if (ret < 0) {
		goto drop;
	}

	vpn_stats_update_handshake_init_tx(ctx);

	return 0;
drop:
	if (pkt) {
		net_pkt_unref(pkt);
	}

	if (ret < 0) {
		NET_DBG("Cannot send handshake %s (%d)", "init", ret);
	}

	return ret;
}

static void wg_send_handshake_response(struct wg_iface_context *ctx,
				       struct net_if *iface,
				       struct wg_peer *peer,
				       struct net_sockaddr *my_addr)
{
	struct msg_handshake_response packet = { 0 };
	struct net_pkt *pkt = NULL;
	int ret;

	if (!wg_create_handshake_response(ctx, peer, &packet)) {
		vpn_stats_update_invalid_handshake(ctx);
		return;
	}

	ret = create_packet(iface, my_addr, net_sad(&peer->endpoint), (uint8_t *)&packet,
			    sizeof(packet), NET_IPV4_DSCP_AF41, 0, &pkt);
	if (ret < 0) {
		NET_DBG("Packet creation failed (%d)", ret);
		vpn_stats_update_alloc_failed(ctx);
		return;
	}

	wg_start_session(peer, false);

	net_pkt_set_vpn_iface(pkt, iface);

	NET_DBG("Sending handshake %s from %s:%d to %s:%d", "response",
		net_sprint_addr(my_addr->sa_family,
				(const void *)(&net_sin(my_addr)->sin_addr)),
		net_ntohs(net_sin(my_addr)->sin_port),
		net_sprint_addr(peer->endpoint.ss_family,
				(const void *)(&net_sin(net_sad(&peer->endpoint))->sin_addr)),
		net_ntohs(net_sin(net_sad(&peer->endpoint))->sin_port));

	if (pkt != NULL && send_data(ctx, pkt) < 0) {
		goto drop;
	}

	vpn_stats_update_handshake_resp_tx(ctx);
	return;
drop:
	if (pkt) {
		net_pkt_unref(pkt);
	}
}

static void wg_send_handshake_cookie(struct wg_iface_context *ctx,
				     const uint8_t *mac1,
				     uint32_t index,
				     struct net_sockaddr *addr)
{
	struct msg_cookie_reply packet;
	struct net_pkt *pkt;
	int ret;

	/* Use the port and address to calculate the cookie */
	wg_create_cookie_reply(ctx, &packet, mac1, index,
			       (uint8_t *)&net_sin(addr)->sin_port,
			       addr->sa_family == NET_AF_INET ?
			       (2U + sizeof(struct net_in_addr)) :
			       (2U + sizeof(struct net_in6_addr)));

	ret = create_packet(ctx->wg_ctx->iface, addr, net_sad(&ctx->peer->endpoint),
			    (uint8_t *)&packet, sizeof(packet), NET_IPV4_DSCP_AF41, 0,
			    &pkt);
	if (ret < 0) {
		NET_DBG("Packet creation failed (%d)", ret);
		vpn_stats_update_alloc_failed(ctx);
		return;
	}

	NET_DBG("Sending handshake %s from %s to %s", "cookie",
		net_sprint_addr(addr->sa_family,
				(const void *)(&net_sin(addr)->sin_addr)),
		net_sprint_addr(ctx->peer->endpoint.ss_family,
				(const void *)
				  (&net_sin(net_sad(&ctx->peer->endpoint))->sin_addr)));

	if (send_data(ctx, pkt) < 0) {
		goto drop;
	}

	return;
drop:
	if (pkt) {
		net_pkt_unref(pkt);
	}
}

#include "wg_crypto.c"

/* Lock must be held when calling these lookup functions */
static struct wg_peer *peer_lookup_by_pubkey(struct wg_iface_context *ctx,
					     const char *public_key)
{
	struct wg_peer *peer, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_peers, peer, next, node) {
		if ((ctx == NULL || peer->ctx == ctx) &&
		    memcmp(peer->key.public_key, public_key, WG_PUBLIC_KEY_LEN) == 0) {
			return peer;
		}
	}

	return NULL;
}

static struct wg_peer *peer_lookup_by_id(uint8_t id)
{
	struct wg_peer *peer, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_peers, peer, next, node) {
		if (peer->id == id) {
			return peer;
		}
	}

	return NULL;
}

static struct wg_peer *peer_lookup_by_receiver(struct wg_iface_context *ctx,
					       uint32_t receiver)
{
	struct wg_peer *peer, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_peers, peer, next, node) {
		if (peer->ctx == ctx &&
		    ((peer->session.keypair.current.is_valid &&
		     peer->session.keypair.current.local_index == receiver) ||
		    (peer->session.keypair.next.is_valid &&
		     peer->session.keypair.next.local_index == receiver) ||
		    (peer->session.keypair.prev.is_valid &&
		     peer->session.keypair.prev.local_index == receiver))) {
			return peer;
		}
	}

	return NULL;
}

static struct wg_peer *peer_lookup_by_handshake(struct wg_iface_context *ctx,
						uint32_t receiver)
{
	struct wg_peer *peer, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_peers, peer, next, node) {
		if (peer->ctx == ctx &&
		    peer->handshake.is_valid &&
		    peer->handshake.is_initiator &&
		    peer->handshake.local_index == receiver) {
			return peer;
		}
	}

	return NULL;
}

static struct wg_keypair *get_peer_keypair_for_index(struct wg_peer *peer,
						     uint32_t idx)
{
	if (peer->session.keypair.current.is_valid &&
	    peer->session.keypair.current.local_index == idx) {
		return &peer->session.keypair.current;
	} else if (peer->session.keypair.next.is_valid &&
		   peer->session.keypair.next.local_index == idx) {
		return &peer->session.keypair.next;
	} else if (peer->session.keypair.prev.is_valid &&
		   peer->session.keypair.prev.local_index == idx) {
		return &peer->session.keypair.prev;
	}

	return NULL;
}

static bool is_index_used(struct wg_iface_context *ctx, uint32_t index)
{
	struct wg_peer *peer, *next;
	bool found = false;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_peers, peer, next, node) {
		found = (peer->ctx == ctx &&
			 (index == peer->session.keypair.current.local_index ||
			  index == peer->session.keypair.prev.local_index ||
			  index == peer->session.keypair.next.local_index ||
			  index == peer->handshake.local_index));
	}

	return found;
}

static uint32_t generate_unique_index(struct wg_iface_context *ctx)
{
	uint32_t index;

	do {
		do {
			(void)sys_csrand_get(&index, sizeof(index));
		} while ((index == 0) || (index == 0xFFFFFFFF));

	} while (is_index_used(ctx, index));

	return index;
}

static bool extract_public_key(const char *str, uint8_t *out, size_t outlen)
{
	size_t len = 0U;
	int ret;

	ret = base64_decode(out, outlen, &len, str, strlen(str));
	if (ret < 0) {
		NET_DBG("base64 decode of \"%s\" failed, olen %zd (%d)",
			str, len, ret);
		return false;
	}

	if (len != outlen) {
		NET_DBG("Invalid length %zd vs %zd", len, outlen);
		return false;
	}

	return true;
}

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct net_if **ret_iface = user_data;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		return;
	}

	if (net_virtual_get_iface_capabilities(iface) != VIRTUAL_INTERFACE_VPN) {
		return;
	}

	/* Ignore already attached interfaces */
	if (net_virtual_get_iface(iface) != NULL) {
		return;
	}

	if (*ret_iface == NULL) {
		*ret_iface = iface;
	}
}

static bool wg_peer_init(struct wg_iface_context *ctx,
			 struct wg_peer *peer,
			 const uint8_t *public_key,
			 const uint8_t *preshared_key)
{
	bool is_valid = false;
	int ret;

	memset(peer, 0, sizeof(struct wg_peer));

	if (ctx == NULL) {
		return false;
	}

	memcpy(peer->key.public_key, public_key, WG_PUBLIC_KEY_LEN);
	memset(peer->greatest_timestamp, 0, sizeof(peer->greatest_timestamp));

	if (preshared_key != NULL) {
		memcpy(peer->key.preshared, preshared_key, WG_SESSION_KEY_LEN);
	} else {
		crypto_zero(peer->key.preshared, WG_SESSION_KEY_LEN);
	}

	ret = wireguard_x25519(peer->key.public_dh, ctx->private_key,
			       peer->key.public_key);
	if (ret == 0) {
		memset(&peer->handshake, 0, sizeof(struct wg_handshake));
		peer->handshake.is_valid = false;

		peer->cookie_secret_expires =
			sys_timepoint_calc(K_MSEC(COOKIE_SECRET_MAX_AGE_MSEC));
		memset(&peer->cookie, 0, WG_COOKIE_LEN);

		wg_mac_key(peer->label_mac1_key, peer->key.public_key,
			   LABEL_MAC1, sizeof(LABEL_MAC1));
		wg_mac_key(peer->label_cookie_key, peer->key.public_key,
			   LABEL_COOKIE, sizeof(LABEL_COOKIE));
		is_valid = true;
	} else {
		NET_DBG("Cannot calculate DH public key for peer");
		crypto_zero(peer->key.public_dh, WG_PUBLIC_KEY_LEN);
	}

	return is_valid;
}

static void peer_set_allowed_addr(struct wg_peer *peer,
				  struct wireguard_peer_config *peer_config)
{
	ARRAY_FOR_EACH(peer_config->allowed_ip, i) {
		struct wireguard_allowed_ip *allowed_ip = &peer_config->allowed_ip[i];

		if (allowed_ip->is_valid) {
			peer->allowed_ip[i].is_valid = true;
			peer->allowed_ip[i].mask_len = allowed_ip->mask_len;

			memcpy(&peer->allowed_ip[i].addr, &allowed_ip->addr,
			       sizeof(peer->allowed_ip[i].addr));

			NET_DBG("Peer %d allowed IP %s/%d",
				peer->id,
				net_sprint_addr(peer->allowed_ip[i].addr.family,
						&peer->allowed_ip[i].addr.in_addr),
				peer->allowed_ip[i].mask_len);
		} else {
			memset(&peer->allowed_ip[i], 0, sizeof(peer->allowed_ip[i]));
			peer->allowed_ip[i].is_valid = false;
		}
	}
}

int wireguard_peer_add(struct wireguard_peer_config *peer_config,
		       struct net_if **peer_iface)
{
	static int id;
	uint8_t public_key[WG_PUBLIC_KEY_LEN];
	struct net_event_vpn_peer event;
	struct wg_iface_context *ctx;
	struct net_if *iface = NULL;
	struct wg_peer *peer;
	sys_snode_t *node;
	int ret;

	if (peer_config->public_key == NULL) {
		NET_DBG("Public key not set");
		return -EINVAL;
	}

	if (!extract_public_key(peer_config->public_key, public_key,
				sizeof(public_key))) {
		NET_DBG("Invalid public_key base64 format");
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	peer = peer_lookup_by_pubkey(NULL, public_key);
	if (peer != NULL) {
		ret = -EALREADY;
		goto out;
	}

	/* Try to find out available virtual network interface */
	net_if_foreach(iface_cb, &iface);

	if (iface == NULL) {
		ret = -ENOMEM;
		NET_INFO("No available Wireguard interfaces found");
		goto out;
	}

	/* We could find an interface, now allocate the peer */
	node = sys_slist_get(&peer_list);
	if (node == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	peer = CONTAINER_OF(node, struct wg_peer, node);

	ctx = net_if_get_device(iface)->data;

	if (!wg_peer_init(ctx, peer, public_key, peer_config->preshared_key)) {
		NET_DBG("Peer init failed");
		ret = -EINVAL;
		sys_slist_prepend(&peer_list, node);
		goto out;
	}

	ret = net_virtual_interface_attach(iface, wg_ctx.iface);
	if (ret < 0) {
		NET_DBG("Cannot attach %d to %d",
			net_if_get_by_iface(iface),
			net_if_get_by_iface(wg_ctx.iface));
		sys_slist_prepend(&peer_list, node);
		goto out;
	}

	memcpy(&peer->cfg_endpoint, &peer_config->endpoint_ip,
	       sizeof(peer->cfg_endpoint));

	if (net_sin(net_sad(&peer->cfg_endpoint))->sin_port == 0) {
		net_sin(net_sad(&peer->cfg_endpoint))->sin_port = net_htons(WG_DEFAULT_PORT);
	}

	peer->id = ++id;
	peer->iface = iface;
	*peer_iface = peer->iface;
	peer->ctx = ctx;

	peer_set_allowed_addr(peer, peer_config);

	if (peer_config->keepalive_interval > KEEPALIVE_DEFAULT) {
		peer->keepalive_interval = (uint16_t)peer_config->keepalive_interval;
		peer->keepalive_expires =
			sys_timepoint_calc(K_SECONDS(peer->keepalive_interval));
	} else {
		peer->keepalive_interval = 0;
	}

	sys_slist_prepend(&active_peers, node);

	net_if_up(peer->iface);

	ret = peer->id;

	NET_DBG("Peer %d attached to interface %d", ret,
		net_if_get_by_iface(peer->iface));

	event.id = ret;
	event.public_key = peer_config->public_key;
	event.keepalive_interval = peer->keepalive_interval;
	event.endpoint = net_sad(&peer->cfg_endpoint);

	ARRAY_FOR_EACH(peer->allowed_ip, i) {
		struct wg_allowed_ip *allowed_ip = &peer->allowed_ip[i];

		event.allowed_ip[i] = (struct wireguard_allowed_ip *)allowed_ip;
	}

	event.allowed_ip[WIREGUARD_MAX_SRC_IPS] = NULL;

	net_mgmt_event_notify_with_info(NET_EVENT_VPN_PEER_ADD, peer->iface,
					&event, sizeof(event));

out:
	k_mutex_unlock(&lock);

	return ret;
}

static void wg_peer_cleanup(struct wg_peer *peer)
{
	memset(&peer->key, 0, sizeof(peer->key));

	peer->id = 0;
	peer->first_valid = false;
}

int wireguard_peer_remove(int peer_id)
{
	struct wg_peer *peer;
	int ret;

	k_mutex_lock(&lock, K_FOREVER);

	if (sys_slist_is_empty(&active_peers)) {
		ret = -ENOENT;
		goto out;
	}

	peer = peer_lookup_by_id(peer_id);
	if (peer == NULL) {
		ret = -ENOENT;
		goto out;
	}

	if (sys_slist_find_and_remove(&active_peers, &peer->node) == false) {
		ret = -EFAULT;
		goto out;
	}

	sys_slist_prepend(&peer_list, &peer->node);

	net_mgmt_event_notify_with_info(NET_EVENT_VPN_PEER_DEL, peer->iface,
					&peer->id,
					sizeof(peer->id));

	wg_peer_cleanup(peer);

	/* Detach the virtual interface from the control interface and
	 * turn off the Wireguard virtual interface so that packets cannot be
	 * sent through it.
	 */
	(void)net_virtual_interface_attach(peer->iface, NULL);

	net_mgmt_event_notify(NET_EVENT_VPN_DISCONNECTED, peer->iface);

	net_if_down(peer->iface);

	ret = 0;
out:
	k_mutex_unlock(&lock);

	return ret;
}

void wireguard_peer_foreach(wg_peer_cb_t cb, void *user_data)
{
	struct wg_peer *peer, *next;

	k_mutex_lock(&lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_peers,
					  peer, next, node) {
		cb(peer, user_data);
	}

	k_mutex_unlock(&lock);
}

#if defined(CONFIG_NET_STATISTICS_VPN) && defined(CONFIG_NET_STATISTICS_USER_API)

static int wg_stats_get(uint64_t mgmt_request, struct net_if *iface,
			 void *data, size_t len)
{
	size_t len_chk = 0;
	struct net_stats_vpn *src = NULL;
	struct wg_peer *peer, *next;
	int ret = -ENOENT;

	switch (NET_MGMT_GET_COMMAND(mgmt_request)) {
	case NET_REQUEST_STATS_CMD_GET_VPN:
		if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
			return -ENOENT;
		}

		if (net_virtual_get_iface_capabilities(iface) != VIRTUAL_INTERFACE_VPN) {
			return -ENOENT;
		}

		len_chk = sizeof(struct net_stats_vpn);

		k_mutex_lock(&lock, K_FOREVER);

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_peers, peer, next, node) {
			if (peer->iface != iface) {
				continue;
			}

			src = &peer->ctx->stats;
			ret = 0;
			break;
		}

		k_mutex_unlock(&lock);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

	if (ret < 0) {
		return ret;
	}

	if (len != len_chk || src == NULL) {
		return -EINVAL;
	}

	memcpy(data, src, len);

	return 0;
}

NET_MGMT_REGISTER_REQUEST_HANDLER(NET_REQUEST_STATS_GET_VPN, wg_stats_get);

#endif /* CONFIG_NET_STATISTICS_USER_API && CONFIG_NET_STATISTICS_VPN */

static int get_current_time(uint64_t *seconds, uint32_t *nanoseconds)
{
	uint64_t millis = k_uptime_get();

	*seconds = millis / MSEC_PER_SEC;
	*nanoseconds = (millis % MSEC_PER_SEC) * NSEC_PER_MSEC;

	return 0;
}

/* Declare a default function but allow the user to override it */
__weak FUNC_ALIAS(get_current_time, wireguard_get_current_time, int);

SYS_INIT(wireguard_init, APPLICATION, CONFIG_NET_INIT_PRIO);
