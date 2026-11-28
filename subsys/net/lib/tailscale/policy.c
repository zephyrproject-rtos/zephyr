/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <mbedtls/platform_util.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/virtual_mgmt.h>
#include "../wireguard/wg_private.h"
#include <zephyr/sys/base64.h>

#include "policy.h"
#include "net_stack.h"
#ifdef CONFIG_TAILSCALE_ROUTE_POLICY
#include "routes.h"
#endif
#ifdef CONFIG_TAILSCALE_TRANSPORT
#include "transport.h"
#endif
#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
#include "ingress.h"
#endif

#if defined(CONFIG_SOC_FAMILY_ESPRESSIF_SPIRAM)
#define POLICY_STORAGE __attribute__((section(".ext_ram.bss")))
#else
#define POLICY_STORAGE
#endif

#define NODE_KEY_SIZE TAILSCALE_PEER_PUBLIC_KEY_SIZE
#define BASE64_KEY_SIZE (4 * ((NODE_KEY_SIZE + 2) / 3) + 1)
#define POLICY_LEASE_MS (CONFIG_TAILSCALE_POLICY_LEASE_SECONDS * 1000U)
#define WITHDRAW_RETRY_MS 250

LOG_MODULE_REGISTER(tailscale_client_policy, LOG_LEVEL_INF);

struct self_addresses {
	struct net_in_addr ipv4;
	struct net_in6_addr ipv6;
	bool have_ipv4;
	bool have_ipv6;
};

struct policy_input {
	const struct tailscale_peer_snapshot *peers;
	const struct tailscale_packet_filter *filter;
	struct self_addresses self;
	int64_t self_deadline;
	int64_t next_peer_deadline;
#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
	uint64_t revision;
	uint8_t routed[TAILSCALE_PEER_INVENTORY_CAPACITY];
	bool exit;
	bool allow_lan;
#endif
};

