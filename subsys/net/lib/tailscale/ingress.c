/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/wireguard.h>

#include "../sockets/sockets_internal.h"
#include "ingress.h"
#include "net_stack.h"

#define TCP_FIN BIT(0)
#define TCP_SYN BIT(1)
#define TCP_RST BIT(2)
#define TCP_ACK BIT(4)

static struct k_spinlock admission_lock;
static struct tailscale_service_rule service_rules[TAILSCALE_SERVICE_MAX_RULES];
static size_t rule_count;
enum flow_state {
	FLOW_FREE,
	FLOW_TCP_CONNECTING,
	FLOW_CONNECTED,
	FLOW_TCP_CLOSING,
};

struct return_flow {
	uint8_t local[sizeof(struct net_in6_addr)];
	uint8_t remote[sizeof(struct net_in6_addr)];
	uint16_t local_port;
	uint16_t remote_port;
	uint8_t family;
	uint8_t protocol;
	uint64_t revision;
	uint64_t peer_identity;
	int64_t udp_deadline;
	enum flow_state state;
};
static uint64_t active_revision;
static int64_t active_deadline;

/* A pre-queue record binds the packet to forwarding policy and the native socket
 * that submitted it. A closed or recycled UDP socket cannot inherit it.
 */
static struct {
	const struct net_pkt *pkt;
	const struct net_context *ctx;
	uint64_t revision;
} queued[CONFIG_NET_PKT_TX_COUNT];

static struct native_flow {
	struct return_flow flow;
	const struct net_context *ctx;
	int64_t deadline;
} native_flows[TAILSCALE_SOCKET_MAX_FLOWS];

static bool valid_protocol(uint8_t protocol)
{
	return protocol == NET_IPPROTO_TCP || protocol == NET_IPPROTO_UDP;
}

int tailscale_ingress_validate(const struct tailscale_service_rule *rules, size_t count)
{
	if ((count != 0U && rules == NULL) || count > ARRAY_SIZE(service_rules)) {
		return -EINVAL;
	}
	for (size_t i = 0; i < count; i++) {
		if (rules[i].port == 0U || !valid_protocol(rules[i].protocol)) {
			return -EINVAL;
		}
		for (size_t j = 0; j < i; j++) {
			if (rules[i].port == rules[j].port &&
			    rules[i].protocol == rules[j].protocol) {
				return -EALREADY;
			}
		}
	}

	return 0;
}

int tailscale_ingress_replace(const struct tailscale_service_rule *rules, size_t count)
{
	int rc = tailscale_ingress_validate(rules, count);

	if (rc != 0) {
		return rc;
	}

	k_spinlock_key_t key = k_spin_lock(&admission_lock);

	if (count != 0U) {
		memcpy(service_rules, rules, count * sizeof(rules[0]));
	}
	rule_count = count;
	k_spin_unlock(&admission_lock, key);
	return 0;
}

size_t tailscale_ingress_rules(struct tailscale_service_rule rules[TAILSCALE_SERVICE_MAX_RULES])
{
	k_spinlock_key_t key = k_spin_lock(&admission_lock);
	size_t count = rule_count;

	memcpy(rules, service_rules, count * sizeof(rules[0]));
	k_spin_unlock(&admission_lock, key);
	return count;
}

static size_t address_size(uint8_t family)
{
	return family == NET_AF_INET ? sizeof(struct net_in_addr) : sizeof(struct net_in6_addr);
}

static const void *endpoint_address(const struct net_sockaddr *address)
{
	if (address->sa_family == NET_AF_INET) {
		return &net_sin(address)->sin_addr;
	}
	return &net_sin6(address)->sin6_addr;
}

static uint16_t endpoint_port(const struct net_sockaddr *address)
{
	return net_ntohs(address->sa_family == NET_AF_INET ? net_sin(address)->sin_port :
							   net_sin6(address)->sin6_port);
}

static bool revision_live(uint64_t revision, int64_t now)
{
	return revision != 0U && revision == active_revision && now < active_deadline;
}

static bool return_matches(const struct return_flow *flow,
			   const struct tailscale_filter_tuple *tuple, uint64_t revision,
			   uint64_t peer_identity, int64_t now)
{
	size_t size = address_size(tuple->family);
	uint8_t flags = tuple->tcp_flags & (TCP_SYN | TCP_FIN | TCP_RST | TCP_ACK);

	if ((flow->state != FLOW_TCP_CONNECTING && flow->state != FLOW_CONNECTED &&
	     flow->state != FLOW_TCP_CLOSING) || flow->peer_identity != peer_identity ||
	    flow->revision != revision || flow->family != tuple->family ||
	    flow->protocol != tuple->protocol || flow->local_port != tuple->destination_port ||
	    flow->remote_port != tuple->source_port ||
	    memcmp(flow->local, tuple->destination, size) != 0 ||
	    memcmp(flow->remote, tuple->source, size) != 0) {
		return false;
	}
	if (flow->protocol == NET_IPPROTO_UDP) {
		return now < flow->udp_deadline;
	}
	if ((flags & TCP_SYN) != 0U) {
		return flow->state != FLOW_TCP_CLOSING && flags == (TCP_SYN | TCP_ACK);
	}
	if ((flags & TCP_RST) != 0U) {
		return (flags & TCP_FIN) == 0U;
	}
	return flow->state != FLOW_TCP_CONNECTING && (flags & TCP_ACK) != 0U;
}

