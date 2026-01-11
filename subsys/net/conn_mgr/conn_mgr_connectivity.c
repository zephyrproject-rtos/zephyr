/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(conn_mgr_conn, CONFIG_NET_CONNECTION_MANAGER_LOG_LEVEL);

#include <zephyr/net/net_if.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/net/conn_mgr_monitor.h>
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_connectivity_impl.h>
#include "conn_mgr_private.h"

int conn_mgr_if_connect(struct net_if *iface)
{
	struct conn_mgr_conn_binding *binding;
	struct conn_mgr_conn_api *api;
	int status;

	LOG_DBG("iface %p connect", iface);

	binding = conn_mgr_if_get_binding(iface);
	if (binding == NULL) {
		return -ENOTSUP;
	}

	api = binding->impl->api;
	if (!api->connect) {
		return -ENOTSUP;
	}

	conn_mgr_binding_lock(binding);

	if (!net_if_is_admin_up(iface)) {
		status = net_if_up(iface);
		if (status) {
			goto out;
		}
	}

	conn_mgr_if_set_flag(iface, CONN_MGR_IF_DISCONNECTING, false);

	status = api->connect(binding);

out:
	conn_mgr_binding_unlock(binding);

	return status;
}

static int conn_mgr_if_disconnect_internal(struct net_if *iface, bool idle_timeout)
{
	struct conn_mgr_conn_binding *binding;
	struct conn_mgr_conn_api *api;
	int status = 0;

	LOG_DBG("iface %p disconnect", iface);

	binding = conn_mgr_if_get_binding(iface);
	if (binding == NULL) {
		return -ENOTSUP;
	}

	api = binding->impl->api;
	if (!api->disconnect) {
		return -ENOTSUP;
	}

	conn_mgr_binding_lock(binding);

	if (!net_if_is_admin_up(iface)) {
		goto out;
	}

	if (!idle_timeout) {
		conn_mgr_if_set_flag(iface, CONN_MGR_IF_DISCONNECTING, true);
	}

	status = api->disconnect(binding);

out:
	conn_mgr_binding_unlock(binding);

	return status;
}

int conn_mgr_if_disconnect(struct net_if *iface)
{
	return conn_mgr_if_disconnect_internal(iface, false);
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
	if (binding == NULL) {
		*optlen = 0;
		return -ENOTSUP;
	}

	api = binding->impl->api;
	if (!api->get_opt) {
		*optlen = 0;
		return -ENOTSUP;
	}

	conn_mgr_binding_lock(binding);

	status = api->get_opt(binding, optname, optval, optlen);

	conn_mgr_binding_unlock(binding);

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
	if (binding == NULL) {
		return -ENOTSUP;
	}

	api = binding->impl->api;
	if (!api->set_opt) {
		return -ENOTSUP;
	}

	conn_mgr_binding_lock(binding);

	status = api->set_opt(binding, optname, optval, optlen);

	conn_mgr_binding_unlock(binding);

	return status;
}

int conn_mgr_if_set_flag(struct net_if *iface, enum conn_mgr_if_flag flag, bool value)
{
	struct conn_mgr_conn_binding *binding;

	if (flag >= CONN_MGR_NUM_IF_FLAGS) {
		return -EINVAL;
	}

	binding = conn_mgr_if_get_binding(iface);
	if (binding == NULL) {
		return -ENOTSUP;
	}

	conn_mgr_binding_set_flag(binding, flag, value);

	return 0;
}

bool conn_mgr_if_get_flag(struct net_if *iface, enum conn_mgr_if_flag flag)
{
	struct conn_mgr_conn_binding *binding;

	if (flag >= CONN_MGR_NUM_IF_FLAGS) {
		return false;
	}

	binding = conn_mgr_if_get_binding(iface);
	if (binding == NULL) {
		return false;
	}

	return conn_mgr_binding_get_flag(binding, flag);
}

int conn_mgr_if_get_timeout(struct net_if *iface)
{
	struct conn_mgr_conn_binding *binding = conn_mgr_if_get_binding(iface);
	int value;

	if (binding == NULL) {
		return CONN_MGR_IF_NO_TIMEOUT;
	}

	conn_mgr_binding_lock(binding);

	value = binding->timeout;

	conn_mgr_binding_unlock(binding);

	return value;
}

