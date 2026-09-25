/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Jeff Welder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Bluetooth GATT client transport for the MCUmgr SMP client.
 *
 * Discovers the SMP service on a connected peer, subscribes to its notifications, writes
 * outbound SMP packets as ATT_MTU-3 sized Write Without Response fragments and feeds the
 * notifications back through the reassembly context into the SMP core.
 */

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt_defines.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt_client.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>

#include <mgmt/mcumgr/transport/smp_internal.h>
#include <mgmt/mcumgr/transport/smp_reassembly.h>

LOG_MODULE_DECLARE(mcumgr_smp, CONFIG_MCUMGR_TRANSPORT_LOG_LEVEL);

/* ATT Write Command opcode and handle */
#define SMP_BT_CLIENT_ATT_OVERHEAD 3U

#define SMP_BT_CLIENT_TX_TIMEOUT K_MSEC(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDIT_TIMEOUT)

static const struct bt_uuid_128 smp_bt_client_svc_uuid = BT_UUID_INIT_128(SMP_BT_SVC_UUID_VAL);
static const struct bt_uuid_128 smp_bt_client_chr_uuid = BT_UUID_INIT_128(SMP_BT_CHR_UUID_VAL);
static const struct bt_uuid_16 smp_bt_client_ccc_uuid = BT_UUID_INIT_16(BT_UUID_GATT_CCC_VAL);

enum smp_bt_client_link_state {
	SMP_BT_CLIENT_IDLE = 0,
	SMP_BT_CLIENT_ATTACHING,
	SMP_BT_CLIENT_READY,
};

static struct smp_transport smp_bt_client_transport;
static struct smp_client_transport_entry smp_bt_client_entry = {
	.smpt = &smp_bt_client_transport,
	.smpt_type = SMP_BLUETOOTH_CLIENT_TRANSPORT,
#ifdef CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS
	.name = "Bluetooth client",
#endif
};

static atomic_t smp_bt_client_state = ATOMIC_INIT(SMP_BT_CLIENT_IDLE);

/* Incremented on every attach, so that a packet being written out can tell that the
 * target it latched has since been replaced.
 */
static atomic_t smp_bt_client_generation = ATOMIC_INIT(0);

/* Target connection and value handle, always read together under the lock */
static struct k_spinlock smp_bt_client_target_lock;
static struct bt_conn *smp_bt_client_target;
static uint16_t smp_bt_client_tx_handle;

struct smp_bt_client_link {
	struct bt_conn *conn;
	uint16_t value_handle;
	uint32_t generation;
};

/* Serializes attach and detach. Detach sets the cancel flag and wakes a pending attach,
 * then waits for it to unwind before tearing down.
 */
static K_MUTEX_DEFINE(smp_bt_client_api_lock);
static atomic_t smp_bt_client_cancel = ATOMIC_INIT(0);

/* Handshake between the GATT callbacks and a waiting attach */
static K_SEM_DEFINE(smp_bt_client_attach_sem, 0, 1);
static int smp_bt_client_attach_result;
static const char *smp_bt_client_attach_reason;
static atomic_t smp_bt_client_subscribing = ATOMIC_INIT(0);

/* Written only by the discovery callbacks */
static uint16_t smp_bt_client_svc_end_handle;
static uint16_t smp_bt_client_value_handle;
static uint16_t smp_bt_client_ccc_handle;

static struct bt_gatt_discover_params smp_bt_client_discover_params;

/* The host owns the subscription parameters while they are linked into its subscription
 * list, and while a CCC write carrying them is in flight, which the host tracks in
 * BT_GATT_SUBSCRIBE_FLAG_WRITE_PENDING. Attach only rewrites them when neither holds.
 */
static struct bt_gatt_subscribe_params smp_bt_client_sub_params;
static atomic_t smp_bt_client_sub_linked = ATOMIC_INIT(0);

