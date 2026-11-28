/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <mbedtls/platform_util.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/random/random.h>
#ifdef CONFIG_SHELL
#include <zephyr/shell/shell.h>
#endif
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>
#include "control_internal.h"
#include "transport.h"
#ifdef CONFIG_TAILSCALE_DERP
#include "derp_conn.h"
#endif
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
#include "proxy.h"
#endif
#include "disco_box.h"

LOG_MODULE_REGISTER(tailscale_transport, LOG_LEVEL_INF);

#ifdef CONFIG_SOC_FAMILY_ESPRESSIF_SPIRAM
#define STORAGE __attribute__((section(".ext_ram.bss")))
#else
#define STORAGE
#endif
#define DIRECT_TRUST_MS 6500
#define DIRECT_REFRESH_MS 250
#define MAX_IDLE_MS     120000
#define RELAY_QUEUE_DEPTH 6
#define RELAY_SECONDARY_DWELL_MS 3000

struct tailscale_datagram_frame {
	void *fifo_reserved;
	struct wireguard_datagram_path reply;
	uint32_t epoch;
	int id;
	uint16_t len;
	bool has_reply;
	uint8_t data[TAILSCALE_DATAGRAM_MAX];
};

K_MEM_SLAB_DEFINE_STATIC(frames, ROUND_UP(sizeof(struct tailscale_datagram_frame), sizeof(void *)),
			 6, sizeof(void *));
static K_FIFO_DEFINE(outgoing);
static struct k_spinlock outgoing_lock;
static K_MUTEX_DEFINE(plan_lock);
static K_THREAD_STACK_DEFINE(datagram_stack, 8192);
static struct k_thread datagram_worker;
static STORAGE struct tailscale_transport_plan desired, current;

#ifdef CONFIG_TAILSCALE_DERP
struct channel {
	struct derp_conn conn;
	struct tailscale_derp_node node;
	int64_t retry;
	uint16_t region;
	size_t next_node;
};

struct relay_frame {
	void *fifo_reserved;
	uint32_t epoch;
	uint16_t region;
	uint16_t len;
	bool report_sent;
	bool report_drop;
	uint8_t data[DERP_FRAME_MAX];
};

static struct k_mem_slab relay_tx_frames, relay_rx_frames;
static STORAGE struct relay_frame relay_tx_storage[RELAY_QUEUE_DEPTH];
static STORAGE struct relay_frame relay_rx_storage[RELAY_QUEUE_DEPTH];
static K_FIFO_DEFINE(relay_outgoing);
static K_FIFO_DEFINE(relay_incoming);
static K_MUTEX_DEFINE(relay_send_lock);
static K_THREAD_STACK_DEFINE(relay_stack, 8192);
static struct k_thread relay_worker;
static STORAGE struct tailscale_transport_plan relay_plan;
static STORAGE struct channel channels[DERP_CONNECTIONS];
static STORAGE uint8_t relay_buf[DERP_FRAME_MAX + 1];
static atomic_t connected_regions[DERP_CONNECTIONS];
static atomic_t relay_ready_epoch;
static int64_t secondary_until;
#endif

static STORAGE uint8_t buf[TAILSCALE_DATAGRAM_MAX + 1
			   + (IS_ENABLED(CONFIG_TAILSCALE_REQUIRED_PROXY) ? 22 : 0)];
static struct {
	int id;
	struct net_sockaddr_storage endpoint;
	int64_t direct_until;
	int64_t last_probe;
	int64_t next_announce;
	int64_t next_candidate;
	size_t candidate_index;
	int64_t challenge_until;
	struct net_sockaddr_storage challenge_target;
	struct net_sockaddr_storage verified;
	uint8_t transaction[12];
	uint8_t disco[32];
	uint8_t box_key[32];
	bool have_box_key;
} paths[CONFIG_WIREGUARD_MAX_PEER];
static atomic_t epoch, running, home_region;
static atomic_t direct_rx, direct_tx, drops;
#if defined(CONFIG_TAILSCALE_DERP) || defined(CONFIG_SHELL)
static atomic_t connects, relay_rx, relay_tx, denied, unknown, relay_rx_drops;
#endif
static struct net_if *underlay;
static struct net_sockaddr_storage local;
static struct net_ifreq udp_binding;
static struct net_sockaddr_in initial_ipv4;
#ifdef CONFIG_TAILSCALE_DERP
static sec_tag_t ca_tag;
#endif
static mbedtls_svc_key_id_t node_key, disco_key;
static uint8_t disco_public[32];
static int udp_fd = -1, udp6_fd = -1;
static struct net_sockaddr_storage local6;
static struct net_sockaddr_storage mapped;
static int64_t mapped_until;
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
static int association = -1;
static struct k_spinlock proxy_lock;
static struct net_sockaddr_in udp_relay;
static atomic_t udp_generation, proxy_generation;
static atomic_val_t associated_generation;
static int64_t associate_after;
static STORAGE uint8_t socks_datagram[TAILSCALE_DATAGRAM_MAX + 22];

static bool proxy_endpoint(struct net_sockaddr_in *endpoint)
{
	K_SPINLOCK(&proxy_lock) {
		*endpoint = udp_relay;
	}
	return endpoint->sin_family == NET_AF_INET;
}

static void close_association(void)
{
	K_SPINLOCK(&proxy_lock) {
		udp_relay = (struct net_sockaddr_in){0};
	}
	if (association >= 0) {
		zsock_close(association);
		association = -1;
		atomic_inc(&proxy_generation);
	}
}

static void refresh_association(void)
{
	if (CONFIG_TAILSCALE_SOCKS_UDP_PORT == 0) {
		return;
	}
	atomic_val_t generation = atomic_get(&udp_generation);

	if (generation != associated_generation) {
		close_association();
		associated_generation = generation;
		associate_after = 0;
	}
	if (association >= 0) {
		struct zsock_pollfd pfd = {.fd = association, .events = ZSOCK_POLLIN};

		int rc = zsock_poll(&pfd, 1, 0);

		if (rc == 0 || (rc > 0 && pfd.revents == 0)) {
			return;
		}
		close_association();
	}
	if (relay_plan.home && k_uptime_get() >= associate_after) {
		struct net_sockaddr_in endpoint;

		association = tailscale_proxy_associate(underlay, &endpoint);
		if (association >= 0) {
			K_SPINLOCK(&proxy_lock) {
				udp_relay = endpoint;
			}
			atomic_inc(&proxy_generation);
		}
		associate_after = k_uptime_get() + 3000;
	}
}
#endif

#ifdef CONFIG_TAILSCALE_DERP
static bool has_region(const struct tailscale_transport_plan *p, uint16_t id, bool home)
{
	for (size_t i = 0; i < p->map.count; i++) {
		if (p->map.nodes[i].region == id && (!home || !p->map.nodes[i].no_home)) {
			return true;
		}
	}
	return false;
}
#endif