static bool service_allowed(uint8_t protocol, uint16_t port)
{
	for (size_t i = 0; i < rule_count; ++i) {
		if (port == service_rules[i].port && protocol == service_rules[i].protocol) {
			return true;
		}
	}
	return false;
}

bool tailscale_ingress_admit(const struct tailscale_filter_tuple *tuple, bool tailscale_allowed,
			   uint64_t revision, uint64_t peer_identity)
{
	int64_t now = k_uptime_get();
	k_spinlock_key_t key = k_spin_lock(&admission_lock);
	bool allowed = false;

	if (!allowed && peer_identity != 0U && revision_live(revision, now)) {
		for (size_t i = 0; i < ARRAY_SIZE(native_flows); ++i) {
			struct native_flow *entry = &native_flows[i];
			struct return_flow *flow = &entry->flow;

			if (now >= entry->deadline ||
			    !return_matches(flow, tuple, revision, peer_identity, now)) {
				continue;
			}
			allowed = true;
			if (flow->protocol == NET_IPPROTO_TCP && (tuple->tcp_flags & TCP_RST)) {
				flow->state = FLOW_FREE;
			}
			break;
		}
	}
	if (!allowed && tailscale_allowed && revision_live(revision, now)) {
		allowed = service_allowed(tuple->protocol, tuple->destination_port);
	}
	k_spin_unlock(&admission_lock, key);
	return allowed;
}

void tailscale_ingress_queue(const struct net_pkt *pkt)
{
	k_spinlock_key_t key = k_spin_lock(&admission_lock);
	size_t slot = ARRAY_SIZE(queued);

	for (size_t i = 0; i < ARRAY_SIZE(queued); ++i) {
		if (queued[i].pkt == pkt) {
			slot = i;
			break;
		}
		if (queued[i].pkt == NULL && slot == ARRAY_SIZE(queued)) {
			slot = i;
		}
	}
	if (slot < ARRAY_SIZE(queued) && revision_live(active_revision, k_uptime_get())) {
		queued[slot].pkt = pkt;
		queued[slot].ctx = net_pkt_context((struct net_pkt *)pkt);
		queued[slot].revision = active_revision;
	}
	k_spin_unlock(&admission_lock, key);
}

void tailscale_net_unqueue(const struct net_pkt *pkt)
{
	k_spinlock_key_t key = k_spin_lock(&admission_lock);

	for (size_t i = 0; i < ARRAY_SIZE(queued); ++i) {
		if (queued[i].pkt == pkt) {
			queued[i].pkt = NULL;
			queued[i].ctx = NULL;
			break;
		}
	}
	k_spin_unlock(&admission_lock, key);
}

void zsock_native_pre_close(struct net_context *ctx)
{
	int64_t deadline = k_uptime_get() + TAILSCALE_SOCKET_UDP_REPLY_MS;
	bool tcp = net_context_get_proto(ctx) == NET_IPPROTO_TCP;
	net_sa_family_t family = net_context_get_family(ctx);
	k_spinlock_key_t key = k_spin_lock(&admission_lock);

	for (size_t i = 0; i < ARRAY_SIZE(queued); ++i) {
		if (queued[i].ctx == ctx) {
			queued[i].pkt = NULL;
			queued[i].ctx = NULL;
		}
	}
	for (size_t i = 0; i < ARRAY_SIZE(native_flows); ++i) {
		struct native_flow *entry = &native_flows[i];
		struct return_flow *flow = &entry->flow;

		if (entry->ctx == ctx) {
			flow->state = FLOW_FREE;
			entry->ctx = NULL;
		} else if (tcp && flow->protocol == NET_IPPROTO_TCP && flow->family == family &&
			   flow->local_port == endpoint_port(&ctx->local) &&
			   flow->remote_port == endpoint_port(&ctx->remote) &&
			   memcmp(flow->remote, endpoint_address(&ctx->remote),
				  address_size(family)) == 0) {
			/* Keep the grant while the stack finishes the close handshake. */
			flow->state = FLOW_TCP_CLOSING;
			entry->deadline = MIN(entry->deadline, deadline);
		}
	}
	k_spin_unlock(&admission_lock, key);
}