/*
 * Every fragment takes a credit that the write completion callback returns, so the
 * transmit path never polls on the MCUmgr work queue. A credit is returned only after its
 * ATT buffer is back in the pool, so more credits than buffers would let a fragment block
 * inside the host's buffer allocation, which has no timeout on this thread.
 */
BUILD_ASSERT(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDITS <= CONFIG_BT_ATT_TX_COUNT,
	     "MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDITS must not exceed BT_ATT_TX_COUNT");

static K_SEM_DEFINE(smp_bt_client_tx_sem, CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDITS,
		    CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDITS);

/* The reassembly context has no locking of its own, and is used from the notify callback
 * and from teardown on the caller's thread.
 */
static K_MUTEX_DEFINE(smp_bt_client_reassembly_lock);

static bool smp_bt_client_conn_up(struct bt_conn *conn)
{
	struct bt_conn_info info;

	return bt_conn_get_info(conn, &info) == 0 && info.state == BT_CONN_STATE_CONNECTED;
}

static void smp_bt_client_tx_credits_replenish(void)
{
	/* k_sem_give() saturates at the limit */
	for (uint32_t i = 0U; i < CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDITS; i++) {
		k_sem_give(&smp_bt_client_tx_sem);
	}
}

static void smp_bt_client_tx_done(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(user_data);

	k_sem_give(&smp_bt_client_tx_sem);
}

static bool smp_bt_client_link_claim(struct smp_bt_client_link *link)
{
	bool claimed = false;
	k_spinlock_key_t key = k_spin_lock(&smp_bt_client_target_lock);

	if (smp_bt_client_target != NULL && smp_bt_client_tx_handle != 0U) {
		link->conn = bt_conn_ref(smp_bt_client_target);
		link->value_handle = smp_bt_client_tx_handle;
		link->generation = (uint32_t)atomic_get(&smp_bt_client_generation);
		claimed = true;
	}

	k_spin_unlock(&smp_bt_client_target_lock, key);

	return claimed;
}

static bool smp_bt_client_link_current(const struct smp_bt_client_link *link)
{
	return atomic_get(&smp_bt_client_state) == SMP_BT_CLIENT_READY &&
	       (uint32_t)atomic_get(&smp_bt_client_generation) == link->generation;
}

static struct bt_conn *smp_bt_client_conn_claim(void)
{
	struct bt_conn *conn = NULL;
	k_spinlock_key_t key = k_spin_lock(&smp_bt_client_target_lock);

	if (smp_bt_client_target != NULL) {
		conn = bt_conn_ref(smp_bt_client_target);
	}

	k_spin_unlock(&smp_bt_client_target_lock, key);

	return conn;
}

/* Publishes the value handle, unless a disconnect has already cleared the target */
static bool smp_bt_client_tx_handle_install(uint16_t handle)
{
	bool installed = false;
	k_spinlock_key_t key = k_spin_lock(&smp_bt_client_target_lock);

	if (smp_bt_client_target != NULL) {
		smp_bt_client_tx_handle = handle;
		installed = true;
	}

	k_spin_unlock(&smp_bt_client_target_lock, key);

	return installed;
}

static void smp_bt_client_target_clear(void)
{
	struct bt_conn *conn;
	k_spinlock_key_t key = k_spin_lock(&smp_bt_client_target_lock);

	conn = smp_bt_client_target;
	smp_bt_client_target = NULL;
	smp_bt_client_tx_handle = 0U;
	k_spin_unlock(&smp_bt_client_target_lock, key);

	if (conn != NULL) {
		bt_conn_unref(conn);
	}
}

static void smp_bt_client_reassembly_clear(void)
{
	k_mutex_lock(&smp_bt_client_reassembly_lock, K_FOREVER);
	(void)smp_reassembly_drop(&smp_bt_client_transport);
	k_mutex_unlock(&smp_bt_client_reassembly_lock);
}

