/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Wireguard VPN implementation
 */

#include <limits.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(wireguard, CONFIG_WIREGUARD_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/clock.h>
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
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_log.h>

#ifdef CONFIG_TAILSCALE
#include "../tailscale/net_stack.h"
#endif

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

#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0 && \
	(!defined(MBEDTLS_THREADING_C) || !defined(MBEDTLS_THREADING_PTHREAD))
#error "The WireGuard handshake work queue requires the Mbed TLS pthread backend"
#endif

/* Reference implementations needed when not using PSA or for algorithms
 * not available in PSA (Blake and XChaCha20-Poly1305)
 */
#include "crypto/crypto.c"

static struct wg_iface_context *get_wg_iface_context(struct net_if *iface);
static int interface_send(struct net_if *iface, struct net_pkt *pkt);
static int send_inner_packet(struct net_if *iface, struct net_pkt *pkt, struct wg_peer *peer);
static int send_datagram(struct wg_peer *peer, const struct wireguard_datagram_path *path,
			 const uint8_t *data, size_t len, uint8_t dscp, struct net_pkt *plaintext);
static void update_peer_addr(struct wg_peer *peer, struct net_sockaddr *peer_addr);

#if defined(CONFIG_WIREGUARD_TXRX_DEBUG)
#define DEBUG_TX 1
#define DEBUG_RX 1
#else
#define DEBUG_TX 0
#define DEBUG_RX 0
#endif

#define WG_TIMER_PERIOD 500 /* ms */

#if CONFIG_WIREGUARD_WORKQ_STACK_SIZE > 0
static K_KERNEL_STACK_DEFINE(wg_workq_stack, CONFIG_WIREGUARD_WORKQ_STACK_SIZE);
static struct k_work_q wg_workq;
#define WG_WORKQ (&wg_workq)
#else
#define WG_WORKQ (&k_sys_work_q)
#endif

#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
static K_KERNEL_STACK_DEFINE(wg_handshake_stack, CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE);
static struct k_work_q wg_handshake_workq;
static void wg_native_handshake_rx(struct k_work *work);
static void wg_managed_handshake_tx(struct k_work *work);
static K_WORK_DEFINE(wg_handshake_rx_work, wg_native_handshake_rx);
static K_WORK_DEFINE(wg_handshake_tx_work, wg_managed_handshake_tx);
static K_FIFO_DEFINE(wg_handshake_rx_queue);
static struct wg_peer *unlocked_handshake_peer;

struct wg_handshake_datagram {
	void *fifo_reserved;
	struct wg_iface_context *ctx;
	uint64_t policy_generation;
	atomic_val_t iface_generation;
	struct wireguard_datagram_path path;
	union wg_msg msg;
};

K_MEM_SLAB_DEFINE_STATIC(wg_handshake_rx_slab, sizeof(struct wg_handshake_datagram),
			 4, sizeof(void *));
#endif

#define WG_BUF_COUNT CONFIG_WIREGUARD_BUF_COUNT
#define WG_MAX_BUF_SIZE CONFIG_WIREGUARD_BUF_LEN

#define WG_DEFAULT_PORT 51820

#if defined(CONFIG_WIREGUARD_MULTI_PEER)
#define WG_IFACE_COUNT CONFIG_WIREGUARD_MAX_INTERFACES
#else
#define WG_IFACE_COUNT CONFIG_WIREGUARD_MAX_PEER
#endif

NET_PKT_SLAB_DEFINE(decrypted_pkts, WG_BUF_COUNT);

NET_BUF_POOL_FIXED_DEFINE(rx_msg_pool, WG_BUF_COUNT, WG_MAX_BUF_SIZE, 0, NULL);

/* The WireGuard mutex protects this scratch area through datagram consumption. */
static uint8_t tx_msg[WG_MAX_BUF_SIZE];

static const uint8_t zero_key[WG_PUBLIC_KEY_LEN];

static K_MUTEX_DEFINE(lock);

#if defined(CONFIG_WIREGUARD_PEER_POLICY)
#define WG_PEER_SLOTS (CONFIG_WIREGUARD_MAX_PEER + CONFIG_WIREGUARD_POLICY_STAGING_PEERS)
#else
#define WG_PEER_SLOTS CONFIG_WIREGUARD_MAX_PEER
#endif

#if defined(CONFIG_WIREGUARD_MULTI_PEER)
#define ROUTE_WORDS DIV_ROUND_UP(WG_PEER_SLOTS, 32)

/* Lifecycle writers serialize removal. Readers pin immutable peer slots until
 * their lookup completes, without holding a spinlock while matching prefixes.
 */
static struct k_spinlock tx_routes_lock;
static unsigned int route_epoch;
static unsigned int route_readers[2];
static K_SEM_DEFINE(routes_idle, 0, 1);
typedef struct wg_allowed_ip wg_peer_prefixes_t[CONFIG_WIREGUARD_MAX_SRC_IPS];
#endif

static struct wg_peer peers[WG_PEER_SLOTS];
static sys_slist_t peer_list;
static sys_slist_t active_peers;
static int last_peer_id;

#if defined(CONFIG_WIREGUARD_PEER_POLICY)
/* A route reader pins one bank. Unmanaged readers continue using peer slots. */
static wg_peer_prefixes_t policy_routes[2][WG_PEER_SLOTS];
static int64_t policy_route_expiry[2][WG_PEER_SLOTS];
static unsigned int policy_bank;
static K_SEM_DEFINE(policy_handoffs_idle, 0, 1);
static sys_slist_t policy_handoffs;
#endif

static struct wg_context {
	struct k_work_delayable wg_periodic_timer;
	struct net_mgmt_event_callback wg_mgmt_cb;
	struct net_if *iface; /* control interface */
	atomic_t attached_peers;
	atomic_t lifecycle_busy;
	int ifindex;          /* output network interface if set */
	uint8_t construction_hash[WG_HASH_LEN];
	uint8_t identifier_hash[WG_HASH_LEN];

	/* Local Wireguard port */
	uint16_t port;

	bool status;
} wg_ctx;

enum wg_peer_iface_state {
	WG_PEER_IFACE_FREE,
	WG_PEER_IFACE_ADDING,
	WG_PEER_IFACE_ACTIVE,
	WG_PEER_IFACE_RETIRING,
};

/* Each Wireguard virtual interface is tied to one specific wg_iface_context */
struct wg_iface_context {
	const char *name;
	struct net_if *iface;
	struct net_if *attached_to;
	struct wg_context *wg_ctx;
	struct wg_peer *peer;
#if defined(CONFIG_WIREGUARD_MULTI_PEER)
	uint32_t route_mask[ROUTE_WORDS];
#endif
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	wireguard_policy_input_t policy_input;
	void *policy_context;
	atomic_t policy_blocked;
	unsigned int handoff_epoch;
	unsigned int handoffs[2];
#endif
#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
	uint64_t handshake_policy_generation;
	atomic_t handshake_iface_generation;
#endif
	bool connected;
	bool notifying;

#if defined(CONFIG_NET_STATISTICS_VPN)
	struct net_stats_vpn stats;
#endif /* CONFIG_NET_STATISTICS_VPN */

	/* PSA key ID for interface private key */
	psa_key_id_t private_key_id;

	uint8_t public_key[WG_PUBLIC_KEY_LEN];

	uint8_t cookie_secret[WG_HASH_LEN];
	k_timepoint_t cookie_secret_expires;

	uint8_t label_cookie_key[WG_SESSION_KEY_LEN];
	uint8_t label_mac1_key[WG_SESSION_KEY_LEN];

	atomic_t peer_state;
	uint8_t slot;
	bool shared : 1;
	bool status : 1;
	bool init_done : 1;
};

#include "wg_stats.h"