int conn_mgr_if_set_timeout(struct net_if *iface, int timeout)
{
	struct conn_mgr_conn_binding *binding = conn_mgr_if_get_binding(iface);

	if (binding == NULL) {
		return -ENOTSUP;
	}

	conn_mgr_binding_lock(binding);

	binding->timeout = timeout;

	conn_mgr_binding_unlock(binding);

	return 0;
}

int conn_mgr_if_get_idle_timeout(struct net_if *iface)
{
	struct conn_mgr_conn_binding *binding = conn_mgr_if_get_binding(iface);
	int value;

	if (binding == NULL) {
		return CONN_MGR_IF_NO_TIMEOUT;
	}

	conn_mgr_binding_lock(binding);

	value = binding->idle_timeout;

	conn_mgr_binding_unlock(binding);

	return value;
}

int conn_mgr_if_set_idle_timeout(struct net_if *iface, int timeout)
{
	struct conn_mgr_conn_binding *binding = conn_mgr_if_get_binding(iface);

	if (binding == NULL) {
		return -ENOTSUP;
	}

	conn_mgr_binding_lock(binding);

	binding->idle_timeout = timeout;

	conn_mgr_binding_unlock(binding);

	return 0;
}

void conn_mgr_if_used(struct net_if *iface)
{
	struct conn_mgr_conn_binding *binding = conn_mgr_if_get_binding(iface);

	if (binding == NULL) {
		return;
	}
	if (binding->idle_timeout == CONN_MGR_IF_NO_TIMEOUT) {
		return;
	}
	k_work_reschedule(&binding->idle_worker, K_SECONDS(binding->idle_timeout));
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

/**
 * @brief Take the provided iface admin-down.
 *
 * Called automatically by conn_mgr when an iface has lost connection and will not attempt to
 * regain it.
 *
 * @param iface - The iface to take admin-down
 */
static void conn_mgr_conn_if_auto_admin_down(struct net_if *iface)
{
	struct conn_mgr_conn_binding *binding = conn_mgr_if_get_binding(iface);

	/* NOTE: This will be double-fired for ifaces that are both non-persistent
	 * and are being directly requested to disconnect, since both of these conditions
	 * separately trigger conn_mgr_conn_if_auto_admin_down.
	 *
	 * This is fine, because net_if_down is idempotent, but if you are adding other
	 * behaviors to this function, bear it in mind.
	 */

	/* Ignore ifaces that don't have connectivity implementations */
	if (binding == NULL) {
		return;
	}

	/* Cancel any pending idle timeouts */
	k_work_cancel_delayable(&binding->idle_worker);

	/* Take the iface admin-down if AUTO_DOWN is enabled */
	if (IS_ENABLED(CONFIG_NET_CONNECTION_MANAGER_AUTO_IF_DOWN) &&
	    !conn_mgr_if_get_flag(iface, CONN_MGR_IF_NO_AUTO_DOWN)) {
		net_if_down(iface);
	}
}

static void conn_mgr_conn_handle_iface_up(struct net_if *iface)
{
	struct conn_mgr_conn_binding *binding = conn_mgr_if_get_binding(iface);
	int idle_timeout;

	/* Ignore ifaces that don't have connectivity implementations */
	if (binding == NULL) {
		return;
	}

	idle_timeout = conn_mgr_if_get_idle_timeout(iface);
	if (idle_timeout == CONN_MGR_IF_NO_TIMEOUT) {
		/* No idle timeout configured for the interface */
		return;
	}

	/* Start the idle timeout */
	k_work_reschedule(&binding->idle_worker, K_SECONDS(idle_timeout));
}

/**
 * @brief Perform automated behaviors in response to any iface that loses oper-up state.
 *
 * This is how conn_mgr_conn automatically takes such ifaces admin-down if they are not persistent.
 *
 * @param iface - The iface which lost oper-up state.
 */
static void conn_mgr_conn_handle_iface_down(struct net_if *iface)
{
	/* Ignore ifaces that don't have connectivity implementations */
	if (!conn_mgr_if_is_bound(iface)) {
		return;
	}

	/* If the iface is persistent, we expect it to try to reconnect, unless
	 * disconnect was explicitly initiated by the application.
	 */
	if (conn_mgr_if_get_flag(iface, CONN_MGR_IF_PERSISTENT) &&
	    !conn_mgr_if_get_flag(iface, CONN_MGR_IF_DISCONNECTING)) {
		return;
	}

	conn_mgr_if_set_flag(iface, CONN_MGR_IF_DISCONNECTING, false);

	/* Otherwise, we do not expect the iface to reconnect, and we should call
	 * conn_mgr_conn_if_auto_admin_down
	 */
	conn_mgr_conn_if_auto_admin_down(iface);
}

static struct net_mgmt_event_callback conn_mgr_conn_iface_cb;
static void conn_mgr_conn_iface_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
					struct net_if *iface)
{
	if ((mgmt_event & CONN_MGR_CONN_IFACE_EVENTS_MASK) != mgmt_event) {
		return;
	}

