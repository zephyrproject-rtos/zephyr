/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright (c) 2022-2026 Nordic Semiconductor ASA
 * Copyright (c) 2026 Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Bluetooth transport for the MCUmgr SMP protocol.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>
#include <zephyr/mgmt/mcumgr/mgmt/handlers.h>
#include <errno.h>

#include <mgmt/mcumgr/transport/smp_internal.h>
#include <mgmt/mcumgr/transport/smp_reassembly.h>

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
#include <zephyr/mgmt/mcumgr/grp/transport_mgmt/transport_mgmt.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>
#include <zcbor_common.h>
#include <zcbor_decode.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(mcumgr_smp, CONFIG_MCUMGR_TRANSPORT_LOG_LEVEL);

#define RESTORE_TIME COND_CODE_1(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL, \
				 (CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL_RESTORE_TIME), \
				 (0))
#define RETRY_TIME COND_CODE_1(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL, \
			       (CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL_RETRY_TIME), \
			       (0))

#define CONN_PARAM_SMP COND_CODE_1(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL,		\
				   BT_LE_CONN_PARAM(						\
					CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL_MIN_INT,  \
					CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL_MAX_INT,  \
					CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL_LATENCY,  \
					CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL_TIMEOUT), \
					(NULL))
#define CONN_PARAM_PREF	COND_CODE_1(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL,	\
				    BT_LE_CONN_PARAM(					\
					CONFIG_BT_PERIPHERAL_PREF_MIN_INT,		\
					CONFIG_BT_PERIPHERAL_PREF_MAX_INT,		\
					CONFIG_BT_PERIPHERAL_PREF_LATENCY,		\
					CONFIG_BT_PERIPHERAL_PREF_TIMEOUT),		\
				    (NULL))

/* Permission levels for GATT characteristics of the SMP service. */
#ifndef CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW_AUTHEN
#define CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW_AUTHEN 0
#endif
#ifndef CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW_ENCRYPT
#define CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW_ENCRYPT 0
#endif
#ifndef CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW
#define CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW 0
#endif

#define SMP_GATT_PERM (							\
	CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW_AUTHEN ?			\
	(BT_GATT_PERM_READ_AUTHEN | BT_GATT_PERM_WRITE_AUTHEN) :	\
	CONFIG_MCUMGR_TRANSPORT_BT_PERM_RW_ENCRYPT ?			\
	(BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT) :	\
	(BT_GATT_PERM_READ | BT_GATT_PERM_WRITE))			\

#define SMP_GATT_PERM_WRITE_MASK \
	(BT_GATT_PERM_WRITE | BT_GATT_PERM_WRITE_ENCRYPT | BT_GATT_PERM_WRITE_AUTHEN)

/* Minimum number of bytes that must be able to be sent with a notification to a target device
 * before giving up
 */
#define SMP_BT_MINIMUM_MTU_SEND_FAILURE 20

#define BT_SERVICE_ATTRIBUTE_SIZE 1
#define BT_CHARACTERISTIC_ATTRIBUTE_SIZE 2
#define BT_GATT_MESSAGE_OVERHEAD 3

static ssize_t smp_bt_chr_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset, uint8_t flags);

static void smp_bt_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value);

#define SMP_BT_ATTRS									\
	BT_GATT_PRIMARY_SERVICE(SMP_BT_SVC_UUID),					\
	BT_GATT_CHARACTERISTIC(SMP_BT_CHR_UUID,						\
			       BT_GATT_CHRC_WRITE_WITHOUT_RESP |			\
			       BT_GATT_CHRC_NOTIFY,					\
			       SMP_GATT_PERM & SMP_GATT_PERM_WRITE_MASK,		\
			       NULL, smp_bt_chr_write, NULL),				\
	BT_GATT_CCC(smp_bt_ccc_changed,							\
		    SMP_GATT_PERM),

#ifdef CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL
/* Verification of SMP Connection Parameters configuration that is not possible in the Kconfig. */
BUILD_ASSERT((CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL_TIMEOUT * 4U) >
	     ((1U + CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL_LATENCY) *
	      CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL_MAX_INT));
#endif

struct smp_bt_user_data {
	struct bt_conn *conn;
	uint8_t id;
};

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
static uint8_t smp_bt_bridge_discover(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				      struct bt_gatt_discover_params *params);

static uint8_t smp_bt_bridge_notify(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
				    const void *data, uint16_t len);

static struct smp_bt_user_data incoming_bridge_data;

static const struct bt_uuid_128 bt_uuid_smp_svc = BT_UUID_INIT_128(SMP_BT_SVC_UUID_VAL);
static const struct bt_uuid_128 bt_uuid_smp_chr = BT_UUID_INIT_128(SMP_BT_CHR_UUID_VAL);
static const struct bt_uuid_16 bt_uuid_smp_ccc = BT_UUID_INIT_16(BT_UUID_GATT_CCC_VAL);
static struct bt_gatt_discover_params bt_smp_discover_params = {
	.func = smp_bt_bridge_discover,
	.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
};

static struct bt_gatt_subscribe_params bt_smp_subscribe_params = {
	.value = BT_GATT_CCC_NOTIFY,
	.notify = smp_bt_bridge_notify,
};
static bool outgoing_connection_was_successful;
#endif