static int create_packet(struct net_if *iface,
			 struct net_sockaddr *src,
			 struct net_sockaddr *dst,
			 const uint8_t *packet,
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

static struct net_if *peer_underlay(const struct wg_peer *peer)
{
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	return peer->underlay;
#else
	ARG_UNUSED(peer);
	return NULL;
#endif
}

static int select_target_iface(struct wg_peer *peer,
			       struct net_sockaddr *addr,
			       struct net_sockaddr_storage *dst,
			       struct net_if **iface)
{
	struct net_if *underlay = peer_underlay(peer);
	struct net_if *target = underlay;
	int ifindex = peer->ctx->wg_ctx->ifindex;

	if (target == NULL && ifindex > 0) {
		target = net_if_get_by_index(ifindex);
	}
	if (underlay != NULL && !net_if_is_up(underlay)) {
		return -ENETDOWN;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && dst->ss_family == NET_AF_INET6) {
		const struct net_in6_addr *src;

		if (target != NULL) {
			*iface = target;
			src = net_if_ipv6_select_src_addr(target,
							  &net_sin6(net_sad(dst))->sin6_addr);
		} else {
			*iface = net_if_ipv6_select_src_iface_addr(
				&net_sin6(net_sad(dst))->sin6_addr, &src);
		}
		if (underlay != NULL && net_ipv6_is_addr_unspecified(src)) {
			return -EADDRNOTAVAIL;
		}

		net_ipv6_addr_copy_raw((uint8_t *)(&net_sin6(addr)->sin6_addr),
				       (const uint8_t *)src);
		addr->sa_family = NET_AF_INET6;

	} else if (IS_ENABLED(CONFIG_NET_IPV4) && dst->ss_family == NET_AF_INET) {
		const struct net_in_addr *src;

		if (target != NULL) {
			*iface = target;
			src = net_if_ipv4_select_src_addr(target, &net_sin(net_sad(dst))->sin_addr);
		} else {
			*iface = net_if_ipv4_select_src_iface_addr(
				&net_sin(net_sad(dst))->sin_addr, &src);
		}
		if (underlay != NULL && net_ipv4_is_addr_unspecified(src)) {
			return -EADDRNOTAVAIL;
		}

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

/* Called with the WireGuard mutex. Pinned route readers use their own bank. */
static int64_t peer_deadline(const struct wg_peer *peer)
{
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	if (peer->policy_identity != 0U) {
		return policy_route_expiry[policy_bank][peer - peers];
	}
#endif
	return 0;
}

static bool peer_policy_denied(const struct wg_peer *peer)
{
	int64_t deadline = peer_deadline(peer);

#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	if (peer->policy_identity != 0U && atomic_get(&peer->ctx->policy_blocked) != 0) {
		return true;
	}
#endif
	return deadline != 0 && k_uptime_get() >= deadline;
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

		if (!peer->session.keypair.current.is_valid &&
		    (peer->last_initiation_tx != 0 || peer->first_valid)) {
			return true;
		}
	}

	return false;
}

/* With a work queue of its own, a handshake initiation started outside of it
 * is left to the periodic timer, so that its X25519 computation runs on the
 * Wireguard work queue rather than on the stack of the calling thread.
 */
static bool defer_handshake(void)
{
#if CONFIG_WIREGUARD_WORKQ_STACK_SIZE > 0
	return k_current_get() != k_work_queue_thread_get(&wg_workq);
#else
	return false;
#endif
}

static bool defer_managed_handshake(const struct wg_peer *peer)
{
#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
	if (peer->policy_identity != 0U) {
		k_work_submit_to_queue(&wg_handshake_workq, &wg_handshake_tx_work);
		return true;
	}
#else
	ARG_UNUSED(peer);
#endif
	return false;
}

static int start_handshake(struct wg_iface_context *ctx,
			   struct wg_peer *peer)
{
	struct msg_handshake_init msg;
	int ret;

#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	if (peer->policy_identity != 0U && peer->transport.send == NULL &&
	    peer->policy_endpoint_learned && peer->cfg_endpoint.ss_family != NET_AF_UNSPEC &&
	    peer->first_valid && !peer->session.keypair.current.is_valid &&
	    !peer->session.keypair.next.is_valid) {
		/* The last established session ended. Retry the current advertised endpoint. */
		peer->endpoint = peer->cfg_endpoint;
		peer->policy_endpoint_learned = false;
	}
#endif
	if (!wg_create_handshake_init(ctx, peer, &msg)) {
		return -EINVAL;
	}

	ret = send_datagram(peer, NULL, (const uint8_t *)&msg, sizeof(msg),
			    NET_IPV4_DSCP_AF41, NULL);
	if (ret < 0) {
		NET_DBG("Cannot send handshake initiation (%d)", ret);
		return ret;
	}

	vpn_stats_update_handshake_init_tx(ctx);
	peer->send_handshake = false;
	peer->last_initiation_tx = sys_clock_tick_get_32();
	peer->rekey_expires = sys_timepoint_calc(K_SECONDS(REKEY_TIMEOUT));
	memcpy(peer->handshake_mac1, msg.mac1, WG_COOKIE_LEN);
	peer->handshake_mac1_valid = true;

	ret = k_work_schedule_for_queue(WG_WORKQ, &ctx->wg_ctx->wg_periodic_timer,
					K_MSEC(WG_TIMER_PERIOD));
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

	if (peer_policy_denied(peer)) {
		return -EACCES;
	}

	/* Without a session there is nothing to keep alive: start the
	 * handshake instead. Before the first handshake no endpoint is learnt
	 * yet, so initiate to the configured one, as the first data packet to
	 * the peer does.
	 */
	if (!peer->session.keypair.current.is_valid) {
		if (peer->endpoint.ss_family == NET_AF_UNSPEC) {
			memcpy(&peer->endpoint, &peer->cfg_endpoint, sizeof(peer->endpoint));
		}

		if (peer->endpoint.ss_family == NET_AF_UNSPEC && peer->transport.send == NULL) {
			/* The peer has to contact us first */
			return -EDESTADDRREQ;
		}

		/* The periodic timer resends an initiation left unanswered while
		 * there is no session. Setting send_handshake here would outlive
		 * the handshake and start another one after the rekey timeout.
		 */
		if (!can_send_init(peer)) {
			return 0;
		}

		/* Cleared once the initiation is sent, else the periodic timer
		 * retries it.
		 */
		peer->send_handshake = true;
		if (defer_managed_handshake(peer)) {
			peer->last_tx = sys_clock_tick_get_32();
			return 0;
		}

		if (defer_handshake()) {
			(void)k_work_reschedule_for_queue(WG_WORKQ,
							  &ctx->wg_ctx->wg_periodic_timer,
							  K_NO_WAIT);
		} else {
			ret = start_handshake(ctx, peer);
			if (ret < 0) {
				(void)k_work_schedule_for_queue(WG_WORKQ,
								&ctx->wg_ctx->wg_periodic_timer,
								K_MSEC(WG_TIMER_PERIOD));
				return ret;
			}
		}

		/* As the first data packet does, so that it does not start
		 * another handshake.
		 */
		peer->last_tx = sys_clock_tick_get_32();

		return 0;
	}

	if (peer->transport.send == NULL) {
		ret = select_target_iface(peer, addr, &peer->endpoint, &target_iface);
		if (ret < 0) {
			vpn_stats_update_invalid_ip_family(ctx);
			return ret;
		}
	} else {
		target_iface = ctx->iface;
	}

	pkt = net_pkt_alloc_with_buffer(target_iface, 0, NET_AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		NET_DBG("Cannot allocate keepalive packet");
		vpn_stats_update_alloc_failed(ctx);
		return -ENOMEM;
	}

	net_pkt_set_vpn_peer_id(pkt, peer->id);
	net_pkt_set_vpn_iface(pkt, target_iface);
	net_pkt_set_iface(pkt, target_iface);

	/* Send empty keepalive packet directly */
	ret = send_inner_packet(ctx->iface, pkt, peer);

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
			if (!net_if_is_up(peer->iface)) {
				ret = -ENETDOWN;
				break;
			}
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
		if (!net_if_is_up(peer->ctx->iface) || peer_policy_denied(peer)) {
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

		if (should_send_init(peer) && !defer_managed_handshake(peer)) {
			start_handshake(peer->ctx, peer);
		}
	}

	k_mutex_unlock(&lock);

	ret = k_work_reschedule_for_queue(WG_WORKQ, dwork, K_MSEC(WG_TIMER_PERIOD));
	if (ret < 0) {
		NET_DBG("Cannot schedule %s work (%d)", "periodic", ret);
	}
}

#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
static void wg_managed_handshake_tx(struct k_work *work)
{
	struct wg_peer *peer;
	struct wg_peer *next;
	int after_id = 0;

	ARG_UNUSED(work);

	k_mutex_lock(&lock, K_FOREVER);
	for (;;) {
		next = NULL;
		SYS_SLIST_FOR_EACH_CONTAINER(&active_peers, peer, node) {
			if (peer->policy_identity != 0U && peer->id > after_id &&
			    (next == NULL || peer->id < next->id) && net_if_is_up(peer->iface) &&
			    !peer_policy_denied(peer) && should_send_init(peer)) {
				next = peer;
			}
		}
		if (next == NULL) {
			break;
		}
		after_id = next->id;
		if (start_handshake(next->ctx, next) < 0) {
			(void)k_work_schedule_for_queue(WG_WORKQ, &wg_ctx.wg_periodic_timer,
							 K_MSEC(WG_TIMER_PERIOD));
		}
	}
	k_mutex_unlock(&lock);
}
#endif

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

	blake2s_init(&bl_ctx, WG_HASH_LEN, NULL, 0);
	blake2s_update(&bl_ctx, CONSTRUCTION, sizeof(CONSTRUCTION));
	blake2s_final(&bl_ctx, ctx->construction_hash);

	blake2s_init(&bl_ctx, WG_HASH_LEN, NULL, 0);
	blake2s_update(&bl_ctx, ctx->construction_hash,
				 sizeof(ctx->construction_hash));
	blake2s_update(&bl_ctx, IDENTIFIER, sizeof(IDENTIFIER));
	blake2s_final(&bl_ctx, ctx->identifier_hash);
}

