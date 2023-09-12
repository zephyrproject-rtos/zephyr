/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rs9116w_ble_conn.h"

#include <zephyr/drivers/bluetooth/rs9116w.h>

/* Peripheral timeout to initialize Connection Parameter Update procedure */
#define CONN_UPDATE_TIMEOUT K_MSEC(CONFIG_BT_CONN_PARAM_UPDATE_TIMEOUT)

#define ENC_KEY_DEFAULT_SIZE 16

uint16_t conn_mtu[CONFIG_BT_MAX_CONN];

/**
 * @brief Convert a bt_conn_state_t enum to a string representation
 *
 * @param state
 * @return const char* String representation of the state
 */
static inline const char *state2str(enum bt_conn_state_t state)
{
	switch (state) {
	case BT_CONN_DISCONNECTED:
		return "disconnected";
	case BT_CONN_DISCONNECT_COMPLETE:
		return "disconnect-complete";
	case BT_CONN_CONNECT_SCAN:
		return "connect-scan";
	case BT_CONN_CONNECT_DIR_ADV:
		return "connect-dir-adv";
	case BT_CONN_CONNECT_ADV:
		return "connect-adv";
	case BT_CONN_CONNECT_AUTO:
		return "connect-auto";
	case BT_CONN_CONNECT:
		return "connect";
	case BT_CONN_CONNECTED:
		return "connected";
	case BT_CONN_DISCONNECT:
		return "disconnect";
	default:
		return "(unknown)";
	}
}

static struct bt_conn acl_conns[CONFIG_BT_MAX_CONN];
static struct bt_conn_cb *callback_list;

#if defined(CONFIG_BT_SMP)
#include "rs9116w_ble_smp.h"
const struct bt_conn_auth_cb *bt_auth;
sys_slist_t bt_auth_info_cbs = SYS_SLIST_STATIC_INIT(&bt_auth_info_cbs);
#endif /* CONFIG_BT_SMP */

struct bt_conn *bt_conn_ref(struct bt_conn *conn)
{
	atomic_val_t old;

	/* Reference counter must be checked to avoid incrementing ref from
	 * zero, then we should return NULL instead.
	 * Loop on clear-and-set in case someone has modified the reference
	 * count since the read, and start over again when that happens.
	 */
	do {
		old = atomic_get(&conn->ref);

		if (!old) {
			return NULL;
		}
	} while (!atomic_cas(&conn->ref, old, old + 1));

	BT_DBG("handle %u ref %u -> %u", conn->handle, (uint32_t)old,
	       (uint32_t)(old + 1));

	return conn;
}

bool bt_conn_is_peer_addr_le(const struct bt_conn *conn, uint8_t id,
			     const bt_addr_le_t *peer)
{
	ARG_UNUSED(id);

	if (!memcmp(peer->a.val, conn->le.dst.a.val, 6)) {
		return true;
	}

	/* Check against initial connection address */
	if (conn->role == BT_HCI_ROLE_MASTER) {
		return memcmp(peer->a.val, conn->le.resp_addr.a.val, 6) == 0;
	}

	return memcmp(peer->a.val, conn->le.init_addr.a.val, 6) == 0;
}

void bt_conn_unref(struct bt_conn *conn)
{
	atomic_val_t old;

	old = atomic_dec(&conn->ref);

	BT_DBG("handle %u ref %u -> %u", conn->handle, (uint32_t)old,
	       (uint32_t)atomic_get(&conn->ref));

	__ASSERT(old > 0, "Conn reference counter is 0");

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) && conn->type == BT_CONN_TYPE_LE &&
	    atomic_get(&conn->ref) == 0) {
		bt_le_adv_resume();
	}
}

struct bt_conn *bt_conn_lookup_addr_le(uint8_t id, const bt_addr_le_t *peer)
{
	uint64_t addr_key;