/* Verification of user data being able to fit */
BUILD_ASSERT(sizeof(struct smp_bt_user_data) <= CONFIG_MCUMGR_TRANSPORT_NETBUF_USER_DATA_SIZE,
	     "CONFIG_MCUMGR_TRANSPORT_NETBUF_USER_DATA_SIZE not large enough to fit Bluetooth"
	     " user data");

enum {
	/* SMP server values */
	CONN_PARAM_SMP_REQUESTED = BIT(0),

	/* SMP client values */
	INCOMING_CONNECTION = BIT(1),
	OUTGOING_CONNECTION = BIT(2),
	CONNECTED = BIT(3),
	DISCOVERED = BIT(4),
};

struct conn_param_data {
	struct bt_conn *conn;
#if defined(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL)
	struct k_work_delayable dwork;
	struct k_work_delayable ework;
#endif
#if defined(CONFIG_MCUMGR_GRP_TRANSPORT) || defined(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL)
	uint8_t state;
#endif
	uint8_t id;
	struct k_sem smp_notify_sem;
};

static uint8_t next_id;
static struct smp_transport smp_bt_transport;
static struct conn_param_data conn_data[CONFIG_BT_MAX_CONN];

static void connected(struct bt_conn *conn, uint8_t err);
static void disconnected(struct bt_conn *conn, uint8_t reason);

#ifdef CONFIG_MCUMGR_TRANSPORT_BT_DYNAMIC_SVC_REGISTRATION
static struct bt_gatt_attr attr_smp_bt_svc[] = {SMP_BT_ATTRS};
static struct bt_gatt_service smp_bt_svc = BT_GATT_SERVICE(attr_smp_bt_svc);
#else
BT_GATT_SERVICE_DEFINE(smp_bt_svc, SMP_BT_ATTRS);
#endif

/* Bluetooth connection callback handlers */
BT_CONN_CB_DEFINE(mcumgr_bt_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

#if defined(CONFIG_SMP_CLIENT) || defined(CONFIG_MCUMGR_GRP_TRANSPORT)
static struct smp_client_transport_entry smp_client_transport = {
	.smpt = &smp_bt_transport,
	.smpt_type = SMP_BLUETOOTH_TRANSPORT,
#ifdef CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS
	.name = "Bluetooth",
#endif
};
#endif

/* Helper function that allocates conn_param_data for a conn. */
static struct conn_param_data *conn_param_data_alloc(struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn_data); i++) {
		if (conn_data[i].conn == NULL && conn_data[i].id == 0) {
			bool valid = false;

			conn_data[i].conn = conn;

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT) || defined(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL)
			conn_data[i].state = 0;
#endif

			/* Generate an ID for this connection and reset semaphore */
			while (!valid) {
				valid = true;
				conn_data[i].id = next_id;
				++next_id;

				if (next_id == 0) {
					/* Avoid use of 0 (invalid ID) */
					++next_id;
				}

				for (size_t l = 0; l < ARRAY_SIZE(conn_data); l++) {
					if (l != i && conn_data[l].conn != NULL &&
					    conn_data[l].id == conn_data[i].id) {
						valid = false;
						break;
					}
				}
			}

			k_sem_reset(&conn_data[i].smp_notify_sem);

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

	return NULL;
}

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
/* Helper function that returns conn_param_data for an outgoing connection. */
static struct conn_param_data *outgoing_conn_param_data_get(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn_data); i++) {
		if ((conn_data[i].state & OUTGOING_CONNECTION) != 0 && conn_data[i].id != 0) {
			return &conn_data[i];
		}
	}

	return NULL;
}
#endif

/* SMP Bluetooth notification sent callback */
static void smp_notify_finished(struct bt_conn *conn, void *user_data)
{
	struct conn_param_data *cpd = conn_param_data_get(conn);

	__ASSERT(cpd != NULL, "Invalid CPD for connection: %p", conn);

	if (cpd != NULL) {
		k_sem_give(&cpd->smp_notify_sem);
	}
}

#if defined(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL)
/* Sets connection parameters for a given conn. */
static void conn_param_set(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	struct conn_param_data *cpd = conn_param_data_get(conn);

	if (cpd != NULL) {
		int ret = bt_conn_le_param_update(conn, param);
		if (ret && (ret != -EALREADY)) {
			/* Try again to avoid being stuck with incorrect connection parameters. */
			(void)k_work_reschedule(&cpd->ework, K_MSEC(RETRY_TIME));
		} else {
			(void)k_work_cancel_delayable(&cpd->ework);
		}
	}
}

/* Work handler function for restoring the preferred connection parameters for the connection. */
static void conn_param_on_pref_restore(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct conn_param_data *cpd = CONTAINER_OF(dwork, struct conn_param_data, dwork);

	if (cpd != NULL) {
		conn_param_set(cpd->conn, CONN_PARAM_PREF);
		cpd->state &= ~CONN_PARAM_SMP_REQUESTED;
	}
}