int tailscale_transport_stage(struct tailscale_transport_plan *plan,
			      const struct tailscale_peer_snapshot *snapshot)
{
#if defined(CONFIG_TAILSCALE_DERP) || defined(CONFIG_TAILSCALE_DISCOVER_ENDPOINT)
	plan->map = snapshot->derp;
#else
	ARG_UNUSED(snapshot);
#endif
	plan->epoch = (uint32_t)atomic_get(&epoch) + 1U;
#ifdef CONFIG_TAILSCALE_DERP
	memset(plan->regions, 0, sizeof(plan->regions));
	plan->home = 0;
	for (size_t i = 0; i < plan->map.count; i++) {
		const struct tailscale_derp_node *n = &plan->map.nodes[i];

		if (!n->no_home && (plan->home == 0 || n->region < plan->home)) {
			plan->home = n->region;
		}
	}
	if (has_region(plan, snapshot->self.derp_region, true)) {
		plan->home = snapshot->self.derp_region;
	}
	/* A single selected peer needs one connection when both ends use its
	 * advertised region. Region IDs carry no latency ordering.
	 */
	if (CONFIG_TAILSCALE_PEER_STABLE_ID[0] && plan->count == 1 &&
	    has_region(plan, plan->peers[0].region, true)) {
		plan->home = plan->peers[0].region;
	}
	plan->regions[0] = plan->home;
	for (size_t i = 0; i < plan->count; i++) {
		struct tailscale_transport_peer *peer = &plan->peers[i];

		if (!has_region(plan, peer->region, false)) {
			peer->region = plan->home;
		}
		if (plan->regions[1] == 0U && peer->region != plan->home) {
			plan->regions[1] = peer->region;
		}
	}
#endif
	return 0;
}

uint16_t tailscale_transport_home(void)
{
	return atomic_get(&home_region);
}

void tailscale_transport_publish(const struct tailscale_transport_plan *plan)
{
	k_mutex_lock(&plan_lock, K_FOREVER);
	if (plan == NULL) {
		desired.count = 0;
		mapped_until = 0;
#if defined(CONFIG_TAILSCALE_DERP) || defined(CONFIG_TAILSCALE_DISCOVER_ENDPOINT)
		desired.map.count = 0;
#endif
#ifdef CONFIG_TAILSCALE_DERP
		desired.home = 0;
		memset(desired.regions, 0, sizeof(desired.regions));
#endif
		desired.epoch = (uint32_t)atomic_get(&epoch) + 1U;
	} else {
		desired = *plan;
	}
	atomic_set(&epoch, desired.epoch);
#ifdef CONFIG_TAILSCALE_DERP
	if (desired.home != (uint16_t)atomic_get(&home_region)) {
		atomic_clear(&home_region);
	}
#endif
	k_mutex_unlock(&plan_lock);
#ifdef CONFIG_TAILSCALE_DERP
	/* Wait for a relay send admitted under the retired epoch without
	 * blocking the datagram worker's access to the published plan.
	 */
	k_mutex_lock(&relay_send_lock, K_FOREVER);
	k_mutex_unlock(&relay_send_lock);
#endif
}

int tailscale_transport_send(int id, const struct wireguard_datagram_path *reply,
			     const uint8_t *data, size_t len, void *stamp)
{
	struct tailscale_datagram_frame *frame;

	if (len > TAILSCALE_DATAGRAM_MAX) {
		return -EMSGSIZE;
	}
	if (!atomic_get(&running)) {
		return -ENETDOWN;
	}
	if (k_mem_slab_alloc(&frames, (void **)&frame, K_NO_WAIT) != 0) {
		atomic_inc(&drops);
		return -ENOBUFS;
	}
	frame->id = id;
	frame->epoch = POINTER_TO_UINT(stamp);
	frame->has_reply = reply != NULL;
	if (reply != NULL) {
		frame->reply = *reply;
	}
	frame->len = len;
	memcpy(frame->data, data, len);
	k_spinlock_key_t key = k_spin_lock(&outgoing_lock);
	bool live = atomic_get(&running);

	if (live) {
		k_fifo_put(&outgoing, frame);
	}
	k_spin_unlock(&outgoing_lock, key);
	if (!live) {
		k_mem_slab_free(&frames, frame);
		return -ENETDOWN;
	}
	return 0;
}

#ifdef CONFIG_TAILSCALE_DERP
static void disconnect(struct channel *ch)
{
	atomic_clear(&connected_regions[ch - channels]);
	if (ch == &channels[0]) {
		atomic_clear(&home_region);
	}
	if (ch->conn.io.fd >= 0) {
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
		tailscale_proxy_close(&ch->conn.io);
#else
		zsock_close(ch->conn.io.fd);
#endif
		ch->conn.io.fd = -1;
	}
	ch->conn.io.rx_pos = 0;
	ch->conn.io.rx_len = 0;
	ch->retry = k_uptime_get() + 800;
}
#endif

static bool same_endpoint(const struct net_sockaddr_storage *a,
			  const struct net_sockaddr_storage *b)
{
	const struct net_sockaddr_in *left = (const void *)a;
	const struct net_sockaddr_in *right = (const void *)b;

	if (a->ss_family == NET_AF_INET6 && b->ss_family == NET_AF_INET6) {
		const struct net_sockaddr_in6 *lhs = (const void *)a;
		const struct net_sockaddr_in6 *rhs = (const void *)b;

		return lhs->sin6_port == rhs->sin6_port &&
		       net_ipv6_addr_cmp(&lhs->sin6_addr, &rhs->sin6_addr);
	}
	return a->ss_family == NET_AF_INET && b->ss_family == NET_AF_INET &&
	       left->sin_port == right->sin_port &&
	       net_ipv4_addr_cmp(&left->sin_addr, &right->sin_addr);
}

static int shared_box_key(const struct tailscale_transport_peer *peer, uint8_t key[32])
{
	static const uint8_t zero[32];
	uint8_t shared[32];
	int rc = control_x25519_agree(disco_key, peer->disco, shared);

	if (rc == 0 && memcmp(shared, zero, sizeof(zero)) != 0) {
		disco_box_precompute(key, shared);
	} else {
		rc = -EACCES;
	}
	mbedtls_platform_zeroize(shared, sizeof(shared));
	return rc;
}

static void refresh_datagrams(void)
{
	k_mutex_lock(&plan_lock, K_FOREVER);
	current = desired;
	k_mutex_unlock(&plan_lock);
	if (!IS_ENABLED(CONFIG_TAILSCALE_DISCO)) {
		return;
	}
	memset(paths + current.count, 0, (ARRAY_SIZE(paths) - current.count) * sizeof(paths[0]));
	for (size_t i = 0; i < current.count; i++) {
		if (paths[i].id != current.peers[i].id ||
		    memcmp(paths[i].disco, current.peers[i].disco, 32) != 0 ||
		    memcmp(&paths[i].endpoint, &current.peers[i].endpoint,
			   sizeof(paths[i].endpoint)) != 0) {
			memset(&paths[i], 0, sizeof(paths[i]));
			memcpy(paths[i].disco, current.peers[i].disco, 32);
			paths[i].id = current.peers[i].id;
			paths[i].endpoint = current.peers[i].endpoint;
			paths[i].have_box_key =
				shared_box_key(&current.peers[i], paths[i].box_key) == 0;
		}
	}
}

#ifdef CONFIG_TAILSCALE_DERP
static bool peers_use_region(const struct tailscale_transport_plan *plan, uint16_t region)
{
	for (size_t i = 0; i < plan->count; i++) {
		if (plan->peers[i].region == region) {
			return true;
		}
	}
	return false;
}