static int wireguard_init(void)
{
	struct net_sockaddr_storage local_addr_storage = { 0 };
	struct net_sockaddr *local_addr = net_sad(&local_addr_storage);
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

	wg_crypto_init();
	crypto_init(ctx);

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		local_addr->sa_family = NET_AF_INET6;

		/* Note that if IPv4 is enabled, then v4-to-v6-mapping option
		 * is set and the system will use the IPv6 socket to provide
		 * IPv4 connectivity.
		 */
	} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
		local_addr->sa_family = NET_AF_INET;
	}

	if (CONFIG_WIREGUARD_PORT > 0) {
		port = CONFIG_WIREGUARD_PORT;
	} else {
		port = get_port(local_addr);
		if (port == 0) {
			port = WG_DEFAULT_PORT;
		}

		NET_INFO("Wireguard service port %d", port);
	}

	ret = net_udp_register(local_addr->sa_family,
			       NULL,
			       local_addr,
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

#if CONFIG_WIREGUARD_WORKQ_STACK_SIZE > 0
	k_work_queue_start(&wg_workq, wg_workq_stack, K_KERNEL_STACK_SIZEOF(wg_workq_stack),
			   CONFIG_WIREGUARD_WORKQ_PRIORITY,
			   &(const struct k_work_queue_config){ .name = "wireguard" });
#endif

#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
	k_work_queue_start(&wg_handshake_workq, wg_handshake_stack,
			   K_KERNEL_STACK_SIZEOF(wg_handshake_stack),
			   K_LOWEST_APPLICATION_THREAD_PRIO,
			   &(const struct k_work_queue_config){ .name = "wireguard-handshake" });
#endif

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

static bool peer_matches_rx(const struct wg_rx_scope *scope, const struct wg_peer *peer)
{
	struct net_if *underlay = peer_underlay(peer);

	return (scope->ctx == NULL || scope->ctx == peer->ctx) &&
	       (scope->expected_peer == NULL || scope->expected_peer == peer) &&
	       scope->native_udp == (peer->transport.send == NULL) &&
	       (!scope->native_udp || underlay == NULL || underlay == scope->native_iface) &&
	       net_if_is_up(peer->iface) && !peer_policy_denied(peer);
}

static bool peer_matches_handshake_rx(const struct wg_rx_scope *scope, const struct wg_peer *peer)
{
#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
	/* Only the dedicated worker drops the mutex while using a peer transcript. */
	if (peer == unlocked_handshake_peer) {
		return false;
	}
#endif
	return peer_matches_rx(scope, peer);
}

static struct wg_peer *select_rx_peer(const struct wg_rx_scope *scope, uint32_t index,
				      bool handshake)
{
	struct wg_peer *peer = handshake ? peer_lookup_by_handshake(scope->ctx, index) :
					   peer_lookup_by_receiver(scope->ctx, index);

	if (peer == NULL) {
		return NULL;
	}
	return (handshake ? peer_matches_handshake_rx(scope, peer) : peer_matches_rx(scope, peer)) ?
		peer : NULL;
}

static struct wg_iface_context *native_iface_by_mac1(const struct wg_rx_scope *scope,
						    struct msg_handshake_init *msg)
{
	uint32_t seen[DIV_ROUND_UP(WG_IFACE_COUNT, 32U)] = {0};
	struct wg_peer *peer;

	SYS_SLIST_FOR_EACH_CONTAINER(&active_peers, peer, node) {
		struct wg_iface_context *ctx = peer->ctx;
		uint32_t bit = BIT(ctx->slot % 32U);

		if (!peer_matches_rx(scope, peer) || (seen[ctx->slot / 32U] & bit) != 0U) {
			continue;
		}
		seen[ctx->slot / 32U] |= bit;
		if (wg_check_mac1(ctx, (const uint8_t *)msg,
				  offsetof(struct msg_handshake_init, mac1),
				  msg->mac1)) {
			return ctx;
		}
	}

	return NULL;
}

static int handle_handshake_init(const struct wg_rx_scope *scope,
				 struct wireguard_datagram_path *path,
				 struct msg_handshake_init *msg)
{
	struct wg_iface_context *ctx = scope->ctx;
	struct wg_peer *peer;
	int ret;

	if (ctx == NULL) {
		ctx = native_iface_by_mac1(scope, msg);
		if (ctx == NULL) {
			return -ENOENT;
		}
	} else if (!wg_check_mac1(ctx, (const uint8_t *)msg,
				  offsetof(struct msg_handshake_init, mac1), msg->mac1)) {
		vpn_stats_update_invalid_mac1(ctx);
		return -EINVAL;
	}
	vpn_stats_update_handshake_init_rx(ctx);
	peer = wg_process_initiation_message(ctx, scope, path, msg);
	if (peer == NULL) {
		return -ENOENT;
	}
	if (path->kind == WIREGUARD_PATH_DIRECT) {
		update_peer_addr(peer, net_sad(&path->remote));
	}
	ret = wg_send_handshake_response(peer, path);
	if (ret == 0) {
		(void)k_work_schedule_for_queue(WG_WORKQ, &ctx->wg_ctx->wg_periodic_timer,
						 K_MSEC(WG_TIMER_PERIOD));
	}

	return ret;
}

static int handle_handshake_response(const struct wg_rx_scope *scope,
				     struct wireguard_datagram_path *path,
				     struct msg_handshake_response *msg)
{
	struct wg_peer *peer;

	peer = select_rx_peer(scope, msg->receiver, true);
	if (peer == NULL) {
		return -ENOENT;
	}
	if (!wg_check_handshake_mac(peer, path, (uint8_t *)msg, sizeof(*msg), msg->sender)) {
		return -EINVAL;
	}

	if (!wg_process_handshake_response(peer->ctx, peer, msg)) {
		vpn_stats_update_invalid_handshake(peer->ctx);
		return -EINVAL;
	}
	if (wg_start_session(peer, true) < 0) {
		return -EIO;
	}
	if (path->kind == WIREGUARD_PATH_DIRECT) {
		update_peer_addr(peer, net_sad(&path->remote));
	}
	(void)wg_send_keepalive(peer->ctx, peer);

	vpn_stats_update_handshake_resp_rx(peer->ctx);

	return 0;
}

static void update_peer_addr(struct wg_peer *peer, struct net_sockaddr *peer_addr)
{
	if (peer_addr != NULL) {
		memcpy(&peer->endpoint, peer_addr, sizeof(peer->endpoint));
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
		peer->policy_endpoint_learned = true;
#endif
	}
}

static int handle_cookie_reply(const struct wg_rx_scope *scope, struct msg_cookie_reply *msg)
{
	struct wg_peer *peer = select_rx_peer(scope, msg->receiver, true);

	if (peer == NULL) {
		return -ENOENT;
	}

	return wg_process_cookie_message(peer->ctx, peer, msg) ? 0 : -EINVAL;
}

static int handle_handshake_datagram(const struct wg_rx_scope *scope,
				     struct wireguard_datagram_path *path, union wg_msg *msg)
{
	switch (msg->short_hdr.type) {
	case MESSAGE_HANDSHAKE_INITIATION:
		return handle_handshake_init(scope, path, &msg->init);
	case MESSAGE_HANDSHAKE_RESPONSE:
		return handle_handshake_response(scope, path, &msg->rsp);
	default:
		return handle_cookie_reply(scope, &msg->rpl);
	}
}

#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
static struct wg_iface_context *native_handshake_iface(const struct wg_rx_scope *scope,
						       union wg_msg *msg)
{
	struct wg_peer *peer;
	uint32_t receiver;

	if (msg->short_hdr.type == MESSAGE_HANDSHAKE_INITIATION) {
		return native_iface_by_mac1(scope, &msg->init);
	}
	receiver = msg->short_hdr.type == MESSAGE_HANDSHAKE_RESPONSE ?
		msg->rsp.receiver : msg->rpl.receiver;
	peer = select_rx_peer(scope, receiver, true);
	return peer != NULL ? peer->ctx : NULL;
}

static void wg_native_handshake_rx(struct k_work *work)
{
	struct wg_handshake_datagram *job = k_fifo_get(&wg_handshake_rx_queue, K_NO_WAIT);
	struct wg_rx_scope scope = {.native_udp = true};

	if (job == NULL) {
		return;
	}
	k_mutex_lock(&lock, K_FOREVER);
	scope.ctx = job->ctx;
	scope.native_iface = job->path.iface;
	if (net_if_is_up(job->ctx->iface) &&
	    job->policy_generation == job->ctx->handshake_policy_generation &&
	    job->iface_generation == atomic_get(&job->ctx->handshake_iface_generation)) {
		(void)handle_handshake_datagram(&scope, &job->path, &job->msg);
	}
	k_mutex_unlock(&lock);
	k_mem_slab_free(&wg_handshake_rx_slab, job);
	if (!k_fifo_is_empty(&wg_handshake_rx_queue)) {
		k_work_submit_to_queue(&wg_handshake_workq, work);
	}
}
#endif

static int receive_handshake(const struct wg_rx_scope *scope, struct wireguard_datagram_path *path,
			     struct net_pkt *pkt, size_t len)
{
	union wg_msg msg;

	if (net_pkt_read(pkt, &msg, len) < 0) {
		return -EINVAL;
	}
#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
	if (scope->native_udp) {
		struct wg_iface_context *ctx = native_handshake_iface(scope, &msg);

		if (ctx == NULL) {
			return -ENOENT;
		}
		if (ctx->policy_input != NULL) {
			struct wg_handshake_datagram *job;
			int ret = k_mem_slab_alloc(&wg_handshake_rx_slab, (void **)&job, K_NO_WAIT);

			if (ret < 0) {
				return ret;
			}
			job->ctx = ctx;
			job->policy_generation = ctx->handshake_policy_generation;
			job->iface_generation = atomic_get(&ctx->handshake_iface_generation);
			job->path = *path;
			memcpy(&job->msg, &msg, len);
			k_fifo_put(&wg_handshake_rx_queue, job);
			k_work_submit_to_queue(&wg_handshake_workq, &wg_handshake_rx_work);
			return 0;
		}
	}
#endif
	return handle_handshake_datagram(scope, path, &msg);
}

ZTESTABLE_STATIC int handle_transport_data(struct wg_peer *peer,
					   struct net_sockaddr *peer_addr,
					   struct net_pkt *pkt,
					   size_t ip_udp_hdr_len,
					   size_t data_len, struct wg_rx_result *result)
{
	NET_PKT_DATA_ACCESS_DEFINE(access, struct msg_transport_data);
	struct msg_transport_data *msg;
	struct wg_peer *session_peer;

	msg = (struct msg_transport_data *)net_pkt_get_data(pkt, &access);
	if (msg == NULL) {
		return -EINVAL;
	}

	session_peer = peer_lookup_by_receiver(peer->ctx, msg->receiver);
	if (session_peer == NULL) {
		/* No valid session for this receiver index. The remote peer
		 * might have a stale session. Trigger a new handshake if we
		 * haven't done so recently.
		 *
		 * Note that this message is not authenticated in any way. As
		 * the receiver index is unknown, there is no key to verify it
		 * with, so anyone able to send an UDP packet to our port can
		 * craft one. It must therefore never be allowed to change the
		 * peer endpoint, as that would let an off-path attacker steer
		 * our handshake initiations, and thus the tunnel, to an
		 * address of their choosing. Send the handshake only to the
		 * endpoint that we have learnt from authenticated messages,
		 * or to the configured one if we have none yet.
		 */
		if (!peer->handshake.is_valid && !peer->session.keypair.current.is_valid) {
			if (peer->endpoint.ss_family == NET_AF_UNSPEC) {
				memcpy(&peer->endpoint, &peer->cfg_endpoint,
				       sizeof(peer->endpoint));
			}

			if (peer->endpoint.ss_family == NET_AF_UNSPEC) {
				/* No known endpoint to initiate to. This peer
				 * can only be reached after it has contacted
				 * us with a valid handshake initiation.
				 */
				return -ENOENT;
			}

			peer->send_handshake = true;
			(void)k_work_schedule_for_queue(WG_WORKQ,
							&peer->ctx->wg_ctx->wg_periodic_timer,
							K_NO_WAIT);
		}
		return -ENOENT;
	}

	if (session_peer != peer) {
		return -ENOENT;
	}

	return wg_process_data_message(peer->ctx, peer, msg, pkt,
				       ip_udp_hdr_len, peer_addr, result);
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

static struct wg_peer *native_udp_peer(struct net_if *ctrl_iface)
{
	struct wg_peer *peer;
	struct wg_peer *first = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&active_peers, peer, node) {
		if (peer->ctx->attached_to == ctrl_iface && peer->transport.send == NULL &&
		    net_if_is_up(peer->iface) &&
		    (first == NULL || peer->id < first->id)) {
			first = peer;
		}
	}
	return first;
}

static bool has_authenticated_peer(struct wg_iface_context *ctx)
{
	struct wg_peer *peer;

	SYS_SLIST_FOR_EACH_CONTAINER(&active_peers, peer, node) {
		if (peer->ctx == ctx && peer->first_valid) {
			return true;
		}
	}
	return false;
}

/* Serialize notifications across unlocked and possibly reentrant event handlers. */
static void notify_connectivity(struct wg_iface_context *ctx)
{
	bool connected;

	if (ctx->notifying) {
		return;
	}
	ctx->notifying = true;
	while ((connected = has_authenticated_peer(ctx)) != ctx->connected) {
		ctx->connected = connected;
		k_mutex_unlock(&lock);
		net_mgmt_event_notify(connected ? NET_EVENT_VPN_CONNECTED :
						  NET_EVENT_VPN_DISCONNECTED, ctx->iface);
		k_mutex_lock(&lock, K_FOREVER);
	}
	ctx->notifying = false;
}

#if defined(CONFIG_WIREGUARD_PEER_POLICY)
struct wg_policy_handoff {
	sys_snode_t node;
	struct wg_iface_context *ctx;
	k_tid_t thread;
	unsigned int epoch;
};

/* The WireGuard mutex protects handoff accounting. Only the retired epoch is
 * drained after publication, so new traffic cannot delay cutover.
 */
static void policy_handoff_begin(struct wg_iface_context *ctx, struct wg_policy_handoff *handoff)
{
	handoff->ctx = ctx;
	handoff->thread = k_current_get();
	handoff->epoch = ctx->handoff_epoch;
	ctx->handoffs[handoff->epoch]++;
	sys_slist_prepend(&policy_handoffs, &handoff->node);
}

static void policy_handoff_end(struct wg_policy_handoff *handoff)
{
	struct wg_iface_context *ctx = handoff->ctx;

	(void)sys_slist_find_and_remove(&policy_handoffs, &handoff->node);
	if (--ctx->handoffs[handoff->epoch] == 0U && handoff->epoch != ctx->handoff_epoch) {
		k_sem_give(&policy_handoffs_idle);
	}
}
#endif

/* Drop the crypto lock before TCP or event handlers acquire socket and interface locks.
 * Removal checks deliveries before retiring the interface or recycling its peer slot.
 */
static int deliver_rx_result(struct wg_peer *peer, struct wg_rx_result *result, int ret)
{
	enum net_verdict verdict = NET_CONTINUE;
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	struct wg_policy_handoff handoff = {0};
#endif

	if (!result->connected && result->plaintext == NULL) {
		return ret;
	}
	if (peer_policy_denied(peer)) {
		if (result->plaintext != NULL) {
			net_pkt_unref(result->plaintext);
		}
		return -EACCES;
	}

	peer->deliveries++;
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	if (peer->ctx->policy_input != NULL) {
		policy_handoff_begin(peer->ctx, &handoff);
	}
#endif
	if (result->connected) {
		notify_connectivity(peer->ctx);
	}
	k_mutex_unlock(&lock);
	if (result->plaintext != NULL) {
		verdict = net_if_l2(peer->iface)->recv(peer->iface, result->plaintext);
		if (verdict == NET_DROP) {
			net_pkt_unref(result->plaintext);
		}
	}
	k_mutex_lock(&lock, K_FOREVER);
	peer->deliveries--;
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	if (handoff.ctx != NULL) {
		policy_handoff_end(&handoff);
	}
#endif

	if (result->plaintext != NULL) {
		if (verdict == NET_DROP) {
			vpn_stats_update_drop_rx(peer->ctx);
			return -ENETRESET;
		}
		vpn_stats_update_valid_rx(peer->ctx);
	}
	return ret;
}

static int wg_receive_datagram(const struct wg_rx_scope *scope, struct net_pkt *pkt, size_t offset,
			       struct wireguard_datagram_path *path)
{
	NET_PKT_DATA_ACCESS_DEFINE(access, struct wg_msg_hdr);
	struct net_sockaddr *remote = path->kind == WIREGUARD_PATH_DIRECT ?
		net_sad(&path->remote) : NULL;
	struct wg_msg_hdr *hdr;
	struct wg_rx_result result = { 0 };
	struct wg_peer *peer;
	size_t len = net_pkt_get_len(pkt);
	int ret;

	if (offset > len || len - offset < sizeof(struct wg_msg_hdr)) {
		return -EMSGSIZE;
	}

	net_pkt_cursor_init(pkt);
	if (net_pkt_skip(pkt, offset) < 0) {
		return -EMSGSIZE;
	}

	hdr = (struct wg_msg_hdr *)net_pkt_get_data(pkt, &access);
	if (hdr == NULL) {
		return -EMSGSIZE;
	}

	if (hdr->reserved[0] != 0 || hdr->reserved[1] != 0 || hdr->reserved[2] != 0) {
		return -EINVAL;
	}

	switch (hdr->type) {
	case MESSAGE_HANDSHAKE_INITIATION:
		if (len - offset != sizeof(struct msg_handshake_init)) {
			return -EMSGSIZE;
		}
		return receive_handshake(scope, path, pkt, len - offset);
	case MESSAGE_HANDSHAKE_RESPONSE:
		if (len - offset != sizeof(struct msg_handshake_response)) {
			return -EMSGSIZE;
		}
		return receive_handshake(scope, path, pkt, len - offset);
	case MESSAGE_COOKIE_REPLY:
		if (len - offset != sizeof(struct msg_cookie_reply)) {
			return -EMSGSIZE;
		}
		return receive_handshake(scope, path, pkt, len - offset);
	case MESSAGE_TRANSPORT_DATA:
		if (len - offset < sizeof(struct msg_transport_data) + WG_AUTHTAG_LEN) {
			return -EMSGSIZE;
		}
		peer = scope->expected_peer;
		if (peer == NULL) {
			NET_PKT_DATA_ACCESS_DEFINE(data_access, struct msg_transport_data);
			struct msg_transport_data *data_hdr =
				(struct msg_transport_data *)net_pkt_get_data(pkt, &data_access);

			if (data_hdr == NULL) {
				return -EINVAL;
			}
			peer = select_rx_peer(scope, data_hdr->receiver, false);
			if (peer == NULL) {
				return -ENOENT;
			}
			ret = wg_process_data_message(peer->ctx, peer, data_hdr, pkt, offset,
						      remote, &result);
		} else {
			if (!peer_matches_rx(scope, peer)) {
				return -EACCES;
			}
			ret = handle_transport_data(peer, remote, pkt, offset, len - offset,
						    &result);
		}
		return deliver_rx_result(peer, &result, ret);
	default:
		return -EINVAL;
	}
}

static enum net_verdict wg_ctrl_recv(struct net_if *iface, struct net_pkt *pkt)
{
	struct wireguard_datagram_path path = {
		.kind = WIREGUARD_PATH_DIRECT,
		.iface = net_pkt_vpn_iface(pkt),
	};
	struct net_sockaddr *local = net_sad(&path.local);
	struct net_sockaddr *remote = net_sad(&path.remote);
	union net_proto_header *udp_hdr = net_pkt_vpn_udp_hdr(pkt);
	union net_ip_header *ip_hdr = net_pkt_vpn_ip_hdr(pkt);
	struct wg_rx_scope scope = {.native_iface = path.iface, .native_udp = true};
	size_t vpn_offset;
	int ret;

	if (pkt->buffer == NULL || ip_hdr->ipv4 == NULL || udp_hdr->udp == NULL) {
		return NET_DROP;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == NET_AF_INET) {
		memcpy(&net_sin(remote)->sin_addr, ip_hdr->ipv4->src, sizeof(struct net_in_addr));
		memcpy(&net_sin(local)->sin_addr, ip_hdr->ipv4->dst, sizeof(struct net_in_addr));
		net_sin(remote)->sin_port = udp_hdr->udp->src_port;
		net_sin(local)->sin_port = udp_hdr->udp->dst_port;
	} else if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == NET_AF_INET6) {
		memcpy(&net_sin6(remote)->sin6_addr, ip_hdr->ipv6->src,
		       sizeof(struct net_in6_addr));
		memcpy(&net_sin6(local)->sin6_addr, ip_hdr->ipv6->dst, sizeof(struct net_in6_addr));
		net_sin6(remote)->sin6_port = udp_hdr->udp->src_port;
		net_sin6(local)->sin6_port = udp_hdr->udp->dst_port;
	} else {
		return NET_DROP;
	}

	remote->sa_family = net_pkt_family(pkt);
	local->sa_family = remote->sa_family;
	vpn_offset = net_pkt_ip_hdr_len(pkt) + net_pkt_ip_opts_len(pkt) + NET_UDPH_LEN;

	k_mutex_lock(&lock, K_FOREVER);
	if (!IS_ENABLED(CONFIG_WIREGUARD_MULTI_PEER)) {
		scope.expected_peer = native_udp_peer(iface);
		if (scope.expected_peer == NULL) {
			k_mutex_unlock(&lock);
			return NET_DROP;
		}
		scope.ctx = scope.expected_peer->ctx;
	}
	ret = wg_receive_datagram(&scope, pkt, vpn_offset, &path);
	k_mutex_unlock(&lock);
	if (ret < 0) {
		NET_DBG("DROP: Invalid Wireguard datagram (%d)", ret);
		return NET_DROP;
	}

	net_pkt_unref(pkt);
	return NET_OK;
}