bool tailscale_ingress_transmit(const struct net_pkt *pkt,
				const struct tailscale_filter_tuple *tuple,
				bool service_reply, uint64_t revision, uint64_t identity)
{
	const struct net_context *ctx = NULL;
	struct native_flow *match = NULL;
	struct native_flow *free_entry = NULL;
	size_t width = address_size(tuple->family);
	bool udp = tuple->protocol == NET_IPPROTO_UDP;
	uint8_t flags = tuple->tcp_flags & (TCP_SYN | TCP_FIN | TCP_RST | TCP_ACK);
	int64_t now = k_uptime_get();
	bool staged = false;
	bool allowed = false;
	k_spinlock_key_t key = k_spin_lock(&admission_lock);

	for (size_t i = 0; i < ARRAY_SIZE(queued); ++i) {
		if (queued[i].pkt == pkt) {
			staged = queued[i].revision == revision;
			ctx = queued[i].ctx;
			queued[i].pkt = NULL;
			queued[i].ctx = NULL;
			break;
		}
	}
	if (!staged || !revision_live(revision, now) || identity == 0U) {
		goto out;
	}
	if (service_reply && service_allowed(tuple->protocol, tuple->source_port)) {
		allowed = true;
		goto out;
	}
	if (udp && ctx == NULL) {
		goto out;
	}
	for (size_t i = 0; i < ARRAY_SIZE(native_flows); ++i) {
		struct native_flow *entry = &native_flows[i];
		struct return_flow *flow = &entry->flow;

		if (flow->state == FLOW_FREE || flow->revision != revision ||
		    now >= entry->deadline) {
			if (free_entry == NULL) {
				free_entry = entry;
			}
			continue;
		}
		if (flow->peer_identity == identity && flow->family == tuple->family &&
		    flow->protocol == tuple->protocol && flow->local_port == tuple->source_port &&
		    flow->remote_port == tuple->destination_port &&
		    memcmp(flow->local, tuple->source, width) == 0 &&
		    memcmp(flow->remote, tuple->destination, width) == 0) {
			match = entry;
			break;
		}
	}
	if (match == NULL) {
		if (free_entry == NULL || (!udp && flags != TCP_SYN)) {
			goto out;
		}
		match = free_entry;
		*match = (struct native_flow){
			.ctx = ctx,
			.flow = {
				.local_port = tuple->source_port,
				.remote_port = tuple->destination_port,
				.family = tuple->family,
				.protocol = tuple->protocol,
				.revision = revision,
				.peer_identity = identity,
				.state = udp ? FLOW_CONNECTED : FLOW_TCP_CONNECTING,
			},
		};
		memcpy(match->flow.local, tuple->source, width);
		memcpy(match->flow.remote, tuple->destination, width);
	}
	if (udp) {
		if (match->ctx != ctx) {
			goto out;
		}
		match->deadline = now + TAILSCALE_SOCKET_UDP_REPLY_MS;
		match->flow.udp_deadline = match->deadline;
	} else if ((flags & TCP_RST) != 0U) {
		match->flow.state = FLOW_FREE;
	} else if ((flags & TCP_FIN) != 0U) {
		match->flow.state = FLOW_TCP_CLOSING;
		match->deadline = now + TAILSCALE_SOCKET_UDP_REPLY_MS;
	} else if (flags == TCP_SYN) {
		match->deadline = now + TAILSCALE_SOCKET_UDP_REPLY_MS;
	} else if ((flags & TCP_ACK) != 0U && match->flow.state != FLOW_TCP_CLOSING) {
		match->flow.state = FLOW_CONNECTED;
		match->deadline = INT64_MAX;
	}
	allowed = true;
out:
	k_spin_unlock(&admission_lock, key);
	return allowed;
}

void tailscale_ingress_set_revision(uint64_t revision, int64_t deadline_ms)
{
	k_spinlock_key_t key = k_spin_lock(&admission_lock);

	if (revision != active_revision) {
		memset(queued, 0, sizeof(queued));
	}
	active_revision = revision;
	active_deadline = deadline_ms;
	k_spin_unlock(&admission_lock, key);
}

int tailscale_ingress_bind(struct net_if *iface, int fd, const struct net_sockaddr *local)
{
	struct net_ifreq name = {0};
	bool ipv6 = local->sa_family == NET_AF_INET6;
	net_socklen_t len = ipv6 ? sizeof(struct net_sockaddr_in6) :
				  sizeof(struct net_sockaddr_in);
	int rc = net_if_get_name(iface, name.ifr_name, sizeof(name.ifr_name));
	int v6only = 1;

	if (rc < 0) {
		return rc;
	}
	if (zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_BINDTODEVICE,
			     &name, sizeof(name)) < 0 ||
	    (ipv6 && zsock_setsockopt(fd, NET_IPPROTO_IPV6, ZSOCK_IPV6_V6ONLY,
				     &v6only, sizeof(v6only)) < 0) ||
	    zsock_bind(fd, local, len) < 0) {
		return -errno;
	}
	return 0;
}

void tailscale_ingress_abort_tcp(int fd)
{
	struct net_linger linger = {.l_onoff = 1, .l_linger = 0};

	(void)zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_LINGER, &linger, sizeof(linger));
	zsock_close(fd);
}
