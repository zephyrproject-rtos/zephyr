/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2022-2026 Nordic Semiconductor ASA
 * SPDX-FileCopyrightText: Copyright (c) 2026 Jamie McCrae
 * SPDX-FileCopyrightText: Copyright (c) 2026 Jeff Welder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Bluetooth client transport for the MCUmgr SMP protocol.
 *
 * Discovers the SMP service of a peer, subscribes to its notifications, writes SMP requests
 * as ATT_MTU-3 sized writes without response and passes the responses the peer notifies
 * through the reassembly context to the SMP core. The transport serves one target at a time,
 * as the outgoing side of a transport management bridge.
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
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>

#include <mgmt/mcumgr/transport/smp_internal.h>
#include <mgmt/mcumgr/transport/smp_reassembly.h>

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
#include <zephyr/mgmt/mcumgr/grp/transport_mgmt/transport_mgmt.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>
#endif

LOG_MODULE_DECLARE(mcumgr_smp, CONFIG_MCUMGR_TRANSPORT_LOG_LEVEL);

/* ATT Write Command opcode and handle */
#define SMP_BT_CLIENT_ATT_OVERHEAD 3U

#define SMP_BT_CLIENT_TX_TIMEOUT K_MSEC(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDIT_TIMEOUT)

/* How long to wait before hanging up again on a link that a bridge has let go of */
#define SMP_BT_CLIENT_HANGUP_RETRY K_SECONDS(1)

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
/* How long a bridge connect waits for the link and the SMP service of the peer: the ATT
 * transaction timeout.
 */
#define SMP_BT_CLIENT_BRIDGE_TIMEOUT K_SECONDS(30)
#endif

enum smp_bt_client_link_state {
	SMP_BT_CLIENT_IDLE = 0,
	SMP_BT_CLIENT_ATTACHING,
	SMP_BT_CLIENT_READY,
};

static struct smp_transport smp_bt_client_transport;

/* Written under smp_bt_client_lock, read without it on the data paths */
static atomic_t smp_bt_client_state = ATOMIC_INIT(SMP_BT_CLIENT_IDLE);

/* Incremented on every claim of the transport, so that a packet being written out or a
 * teardown can tell that the target it latched has since been replaced.
 */
static atomic_t smp_bt_client_generation = ATOMIC_INIT(0);

/* Guards the claims of the transport and the target, whose value handle is always read
 * together with it.
 */
static struct k_spinlock smp_bt_client_lock;
static struct bt_conn *smp_bt_client_target;
static uint16_t smp_bt_client_tx_handle;

/* A link that a bridge created and has let go of before it ended. The connection callbacks
 * drop the reference once it has ended, and disconnect it if it comes up after its creation
 * was cancelled.
 */
static struct bt_conn *smp_bt_client_orphan;

struct smp_bt_client_link {
	struct bt_conn *conn;
	uint16_t value_handle;
	uint32_t generation;
};

/* Handshake between the Bluetooth callbacks and a waiting bridge connect */
static K_SEM_DEFINE(smp_bt_client_attach_sem, 0, 1);
static int smp_bt_client_attach_result;
static const char *smp_bt_client_attach_reason;
static atomic_t smp_bt_client_subscribing = ATOMIC_INIT(0);

/* Written only by the discovery callbacks */
static uint16_t smp_bt_client_svc_end_handle;
static uint16_t smp_bt_client_value_handle;
static uint16_t smp_bt_client_ccc_handle;

/* The host uses the subscription parameters while they are linked into its subscription
 * list, until it reports their removal with notify(NULL). Once bt_gatt_unsubscribe() has
 * unlinked them, it uses them for the CCC write that disables the notifications, until it
 * reports a success with notify(NULL), runs the subscribe callback with a failure, or the
 * link ends. A claim only succeeds when the host uses them for neither.
 */
static struct bt_gatt_subscribe_params smp_bt_client_sub_params;
static atomic_t smp_bt_client_sub_linked = ATOMIC_INIT(0);

/* The link of a disabling CCC write in flight. Only compared, never dereferenced: the
 * disconnected callback of the link clears it before the connection object can be reused.
 */
static atomic_ptr_t smp_bt_client_unsub_conn = ATOMIC_PTR_INIT(NULL);

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
 * and from teardown on other threads. Also guards smp_bt_client_rx_discard.
 */
static K_MUTEX_DEFINE(smp_bt_client_reassembly_lock);

/* Bytes still to skip of a packet from the peer that could not be collected */
static uint32_t smp_bt_client_rx_discard;

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

