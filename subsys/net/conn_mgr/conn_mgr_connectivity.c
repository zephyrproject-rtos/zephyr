/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(conn_mgr_conn, CONFIG_NET_CONNECTION_MANAGER_LOG_LEVEL);

#include <zephyr/net/net_if.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/sys/iterable_sections.h>

#include "conn_mgr_private.h"

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

	if (!net_if_is_admin_up(iface)) {
		status = net_if_up(iface);
		if (status) {
			goto out;
		}
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
	int status = 0;

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

	if (!net_if_is_admin_up(iface)) {
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

/* Automated behavior handling */

/**
 * @brief Perform automated behaviors in response to ifaces going admin-up.
 *
 * @param iface - The iface which became admin-up.
 */
static void conn_mgr_conn_handle_iface_admin_up(struct net_if *iface)
{
	int err;

	/* Ignore ifaces that don't have connectivity implementations */
	if (!conn_mgr_if_is_bound(iface)) {
		return;
	}

	/* Ignore ifaces for which auto-connect is disabled */
	if (conn_mgr_if_get_flag(iface, CONN_MGR_IF_NO_AUTO_CONNECT)) {
		return;
	}

	/* Otherwise, automatically instruct the iface to connect */
	err = conn_mgr_if_connect(iface);
	if (err < 0) {
		NET_ERR("iface auto-connect failed: %d", err);
	}
}

static struct net_mgmt_event_callback conn_mgr_conn_iface_cb;
static void conn_mgr_conn_iface_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
					struct net_if *iface)
{
	if ((mgmt_event & CONN_MGR_CONN_IFACE_EVENTS_MASK) != mgmt_event) {
		return;
	}

	switch (mgmt_event) {
	case NET_EVENT_IF_UP:
		conn_mgr_conn_handle_iface_admin_up(iface);
		break;
	}
}

void conn_mgr_conn_init(void)
{
	/* Initialize connectivity bindings. */
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

	/* Set up event listeners for automated behaviors */
	net_mgmt_init_event_callback(&conn_mgr_conn_iface_cb, conn_mgr_conn_iface_handler,
				     CONN_MGR_CONN_IFACE_EVENTS_MASK);
	net_mgmt_add_event_callback(&conn_mgr_conn_iface_cb);

	/* Trigger any initial automated behaviors for ifaces */
	STRUCT_SECTION_FOREACH(conn_mgr_conn_binding, binding) {
		if (binding->impl->api) {
			/* We need to fire conn_mgr_conn_handle_iface_admin_up for any
			 * (connectivity-enabled) ifaces that went admin-up before we registerred
			 * the event callback that typically handles this.
			 */
			if (net_if_is_admin_up(binding->iface)) {
				conn_mgr_conn_handle_iface_admin_up(binding->iface);
			}
		}
	}
}
