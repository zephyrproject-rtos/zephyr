/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(conn_mgr_conn, CONFIG_NET_CONNECTION_MANAGER_LOG_LEVEL);

#include <zephyr/net/net_if.h>
#include <zephyr/net/conn_mgr_connectivity.h>

/**
 * @brief Retrieves the conn_mgr binding struct for a provided iface if it exists.
 *
 * Bindings for connectivity implementations with missing API structs are ignored.
 *
 * @param iface - bound network interface to obtain the binding struct for.
 * @return struct conn_mgr_conn_binding* Pointer to the retrieved binding struct if it exists,
 *	   NULL otherwise.
 */
static inline struct conn_mgr_conn_binding *conn_mgr_if_get_binding(struct net_if *iface)
{
	STRUCT_SECTION_FOREACH(conn_mgr_conn_binding, binding) {
		if (iface == binding->iface) {
			if (binding->impl->api) {
				return binding;
			}
			return NULL;
		}
	}
	return NULL;
}

int conn_mgr_if_connect(struct net_if *iface)
{
	struct conn_mgr_conn_binding *binding;
	struct conn_mgr_conn_api *api;
	int status;

	LOG_DBG("iface %p connect", iface);

	binding = conn_mgr_if_get_binding(iface);
	if (!binding) {
		return -ENOTSUP;
	}

	api = binding->impl->api;
	if (!api->connect) {
		return -ENOTSUP;
	}

	k_mutex_lock(binding->mutex, K_FOREVER);

	if (!net_if_flag_is_set(iface, NET_IF_UP)) {
		status = -ESHUTDOWN;
		goto out;
	}

	status = api->connect(binding);

out:
	k_mutex_unlock(binding->mutex);

	return status;
}

int conn_mgr_if_disconnect(struct net_if *iface)
{
	struct conn_mgr_conn_binding *binding;
	struct conn_mgr_conn_api *api;
	int status;

	LOG_DBG("iface %p disconnect", iface);

	binding = conn_mgr_if_get_binding(iface);
	if (!binding) {
		return -ENOTSUP;
	}

	api = binding->impl->api;
	if (!api->disconnect) {
		return -ENOTSUP;
	}


	k_mutex_lock(binding->mutex, K_FOREVER);

	if (!net_if_flag_is_set(iface, NET_IF_UP)) {
		status = -EALREADY;
		goto out;
	}

	status = api->disconnect(binding);

out:
	k_mutex_unlock(binding->mutex);

	return status;
}

bool conn_mgr_if_is_bound(struct net_if *iface)
{
	struct conn_mgr_conn_binding *binding = conn_mgr_if_get_binding(iface);

	return binding != NULL;
}

int conn_mgr_if_get_opt(struct net_if *iface, int optname, void *optval, size_t *optlen)
{
	struct conn_mgr_conn_binding *binding;
	struct conn_mgr_conn_api *api;
	int status;

	if (!optlen) {
		return -EINVAL;
	}

	if (!optval) {
		*optlen = 0;
		return -EINVAL;
	}

	binding = conn_mgr_if_get_binding(iface);
	if (!binding) {
		*optlen = 0;
		return -ENOTSUP;
	}

	api = binding->impl->api;
	if (!api->get_opt) {
		*optlen = 0;
		return -ENOTSUP;
	}

	k_mutex_lock(binding->mutex, K_FOREVER);

	status = api->get_opt(binding, optname, optval, optlen);

	k_mutex_unlock(binding->mutex);

	return status;
}

int conn_mgr_if_set_opt(struct net_if *iface, int optname, const void *optval, size_t optlen)
{
	struct conn_mgr_conn_binding *binding;
	struct conn_mgr_conn_api *api;
	int status;

	if (!optval) {
		return -EINVAL;
	}

	binding = conn_mgr_if_get_binding(iface);
	if (!binding) {
		return -ENOTSUP;
	}

	api = binding->impl->api;
	if (!api->set_opt) {
		return -ENOTSUP;
	}

	k_mutex_lock(binding->mutex, K_FOREVER);

	status = api->set_opt(binding, optname, optval, optlen);

	k_mutex_unlock(binding->mutex);

	return status;
}

int conn_mgr_if_set_flag(struct net_if *iface, enum conn_mgr_if_flag flag, bool value)
{
	struct conn_mgr_conn_binding *binding;

	if (flag >= CONN_MGR_NUM_IF_FLAGS) {
		return -EINVAL;
	}

	binding = conn_mgr_if_get_binding(iface);
	if (!binding) {
		return -ENOTSUP;
	}

	k_mutex_lock(binding->mutex, K_FOREVER);

	binding->flags &= ~BIT(flag);
	if (value) {
		binding->flags |= BIT(flag);
	}

	k_mutex_unlock(binding->mutex);

	return 0;
}

bool conn_mgr_if_get_flag(struct net_if *iface, enum conn_mgr_if_flag flag)
{
	struct conn_mgr_conn_binding *binding;
	bool value;

	if (flag >= CONN_MGR_NUM_IF_FLAGS) {
		return false;
	}

	binding = conn_mgr_if_get_binding(iface);
	if (!binding) {
		return false;
	}

	k_mutex_lock(binding->mutex, K_FOREVER);

	value = !!(binding->flags & BIT(flag));

	k_mutex_unlock(binding->mutex);

	return value;
}

int conn_mgr_if_get_timeout(struct net_if *iface)
{
	struct conn_mgr_conn_binding *binding = conn_mgr_if_get_binding(iface);
	int value;

	if (!binding) {
		return false;
	}

	k_mutex_lock(binding->mutex, K_FOREVER);

	value = binding->timeout;

	k_mutex_unlock(binding->mutex);

	return value;
}

int conn_mgr_if_set_timeout(struct net_if *iface, int timeout)
{
	struct conn_mgr_conn_binding *binding = conn_mgr_if_get_binding(iface);

	if (!binding) {
		return -ENOTSUP;
	}

	k_mutex_lock(binding->mutex, K_FOREVER);

	binding->timeout = timeout;

	k_mutex_unlock(binding->mutex);

	return 0;
}

void conn_mgr_conn_init(void)
{
	STRUCT_SECTION_FOREACH(conn_mgr_conn_binding, binding) {
		if (!(binding->impl->api)) {
			LOG_ERR("Connectivity implementation has NULL API, and will be treated as "
				"non-existent.");
		} else if (binding->impl->api->init) {
			k_mutex_lock(binding->mutex, K_FOREVER);

			/* Set initial default values for binding state */

			binding->timeout = CONN_MGR_IF_NO_TIMEOUT;

			/* Call binding initializer */

			binding->impl->api->init(binding);

			k_mutex_unlock(binding->mutex);
		}
	}
}