#ifdef CONFIG_TAILSCALE_TRANSPORT
static POLICY_STORAGE struct tailscale_transport_plan transport_plan;
#endif
static POLICY_STORAGE struct tailscale_policy policy;
static POLICY_STORAGE struct wireguard_policy_peer candidate[CONFIG_WIREGUARD_MAX_PEER];
static POLICY_STORAGE unsigned char keys[CONFIG_WIREGUARD_MAX_PEER][BASE64_KEY_SIZE];
static struct policy_input inputs[2];
static struct net_if *physical;
#ifdef CONFIG_TAILSCALE_ROUTE_POLICY
static struct tailscale_routes preferences;
static char pinned_exit[TAILSCALE_PEER_STABLE_ID_SIZE];
static uint64_t exit_owner;
#endif
static K_MUTEX_DEFINE(policy_lock);
#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
static POLICY_STORAGE struct {
	struct net_if *overlay;
	struct net_if *underlay;
	struct tailscale_peer_prefix prefixes[CONFIG_WIREGUARD_MAX_PEER * WIREGUARD_MAX_SRC_IPS];
	size_t prefix_count;
	bool exit;
	bool allow_lan;
} routing;
#endif
static struct k_spinlock summary_lock;
static struct tailscale_peer_inventory_summary installed_summary;
static int64_t installed_deadline;
static void expire_policy(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(policy_expiry, expire_policy);
static struct {
	struct net_if *iface;
	struct self_addresses self;
	uint8_t expected_key[NODE_KEY_SIZE];
	int64_t lease_deadline;
	size_t peer_count;
	uint32_t last_generation;
	uint16_t previous_mtu;
	unsigned int input;
	bool stopping;
	bool have_address;
	bool have_generation;
	bool require_new_generation;
	bool withdraw_pending;
	bool initialized;
} active;

static bool lease_valid(void)
{
	int64_t now = k_uptime_get();
	k_spinlock_key_t key = k_spin_lock(&summary_lock);
	bool valid = now < installed_deadline;

	k_spin_unlock(&summary_lock, key);
	return valid;
}

#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
static void publish_routing(struct net_if *iface)
{
	k_spinlock_key_t key = k_spin_lock(&summary_lock);

	routing.overlay = iface;
	routing.underlay = physical;
	routing.exit = preferences.exit_node[0] != '\0';
	routing.allow_lan = preferences.allow_lan;
	if (iface == NULL) {
		routing.prefix_count = 0;
	}
	k_spin_unlock(&summary_lock, key);
}

static void publish_prefixes(size_t count)
{
	k_spinlock_key_t key = k_spin_lock(&summary_lock);

	routing.prefix_count = 0;
	for (size_t i = 0; i < count; ++i) {
		const struct wireguard_peer_config *config = &candidate[i].config;

		for (size_t j = 0; j < ARRAY_SIZE(config->allowed_ip); ++j) {
			const struct wireguard_allowed_ip *prefix = &config->allowed_ip[j];

			if (!prefix->is_valid || prefix->mask_len == 0U) {
				continue;
			}
			struct tailscale_peer_prefix *entry =
				&routing.prefixes[routing.prefix_count++];

			entry->family = prefix->addr.family;
			entry->length = prefix->mask_len;
			if (entry->family == NET_AF_INET) {
				memcpy(entry->address, &prefix->addr.in_addr,
				       sizeof(prefix->addr.in_addr));
			} else {
				memcpy(entry->address, &prefix->addr.in6_addr,
				       sizeof(prefix->addr.in6_addr));
			}
		}
	}
	k_spin_unlock(&summary_lock, key);
}

bool tailscale_net_active(void)
{
	k_spinlock_key_t key = k_spin_lock(&summary_lock);
	bool managed = routing.overlay != NULL;

	k_spin_unlock(&summary_lock, key);
	return managed;
}

struct net_if *tailscale_net_route(net_sa_family_t family, const void *destination)
{
	static const struct net_in6_addr tailnet_v6 = {
		.s6_addr = {0xfd, 0x7a, 0x11, 0x5c, 0xa1, 0xe0},
	};
	struct net_if *overlay;
	struct net_if *lan;
	k_spinlock_key_t key;
	bool exit_configured;
	bool allow_lan;
	bool captured;

	if (destination == NULL) {
		return NULL;
	}
	if (family == NET_AF_INET) {
		const struct net_in_addr *addr = destination;

		if (net_ipv4_is_addr_unspecified(addr) || net_ipv4_is_addr_loopback(addr) ||
		    net_ipv4_is_addr_mcast(addr) || net_ipv4_is_ll_addr(addr) ||
		    net_ipv4_addr_cmp(addr, net_ipv4_broadcast_address())) {
			return NULL;
		}
		captured = (net_ntohl(addr->s_addr) & 0xffc00000U) == 0x64400000U;
	} else {
		const struct net_in6_addr *addr = destination;

		if (net_ipv6_is_addr_unspecified(addr) || net_ipv6_is_addr_loopback(addr) ||
		    net_ipv6_is_addr_mcast(addr) || net_ipv6_is_ll_addr(addr)) {
			return NULL;
		}
		captured = net_ipv6_is_prefix(addr->s6_addr, tailnet_v6.s6_addr, 48);
	}

	key = k_spin_lock(&summary_lock);
	overlay = routing.overlay;
	lan = routing.underlay;
	exit_configured = routing.exit;
	allow_lan = routing.allow_lan;
	for (size_t i = 0; !captured && i < routing.prefix_count; ++i) {
		const struct tailscale_peer_prefix *prefix = &routing.prefixes[i];

		captured = prefix->family == family &&
			   net_ipv6_is_prefix(destination, prefix->address, prefix->length);
	}
	k_spin_unlock(&summary_lock, key);

	/* Keep matching traffic on the overlay during lease or exit outages. */
	if (overlay == NULL || captured) {
		return overlay;
	}
	if (!exit_configured) {
		return NULL;
	}
	if (allow_lan && lan != NULL &&
	    (family == NET_AF_INET ? net_if_ipv4_addr_onlink(&lan, destination) :
				     net_if_ipv6_addr_onlink(&lan, destination))) {
		return lan;
	}
	return overlay;
}

void tailscale_net_queue(struct net_if *iface, const struct net_pkt *pkt)
{
	k_spinlock_key_t key = k_spin_lock(&summary_lock);
	bool managed = routing.overlay == iface;

	k_spin_unlock(&summary_lock, key);
	if (managed) {
		tailscale_ingress_queue(pkt);
	}
}

bool tailscale_net_transmit(struct net_if *iface, const struct net_pkt *pkt, uint64_t peer,
			    const uint8_t *ip, size_t len, const void *policy_input)
{
	const struct policy_input *input = policy_input;
	struct tailscale_filter_tuple tuple;
	struct tailscale_filter_tuple reverse;
	k_spinlock_key_t key = k_spin_lock(&summary_lock);
	bool managed = routing.overlay == iface;

	k_spin_unlock(&summary_lock, key);
	if (!managed) {
		return true;
	}
	if (input == NULL || !lease_valid() || !tailscale_packet_filter_parse(ip, len, &tuple)) {
		return false;
	}
	if (tuple.family == NET_AF_INET) {
		if (!input->self.have_ipv4 ||
		    memcmp(tuple.source, &input->self.ipv4, sizeof(input->self.ipv4)) != 0) {
			return false;
		}
	} else if (!input->self.have_ipv6 ||
		   memcmp(tuple.source, &input->self.ipv6, sizeof(input->self.ipv6)) != 0) {
		return false;
	}
	reverse = tuple;
	reverse.source = tuple.destination;
	reverse.destination = tuple.source;
	reverse.source_port = tuple.destination_port;
	reverse.destination_port = tuple.source_port;

	return tailscale_ingress_transmit(pkt, &tuple,
					  tailscale_packet_filter_match(input->filter, &reverse),
					  input->revision, peer);
}

#endif

static void publish_summary(const struct tailscale_peer_inventory_summary *summary,
			    int64_t deadline)
{
	k_spinlock_key_t key = k_spin_lock(&summary_lock);

	if (summary != NULL) {
		installed_summary = *summary;
	} else {
		installed_summary.valid = false;
		installed_summary.peer_count = 0;
	}
	installed_deadline = deadline;
	k_spin_unlock(&summary_lock, key);
#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
	tailscale_ingress_set_revision(summary && summary->valid ? inputs[active.input].revision
								 : 0,
				       MIN(deadline, inputs[active.input].next_peer_deadline));
#endif
	if (summary != NULL && summary->valid) {
		net_if_dormant_off(active.iface);
	} else {
		net_if_dormant_on(active.iface);
	}
}

static bool allow_input(uint64_t identity, const uint8_t *ip, size_t len, void *context)
{
	const struct policy_input *input = context;
	bool local = false;

	ARG_UNUSED(identity);
	if (input == NULL || !lease_valid()) {
		return false;
	}
#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
	struct tailscale_filter_tuple tuple;

	if (!tailscale_packet_filter_parse(ip, len, &tuple)) {
		return false;
	}
	if (tuple.family == NET_AF_INET) {
		local = input->self.have_ipv4 &&
			memcmp(tuple.destination, &input->self.ipv4, sizeof(input->self.ipv4)) == 0;
	} else {
		local = input->self.have_ipv6 &&
			memcmp(tuple.destination, &input->self.ipv6, sizeof(input->self.ipv6)) == 0;
	}
	return local && tailscale_ingress_admit(&tuple, tailscale_packet_filter_match(input->filter,
							      &tuple), input->revision, identity);
#else
	if (len >= 20U && (ip[0] >> 4) == 4U && input->self.have_ipv4) {
		local = memcmp(ip + 16, &input->self.ipv4, sizeof(input->self.ipv4)) == 0;
	} else if (len >= 40U && (ip[0] >> 4) == 6U && input->self.have_ipv6) {
		local = memcmp(ip + 24, &input->self.ipv6, sizeof(input->self.ipv6)) == 0;
	}
	return local && tailscale_packet_filter_accept(input->filter, ip, len);
#endif
}

static bool host_prefix(const struct tailscale_peer_prefix *prefix)
{
	const uint8_t *address = prefix->address;

	if (prefix->family == NET_AF_INET) {
		return prefix->length == 32U && !net_ipv4_is_addr_loopback_raw(address) &&
		       !net_ipv4_is_addr_unspecified_raw(address) &&
		       !net_ipv4_is_addr_mcast_raw(address) &&
		       memcmp(address, (uint8_t[]){255, 255, 255, 255}, 4) != 0;
	}
	return prefix->family == NET_AF_INET6 && prefix->length == 128U &&
	       !net_ipv6_is_addr_loopback_raw(address) &&
	       !net_ipv6_is_addr_unspecified_raw(address) && !net_ipv6_is_addr_mcast_raw(address);
}

static bool self_prefix(const struct self_addresses *self,
			const struct tailscale_peer_prefix *prefix)
{
	if (prefix->family == NET_AF_INET) {
		return self->have_ipv4 &&
		       memcmp(&self->ipv4, prefix->address, sizeof(self->ipv4)) == 0;
	}
	return self->have_ipv6 && memcmp(&self->ipv6, prefix->address, sizeof(self->ipv6)) == 0;
}

static int read_self(const struct tailscale_peer_snapshot *snapshot, struct self_addresses *self)
{
	if (!snapshot->have_self ||
	    memcmp(active.expected_key, snapshot->self.key, NODE_KEY_SIZE) != 0) {
		return -EKEYREJECTED;
	}
	for (size_t i = 0; i < snapshot->self.address_count; ++i) {
		const struct tailscale_peer_prefix *prefix = &snapshot->self.addresses[i];

		if (!host_prefix(prefix)) {
			return -ENOTSUP;
		}
		if (prefix->family == NET_AF_INET && !self->have_ipv4) {
			memcpy(&self->ipv4, prefix->address, sizeof(self->ipv4));
			self->have_ipv4 = true;
		} else if (prefix->family == NET_AF_INET6 && !self->have_ipv6) {
			memcpy(&self->ipv6, prefix->address, sizeof(self->ipv6));
			self->have_ipv6 = true;
		} else {
			return -ENOTSUP;
		}
	}
	if (!self->have_ipv4) {
		return -ENOTSUP;
	}
	if (active.have_address &&
	    (self->have_ipv4 != active.self.have_ipv4 || self->have_ipv6 != active.self.have_ipv6 ||
	     !net_ipv4_addr_cmp(&self->ipv4, &active.self.ipv4) ||
	     (self->have_ipv6 && !net_ipv6_addr_cmp(&self->ipv6, &active.self.ipv6)))) {
		return -EKEYREJECTED;
	}
	return 0;
}

static int add_self(const struct self_addresses *self)
{
	struct net_in_addr mask = {.s_addr = UINT32_MAX};
	struct net_if *owner = NULL;
	struct net_if_addr *existing;

	/* Existing sockets can keep a withdrawn tunnel address alive. Let
	 * net_if_addr_add restore it when control grants it again.
	 */
	existing = net_if_ipv4_addr_lookup(&self->ipv4, &owner);
	if (existing != NULL && (owner != active.iface || existing->is_added)) {
		return -EADDRINUSE;
	}
	if (self->have_ipv6) {
		existing = net_if_ipv6_addr_lookup(&self->ipv6, &owner);
		if (existing != NULL && (owner != active.iface || existing->is_added)) {
			return -EADDRINUSE;
		}
	}
	if (net_if_ipv4_addr_add(active.iface, &self->ipv4, NET_ADDR_MANUAL, 0) == NULL) {
		return -ENOMEM;
	}
	if (!net_if_ipv4_set_netmask_by_addr(active.iface, &self->ipv4, &mask) ||
	    (self->have_ipv6 &&
	     net_if_ipv6_addr_add(active.iface, &self->ipv6, NET_ADDR_MANUAL, 0) == NULL)) {
		net_if_ipv4_addr_rm(active.iface, &self->ipv4);
		return -ENOMEM;
	}
	return 0;
}

static void remove_self(const struct self_addresses *self)
{
	if (self->have_ipv4) {
		net_if_ipv4_addr_rm(active.iface, &self->ipv4);
	}
	if (self->have_ipv6) {
		net_if_ipv6_addr_rm(active.iface, &self->ipv6);
	}
}

static void select_endpoint(const struct tailscale_peer *peer, struct net_sockaddr_storage *result)
{
	struct net_sockaddr_in *dst = net_sin(net_sad(result));

	for (size_t i = 0; i < peer->endpoint_count; ++i) {
		const struct tailscale_peer_endpoint *src = &peer->endpoints[i];
		struct net_in_addr address;

		if (src->family != NET_AF_INET) {
			continue;
		}
		memcpy(&address, src->address, sizeof(address));
		bool on_link = physical != NULL && net_if_ipv4_addr_mask_cmp(physical, &address);

		if (dst->sin_family == NET_AF_UNSPEC || on_link) {
			dst->sin_addr = address;
			dst->sin_family = NET_AF_INET;
			dst->sin_port = net_htons(src->port);
			if (on_link) {
				return;
			}
		}
	}
}

static int prepare_peer(const struct tailscale_peer *peer, const struct self_addresses *self,
			size_t slot, uint8_t selected_prefixes)
{
	struct wireguard_policy_peer *row = &candidate[slot];
	size_t count = peer->allowed_ip_count;
	size_t encoded_len;
	size_t used = 0;

	if (memcmp(active.expected_key, peer->key, NODE_KEY_SIZE) == 0) {
		return -EINVAL;
	}
	if (base64_encode(keys[slot], sizeof(keys[slot]), &encoded_len, peer->key,
			  sizeof(peer->key)) != 0) {
		return -EINVAL;
	}

	*row = (struct wireguard_policy_peer){.identity = peer->id};
	row->config.public_key = (const char *)keys[slot];
	row->underlay = physical;
	select_endpoint(peer, &row->config.endpoint_ip);

	for (size_t i = 0; i < count; ++i) {
		const struct tailscale_peer_prefix *prefix = &peer->allowed_ips[i];
		struct wireguard_allowed_ip *allowed;

		if (!(selected_prefixes & BIT(i))) {
			continue;
		}
		if (used == ARRAY_SIZE(row->config.allowed_ip)) {
			return -ENOSPC;
		}
		allowed = &row->config.allowed_ip[used++];
		bool host = prefix->length == (prefix->family == NET_AF_INET ? 32 : 128);

		if ((!IS_ENABLED(CONFIG_TAILSCALE_ROUTE_POLICY) || host) && !host_prefix(prefix)) {
			return -ENOTSUP;
		}
		if (host && self_prefix(self, prefix)) {
			return -EINVAL;
		}
		allowed->addr.family = prefix->family;
		if (prefix->family == NET_AF_INET) {
			memcpy(&allowed->addr.in_addr, prefix->address,
			       sizeof(allowed->addr.in_addr));
		} else {
			memcpy(&allowed->addr.in6_addr, prefix->address,
			       sizeof(allowed->addr.in6_addr));
		}
		allowed->mask_len = prefix->length;
		allowed->is_valid = true;
	}
	return 0;
}

static int credential_deadline(const struct tailscale_peer_snapshot *snapshot,
			       const struct tailscale_peer *peer, int64_t *deadline)
{
	if (!peer->key_expires) {
		*deadline = INT64_MAX;
		return 0;
	}
	if (!snapshot->have_control_time) {
		return -ENODATA;
	}
	*deadline = peer->key_expiry_ms - snapshot->control_epoch_at_boot_ms;
	return 0;
}

#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
static bool filter_address_equal(const struct tailscale_filter_address *a,
				 const struct tailscale_filter_address *b)
{
	return a->family == b->family && memcmp(a->first, b->first, sizeof(a->first)) == 0 &&
	       memcmp(a->last, b->last, sizeof(a->last)) == 0;
}

static bool filter_equal(const struct tailscale_packet_filter *old,
			 const struct tailscale_packet_filter *next)
{
	if (old->reserve_ssh != next->reserve_ssh || old->count != next->count) {
		return false;
	}
	for (size_t i = 0; i < old->count; i++) {
		const struct tailscale_packet_filter_rule *a = &old->rules[i];
		const struct tailscale_packet_filter_rule *b = &next->rules[i];

		if (a->protocols != b->protocols || a->source_count != b->source_count ||
		    a->destination_count != b->destination_count) {
			return false;
		}
		for (size_t j = 0; j < a->source_count; j++) {
			if (!filter_address_equal(&a->sources[j], &b->sources[j])) {
				return false;
			}
		}
		for (size_t j = 0; j < a->destination_count; j++) {
			const struct tailscale_packet_filter_dst *src = &a->destinations[j];
			const struct tailscale_packet_filter_dst *dst = &b->destinations[j];

			if (src->first_port != dst->first_port ||
			    src->last_port != dst->last_port ||
			    !filter_address_equal(&src->address, &dst->address)) {
				return false;
			}
		}
	}
	return true;
}

static bool peer_routes_equal(const struct tailscale_peer *old, uint8_t old_mask,
			      const struct tailscale_peer *next, uint8_t next_mask)
{
	if (POPCOUNT(old_mask) != POPCOUNT(next_mask) ||
	    memcmp(old->key, next->key, sizeof(old->key)) != 0 ||
	    strcmp(old->stable_id, next->stable_id) != 0) {
		return false;
	}
	for (size_t i = 0; i < old->allowed_ip_count; i++) {
		const struct tailscale_peer_prefix *a = &old->allowed_ips[i];
		bool match = false;

		if ((old_mask & BIT(i)) == 0U) {
			continue;
		}
		for (size_t j = 0; j < next->allowed_ip_count; j++) {
			const struct tailscale_peer_prefix *b = &next->allowed_ips[j];

			if ((next_mask & BIT(j)) != 0U && a->family == b->family &&
			    a->length == b->length &&
			    memcmp(a->address, b->address, sizeof(a->address)) == 0) {
				match = true;
				break;
			}
		}
		if (!match) {
			return false;
		}
	}
	return true;
}

static bool forwarding_unchanged(const struct policy_input *next, size_t count)
{
	const struct policy_input *old = &inputs[active.input];

	if (!active.have_address || !lease_valid() || count != active.peer_count ||
	    old->peers->self.id != next->peers->self.id || old->exit != next->exit ||
	    (next->exit && old->allow_lan != next->allow_lan) ||
	    !filter_equal(old->filter, next->filter)) {
		return false;
	}
	for (size_t i = 0; i < next->peers->count; i++) {
		const struct tailscale_peer *peer = &next->peers->peers[i];
		size_t j;

		if (next->routed[i] == 0U) {
			continue;
		}
		for (j = 0; j < old->peers->count; j++) {
			if (old->peers->peers[j].id == peer->id) {
				break;
			}
		}
		if (j == old->peers->count ||
		    !peer_routes_equal(&old->peers->peers[j], old->routed[j], peer,
				       next->routed[i])) {
			return false;
		}
	}
	return true;
}
#endif

static int install_policy(const struct tailscale_peer_snapshot *peers,
			  const struct tailscale_packet_filter *filter, void *context)
{
	struct policy_input *next = &inputs[active.input ^ 1U];
#ifdef CONFIG_TAILSCALE_ROUTE_POLICY
	struct route_plan plan;
#endif
	struct self_addresses self = {0};
	int64_t now = k_uptime_get();
	int64_t deadline;
	bool add_address = !active.have_address;
	size_t count = 0;
	int rc = read_self(peers, &self);

	ARG_UNUSED(context);
	if (rc == 0) {
		rc = credential_deadline(peers, &peers->self, &deadline);
	}
	if (rc != 0) {
		return rc;
	}
	if (now >= deadline) {
		return -EKEYREJECTED;
	}
	*next = (struct policy_input){
		.peers = peers,
		.filter = filter,
		.self = self,
		.self_deadline = deadline,
		.next_peer_deadline = INT64_MAX,
#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
		.exit = preferences.exit_node[0] != '\0',
		.allow_lan = preferences.allow_lan,
#endif
	};
#ifdef CONFIG_TAILSCALE_ROUTE_POLICY
	rc = tailscale_routes_plan(&plan, peers, &preferences, pinned_exit, now);
	if (rc != 0) {
		return rc;
	}
#endif
	for (size_t i = 0; i < peers->count; ++i) {
		const struct tailscale_peer *peer = &peers->peers[i];

		const char *selected = CONFIG_TAILSCALE_PEER_STABLE_ID;

		if (peer->expired || (selected[0] && strcmp(selected, peer->stable_id) != 0)) {
			continue;
		}
		rc = credential_deadline(peers, peer, &deadline);
		if (rc != 0) {
			return rc;
		}
		if (now >= deadline) {
			continue;
		}
		if (selected[0] && count != 0) {
			return -EADDRINUSE;
		}
		uint8_t selected_prefixes = UINT8_MAX;
#ifdef CONFIG_TAILSCALE_ROUTE_POLICY
		selected_prefixes = plan.prefixes[i];
		if (!selected_prefixes) {
			continue;
		}
#else
		if (peer->allowed_ip_count == 0U) {
			return -ENOSPC;
		}
#endif
		if (count == ARRAY_SIZE(candidate)) {
			return -ENOSPC;
		}
		rc = prepare_peer(peer, &self, count, selected_prefixes);
		if (rc != 0) {
			return rc;
		}

#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
		next->routed[i] = selected_prefixes;
#endif
#ifdef CONFIG_TAILSCALE_TRANSPORT
		struct tailscale_transport_peer *target = &transport_plan.peers[count];

		target->region = peer->derp_region;
		target->candidate_count = peer->endpoint_count;
		memcpy(target->candidates, peer->endpoints, sizeof(target->candidates));
		target->endpoint = candidate[count].config.endpoint_ip;
		memcpy(target->key, peer->key, sizeof(target->key));
		memcpy(target->disco, peer->disco_key, sizeof(target->disco));
		candidate[count].peer_id = &target->id;
#endif
		int64_t effective_deadline = MIN(deadline, next->self_deadline);

		candidate[count].expires_at_ms = effective_deadline == INT64_MAX ?
						 0 : effective_deadline;
		next->next_peer_deadline = MIN(next->next_peer_deadline, deadline);
		++count;
	}
#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
	/* Control metadata changes must not invalidate existing socket grants. */
	next->revision = inputs[active.input].revision;
	if (!forwarding_unchanged(next, count)) {
		next->revision++;
	}
#endif
#ifdef CONFIG_TAILSCALE_TRANSPORT
	transport_plan.count = count;
	rc = tailscale_transport_stage(&transport_plan, peers);
	if (rc != 0) {
		return rc;
	}
	struct wireguard_transport transport = {
		.send = tailscale_transport_send,
		.user_data = UINT_TO_POINTER(transport_plan.epoch),
	};

	for (size_t i = 0; i < count; i++) {
		candidate[i].transport = &transport;
	}
#endif
	if (add_address) {
		rc = add_self(&self);
		if (rc != 0) {
			return rc;
		}
	}

	rc = wireguard_replace_peers(active.iface, candidate, count, allow_input, next);
	if (rc == 0) {
		active.self = self;
		active.have_address = true;
		active.input ^= 1U;
		active.peer_count = count;
#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
		publish_prefixes(count);
#endif
#ifdef CONFIG_TAILSCALE_ROUTE_POLICY
		exit_owner = plan.exit;
		for (size_t i = 0; plan.exit && i < peers->count; i++) {
			if (peers->peers[i].id == plan.exit) {
				memcpy(pinned_exit, peers->peers[i].stable_id, sizeof(pinned_exit));
				break;
			}
		}
#endif
#ifdef CONFIG_TAILSCALE_TRANSPORT
		tailscale_transport_publish(&transport_plan);
#endif
	} else if (add_address) {
		remove_self(&self);
	}
	return rc;
}

int tailscale_client_policy_start_hosted(struct net_if *iface, const uint8_t private_key[32],
				const uint8_t public_key[32])
{
	uint8_t key_bytes[NODE_KEY_SIZE];
	struct virtual_interface_req_params key = {
		.private_key = {.data = key_bytes, .len = sizeof(key_bytes)},
	};
	int rc = -EBUSY;

	if (iface == NULL || private_key == NULL || public_key == NULL) {
		return -EINVAL;
	}
	k_mutex_lock(&policy_lock, K_FOREVER);
	if (active.iface == NULL) {
		memcpy(key_bytes, private_key, sizeof(key_bytes));
		rc = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PRIVATE_KEY, iface, &key,
			      sizeof(key));
		mbedtls_platform_zeroize(key_bytes, sizeof(key_bytes));
		if (rc == 0) {
			if (!active.initialized) {
				tailscale_policy_init(&policy);
				active.initialized = true;
			}
			active.previous_mtu = net_if_get_mtu(iface);
			net_if_set_mtu(iface, 1280);
			net_if_dormant_on(iface);
			rc = wireguard_replace_peers(iface, NULL, 0, allow_input, NULL);
			if (rc != 0) {
				int cleanup = wireguard_release_peers(iface);

				net_if_dormant_off(iface);
				net_if_set_mtu(iface, active.previous_mtu);
				if (cleanup != 0 && cleanup != -ENOENT) {
					LOG_ERR("Could not release the provisioned identity (%d)",
						cleanup);
				}
			}
		}
		if (rc == 0) {
			active.iface = iface;
#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
			publish_routing(iface);
#endif
			active.stopping = false;
			memcpy(active.expected_key, public_key, sizeof(active.expected_key));
		}
	}
	k_mutex_unlock(&policy_lock);
	return rc;
}