static void refresh_relays(void)
{
	k_mutex_lock(&plan_lock, K_FOREVER);
	relay_plan = desired;
	k_mutex_unlock(&plan_lock);
	for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
		struct channel *ch = &channels[i];
		uint16_t region = relay_plan.regions[i];
		bool keep = false;

		if (i != 0U && ch->region != 0U && ch->region != relay_plan.home &&
		    peers_use_region(&relay_plan, ch->region)) {
			region = ch->region;
		}
		for (size_t j = 0; j < relay_plan.map.count; j++) {
			if (ch->region == region &&
			    !memcmp(&ch->node, &relay_plan.map.nodes[j], sizeof(ch->node))) {
				keep = true;
				break;
			}
		}
		if (!keep) {
			disconnect(ch);
			ch->region = region;
			ch->retry = 0;
			ch->next_node = 0;
		} else if (ch->conn.io.fd >= 0 && ch->region == relay_plan.home &&
			   relay_plan.epoch == (uint32_t)atomic_get(&epoch)) {
			atomic_set(&home_region, relay_plan.home);
		}
	}
	atomic_set(&relay_ready_epoch, relay_plan.epoch);
}

static void reconnect(struct channel *ch)
{
	if (ch->region == 0 || ch->conn.io.fd >= 0 || k_uptime_get() < ch->retry) {
		return;
	}
	for (size_t n = 0; n < relay_plan.map.count; n++) {
		size_t i = ch->next_node++ % relay_plan.map.count;
		const struct tailscale_derp_node *node = &relay_plan.map.nodes[i];

		if (node->region != ch->region) {
			continue;
		}
		ch->node = *node;
		int rc = derp_conn_open(&ch->conn, node, underlay, ca_tag, node_key,
					ch->region == relay_plan.home, relay_buf);

		if (rc == 0) {
			atomic_set(&connected_regions[ch - channels], ch->region);
			atomic_inc(&connects);
			if (ch->region == relay_plan.home &&
			    relay_plan.epoch == (uint32_t)atomic_get(&epoch)) {
				atomic_set(&home_region, ch->region);
				/* Start the peer session before sending its first UDP payload. */
				if (CONFIG_TAILSCALE_PEER_STABLE_ID[0] && relay_plan.count == 1 &&
				    relay_plan.peers[0].region == ch->region) {
					(void)wireguard_peer_keepalive(relay_plan.peers[0].id);
				}
			}
			LOG_INF("Authenticated DERP region %u", ch->region);
		} else {
			LOG_WRN("DERP region %u connect error %d", ch->region, rc);
		}
		ch->retry = k_uptime_get() + 1000;
		break;
	}
}
#endif

static int write_direct(const struct net_sockaddr_storage *to, const void *data, size_t len)
{
	bool v6 = to->ss_family == NET_AF_INET6;
	net_socklen_t size = v6 ? sizeof(struct net_sockaddr_in6) : sizeof(struct net_sockaddr_in);
	int fd = v6 ? udp6_fd : udp_fd;

	if (!v6 && to->ss_family != NET_AF_INET) {
		return -EAFNOSUPPORT;
	}
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
	struct net_sockaddr_storage relay = {0};

	if (!proxy_endpoint(net_sin(net_sad(&relay))) || len > TAILSCALE_DATAGRAM_MAX) {
		return -ENETDOWN;
	}
	const struct net_sockaddr_in *v4dst = (const void *)to;
	const struct net_sockaddr_in6 *v6dst = (const void *)to;
	size_t addr_size = v6 ? 16 : 4;

	memset(socks_datagram, 0, 3);
	socks_datagram[3] = v6 ? 4 : 1;
	memcpy(socks_datagram + 4,
	       v6 ? (const void *)&v6dst->sin6_addr : (const void *)&v4dst->sin_addr, addr_size);
	memcpy(socks_datagram + 4 + addr_size, v6 ? &v6dst->sin6_port : &v4dst->sin_port, 2);
	memcpy(socks_datagram + 6 + addr_size, data, len);
	len += 6 + addr_size;
	data = socks_datagram;
	to = &relay;
	size = sizeof(struct net_sockaddr_in);
	fd = udp_fd;
#endif
	if (fd < 0) {
		return -EAFNOSUPPORT;
	}
	ssize_t n = zsock_sendto(fd, data, len, ZSOCK_MSG_DONTWAIT, (const void *)to, size);

	return n == len ? 0 : n < 0 ? -errno : -EIO;
}

/* The datagram worker owns the STUN transaction. Control reads the mapping under plan_lock. */
#ifdef CONFIG_TAILSCALE_DISCOVER_ENDPOINT
static struct net_sockaddr_storage stun_server;
static uint8_t stun_id[12];
static int64_t stun_next, stun_deadline;
static size_t stun_node;
#endif
#if defined(CONFIG_TAILSCALE_DISCOVER_ENDPOINT) || defined(CONFIG_SHELL)
static atomic_t binding_requests, binding_replies;
#endif

void tailscale_transport_get_endpoints(struct tailscale_transport_endpoints *endpoints)
{
	k_mutex_lock(&plan_lock, K_FOREVER);
	*endpoints = (struct tailscale_transport_endpoints){
		.local_ipv4 = local,
		.local_ipv6 = local6,
		.public_ipv4 = mapped_until > k_uptime_get()
				? mapped : (struct net_sockaddr_storage){0},
	};
	k_mutex_unlock(&plan_lock);
}

static int bind_datagram(const struct net_sockaddr_storage *address)
{
	const bool ipv6 = address->ss_family == NET_AF_INET6;
	net_socklen_t len = ipv6 ? sizeof(struct net_sockaddr_in6) : sizeof(struct net_sockaddr_in);
	int32_t enabled = 1;
	int fd = zsock_socket(address->ss_family, NET_SOCK_DGRAM, NET_IPPROTO_UDP);

	if (fd < 0) {
		return -1;
	}
	if ((ipv6 && zsock_setsockopt(fd, NET_IPPROTO_IPV6, ZSOCK_IPV6_V6ONLY, &enabled,
				      sizeof(enabled)) < 0) ||
	    zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_BINDTODEVICE, &udp_binding,
			     sizeof(udp_binding)) < 0 ||
	    zsock_bind(fd, net_sad(address), len) < 0) {
		LOG_DBG("UDP bind failed (%d)", errno);
		zsock_close(fd);
		return -1;
	}
	return fd;
}

static bool refresh_socket(int *fd, const struct net_sockaddr_storage *bound,
			   struct net_sockaddr_storage *address)
{
	const bool was_open = *fd >= 0;

	if ((!was_open && address->ss_family == NET_AF_UNSPEC) ||
	    (was_open && same_endpoint(bound, address))) {
		return false;
	}
	if (was_open) {
		zsock_close(*fd);
	}
	*fd = address->ss_family == NET_AF_UNSPEC ? -1 : bind_datagram(address);
	if (*fd < 0) {
		*address = (struct net_sockaddr_storage){0};
	}
	return was_open || *fd >= 0;
}

static void pick_ipv6(struct net_if *iface, struct net_if_addr *address, void *user_data)
{
	struct net_sockaddr_in6 *candidate = user_data;
	const struct net_in6_addr *addr = &address->address.in6_addr;
	const bool global = (addr->s6_addr[0] & 0xe0U) == 0x20U;

	ARG_UNUSED(iface);
	if (address->addr_state == NET_ADDR_PREFERRED &&
	    (global || net_ipv6_is_ula_addr(addr)) &&
	    (candidate->sin6_family == NET_AF_UNSPEC ||
	     (global && net_ipv6_is_ula_addr(&candidate->sin6_addr)))) {
		candidate->sin6_family = NET_AF_INET6;
		candidate->sin6_port = initial_ipv4.sin_port;
		candidate->sin6_addr = *addr;
	}
}

