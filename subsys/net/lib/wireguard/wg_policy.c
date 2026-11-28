/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_WIREGUARD_PEER_POLICY)
/* The lifecycle reservation protects the inactive route bank and this scratch
 * space while staging peers, changing attachments, or draining deliveries.
 */
static struct {
	const struct wireguard_policy_peer *selected[WG_PEER_SLOTS];
	int events[WG_PEER_SLOTS];
	uint32_t route_mask[ROUTE_WORDS];
	size_t additions;
} policy_update;

static bool policy_called_from_handoff(void)
{
	const struct wg_policy_handoff *handoff;

	SYS_SLIST_FOR_EACH_CONTAINER(&policy_handoffs, handoff, node) {
		if (handoff->thread == k_current_get()) {
			return true;
		}
	}
	return false;
}

static int policy_validate_config(const struct wireguard_policy_peer *entry,
				  uint8_t public_key[WG_PUBLIC_KEY_LEN])
{
	const struct wireguard_peer_config *config = &entry->config;
	net_sa_family_t family = config->endpoint_ip.ss_family;

	if ((entry->transport != NULL && entry->transport->send == NULL) ||
	    entry->identity == 0U || entry->expires_at_ms < 0 || config->public_key == NULL ||
	    !extract_public_key(config->public_key, public_key, WG_PUBLIC_KEY_LEN) ||
	    config->keepalive_interval < 0 || config->keepalive_interval > UINT16_MAX) {
		return -EINVAL;
	}
	if (config->preshared_key != NULL &&
	    memcmp(config->preshared_key, zero_key, WG_SESSION_KEY_LEN) != 0) {
		return -ENOTSUP;
	}
	if (family != NET_AF_UNSPEC &&
	    !(IS_ENABLED(CONFIG_NET_IPV4) && family == NET_AF_INET) &&
	    !(IS_ENABLED(CONFIG_NET_IPV6) && family == NET_AF_INET6)) {
		return -EAFNOSUPPORT;
	}

	return 0;
}

static int policy_copy_prefix(struct wg_allowed_ip *out, const struct wireguard_allowed_ip *in)
{
	const uint8_t *address;
	uint8_t *canonical;
	uint8_t bytes;
	uint8_t tail;

	if (IS_ENABLED(CONFIG_NET_IPV4) && in->addr.family == NET_AF_INET &&
	    in->mask_len <= 32U) {
		address = (const uint8_t *)&in->addr.in_addr;
		canonical = (uint8_t *)&out->addr.in_addr;
	} else if (IS_ENABLED(CONFIG_NET_IPV6) && in->addr.family == NET_AF_INET6 &&
		   in->mask_len <= 128U) {
		address = in->addr.in6_addr.s6_addr;
		canonical = out->addr.in6_addr.s6_addr;
	} else {
		return -EINVAL;
	}

	out->addr.family = in->addr.family;
	out->mask_len = in->mask_len;
	out->is_valid = true;
	bytes = DIV_ROUND_UP(in->mask_len, 8U);
	tail = in->mask_len % 8U;
	memcpy(canonical, address, bytes);
	if (tail != 0U) {
		canonical[bytes - 1U] &= UINT8_MAX << (8U - tail);
	}

	return 0;
}

static int policy_set_prefixes(wg_peer_prefixes_t *routes, size_t slot,
			       const struct wireguard_peer_config *config)
{
	memset(routes[slot], 0, sizeof(routes[slot]));

	ARRAY_FOR_EACH(config->allowed_ip, i) {
		const struct wireguard_allowed_ip *input = &config->allowed_ip[i];
		struct wg_allowed_ip *prefix = &routes[slot][i];
		const uint8_t *address;
		int ret;

		if (!input->is_valid) {
			continue;
		}
		ret = policy_copy_prefix(prefix, input);
		if (ret < 0) {
			return ret;
		}
		address = prefix->addr.family == NET_AF_INET ?
			  (const uint8_t *)&prefix->addr.in_addr : prefix->addr.in6_addr.s6_addr;

		ARRAY_FOR_EACH(policy_update.selected, j) {
			if (j != slot && policy_update.selected[j] == NULL) {
				continue;
			}
			for (size_t p = 0; p < (j == slot ? i : CONFIG_WIREGUARD_MAX_SRC_IPS);
			     p++) {
				const struct wg_allowed_ip *other = &routes[j][p];

				if (other->mask_len == prefix->mask_len &&
				    wg_allowed_ip_match(other, prefix->addr.family, address)) {
					return -EADDRINUSE;
				}
			}
		}
	}

	return 0;
}