/* Latches the target for a packet */
static bool smp_bt_client_link_claim(struct smp_bt_client_link *link)
{
	bool claimed = false;
	k_spinlock_key_t key = k_spin_lock(&smp_bt_client_lock);

	if (atomic_get(&smp_bt_client_state) == SMP_BT_CLIENT_READY) {
		link->conn = bt_conn_ref(smp_bt_client_target);
		link->value_handle = smp_bt_client_tx_handle;
		link->generation = (uint32_t)atomic_get(&smp_bt_client_generation);
		claimed = true;
	}

	k_spin_unlock(&smp_bt_client_lock, key);

	return claimed;
}

/* The host marks a dropped link before it calls back, so a packet also stops waiting for a
 * link that is gone while the Bluetooth callbacks are held up.
 */
static bool smp_bt_client_link_current(const struct smp_bt_client_link *link)
{
	return atomic_get(&smp_bt_client_state) == SMP_BT_CLIENT_READY &&
	       (uint32_t)atomic_get(&smp_bt_client_generation) == link->generation &&
	       smp_bt_client_conn_up(link->conn);
}

static void smp_bt_client_reassembly_clear(void)
{
	k_mutex_lock(&smp_bt_client_reassembly_lock, K_FOREVER);
	(void)smp_reassembly_drop(&smp_bt_client_transport);
	smp_bt_client_rx_discard = 0U;
	k_mutex_unlock(&smp_bt_client_reassembly_lock);
}

/* The host unlinks the node before bt_gatt_unsubscribe() returns success, cancels a CCC
 * write that is still in flight without running its callback, and queues the disabling
 * write. On failure no write is queued.
 */
static void smp_bt_client_unsubscribe(struct bt_conn *conn)
{
	int rc;

	if (atomic_get(&smp_bt_client_sub_linked) == 0) {
		return;
	}

	/* Set before the call: with no CCC write to make, it reports the removal with
	 * notify(NULL) before it returns
	 */
	atomic_ptr_set(&smp_bt_client_unsub_conn, conn);

	rc = bt_gatt_unsubscribe(conn, &smp_bt_client_sub_params);
	if (rc == 0) {
		atomic_clear(&smp_bt_client_sub_linked);
		return;
	}

	atomic_ptr_clear(&smp_bt_client_unsub_conn);

	if (rc == -EINVAL) {
		/* Not linked */
		atomic_clear(&smp_bt_client_sub_linked);
	} else if (rc != -ENOTCONN) {
		/* The node stays linked until the link ends */
		LOG_WRN("SMP client unsubscribe failed (err %d)", rc);
	} else {
		/* The host removes the subscription of a dropped link itself */
	}
}

/*
 * Claims the transport for a bridge connect. Fails while a target is attached or is still
 * being torn down, while the host holds the subscription of a previous target and while the
 * link of the previous bridge has not ended.
 */
static int smp_bt_client_claim(uint32_t *generation)
{
	bool held = false;
	int rc = -EBUSY;
	k_spinlock_key_t key = k_spin_lock(&smp_bt_client_lock);

	if (atomic_get(&smp_bt_client_state) != SMP_BT_CLIENT_IDLE ||
	    smp_bt_client_target != NULL) {
		/* Busy */
	} else if (smp_bt_client_orphan != NULL) {
		/* Busy */
	} else if (atomic_get(&smp_bt_client_sub_linked) != 0 ||
		   atomic_ptr_get(&smp_bt_client_unsub_conn) != NULL) {
		held = true;
	} else {
		atomic_set(&smp_bt_client_state, SMP_BT_CLIENT_ATTACHING);
		*generation = (uint32_t)atomic_inc(&smp_bt_client_generation) + 1U;
		rc = 0;
	}

	k_spin_unlock(&smp_bt_client_lock, key);

	if (held) {
		LOG_WRN("SMP client subscription still held by the host");
	}

	if (rc == 0) {
		/* Credits the previous link never returned died with it */
		smp_bt_client_tx_credits_replenish();
	}

	return rc;
}

static void smp_bt_client_target_set(struct bt_conn *conn)
{
	k_spinlock_key_t key = k_spin_lock(&smp_bt_client_lock);

	smp_bt_client_target = bt_conn_ref(conn);
	k_spin_unlock(&smp_bt_client_lock, key);
}