/* Work handler function for retrying on conn negotiation API error. */
static void conn_param_on_error_retry(struct k_work *work)
{
	struct k_work_delayable *ework = k_work_delayable_from_work(work);
	struct conn_param_data *cpd = CONTAINER_OF(ework, struct conn_param_data, ework);
	struct bt_le_conn_param *param = (cpd->state & CONN_PARAM_SMP_REQUESTED) ?
		CONN_PARAM_SMP : CONN_PARAM_PREF;

	conn_param_set(cpd->conn, param);
}

static void conn_param_smp_enable(struct bt_conn *conn)
{
	struct conn_param_data *cpd = conn_param_data_get(conn);

	if (cpd != NULL) {
		if (!(cpd->state & CONN_PARAM_SMP_REQUESTED)) {
			conn_param_set(conn, CONN_PARAM_SMP);
			cpd->state |= CONN_PARAM_SMP_REQUESTED;
		}

		/* SMP characteristic in use; refresh the restore timeout. */
		(void)k_work_reschedule(&cpd->dwork, K_MSEC(RESTORE_TIME));
	}
}
#endif

/**
 * Write handler for the SMP characteristic; processes an incoming SMP request.
 */
static ssize_t smp_bt_chr_write(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	struct conn_param_data *cpd = conn_param_data_get(conn);
#ifdef CONFIG_MCUMGR_TRANSPORT_BT_REASSEMBLY
	int ret;
	bool started;

	if (cpd == NULL) {
		LOG_ERR("NULL cpd object for connection %p", (void *)conn);
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	started = (smp_reassembly_expected(&smp_bt_transport) >= 0);

	LOG_DBG("started = %s, buf len = %d", started ? "true" : "false", len);
	LOG_HEXDUMP_DBG(buf, len, "buf = ");

	ret = smp_reassembly_collect(&smp_bt_transport, buf, len);
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
			(struct smp_bt_user_data *)smp_reassembly_get_ud(&smp_bt_transport);

		if (ud != NULL) {
			ud->conn = NULL;
			ud->id = 0;
		}

		smp_reassembly_drop(&smp_bt_transport);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}

	if (!started) {
		/*
		 * Transport context is attached to the buffer after first fragment
		 * has been collected.
		 */
		struct smp_bt_user_data *ud = smp_reassembly_get_ud(&smp_bt_transport);

#if defined(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL)
		conn_param_smp_enable(conn);
#endif

		ud->conn = conn;
		ud->id = cpd->id;
	}

	/* No more bytes are expected for this packet */
	if (ret == 0) {
		smp_reassembly_complete(&smp_bt_transport, false);
	}

	/* BT expects entire len to be consumed */
	return len;
#else
	struct smp_bt_user_data *ud;
	struct net_buf *nb;

	if (cpd == NULL) {
		LOG_ERR("NULL cpd object for connection %p", (void *)conn);
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	nb = smp_packet_alloc();
	if (!nb) {
		LOG_DBG("failed net_buf alloc for SMP packet");
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	if (net_buf_tailroom(nb) < len) {
		LOG_DBG("SMP packet len (%" PRIu16 ") > net_buf len (%zu)",
			len, net_buf_tailroom(nb));
		smp_packet_free(nb);
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}

	net_buf_add_mem(nb, buf, len);

	ud = net_buf_user_data(nb);
	ud->conn = conn;
	ud->id = cpd->id;

#if defined(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL)
	conn_param_smp_enable(conn);
#endif

	smp_rx_req(&smp_bt_transport, nb);

	return len;
#endif
}

static void smp_bt_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
#ifdef CONFIG_MCUMGR_TRANSPORT_BT_REASSEMBLY
	if (smp_reassembly_expected(&smp_bt_transport) >= 0 && value == 0) {
		struct smp_bt_user_data *ud = smp_reassembly_get_ud(&smp_bt_transport);

		ud->conn = NULL;
		ud->id = 0;

		smp_reassembly_drop(&smp_bt_transport);
	}
#endif
}

int smp_bt_notify(struct bt_conn *conn, const void *data, uint16_t len)
{
	return bt_gatt_notify(conn, (attr_smp_bt_svc + BT_CHARACTERISTIC_ATTRIBUTE_SIZE), data,
			      len);
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

	return ud->conn;
}

/**
 * Calculates the maximum fragment size to use when sending the specified
 * response packet.
 */
static uint16_t smp_bt_conn_get_mtu(struct bt_conn *conn)
{
	/* Account for the three-byte notification header. */
	return bt_gatt_get_mtu(conn) - BT_GATT_MESSAGE_OVERHEAD;
}

static uint16_t smp_bt_nb_get_mtu(const struct net_buf *nb)
{
	struct bt_conn *conn;

	conn = smp_bt_conn_from_pkt(nb);
	if (conn == NULL) {
		return 0;
	}

	return smp_bt_conn_get_mtu(conn);
}

static void smp_bt_ud_free(void *ud)
{
	struct smp_bt_user_data *user_data = ud;

	if (user_data->conn) {
		user_data->conn = NULL;
		user_data->id = 0;
	}
}

static int smp_bt_ud_copy(struct net_buf *dst, const struct net_buf *src)
{
	struct smp_bt_user_data *src_ud = net_buf_user_data(src);
	struct smp_bt_user_data *dst_ud = net_buf_user_data(dst);

	if (src_ud->conn) {
		dst_ud->conn = src_ud->conn;
		dst_ud->id = src_ud->id;
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
		.attr = (attr_smp_bt_svc + BT_CHARACTERISTIC_ATTRIBUTE_SIZE),
		.func = smp_notify_finished,
		.data = nb->data,
	};
	bool sent = false;
	struct bt_conn_info info;
	struct conn_param_data *cpd;
	struct smp_bt_user_data *ud;

	conn = smp_bt_conn_from_pkt(nb);
	if (conn == NULL) {
		rc = MGMT_ERR_ENOENT;
		goto cleanup;
	}

	/* Verify that the device is connected, the necessity for this check is that the remote
	 * device might have sent a command and disconnected before the command has been processed
	 * completely, if this happens then the connection details will still be valid due to
	 * the incremented connection reference count, but the connection has actually been
	 * dropped, this avoids waiting for a semaphore that will never be given which would
	 * otherwise cause a deadlock.
	 */
	rc = bt_conn_get_info(conn, &info);

	if (rc != 0 || info.state != BT_CONN_STATE_CONNECTED) {
		/* Remote device has disconnected */
		rc = MGMT_ERR_ENOENT;
		goto cleanup;
	}

	/* Send data in chunks of the MTU size */
	mtu_size = smp_bt_conn_get_mtu(conn);

	if (mtu_size == 0U) {
		/* The transport cannot support a transmission right now. */
		rc = MGMT_ERR_EUNKNOWN;
		goto cleanup;
	}

	cpd = conn_param_data_get(conn);
	ud = net_buf_user_data(nb);

	if (cpd == NULL || cpd->id == 0 || cpd->id != ud->id) {
		/* The device that sent this packet has disconnected or is not the same active
		 * connection, drop the outgoing data
		 */
		rc = MGMT_ERR_ENOENT;
		goto cleanup;
	}

	k_sem_reset(&cpd->smp_notify_sem);

	while (off < nb->len) {
		if (cpd->id == 0 || cpd->id != ud->id) {
			/* The device that sent this packet has disconnected or is not the same
			 * active connection, drop the outgoing data
			 */
			rc = MGMT_ERR_ENOENT;
			goto cleanup;
		}

		if ((off + mtu_size) > nb->len) {
			/* Final packet, limit size */
			mtu_size = nb->len - off;
		}

		notify_param.len = mtu_size;
		rc = bt_gatt_notify_cb(conn, &notify_param);

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

			/* Wait for the completion (or disconnect) semaphore before
			 * continuing, allowing other parts of the system to run.
			 */
			k_sem_take(&cpd->smp_notify_sem, K_FOREVER);
		} else {
			/* No connection, cannot continue */
			rc = MGMT_ERR_EUNKNOWN;
			break;
		}
	}

cleanup:
	smp_bt_ud_free(net_buf_user_data(nb));
	smp_packet_free(nb);

	return rc;
}