#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
static struct {
	struct wg_iface_context *ctx;
	struct wg_peer *peer;
	uint8_t public_key[WG_PUBLIC_KEY_LEN];
	int result;
} stage_peer_request;
static K_SEM_DEFINE(stage_peer_done, 0, 1);

static void policy_init_peer_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	stage_peer_request.result = wg_peer_init(stage_peer_request.ctx, stage_peer_request.peer,
						 stage_peer_request.public_key, NULL);
	k_sem_give(&stage_peer_done);
}

static K_WORK_DEFINE(stage_peer_work, policy_init_peer_handler);
#endif

static int policy_init_peer(struct wg_iface_context *ctx, struct wg_peer *peer,
			    const uint8_t *public_key)
{
#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
	int ret;

	/* The lifecycle guard owns the reserved slot and interface key until work completes. */
	k_mutex_unlock(&lock);
	if (k_current_get() == k_work_queue_thread_get(&wg_handshake_workq)) {
		ret = wg_peer_init(ctx, peer, public_key, NULL);
	} else {
		stage_peer_request.ctx = ctx;
		stage_peer_request.peer = peer;
		memcpy(stage_peer_request.public_key, public_key, WG_PUBLIC_KEY_LEN);
		ret = k_work_submit_to_queue(&wg_handshake_workq, &stage_peer_work);
		if (ret >= 0) {
			ret = k_sem_take(&stage_peer_done, K_FOREVER);
			if (ret == 0) {
				ret = stage_peer_request.result;
			}
		}
	}
	k_mutex_lock(&lock, K_FOREVER);
	return ret;
#else
	return wg_peer_init(ctx, peer, public_key, NULL);
#endif
}

static int policy_stage_peer(struct wg_iface_context *ctx, wg_peer_prefixes_t *routes,
			     const struct wireguard_policy_peer *entry)
{
	uint8_t public_key[WG_PUBLIC_KEY_LEN];
	struct wg_peer *peer;
	size_t slot;
	int ret;

	ret = policy_validate_config(entry, public_key);
	if (ret < 0) {
		return ret;
	}
	if (entry->underlay != NULL &&
	    (net_if_get_by_iface(entry->underlay) < 0 || entry->underlay == wg_ctx.iface ||
	     get_wg_iface_context(entry->underlay) != NULL)) {
		return -EINVAL;
	}
	if (memcmp(public_key, ctx->public_key, sizeof(public_key)) == 0) {
		return -EINVAL;
	}
	ARRAY_FOR_EACH(policy_update.selected, i) {
		const struct wireguard_policy_peer *previous = policy_update.selected[i];

		if (previous != NULL &&
		    (previous->identity == entry->identity ||
		     memcmp(peers[i].key.public_key, public_key, sizeof(public_key)) == 0)) {
			return -EALREADY;
		}
	}

	peer = peer_lookup_by_pubkey(ctx, (const char *)public_key);
	if (peer == NULL || peer->policy_identity != entry->identity) {
		sys_snode_t *node;

		if (policy_update.additions >= (size_t)(INT_MAX - last_peer_id)) {
			return -ENOSPC;
		}
		node = sys_slist_get(&peer_list);
		if (node == NULL) {
			return -ENOSPC;
		}
		peer = CONTAINER_OF(node, struct wg_peer, node);
		ret = policy_init_peer(ctx, peer, public_key);
		if (ret < 0) {
			crypto_zero(peer, sizeof(*peer));
			sys_slist_prepend(&peer_list, node);
			return ret;
		}

		peer->ctx = ctx;
		peer->iface = ctx->iface;
		peer->policy_identity = entry->identity;
		policy_update.additions++;
		peer->id = last_peer_id + (int)policy_update.additions;
		policy_update.events[peer - peers] = peer->id;
	}

	slot = peer - peers;
	ret = policy_set_prefixes(routes, slot, &entry->config);
	if (ret < 0) {
		return ret;
	}
	policy_update.selected[slot] = entry;
	policy_update.route_mask[slot / 32U] |= BIT(slot % 32U);

	return 0;
}