static void smp_bt_client_orphan_release(const struct bt_conn *conn)
{
	struct bt_conn *orphan = NULL;
	k_spinlock_key_t key = k_spin_lock(&smp_bt_client_lock);

	if (smp_bt_client_orphan == conn) {
		orphan = smp_bt_client_orphan;
		smp_bt_client_orphan = NULL;
	}

	k_spin_unlock(&smp_bt_client_lock, key);

	if (orphan != NULL) {
		bt_conn_unref(orphan);
	}
}

static void smp_bt_client_bridge_hangup(struct bt_conn *conn);

static void smp_bt_client_hangup_retry(struct k_work *work)
{
	struct bt_conn *conn = NULL;
	k_spinlock_key_t key = k_spin_lock(&smp_bt_client_lock);

	ARG_UNUSED(work);

	if (smp_bt_client_orphan != NULL) {
		conn = bt_conn_ref(smp_bt_client_orphan);
	}

	k_spin_unlock(&smp_bt_client_lock, key);

	if (conn != NULL) {
		smp_bt_client_bridge_hangup(conn);
		bt_conn_unref(conn);
	}
}

static K_WORK_DELAYABLE_DEFINE(smp_bt_client_hangup_work, smp_bt_client_hangup_retry);

/*
 * Disconnects, or cancels the creation of, a link that a bridge has let go of. A failure is
 * retried on the MCUmgr work queue: no connection callback releases a link that stays up,
 * and until it ends no bridge can claim the transport.
 */
static void smp_bt_client_bridge_hangup(struct bt_conn *conn)
{
	struct bt_conn_info info;
	int rc;

	rc = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (rc != 0 && rc != -ENOTCONN) {
		LOG_ERR("Failed to disconnect BT MCUmgr outgoing bridge: %d", rc);
		(void)k_work_schedule_for_queue(smp_get_wq(), &smp_bt_client_hangup_work,
						SMP_BT_CLIENT_HANGUP_RETRY);
	}

	/* No connection callback follows for a link that has ended */
	if (bt_conn_get_info(conn, &info) != 0 || info.state == BT_CONN_STATE_DISCONNECTED) {
		smp_bt_client_orphan_release(conn);
	}
}

/*
 * Tears down the target of the claim made under @p generation, unless that has already been
 * done, and disconnects its link. Does not block on the Bluetooth host, so it is safe on any
 * thread.
 *
 * Returns the state the target was in, or SMP_BT_CLIENT_IDLE if it was not torn down here.
 */
static enum smp_bt_client_link_state smp_bt_client_release(uint32_t generation)
{
	enum smp_bt_client_link_state state;
	struct bt_conn *conn = NULL;
	k_spinlock_key_t key = k_spin_lock(&smp_bt_client_lock);

	state = (enum smp_bt_client_link_state)atomic_get(&smp_bt_client_state);

	if (state == SMP_BT_CLIENT_IDLE ||
	    (uint32_t)atomic_get(&smp_bt_client_generation) != generation) {
		k_spin_unlock(&smp_bt_client_lock, key);
		return SMP_BT_CLIENT_IDLE;
	}

	/* Stop collecting notifications and sending packets before the context is dropped */
	atomic_set(&smp_bt_client_state, SMP_BT_CLIENT_IDLE);
	k_spin_unlock(&smp_bt_client_lock, key);

	smp_bt_client_reassembly_clear();
	smp_rx_clear(&smp_bt_client_transport);

	/* The target is cleared last, as no claim succeeds while it is set */
	key = k_spin_lock(&smp_bt_client_lock);

	if ((uint32_t)atomic_get(&smp_bt_client_generation) == generation) {
		conn = smp_bt_client_target;
		smp_bt_client_target = NULL;
		smp_bt_client_tx_handle = 0U;

		if (conn != NULL) {
			/* A bridge claims only while there is no orphan */
			__ASSERT_NO_MSG(smp_bt_client_orphan == NULL);
			smp_bt_client_orphan = conn;
			conn = bt_conn_ref(conn);
		}
	}

	k_spin_unlock(&smp_bt_client_lock, key);

	if (conn == NULL) {
		return state;
	}

	smp_bt_client_unsubscribe(conn);
	smp_bt_client_bridge_hangup(conn);

	bt_conn_unref(conn);