#ifdef CONFIG_MCUMGR_TRANSPORT_BT_DYNAMIC_SVC_REGISTRATION
int smp_bt_register(void)
{
	return bt_gatt_service_register(&smp_bt_svc);
}

int smp_bt_unregister(void)
{
	return bt_gatt_service_unregister(&smp_bt_svc);
}
#endif

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
static uint8_t smp_bt_bridge_notify(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
				    const void *data, uint16_t len)
{
	struct conn_param_data *cpd = conn_param_data_get(conn);

#ifdef CONFIG_MCUMGR_TRANSPORT_BT_REASSEMBLY
	int ret;
	bool started;
	const uint8_t *buf = (uint8_t *)data;

	if (!data) {
		LOG_INF("Outgoing CCC changed to 0");
		params->value_handle = 0U;
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return BT_GATT_ITER_STOP;
	}

	if (cpd == NULL) {
		LOG_ERR("NULL cpd object for connection %p", conn);
		return BT_GATT_ITER_STOP;
	}

	started = (smp_reassembly_expected(&smp_bt_transport) >= 0);

	LOG_DBG("started = %s, buf len = %d", started ? "true" : "false", len);
	LOG_HEXDUMP_DBG(buf, len, "buf = ");

	ret = smp_reassembly_collect(&smp_bt_transport, buf, len);
	LOG_DBG("collect = %d", ret);

	/*
	 * Collection can fail only due to failing to allocate memory or by receiving
	 * more data than expected.
	 */
	if (ret == -ENOMEM) {
		/* Failed to collect the buffer */
		return BT_GATT_ITER_CONTINUE;
	} else if (ret < 0) {
		/* Failed operation on already allocated buffer, drop the packet and report
		 * error.
		 */
		struct smp_bt_user_data *ud =
			(struct smp_bt_user_data *)smp_reassembly_get_ud(&smp_bt_transport);

		if (ud != NULL) {
			ud->conn = NULL;
			ud->id = 0;
		}

		smp_reassembly_drop(&smp_bt_transport);
		return BT_GATT_ITER_CONTINUE;
	}

	if (!started) {
		/*
		 * Transport context is attached to the buffer after first fragment
		 * has been collected.
		 */
		struct smp_bt_user_data *ud = smp_reassembly_get_ud(&smp_bt_transport);

		ud->conn = conn;
		ud->id = cpd->id;
	}

	/* No more bytes are expected for this packet */
	if (ret == 0) {
		smp_reassembly_complete(&smp_bt_transport, false);
	}
#else
	struct smp_bt_user_data *ud;
	struct net_buf *nb;

	if (!data) {
		LOG_INF("Outgoing CCC changed to 0");
		params->value_handle = 0U;
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		return BT_GATT_ITER_STOP;
	}

	if (cpd == NULL) {
		LOG_ERR("NULL cpd object for connection %p", (void *)conn);
		return BT_GATT_ITER_CONTINUE;
	}

	nb = smp_packet_alloc();
	if (!nb) {
		LOG_DBG("failed net_buf alloc for SMP packet");
		return BT_GATT_ITER_CONTINUE;
	}

	if (net_buf_tailroom(nb) < len) {
		LOG_DBG("SMP packet len (%" PRIu16 ") > net_buf len (%zu)",
			len, net_buf_tailroom(nb));
		smp_packet_free(nb);
		return BT_GATT_ITER_CONTINUE;
	}

	net_buf_add_mem(nb, (const uint8_t *)data, len);

	ud = net_buf_user_data(nb);
	ud->conn = conn;
	ud->id = cpd->id;

	smp_rx_req(&smp_bt_transport, nb);
#endif

	return BT_GATT_ITER_CONTINUE;
}

