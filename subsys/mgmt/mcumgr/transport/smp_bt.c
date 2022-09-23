/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Bluetooth transport for the mcumgr SMP protocol.
 */

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/mgmt/mcumgr/smp_bt.h>
#include <zephyr/mgmt/mcumgr/buf.h>

#include <zephyr/mgmt/mcumgr/smp.h>
#include <mgmt/mgmt.h>
#include "../smp_internal.h"
#include "../smp_reassembly.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(mcumgr_smp, CONFIG_MCUMGR_SMP_LOG_LEVEL);

#define RESTORE_TIME	COND_CODE_1(CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL, \
				(CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL_RESTORE_TIME), \
				(0))
#define RETRY_TIME	COND_CODE_1(CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL, \
				(CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL_RETRY_TIME), \
				(0))

#define CONN_PARAM_SMP	COND_CODE_1(CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL,		  \
				BT_LE_CONN_PARAM(					  \
					CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL_MIN_INT,  \
					CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL_MAX_INT,  \
					CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL_LATENCY,  \
					CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL_TIMEOUT), \
					(NULL))
#define CONN_PARAM_PREF	COND_CODE_1(CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL, \
				BT_LE_CONN_PARAM(			     \
					CONFIG_BT_PERIPHERAL_PREF_MIN_INT,   \
					CONFIG_BT_PERIPHERAL_PREF_MAX_INT,   \
					CONFIG_BT_PERIPHERAL_PREF_LATENCY,   \
					CONFIG_BT_PERIPHERAL_PREF_TIMEOUT),  \
				(NULL))

/* Minimum number of bytes that must be able to be sent with a notification to a target device
 * before giving up
 */
#define SMP_BT_MINIMUM_MTU_SEND_FAILURE 20

#ifdef CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL
/* Verification of SMP Connection Parameters configuration that is not possible in the Kconfig. */
BUILD_ASSERT((CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL_TIMEOUT * 4U) >
	     ((1U + CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL_LATENCY) *
	      CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL_MAX_INT));
#endif

struct smp_bt_user_data {
	struct bt_conn *conn;
};

enum {
	CONN_PARAM_SMP_REQUESTED = BIT(0),
};

struct conn_param_data {
	struct bt_conn *conn;
	struct k_work_delayable dwork;
	struct k_work_delayable ework;
	uint8_t state;
};

static struct zephyr_smp_transport smp_bt_transport;
static struct conn_param_data conn_data[CONFIG_BT_MAX_CONN];

K_SEM_DEFINE(smp_notify_sem, 0, 1);

/* SMP service.
 * {8D53DC1D-1DB7-4CD3-868B-8A527460AA84}
 */
static struct bt_uuid_128 smp_bt_svc_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0x8d53dc1d, 0x1db7, 0x4cd3, 0x868b, 0x8a527460aa84));

/* SMP characteristic; used for both requests and responses.
 * {DA2E7828-FBCE-4E01-AE9E-261174997C48}
 */
static struct bt_uuid_128 smp_bt_chr_uuid = BT_UUID_INIT_128(
	BT_UUID_128_ENCODE(0xda2e7828, 0xfbce, 0x4e01, 0xae9e, 0x261174997c48));

/* SMP Bluetooth notification sent callback */
static void smp_notify_finished(struct bt_conn *conn, void *user_data)
{
	k_sem_give(&smp_notify_sem);
}

/* Helper function that allocates conn_param_data for a conn. */
static struct conn_param_data *conn_param_data_alloc(struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn_data); i++) {
		if (conn_data[i].conn == NULL) {
			conn_data[i].conn = conn;
			return &conn_data[i];
		}
	}

	/* Conn data must exists. */
	__ASSERT_NO_MSG(false);
	return NULL;
}

/* Helper function that returns conn_param_data associated with a conn. */
static struct conn_param_data *conn_param_data_get(const struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn_data); i++) {
		if (conn_data[i].conn == conn) {
			return &conn_data[i];
		}
	}

	/* Conn data must exists. */
	__ASSERT_NO_MSG(false);
	return NULL;
}