	return state;
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

/* The SMP layer may keep its own reference to @p nb, so the buffer is only read here, and
 * exactly one reference is released on every path.
 */
static int smp_bt_client_send(struct net_buf *nb, int no_link_rc)
{
	struct smp_bt_client_link link;
	int rc = no_link_rc;

	if (smp_bt_client_link_claim(&link)) {
		rc = smp_bt_client_tx_frags(&link, nb);
		bt_conn_unref(link.conn);
	}

	smp_packet_free(nb);

	return rc;
}

/* Packets of the SMP client have no target: the transport only carries bridge traffic */
static int smp_bt_client_tx_pkt(struct net_buf *nb)
{
	smp_packet_free(nb);

	return MGMT_ERR_ENOENT;
}

/* Skips the rest of the packet whose first fragment is @p data, by the length in its header */
static void smp_bt_client_rx_skip(const uint8_t *data, uint16_t length)
{
	struct smp_hdr hdr;
	uint32_t total;

	if (length < sizeof(hdr)) {
		return;
	}

	memcpy(&hdr, data, sizeof(hdr));
	total = sizeof(hdr) + sys_be16_to_cpu(hdr.nh_len);
	smp_bt_client_rx_discard = (total > length) ? (total - length) : 0U;
}

static uint8_t smp_bt_client_notify_cb(struct bt_conn *conn,
				       struct bt_gatt_subscribe_params *params, const void *data,
				       uint16_t length)
{
	bool first;
	int ret;

	ARG_UNUSED(conn);

	if (data == NULL) {
		/* The host has removed the node from its subscription list, or has had the
		 * disabling CCC write of an unsubscribe accepted, and no longer uses the
		 * parameters.
		 */
		atomic_ptr_clear(&smp_bt_client_unsub_conn);
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
		goto unlock;
	}

	if (smp_bt_client_rx_discard > 0U) {
		smp_bt_client_rx_discard -= MIN(smp_bt_client_rx_discard, length);
		goto unlock;
	}

	first = smp_reassembly_expected(&smp_bt_client_transport) < 0;

	ret = smp_reassembly_collect(&smp_bt_client_transport, data, length);

	if (ret == -ENOMEM) {
		LOG_WRN("SMP client reassembly buffer exhausted");
	} else if (ret < 0) {
		LOG_WRN_RATELIMIT("SMP client reassembly failed (%d), dropping packet", ret);
		(void)smp_reassembly_drop(&smp_bt_client_transport);
	} else if (ret == 0) {
		(void)smp_reassembly_complete(&smp_bt_client_transport, false);
	} else {
		/* More fragments expected */
	}

	/* The rest of a packet that could not be collected, such as a response longer than the
	 * buffer, would otherwise be taken for the start of the next packet.
	 */
	if (ret < 0 && first) {
		smp_bt_client_rx_skip(data, length);
	}

unlock:
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
	/* Only the response to the enabling write of a waiting connect is a verdict; the
	 * response to an unsubscribe's disabling write is not.
	 */
	if (params->value != BT_GATT_CCC_NOTIFY) {
		/* notify(NULL) follows a success. After a failure the host no longer uses the
		 * parameters.
		 */
		if (err != 0U) {
			atomic_ptr_clear(&smp_bt_client_unsub_conn);
		}

		return;
	}

	if (atomic_get(&smp_bt_client_subscribing) == 0) {
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
	static const struct bt_uuid_16 ccc_uuid = BT_UUID_INIT_16(BT_UUID_GATT_CCC_VAL);
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
	params->uuid = &ccc_uuid.uuid;
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
	static const struct bt_uuid_128 chr_uuid = BT_UUID_INIT_128(SMP_BT_CHR_UUID_VAL);
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

	params->uuid = &chr_uuid.uuid;
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

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
/* Tears down a bridge whose link was lost. Runs on the MCUmgr work queue, as the
 * transport_mgmt lock must not be taken on the Bluetooth RX thread: the holder of the lock
 * may wait for a callback that runs there.
 */
static void smp_bt_client_bridge_lost(struct k_work *work)
{
	bool bridged;
	k_spinlock_key_t key;

	ARG_UNUSED(work);

	/* A new bridge on this transport means that the lost one is gone */
	key = k_spin_lock(&smp_bt_client_lock);
	bridged = atomic_get(&smp_bt_client_state) != SMP_BT_CLIENT_IDLE;
	k_spin_unlock(&smp_bt_client_lock, key);

	if (!bridged) {
		(void)transport_mgmt_disconnect_transport(&smp_bt_client_transport, false, true);
	}
}

static K_WORK_DEFINE(smp_bt_client_bridge_lost_work, smp_bt_client_bridge_lost);

/* Handshake between the connected callback and a bridge connect waiting for its link. The
 * host runs the callback after it has marked the link connected, so it can come while the
 * connect already waits for discovery on smp_bt_client_attach_sem.
 */
static K_SEM_DEFINE(smp_bt_client_connect_sem, 0, 1);
static int smp_bt_client_connect_result;

static void smp_bt_client_connected(struct bt_conn *conn, uint8_t err)
{
	bool orphan;
	bool ours;
	k_spinlock_key_t key = k_spin_lock(&smp_bt_client_lock);

	orphan = (conn == smp_bt_client_orphan);
	ours = (conn == smp_bt_client_target &&
		atomic_get(&smp_bt_client_state) == SMP_BT_CLIENT_ATTACHING);
	k_spin_unlock(&smp_bt_client_lock, key);

	if (orphan) {
		if (err == 0U) {
			/* Its creation was cancelled too late */
			smp_bt_client_bridge_hangup(conn);
		} else {
			smp_bt_client_orphan_release(conn);
		}
	} else if (ours) {
		if (err == 0U) {
			LOG_INF("Connected to: %s (%p)", bt_conn_dst_str(conn), (void *)conn);
		} else {
			LOG_ERR("Failed to connect to: %s (%u)", bt_conn_dst_str(conn), err);
		}

		smp_bt_client_connect_result = (err == 0U) ? 0 : -ENOTCONN;
		k_sem_give(&smp_bt_client_connect_sem);
	} else {
		/* Not a link of this transport */
	}
}
#endif /* CONFIG_MCUMGR_GRP_TRANSPORT */

static void smp_bt_client_disconnected(struct bt_conn *conn, uint8_t reason)
{
	enum smp_bt_client_link_state state;
	uint32_t generation;
	k_spinlock_key_t key;

	ARG_UNUSED(reason);

	smp_bt_client_orphan_release(conn);
	/* A disabling CCC write still in flight on the link has failed. The host reports that
	 * only when the link had other subscriptions.
	 */
	(void)atomic_ptr_cas(&smp_bt_client_unsub_conn, conn, NULL);

	key = k_spin_lock(&smp_bt_client_lock);

	if (conn != smp_bt_client_target) {
		k_spin_unlock(&smp_bt_client_lock, key);
		return;
	}

	generation = (uint32_t)atomic_get(&smp_bt_client_generation);
	k_spin_unlock(&smp_bt_client_lock, key);

	LOG_DBG("SMP client target disconnected, detaching transport");

	state = smp_bt_client_release(generation);
	if (state == SMP_BT_CLIENT_IDLE) {
		/* Torn down elsewhere */
		return;
	}

	/* The host does not complete writes on a dropped link, so return their credits
	 * to release a transmit blocked on one.
	 */
	smp_bt_client_tx_credits_replenish();

	smp_bt_client_attach_result = -ENOTCONN;
	smp_bt_client_attach_reason = NULL;
	k_sem_give(&smp_bt_client_attach_sem);

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
	if (state == SMP_BT_CLIENT_READY) {
		(void)k_work_submit_to_queue(smp_get_wq(), &smp_bt_client_bridge_lost_work);
	}
#endif
}

BT_CONN_CB_DEFINE(smp_bt_client_conn_callbacks) = {
#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
	.connected = smp_bt_client_connected,
#endif
	.disconnected = smp_bt_client_disconnected,
};

static int smp_bt_client_attach_wait(k_timepoint_t deadline)
{
	if (k_sem_take(&smp_bt_client_attach_sem, sys_timepoint_timeout(deadline)) != 0) {
		return -ETIMEDOUT;
	}

	if (smp_bt_client_attach_reason != NULL) {
		LOG_ERR("SMP client attach failed: %s (%d)", smp_bt_client_attach_reason,
			smp_bt_client_attach_result);
	}

	return smp_bt_client_attach_result;
}

static int smp_bt_client_discover(struct bt_conn *conn, k_timepoint_t deadline)
{
	static const struct bt_uuid_128 svc_uuid = BT_UUID_INIT_128(SMP_BT_SVC_UUID_VAL);
	/* Used by the host until the terminal callback */
	static struct bt_gatt_discover_params params;
	int rc;

	smp_bt_client_svc_end_handle = 0U;
	smp_bt_client_value_handle = 0U;
	smp_bt_client_ccc_handle = 0U;

	params.uuid = &svc_uuid.uuid;
	params.func = smp_bt_client_svc_cb;
	params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
	params.type = BT_GATT_DISCOVER_PRIMARY;

	k_sem_reset(&smp_bt_client_attach_sem);

	rc = bt_gatt_discover(conn, &params);
	if (rc != 0) {
		LOG_ERR("SMP service discovery not started (err %d)", rc);
		return rc;
	}

	rc = smp_bt_client_attach_wait(deadline);
	if (rc == -ETIMEDOUT) {
		/* Runs the terminal callback, after which the host no longer uses the
		 * discovery parameters.
		 */
		bt_gatt_cancel(conn, &params);
	}

	return rc;
}

static int smp_bt_client_subscribe(struct bt_conn *conn, k_timepoint_t deadline)
{
	int rc;

	smp_bt_client_sub_params.value_handle = smp_bt_client_value_handle;
	smp_bt_client_sub_params.ccc_handle = smp_bt_client_ccc_handle;
	smp_bt_client_sub_params.value = BT_GATT_CCC_NOTIFY;
	atomic_set_bit(smp_bt_client_sub_params.flags, BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	k_sem_reset(&smp_bt_client_attach_sem);

	/* Set before the call: the host can report the node unlinked as soon as it exists */
	atomic_set(&smp_bt_client_sub_linked, 1);
	atomic_set(&smp_bt_client_subscribing, 1);

	rc = bt_gatt_subscribe(conn, &smp_bt_client_sub_params);
	if (rc != 0) {
		if (rc != -EALREADY) {
			/* Nothing is linked and no write is in flight */
			atomic_clear(&smp_bt_client_sub_linked);
		}

		LOG_ERR("SMP characteristic subscribe failed (err %d)", rc);
	} else {
		/* The transport is the only subscriber to the characteristic, so the host has
		 * queued a CCC write, and the subscribe callback reports its result, which may
		 * already have come.
		 */
		rc = smp_bt_client_attach_wait(deadline);
	}

	atomic_clear(&smp_bt_client_subscribing);

	return rc;
}

/* Sets up the SMP service of the target of the claim made under @p generation */
static int smp_bt_client_attach_link(struct bt_conn *conn, uint32_t generation,
				     k_timepoint_t deadline)
{
	bool ready = false;
	k_spinlock_key_t key;
	int rc;

	rc = smp_bt_client_discover(conn, deadline);
	if (rc != 0) {
		return rc;
	}

	rc = smp_bt_client_subscribe(conn, deadline);
	if (rc != 0) {
		return rc;
	}

	/* A disconnect after the last wait has already torn the target down */
	key = k_spin_lock(&smp_bt_client_lock);

	if ((uint32_t)atomic_get(&smp_bt_client_generation) == generation &&
	    atomic_get(&smp_bt_client_state) == SMP_BT_CLIENT_ATTACHING &&
	    smp_bt_client_target != NULL) {
		smp_bt_client_tx_handle = smp_bt_client_value_handle;
		atomic_set(&smp_bt_client_state, SMP_BT_CLIENT_READY);
		ready = true;
	}

	k_spin_unlock(&smp_bt_client_lock, key);

	if (!ready) {
		return -ENOTCONN;
	}

	LOG_INF("SMP client transport attached (value handle 0x%04x, MTU %u)",
		smp_bt_client_value_handle, bt_gatt_get_mtu(conn));

	return 0;
}

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
/* Waits for the link a bridge connect created and sets up the SMP service of the peer */
static int smp_bt_client_bridge_setup(struct bt_conn *conn, uint32_t generation,
				      k_timepoint_t deadline)
{
	struct bt_conn_info info;
	int rc;

	/* The host marks the link connected before it runs the connected callbacks, which can
	 * come before or after this check, and before or after the target was set. A callback
	 * that comes after it only gives smp_bt_client_connect_sem, which is waited on only
	 * while the link is connecting.
	 */
	rc = bt_conn_get_info(conn, &info);
	if (rc != 0) {
		return rc;
	}

	if (info.state == BT_CONN_STATE_CONNECTING) {
		if (k_sem_take(&smp_bt_client_connect_sem, sys_timepoint_timeout(deadline)) != 0) {
			rc = -ETIMEDOUT;
		} else {
			rc = smp_bt_client_connect_result;
		}
	} else if (info.state != BT_CONN_STATE_CONNECTED) {
		rc = -ENOTCONN;
	} else {
		/* Already connected */
	}

	if (rc == 0) {
		rc = smp_bt_client_attach_link(conn, generation, deadline);
	}

	return rc;
}

static bool smp_bt_client_bridge_connect(struct smp_transport_bridge *bridge, bool outgoing,
					 uint32_t mode, bool same_transport,
					 zcbor_state_t *input_data, zcbor_state_t *output_data)
{
	const k_timepoint_t deadline = sys_timepoint_calc(SMP_BT_CLIENT_BRIDGE_TIMEOUT);
	struct bt_conn_le_create_param *create_param = BT_CONN_LE_CREATE_CONN;
	struct bt_conn *conn = NULL;
	uint32_t address_type = 0U;
	struct zcbor_string address = { 0 };
	uint8_t address_buffer[BT_ADDR_STR_LEN];
	bool le_coded = false;
	uint32_t generation;
	bt_addr_le_t addr;
	size_t decoded = 0U;
	bool ok;
	int rc;

	struct zcbor_map_decode_key_val bt_bridge_connect_decode[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("address_type", zcbor_uint32_decode,
					     &address_type),
		ZCBOR_MAP_DECODE_KEY_DECODER("address", zcbor_tstr_decode, &address),
		ZCBOR_MAP_DECODE_KEY_DECODER("le_coded", zcbor_bool_decode, &le_coded),
	};

	ARG_UNUSED(bridge);
	ARG_UNUSED(mode);
	ARG_UNUSED(same_transport);

	if (!outgoing) {
		/* The client connects to SMP servers, so it is never the incoming side */
		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_TRANSPORT_INGOING_NOT_SUPPORTED);
		return false;
	}

	ok = zcbor_map_decode_bulk(input_data, bt_bridge_connect_decode,
				   ARRAY_SIZE(bt_bridge_connect_decode), &decoded) == 0;

	if (!ok || decoded < 1U || !zcbor_map_decode_bulk_key_found(bt_bridge_connect_decode,
					ARRAY_SIZE(bt_bridge_connect_decode), "address")) {
		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_CONNECT_MISSING_PARAMETER);
		LOG_ERR("Missing required bridge parameter");
		return false;
	}

	if (address_type != BT_ADDR_LE_PUBLIC && address_type != BT_ADDR_LE_RANDOM) {
		LOG_ERR("Invalid address type");
		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_CONNECT_INVALID_PARAMETER);
		return false;
	}

	if (address.len != (BT_ADDR_STR_LEN - 1)) {
		LOG_ERR("Invalid address length");
		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_CONNECT_INVALID_PARAMETER);
		return false;
	}