static uint8_t smp_bt_bridge_discover(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				      struct bt_gatt_discover_params *params)
{
	int rc;
	struct conn_param_data *cpd = conn_param_data_get(conn);

	if (cpd == NULL) {
		LOG_ERR("Invalid CPD for connection: %p", conn);
		(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	}

	if (!attr) {
		if ((cpd->state & DISCOVERED) == 0) {
			/* Missing service or characteristics, disconnect the device */
			LOG_ERR("Service discovery finished, missing SMP service");
			(void)bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
		} else {
			LOG_DBG("Service discovery finished");
		}

		return BT_GATT_ITER_STOP;
	}

	if (!bt_uuid_cmp(bt_smp_discover_params.uuid, SMP_BT_SVC_UUID)) {
		bt_smp_discover_params.uuid = &bt_uuid_smp_chr.uuid;
		bt_smp_discover_params.start_handle = attr->handle + BT_SERVICE_ATTRIBUTE_SIZE;
		bt_smp_discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

		rc = bt_gatt_discover(conn, &bt_smp_discover_params);

		if (rc != 0) {
			LOG_ERR("SMP characteristic discovery failed: %d", rc);
			outgoing_connection_was_successful = false;
			k_sem_give(&cpd->smp_notify_sem);
		}
	} else if (!bt_uuid_cmp(bt_smp_discover_params.uuid, SMP_BT_CHR_UUID)) {
		bt_smp_discover_params.uuid = &bt_uuid_smp_ccc.uuid;
		bt_smp_discover_params.start_handle = attr->handle +
							BT_CHARACTERISTIC_ATTRIBUTE_SIZE;
		bt_smp_discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
		bt_smp_subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

		rc = bt_gatt_discover(conn, &bt_smp_discover_params);

		if (rc != 0) {
			LOG_ERR("SMP CCCD discovery failed: %d", rc);
			outgoing_connection_was_successful = false;
			k_sem_give(&cpd->smp_notify_sem);
		}
	} else {
		bt_smp_subscribe_params.ccc_handle = attr->handle;

		rc = bt_gatt_subscribe(conn, &bt_smp_subscribe_params);

		if (rc != 0 && rc != -EALREADY) {
			LOG_ERR("SMP CCCD subscription failed: %d", rc);
			outgoing_connection_was_successful = false;
			k_sem_give(&cpd->smp_notify_sem);
		} else {
			LOG_DBG("SMP CCCD subscription done");
			outgoing_connection_was_successful = true;
			k_sem_give(&cpd->smp_notify_sem);
		}
	}

	return BT_GATT_ITER_STOP;
}
#endif

/* BT connected callback. */
static void connected(struct bt_conn *conn, uint8_t err)
{
#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
	struct conn_param_data *cpd = NULL;
	int rc;

	if (err == 0) {
		cpd = conn_param_data_get(conn);

		if (cpd == NULL) {
			cpd = conn_param_data_alloc(conn);
		}
	} else {
		cpd = conn_param_data_get(conn);

		if (cpd == NULL) {
			LOG_INF("NULL cpd object for failed connection %p", (void *)conn);
			return;
		}

		if ((cpd->state & OUTGOING_CONNECTION) != 0) {
			LOG_ERR("Failed to connect to: %s (%u)", bt_conn_dst_str(conn), err);
			k_sem_give(&cpd->smp_notify_sem);
			bt_conn_drop(&conn);
			outgoing_connection_was_successful = false;
		}

		return;
	}

	if ((cpd->state & OUTGOING_CONNECTION) == 0) {
		return;
	}

	/* Set up outgoing connection for service discovery */
	cpd->state |= CONNECTED;
	LOG_INF("Connected to: %s (%p)", bt_conn_dst_str(conn), conn);

	bt_smp_discover_params.uuid = &bt_uuid_smp_svc.uuid;
	bt_smp_discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
	bt_smp_discover_params.type = BT_GATT_DISCOVER_PRIMARY;

	rc = bt_gatt_discover(conn, &bt_smp_discover_params);

	if (rc != 0) {
		LOG_ERR("SMP service discovery failed: %d", rc);
		outgoing_connection_was_successful = false;
		k_sem_give(&cpd->smp_notify_sem);
	}
#else
	if (err == 0) {
		(void)conn_param_data_alloc(conn);
	}
#endif
}

/* BT disconnected callback. */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct conn_param_data *cpd = conn_param_data_get(conn);

	/* Remove all pending requests from this device which have yet to be processed from the
	 * FIFO (for this specific connection).
	 */
	smp_rx_remove_invalid(&smp_bt_transport, (void *)conn);

	/* Force giving the notification semaphore here, this is only needed if there is a pending
	 * outgoing packet when the device has disconnected, as in this case the notification
	 * callback will not be called and this is needed to prevent a deadlock.
	 */
	if (cpd != NULL) {
		/* Clear cpd. */
		cpd->id = 0;
		cpd->conn = NULL;

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
		if ((cpd->state & OUTGOING_CONNECTION) != 0) {
			if ((cpd->state & CONNECTED) == 0) {
				return;
			}

			bt_conn_drop(&conn);

			(void)transport_mgmt_disconnect_transport(&smp_bt_transport, false, true);
		} else {
#endif
#ifdef CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL
			/* Cancel work if ongoing. */
			(void)k_work_cancel_delayable(&cpd->dwork);
			(void)k_work_cancel_delayable(&cpd->ework);
#endif

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
			if ((cpd->state & INCOMING_CONNECTION) != 0) {
				incoming_bridge_data.conn = NULL;
				incoming_bridge_data.id = 0;
				(void)transport_mgmt_disconnect_transport(&smp_bt_transport, true,
									  false);
			}
		}
#endif

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT) || defined(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL)
		cpd->state = 0;
#endif

		k_sem_give(&cpd->smp_notify_sem);
	} else {
		LOG_ERR("NULL cpd object for connection %p", (void *)conn);
	}
}