static void refresh_direct(void)
{
	struct net_sockaddr_storage ipv4 = {0};
	struct net_sockaddr_storage ipv6 = {0};
	const bool proxied = IS_ENABLED(CONFIG_TAILSCALE_REQUIRED_PROXY);
	bool changed;

	if (net_if_is_up(underlay)) {
		bool use_ipv4 = true;

#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
		use_ipv4 = CONFIG_TAILSCALE_SOCKS_UDP_PORT != 0;
#endif
		net_if_lock(underlay);
		if (use_ipv4) {
			const struct net_in_addr *addr =
				net_if_ipv4_get_global_addr(underlay, NET_ADDR_PREFERRED);

			if (addr == NULL && net_ipv4_is_addr_loopback(&initial_ipv4.sin_addr) &&
			    net_if_ipv4_addr_lookup_by_iface(underlay, &initial_ipv4.sin_addr) !=
				    NULL) {
				addr = &initial_ipv4.sin_addr;
			}
			if (addr != NULL) {
				*net_sin(net_sad(&ipv4)) = (struct net_sockaddr_in){
					.sin_family = NET_AF_INET,
					.sin_port = initial_ipv4.sin_port,
					.sin_addr = *addr,
				};
			}
		}
		if (!proxied) {
			net_if_ipv6_addr_foreach(underlay, pick_ipv6, net_sin6(net_sad(&ipv6)));
		}
		net_if_unlock(underlay);
	}
	changed = refresh_socket(&udp_fd, &local, &ipv4);
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
	static atomic_val_t last_proxy_generation;
	atomic_val_t generation = atomic_get(&proxy_generation);

	if (changed) {
		atomic_inc(&udp_generation);
	}
	changed |= generation != last_proxy_generation;
	last_proxy_generation = generation;
#endif
	changed |= refresh_socket(&udp6_fd, &local6, &ipv6);
	if (!changed) {
		return;
	}

	/* A discovered mapping and authenticated path belong to the previous sockets. */
	k_mutex_lock(&plan_lock, K_FOREVER);
	local = ipv4;
	local6 = ipv6;
	mapped_until = 0;
	k_mutex_unlock(&plan_lock);
#ifdef CONFIG_TAILSCALE_DISCOVER_ENDPOINT
	stun_next = 0;
	stun_deadline = 0;
#endif
	for (size_t i = 0; i < current.count; i++) {
		paths[i].direct_until = 0;
		paths[i].challenge_until = 0;
		paths[i].last_probe = 0;
		paths[i].next_candidate = 0;
		paths[i].next_announce = 0;
		paths[i].verified = (struct net_sockaddr_storage){0};
	}
}

#ifdef CONFIG_TAILSCALE_DISCOVER_ENDPOINT
#define STUN_COOKIE 0x2112a442U

static void discover_endpoint(void)
{
	int64_t now = k_uptime_get();
	uint8_t request[40] = {0};
	struct net_sockaddr_in server = {.sin_family = NET_AF_INET};

	if (now < stun_next || current.map.count == 0U) {
		return;
	}
	stun_next = now + 3000;
	for (size_t n = 0; n < current.map.count; ++n) {
		const struct tailscale_derp_node *node =
			&current.map.nodes[stun_node++ % current.map.count];

		if (node->no_home || node->stun_port == 0U ||
		    net_addr_pton(NET_AF_INET, node->ipv4, &server.sin_addr) != 0) {
			continue;
		}
		server.sin_port = net_htons(node->stun_port);
		if (sys_csrand_get(stun_id, sizeof(stun_id)) != 0) {
			return;
		}
		sys_put_be16(1, request);
		sys_put_be16(sizeof(request) - 20, request + 2);
		sys_put_be32(STUN_COOKIE, request + 4);
		memcpy(request + 8, stun_id, sizeof(stun_id));
		sys_put_be16(0x8022, request + 20);
		sys_put_be16(8, request + 22);
		memcpy(request + 24, "tailnode", 8);
		sys_put_be16(0x8028, request + 32);
		sys_put_be16(4, request + 34);
		sys_put_be32(crc32_ieee(request, 32) ^ 0x5354554eU, request + 36);
		memcpy(&stun_server, &server, sizeof(server));
		stun_deadline =
			write_direct(&stun_server, request, sizeof(request)) == 0 ? now + 3000 : 0;
		if (stun_deadline != 0) {
			atomic_inc(&binding_requests);
		}
		return;
	}
}

static bool receive_stun(const struct net_sockaddr_storage *source, const uint8_t *data, size_t len)
{
	struct net_sockaddr_in result = {.sin_family = NET_AF_INET};
	int64_t now = k_uptime_get();

	if (len < 20 || sys_get_be32(data + 4) != STUN_COOKIE) {
		return false;
	}
	if (now >= stun_deadline || !same_endpoint(source, &stun_server) ||
	    sys_get_be16(data) != 0x0101 || sys_get_be16(data + 2) != len - 20 ||
	    memcmp(data + 8, stun_id, sizeof(stun_id)) != 0) {
		return true;
	}
	for (size_t pos = 20; pos < len;) {
		if (len - pos < 4) {
			return true;
		}
		size_t size = sys_get_be16(data + pos + 2);
		size_t extent = 4 + ROUND_UP(size, 4);
		uint16_t type = sys_get_be16(data + pos);

		if (extent > len - pos) {
			return true;
		}
		if (type == 0x0020 && size == 8 && data[pos + 5] == 1) {
			if (result.sin_port != 0) {
				return true;
			}
			result.sin_port = net_htons(sys_get_be16(data + pos + 6) ^ 0x2112);
			sys_put_be32(sys_get_be32(data + pos + 8) ^ STUN_COOKIE,
				     (uint8_t *)&result.sin_addr);
		} else if (type == 0x8028 && (size != 4 || pos + extent != len ||
					      sys_get_be32(data + pos + 4) !=
						      (crc32_ieee(data, pos) ^ 0x5354554eU))) {
			return true;
		}
		pos += extent;
	}
	if (result.sin_port && !net_ipv4_is_addr_unspecified(&result.sin_addr) &&
	    !net_ipv4_is_addr_loopback(&result.sin_addr) &&
	    !net_ipv4_is_addr_mcast(&result.sin_addr)) {
		k_mutex_lock(&plan_lock, K_FOREVER);
		if (current.epoch == desired.epoch) {
			memcpy(&mapped, &result, sizeof(result));
			mapped_until = now + 45000;
			atomic_inc(&binding_replies);
		}
		k_mutex_unlock(&plan_lock);
		stun_next = now + 20000;
		stun_deadline = 0;
	}
	return true;
}
#endif

#ifdef CONFIG_TAILSCALE_DERP
static struct channel *connected_relay(uint16_t region)
{
	for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
		if (channels[i].region == region && channels[i].conn.io.fd >= 0) {
			return &channels[i];
		}
	}
	return NULL;
}
#endif

static bool relay_available(uint16_t region)
{
#ifdef CONFIG_TAILSCALE_DERP
	if (!region || current.epoch != (uint32_t)atomic_get(&relay_ready_epoch)) {
		return false;
	}
	for (size_t i = 0; i < ARRAY_SIZE(connected_regions); i++) {
		if (region == atomic_get(&connected_regions[i])) {
			return true;
		}
	}
#else
	ARG_UNUSED(region);
#endif
	return false;
}