	addr.type = address_type;

	memcpy(address_buffer, address.value, (BT_ADDR_STR_LEN - 1));
	address_buffer[BT_ADDR_STR_LEN - 1] = 0U;
	rc = bt_addr_from_str(address_buffer, &addr.a);

	if (rc != 0) {
		LOG_ERR("Failed to convert address: %d", rc);
		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_CONNECT_INVALID_PARAMETER);
		return false;
	}

	if (le_coded == true) {
#if defined(CONFIG_HAS_BT_CTLR) && !defined(CONFIG_BT_CTLR_PHY_CODED)
		LOG_ERR("Requested LE CODED PHY is not supported");
		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_CONNECT_UNSUPPORTED_PARAMETER);
		return false;
#else
		create_param->options |= (BT_CONN_LE_OPT_CODED | BT_CONN_LE_OPT_NO_1M);
		LOG_DBG("Using LE coded PHY");
#endif
	} else {
		LOG_DBG("Using 1M PHY");
	}

	if (smp_bt_client_claim(&generation) != 0) {
		LOG_ERR("Bluetooth client transport in use");
		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_CONNECT_FAILED);
		return false;
	}

	k_sem_reset(&smp_bt_client_connect_sem);

	rc = bt_conn_le_create(&addr, create_param, BT_LE_CONN_PARAM_DEFAULT, &conn);

	if (rc != 0) {
		LOG_ERR("Connection failed: %d", rc);
		(void)smp_bt_client_release(generation);

#if !defined(CONFIG_HAS_BT_CTLR)
		if (rc == -EINVAL || rc == -EIO) {
			smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
					TRANSPORT_MGMT_ERR_CONNECT_UNSUPPORTED_PARAMETER);
			return false;
		}