static int wg_ctrl_send(const struct device *dev, struct net_pkt *pkt)
{
	__maybe_unused struct wg_context *ctx = dev->data;

	net_stats_update_bytes_sent(ctx->iface, net_pkt_get_len(pkt));

	net_pkt_set_iface(pkt, net_pkt_vpn_iface(pkt));

	if (DEBUG_TX) {
		char str[sizeof("TX ctrl iface xx to xx")];

		snprintk(str, sizeof(str), "TX ctrl iface %d to %d",
			 net_if_get_by_iface(net_if_lookup_by_dev(dev)),
			 net_if_get_by_iface(net_pkt_iface(pkt)));

		net_pkt_hexdump(pkt, str);
	}

	return net_try_send_data(pkt, K_NO_WAIT);
}

static int wg_ctrl_start(const struct device *dev)
{
	struct wg_context *ctx = dev->data;
	int ret = 0;

	if (ctx->status) {
		return -EALREADY;
	}

	if (atomic_get(&ctx->attached_peers) == 0) {
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

static DEVICE_API(dummy, wg_api) = {
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
	atomic_val_t state = atomic_get(&ctx->peer_state);
	int ret = 0;

	if (state == WG_PEER_IFACE_RETIRING) {
		return -EBUSY;
	}

	if (ctx->status) {
		return -EALREADY;
	}

	ctx->status = true;
#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
	atomic_inc(&ctx->handshake_iface_generation);
#endif

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
#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
	atomic_inc(&ctx->handshake_iface_generation);
#endif

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
	k_mutex_lock(&lock, K_FOREVER);
	ctx->attached_to = lower_iface;
	k_mutex_unlock(&lock);

	return 0;
}

#if defined(CONFIG_WIREGUARD_MULTI_PEER)
static struct wg_peer *match_routes(const uint32_t *mask, const wg_peer_prefixes_t *routes,
				    const int64_t *deadlines, net_sa_family_t family,
				    const uint8_t *address)
{
	struct wg_peer *owner = NULL;
	int64_t now = -1;
	int best_len = -1;

	for (size_t word = 0; word < ROUTE_WORDS; word++) {
		uint32_t bits = mask[word];

		while (bits != 0U) {
			unsigned int bit = find_lsb_set(bits) - 1U;
			size_t slot = word * 32U + bit;
			const struct wg_allowed_ip *prefixes =
				routes != NULL ? routes[slot] : peers[slot].allowed_ip;

			bits &= bits - 1U;
			if (deadlines != NULL && deadlines[slot] != 0) {
				if (now < 0) {
					now = k_uptime_get();
				}
				if (now >= deadlines[slot]) {
					continue;
				}
			}
			for (size_t i = 0; i < CONFIG_WIREGUARD_MAX_SRC_IPS; i++) {
				const struct wg_allowed_ip *prefix = &prefixes[i];

				if (prefix->mask_len > best_len &&
				    wg_allowed_ip_match(prefix, family, address)) {
					owner = &peers[slot];
					best_len = prefix->mask_len;
				}
			}
		}
	}
	return owner;
}
#endif

/* Called with the WireGuard mutex. */
static struct wg_peer *peer_route_by_prefix(struct wg_iface_context *ctx, net_sa_family_t family,
					    const uint8_t *address)
{
#if defined(CONFIG_WIREGUARD_MULTI_PEER)
	const wg_peer_prefixes_t *routes = NULL;
	const int64_t *deadlines = NULL;

#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	if (ctx->policy_input != NULL) {
		if (atomic_get(&ctx->policy_blocked) != 0) {
			return NULL;
		}
		routes = policy_routes[policy_bank];
		deadlines = policy_route_expiry[policy_bank];
	}
#endif
	return match_routes(ctx->route_mask, routes, deadlines, family, address);
#else
	return NULL;
#endif
}

static bool wg_peer_owns_ip(struct wg_peer *peer, net_sa_family_t family, const uint8_t *address)
{
	if (peer->ctx->shared) {
		return peer == peer_route_by_prefix(peer->ctx, family, address);
	}
	return wg_peer_is_allowed_ip(peer, family, address);
}

#if defined(CONFIG_WIREGUARD_MULTI_PEER)
static int queued_tx_destination(struct net_pkt *pkt, uint8_t *destination)
{
	size_t header_size;
	size_t offset;
	size_t size;

	switch (net_pkt_family(pkt)) {
	case NET_AF_INET:
		header_size = sizeof(struct net_ipv4_hdr);
		offset = offsetof(struct net_ipv4_hdr, dst);
		size = sizeof(struct net_in_addr);
		break;
	case NET_AF_INET6:
		header_size = sizeof(struct net_ipv6_hdr);
		offset = offsetof(struct net_ipv6_hdr, dst);
		size = sizeof(struct net_in6_addr);
		break;
	default:
		return -EAFNOSUPPORT;
	}

	if (net_pkt_get_len(pkt) < header_size ||
	    net_buf_linearize(destination, size, pkt->buffer, offset, size) != size) {
		return -EINVAL;
	}
	return 0;
}

static int route_peer_id(struct wg_iface_context *ctx, net_sa_family_t family,
			 const uint8_t *destination, bool managed_only, uint64_t *identity)
{
	const wg_peer_prefixes_t *routes = NULL;
	const int64_t *deadlines = NULL;
	uint32_t mask[ROUTE_WORDS];
	unsigned int epoch;
	struct wg_peer *peer;
	k_spinlock_key_t key;
	bool wake;
	int id;

	if (identity != NULL) {
		*identity = 0;
	}
	key = k_spin_lock(&tx_routes_lock);
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	if (ctx->policy_input != NULL) {
		if (atomic_get(&ctx->policy_blocked) != 0) {
			k_spin_unlock(&tx_routes_lock, key);
			return -ENETUNREACH;
		}
		routes = policy_routes[policy_bank];
		deadlines = policy_route_expiry[policy_bank];
	}
#endif
	if (managed_only && routes == NULL) {
		k_spin_unlock(&tx_routes_lock, key);
		return -ENETUNREACH;
	}
	epoch = route_epoch;
	route_readers[epoch]++;
	memcpy(mask, ctx->route_mask, sizeof(mask));
	k_spin_unlock(&tx_routes_lock, key);

	peer = match_routes(mask, routes, deadlines, family, destination);
	id = peer != NULL ? peer->id : -ENETUNREACH;

#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	if (routes != NULL && atomic_get(&ctx->policy_blocked) != 0) {
		id = -ENETUNREACH;
	}
	if (id > 0 && identity != NULL) {
		*identity = peer->policy_identity;
	}
#endif
	key = k_spin_lock(&tx_routes_lock);
	wake = --route_readers[epoch] == 0U && epoch != route_epoch;
	k_spin_unlock(&tx_routes_lock, key);
	if (wake) {
		k_sem_give(&routes_idle);
	}
	return id;
}