static int queue_relay(uint16_t region, const uint8_t *key, const uint8_t *data, size_t len,
		       bool report_sent, bool report_drop)
{
#ifdef CONFIG_TAILSCALE_DERP
	struct relay_frame *frame;

	if (!relay_available(region) && !has_region(&current, region, false)) {
		return -ENETDOWN;
	}
	if (k_mem_slab_alloc(&relay_tx_frames, (void **)&frame, K_NO_WAIT) != 0) {
		return -ENOBUFS;
	}
	frame->epoch = current.epoch;
	frame->region = region;
	frame->len = len + 32;
	frame->report_sent = report_sent;
	frame->report_drop = report_drop;
	memcpy(frame->data, key, 32);
	memcpy(frame->data + 32, data, len);
	k_fifo_put(&relay_outgoing, frame);
	return 0;
#else
	ARG_UNUSED(region);
	ARG_UNUSED(key);
	ARG_UNUSED(data);
	ARG_UNUSED(len);
	ARG_UNUSED(report_sent);
	ARG_UNUSED(report_drop);
	return -ENETDOWN;
#endif
}

#ifdef CONFIG_TAILSCALE_DERP
static void write_relay(const struct relay_frame *frame)
{
	struct channel *ch = connected_relay(frame->region);
	int rc = -ENETDOWN;

	if (ch != NULL) {
		rc = derp_conn_write(&ch->conn, DERP_SEND, frame->data, frame->len);
		if (rc != 0 && rc != -EAGAIN) {
			disconnect(ch);
		}
	}
	if (rc == 0) {
		if (frame->report_sent) {
			atomic_inc(&relay_tx);
		}
	} else if (frame->report_drop) {
		atomic_inc(&drops);
	}
}
#endif

static void transmit(const struct tailscale_datagram_frame *f)
{
	if (f->epoch != current.epoch || f->epoch != (uint32_t)atomic_get(&epoch)) {
		return;
	}
	for (size_t i = 0; i < current.count; i++) {
		const struct tailscale_transport_peer *p = &current.peers[i];

		if (p->id != f->id) {
			continue;
		}
		bool direct = IS_ENABLED(CONFIG_TAILSCALE_DISCO) &&
			      (!f->has_reply || f->reply.kind == WIREGUARD_PATH_DIRECT);
		bool relay = IS_ENABLED(CONFIG_TAILSCALE_DERP) &&
			     (f->has_reply ? f->reply.kind == WIREGUARD_PATH_RELAY
					   : paths[i].direct_until <= k_uptime_get());
		bool sent = false;

		/* Probe an unconfirmed UDP path at most once per second while relaying. */
		if (direct && !f->has_reply && relay && relay_available(p->region)) {
			int64_t now = k_uptime_get();

			direct = now - paths[i].last_probe >= 1000;
			if (direct) {
				paths[i].last_probe = now;
			}
		}
		const struct net_sockaddr_storage *target =
			paths[i].verified.ss_family && paths[i].direct_until > k_uptime_get()
				? &paths[i].verified
				: &p->endpoint;

		if (direct) {
			if (write_direct(f->has_reply ? &f->reply.remote : target, f->data,
					 f->len) == 0) {
				atomic_inc(&direct_tx);
				sent = true;
			} else if (!f->has_reply) {
				paths[i].direct_until = 0;
				relay = IS_ENABLED(CONFIG_TAILSCALE_DERP);
			}
		}
		if (relay && queue_relay(f->has_reply ? f->reply.relay_id : p->region, p->key,
					 f->data, f->len, true, !sent) == 0) {
			sent = true;
		}
		if (!sent) {
			atomic_inc(&drops);
		}
		return;
	}
}

static const uint8_t disco_magic[] = {0x54, 0x53, 0xf0, 0x9f, 0x92, 0xac};

static bool disco_message(const uint8_t *data, size_t len)
{
	return len >= sizeof(disco_magic) && !memcmp(data, disco_magic, sizeof(disco_magic));
}

#define DISCO_HEADER_SIZE (sizeof(disco_magic) + 32 + DISCO_BOX_NONCE_SIZE)
#define DISCO_ADDR_SIZE   18

static int send_disco(const struct tailscale_transport_peer *peer, const uint8_t key[32],
		      const uint8_t *data, size_t len,
		      const struct net_sockaddr_storage *remote, uint16_t region)
{
	uint8_t msg[DISCO_HEADER_SIZE + DISCO_BOX_TAG_SIZE + 46];
	uint8_t *nonce = msg + sizeof(disco_magic) + sizeof(disco_public);

	if (len > 46) {
		return -EMSGSIZE;
	}
	memcpy(msg, disco_magic, sizeof(disco_magic));
	memcpy(msg + sizeof(disco_magic), disco_public, sizeof(disco_public));
	int rc = sys_csrand_get(nonce, DISCO_BOX_NONCE_SIZE);

	if (rc == 0) {
		rc = disco_box_seal(key, nonce, data, len, msg + DISCO_HEADER_SIZE);
	}
	if (rc != 0) {
		return rc;
	}
	len += DISCO_HEADER_SIZE + DISCO_BOX_TAG_SIZE;
	return remote == NULL ? queue_relay(region, peer->key, msg, len, false, false)
			      : write_direct(remote, msg, len);
}

static void challenge(size_t i, const uint8_t key[32], const struct net_sockaddr_storage *target)
{
	uint8_t ping[14] = {1, 0};

	if (sys_csrand_get(ping + 2, 12) == 0 &&
	    send_disco(&current.peers[i], key, ping, sizeof(ping), target, 0) == 0) {
		memcpy(paths[i].transaction, ping + 2, 12);
		paths[i].challenge_target = *target;
		paths[i].challenge_until = k_uptime_get() + 1500;
	}
}

static void encode_address(uint8_t out[DISCO_ADDR_SIZE], const void *address)
{
	const struct net_sockaddr_in *v4 = address;
	const struct net_sockaddr_in6 *v6 = address;

	if (v6->sin6_family == NET_AF_INET6) {
		memcpy(out, &v6->sin6_addr, 16);
		memcpy(out + 16, &v6->sin6_port, 2);
		return;
	}
	memset(out, 0, 10);
	out[10] = out[11] = 0xff;
	memcpy(out + 12, &v4->sin_addr, 4);
	memcpy(out + 16, &v4->sin_port, 2);
}

static bool usable_target(const struct net_sockaddr_in *addr)
{
	return addr->sin_port && !net_ipv4_is_addr_unspecified(&addr->sin_addr) &&
	       !net_ipv4_is_addr_mcast(&addr->sin_addr) &&
	       !net_ipv4_is_addr_loopback(&addr->sin_addr) &&
	       !net_ipv4_is_addr_bcast(underlay, &addr->sin_addr);
}