/* The host unlinks the node before bt_gatt_unsubscribe() returns success */
static void smp_bt_client_unsubscribe(struct bt_conn *conn)
{
	int rc;

	if (atomic_get(&smp_bt_client_sub_linked) == 0) {
		return;
	}

	rc = bt_gatt_unsubscribe(conn, &smp_bt_client_sub_params);
	if (rc == 0) {
		atomic_clear(&smp_bt_client_sub_linked);
	} else {
		LOG_WRN("SMP client unsubscribe failed (err %d)", rc);
	}
}

/*
 * A packet that has not started may be dropped when no credit turns up. Once a fragment
 * is on the wire the peer holds half a packet and would misframe whatever comes next, so
 * a started packet keeps waiting until its link stops being the target.
 */
static bool smp_bt_client_tx_credit_take(const struct smp_bt_client_link *link, bool started)
{
	while (k_sem_take(&smp_bt_client_tx_sem, SMP_BT_CLIENT_TX_TIMEOUT) != 0) {
		if (!started) {
			LOG_WRN("SMP client transmit credit starved after %d ms",
				CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDIT_TIMEOUT);
			return false;
		}

		if (!smp_bt_client_link_current(link)) {
			LOG_WRN("SMP client target went away with a packet part written");
			return false;
		}
	}

	return true;
}

static int smp_bt_client_tx_frags(const struct smp_bt_client_link *link, struct net_buf *nb)
{
	uint16_t mtu = bt_gatt_get_mtu(link->conn);
	uint16_t frag_max;
	uint16_t off = 0U;

	if (mtu <= SMP_BT_CLIENT_ATT_OVERHEAD) {
		return MGMT_ERR_EUNKNOWN;
	}

	frag_max = mtu - SMP_BT_CLIENT_ATT_OVERHEAD;

	while (off < nb->len) {
		uint16_t len = MIN(frag_max, (uint16_t)(nb->len - off));
		int err;

		if (!smp_bt_client_tx_credit_take(link, off != 0U)) {
			return MGMT_ERR_EUNKNOWN;
		}

		if (!smp_bt_client_link_current(link)) {
			k_sem_give(&smp_bt_client_tx_sem);
			return MGMT_ERR_ENOENT;
		}

		err = bt_gatt_write_without_response_cb(link->conn, link->value_handle,
							&nb->data[off], len, false,
							smp_bt_client_tx_done, NULL);
		if (err != 0) {
			/* No completion callback for a write the host did not accept */
			k_sem_give(&smp_bt_client_tx_sem);
			LOG_WRN("SMP client fragment write failed (err %d, %u of %u bytes sent)",
				err, off, nb->len);

			return (err == -ENOMEM) ? MGMT_ERR_ENOMEM : MGMT_ERR_EUNKNOWN;
		}

		off += len;
	}

	return MGMT_ERR_EOK;
}

/* The SMP client keeps its own reference to @p nb for retries, so the buffer is only read
 * here, and exactly one reference is released on every path.
 */
static int smp_bt_client_tx_pkt(struct net_buf *nb)
{
	struct smp_bt_client_link link;
	int rc = MGMT_ERR_ENOENT;

	if (atomic_get(&smp_bt_client_state) == SMP_BT_CLIENT_READY &&
	    smp_bt_client_link_claim(&link)) {
		if (smp_bt_client_link_current(&link)) {
			rc = smp_bt_client_tx_frags(&link, nb);
		}

		bt_conn_unref(link.conn);
	}

	smp_packet_free(nb);

	return rc;
}

static uint16_t smp_bt_client_get_mtu(const struct net_buf *nb)
{
	struct smp_bt_client_link link;
	uint16_t mtu;

	ARG_UNUSED(nb);

	if (!smp_bt_client_link_claim(&link)) {
		return 0U;
	}

	mtu = bt_gatt_get_mtu(link.conn);
	bt_conn_unref(link.conn);

	if (mtu <= SMP_BT_CLIENT_ATT_OVERHEAD) {
		return 0U;
	}

	return mtu - SMP_BT_CLIENT_ATT_OVERHEAD;
}