static int revoke_policy(void)
{
	int rc;

	wireguard_policy_block(active.iface);
#ifdef CONFIG_TAILSCALE_TRANSPORT
	tailscale_transport_publish(NULL);
#endif
	active.lease_deadline = 0;
	publish_summary(NULL, 0);
	active.require_new_generation = active.have_generation;
	rc = wireguard_replace_peers(active.iface, NULL, 0, allow_input, NULL);
	active.withdraw_pending = rc != 0;
	if (rc == 0 && active.have_address) {
		remove_self(&active.self);
		active.have_address = false;
	}
	return rc;
}

static int retire_expired(void)
{
	const struct policy_input *input = &inputs[active.input];
	int64_t now = k_uptime_get();

	if (active.withdraw_pending || (active.lease_deadline != 0 &&
					now >= MIN(active.lease_deadline, input->self_deadline))) {
		return revoke_policy();
	}
	if (active.lease_deadline != 0 && now >= input->next_peer_deadline) {
		int rc = install_policy(input->peers, input->filter, NULL);

		if (rc == -EKEYREJECTED) {
			return revoke_policy();
		}
		if (rc == 0) {
			k_spinlock_key_t key = k_spin_lock(&summary_lock);

			struct tailscale_peer_inventory_summary summary = installed_summary;
			int64_t deadline = installed_deadline;

			k_spin_unlock(&summary_lock, key);
			summary.peer_count = active.peer_count;
			publish_summary(&summary, deadline);
		}
		return rc;
	}
	return 0;
}