#endif

		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_CONNECT_FAILED);
		return false;
	}

	smp_bt_client_target_set(conn);
	rc = smp_bt_client_bridge_setup(conn, generation, deadline);

	/* The target holds a reference of its own */
	bt_conn_unref(conn);

	if (rc != 0) {
		LOG_ERR("Outgoing bridge setup failed: %d", rc);
		(void)smp_bt_client_release(generation);
		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_CONNECT_FAILED);
		return false;
	}

	return true;
}

static void smp_bt_client_bridge_disconnect(struct smp_transport_bridge *bridge, bool outgoing)
{
	ARG_UNUSED(bridge);
	ARG_UNUSED(outgoing);

	(void)smp_bt_client_release((uint32_t)atomic_get(&smp_bt_client_generation));
}

static int smp_bt_client_bridge_tx(const struct smp_transport_bridge *bridge, struct net_buf *nb,
				   bool outgoing)
{
	ARG_UNUSED(bridge);
	ARG_UNUSED(outgoing);

	return smp_bt_client_send(nb, MGMT_ERR_BRIDGED_CONNECTION_UNAVAILABLE);
}

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS)
static bool smp_bt_client_bridge_modes(zcbor_state_t *output_data, int *rc)
{
	bool ok;

	ok = zcbor_map_start_encode(output_data, 4) &&
	     zcbor_tstr_put_lit(output_data, "id") &&
	     zcbor_uint32_put(output_data, 0) &&
	     zcbor_tstr_put_lit(output_data, "description") &&
	     zcbor_tstr_put_lit(output_data, "Bluetooth Low Energy") &&
	     zcbor_tstr_put_lit(output_data, "incoming") &&
	     zcbor_bool_put(output_data, IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_BT)) &&
	     zcbor_tstr_put_lit(output_data, "outgoing") &&
	     zcbor_bool_put(output_data, true) &&
	     zcbor_map_end_encode(output_data, 4);

	*rc = MGMT_RETURN_CHECK(ok);
	return ok;
}