static uint8_t smp_bt_client_notify_cb(struct bt_conn *conn,
				       struct bt_gatt_subscribe_params *params, const void *data,
				       uint16_t length)
{
	int ret;

	ARG_UNUSED(conn);
	ARG_UNUSED(params);

	if (data == NULL) {
		/* The host has removed the node from its subscription list */
		atomic_clear(&smp_bt_client_sub_linked);
		return BT_GATT_ITER_STOP;
	}

	if (length == 0U) {
		return BT_GATT_ITER_CONTINUE;
	}

	/* The state is tested under the lock, so a fragment cannot be collected after a
	 * teardown has dropped the context.
	 */
	k_mutex_lock(&smp_bt_client_reassembly_lock, K_FOREVER);

	if (atomic_get(&smp_bt_client_state) != SMP_BT_CLIENT_READY) {
		k_mutex_unlock(&smp_bt_client_reassembly_lock);
		return BT_GATT_ITER_CONTINUE;
	}

	ret = smp_reassembly_collect(&smp_bt_client_transport, data, length);

	if (ret == -ENOMEM) {
		LOG_WRN("SMP client reassembly buffer exhausted");
	} else if (ret < 0) {
		LOG_WRN("SMP client reassembly failed (%d), dropping packet", ret);
		(void)smp_reassembly_drop(&smp_bt_client_transport);
	} else if (ret == 0) {
		(void)smp_reassembly_complete(&smp_bt_client_transport, false);
	} else {
		/* More fragments expected */
	}

	k_mutex_unlock(&smp_bt_client_reassembly_lock);

	/* BT_GATT_ITER_STOP on data would unsubscribe */
	return BT_GATT_ITER_CONTINUE;
}

static void smp_bt_client_attach_signal(struct bt_conn *conn, int result, const char *reason)
{
	if (result != 0 && !smp_bt_client_conn_up(conn)) {
		result = -ENOTCONN;
		reason = NULL;
	}

	smp_bt_client_attach_result = result;
	smp_bt_client_attach_reason = reason;
	k_sem_give(&smp_bt_client_attach_sem);
}

static void smp_bt_client_subscribe_cb(struct bt_conn *conn, uint8_t err,
				       struct bt_gatt_subscribe_params *params)
{
	/* Only the response to the enabling write of a waiting attach is a verdict; the
	 * response to an unsubscribe's disabling write is not.
	 */
	if (params->value != BT_GATT_CCC_NOTIFY || atomic_get(&smp_bt_client_subscribing) == 0) {
		return;
	}

	smp_bt_client_attach_signal(conn, (err == 0U) ? 0 : -EIO,
				    (err == 0U) ? NULL : "subscription rejected by the peer");
}

static uint8_t smp_bt_client_ccc_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    struct bt_gatt_discover_params *params)
{
	ARG_UNUSED(params);

	if (attr == NULL) {
		smp_bt_client_attach_signal(conn, -ENOTSUP, "SMP characteristic has no CCC");
		return BT_GATT_ITER_STOP;
	}

	/* The host does not range check descriptor handles, and subscribing with a zero
	 * handle asserts.
	 */
	if (attr->handle <= smp_bt_client_value_handle ||
	    attr->handle > smp_bt_client_svc_end_handle) {
		smp_bt_client_attach_signal(conn, -ENOTSUP, "CCC handle outside the service");
		return BT_GATT_ITER_STOP;
	}

	smp_bt_client_ccc_handle = attr->handle;
	smp_bt_client_attach_signal(conn, 0, NULL);

	return BT_GATT_ITER_STOP;
}