	memcpy(&addr_key, peer->a.val, 6);
	addr_key %= CONFIG_BT_MAX_CONN;
	uint16_t target = addr_key;

	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {

		struct bt_conn *conn = bt_conn_ref(&acl_conns[target]);

		if (!conn) {
			continue;
		}

		if (conn->type != BT_CONN_TYPE_LE) {
			bt_conn_unref(conn);
			continue;
		}

		if (!bt_conn_is_peer_addr_le(conn, id, peer)) {
			bt_conn_unref(conn);
			continue;
		}

		return conn;
	}

	return NULL;
}

struct bt_conn *bt_conn_new(struct bt_conn *conns, size_t size)
{
	struct bt_conn *conn = NULL;
	int i;

	for (i = 0; i < size; i++) {
		if (atomic_cas(&conns[i].ref, 0, 1)) {
			conn = &conns[i];
			break;
		}
	}

	if (!conn) {
		return NULL;
	}

	(void)memset(conn, 0, offsetof(struct bt_conn, ref));
	conn->handle = i;

	return conn;
}

/**
 * @brief Allocate net connection
 *
 * @return New connection or NULL in case of full buffer
 */
static struct bt_conn *acl_conn_new(void)
{
	return bt_conn_new(acl_conns, ARRAY_SIZE(acl_conns));
}

struct bt_conn *bt_conn_add_le(uint8_t id, const bt_addr_le_t *peer)
{
	struct bt_conn *conn = acl_conn_new();

	if (!conn) {
		return NULL;
	}

	conn->id = id;
	bt_addr_le_copy(&conn->le.dst, peer);

#if defined(CONFIG_BT_SMP)
	conn->sec_level = BT_SECURITY_L1;
	conn->required_sec_level = BT_SECURITY_L1;
#endif /* CONFIG_BT_SMP */

	conn->type = BT_CONN_TYPE_LE;
	conn->le.interval_min = BT_GAP_INIT_CONN_INT_MIN;
	conn->le.interval_max = BT_GAP_INIT_CONN_INT_MAX;

	return conn;
}

void bt_conn_cb_register(struct bt_conn_cb *cb)
{
	cb->_next = callback_list;
	callback_list = cb;
}

void notify_connected(struct bt_conn *conn)
{
	struct bt_conn_cb *cb;

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->connected) {
			cb->connected(conn, conn->err);
		}
	}

	if (!conn->err) {
		bt_gatt_connected(conn);
	}
}

void notify_disconnected(struct bt_conn *conn)
{
	struct bt_conn_cb *cb;

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->disconnected) {
			cb->disconnected(conn, conn->err);
		}
	}

	bt_gatt_disconnected(conn);
}