void wireguard_prepare_tx(struct net_if *iface, struct net_pkt *pkt)
{
	struct wg_iface_context *ctx = get_wg_iface_context(iface);
	uint8_t destination[sizeof(struct net_in6_addr)];
	int id;

	if (ctx == NULL) {
		return;
	}
	id = queued_tx_destination(pkt, destination);
	if (id == 0) {
		id = route_peer_id(ctx, net_pkt_family(pkt), destination, false, NULL);
	}

	/* Each submission selects its owner afresh, including clones and forwards. */
	net_pkt_set_vpn_peer_id(pkt, id);
#ifdef CONFIG_TAILSCALE
	tailscale_net_queue(iface, pkt);
#endif
}
#endif

static int select_tx_peer(struct wg_iface_context *ctx, struct net_pkt *pkt, struct wg_peer **peer)
{
	size_t pkt_len = net_pkt_get_len(pkt);
	net_sa_family_t family = net_pkt_family(pkt);
	const uint8_t *destination;

#if defined(CONFIG_WIREGUARD_MULTI_PEER)
	int queued_peer_id = net_pkt_vpn_peer_id(pkt);

	if (queued_peer_id > 0) {
		uint8_t address[sizeof(struct net_in6_addr)];
		int ret = queued_tx_destination(pkt, address);

		if (ret < 0) {
			return ret;
		}
		*peer = peer_route_by_prefix(ctx, family, address);
		if (*peer == NULL || (*peer)->id != queued_peer_id) {
			vpn_stats_update_drop_tx(ctx);
			return -ESTALE;
		}
		return 0;
	}
#endif

	if (!ctx->shared) {
		*peer = ctx->peer != NULL ? ctx->peer : peer_lookup_by_iface(ctx->iface);
		if (*peer == NULL) {
			return -ENOENT;
		}
	}
	if (pkt_len == 0U) {
		return ctx->shared ? -EDESTADDRREQ : 0;
	}

	if (family == NET_AF_INET6) {
		if (pkt_len < sizeof(struct net_ipv6_hdr)) {
			vpn_stats_update_invalid_packet_len(ctx);
			return -EINVAL;
		}
		destination = NET_IPV6_HDR(pkt)->dst;
	} else if (family == NET_AF_INET) {
		if (pkt_len < sizeof(struct net_ipv4_hdr)) {
			vpn_stats_update_invalid_packet_len(ctx);
			return -EINVAL;
		}
		destination = NET_IPV4_HDR(pkt)->dst;
	} else {
		vpn_stats_update_invalid_ip_family(ctx);
		return -EAFNOSUPPORT;
	}

	if (ctx->shared) {
		*peer = peer_route_by_prefix(ctx, family, destination);
		if (*peer != NULL) {
			return 0;
		}
	} else if (wg_peer_is_allowed_ip(*peer, family, destination)) {
		return 0;
	}
	vpn_stats_update_denied_ip(ctx);

	return -ENETUNREACH;
}