static bool policy_endpoints_equal(const struct net_sockaddr_storage *a,
				   const struct net_sockaddr_storage *b)
{
	const struct net_sockaddr_in *a4 = net_sin(net_sad(a));
	const struct net_sockaddr_in *b4 = net_sin(net_sad(b));

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		return net_sockaddr_cmp(net_sad(a), net_sad(b));
	}

	return a->ss_family == NET_AF_INET && b->ss_family == NET_AF_INET &&
	       a4->sin_port == b4->sin_port && net_ipv4_addr_cmp(&a4->sin_addr, &b4->sin_addr);
}

static void policy_update_config(struct wg_peer *peer, const struct wireguard_peer_config *config)
{
	struct net_sockaddr_storage endpoint = config->endpoint_ip;
	uint16_t keepalive = (uint16_t)config->keepalive_interval;

	if (endpoint.ss_family != NET_AF_UNSPEC &&
	    net_sin(net_sad(&endpoint))->sin_port == 0U) {
		net_sin(net_sad(&endpoint))->sin_port = net_htons(WG_DEFAULT_PORT);
	}
	if (!peer->policy_endpoint_learned ||
	    (peer->transport.send == NULL && endpoint.ss_family != NET_AF_UNSPEC &&
	     !policy_endpoints_equal(&endpoint, &peer->cfg_endpoint))) {
		peer->endpoint = endpoint;
		peer->policy_endpoint_learned = false;
	}
	peer->cfg_endpoint = endpoint;
	if (peer->keepalive_interval != keepalive) {
		peer->keepalive_interval = keepalive;
		peer->keepalive_expires = sys_timepoint_calc(K_SECONDS(keepalive));
	}
}

static int policy_attach(struct wg_iface_context *ctx)
{
	int ret;

	k_mutex_unlock(&lock);
	ret = net_virtual_interface_attach(ctx->iface, wg_ctx.iface);
	if (ret == 0) {
		atomic_inc(&wg_ctx.attached_peers);
		ret = net_if_up(ctx->iface);
		if (ret == -EALREADY) {
			ret = 0;
		} else if (ret < 0) {
			(void)net_virtual_interface_attach(ctx->iface, NULL);
			atomic_dec(&wg_ctx.attached_peers);
		}
	}
	k_mutex_lock(&lock, K_FOREVER);

	return ret;
}

static void policy_publish(struct wg_iface_context *ctx, unsigned int next_bank,
			   wireguard_policy_input_t allow_input, void *context)
{
	unsigned int handoff_epoch = ctx->handoff_epoch;
	struct wg_peer *peer;
	struct wg_peer *next;
	k_spinlock_key_t key;
	bool wait_handoffs = ctx->handoffs[handoff_epoch] != 0U;
	bool wait_routes;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&active_peers, peer, next, node) {
		size_t slot = peer - peers;

		if (peer->ctx == ctx && policy_update.selected[slot] == NULL) {
			(void)sys_slist_find_and_remove(&active_peers, &peer->node);
			policy_update.events[slot] = -peer->id;
			memset(policy_routes[next_bank][slot], 0, sizeof(policy_routes[0][slot]));
			policy_route_expiry[next_bank][slot] = 0;
		}
	}

	ARRAY_FOR_EACH(policy_update.selected, slot) {
		const struct wireguard_policy_peer *entry = policy_update.selected[slot];

		if (entry == NULL) {
			continue;
		}
		peers[slot].transport = entry->transport != NULL ? *entry->transport :
							       (struct wireguard_transport){0};
		peers[slot].underlay = entry->underlay;
		if (entry->peer_id != NULL) {
			*entry->peer_id = peers[slot].id;
		}
		policy_update_config(&peers[slot], &entry->config);
		policy_route_expiry[next_bank][slot] = entry->expires_at_ms;
		/* Locked peer inspection uses the same prefixes as the new bank. */
		memcpy(peers[slot].allowed_ip, policy_routes[next_bank][slot],
		       sizeof(peers[slot].allowed_ip));
		if (policy_update.events[slot] > 0) {
			sys_slist_prepend(&active_peers, &peers[slot].node);
		}
	}
	last_peer_id += (int)policy_update.additions;

	ctx->shared = true;
	ctx->policy_context = context;