static void maintain_candidates(void)
{
	struct tailscale_transport_endpoints endpoints;
	int64_t now = k_uptime_get();

	tailscale_transport_get_endpoints(&endpoints);
	for (size_t i = 0; i < current.count; i++) {
		struct tailscale_transport_peer *peer = &current.peers[i];
		const uint8_t *key = paths[i].box_key;

		if (paths[i].direct_until > now + 1000 || paths[i].next_candidate > now) {
			continue;
		}
		paths[i].next_candidate = now + 2000;
		if (!paths[i].have_box_key) {
			continue;
		}
		if (paths[i].next_announce <= now && relay_available(peer->region) &&
		    (endpoints.public_ipv4.ss_family == NET_AF_INET || udp6_fd >= 0)) {
			uint8_t message[2 + 2 * DISCO_ADDR_SIZE] = {3, 0};
			size_t len = 2;

			if (endpoints.public_ipv4.ss_family == NET_AF_INET) {
				encode_address(message + len, &endpoints.public_ipv4);
				len += DISCO_ADDR_SIZE;
			}
			if (udp6_fd >= 0) {
				encode_address(message + len, &local6);
				len += DISCO_ADDR_SIZE;
			}
			(void)send_disco(peer, key, message, len, NULL, peer->region);
			paths[i].next_announce = now + 10000;
		}
		if (paths[i].challenge_until <= now && peer->candidate_count) {
			struct net_sockaddr_storage dst = {0};
			struct net_sockaddr_in *v4 = (void *)&dst;

			for (size_t n = 0; n < peer->candidate_count; n++) {
				const struct tailscale_peer_endpoint *ep =
					&peer->candidates[paths[i].candidate_index++ %
							  peer->candidate_count];

				if (ep->family == NET_AF_INET6 &&
				    (ep->address[0] & 0xe0U) == 0x20U) {
					struct net_sockaddr_in6 *v6 = (void *)&dst;

					v6->sin6_family = NET_AF_INET6;
					v6->sin6_port = net_htons(ep->port);
					memcpy(&v6->sin6_addr, ep->address, 16);
					challenge(i, key, &dst);
					break;
				}
				if (ep->family != NET_AF_INET) {
					continue;
				}
				v4->sin_family = NET_AF_INET;
				v4->sin_port = net_htons(ep->port);
				memcpy(&v4->sin_addr, ep->address, 4);
				if (usable_target(v4)) {
					challenge(i, key, &dst);
					break;
				}
			}
		}
	}
}

static void receive_disco(size_t index, const uint8_t *msg, size_t len,
			  const struct net_sockaddr_storage *remote, uint16_t region)
{
	static uint8_t decoded[TAILSCALE_DATAGRAM_MAX];
	const struct tailscale_transport_peer *peer = &current.peers[index];
	const uint8_t *key = paths[index].box_key;
	const size_t overhead = DISCO_HEADER_SIZE + DISCO_BOX_TAG_SIZE;

	if (len < overhead + 2 || len > TAILSCALE_DATAGRAM_MAX ||
	    memcmp(msg + 6, peer->disco, 32) != 0 || !paths[index].have_box_key) {
		return;
	}
	int rc = disco_box_open(key, msg + 38, msg + DISCO_HEADER_SIZE, len - DISCO_HEADER_SIZE,
				decoded);
	len -= overhead;
	if (rc != 0 || decoded[1] != 0) {
		return;
	}
	if (decoded[0] == 2 && remote && len == 32 &&
	    paths[index].challenge_until > k_uptime_get() &&
	    same_endpoint(remote, &paths[index].challenge_target) &&
	    memcmp(decoded + 2, paths[index].transaction, 12) == 0) {
		paths[index].verified = *remote;
		paths[index].direct_until = k_uptime_get() + DIRECT_TRUST_MS;
		paths[index].challenge_until = 0;
	} else if (decoded[0] == 1 && len >= 14 &&
		   (len < 46 || !memcmp(peer->key, decoded + 14, 32))) {
		uint8_t pong[32] = {2, 0};
		struct net_sockaddr_in relay = {.sin_family = NET_AF_INET,
						.sin_port = net_htons(region)};

		memcpy(&relay.sin_addr, (uint8_t[]){127, 3, 3, 40}, 4);
		memcpy(pong + 2, decoded + 2, 12);
		encode_address(pong + 14, remote != NULL ? (const void *)remote : &relay);
		(void)send_disco(peer, key, pong, sizeof(pong), remote, region);
		if (remote &&
		    (paths[index].challenge_until <= k_uptime_get() ||
		     !same_endpoint(remote, &paths[index].challenge_target)) &&
		    (paths[index].direct_until <= k_uptime_get() + 1000 ||
		     !same_endpoint(remote, &paths[index].verified))) {
			challenge(index, key, remote);
		}
	} else if (decoded[0] == 3 && remote == NULL && len > 2 &&
		   (len - 2) % DISCO_ADDR_SIZE == 0 &&
		   paths[index].direct_until <= k_uptime_get()) {
		/* Only a control-authorized peer over DERP may request a probe.
		 * A matching response must prove the advertised UDP tuple.
		 */
		static const uint8_t mapped_prefix[12] = {[10] = 0xff, [11] = 0xff};
		struct net_sockaddr_storage dst = {0};
		struct net_sockaddr_in *v4 = (void *)&dst;

		for (size_t n = 2; n < len; n += DISCO_ADDR_SIZE) {
			if ((decoded[n] & 0xe0U) == 0x20U && sys_get_be16(decoded + n + 16) != 0) {
				struct net_sockaddr_in6 *v6 = (void *)&dst;

				v6->sin6_family = NET_AF_INET6;
				memcpy(&v6->sin6_addr, decoded + n, 16);
				memcpy(&v6->sin6_port, decoded + n + 16, 2);
				challenge(index, key, &dst);
				break;
			}
			if (memcmp(decoded + n, mapped_prefix, sizeof(mapped_prefix))) {
				continue;
			}
			v4->sin_family = NET_AF_INET;
			memcpy(&v4->sin_addr, decoded + n + 12, 4);
			memcpy(&v4->sin_port, decoded + n + 16, 2);
			if (usable_target(v4) && !net_ipv4_is_private_addr(&v4->sin_addr)) {
				challenge(index, key, &dst);
				break;
			}
		}
	}
}

#ifdef CONFIG_TAILSCALE_DERP
static size_t peer_for_key(const struct tailscale_transport_plan *plan, const uint8_t *key)
{
	for (size_t i = 0; i < plan->count; i++) {
		if (memcmp(key, plan->peers[i].key, 32) == 0) {
			return i;
		}
	}
	return plan->count;
}

static void receive_relay(struct channel *ch)
{
	struct relay_frame *frame;
	uint8_t type;
	size_t len;
	int rc = derp_conn_read(&ch->conn, &type, relay_buf, &len);

	if (rc != 0) {
		disconnect(ch);
		return;
	}
	switch (type) {
	case DERP_RECV:
		if (len <= 32 || relay_plan.epoch != (uint32_t)atomic_get(&epoch)) {
			break;
		}
		if (peer_for_key(&relay_plan, relay_buf) == relay_plan.count) {
			atomic_inc(&unknown);
			atomic_inc(&denied);
			break;
		}
		if (k_mem_slab_alloc(&relay_rx_frames, (void **)&frame, K_NO_WAIT) != 0) {
			atomic_inc(&relay_rx_drops);
			break;
		}
		frame->epoch = relay_plan.epoch;
		frame->region = ch->region;
		frame->len = len;
		memcpy(frame->data, relay_buf, len);
		k_fifo_put(&relay_incoming, frame);
		break;
	case DERP_PING:
		if (len != 8 || derp_conn_write(&ch->conn, DERP_PONG, relay_buf, len) != 0) {
			disconnect(ch);
		}
		break;
	case DERP_KEEP_ALIVE:
		if (len != 0) {
			disconnect(ch);
		}
		break;
	case DERP_HEALTH:
		if (len == 0) {
			break;
		}
		__fallthrough;
	case DERP_RESTARTING:
		disconnect(ch);
		break;
	default:
		break;
	}
}