	switch (mgmt_event) {
	case NET_EVENT_IF_UP:
		conn_mgr_conn_handle_iface_up(iface);
		break;
	case NET_EVENT_IF_DOWN:
		conn_mgr_conn_handle_iface_down(iface);
		break;
	case NET_EVENT_IF_ADMIN_UP:
		conn_mgr_conn_handle_iface_admin_up(iface);
		break;
	}
}

static struct net_mgmt_event_callback conn_mgr_conn_self_cb;
static void conn_mgr_conn_self_handler(struct net_mgmt_event_callback *cb, uint64_t mgmt_event,
				       struct net_if *iface)
{
	if ((mgmt_event & CONN_MGR_CONN_SELF_EVENTS_MASK) != mgmt_event) {
		return;
	}

	switch (NET_MGMT_GET_COMMAND(mgmt_event)) {
	case NET_EVENT_CONN_CMD_IF_FATAL_ERROR:
		if (cb->info) {
			NET_ERR("Fatal connectivity error on iface %d (%p). Reason: %d.",
				net_if_get_by_iface(iface), iface, *((int *)cb->info)
			);
		} else {
			NET_ERR("Unknown fatal connectivity error on iface %d (%p).",
				net_if_get_by_iface(iface), iface
			);
		}
	__fallthrough;
	case NET_EVENT_CONN_CMD_IF_TIMEOUT:
		/* If a timeout or fatal error occurs, we do not expect the iface to try to
		 * reconnect, so call conn_mgr_conn_if_auto_admin_down.
		 */
		conn_mgr_conn_if_auto_admin_down(iface);
		break;
	case NET_EVENT_CONN_CMD_IF_IDLE_TIMEOUT:
		/* Interface is idle, disconnect */
		LOG_DBG("iface %d (%p) idle", net_if_get_by_iface(iface), iface);
		conn_mgr_if_disconnect_internal(iface, true);
		break;
	}
}

static void conn_mgr_iface_idle_fn(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct conn_mgr_conn_binding *binding =
		CONTAINER_OF(dwork, struct conn_mgr_conn_binding, idle_worker);

	LOG_DBG("iface %d (%p) idle", net_if_get_by_iface(binding->iface), binding->iface);
	net_mgmt_event_notify(NET_EVENT_CONN_IF_IDLE_TIMEOUT, binding->iface);
}

void conn_mgr_conn_init(void)
{
	/* Initialize connectivity bindings. */
	STRUCT_SECTION_FOREACH(conn_mgr_conn_binding, binding) {
		if (!(binding->impl->api)) {
			LOG_ERR("Connectivity implementation has NULL API, and will be treated as "
				"non-existent.");
		} else if (binding->impl->api->init) {
			conn_mgr_binding_lock(binding);

			/* Initialise idle worker */

			k_work_init_delayable(&binding->idle_worker, conn_mgr_iface_idle_fn);

			/* Set initial default values for binding state */

			binding->timeout = CONN_MGR_IF_NO_TIMEOUT;
			binding->idle_timeout = CONN_MGR_IF_NO_TIMEOUT;

			/* Call binding initializer */

			binding->impl->api->init(binding);

			conn_mgr_binding_unlock(binding);
		}
	}

	/* Set up event listeners for automated behaviors */
	net_mgmt_init_event_callback(&conn_mgr_conn_iface_cb, conn_mgr_conn_iface_handler,
				     CONN_MGR_CONN_IFACE_EVENTS_MASK);
	net_mgmt_add_event_callback(&conn_mgr_conn_iface_cb);

	net_mgmt_init_event_callback(&conn_mgr_conn_self_cb, conn_mgr_conn_self_handler,
				     CONN_MGR_CONN_SELF_EVENTS_MASK);
	net_mgmt_add_event_callback(&conn_mgr_conn_self_cb);

	/* Trigger any initial automated behaviors for ifaces */
	STRUCT_SECTION_FOREACH(conn_mgr_conn_binding, binding) {
		if (binding->impl->api) {
			/* We need to fire conn_mgr_conn_handle_iface_admin_up for any
			 * (connectivity-enabled) ifaces that went admin-up before we registered
			 * the event callback that typically handles this.
			 */
			if (net_if_is_admin_up(binding->iface)) {
				conn_mgr_conn_handle_iface_admin_up(binding->iface);
			}
		}
	}
}