#if CONFIG_WIREGUARD_HANDSHAKE_WORKQ_STACK_SIZE > 0
	ctx->handshake_policy_generation++;
#endif
	ctx->handoff_epoch ^= 1U;
	key = k_spin_lock(&tx_routes_lock);
	memcpy(ctx->route_mask, policy_update.route_mask, sizeof(ctx->route_mask));
	ctx->policy_input = allow_input;
	policy_bank = next_bank;
	atomic_clear(&ctx->policy_blocked);
	wait_routes = route_readers[route_epoch] != 0U;
	if (wait_routes) {
		route_epoch ^= 1U;
	}
	k_spin_unlock(&tx_routes_lock, key);
	atomic_set(&ctx->peer_state, WG_PEER_IFACE_ACTIVE);

	k_mutex_unlock(&lock);
	if (wait_routes) {
		k_sem_take(&routes_idle, K_FOREVER);
	}
	if (wait_handoffs) {
		k_sem_take(&policy_handoffs_idle, K_FOREVER);
	}
	k_mutex_lock(&lock, K_FOREVER);

	ARRAY_FOR_EACH(policy_update.events, slot) {
		if (policy_update.events[slot] < 0) {
			wg_peer_cleanup(&peers[slot]);
		}
	}
	notify_connectivity(ctx);
}

static int policy_replace(struct wg_iface_context *ctx, const struct wireguard_policy_peer *set,
			  size_t count, wireguard_policy_input_t allow_input, void *context)
{
	unsigned int next_bank = policy_bank ^ 1U;
	bool first = ctx->policy_input == NULL;
	bool needs_native = first && count == 0;
	struct wg_peer *peer;
	size_t active = count;
	int ret;

	if (policy_called_from_handoff()) {
		return -EBUSY;
	}
	/* Keep staging slots unavailable to active peers on other interfaces. */
	SYS_SLIST_FOR_EACH_CONTAINER(&active_peers, peer, node) {
		if (peer->ctx != ctx && ++active > CONFIG_WIREGUARD_MAX_PEER) {
			return -ENOSPC;
		}
	}
	for (size_t i = 0; i < count; i++) {
		if (set[i].transport == NULL) {
			needs_native = true;
			break;
		}
	}
	if (needs_native && native_key_in_use(ctx)) {
		return -EADDRINUSE;
	}
	if (first) {
		if (atomic_get(&ctx->peer_state) != WG_PEER_IFACE_FREE ||
		    ctx->attached_to != NULL) {
			return -EBUSY;
		}
		if (ctx->private_key_id == PSA_KEY_ID_NULL) {
			return -EINVAL;
		}
		atomic_set(&ctx->peer_state, WG_PEER_IFACE_ADDING);
	}

	memset(&policy_update, 0, sizeof(policy_update));
	memcpy(policy_routes[next_bank], policy_routes[policy_bank], sizeof(policy_routes[0]));
	memcpy(policy_route_expiry[next_bank], policy_route_expiry[policy_bank],
	       sizeof(policy_route_expiry[0]));
	for (size_t i = 0; i < count; i++) {
		ret = policy_stage_peer(ctx, policy_routes[next_bank], &set[i]);
		if (ret < 0) {
			goto rollback;
		}
		/* Some PSA implementations require serialization. Let waiting
		 * packet processing run between peer preparations.
		 */
		k_mutex_unlock(&lock);
		k_mutex_lock(&lock, K_FOREVER);
	}
	if (first) {
		ret = policy_attach(ctx);
		if (ret < 0) {
			goto rollback;
		}
	}

	policy_publish(ctx, next_bank, allow_input, context);
	k_mutex_unlock(&lock);

	ARRAY_FOR_EACH(policy_update.events, slot) {
		int id = policy_update.events[slot];

		if (id > 0) {
			notify_peer_add(&peers[slot]);
		} else if (id < 0) {
			uint32_t retired_id = -id;

			net_mgmt_event_notify_with_info(NET_EVENT_VPN_PEER_DEL, ctx->iface,
							&retired_id, sizeof(retired_id));
		}
	}
	k_mutex_lock(&lock, K_FOREVER);
	memset(&policy_update, 0, sizeof(policy_update));

	return 0;

rollback:
	ARRAY_FOR_EACH(policy_update.events, slot) {
		if (policy_update.events[slot] > 0) {
			wg_peer_cleanup(&peers[slot]);
		}
	}
	if (first) {
		release_peer_iface(ctx);
	}
	memset(&policy_update, 0, sizeof(policy_update));

	return ret;
}
#endif