void bt_conn_set_state(struct bt_conn *conn, enum bt_conn_state_t state)
{
	enum bt_conn_state_t old_state;

	BT_DBG("%s -> %s", state2str(conn->state), state2str(state));

	if (conn->state == state) {
		BT_WARN("no transition %s", state2str(state));
		return;
	}

	old_state = conn->state;
	conn->state = state;

	/* Actions needed for exiting the old state */
	switch (old_state) {
	case BT_CONN_DISCONNECTED:
		/* Take a reference for the first state transition after
		 * bt_conn_add_le() and keep it until reaching DISCONNECTED
		 * again.
		 */
		if (conn->type != BT_CONN_TYPE_ISO) {
			bt_conn_ref(conn);
		}
		break;
	case BT_CONN_CONNECT:
		break;
	default:
		break;
	}

	/* Actions needed for entering the new state */
	switch (conn->state) {
	case BT_CONN_CONNECTED:
		k_fifo_init(&conn->tx_queue);
		sys_slist_init(&conn->channels);

		if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
		    conn->role == BT_CONN_ROLE_PERIPHERAL) {
		}

		break;
	case BT_CONN_DISCONNECTED:
		/* Notify disconnection and queue a dummy buffer to wake
		 * up and stop the tx thread for states where it was
		 * running.
		 */
		switch (old_state) {
		case BT_CONN_DISCONNECT_COMPLETE:
			atomic_set_bit(conn->flags, BT_CONN_CLEANUP);
			/* The last ref will be dropped during cleanup */
			break;
		case BT_CONN_CONNECT:
			/* LE Create Connection command failed. This might be
			 * directly from the API, don't notify application in
			 * this case.
			 */
			if (conn->err) {
				notify_connected(conn);
			}

			bt_conn_unref(conn);
			break;
		case BT_CONN_CONNECT_SCAN:
			/* this indicate LE Create Connection with peer address
			 * has been stopped. This could either be triggered by
			 * the application through bt_conn_disconnect or by
			 * timeout set by bt_conn_le_create_param.timeout.
			 */
			if (conn->err) {
				notify_connected(conn);
			}

			bt_conn_unref(conn);
			break;
		case BT_CONN_CONNECT_DIR_ADV:
			/* this indicate Directed advertising stopped */
			if (conn->err) {
				notify_connected(conn);
			}

			bt_conn_unref(conn);
			break;
		case BT_CONN_CONNECT_AUTO:
			/* this indicates LE Create Connection with filter
			 * policy has been stopped. This can only be triggered
			 * by the application, so don't notify.
			 */
			bt_conn_unref(conn);
			break;
		case BT_CONN_CONNECT_ADV:
			/* This can only happen when application stops the
			 * advertiser, conn->err is never set in this case.
			 */
			bt_conn_unref(conn);
			break;
		case BT_CONN_CONNECTED:
		case BT_CONN_DISCONNECT:
		case BT_CONN_DISCONNECTED:
			/* Cannot happen. */
			BT_WARN("Invalid (%u) old state", state);
			break;
		}
		break;
	case BT_CONN_CONNECT_AUTO:
		break;
	case BT_CONN_CONNECT_ADV:
		break;
	case BT_CONN_CONNECT_SCAN:
		break;
	case BT_CONN_CONNECT_DIR_ADV:
		break;
	case BT_CONN_CONNECT:
		if (conn->type == BT_CONN_TYPE_SCO) {
			break;
		}
		break;
	case BT_CONN_DISCONNECT:
		break;
	case BT_CONN_DISCONNECT_COMPLETE:
		break;
	default:
		BT_WARN("no valid (%u) state was set", state);

		break;
	}
}

bool bt_conn_exists_le(uint8_t id, const bt_addr_le_t *peer)
{
	struct bt_conn *conn = bt_conn_lookup_addr_le(id, peer);

	if (conn) {
		/* Connection object already exists.
		 * If the connection state is not "disconnected",then the
		 * connection was created but has not yet been disconnected.
		 * If the connection state is "disconnected" then the connection
		 * still has valid references. The last reference of the stack
		 * is released after the disconnected callback.
		 */
		BT_WARN("Found valid connection in %s state",
			state2str(conn->state));
		bt_conn_unref(conn);
		return true;
	}

	return false;
}

/**
 * @brief Initialized bluetooth connection handling
 *
 */
int bt_conn_init(void)
{
#if defined(CONFIG_BT_SMP)
	bt_smp_init();
#endif
	bt_gap_init();
	bt_gatt_init();

	return 0;
}

/**
 * @brief Cleanup disconnected/disconnecting connections
 *
 */
void rsi_connection_cleanup_task(void)
{
	for (int i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct bt_conn *conn = &acl_conns[i];

		if (conn->state == BT_CONN_DISCONNECT_COMPLETE) {
			bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
		}
		if (atomic_test_bit(conn->flags, BT_CONN_CLEANUP)) {
			if (atomic_get(&conn->ref) != 0) {
				atomic_set(&conn->ref, 1);
				bt_conn_unref(conn);
			}
			atomic_clear_bit(conn->flags, BT_CONN_CLEANUP);
		}
	}
}