static int send_inner_packet(struct net_if *iface, struct net_pkt *pkt, struct wg_peer *peer)
{
	struct wg_iface_context *ctx = net_if_get_device(iface)->data;
	struct msg_transport_data hdr = { 0 };
	struct wg_keypair *keypair;
	uint8_t *payload = tx_msg + sizeof(hdr);
	size_t pkt_len, copied, padded_len, msg_len;
	int ret;

#if defined(CONFIG_WIREGUARD_MULTI_PEER)
	if (peer == NULL && net_pkt_vpn_peer_id(pkt) < 0) {
		vpn_stats_update_drop_tx(ctx);
		return net_pkt_vpn_peer_id(pkt);
	}
#endif
	if (ctx->attached_to == NULL) {
		return -ENOENT;
	}

	if (net_pkt_get_len(pkt) != 0 && net_pkt_family(pkt) != NET_AF_INET &&
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

	if (peer == NULL) {
		ret = select_tx_peer(ctx, pkt, &peer);
		if (ret < 0) {
			return ret;
		}
	}
	if (peer_policy_denied(peer)) {
		return -EACCES;
	}

	if (!peer->session.keypair.current.is_valid) {
		ret = wg_send_keepalive(ctx, peer);
		return ret < 0 ? ret : -EAGAIN;
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

	msg_len = sizeof(hdr) + padded_len + WG_AUTHTAG_LEN;
	if (msg_len > sizeof(tx_msg)) {
		vpn_stats_update_invalid_packet_len(ctx);
		return -EMSGSIZE;
	}

	hdr.type = MESSAGE_TRANSPORT_DATA;
	hdr.receiver = keypair->remote_index;
	sys_put_le64(keypair->sending_counter, (uint8_t *)&hdr.counter);
	memcpy(tx_msg, &hdr, sizeof(hdr));

	copied = net_buf_linearize(payload, pkt_len, pkt->buffer, 0, pkt_len);
	if (copied != pkt_len) {
		crypto_zero(tx_msg, msg_len);
		vpn_stats_update_alloc_failed(ctx);
		return -EMSGSIZE;
	}
#ifdef CONFIG_TAILSCALE
	if (pkt_len != 0U && !tailscale_net_transmit(iface, pkt, peer->policy_identity,
						    payload, pkt_len, ctx->policy_context)) {
		crypto_zero(tx_msg, msg_len);
		return -EACCES;
	}
#endif
	memset(payload + pkt_len, 0, padded_len - pkt_len);

	ret = wg_encrypt_packet(payload, payload, padded_len, keypair);
	if (ret == 0) {
		ret = send_datagram(peer, NULL, tx_msg, msg_len, 0, pkt_len != 0U ? pkt : NULL);
	}
	crypto_zero(tx_msg, msg_len);
	if (ret < 0) {
		vpn_stats_update_drop_tx(ctx);
		return ret;
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

	net_pkt_unref(pkt);
	return 0;
}

static int interface_send(struct net_if *iface, struct net_pkt *pkt)
{
	int ret;

	k_mutex_lock(&lock, K_FOREVER);
	ret = send_inner_packet(iface, pkt, NULL);
	k_mutex_unlock(&lock);

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
	int ret;

	if (ctx == NULL || config->private_key.len != WG_PRIVATE_KEY_LEN) {
		return -EINVAL;
	}

	ret = wg_set_interface_private_key(ctx, config->private_key.data);
	if (ret < 0) {
		return ret;
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
	int ret;

	switch (type) {
	case VIRTUAL_INTERFACE_CONFIG_TYPE_PRIVATE_KEY:
		if (k_mutex_lock(&lock, K_NO_WAIT) != 0) {
			return -EBUSY;
		}
		if (atomic_get(&ctx->peer_state) != WG_PEER_IFACE_FREE) {
			ret = -EBUSY;
		} else {
			ret = init_iface_context(ctx, config);
		}
		k_mutex_unlock(&lock);
		return ret;

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
		if (k_mutex_lock(&lock, K_NO_WAIT) != 0) {
			return -EBUSY;
		}
		memcpy(config->public_key.data, ctx->public_key,
		       sizeof(ctx->public_key));
		config->public_key.len = sizeof(ctx->public_key);
		k_mutex_unlock(&lock);
		return 0;

	default:
		break;
	}

	return -ENOTSUP;
}

static DEVICE_API(virtual_interface, wg_iface_api) = {
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
		.slot = x,						\
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

LISTIFY(WG_IFACE_COUNT, WG_INTERFACE_INIT, (;), _);

static int create_ipv4_packet(struct net_if *iface,
			      struct net_sockaddr *src,
			      struct net_sockaddr *dst,
			      const uint8_t *packet,
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
					K_NO_WAIT);
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
			      const uint8_t *packet,
			      size_t packet_len,
			      struct net_pkt **reply_pkt)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(iface,
					NET_UDPH_LEN + packet_len,
					NET_AF_INET6, NET_IPPROTO_UDP,
					K_NO_WAIT);
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
			 const uint8_t *packet,
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
		ret = net_pkt_write(*pkt, packet, packet_len);
		if (ret < 0) {
			NET_DBG("Cannot write packet data");
			goto out;
		}
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
static K_FIFO_DEFINE(native_tx_queue);

#if defined(CONFIG_WIREGUARD_PEER_POLICY)
struct wg_native_policy_tx {
	void *fifo_reserved;
	struct net_pkt *pkt;
	int peer_id;
	int64_t deadline;
	net_sa_family_t family;
	uint8_t destination[sizeof(struct net_in6_addr)];
};

static K_FIFO_DEFINE(native_policy_tx_queue);
K_MEM_SLAB_DEFINE_STATIC(native_policy_tx_slab, sizeof(struct wg_native_policy_tx),
			 CONFIG_NET_PKT_TX_COUNT, __alignof__(struct wg_native_policy_tx));

static int policy_queue_datagram(struct wg_peer *peer, struct net_pkt *pkt,
				 struct net_pkt *plaintext)
{
	struct wg_native_policy_tx *tx;
	void *block;
	int ret;

	ret = k_mem_slab_alloc(&native_policy_tx_slab, &block, K_NO_WAIT);
	if (ret < 0) {
		return ret;
	}
	tx = block;
	tx->family = NET_AF_UNSPEC;
	if (plaintext != NULL) {
		ret = queued_tx_destination(plaintext, tx->destination);
		if (ret < 0) {
			k_mem_slab_free(&native_policy_tx_slab, tx);
			return ret;
		}
		tx->family = net_pkt_family(plaintext);
	}
	tx->peer_id = peer->id;
	tx->deadline = peer_deadline(peer);
	tx->pkt = pkt;
	k_fifo_put(&native_policy_tx_queue, tx);

	return 0;
}

static struct net_pkt *policy_take_datagram(struct wg_policy_handoff *handoff)
{
	struct wg_native_policy_tx *tx = k_fifo_get(&native_policy_tx_queue, K_NO_WAIT);
	struct wg_peer *peer;
	struct net_pkt *pkt;

	if (tx == NULL) {
		return NULL;
	}
	pkt = tx->pkt;
	k_mutex_lock(&lock, K_FOREVER);
	peer = peer_lookup_by_id(tx->peer_id);
	if (peer != NULL &&
	    peer->transport.send == NULL &&
	    (peer->underlay == NULL || net_pkt_vpn_iface(pkt) == peer->underlay) &&
	    (tx->family == NET_AF_UNSPEC ||
	     peer_route_by_prefix(peer->ctx, tx->family, tx->destination) == peer) &&
	    !peer_policy_denied(peer) && (tx->deadline == 0 || k_uptime_get() < tx->deadline)) {
		policy_handoff_begin(peer->ctx, handoff);
	}
	k_mutex_unlock(&lock);
	k_mem_slab_free(&native_policy_tx_slab, tx);
	if (handoff->ctx == NULL) {
		net_pkt_unref(pkt);
		return NULL;
	}

	return pkt;
}
#endif

static void native_tx_handler(struct k_work *work)
{
	const struct device *dev = net_if_get_device(wg_ctx.iface);
	const struct dummy_driver_api *api = dev->api;
	struct net_pkt *pkt = k_fifo_get(&native_tx_queue, K_NO_WAIT);
	bool again;
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	struct wg_policy_handoff handoff = {0};
#endif

	if (pkt != NULL && net_l2_send(api->send, dev, wg_ctx.iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	pkt = policy_take_datagram(&handoff);
	if (pkt != NULL && net_l2_send(api->send, dev, wg_ctx.iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}
	if (handoff.ctx != NULL) {
		k_mutex_lock(&lock, K_FOREVER);
		policy_handoff_end(&handoff);
		k_mutex_unlock(&lock);
	}
#endif
	again = !k_fifo_is_empty(&native_tx_queue);
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	again = again || !k_fifo_is_empty(&native_policy_tx_queue);
#endif
	if (again) {
		k_work_submit_to_queue(WG_WORKQ, work);
	}
}

static K_WORK_DEFINE(native_tx_work, native_tx_handler);

static int queue_native_datagram(struct wg_peer *peer, struct net_pkt *pkt,
				 struct net_pkt *plaintext)
{
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	if (peer->policy_identity != 0U) {
		return policy_queue_datagram(peer, pkt, plaintext);
	}
#endif
	k_fifo_put(&native_tx_queue, pkt);
	return 0;
}

static int send_datagram(struct wg_peer *peer, const struct wireguard_datagram_path *path,
			 const uint8_t *data, size_t len, uint8_t dscp, struct net_pkt *plaintext)
{
	struct net_sockaddr_storage local = { 0 };
	struct net_sockaddr_storage remote;
	struct net_if *underlay = peer_underlay(peer);
	struct net_if *iface;
	struct net_pkt *pkt = NULL;
	int ret;

	if (peer_policy_denied(peer)) {
		return -EACCES;
	}
	if (peer->transport.send != NULL) {
		return peer->transport.send(peer->id, path, data, len, peer->transport.user_data);
	}

	if (path != NULL) {
		iface = path->iface;
		if (underlay != NULL && iface != underlay) {
			return -ENETUNREACH;
		}
		local = path->local;
		remote = path->remote;
	} else {
		remote = peer->endpoint;
		ret = select_target_iface(peer, net_sad(&local), &remote, &iface);
		if (ret < 0) {
			vpn_stats_update_invalid_ip_family(peer->ctx);
			return ret;
		}
	}

	if (iface == NULL || iface == peer->iface) {
		return -ENETUNREACH;
	}

	ret = create_packet(iface, net_sad(&local), net_sad(&remote), data, len, dscp, 0, &pkt);
	if (ret == 0) {
		net_pkt_set_vpn_iface(pkt, iface);
		if (underlay != NULL && remote.ss_family == NET_AF_INET6) {
			net_pkt_set_ipv6_iface_bound(pkt, true);
		}
		/* Only the completed outer packet crosses the lock boundary.
		 * The TX packet slab bounds this FIFO, including during congestion.
		 */
		ret = queue_native_datagram(peer, pkt, plaintext);
		if (ret == 0) {
			k_work_submit_to_queue(WG_WORKQ, &native_tx_work);
		}
	} else {
		vpn_stats_update_alloc_failed(peer->ctx);
	}
	if (ret < 0 && pkt != NULL) {
		net_pkt_unref(pkt);
	}
	return ret;
}

static int wg_send_handshake_response(struct wg_peer *peer, struct wireguard_datagram_path *path)
{
	struct msg_handshake_response packet = { 0 };
	int ret;

	if (!wg_create_handshake_response(peer->ctx, peer, &packet)) {
		vpn_stats_update_invalid_handshake(peer->ctx);
		return -EINVAL;
	}

	ret = wg_start_session(peer, false);
	if (ret == 0) {
		ret = send_datagram(peer, path, (uint8_t *)&packet, sizeof(packet),
				    NET_IPV4_DSCP_AF41, NULL);
	}
	if (ret == 0) {
		vpn_stats_update_handshake_resp_tx(peer->ctx);
	}
	return ret;
}

#include "wg_psa.c"

#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
struct wg_handshake_pin {
	struct wg_policy_handoff handoff;
	struct net_if *underlay;
	wireguard_datagram_send_t send;
	uint64_t policy_generation;
	atomic_val_t iface_generation;
};

static bool handshake_can_unlock(const struct wg_iface_context *ctx)
{
	return ctx->policy_input != NULL &&
	       k_current_get() == k_work_queue_thread_get(&wg_handshake_workq);
}

static void handshake_pin_begin(struct wg_iface_context *ctx, struct wg_peer *peer,
				struct wg_handshake_pin *pin)
{
	pin->policy_generation = ctx->handshake_policy_generation;
	pin->iface_generation = atomic_get(&ctx->handshake_iface_generation);
	if (peer != NULL) {
		pin->underlay = peer->underlay;
		pin->send = peer->transport.send;
	}
	policy_handoff_begin(ctx, &pin->handoff);
	unlocked_handshake_peer = peer;
	k_mutex_unlock(&lock);
}

static bool handshake_pin_end(struct wg_handshake_pin *pin, struct wg_peer *peer)
{
	struct wg_iface_context *ctx = pin->handoff.ctx;

	k_mutex_lock(&lock, K_FOREVER);
	unlocked_handshake_peer = NULL;
	policy_handoff_end(&pin->handoff);
	if (!net_if_is_up(ctx->iface) ||
	    pin->iface_generation != atomic_get(&ctx->handshake_iface_generation)) {
		return false;
	}
	/* An initiation without an identified peer belongs to the policy that received it. */
	return peer == NULL ? pin->policy_generation == ctx->handshake_policy_generation :
		peer_lookup_by_id(peer->id) == peer && !peer_policy_denied(peer) &&
		pin->underlay == peer->underlay && pin->send == peer->transport.send;
}
#endif

static int handshake_key_agreement(struct wg_iface_context *ctx, struct wg_peer *peer,
				   uint8_t *secret, psa_key_id_t private_key,
				   const uint8_t *public_key)
{
#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
	if (handshake_can_unlock(ctx)) {
		struct wg_handshake_pin pin;
		int ret;

		handshake_pin_begin(ctx, peer, &pin);
		ret = wg_psa_x25519_hybrid(secret, private_key, public_key);
		return handshake_pin_end(&pin, peer) ? ret : -ECANCELED;
	}
#else
	ARG_UNUSED(ctx);
	ARG_UNUSED(peer);
#endif
	return wg_psa_x25519_hybrid(secret, private_key, public_key);
}

static int handshake_generate_keypair(struct wg_peer *peer, uint8_t *public_key)
{
#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
	if (handshake_can_unlock(peer->ctx)) {
		struct wg_handshake_pin pin;
		psa_key_id_t private_key = PSA_KEY_ID_NULL;
		int ret;

		handshake_pin_begin(peer->ctx, peer, &pin);
		ret = wg_psa_generate_x25519_keypair(&private_key, public_key);
		if (!handshake_pin_end(&pin, peer)) {
			wg_psa_destroy_key(private_key);
			return -ECANCELED;
		}
		if (ret == 0) {
			peer->handshake.ephemeral_private_id = private_key;
		}
		return ret;
	}
#endif
	return wg_psa_generate_x25519_keypair(&peer->handshake.ephemeral_private_id, public_key);
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

ZTESTABLE_STATIC struct wg_peer *peer_lookup_by_id(int id)
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
	struct wg_peer *peer;
	struct wg_peer *match = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&active_peers, peer, node) {
		if ((ctx == NULL || peer->ctx == ctx) &&
		    get_peer_keypair_for_index(peer, receiver) != NULL) {
			if (match != NULL) {
				return NULL;
			}
			match = peer;
		}
	}

	return match;
}

static struct wg_peer *peer_lookup_by_handshake(struct wg_iface_context *ctx,
						uint32_t receiver)
{
	struct wg_peer *peer;
	struct wg_peer *match = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&active_peers, peer, node) {
		if ((ctx == NULL || peer->ctx == ctx) &&
		    peer->handshake.is_valid &&
		    peer->handshake.is_initiator &&
		    peer->handshake.local_index == receiver) {
			if (match != NULL) {
				return NULL;
			}
			match = peer;
		}
	}

	return match;
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
		found = ((ctx == NULL || peer->ctx == ctx) &&
			 (index == peer->session.keypair.current.local_index ||
			  index == peer->session.keypair.prev.local_index ||
			  index == peer->session.keypair.next.local_index ||
			  index == peer->handshake.local_index));
		if (found) {
			break;
		}
	}

	return found;
}

static uint32_t generate_unique_index(struct wg_iface_context *ctx)
{
	uint32_t index = 0;
	int ret;

	if (IS_ENABLED(CONFIG_WIREGUARD_MULTI_PEER)) {
		/* All native identities share the same UDP listener. */
		ctx = NULL;
	}

	do {
		do {
			ret = sys_csrand_get((uint8_t *)&index, sizeof(index));
			if (ret < 0) {
				/* Index does not need to be a secret but should be
				 * random enough to avoid collisions.
				 */
				index = sys_rand32_get();
			}
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

static struct wg_iface_context *get_wg_iface_context(struct net_if *iface)
{
	const struct device *dev;

	if (iface == NULL || net_if_get_by_iface(iface) < 0) {
		return NULL;
	}
	dev = net_if_get_device(iface);
	return dev != NULL && dev->api == &wg_iface_api ? dev->data : NULL;
}

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct net_if **ret_iface = user_data;
	struct wg_iface_context *ctx = get_wg_iface_context(iface);

	if (*ret_iface == NULL && ctx != NULL &&
	    atomic_get(&ctx->peer_state) == WG_PEER_IFACE_FREE && ctx->attached_to == NULL) {
		*ret_iface = iface;
	}
}

uint64_t wireguard_policy_route_owner(struct net_if *iface, net_sa_family_t family,
				     const void *address)
{
	uint64_t identity = 0;

#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	struct wg_iface_context *ctx = get_wg_iface_context(iface);

	if (ctx != NULL && address != NULL && net_if_is_up(iface)) {
		(void)route_peer_id(ctx, family, address, true, &identity);
	}
#endif
	return identity;
}

void wireguard_policy_block(struct net_if *iface)
{
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	struct wg_iface_context *ctx = get_wg_iface_context(iface);

	if (ctx != NULL) {
		atomic_set(&ctx->policy_blocked, 1);
	}
#endif
}

static int wg_peer_init(struct wg_iface_context *ctx,
			 struct wg_peer *peer,
			 const uint8_t *public_key,
			 const uint8_t *preshared_key)
{
	int ret;

	memset(peer, 0, sizeof(struct wg_peer));

	if (ctx == NULL) {
		return -EINVAL;
	}

	/* Copy raw public key for lookups */
	memcpy(peer->key.public_key, public_key, WG_PUBLIC_KEY_LEN);
	memset(peer->greatest_timestamp, 0, sizeof(peer->greatest_timestamp));

	/* Import keys into PSA */
	ret = wg_set_peer_keys(peer, public_key, preshared_key);
	if (ret < 0) {
		return ret;
	}

	/* Precompute static DH */
	ret = wg_precompute_static_dh(ctx, peer);
	if (ret < 0) {
		NET_DBG("Cannot precompute static DH for peer %d (%d)", peer->id, ret);
		wg_cleanup_peer_keys(peer);
		return ret;
	}

	memset(&peer->handshake, 0, sizeof(struct wg_handshake));
	peer->handshake.is_valid = false;

	peer->cookie_secret_expires =
		sys_timepoint_calc(K_MSEC(COOKIE_SECRET_MAX_AGE_MSEC));
	memset(&peer->cookie, 0, WG_COOKIE_LEN);

	wg_mac_key(peer->label_mac1_key, peer->key.public_key,
		   LABEL_MAC1, sizeof(LABEL_MAC1));
	wg_mac_key(peer->label_cookie_key, peer->key.public_key,
		   LABEL_COOKIE, sizeof(LABEL_COOKIE));
	return 0;
}

static void peer_set_allowed_addr(struct wg_peer *peer,
				  const struct wireguard_peer_config *peer_config)
{
	ARRAY_FOR_EACH(peer_config->allowed_ip, i) {
		const struct wireguard_allowed_ip *allowed_ip = &peer_config->allowed_ip[i];

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

static void wg_peer_cleanup(struct wg_peer *peer)
{
	wg_cleanup_peer_keys(peer);
	crypto_zero(peer, sizeof(*peer));
	sys_slist_prepend(&peer_list, &peer->node);
}

static int validate_shared_prefixes(struct wg_iface_context *ctx,
				    const struct wireguard_peer_config *config)
{
	struct wg_peer *peer;

	ARRAY_FOR_EACH(config->allowed_ip, i) {
		const struct wireguard_allowed_ip *prefix = &config->allowed_ip[i];
		net_sa_family_t family = prefix->addr.family;
		const uint8_t *address;

		if (!prefix->is_valid) {
			continue;
		}
		if (family == NET_AF_INET && prefix->mask_len <= 32U) {
			address = (const uint8_t *)&prefix->addr.in_addr;
		} else if (family == NET_AF_INET6 && prefix->mask_len <= 128U) {
			address = prefix->addr.in6_addr.s6_addr;
		} else {
			return -EINVAL;
		}

		SYS_SLIST_FOR_EACH_CONTAINER(&active_peers, peer, node) {
			if (peer->ctx != ctx) {
				continue;
			}
			ARRAY_FOR_EACH(peer->allowed_ip, j) {
				const struct wg_allowed_ip *existing = &peer->allowed_ip[j];

				if (existing->mask_len == prefix->mask_len &&
				    wg_allowed_ip_match(existing, family, address)) {
					return -EADDRINUSE;
				}
			}
		}
	}

	return 0;
}

#if defined(CONFIG_WIREGUARD_PEER_POLICY)
struct wg_policy_key_lookup {
	struct wg_iface_context *ctx;
	bool in_use;
};

static void check_managed_key(struct net_if *iface, void *user_data)
{
	struct wg_policy_key_lookup *lookup = user_data;
	struct wg_iface_context *ctx = get_wg_iface_context(iface);

	if (ctx != NULL && ctx != lookup->ctx && ctx->policy_input != NULL &&
	    memcmp(ctx->public_key, lookup->ctx->public_key, WG_PUBLIC_KEY_LEN) == 0) {
		lookup->in_use = true;
	}
}
#endif

static bool native_key_in_use(struct wg_iface_context *ctx)
{
	struct wg_peer *peer;

	SYS_SLIST_FOR_EACH_CONTAINER(&active_peers, peer, node) {
		if (peer->ctx != ctx && peer->transport.send == NULL &&
		    memcmp(peer->ctx->public_key, ctx->public_key, WG_PUBLIC_KEY_LEN) == 0) {
			return true;
		}
	}
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	struct wg_policy_key_lookup lookup = {.ctx = ctx};

	net_if_foreach(check_managed_key, &lookup);
	return lookup.in_use;
#else
	return false;
#endif
}

static void release_peer_iface(struct wg_iface_context *ctx)
{
	ctx->shared = false;
	atomic_set(&ctx->peer_state, WG_PEER_IFACE_FREE);
}

static void publish_peer(struct wg_peer *peer)
{
#if defined(CONFIG_WIREGUARD_MULTI_PEER)
	size_t slot = peer - peers;
	k_spinlock_key_t key = k_spin_lock(&tx_routes_lock);

	peer->ctx->route_mask[slot / 32U] |= BIT(slot % 32U);
	k_spin_unlock(&tx_routes_lock, key);
#endif
	sys_slist_prepend(&active_peers, &peer->node);
}

static bool unpublish_peer(struct wg_peer *peer)
{
	if (!sys_slist_find_and_remove(&active_peers, &peer->node)) {
		return false;
	}
#if defined(CONFIG_WIREGUARD_MULTI_PEER)
	size_t slot = peer - peers;
	k_spinlock_key_t key = k_spin_lock(&tx_routes_lock);
	bool wait = route_readers[route_epoch] != 0U;

	peer->ctx->route_mask[slot / 32U] &= ~BIT(slot % 32U);
	if (wait) {
		route_epoch ^= 1U;
	}
	k_spin_unlock(&tx_routes_lock, key);
	if (wait) {
		/* New lookups cannot delay recycling this retired slot. */
		k_sem_take(&routes_idle, K_FOREVER);
	}
#endif
	return true;
}

static void copy_peer_public_info(struct wireguard_peer_info *info, const struct wg_peer *peer)
{
	memcpy(info->public_key, peer->key.public_key, sizeof(info->public_key));
	info->endpoint = peer->cfg_endpoint;
	info->keepalive_interval = peer->keepalive_interval;
	ARRAY_FOR_EACH(info->allowed_ip, i) {
		info->allowed_ip[i].addr = peer->allowed_ip[i].addr;
		info->allowed_ip[i].mask_len = peer->allowed_ip[i].mask_len;
		info->allowed_ip[i].is_valid = peer->allowed_ip[i].is_valid;
	}
}

struct wg_peer_add_event {
	struct net_mgmt_event_owned_info owner;
	struct net_event_vpn_peer event;
	struct wireguard_peer_info snapshot;
	char public_key[4 * DIV_ROUND_UP(WG_PUBLIC_KEY_LEN, 3) + 1];
};

#ifdef CONFIG_NET_MGMT_EVENT_QUEUE
/* One snapshot per queued event, one in dispatch, and one being submitted. */
#define WG_PEER_ADD_EVENT_COUNT (CONFIG_NET_MGMT_EVENT_QUEUE_SIZE + 2)
#else
#define WG_PEER_ADD_EVENT_COUNT 1
#endif

K_MEM_SLAB_DEFINE_STATIC_TYPE(peer_add_events, struct wg_peer_add_event, WG_PEER_ADD_EVENT_COUNT);

static void release_peer_add_event(struct net_mgmt_event_owned_info *owner)
{
	k_mem_slab_free(&peer_add_events, CONTAINER_OF(owner, struct wg_peer_add_event, owner));
}

/* The lifecycle operation keeps the peer's configuration stable during the copy. */
static void notify_peer_add(const struct wg_peer *peer)
{
	struct wg_peer_add_event *snapshot;
	void *block;
	size_t len;

	if (k_mem_slab_alloc(&peer_add_events, &block, K_NO_WAIT) != 0) {
		NET_WARN("Could not allocate add event for WireGuard peer %d", peer->id);
		return;
	}
	snapshot = block;
	copy_peer_public_info(&snapshot->snapshot, peer);
	if (base64_encode(snapshot->public_key, sizeof(snapshot->public_key), &len,
			  snapshot->snapshot.public_key,
			  sizeof(snapshot->snapshot.public_key)) != 0) {
		k_mem_slab_free(&peer_add_events, snapshot);
		return;
	}
	snapshot->event = (struct net_event_vpn_peer){
		.id = peer->id,
		.public_key = snapshot->public_key,
		.endpoint = net_sad(&snapshot->snapshot.endpoint),
		.keepalive_interval = snapshot->snapshot.keepalive_interval,
	};
	ARRAY_FOR_EACH(snapshot->snapshot.allowed_ip, i) {
		snapshot->event.allowed_ip[i] = &snapshot->snapshot.allowed_ip[i];
	}
	snapshot->owner = (struct net_mgmt_event_owned_info){
		.data = &snapshot->event,
		.release = release_peer_add_event,
	};
	net_mgmt_event_notify_owned_info(NET_EVENT_VPN_PEER_ADD, peer->iface,
					&snapshot->owner, sizeof(snapshot->event));
}

static int peer_add(const struct wireguard_peer_config *peer_config, struct net_if **peer_iface,
		    struct net_if *iface,
		    const struct wireguard_transport *transport)
{
	uint8_t public_key[WG_PUBLIC_KEY_LEN];
	struct wg_iface_context *ctx;
	struct wg_peer *peer;
	sys_snode_t *node;
	bool shared = iface != NULL;
	bool first;
	int ret;

	if (peer_config == NULL || peer_iface == NULL || peer_config->public_key == NULL) {
		NET_DBG("Public key not set");
		return -EINVAL;
	}

	if (!extract_public_key(peer_config->public_key, public_key,
				sizeof(public_key))) {
		NET_DBG("Invalid public_key base64 format");
		return -EINVAL;
	}

	/* Serialize changes to the control interface's attachment list. Synchronous
	 * management callbacks can call back into WireGuard.
	 */
	if (!atomic_cas(&wg_ctx.lifecycle_busy, 0, 1)) {
		return -EBUSY;
	}

	k_mutex_lock(&lock, K_FOREVER);

	if (!shared && peer_lookup_by_pubkey(NULL, public_key) != NULL) {
		ret = -EALREADY;
		goto out;
	}

	if (last_peer_id == INT_MAX) {
		ret = -ENOSPC;
		goto out;
	}

	if (!shared) {
		/* Find a free dedicated WireGuard interface. */
		net_if_foreach(iface_cb, &iface);
		if (iface == NULL) {
			ret = -ENOMEM;
			NET_INFO("No available Wireguard interfaces found");
			goto out;
		}
	}

	ctx = get_wg_iface_context(iface);
	if (ctx == NULL) {
		ret = -EINVAL;
		goto out;
	}
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	if (ctx->policy_input != NULL) {
		ret = -EBUSY;
		goto out;
	}
#endif
	first = atomic_get(&ctx->peer_state) == WG_PEER_IFACE_FREE;
	if (shared) {
		if ((!first && !ctx->shared) || (first && ctx->attached_to != NULL)) {
			ret = -EBUSY;
			goto out;
		}
		if (ctx->private_key_id == PSA_KEY_ID_NULL) {
			ret = -EINVAL;
			goto out;
		}
		if (peer_lookup_by_pubkey(ctx, public_key) != NULL) {
			ret = -EALREADY;
			goto out;
		}
		ret = validate_shared_prefixes(ctx, peer_config);
		if (ret < 0) {
			goto out;
		}
	}
	if (IS_ENABLED(CONFIG_WIREGUARD_MULTI_PEER) && transport == NULL &&
	    native_key_in_use(ctx)) {
		ret = -EADDRINUSE;
		goto out;
	}

	if (WG_PEER_SLOTS > CONFIG_WIREGUARD_MAX_PEER &&
	    sys_slist_len(&active_peers) >= CONFIG_WIREGUARD_MAX_PEER) {
		ret = -ENOMEM;
		goto out;
	}

	node = sys_slist_get(&peer_list);
	if (node == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	peer = CONTAINER_OF(node, struct wg_peer, node);

	if (wg_peer_init(ctx, peer, public_key, peer_config->preshared_key) < 0) {
		NET_DBG("Peer init failed");
		ret = -EINVAL;
		sys_slist_prepend(&peer_list, node);
		goto out;
	}

	if (transport != NULL) {
		peer->transport = *transport;
	}
	memcpy(&peer->cfg_endpoint, &peer_config->endpoint_ip,
	       sizeof(peer->cfg_endpoint));

	if (net_sin(net_sad(&peer->cfg_endpoint))->sin_port == 0) {
		net_sin(net_sad(&peer->cfg_endpoint))->sin_port = net_htons(WG_DEFAULT_PORT);
	}

	peer->id = ++last_peer_id;
	peer->iface = iface;
	peer->ctx = ctx;

	peer_set_allowed_addr(peer, peer_config);

	if (peer_config->keepalive_interval > KEEPALIVE_DEFAULT) {
		peer->keepalive_interval = (uint16_t)peer_config->keepalive_interval;
		peer->keepalive_expires =
			sys_timepoint_calc(K_SECONDS(peer->keepalive_interval));
	} else {
		peer->keepalive_interval = 0;
	}

	if (first) {
		ctx->shared = shared;
		atomic_set(&ctx->peer_state, WG_PEER_IFACE_ADDING);
		k_mutex_unlock(&lock);

		ret = net_virtual_interface_attach(iface, wg_ctx.iface);
		if (ret < 0) {
			NET_DBG("Cannot attach %d to %d", net_if_get_by_iface(iface),
				net_if_get_by_iface(wg_ctx.iface));
			k_mutex_lock(&lock, K_FOREVER);
			wg_peer_cleanup(peer);
			release_peer_iface(ctx);
			goto out;
		}

		atomic_inc(&wg_ctx.attached_peers);
		ret = net_if_up(iface);
		if (ret < 0 && ret != -EALREADY) {
			atomic_set(&ctx->peer_state, WG_PEER_IFACE_RETIRING);
			(void)net_virtual_interface_attach(iface, NULL);
			atomic_dec(&wg_ctx.attached_peers);
			k_mutex_lock(&lock, K_FOREVER);
			wg_peer_cleanup(peer);
			release_peer_iface(ctx);
			goto out;
		}
		k_mutex_lock(&lock, K_FOREVER);
		atomic_set(&ctx->peer_state, WG_PEER_IFACE_ACTIVE);
	}

	if (!shared) {
		ctx->peer = peer;
	}
	publish_peer(peer);
	ret = peer->id;
	*peer_iface = iface;
	k_mutex_unlock(&lock);

	NET_DBG("Peer %d attached to interface %d", ret,
		net_if_get_by_iface(peer->iface));

	notify_peer_add(peer);

	atomic_clear(&wg_ctx.lifecycle_busy);
	return ret;

out:
	k_mutex_unlock(&lock);
	atomic_clear(&wg_ctx.lifecycle_busy);

	return ret;
}

int wireguard_peer_add(struct wireguard_peer_config *config, struct net_if **iface)
{
	return peer_add(config, iface, NULL, NULL);
}

int wireguard_peer_add_with_transport(struct wireguard_peer_config *config, struct net_if **iface,
				      const struct wireguard_transport *transport)
{
	if (transport == NULL || transport->send == NULL) {
		return -EINVAL;
	}
	return peer_add(config, iface, NULL, transport);
}

int wireguard_peer_add_to_interface(struct net_if *iface,
				    const struct wireguard_peer_config *peer_config,
				    const struct wireguard_transport *transport)
{
	struct net_if *peer_iface;

	if (!IS_ENABLED(CONFIG_WIREGUARD_MULTI_PEER)) {
		return -ENOTSUP;
	}
	if (iface == NULL || (transport != NULL && transport->send == NULL)) {
		return -EINVAL;
	}
	return peer_add(peer_config, &peer_iface, iface, transport);
}

static int validate_datagram(const struct wireguard_datagram_path *path,
			     const uint8_t *data, size_t len)
{
	if (path == NULL || data == NULL) {
		return -EINVAL;
	}
	if (len < sizeof(struct wg_msg_hdr) || len > CONFIG_WIREGUARD_BUF_LEN) {
		return -EMSGSIZE;
	}
	if (path->kind == WIREGUARD_PATH_DIRECT) {
		net_sa_family_t family = path->remote.ss_family;

		if (path->iface == NULL || path->local.ss_family != family ||
		    (family != NET_AF_INET && family != NET_AF_INET6) ||
		    net_sin(net_sad(&path->remote))->sin_port == 0U ||
		    net_sin(net_sad(&path->local))->sin_port == 0U) {
			return -EINVAL;
		}
	} else if (path->kind != WIREGUARD_PATH_RELAY) {
		return -EINVAL;
	}
	return 0;
}

static int inject_datagram(const struct wg_rx_scope *scope,
			   const struct wireguard_datagram_path *path,
			   const uint8_t *data, size_t len)
{
	struct wireguard_datagram_path incoming = *path;
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_rx_alloc_with_buffer(scope->ctx->iface, len, NET_AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		return -ENOMEM;
	}
	ret = net_pkt_write(pkt, data, len);
	if (ret == 0) {
		net_pkt_set_overwrite(pkt, true);
		ret = wg_receive_datagram(scope, pkt, 0, &incoming);
	}
	net_pkt_unref(pkt);
	return ret;
}

int wireguard_peer_receive_datagram(int peer_id, const struct wireguard_datagram_path *path,
				    const uint8_t *data, size_t len)
{
	struct wg_rx_scope scope = {0};
	struct wg_peer *peer;
	int ret = validate_datagram(path, data, len);

	if (ret < 0) {
		return ret;
	}
	k_mutex_lock(&lock, K_FOREVER);
	peer = peer_lookup_by_id(peer_id);
	if (peer == NULL) {
		ret = -ENOENT;
		goto out;
	}
	if (peer->transport.send == NULL) {
		ret = -ENOTSUP;
		goto out;
	}
	if (!net_if_is_up(peer->iface)) {
		ret = -ENETDOWN;
		goto out;
	}
	if (peer_policy_denied(peer)) {
		ret = -EACCES;
		goto out;
	}
	scope.ctx = peer->ctx;
	scope.expected_peer = peer;
	ret = inject_datagram(&scope, path, data, len);
out:
	k_mutex_unlock(&lock);
	return ret;
}

int wireguard_interface_receive_datagram(struct net_if *iface,
					 const struct wireguard_datagram_path *path,
					 const uint8_t *data, size_t len)
{
	struct wg_rx_scope scope = {0};
	int ret;

	if (!IS_ENABLED(CONFIG_WIREGUARD_MULTI_PEER)) {
		return -ENOTSUP;
	}
	ret = validate_datagram(path, data, len);
	if (ret < 0) {
		return ret;
	}
	scope.ctx = get_wg_iface_context(iface);
	if (scope.ctx == NULL || path->kind != WIREGUARD_PATH_DIRECT) {
		return -EINVAL;
	}

	k_mutex_lock(&lock, K_FOREVER);
	if (!scope.ctx->shared || atomic_get(&scope.ctx->peer_state) != WG_PEER_IFACE_ACTIVE) {
		ret = -ENOENT;
	} else if (!net_if_is_up(iface)) {
		ret = -ENETDOWN;
	} else {
		ret = inject_datagram(&scope, path, data, len);
	}
	k_mutex_unlock(&lock);
	return ret;
}

int wireguard_peer_get_info(int peer_id, struct wireguard_peer_info *info)
{
	struct wg_peer *peer;

	if (info == NULL) {
		return -EINVAL;
	}
	k_mutex_lock(&lock, K_FOREVER);
	peer = peer_lookup_by_id(peer_id);
	if (peer != NULL) {
		copy_peer_public_info(info, peer);
	}
	k_mutex_unlock(&lock);
	return peer != NULL ? 0 : -ENOENT;
}

#include "wg_policy.c"

int wireguard_peer_remove(int peer_id)
{
	struct wg_iface_context *ctx;
	struct net_if *iface;
	struct wg_peer *peer;
	bool last;
	int ret;

	if (!atomic_cas(&wg_ctx.lifecycle_busy, 0, 1)) {
		return -EBUSY;
	}

	k_mutex_lock(&lock, K_FOREVER);
	peer = peer_lookup_by_id(peer_id);
	if (peer == NULL) {
		ret = -ENOENT;
		goto out;
	}
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	if (peer->ctx->policy_input != NULL) {
		ret = -EBUSY;
		goto out;
	}
#endif

	if (peer->deliveries != 0U) {
		ret = -EBUSY;
		goto out;
	}

	if (!unpublish_peer(peer)) {
		ret = -EFAULT;
		goto out;
	}

	ctx = peer->ctx;
	iface = peer->iface;
	last = peer_lookup_by_iface(iface) == NULL;
	if (last) {
		atomic_set(&ctx->peer_state, WG_PEER_IFACE_RETIRING);
		ctx->peer = NULL;
		atomic_dec(&wg_ctx.attached_peers);
	}
	k_mutex_unlock(&lock);

	/* Interface shutdown can take network stack locks and invoke callbacks. */
	if (last) {
		ret = net_if_down(iface);
		if (ret == 0 || ret == -EALREADY) {
			ret = net_virtual_interface_attach(iface, NULL);
		}
		if (ret < 0 && ret != -EALREADY) {
			k_mutex_lock(&lock, K_FOREVER);
			atomic_inc(&wg_ctx.attached_peers);
			ctx->peer = ctx->shared ? NULL : peer;
			publish_peer(peer);
			atomic_set(&ctx->peer_state, WG_PEER_IFACE_ACTIVE);
			goto out;
		}
	}

	net_mgmt_event_notify_with_info(NET_EVENT_VPN_PEER_DEL, iface, &peer_id, sizeof(peer_id));

	k_mutex_lock(&lock, K_FOREVER);
	notify_connectivity(ctx);
	if (last) {
		release_peer_iface(ctx);
	}
	wg_peer_cleanup(peer);

	ret = 0;
out:
	k_mutex_unlock(&lock);
	atomic_clear(&wg_ctx.lifecycle_busy);

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
	if (IS_ENABLED(CONFIG_WIREGUARD_USE_REALTIME)) {
		struct timespec now;
		int rc = sys_clock_gettime(SYS_CLOCK_REALTIME, &now);

		if (rc == 0) {
			*seconds = now.tv_sec;
			*nanoseconds = now.tv_nsec;
		}
		return rc;
	}

	uint64_t millis = k_uptime_get();

	*seconds = millis / MSEC_PER_SEC;
	*nanoseconds = (millis % MSEC_PER_SEC) * NSEC_PER_MSEC;

	return 0;
}

/* Declare a default function but allow the user to override it */
__weak FUNC_ALIAS(get_current_time, wireguard_get_current_time, int);

SYS_INIT(wireguard_init, APPLICATION, CONFIG_NET_INIT_PRIO);