int wireguard_replace_peers(struct net_if *iface, const struct wireguard_policy_peer *set,
			    size_t count, wireguard_policy_input_t allow_input, void *context)
{
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	struct wg_iface_context *ctx = get_wg_iface_context(iface);
	int ret;

	if (ctx == NULL || allow_input == NULL || (count != 0U && set == NULL)) {
		return -EINVAL;
	}
	if (count > CONFIG_WIREGUARD_MAX_PEER) {
		return -ENOSPC;
	}
	if (!atomic_cas(&wg_ctx.lifecycle_busy, 0, 1)) {
		return -EBUSY;
	}

	k_mutex_lock(&lock, K_FOREVER);
	ret = policy_replace(ctx, set, count, allow_input, context);
	k_mutex_unlock(&lock);
	atomic_clear(&wg_ctx.lifecycle_busy);

	return ret;
#else
	return -ENOTSUP;
#endif
}

int wireguard_release_peers(struct net_if *iface)
{
#if defined(CONFIG_WIREGUARD_PEER_POLICY)
	struct wg_iface_context *ctx = get_wg_iface_context(iface);
	k_spinlock_key_t key;
	int ret;

	if (ctx == NULL) {
		return -EINVAL;
	}
	k_mutex_lock(&lock, K_FOREVER);
	if (ctx->policy_input == NULL) {
		atomic_val_t state = atomic_get(&ctx->peer_state);

		if (state == WG_PEER_IFACE_ADDING || state == WG_PEER_IFACE_RETIRING) {
			ret = -EBUSY;
		} else if (state == WG_PEER_IFACE_FREE && ctx->attached_to == NULL &&
			   ctx->private_key_id != PSA_KEY_ID_NULL) {
			wg_cleanup_interface_keys(ctx);
			ret = 0;
		} else {
			ret = -ENOENT;
		}
		k_mutex_unlock(&lock);
		return ret;
	}
	if (!atomic_cas(&wg_ctx.lifecycle_busy, 0, 1)) {
		k_mutex_unlock(&lock);
		return -EBUSY;
	}
	ret = policy_replace(ctx, NULL, 0, ctx->policy_input, ctx->policy_context);
	if (ret < 0) {
		goto out;
	}

	atomic_set(&ctx->peer_state, WG_PEER_IFACE_RETIRING);
	k_mutex_unlock(&lock);
	ret = net_if_down(iface);
	if (ret == 0 || ret == -EALREADY) {
		ret = net_virtual_interface_attach(iface, NULL);
	}
	k_mutex_lock(&lock, K_FOREVER);
	if (ret < 0 && ret != -EALREADY) {
		atomic_set(&ctx->peer_state, WG_PEER_IFACE_ACTIVE);
		goto out;
	}

	atomic_dec(&wg_ctx.attached_peers);
	wg_cleanup_interface_keys(ctx);
	ctx->policy_context = NULL;
	key = k_spin_lock(&tx_routes_lock);
	ctx->policy_input = NULL;
	k_spin_unlock(&tx_routes_lock, key);
	release_peer_iface(ctx);
	ret = 0;

out:
	k_mutex_unlock(&lock);
	atomic_clear(&wg_ctx.lifecycle_busy);
	return ret;
#else
	return -ENOTSUP;
#endif
}