#if defined(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL)
static void conn_param_control_init(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn_data); i++) {
		k_work_init_delayable(&conn_data[i].dwork, conn_param_on_pref_restore);
		k_work_init_delayable(&conn_data[i].ework, conn_param_on_error_retry);
	}
}
#endif

static bool smp_bt_query_valid_check(struct net_buf *nb, void *arg)
{
	const struct bt_conn *conn = (struct bt_conn *)arg;
	struct smp_bt_user_data *ud = net_buf_user_data(nb);
	struct conn_param_data *cpd;

	if (conn == NULL || ud == NULL) {
		return false;
	}

	cpd = conn_param_data_get(conn);

	if (cpd == NULL || (ud->conn == conn && cpd->id != ud->id)) {
		return false;
	}

	return true;
}

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
static bool smp_bt_bridge_connect(struct smp_transport_bridge *bridge, bool outgoing,
				  uint32_t mode, bool same_transport, zcbor_state_t *input_data,
				  zcbor_state_t *output_data)
{
	struct cbor_nb_reader *cnr = CONTAINER_OF(input_data, struct cbor_nb_reader, zs[0]);
	struct smp_bt_user_data *ud;
	struct conn_param_data *cpd;

#if CONFIG_BT_MAX_CONN == 1
	/*
	 * With only 1 supported connection, the device cannot support an incoming and outgoing
	 * connection, therefore shortcut early to an error
	 */
	if (same_transport) {
		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_SAME_BRIDGE_DEVICE_DISALLOWED);
		return false;
	}