static void schedule_expiry(int previous_result)
{
	const struct policy_input *input = &inputs[active.input];

	if (previous_result != 0) {
		k_work_reschedule(&policy_expiry, K_MSEC(WITHDRAW_RETRY_MS));
	} else if (active.lease_deadline != 0) {
		int64_t deadline = MIN(active.lease_deadline,
				       MIN(input->self_deadline, input->next_peer_deadline));
		int64_t remaining = deadline - k_uptime_get();

		k_work_reschedule(&policy_expiry, remaining > 0 ? K_MSEC(remaining) : K_NO_WAIT);
	}
}

static void expire_policy(struct k_work *work)
{
	int rc = 0;

	ARG_UNUSED(work);
	k_mutex_lock(&policy_lock, K_FOREVER);
	if (active.iface != NULL && !active.stopping) {
		rc = retire_expired();
		schedule_expiry(rc);
	}
	k_mutex_unlock(&policy_lock);
	if (rc != 0 && rc != -EBUSY) {
		LOG_ERR("Could not retire expired tailnet policy (%d)", rc);
	}
}

int tailscale_client_policy_revoke(void)
{
	int rc = 0;

	k_mutex_lock(&policy_lock, K_FOREVER);
	if (active.iface != NULL && !active.stopping) {
		rc = revoke_policy();
		schedule_expiry(rc);
	}
	k_mutex_unlock(&policy_lock);
	return rc;
}