static uint8_t smp_bt_client_chrc_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				     struct bt_gatt_discover_params *params)
{
	const uint8_t required = BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_NOTIFY;
	const struct bt_gatt_chrc *chrc;
	int err;

	if (attr == NULL) {
		smp_bt_client_attach_signal(conn, -ENOTSUP, "no SMP characteristic");
		return BT_GATT_ITER_STOP;
	}

	chrc = attr->user_data;

	if ((chrc->properties & required) != required) {
		smp_bt_client_attach_signal(conn, -ENOTSUP,
					    "SMP characteristic cannot notify and write");
		return BT_GATT_ITER_STOP;
	}

	smp_bt_client_value_handle = chrc->value_handle;

	if (smp_bt_client_value_handle == 0U ||
	    smp_bt_client_value_handle >= smp_bt_client_svc_end_handle) {
		smp_bt_client_attach_signal(conn, -ENOTSUP, "no room for a CCC in the service");
		return BT_GATT_ITER_STOP;
	}

	/* The SMP service has a single characteristic, so the first CCC after its value
	 * handle is its own.
	 */
	params->uuid = &smp_bt_client_ccc_uuid.uuid;
	params->func = smp_bt_client_ccc_cb;
	params->start_handle = smp_bt_client_value_handle + 1U;
	params->end_handle = smp_bt_client_svc_end_handle;
	params->type = BT_GATT_DISCOVER_DESCRIPTOR;

	err = bt_gatt_discover(conn, params);
	if (err != 0) {
		/* No terminal callback follows a request the host did not queue */
		smp_bt_client_attach_signal(conn, err, "CCC discovery not started");
	}

	return BT_GATT_ITER_STOP;
}

static uint8_t smp_bt_client_svc_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				    struct bt_gatt_discover_params *params)
{
	const struct bt_gatt_service_val *svc;
	int err;

	if (attr == NULL) {
		smp_bt_client_attach_signal(conn, -ENOTSUP, "no SMP service");
		return BT_GATT_ITER_STOP;
	}

	svc = attr->user_data;
	smp_bt_client_svc_end_handle = svc->end_handle;

	if (attr->handle >= smp_bt_client_svc_end_handle) {
		smp_bt_client_attach_signal(conn, -ENOTSUP, "empty SMP service");
		return BT_GATT_ITER_STOP;
	}

	params->uuid = &smp_bt_client_chr_uuid.uuid;
	params->func = smp_bt_client_chrc_cb;
	params->start_handle = attr->handle + 1U;
	params->end_handle = smp_bt_client_svc_end_handle;
	params->type = BT_GATT_DISCOVER_CHARACTERISTIC;

	err = bt_gatt_discover(conn, params);
	if (err != 0) {
		smp_bt_client_attach_signal(conn, err, "characteristic discovery not started");
	}

	return BT_GATT_ITER_STOP;
}

static void smp_bt_client_disconnected(struct bt_conn *conn, uint8_t reason)
{
	k_spinlock_key_t key;
	bool ours;

	ARG_UNUSED(reason);

	key = k_spin_lock(&smp_bt_client_target_lock);
	ours = (conn == smp_bt_client_target);
	k_spin_unlock(&smp_bt_client_target_lock, key);

	if (!ours) {
		return;
	}

	LOG_DBG("SMP client target disconnected, detaching transport");

	atomic_set(&smp_bt_client_state, SMP_BT_CLIENT_IDLE);
	smp_bt_client_reassembly_clear();

	/* The host does not complete writes on a dropped link, so return their credits
	 * to release a transmit blocked on one.
	 */
	smp_bt_client_tx_credits_replenish();

	smp_rx_clear(&smp_bt_client_transport);
	smp_bt_client_target_clear();

	smp_bt_client_attach_result = -ENOTCONN;
	smp_bt_client_attach_reason = NULL;
	k_sem_give(&smp_bt_client_attach_sem);
}

BT_CONN_CB_DEFINE(smp_bt_client_conn_callbacks) = {
	.disconnected = smp_bt_client_disconnected,
};

static int smp_bt_client_attach_wait(k_timepoint_t deadline)
{
	if (atomic_get(&smp_bt_client_cancel) != 0) {
		return -ECANCELED;
	}

	if (k_sem_take(&smp_bt_client_attach_sem, sys_timepoint_timeout(deadline)) != 0) {
		return -ETIMEDOUT;
	}

	if (atomic_get(&smp_bt_client_cancel) != 0) {
		return -ECANCELED;
	}

	if (smp_bt_client_attach_reason != NULL) {
		LOG_ERR("SMP client attach failed: %s (%d)", smp_bt_client_attach_reason,
			smp_bt_client_attach_result);
	}

	return smp_bt_client_attach_result;
}