static bool smp_bt_client_bridge_config_details(uint32_t mode, zcbor_state_t *output_data, int *rc)
{
	bool ok;

	if (mode != 0) {
		smp_mgmt_reset_writer(output_data);
		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_INVALID_MODE);
		*rc = 0;

		return false;
	}

	ok = zcbor_map_start_encode(output_data, 3) &&
	     zcbor_tstr_put_lit(output_data, "name") &&
	     zcbor_tstr_put_lit(output_data, "address_type") &&
	     zcbor_tstr_put_lit(output_data, "type") &&
	     zcbor_uint32_put(output_data, 0) &&
	     zcbor_tstr_put_lit(output_data, "required") &&
	     zcbor_bool_put(output_data, true) &&
	     zcbor_map_end_encode(output_data, 3) &&
	     zcbor_map_start_encode(output_data, 3) &&
	     zcbor_tstr_put_lit(output_data, "name") &&
	     zcbor_tstr_put_lit(output_data, "address") &&
	     zcbor_tstr_put_lit(output_data, "type") &&
	     zcbor_uint32_put(output_data, 3) &&
	     zcbor_tstr_put_lit(output_data, "required") &&
	     zcbor_bool_put(output_data, true) &&
	     zcbor_map_end_encode(output_data, 3) &&
	     zcbor_map_start_encode(output_data, 3) &&
	     zcbor_tstr_put_lit(output_data, "name") &&
	     zcbor_tstr_put_lit(output_data, "le_coded") &&
	     zcbor_tstr_put_lit(output_data, "type") &&
	     zcbor_uint32_put(output_data, 2) &&
	     zcbor_map_end_encode(output_data, 3);

	*rc = MGMT_RETURN_CHECK(ok);
	return ok;
}
#endif /* CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS */
#endif /* CONFIG_MCUMGR_GRP_TRANSPORT */