int bt_conn_get_info(const struct bt_conn *conn, struct bt_conn_info *info)
{
	info->type = conn->type;
	info->role = conn->role;
	info->id = conn->id;

	switch (conn->type) {
	case BT_CONN_TYPE_LE:
		info->le.dst = &conn->le.dst;
		if (conn->role == BT_HCI_ROLE_MASTER) {
			info->le.local = &conn->le.init_addr;
			info->le.remote = &conn->le.resp_addr;
		} else {
			info->le.local = &conn->le.resp_addr;
			info->le.remote = &conn->le.init_addr;
		}
		info->le.interval = conn->le.interval;
		info->le.latency = conn->le.latency;
		info->le.timeout = conn->le.timeout;
	}

	return -EINVAL;
}

int conn_disconnect(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);
	int status = 0;

	if (conn->type == BT_CONN_TYPE_LE) {
		switch (conn->role) {
		case BT_CONN_ROLE_PERIPHERAL:
			status = rsi_ble_disconnect(conn->le.dst.a.val);
			break;
		case BT_CONN_ROLE_CENTRAL:
			break;
		}

		bt_conn_set_state(conn, BT_CONN_DISCONNECT_COMPLETE);
		notify_disconnected(conn);
		bt_conn_unref(conn);
	}

	return status;
}

int bt_conn_disconnect(struct bt_conn *conn, uint8_t reason)
{
	/* Disconnection is initiated by us, so auto connection shall
	 * be disabled. Otherwise the passive scan would be enabled
	 * and we could send LE Create Connection as soon as the remote
	 * starts advertising.
	 */
#if !defined(CONFIG_BT_WHITELIST)
	if (IS_ENABLED(CONFIG_BT_CENTRAL) && conn->type == BT_CONN_TYPE_LE) {
		bt_le_set_auto_conn(&conn->le.dst, NULL);
	}
#endif /* !defined(CONFIG_BT_WHITELIST) */

	switch (conn->state) {
	case BT_CONN_CONNECT_SCAN:
		conn->err = reason;
		bt_conn_set_state(conn, BT_CONN_DISCONNECTED);
		return 0;
	case BT_CONN_CONNECT:
		return 0;
	case BT_CONN_CONNECTED:
		return conn_disconnect(conn, reason);
	case BT_CONN_DISCONNECT:
		return 0;
	case BT_CONN_DISCONNECTED:
	default:
		return -ENOTCONN;
	}
}

int get_active_le_conns(void)
{
	int count = 0;

	for (int i = 0; i < ARRAY_SIZE(acl_conns); i++) {
		if (acl_conns[i].state == BT_CONN_CONNECTED &&
		    acl_conns[i].type == BT_CONN_TYPE_LE) {
			count++;
		}
	}
	return count;
}

struct bt_conn *get_acl_conn(int i)
{
	if (i < CONFIG_BT_MAX_CONN) {
		return &acl_conns[i];
	} else {
		return NULL;
	}
}

int bt_conn_get_remote_info(struct bt_conn *conn,
			    struct bt_conn_remote_info *remote_info)
{
	if (!atomic_test_bit(conn->flags, BT_CONN_AUTO_FEATURE_EXCH) ||
	    (IS_ENABLED(CONFIG_BT_REMOTE_VERSION) &&
	     !atomic_test_bit(conn->flags, BT_CONN_AUTO_VERSION_INFO))) {
		return -EBUSY;
	}

	remote_info->type = conn->type;
#if defined(CONFIG_BT_REMOTE_VERSION)
	/* The conn->rv values will be just zeroes if the operation failed */
	remote_info->version = conn->rv.version;
	remote_info->manufacturer = conn->rv.manufacturer;
	remote_info->subversion = conn->rv.subversion;
#else
	remote_info->version = 0;
	remote_info->manufacturer = 0;
	remote_info->subversion = 0;
#endif

	switch (conn->type) {
	case BT_CONN_TYPE_LE:
		remote_info->le.features = conn->le.features;
		return 0;
#if defined(CONFIG_BT_BREDR)
	case BT_CONN_TYPE_BR:
		/* TODO: Make sure the HCI commands to read br features and
		 *  extended features has finished.
		 */
		return -ENOTSUP;
#endif
	default:
		return -EINVAL;
	}
}