#endif

	if (outgoing) {
		uint32_t address_type = 0;
		struct bt_conn_le_create_param *create_param = BT_CONN_LE_CREATE_CONN;
		struct bt_le_conn_param *connection_param = BT_LE_CONN_PARAM_DEFAULT;
		int decoded = 0;
		int rc;
		struct zcbor_string address = { 0 };
		bt_addr_le_t addr;
		uint8_t address_buffer[BT_ADDR_STR_LEN];
		bool le_coded = false;
		bool ok;

		struct zcbor_map_decode_key_val bt_bride_connect_decode[] = {
			ZCBOR_MAP_DECODE_KEY_DECODER("address_type", zcbor_uint32_decode,
						     &address_type),
			ZCBOR_MAP_DECODE_KEY_DECODER("address", zcbor_tstr_decode, &address),
			ZCBOR_MAP_DECODE_KEY_DECODER("le_coded", zcbor_bool_decode, &le_coded),
		};

		ok = zcbor_map_decode_bulk(input_data, bt_bride_connect_decode,
					   ARRAY_SIZE(bt_bride_connect_decode), &decoded) == 0;

		if (!ok || decoded < 1 || !zcbor_map_decode_bulk_key_found(bt_bride_connect_decode,
						ARRAY_SIZE(bt_bride_connect_decode), "address")) {
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
		address_buffer[BT_ADDR_STR_LEN - 1] = 0;
		rc = bt_addr_from_str(address_buffer, &addr.a);

		if (rc != 0) {
			LOG_ERR("Failed to convert address: %d", rc);
			smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
					TRANSPORT_MGMT_ERR_CONNECT_INVALID_PARAMETER);
			return false;
		}

		/* Reserve a connection parameter object now so that the semaphore is available
		 * for use, the ID will be assigned after the connection is initiated
		 */
		cpd = conn_param_data_alloc(NULL);

		if (cpd == NULL) {
			LOG_ERR("Failed to allocate cpd object");
			smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
					TRANSPORT_MGMT_ERR_UNKNOWN);
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

		cpd->state |= OUTGOING_CONNECTION;

		rc = bt_conn_le_create(&addr, create_param, connection_param, &cpd->conn);

		if (rc != 0) {
			LOG_ERR("Connection failed: %d", rc);
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

		k_sem_take(&cpd->smp_notify_sem, K_FOREVER);

		if (outgoing_connection_was_successful == false) {
			cpd->state = 0;
			cpd->id = 0;
			cpd->conn = NULL;
		} else {
			cpd->state |= DISCOVERED;
		}

		return outgoing_connection_was_successful;
	}

	ud = net_buf_user_data(cnr->nb);
	cpd = conn_param_data_get(ud->conn);

	if (cpd == NULL) {
		LOG_ERR("Invalid CPD for connection: %p", ud->conn);
		return false;
	}

	cpd->state |= INCOMING_CONNECTION;
	incoming_bridge_data.conn = ud->conn;
	incoming_bridge_data.id = ud->id;

	return true;
}

static void smp_bt_bridge_disconnect(struct smp_transport_bridge *bridge, bool outgoing)
{
	if (outgoing == true) {
		struct conn_param_data *cpd = outgoing_conn_param_data_get();

		if (cpd != NULL) {
			int rc = bt_conn_disconnect(cpd->conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);

			if (rc != 0) {
				/* Clear cpd. */
				cpd->id = 0;
				cpd->conn = NULL;
				cpd->state = 0;
				bt_conn_drop(&cpd->conn);
				k_sem_give(&cpd->smp_notify_sem);
				LOG_ERR("Failed to disconnect BT MCUmgr outgoing bridge: %d", rc);
			}
		}
	} else {
		struct conn_param_data *cpd;

		if (incoming_bridge_data.conn == NULL) {
			return;
		}

		cpd = conn_param_data_get(incoming_bridge_data.conn);

		if (cpd != NULL) {
			cpd->state &= ~INCOMING_CONNECTION;
		}

		incoming_bridge_data.conn = NULL;
		incoming_bridge_data.id = 0;
	}
}

static void smp_bt_gatt_written(struct bt_conn *conn, void *user_data)
{
	struct conn_param_data *cpd = conn_param_data_get(conn);

	__ASSERT(cpd != NULL, "Invalid CPD for connection: %p", conn);

	if (cpd != NULL) {
		k_sem_give(&cpd->smp_notify_sem);
	}
}