int tailscale_client_policy_apply(uint32_t generation, uint8_t *json, size_t len,
			enum tailscale_peer_inventory_frame *frame)
{
	struct tailscale_peer_inventory_summary summary;
	enum tailscale_peer_inventory_frame effect;
	int rc;

	if (generation == 0U || json == NULL || len == 0U) {
		return -EINVAL;
	}
	k_mutex_lock(&policy_lock, K_FOREVER);
	if (active.iface == NULL || active.stopping) {
		rc = -ENETDOWN;
		goto out;
	}
	rc = retire_expired();
	if (rc != 0) {
		schedule_expiry(rc);
		goto out;
	}
	if (active.have_generation &&
	    ((int32_t)(generation - active.last_generation) < 0 ||
	     (active.require_new_generation && generation == active.last_generation))) {
		rc = -ESTALE;
		goto out;
	}

	active.last_generation = generation;
	active.have_generation = true;
	active.require_new_generation = false;
	rc = tailscale_policy_apply(&policy, generation, json, len, install_policy, NULL, &effect);
	if (rc == 0) {
		rc = tailscale_policy_get_summary(&policy, &summary);
		if (rc == 0 && summary.valid && summary.generation == generation) {
			if (effect != TAILSCALE_PEER_INVENTORY_OTHER) {
				active.lease_deadline = k_uptime_get() + POLICY_LEASE_MS;
			}
			summary.peer_count = active.peer_count;
			publish_summary(&summary, MIN(active.lease_deadline,
						      inputs[active.input].self_deadline));
		}
		if (rc == 0 && frame != NULL) {
			*frame = effect;
		}
	} else if (rc == -EKEYREJECTED) {
		int revoke_rc = revoke_policy();

		if (revoke_rc != 0) {
			schedule_expiry(revoke_rc);
			rc = revoke_rc;
		}
	}
	if (!active.withdraw_pending) {
		schedule_expiry(0);
	}
out:
	k_mutex_unlock(&policy_lock);
	return rc;
}