static void deliver_relay(const struct relay_frame *frame)
{
	struct wireguard_datagram_path path = {.kind = WIREGUARD_PATH_RELAY,
					       .relay_id = frame->region};
	const uint8_t *data = frame->data + 32;
	size_t len = frame->len - 32U;

	if (frame->epoch != current.epoch || frame->epoch != (uint32_t)atomic_get(&epoch)) {
		return;
	}
	size_t i = peer_for_key(&current, frame->data);

	if (i == current.count) {
		atomic_inc(&unknown);
		atomic_inc(&denied);
		return;
	}
	if (disco_message(data, len)) {
		if (IS_ENABLED(CONFIG_TAILSCALE_DISCO)) {
			receive_disco(i, data, len, NULL, frame->region);
		}
	} else if (wireguard_peer_receive_datagram(current.peers[i].id, &path, data, len) == 0) {
		atomic_inc(&relay_rx);
	} else {
		atomic_inc(&denied);
	}
}
#endif

static void receive_direct(int fd)
{
	struct wireguard_datagram_path path = {.kind = WIREGUARD_PATH_DIRECT,
					       .iface = underlay,
					       .local = fd == udp6_fd ? local6 : local};
	net_socklen_t size = sizeof(path.remote);
	ssize_t n = zsock_recvfrom(fd, buf, sizeof(buf), ZSOCK_MSG_DONTWAIT, (void *)&path.remote,
				   &size);

#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
	struct net_sockaddr_storage relay = {0};

	if (!proxy_endpoint(net_sin(net_sad(&relay))) || n < 10 ||
	    !same_endpoint(&path.remote, &relay) ||
	    buf[0] != 0 || buf[1] != 0 || buf[2] != 0) {
		return;
	}
	size_t header_len;

	if (buf[3] == 1) {
		struct net_sockaddr_in *src = (void *)&path.remote;

		memcpy(&src->sin_addr, buf + 4, 4);
		memcpy(&src->sin_port, buf + 8, 2);
		header_len = 10;
	} else if (buf[3] == 4 && n >= 22) {
		struct net_sockaddr_in6 *src = (void *)&path.remote;

		memset(&path.remote, 0, sizeof(path.remote));
		src->sin6_family = NET_AF_INET6;
		memcpy(&src->sin6_addr, buf + 4, 16);
		memcpy(&src->sin6_port, buf + 20, 2);
		/* SOCKS supplies the remote address. Its IPv6 local address is unknown. */
		path.local = (struct net_sockaddr_storage){0};
		*net_sin6(net_sad(&path.local)) = (struct net_sockaddr_in6){
			.sin6_family = NET_AF_INET6, .sin6_port = initial_ipv4.sin_port,
		};
		header_len = 22;
	} else {
		return;
	}
	n -= header_len;
	memmove(buf, buf + header_len, n);
#endif
	if (n < 4 || n > TAILSCALE_DATAGRAM_MAX || current.epoch != (uint32_t)atomic_get(&epoch)) {
		return;
	}
#ifdef CONFIG_TAILSCALE_DISCOVER_ENDPOINT
	if (receive_stun(&path.remote, buf, n)) {
		return;
	}
#endif
	for (size_t i = 0; i < current.count; i++) {
		if (disco_message(buf, n)) {
			if (n >= 38 && !memcmp(buf + 6, current.peers[i].disco, 32)) {
				receive_disco(i, buf, n, &path.remote, 0);
				return;
			}
			continue;
		}
		if (!same_endpoint(&path.remote, &current.peers[i].endpoint) &&
		    !same_endpoint(&path.remote, &paths[i].verified)) {
			continue;
		}
		int rc = wireguard_peer_receive_datagram(current.peers[i].id, &path, buf, n);

		if (rc == 0) {
			atomic_inc(&direct_rx);
			if (sys_get_le32(buf) == 4) {
				paths[i].verified = path.remote;
				paths[i].direct_until = k_uptime_get() + DIRECT_TRUST_MS;
			}
		}
		return;
	}
}

#ifdef CONFIG_TAILSCALE_DERP
static bool relay_pending(struct control_io *io)
{
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
	if (tailscale_proxy_pending(io)) {
		return true;
	}
#endif
	return io->rx_pos < io->rx_len;
}
#endif

static void run_datagrams(void *unused1, void *unused2, void *unused3)
{
	struct tailscale_datagram_frame *frame;
	int64_t refresh_at = 0;

	while (atomic_get(&running)) {
		if (IS_ENABLED(CONFIG_TAILSCALE_DISCO) && k_uptime_get() >= refresh_at) {
			refresh_direct();
			refresh_at = k_uptime_get() + DIRECT_REFRESH_MS;
		}
		if (current.epoch != (uint32_t)atomic_get(&epoch)) {
			refresh_datagrams();
		}
		struct zsock_pollfd pfds[] = {
			{.fd = udp_fd, .events = ZSOCK_POLLIN},
			{.fd = udp6_fd, .events = ZSOCK_POLLIN},
		};

#ifdef CONFIG_TAILSCALE_DISCOVER_ENDPOINT
		discover_endpoint();
#endif
		if (IS_ENABLED(CONFIG_TAILSCALE_DISCO)) {
			maintain_candidates();
		}

		for (int i = 0; i < 6 && (frame = k_fifo_get(&outgoing, K_NO_WAIT)) != NULL; i++) {
			/* Serialize direct sends with policy publication. */
			k_mutex_lock(&plan_lock, K_FOREVER);
			transmit(frame);
			k_mutex_unlock(&plan_lock);
			k_mem_slab_free(&frames, frame);
		}
		int wait = 10;

#ifdef CONFIG_TAILSCALE_DERP
		if (!k_fifo_is_empty(&relay_incoming)) {
			wait = 0;
		}
#endif

		if (zsock_poll(pfds, ARRAY_SIZE(pfds), wait) < 0) {
			k_sleep(K_MSEC(10));
		} else {
			for (size_t i = 0; i < ARRAY_SIZE(pfds); i++) {
				if (pfds[i].revents & ZSOCK_POLLIN) {
					receive_direct(pfds[i].fd);
				}
			}
		}
#ifdef CONFIG_TAILSCALE_DERP
		struct relay_frame *incoming;

		for (int i = 0; i < RELAY_QUEUE_DEPTH &&
		     (incoming = k_fifo_get(&relay_incoming, K_NO_WAIT)) != NULL; i++) {
			deliver_relay(incoming);
			k_mem_slab_free(&relay_rx_frames, incoming);
		}
#endif
		k_yield();
	}
}