static void smp_bt_client_setup(void)
{
	static struct smp_client_transport_entry smp_bt_client_entry = {
		.smpt = &smp_bt_client_transport,
		.smpt_type = SMP_BLUETOOTH_TRANSPORT,
#ifdef CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS
		.name = "Bluetooth",
#endif
	};
	int rc;

	smp_bt_client_transport.functions.output = smp_bt_client_tx_pkt;

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
	smp_bt_client_transport.functions.bridge_connect = smp_bt_client_bridge_connect;
	smp_bt_client_transport.functions.bridge_disconnect = smp_bt_client_bridge_disconnect;
	smp_bt_client_transport.functions.bridge_output = smp_bt_client_bridge_tx;
#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS)
	smp_bt_client_transport.functions.bridge_modes = smp_bt_client_bridge_modes;
	smp_bt_client_transport.functions.bridge_config_details =
		smp_bt_client_bridge_config_details;
#endif
#endif

	rc = smp_transport_init(&smp_bt_client_transport);
	if (rc != 0) {
		LOG_ERR("Bluetooth client SMP transport init failed (err %d)", rc);
		return;
	}

	/* Set once: the host calls notify without a NULL check */
	smp_bt_client_sub_params.notify = smp_bt_client_notify_cb;
	smp_bt_client_sub_params.subscribe = smp_bt_client_subscribe_cb;

	smp_client_transport_register(&smp_bt_client_entry);
}

MCUMGR_HANDLER_DEFINE(smp_bt_client, smp_bt_client_setup);