static int smp_bt_bridge_tx(const struct smp_transport_bridge *bridge, struct net_buf *nb,
			    bool outgoing)
{
	struct smp_bt_user_data *ud;

	if (outgoing == true) {
		int rc;
		struct conn_param_data *cpd = outgoing_conn_param_data_get();
		uint16_t mtu_size;
		uint16_t off = 0;
		struct bt_conn_info info;
		bool sent = false;

		if (cpd == NULL) {
			LOG_ERR("NULL cpd object for outgoing connection");
			rc = MGMT_ERR_BRIDGED_CONNECTION_UNAVAILABLE;
			goto outgoing_cleanup;
		}

		if ((cpd->state & CONNECTED) == 0 || (cpd->state & DISCOVERED) == 0) {
			LOG_INF("BT bridge connection setup ongoing");
			rc = MGMT_ERR_BRIDGED_CONNECTION_UNAVAILABLE;
			goto outgoing_cleanup;
		}

		/* Verify that the device is connected, the necessity for this check is that the
		 * remote device might have sent a command and disconnected before the command has
		 * been processed completely, if this happens then the connection details will
		 * still be valid due to the incremented connection reference count, but the
		 * connection has actually been dropped, this avoids waiting for a semaphore that
		 * will never be given which would otherwise cause a deadlock.
		 */
		rc = bt_conn_get_info(cpd->conn, &info);

		if (rc != 0 || info.state != BT_CONN_STATE_CONNECTED) {
			rc = MGMT_ERR_BRIDGED_CONNECTION_UNAVAILABLE;
			goto outgoing_cleanup;
		}

		mtu_size = smp_bt_conn_get_mtu(cpd->conn);

		if (mtu_size == 0U) {
			/* The transport cannot support a transmission right now. */
			rc = MGMT_ERR_EUNKNOWN;
			goto outgoing_cleanup;
		}

		while (off < nb->len) {
			if (cpd->id == 0 || (cpd->state & (OUTGOING_CONNECTION | CONNECTED |
					     DISCOVERED)) != (OUTGOING_CONNECTION | CONNECTED |
							      DISCOVERED)) {
				/* The device that sent this packet has disconnected or is not the
				 * same active connection, drop the outgoing data
				 */
				rc = MGMT_ERR_ENOENT;
				goto outgoing_cleanup;
			}

			if ((off + mtu_size) > nb->len) {
				/* Final packet, limit size */
				mtu_size = nb->len - off;
			}

			rc = bt_gatt_write_without_response_cb(cpd->conn,
							bt_smp_subscribe_params.value_handle,
							&nb->data[off], mtu_size, false,
							smp_bt_gatt_written, NULL);

			if (rc == -ENOMEM) {
				if (sent == false) {
					/* Failed to send a packet thus far, try reducing the MTU
					 * size as perhaps the buffer size is limited to a value
					 * which is less than the MTU or there is a configuration
					 * error in the project
					 */
					if (mtu_size < SMP_BT_MINIMUM_MTU_SEND_FAILURE) {
						/* If unable to send a 20 byte message, something
						 * is amiss, no point in continuing
						 */
						rc = MGMT_ERR_ENOMEM;
						break;
					}

					mtu_size /= 2;
				}

				/* No buffers available, wait until the next loop for them to
				 * become available
				 */
				rc = MGMT_ERR_EOK;
				k_yield();
			} else if (rc == 0) {
				off += mtu_size;
				sent = true;

				/* Wait for the completion (or disconnect) semaphore before
				 * continuing, allowing other parts of the system to run.
				 */
				k_sem_take(&cpd->smp_notify_sem, K_FOREVER);
			} else {
				/* No connection, cannot continue */
				rc = MGMT_ERR_EUNKNOWN;
				break;
			}
		}

outgoing_cleanup:
		smp_packet_free(nb);
		return rc;
	}

	ud = net_buf_user_data(nb);

	ud->conn = incoming_bridge_data.conn;
	ud->id = incoming_bridge_data.id;

	return smp_bt_tx_pkt(nb);
}

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS)
static bool smp_bt_bridge_modes(zcbor_state_t *output_data, int *rc)
{
	bool ok;

	ok = zcbor_map_start_encode(output_data, 2) &&
	     zcbor_tstr_put_lit(output_data, "type") &&
	     zcbor_uint32_put(output_data, 0) &&
	     zcbor_tstr_put_lit(output_data, "description") &&
	     zcbor_tstr_put_lit(output_data, "Bluetooth Low Energy") &&
	     zcbor_tstr_put_lit(output_data, "incoming") &&
	     zcbor_bool_put(output_data, true) &&
	     zcbor_tstr_put_lit(output_data, "outgoing") &&
	     zcbor_bool_put(output_data, true) &&
	     zcbor_map_end_encode(output_data, 2);

	*rc = MGMT_RETURN_CHECK(ok);
	return ok;
}

static bool smp_bt_bridge_config_details(uint32_t mode, zcbor_state_t *output_data, int *rc)
{
	bool ok;

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
	     zcbor_tstr_put_lit(output_data, "required") &&
	     zcbor_bool_put(output_data, true) &&
	     zcbor_map_end_encode(output_data, 3);

	return MGMT_RETURN_CHECK(ok);
}
#endif
#endif

static void smp_bt_setup(void)
{
	int rc;
	uint8_t i = 0;

	next_id = 1;

#if defined(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL)
	conn_param_control_init();
#endif

	while (i < CONFIG_BT_MAX_CONN) {
		k_sem_init(&conn_data[i].smp_notify_sem, 0, 1);
		++i;
	}

	smp_bt_transport.functions.output = smp_bt_tx_pkt;
	smp_bt_transport.functions.get_mtu = smp_bt_nb_get_mtu;
	smp_bt_transport.functions.ud_copy = smp_bt_ud_copy;
	smp_bt_transport.functions.ud_free = smp_bt_ud_free;
	smp_bt_transport.functions.query_valid_check = smp_bt_query_valid_check;

#ifdef CONFIG_MCUMGR_GRP_TRANSPORT
	smp_bt_transport.functions.bridge_connect = smp_bt_bridge_connect;
	smp_bt_transport.functions.bridge_disconnect = smp_bt_bridge_disconnect;
	smp_bt_transport.functions.bridge_output = smp_bt_bridge_tx;
#if defined(CONFIG_MCUMGR_GRP_TRANSPORT_INFO_FUNCTIONS)
	smp_bt_transport.functions.bridge_modes = smp_bt_bridge_modes;
	smp_bt_transport.functions.bridge_config_details = smp_bt_bridge_config_details;
#endif
#endif

	rc = smp_transport_init(&smp_bt_transport);

	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_BT_DYNAMIC_SVC_REGISTRATION) && rc == 0) {
		rc = smp_bt_register();
	}

#if defined(CONFIG_SMP_CLIENT) || defined(CONFIG_MCUMGR_GRP_TRANSPORT)
	if (rc == 0) {
		smp_client_transport_register(&smp_client_transport);
	}
#endif

	if (rc != 0) {
		LOG_ERR("Bluetooth SMP transport register failed (err %d)", rc);
	}
}

MCUMGR_HANDLER_DEFINE(smp_bt, smp_bt_setup);