/* Sets connection parameters for a given conn. */
static void conn_param_set(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	int ret = 0;
	struct conn_param_data *cpd = conn_param_data_get(conn);

	ret = bt_conn_le_param_update(conn, param);
	if (ret && (ret != -EALREADY)) {
		/* Try again to avoid being stuck with incorrect connection parameters. */
		(void)k_work_reschedule(&cpd->ework, K_MSEC(RETRY_TIME));
	} else {
		(void)k_work_cancel_delayable(&cpd->ework);
	}
}


/* Work handler function for restoring the preferred connection parameters for the connection. */
static void conn_param_on_pref_restore(struct k_work *work)
{
	struct conn_param_data *cpd = CONTAINER_OF(work, struct conn_param_data, dwork);

	conn_param_set(cpd->conn, CONN_PARAM_PREF);
	cpd->state &= ~CONN_PARAM_SMP_REQUESTED;
}

/* Work handler function for retrying on conn negotiation API error. */
static void conn_param_on_error_retry(struct k_work *work)
{
	struct conn_param_data *cpd = CONTAINER_OF(work, struct conn_param_data, ework);
	struct bt_le_conn_param *param = (cpd->state & CONN_PARAM_SMP_REQUESTED) ?
		CONN_PARAM_SMP : CONN_PARAM_PREF;

	conn_param_set(cpd->conn, param);
}

static void conn_param_smp_enable(struct bt_conn *conn)
{
	struct conn_param_data *cpd = conn_param_data_get(conn);

	if (!(cpd->state & CONN_PARAM_SMP_REQUESTED)) {
		conn_param_set(conn, CONN_PARAM_SMP);
		cpd->state |= CONN_PARAM_SMP_REQUESTED;
	}

	/* SMP characteristic in use; refresh the restore timeout. */
	(void)k_work_reschedule(&cpd->dwork, K_MSEC(RESTORE_TIME));
}

/**
 * Write handler for the SMP characteristic; processes an incoming SMP request.
 */