int tailscale_client_policy_summary(struct tailscale_peer_inventory_summary *summary)
{
	k_spinlock_key_t key;

	if (summary == NULL) {
		return -EINVAL;
	}
	key = k_spin_lock(&summary_lock);
	*summary = installed_summary;
	int64_t deadline = installed_deadline;

	k_spin_unlock(&summary_lock, key);
	if (summary->valid && k_uptime_get() >= deadline) {
		summary->valid = false;
		summary->peer_count = 0;
	}
	return 0;
}

#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
static int dns_authority(const char *name)
{
	if (active.iface == NULL) {
		return -ENOTSUP;
	}
	bool ready = active.have_address && !active.stopping && lease_valid();
	int rc = tailscale_policy_dns_authority(&policy, name, ready);

	if (preferences.exit_node[0] != '\0' && rc == -ENOTSUP) {
		/* System resolvers may be bound to a physical interface. */
		return ready ? -ENETUNREACH : -ENETDOWN;
	}
	return rc;
}

int tailscale_client_policy_dns_authority(const char *name)
{
	k_mutex_lock(&policy_lock, K_FOREVER);
	int rc = dns_authority(name);

	k_mutex_unlock(&policy_lock);
	return rc;
}

bool tailscale_net_dns_offload_allowed(const char *host)
{
	if (host == NULL) {
		return true;
	}
	k_mutex_lock(&policy_lock, K_FOREVER);
	bool allowed = dns_authority(host) == -ENOTSUP;

	k_mutex_unlock(&policy_lock);
	return allowed;
}