enum conn_mgr_conn_all_if_oper {
	ALL_IF_UP,
	ALL_IF_DOWN,
	ALL_IF_CONNECT,
	ALL_IF_DISCONNECT
};

struct conn_mgr_conn_all_if_ctx {
	bool skip_ignored;
	enum conn_mgr_conn_all_if_oper operation;
	int status;
};

/* Per-iface callback for conn_mgr_conn_all_if_up */
static void conn_mgr_conn_all_if_cb(struct net_if *iface, void *user_data)
{
	int status = 0;
	struct conn_mgr_conn_all_if_ctx *context = (struct conn_mgr_conn_all_if_ctx *)user_data;

	/* Skip ignored ifaces if so desired */
	if (context->skip_ignored && conn_mgr_is_iface_ignored(iface)) {
		return;
	}

	/* Perform the requested operation */
	switch (context->operation) {
	case ALL_IF_UP:
		/* Do not take iface admin up if it already is. */
		if (net_if_is_admin_up(iface)) {
			return;
		}

		status = net_if_up(iface);
		break;
	case ALL_IF_DOWN:
		/* Do not take iface admin down if it already is. */
		if (!net_if_is_admin_up(iface)) {
			return;
		}

		status = net_if_down(iface);
		break;
	case ALL_IF_CONNECT:
		/* Connect operation only supported if iface is bound */
		if (!conn_mgr_if_is_bound(iface)) {
			return;
		}

		status = conn_mgr_if_connect(iface);
		break;
	case ALL_IF_DISCONNECT:
		/* Disconnect operation only supported if iface is bound */
		if (!conn_mgr_if_is_bound(iface)) {
			return;
		}

		status = conn_mgr_if_disconnect(iface);
		break;
	}

	if (status == 0) {
		return;
	}

	if (context->status == 0) {
		context->status = status;
	}

	NET_ERR("%s failed for iface %d (%p). Error: %d",
		context->operation == ALL_IF_UP ?	  "net_if_up" :
		context->operation == ALL_IF_DOWN ?	  "net_if_down" :
		context->operation == ALL_IF_CONNECT ?	  "conn_mgr_if_connect" :
		context->operation == ALL_IF_DISCONNECT ? "conn_mgr_if_disconnect" :
							  "invalid",
		net_if_get_by_iface(iface), iface, status
	);
}

int conn_mgr_all_if_up(bool skip_ignored)
{
	struct conn_mgr_conn_all_if_ctx context = {
		.operation = ALL_IF_UP,
		.skip_ignored = skip_ignored,
		.status = 0
	};

	net_if_foreach(conn_mgr_conn_all_if_cb, &context);

	return context.status;
}

int conn_mgr_all_if_down(bool skip_ignored)
{
	struct conn_mgr_conn_all_if_ctx context = {
		.operation = ALL_IF_DOWN,
		.skip_ignored = skip_ignored,
		.status = 0
	};

	net_if_foreach(conn_mgr_conn_all_if_cb, &context);

	return context.status;
}

int conn_mgr_all_if_connect(bool skip_ignored)
{
	/* First, take all ifaces up.
	 * All bound ifaces will do this automatically when connect is called, but non-bound ifaces
	 * won't, so we must request it explicitly.
	 */
	struct conn_mgr_conn_all_if_ctx context = {
		.operation = ALL_IF_UP,
		.skip_ignored = skip_ignored,
		.status = 0
	};

	net_if_foreach(conn_mgr_conn_all_if_cb, &context);

	/* Now connect all ifaces.
	 * We are delibarately not resetting context.status between these two calls so that
	 * the first nonzero status code encountered between the two of them is what is returned.
	 */
	context.operation = ALL_IF_CONNECT;
	net_if_foreach(conn_mgr_conn_all_if_cb, &context);

	return context.status;
}

int conn_mgr_all_if_disconnect(bool skip_ignored)
{
	struct conn_mgr_conn_all_if_ctx context = {
		.operation = ALL_IF_DISCONNECT,
		.skip_ignored = skip_ignored,
		.status = 0
	};

	net_if_foreach(conn_mgr_conn_all_if_cb, &context);

	return context.status;
}