static int smp_bt_client_discover(struct bt_conn *conn, k_timepoint_t deadline)
{
	int rc;

	smp_bt_client_svc_end_handle = 0U;
	smp_bt_client_value_handle = 0U;
	smp_bt_client_ccc_handle = 0U;

	smp_bt_client_discover_params.uuid = &smp_bt_client_svc_uuid.uuid;
	smp_bt_client_discover_params.func = smp_bt_client_svc_cb;
	smp_bt_client_discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	smp_bt_client_discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	smp_bt_client_discover_params.type = BT_GATT_DISCOVER_PRIMARY;

	k_sem_reset(&smp_bt_client_attach_sem);

	rc = bt_gatt_discover(conn, &smp_bt_client_discover_params);
	if (rc != 0) {
		LOG_ERR("SMP service discovery not started (err %d)", rc);
		return rc;
	}

	rc = smp_bt_client_attach_wait(deadline);
	if (rc == -ETIMEDOUT || rc == -ECANCELED) {
		/* Runs the terminal callback, after which the host no longer uses the
		 * discovery parameters.
		 */
		bt_gatt_cancel(conn, &smp_bt_client_discover_params);
	}

	return rc;
}

static int smp_bt_client_subscribe(struct bt_conn *conn, k_timepoint_t deadline)
{
	int rc;

	smp_bt_client_sub_params.value_handle = smp_bt_client_value_handle;
	smp_bt_client_sub_params.ccc_handle = smp_bt_client_ccc_handle;
	smp_bt_client_sub_params.value = BT_GATT_CCC_NOTIFY;
	memset(smp_bt_client_sub_params.flags, 0, sizeof(smp_bt_client_sub_params.flags));
	atomic_set_bit(smp_bt_client_sub_params.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	k_sem_reset(&smp_bt_client_attach_sem);

	/* Set before the call: the host can report the node unlinked as soon as it exists */
	atomic_set(&smp_bt_client_sub_linked, 1);
	atomic_set(&smp_bt_client_subscribing, 1);

	rc = bt_gatt_subscribe(conn, &smp_bt_client_sub_params);
	if (rc != 0) {
		if (rc != -EALREADY) {
			atomic_clear(&smp_bt_client_sub_linked);
		}

		LOG_ERR("SMP characteristic subscribe failed (err %d)", rc);
	} else if (atomic_test_bit(smp_bt_client_sub_params.flags,
				   BT_GATT_SUBSCRIBE_FLAG_WRITE_PENDING)) {
		rc = smp_bt_client_attach_wait(deadline);
	} else if (k_sem_take(&smp_bt_client_attach_sem, K_NO_WAIT) == 0) {
		/* The CCC write was answered before the call returned */
		rc = (atomic_get(&smp_bt_client_cancel) != 0) ? -ECANCELED
							      : smp_bt_client_attach_result;
	} else {
		/* Another subscription already enabled notifications, so no CCC write
		 * was sent and no response follows.
		 */
		rc = 0;
	}

	atomic_clear(&smp_bt_client_subscribing);

	return rc;
}

int smp_bt_client_attach(struct bt_conn *conn, k_timeout_t timeout)
{
	const k_timepoint_t deadline = sys_timepoint_calc(timeout);
	k_spinlock_key_t key;
	int rc;

	if (conn == NULL) {
		return -EINVAL;
	}

	if (k_mutex_lock(&smp_bt_client_api_lock, K_NO_WAIT) != 0) {
		return -EBUSY;
	}

	if (atomic_get(&smp_bt_client_cancel) != 0) {
		rc = -ECANCELED;
		goto unlock;
	}

	if (atomic_get(&smp_bt_client_state) != SMP_BT_CLIENT_IDLE) {
		rc = -EBUSY;
		goto unlock;
	}

	if (atomic_get(&smp_bt_client_sub_linked) != 0 ||
	    atomic_test_bit(smp_bt_client_sub_params.flags, BT_GATT_SUBSCRIBE_FLAG_WRITE_PENDING)) {
		LOG_WRN("SMP client subscription still held by the host");
		rc = -EBUSY;
		goto unlock;
	}

	atomic_set(&smp_bt_client_state, SMP_BT_CLIENT_ATTACHING);
	atomic_inc(&smp_bt_client_generation);

	/* Credits the previous link never returned died with it */
	smp_bt_client_tx_credits_replenish();

	key = k_spin_lock(&smp_bt_client_target_lock);
	smp_bt_client_target = bt_conn_ref(conn);
	k_spin_unlock(&smp_bt_client_target_lock, key);

	rc = smp_bt_client_discover(conn, deadline);
	if (rc != 0) {
		goto fail;
	}

	rc = smp_bt_client_subscribe(conn, deadline);
	if (rc != 0) {
		goto fail;
	}

	/* A disconnect after the last wait has already cleared the target */
	if (!smp_bt_client_tx_handle_install(smp_bt_client_value_handle) ||
	    !atomic_cas(&smp_bt_client_state, SMP_BT_CLIENT_ATTACHING, SMP_BT_CLIENT_READY)) {
		rc = -ENOTCONN;
		goto fail;
	}

	LOG_INF("SMP client transport attached (value handle 0x%04x, MTU %u)",
		smp_bt_client_value_handle, bt_gatt_get_mtu(conn));

	k_mutex_unlock(&smp_bt_client_api_lock);

	return 0;

fail:
	/* Also cancels a CCC write that is still in flight */
	smp_bt_client_unsubscribe(conn);

	atomic_set(&smp_bt_client_state, SMP_BT_CLIENT_IDLE);
	smp_bt_client_reassembly_clear();
	smp_bt_client_target_clear();

unlock:
	k_mutex_unlock(&smp_bt_client_api_lock);

	return rc;
}

void smp_bt_client_detach(void)
{
	struct bt_conn *conn;

	atomic_set(&smp_bt_client_cancel, 1);
	k_sem_give(&smp_bt_client_attach_sem);

	(void)k_mutex_lock(&smp_bt_client_api_lock, K_FOREVER);

	/* Stop collecting notifications before the context is dropped */
	atomic_set(&smp_bt_client_state, SMP_BT_CLIENT_IDLE);

	conn = smp_bt_client_conn_claim();
	if (conn != NULL) {
		smp_bt_client_unsubscribe(conn);
		bt_conn_unref(conn);
	}

	smp_bt_client_reassembly_clear();
	smp_rx_clear(&smp_bt_client_transport);
	smp_bt_client_target_clear();

	atomic_clear(&smp_bt_client_cancel);
	k_mutex_unlock(&smp_bt_client_api_lock);
}

bool smp_bt_client_is_attached(void)
{
	return atomic_get(&smp_bt_client_state) == SMP_BT_CLIENT_READY;
}

static void smp_bt_client_setup(void)
{
	int rc;

	smp_bt_client_transport.functions.output = smp_bt_client_tx_pkt;
	smp_bt_client_transport.functions.get_mtu = smp_bt_client_get_mtu;

	rc = smp_transport_init(&smp_bt_client_transport);
	if (rc != 0) {
		LOG_ERR("Bluetooth client SMP transport init failed (err %d)", rc);
		return;
	}

	/* Set once: the host calls notify without a NULL check */
	smp_bt_client_sub_params.notify = smp_bt_client_notify_cb;
	smp_bt_client_sub_params.subscribe = smp_bt_client_subscribe_cb;

	(void)smp_client_transport_register(&smp_bt_client_entry);
}

MCUMGR_HANDLER_DEFINE(smp_bt_client, smp_bt_client_setup);