#endif

int tailscale_client_policy_select_dns(const char *name, struct tailscale_dns_selection *selection)
{
	int rc = -ENETDOWN;

	if (name == NULL || selection == NULL) {
		return -EINVAL;
	}
	k_mutex_lock(&policy_lock, K_FOREVER);
	if (active.have_address && lease_valid()) {
		rc = tailscale_policy_select_dns(&policy, name, selection);
	}
	k_mutex_unlock(&policy_lock);
	return rc;
}

int tailscale_client_policy_dns_search_domain(uint64_t revision, size_t index,
				     char domain[TAILSCALE_DNS_NAME_SIZE])
{
	int rc = -ENETDOWN;

	k_mutex_lock(&policy_lock, K_FOREVER);
	if (active.have_address && lease_valid()) {
		rc = tailscale_policy_dns_search_domain(&policy, revision, index, domain);
	}
	k_mutex_unlock(&policy_lock);
	return rc;
}

int tailscale_client_policy_resolve_magic_dns(const char *name, uint8_t family,
				     struct tailscale_dns_answer *answer)
{
	struct tailscale_dns_answer found;
	int rc = -ENETDOWN;

	if (name == NULL || answer == NULL || (family != NET_AF_INET && family != NET_AF_INET6)) {
		return -EINVAL;
	}
	k_mutex_lock(&policy_lock, K_FOREVER);
	if (active.have_address && lease_valid()) {
		rc = tailscale_policy_resolve_magic_dns(&policy, name, family, &found);
	}
	if (rc == 0 && !found.self) {
		for (size_t i = 0; i < found.address_count; i++) {
			if (wireguard_policy_route_owner(active.iface, family,
							 found.addresses[i].address) !=
			    found.node_id) {
				rc = -ENOENT;
				break;
			}
		}
	}
	if (rc == 0) {
		*answer = found;
	}
	k_mutex_unlock(&policy_lock);
	return rc;
}

int tailscale_client_policy_stop(void)
{
	int rc = 0;

	k_mutex_lock(&policy_lock, K_FOREVER);
	if (active.iface != NULL) {
		wireguard_policy_block(active.iface);
		active.stopping = true;
		active.lease_deadline = 0;
		active.withdraw_pending = false;
		k_work_cancel_delayable(&policy_expiry);
		publish_summary(NULL, 0);
		active.require_new_generation = active.have_generation;
		rc = wireguard_release_peers(active.iface);
#ifdef CONFIG_TAILSCALE_TRANSPORT
		tailscale_transport_stop();
#endif
		if (rc == 0) {
			if (active.have_address) {
				remove_self(&active.self);
			}
			net_if_dormant_off(active.iface);
			net_if_set_mtu(active.iface, active.previous_mtu);
			active.iface = NULL;
#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
			publish_routing(NULL);
#endif
			active.have_address = false;
		}
	}
	k_mutex_unlock(&policy_lock);
	return rc;
}

void tailscale_client_policy_set_underlay(struct net_if *iface)
{
	k_mutex_lock(&policy_lock, K_FOREVER);
	physical = iface;
	k_mutex_unlock(&policy_lock);
}