const bt_addr_le_t *bt_conn_get_dst(const struct bt_conn *conn)
{
	return &conn->le.dst;
}

uint8_t bt_conn_index(const struct bt_conn *conn)
{
	ptrdiff_t index;

	switch (conn->type) {
	default:
		index = conn - acl_conns;
		__ASSERT(index >= 0 && index < ARRAY_SIZE(acl_conns),
			 "Invalid bt_conn pointer");
		break;
	}

	return (uint8_t)index;
}

int bt_conn_le_get_tx_power_level(struct bt_conn *conn,
				  struct bt_conn_le_tx_power *tx_power_level)
{
	return -ENOTSUP;
}

int bt_conn_le_get_rssi(const struct bt_conn *conn)
{
	int8_t rssi;

	if (rsi_bt_get_rssi((uint8_t *)conn->le.dst.a.val, &rssi)) {
		return -EIO;
	} else {
		return -rssi;
	}
}

bool bt_le_conn_params_valid(const struct bt_le_conn_param *param)
{
	/* All limits according to BT Core spec 5.0 [Vol 2, Part E, 7.8.12] */

	if (param->interval_min > param->interval_max ||
	    param->interval_min < 6 || param->interval_max > 3200) {
		return false;
	}

	if (param->latency > 499) {
		return false;
	}

	if (param->timeout < 10 || param->timeout > 3200 ||
	    ((param->timeout * 4U) <=
	     ((1U + param->latency) * param->interval_max))) {
		return false;
	}

	return true;
}

static int send_conn_le_param_update(struct bt_conn *conn,
				     const struct bt_le_conn_param *param)
{
	BT_DBG("conn %p features 0x%02x params (%d-%d %d %d)", conn,
	       conn->le.features[0], param->interval_min, param->interval_max,
	       param->latency, param->timeout);

	/* Proceed only if connection parameters contains valid values*/
	if (!bt_le_conn_params_valid(param)) {
		return -EINVAL;
	}

	/* Use LE connection parameter request if both local and remote support
	 * it; or if local role is master then use LE connection update.
	 */
	int rc;

	rc = rsi_ble_conn_params_update(conn->le.dst.a.val, param->interval_min,
					param->interval_max, param->latency,
					param->timeout);

	/* store those in case of fallback to L2CAP */
	if (rc == 0) {
		conn->le.pending_latency = param->latency;
		conn->le.pending_timeout = param->timeout;
	}

	return rc;
}

int bt_conn_le_param_update(struct bt_conn *conn,
			    const struct bt_le_conn_param *param)
{
	BT_DBG("conn %p features 0x%02x params (%d-%d %d %d)", conn,
	       conn->le.features[0], param->interval_min, param->interval_max,
	       param->latency, param->timeout);

	/* Check if there's a need to update conn params */
	if (conn->le.interval >= param->interval_min &&
	    conn->le.interval <= param->interval_max &&
	    conn->le.latency == param->latency &&
	    conn->le.timeout == param->timeout) {
		atomic_clear_bit(conn->flags, BT_CONN_SLAVE_PARAM_SET);
		return -EALREADY;
	}

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		/* if slave conn param update timer expired just send request */
		int ret = send_conn_le_param_update(conn, param);

		if (ret != 0) {
			return ret;
		}

		/* store new conn params to be used by update timer */
		conn->le.interval_min = param->interval_min;
		conn->le.interval_max = param->interval_max;
		conn->le.pending_latency = param->latency;
		conn->le.pending_timeout = param->timeout;
		atomic_set_bit(conn->flags, BT_CONN_SLAVE_PARAM_SET);
	}

	return 0;
}

/* BLE SMP */
#if defined(CONFIG_BT_SMP)
uint8_t bt_conn_enc_key_size(const struct bt_conn *conn)
{
	return ENC_KEY_DEFAULT_SIZE;
}