#ifdef CONFIG_TAILSCALE_DERP
static void run_relays(void *unused1, void *unused2, void *unused3)
{
	struct relay_frame *frame;

	while (atomic_get(&running)) {
		if (relay_plan.epoch != (uint32_t)atomic_get(&epoch)) {
			refresh_relays();
		}
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
		refresh_association();
#endif
		for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
			reconnect(&channels[i]);
		}
		for (int i = 0; i < RELAY_QUEUE_DEPTH &&
		     (frame = k_fifo_get(&relay_outgoing, K_NO_WAIT)) != NULL; i++) {
			if (frame->epoch == (uint32_t)atomic_get(&epoch) &&
			    frame->epoch == relay_plan.epoch && frame->region != relay_plan.home &&
			    frame->region != channels[1].region) {
				if (k_uptime_get() < secondary_until) {
					k_fifo_put(&relay_outgoing, frame);
					continue;
				}
				/* Keep home connected and serve other peer regions on demand. */
				disconnect(&channels[1]);
				channels[1].region = frame->region;
				channels[1].retry = 0;
				channels[1].next_node = 0;
				reconnect(&channels[1]);
				secondary_until = k_uptime_get() + RELAY_SECONDARY_DWELL_MS;
			}
			k_mutex_lock(&relay_send_lock, K_FOREVER);
			if (frame->epoch == relay_plan.epoch &&
			    frame->epoch == (uint32_t)atomic_get(&epoch)) {
				write_relay(frame);
			}
			k_mutex_unlock(&relay_send_lock);
			k_mem_slab_free(&relay_tx_frames, frame);
		}
		struct zsock_pollfd pfds[DERP_CONNECTIONS];
		int wait = 10;

		for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
			struct control_io *io = &channels[i].conn.io;

			pfds[i] = (struct zsock_pollfd){.fd = io->fd, .events = ZSOCK_POLLIN};
			if (io->fd >= 0 && relay_pending(io)) {
				wait = 0;
			}
		}
		if (zsock_poll(pfds, ARRAY_SIZE(pfds), wait) < 0) {
			k_sleep(K_MSEC(10));
			continue;
		}
		for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
			struct channel *ch = &channels[i];

			if (ch->conn.io.fd < 0) {
				continue;
			}
			if ((pfds[i].revents & ZSOCK_POLLIN) || relay_pending(&ch->conn.io)) {
				receive_relay(ch);
			} else if (pfds[i].revents ||
				   k_uptime_get() - ch->conn.last_rx > MAX_IDLE_MS) {
				disconnect(ch);
			}
		}
		k_yield();
	}
	for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
		disconnect(&channels[i]);
	}
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
	close_association();
#endif
}
#endif

int tailscale_transport_start(const struct tailscale_control_params *params, struct net_if *lan)
{
	int rc = 0;

	if (atomic_get(&running)) {
		return -EBUSY;
	}
	if (IS_ENABLED(CONFIG_TAILSCALE_DISCO)) {
		rc = net_if_get_name(lan, udp_binding.ifr_name, sizeof(udp_binding.ifr_name));
		if (rc <= 0) {
			return rc < 0 ? rc : -ENODEV;
		}
		initial_ipv4 = *net_sin(net_sad(&params->local_endpoint));
		rc = 0;
	}
#ifdef CONFIG_TAILSCALE_DERP
	rc = k_mem_slab_init(&relay_tx_frames, relay_tx_storage, sizeof(relay_tx_storage[0]),
			     ARRAY_SIZE(relay_tx_storage));
	if (rc == 0) {
		rc = k_mem_slab_init(&relay_rx_frames, relay_rx_storage,
				     sizeof(relay_rx_storage[0]), ARRAY_SIZE(relay_rx_storage));
	}
	if (rc != 0) {
		return rc;
	}
	rc = control_x25519_import(params->node_private, &node_key);
#endif
	if (rc == 0 && IS_ENABLED(CONFIG_TAILSCALE_DISCO)) {
		rc = control_x25519_import(params->disco_private, &disco_key);
		if (rc == 0) {
			rc = control_x25519_public(disco_key, disco_public);
		}
	}
	if (rc != 0) {
		psa_destroy_key(node_key);
		psa_destroy_key(disco_key);
		node_key = disco_key = PSA_KEY_ID_NULL;
		return rc;
	}
	underlay = lan;
	memset(paths, 0, sizeof(paths));
#ifdef CONFIG_TAILSCALE_DERP
	ca_tag = params->ca_tag;
	memset(channels, 0, sizeof(channels));
	secondary_until = 0;
	for (size_t i = 0; i < ARRAY_SIZE(channels); i++) {
		channels[i].conn.io.fd = -1;
	}
#endif
#ifdef CONFIG_TAILSCALE_DISCOVER_ENDPOINT
	stun_next = stun_deadline = 0;
#endif
	tailscale_transport_publish(NULL);
	if (IS_ENABLED(CONFIG_TAILSCALE_DISCO)) {
		refresh_direct();
	}
	atomic_set(&running, 1);
	k_thread_create(&datagram_worker, datagram_stack, K_THREAD_STACK_SIZEOF(datagram_stack),
			run_datagrams, NULL, NULL, NULL, K_PRIO_PREEMPT(7), 0, K_NO_WAIT);
	(void)k_thread_name_set(&datagram_worker, "tailnet-datagram");
#ifdef CONFIG_TAILSCALE_DERP
	k_thread_create(&relay_worker, relay_stack, K_THREAD_STACK_SIZEOF(relay_stack), run_relays,
			NULL, NULL, NULL, K_PRIO_PREEMPT(7), 0, K_NO_WAIT);
	(void)k_thread_name_set(&relay_worker, "tailnet-derp");
#endif
	return 0;
}

void tailscale_transport_stop(void)
{
	struct tailscale_datagram_frame *frame;
	k_spinlock_key_t key = k_spin_lock(&outgoing_lock);
	bool live = atomic_clear(&running);

	k_spin_unlock(&outgoing_lock, key);
	if (!live) {
		return;
	}
	k_thread_join(&datagram_worker, K_FOREVER);
	while ((frame = k_fifo_get(&outgoing, K_NO_WAIT)) != NULL) {
		k_mem_slab_free(&frames, frame);
	}
#ifdef CONFIG_TAILSCALE_DERP
	struct relay_frame *relay;

	k_thread_join(&relay_worker, K_FOREVER);
	while ((relay = k_fifo_get(&relay_outgoing, K_NO_WAIT)) != NULL) {
		k_mem_slab_free(&relay_tx_frames, relay);
	}
	while ((relay = k_fifo_get(&relay_incoming, K_NO_WAIT)) != NULL) {
		k_mem_slab_free(&relay_rx_frames, relay);
	}
#endif
	mbedtls_platform_zeroize(paths, sizeof(paths));
	atomic_clear(&home_region);
#ifdef CONFIG_TAILSCALE_REQUIRED_PROXY
	associate_after = 0;
#endif
	if (udp_fd >= 0) {
		zsock_close(udp_fd);
	}
	k_mutex_lock(&plan_lock, K_FOREVER);
	mapped_until = 0;
	local = local6 = (struct net_sockaddr_storage){0};
	k_mutex_unlock(&plan_lock);
	psa_destroy_key(node_key);
	psa_destroy_key(disco_key);
	node_key = disco_key = PSA_KEY_ID_NULL;
	udp_fd = -1;
	if (udp6_fd >= 0) {
		zsock_close(udp6_fd);
		udp6_fd = -1;
	}
}

#ifdef CONFIG_SHELL
static int status(const struct shell *sh, size_t argc, char **argv)
{
	shell_print(sh,
		    "DERP connects=%ld rx=%ld tx=%ld direct_rx=%ld direct_tx=%ld drops=%ld "
		    "denied=%ld unknown=%ld",
		    (long)atomic_get(&connects), (long)atomic_get(&relay_rx),
		    (long)atomic_get(&relay_tx), (long)atomic_get(&direct_rx),
		    (long)atomic_get(&direct_tx), (long)atomic_get(&drops),
		    (long)atomic_get(&denied), (long)atomic_get(&unknown));
	shell_print(sh, "STUN requests=%ld replies=%ld", (long)atomic_get(&binding_requests),
		    (long)atomic_get(&binding_replies));
	shell_print(sh, "DERP receive queue drops=%ld", (long)atomic_get(&relay_rx_drops));
	return 0;
}
SHELL_CMD_ARG_REGISTER(tailscale_derp_status, NULL, "Show encrypted datagram path counts", status,
		       1, 0);
#endif