#ifdef CONFIG_TAILSCALE_ROUTE_POLICY
int tailscale_routes_set(const struct tailscale_routes *routes)
{
	struct tailscale_routes previous;
	char old_pin[sizeof(pinned_exit)];
	int rc = 0;

	if (!routes || !memchr(routes->exit_node, '\0', sizeof(routes->exit_node))) {
		return -EINVAL;
	}
	k_mutex_lock(&policy_lock, K_FOREVER);
	previous = preferences;
	memcpy(old_pin, pinned_exit, sizeof(old_pin));
	if (strcmp(routes->exit_node, preferences.exit_node)) {
		pinned_exit[0] = '\0';
	} else if (routes->accept_routes == preferences.accept_routes &&
		   routes->allow_lan == preferences.allow_lan) {
		goto out;
	}
	preferences = *routes;
	if (active.have_address && lease_valid()) {
		const struct policy_input *input = &inputs[active.input];

		rc = install_policy(input->peers, input->filter, NULL);
		if (rc != 0) {
			preferences = previous;
			memcpy(pinned_exit, old_pin, sizeof(old_pin));
		} else {
			struct tailscale_peer_inventory_summary summary = installed_summary;

			summary.peer_count = active.peer_count;
			publish_summary(&summary, installed_deadline);
			schedule_expiry(0);
		}
	}
out:
#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
	if (rc == 0) {
		publish_routing(active.iface);
	}
#endif
	k_mutex_unlock(&policy_lock);
	return rc;
}

int tailscale_routes_get(struct tailscale_routes *routes,
			 char stable_id[TAILSCALE_PEER_STABLE_ID_SIZE])
{
	if (!routes || !stable_id) {
		return -EINVAL;
	}
	k_mutex_lock(&policy_lock, K_FOREVER);
	*routes = preferences;
	if (active.have_address && lease_valid() && exit_owner) {
		memcpy(stable_id, pinned_exit, sizeof(pinned_exit));
	} else {
		stable_id[0] = '\0';
	}
	k_mutex_unlock(&policy_lock);
	return 0;
}
#endif

#ifdef CONFIG_TAILSCALE_CLIENT_INGRESS
uint64_t tailscale_client_policy_route_revision(void)
{
	k_mutex_lock(&policy_lock, K_FOREVER);
	uint64_t revision =
		active.have_address && lease_valid() ? inputs[active.input].revision : 0;
	k_mutex_unlock(&policy_lock);
	return revision;
}

static bool public_resolver(net_sa_family_t family, const void *address)
{
	if (family == NET_AF_INET6) {
		return (((const struct net_in6_addr *)address)->s6_addr[0] & 0xe0U) == 0x20U;
	}
	uint32_t addr = net_ntohl(((const struct net_in_addr *)address)->s_addr);

	return (addr >> 24) != 0U && (addr >> 24) != 10U && (addr >> 24) != 127U &&
	       (addr >> 28) < 14U && (addr & 0xffc00000U) != 0x64400000U &&
	       (addr & 0xfff00000U) != 0xac100000U && (addr >> 16) != 0xa9feU &&
	       (addr >> 16) != 0xc0a8U;
}

/* Called with policy_lock held and only for a resolver selected by policy. */
static int route_dns(const struct net_sockaddr *remote, struct net_if **iface,
		     struct net_sockaddr_storage *local)
{
	struct net_if *chosen = active.iface;
	net_sa_family_t family = remote->sa_family;
	const void *dst;
	const void *src;
	void *local_addr;
	size_t size;

	if (!active.have_address || active.stopping || !lease_valid()) {
		return -ENETDOWN;
	}
	memset(local, 0, sizeof(*local));
	local->ss_family = family;
	if (family == NET_AF_INET) {
		dst = &net_sin(remote)->sin_addr;
		local_addr = &net_sin(net_sad(local))->sin_addr;
		size = sizeof(struct net_in_addr);
		if (net_ipv4_is_addr_unspecified(dst) || net_ipv4_is_addr_loopback(dst) ||
		    net_ipv4_is_addr_mcast(dst) ||
		    net_ipv4_addr_cmp(dst, net_ipv4_broadcast_address())) {
			return -EINVAL;
		}
	} else if (family == NET_AF_INET6) {
		dst = &net_sin6(remote)->sin6_addr;
		local_addr = &net_sin6(net_sad(local))->sin6_addr;
		size = sizeof(struct net_in6_addr);
		if (net_ipv6_is_addr_unspecified(dst) || net_ipv6_is_addr_loopback(dst) ||
		    net_ipv6_is_addr_mcast(dst)) {
			return -EINVAL;
		}
	} else {
		return -EAFNOSUPPORT;
	}

	if (wireguard_policy_route_owner(chosen, family, dst) != 0U) {
		if (family == NET_AF_INET6 && !active.self.have_ipv6) {
			return -EADDRNOTAVAIL;
		}
		src = family == NET_AF_INET ? (const void *)&active.self.ipv4 : &active.self.ipv6;
	} else {
		if (physical == NULL || preferences.exit_node[0] != '\0' ||
		    tailscale_net_route(family, dst) != NULL || !public_resolver(family, dst) ||
		    tailscale_routes_claim_resolver(inputs[active.input].peers, family, dst,
						   preferences.accept_routes)) {
			return -ENETUNREACH;
		}
		chosen = physical;
		if (family == NET_AF_INET) {
			src = net_if_ipv4_select_src_addr(chosen, dst);
			if (src == NULL || net_ipv4_is_addr_unspecified(src)) {
				return -EADDRNOTAVAIL;
			}
		} else {
			src = net_if_ipv6_select_src_addr(chosen, dst);
			if (src == NULL || net_ipv6_is_addr_unspecified(src)) {
				return -EADDRNOTAVAIL;
			}
		}
	}
	memcpy(local_addr, src, size);
	*iface = chosen;
	return 0;
}

int tailscale_client_policy_route_dns(const struct net_sockaddr *remote, struct net_if **iface,
				struct net_sockaddr_storage *local, uint64_t *revision)
{
	if (!remote || !iface || !local || !revision) {
		return -EINVAL;
	}
	k_mutex_lock(&policy_lock, K_FOREVER);
	int rc = route_dns(remote, iface, local);

	if (rc == 0) {
		*revision = inputs[active.input].revision;
	}
	k_mutex_unlock(&policy_lock);
	return rc;
}
#endif