static int start_security(struct bt_conn *conn)
{
	if (IS_ENABLED(CONFIG_BT_SMP)) {
		return bt_smp_start_security(conn);
	}

	return -EINVAL;
}

int bt_conn_set_security(struct bt_conn *conn,
			 bt_security_t sec) /* Should be fine */
{
	int err;

	if (conn->state != BT_CONN_CONNECTED) {
		return -ENOTCONN;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_SC_ONLY)) {
		sec = BT_SECURITY_L4;
	}

	if (IS_ENABLED(CONFIG_BT_SMP_OOB_LEGACY_PAIR_ONLY)) {
		sec = BT_SECURITY_L3;
	}

	/* nothing to do */
	if (conn->sec_level >= sec || conn->required_sec_level >= sec) {
		return 0;
	}

	atomic_set_bit_to(conn->flags, BT_CONN_FORCE_PAIR,
			  sec & BT_SECURITY_FORCE_PAIR);
	conn->required_sec_level = sec & ~BT_SECURITY_FORCE_PAIR;

	err = start_security(conn);

	/* reset required security level in case of error */
	if (err) {
		conn->required_sec_level = conn->sec_level;
	}

	return err;
}

bt_security_t bt_conn_get_security(const struct bt_conn *conn)
{
	return conn->sec_level;
}

void security_changed(struct bt_conn *conn, uint8_t status)
{
	struct bt_conn_cb *cb;

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->security_changed) {
			cb->security_changed(conn, conn->sec_level, status);
		}
	}
}
#else
bt_security_t bt_conn_get_security(const struct bt_conn *conn)
{
	return BT_SECURITY_L1;
}
#endif /* CONFIG_BT_SMP */

#if defined(CONFIG_BT_SMP)

int bt_conn_auth_cb_register(
	const struct bt_conn_auth_cb *cb) /* Should be fine */
{
	if (!cb) {
		bt_auth = NULL;
		return 0;
	}

	if (bt_auth) {
		return -EALREADY;
	}

	/* The cancel callback must always be provided if the app provides
	 * interactive callbacks.
	 */
	if (!cb->cancel && (cb->passkey_display || cb->passkey_entry ||
			    cb->passkey_confirm || cb->pairing_confirm)) {
		return -EINVAL;
	}

	bt_auth = cb;
	return 0;
}

int bt_conn_auth_info_cb_register(struct bt_conn_auth_info_cb *cb)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	sys_slist_append(&bt_auth_info_cbs, &cb->node);

	return 0;
}

int bt_conn_auth_info_cb_unregister(struct bt_conn_auth_info_cb *cb)
{
	if (cb == NULL) {
		return -EINVAL;
	}

	if (!sys_slist_find_and_remove(&bt_auth_info_cbs, &cb->node)) {
		return -EALREADY;
	}

	return 0;
}

int bt_conn_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey)
{ /* Todo */
	if (!bt_auth) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_SMP) && conn->type == BT_CONN_TYPE_LE) {
		bt_smp_auth_passkey_entry(conn, passkey);
		return 0;
	}

	return -EINVAL;
}

int bt_conn_auth_passkey_confirm(struct bt_conn *conn) /* Todo */
{
	if (!bt_auth) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_BT_SMP) && conn->type == BT_CONN_TYPE_LE) {
		return bt_smp_auth_passkey_confirm(conn);
	}

	return -EINVAL;
}

int bt_conn_auth_cancel(struct bt_conn *conn) /* Todo */ { return -ENOTSUP; }

int bt_conn_auth_pairing_confirm(struct bt_conn *conn) /* Todo */
{
	return -ENOTSUP;
}

void identity_resolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
		       const bt_addr_le_t *identity)
{
	struct bt_conn_cb *cb;

	for (cb = callback_list; cb; cb = cb->_next) {
		if (cb->security_changed) {
			cb->identity_resolved(conn, rpa, identity);
		}
	}
}
#endif /* CONFIG_BT_SMP */