static ssize_t smp_bt_chr_write(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
#ifdef CONFIG_MCUMGR_SMP_REASSEMBLY_BT
	int ret;
	bool started;

	started = (zephyr_smp_reassembly_expected(&smp_bt_transport) >= 0);

	LOG_DBG("started = %s, buf len = %d", started ? "true" : "false", len);
	LOG_HEXDUMP_DBG(buf, len, "buf = ");

	ret = zephyr_smp_reassembly_collect(&smp_bt_transport, buf, len);
	LOG_DBG("collect = %d", ret);

	/*
	 * Collection can fail only due to failing to allocate memory or by receiving
	 * more data than expected.
	 */
	if (ret == -ENOMEM) {
		/* Failed to collect the buffer */
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	} else if (ret < 0) {
		/* Failed operation on already allocated buffer, drop the packet and report
		 * error.
		 */
		struct smp_bt_user_data *ud =
			(struct smp_bt_user_data *)zephyr_smp_reassembly_get_ud(&smp_bt_transport);

		if (ud != NULL) {
			bt_conn_unref(ud->conn);
			ud->conn = NULL;
		}

		zephyr_smp_reassembly_drop(&smp_bt_transport);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	if (!started) {
		/*
		 * Transport context is attached to the buffer after first fragment
		 * has been collected.
		 */
		struct smp_bt_user_data *ud = zephyr_smp_reassembly_get_ud(&smp_bt_transport);

		if (IS_ENABLED(CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL)) {
			conn_param_smp_enable(conn);
		}

		ud->conn = bt_conn_ref(conn);
	}

	/* No more bytes are expected for this packet */
	if (ret == 0) {
		zephyr_smp_reassembly_complete(&smp_bt_transport, false);
	}

	/* BT expects entire len to be consumed */
	return len;
#else
	struct smp_bt_user_data *ud;
	struct net_buf *nb;

	nb = mcumgr_buf_alloc();
	if (!nb) {
		LOG_DBG("failed net_buf alloc for SMP packet");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	if (net_buf_tailroom(nb) < len) {
		LOG_DBG("SMP packet len (%" PRIu16 ") > net_buf len (%zu)",
			len, net_buf_tailroom(nb));
		mcumgr_buf_free(nb);
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	net_buf_add_mem(nb, buf, len);

	ud = net_buf_user_data(nb);
	ud->conn = bt_conn_ref(conn);

	if (IS_ENABLED(CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL)) {
		conn_param_smp_enable(conn);
	}

	zephyr_smp_rx_req(&smp_bt_transport, nb);

	return len;
#endif
}

static void smp_bt_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
#ifdef CONFIG_MCUMGR_SMP_REASSEMBLY_BT
	if (zephyr_smp_reassembly_expected(&smp_bt_transport) >= 0 && value == 0) {
		struct smp_bt_user_data *ud = zephyr_smp_reassembly_get_ud(&smp_bt_transport);

		bt_conn_unref(ud->conn);
		ud->conn = NULL;

		zephyr_smp_reassembly_drop(&smp_bt_transport);
	}
#endif
}

static struct bt_gatt_attr smp_bt_attrs[] = {
	/* SMP Primary Service Declaration */
	BT_GATT_PRIMARY_SERVICE(&smp_bt_svc_uuid),

	BT_GATT_CHARACTERISTIC(&smp_bt_chr_uuid.uuid,
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP |
			       BT_GATT_CHRC_NOTIFY,
#ifdef CONFIG_MCUMGR_SMP_BT_AUTHEN
			       BT_GATT_PERM_WRITE_AUTHEN,
#else
			       BT_GATT_PERM_WRITE,
#endif
			       NULL, smp_bt_chr_write, NULL),
	BT_GATT_CCC(smp_bt_ccc_changed,
#ifdef CONFIG_MCUMGR_SMP_BT_AUTHEN
			       BT_GATT_PERM_READ_AUTHEN |
			       BT_GATT_PERM_WRITE_AUTHEN),
#else
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
#endif
};

static struct bt_gatt_service smp_bt_svc = BT_GATT_SERVICE(smp_bt_attrs);

int smp_bt_notify(struct bt_conn *conn, const void *data, uint16_t len)
{
	return bt_gatt_notify(conn, smp_bt_attrs + 2, data, len);
}

/**
 * Extracts the Bluetooth connection from a net_buf's user data.
 */
static struct bt_conn *smp_bt_conn_from_pkt(const struct net_buf *nb)
{
	struct smp_bt_user_data *ud = net_buf_user_data(nb);

	if (!ud->conn) {
		return NULL;
	}

	return bt_conn_ref(ud->conn);
}

/**
 * Calculates the maximum fragment size to use when sending the specified
 * response packet.
 */
static uint16_t smp_bt_get_mtu(const struct net_buf *nb)
{
	struct bt_conn *conn;
	uint16_t mtu;

	conn = smp_bt_conn_from_pkt(nb);
	if (conn == NULL) {
		return 0;
	}

	mtu = bt_gatt_get_mtu(conn);
	bt_conn_unref(conn);

	/* Account for the three-byte notification header. */
	return mtu - 3;
}

static void smp_bt_ud_free(void *ud)
{
	struct smp_bt_user_data *user_data = ud;

	if (user_data->conn) {
		bt_conn_unref(user_data->conn);
		user_data->conn = NULL;
	}
}

static int smp_bt_ud_copy(struct net_buf *dst, const struct net_buf *src)
{
	struct smp_bt_user_data *src_ud = net_buf_user_data(src);
	struct smp_bt_user_data *dst_ud = net_buf_user_data(dst);

	if (src_ud->conn) {
		dst_ud->conn = bt_conn_ref(src_ud->conn);
	}

	return 0;
}

/**
 * Transmits the specified SMP response.
 */
static int smp_bt_tx_pkt(struct net_buf *nb)
{
	struct bt_conn *conn;
	int rc = MGMT_ERR_EOK;
	uint16_t off = 0;
	uint16_t mtu_size;
	struct bt_gatt_notify_params notify_param = {
		.attr = smp_bt_attrs + 2,
		.func = smp_notify_finished,
		.data = nb->data,
	};
	bool sent = false;

	conn = smp_bt_conn_from_pkt(nb);
	if (conn == NULL) {
		rc = MGMT_ERR_ENOENT;
		goto cleanup;
	}

	/* Send data in chunks of the MTU size */
	mtu_size = smp_bt_get_mtu(nb);

	if (mtu_size == 0U) {
		/* The transport cannot support a transmission right now. */
		rc = MGMT_ERR_EUNKNOWN;
		goto cleanup;
	}

	k_sem_reset(&smp_notify_sem);

	while (off < nb->len) {
		if ((off + mtu_size) > nb->len) {
			/* Final packet, limit size */
			mtu_size = nb->len - off;
		}

		notify_param.len = mtu_size;

		rc = bt_gatt_notify_cb(conn, &notify_param);
		k_sem_take(&smp_notify_sem, K_FOREVER);

		if (rc == -ENOMEM) {
			if (sent == false) {
				/* Failed to send a packet thus far, try reducing the MTU size
				 * as perhaps the buffer size is limited to a value which is
				 * less than the MTU or there is a configuration error in the
				 * project
				 */
				if (mtu_size < SMP_BT_MINIMUM_MTU_SEND_FAILURE) {
					/* If unable to send a 20 byte message, something is
					 * amiss, no point in continuing
					 */
					rc = MGMT_ERR_ENOMEM;
					break;
				}

				mtu_size /= 2;
			}

			/* No buffers available, wait until the next loop for them to become
			 * available
			 */
			rc = MGMT_ERR_EOK;
			k_yield();
		} else if (rc == 0) {
			off += mtu_size;
			notify_param.data = &nb->data[off];
			sent = true;
		} else {
			rc = MGMT_ERR_EUNKNOWN;
			break;
		}
	}

cleanup:
	if (rc != MGMT_ERR_ENOENT) {
		bt_conn_unref(conn);
	}

	smp_bt_ud_free(net_buf_user_data(nb));
	mcumgr_buf_free(nb);

	return rc;
}

int smp_bt_register(void)
{
	return bt_gatt_service_register(&smp_bt_svc);
}

int smp_bt_unregister(void)
{
	return bt_gatt_service_unregister(&smp_bt_svc);
}

/* BT connected callback. */
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err == 0) {
		conn_param_data_alloc(conn);
	}
}

/* BT disconnected callback. */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct conn_param_data *cpd = conn_param_data_get(conn);

	/* Cancel work if ongoing. */
	(void)k_work_cancel_delayable(&cpd->dwork);
	(void)k_work_cancel_delayable(&cpd->ework);

	/* Clear cpd. */
	cpd->state = 0;
	cpd->conn = NULL;
}

static void conn_param_control_init(void)
{
	/* Register BT callbacks */
	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
	};
	bt_conn_cb_register(&conn_callbacks);

	for (size_t i = 0; i < ARRAY_SIZE(conn_data); i++) {
		k_work_init_delayable(&conn_data[i].dwork, conn_param_on_pref_restore);
		k_work_init_delayable(&conn_data[i].ework, conn_param_on_error_retry);
	}
}

static int smp_bt_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	if (IS_ENABLED(CONFIG_MCUMGR_SMP_BT_CONN_PARAM_CONTROL)) {
		conn_param_control_init();
	}

	zephyr_smp_transport_init(&smp_bt_transport, smp_bt_tx_pkt,
				  smp_bt_get_mtu, smp_bt_ud_copy,
				  smp_bt_ud_free);
	return 0;
}

SYS_INIT(smp_bt_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
