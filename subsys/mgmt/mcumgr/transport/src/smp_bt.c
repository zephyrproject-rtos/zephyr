/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright (c) 2022-2026 Nordic Semiconductor ASA
 * Copyright (c) 2026 Jamie McCrae
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Bluetooth server transport for the MCUmgr SMP protocol, and the incoming side of
 * Bluetooth transport management bridges.
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

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT) && defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT)
#include <zephyr/mgmt/mcumgr/grp/transport_mgmt/transport_mgmt.h>
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

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT) && defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT)
/* Incoming side of a bridge, see smp_bt_client.c for the outgoing side */
static struct smp_bt_user_data incoming_bridge_data;
#endif

/* Verification of user data being able to fit */
BUILD_ASSERT(sizeof(struct smp_bt_user_data) <= CONFIG_MCUMGR_TRANSPORT_NETBUF_USER_DATA_SIZE,
	     "CONFIG_MCUMGR_TRANSPORT_NETBUF_USER_DATA_SIZE not large enough to fit Bluetooth"
	     " user data");

enum {
	/* SMP server values */
	CONN_PARAM_SMP_REQUESTED = BIT(0),

	/* Transport management bridge values */
	INCOMING_CONNECTION = BIT(1),
};

struct conn_param_data {
	struct bt_conn *conn;
#if defined(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL)
	struct k_work_delayable dwork;
	struct k_work_delayable ework;
#endif
#if (defined(CONFIG_MCUMGR_GRP_TRANSPORT) && defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT)) ||        \
	defined(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL)
	uint8_t state;
#endif
	uint8_t id;
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

/* Helper function that allocates conn_param_data for a conn. */
static struct conn_param_data *conn_param_data_alloc(struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn_data); i++) {
		if (conn_data[i].conn == NULL && conn_data[i].id == 0) {
			bool valid = false;

			conn_data[i].conn = conn;

#if (defined(CONFIG_MCUMGR_GRP_TRANSPORT) && defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT)) ||        \
	defined(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL)
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
	struct bt_gatt_notify_params params = {
		.attr = (attr_smp_bt_svc + BT_CHARACTERISTIC_ATTRIBUTE_SIZE),
		.data = data,
		.len = len,
#if defined(CONFIG_BT_EATT)
		/* Fragments must arrive in order, which only a single bearer guarantees */
		.chan_opt = BT_ATT_CHAN_OPT_UNENHANCED_ONLY,
#endif
	};

	return bt_gatt_notify_cb(conn, &params);
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
	uint16_t mtu = bt_gatt_get_uatt_mtu(conn);

	if (mtu == 0U) {
		return 0;
	}

	/* Account for the three-byte notification header. */
	return mtu - BT_GATT_MESSAGE_OVERHEAD;
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
	struct conn_param_data *cpd;
	struct smp_bt_user_data *ud;

	conn = smp_bt_conn_from_pkt(nb);
	if (conn == NULL) {
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

		rc = smp_bt_notify(conn, &nb->data[off], mtu_size);
		if (rc != 0) {
			LOG_WRN("SMP notification failed (err %d), dropping response", rc);
			rc = (rc == -ENOTCONN) ? MGMT_ERR_ENOENT : MGMT_ERR_EUNKNOWN;
			break;
		}

		off += mtu_size;
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

/* BT connected callback. */
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err == 0) {
		(void)conn_param_data_alloc(conn);
	}
}

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT) && defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT)
/* Tears down a bridge whose incoming central has disconnected. Runs on the MCUmgr work queue,
 * as the transport_mgmt lock must not be taken on the Bluetooth RX thread: the holder of the
 * lock may wait for a callback that runs there.
 */
static void smp_bt_bridge_lost(struct k_work *work)
{
	ARG_UNUSED(work);

	/* Bridges are connected on this work queue too, and transport_mgmt allows one incoming
	 * bridge per transport, so a new incoming bridge means that the lost one is gone.
	 */
	if (incoming_bridge_data.conn == NULL) {
		(void)transport_mgmt_disconnect_transport(&smp_bt_transport, true, false);
	}
}

static K_WORK_DEFINE(smp_bt_bridge_lost_work, smp_bt_bridge_lost);
#endif

/* BT disconnected callback. */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct conn_param_data *cpd = conn_param_data_get(conn);

	/* Invalidate the session ID before sweeping the queue so that pending
	 * requests of this connection are dropped; cpd->conn must stay set until
	 * after the sweep because the validity check finds the session through it.
	 */
	if (cpd != NULL) {
		cpd->id = 0;
	}

	/* Remove all pending requests from this device which have yet to be processed from the
	 * FIFO (for this specific connection).
	 */
	smp_rx_remove_invalid(&smp_bt_transport, (void *)conn);

	if (cpd != NULL) {
#if defined(CONFIG_MCUMGR_GRP_TRANSPORT) && defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT)
		uint8_t state = cpd->state;
#endif

		/* Clear cpd. */
		cpd->conn = NULL;

#ifdef CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL
		/* Cancel work if ongoing. */
		(void)k_work_cancel_delayable(&cpd->dwork);
		(void)k_work_cancel_delayable(&cpd->ework);
#endif

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT) && defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT)
		if ((state & INCOMING_CONNECTION) != 0) {
			incoming_bridge_data.conn = NULL;
			incoming_bridge_data.id = 0;
		}
#endif

#if (defined(CONFIG_MCUMGR_GRP_TRANSPORT) && defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT)) ||        \
	defined(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL)
		cpd->state = 0;
#endif

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT) && defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT)
		/* transport_mgmt holds its lock while a bridge output waits for the Bluetooth
		 * stack, so the bridge is torn down on the MCUmgr work queue.
		 */
		if ((state & INCOMING_CONNECTION) != 0) {
			(void)k_work_submit_to_queue(smp_get_wq(), &smp_bt_bridge_lost_work);
		}
#endif
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

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT) && defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT)
/* Only the incoming side: the outgoing side is the Bluetooth client transport */
static bool smp_bt_bridge_connect(struct smp_transport_bridge *bridge, bool outgoing,
				  uint32_t mode, bool same_transport, zcbor_state_t *input_data,
				  zcbor_state_t *output_data)
{
	struct cbor_nb_reader *cnr = CONTAINER_OF(input_data, struct cbor_nb_reader, zs[0]);
	struct smp_bt_user_data *ud;
	struct conn_param_data *cpd;

	ud = net_buf_user_data(cnr->nb);
	cpd = conn_param_data_get(ud->conn);

	if (cpd == NULL) {
		LOG_ERR("Invalid CPD for connection: %p", ud->conn);
		smp_add_cmd_err(output_data, MGMT_GROUP_ID_TRANSPORT,
				TRANSPORT_MGMT_ERR_CONNECTION_ESTABLISHMENT_FATAL_ERROR);

		return false;
	}

	cpd->state |= INCOMING_CONNECTION;
	incoming_bridge_data.conn = ud->conn;
	incoming_bridge_data.id = ud->id;

	return true;
}

static void smp_bt_bridge_disconnect(struct smp_transport_bridge *bridge, bool outgoing)
{
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

static int smp_bt_bridge_tx(const struct smp_transport_bridge *bridge, struct net_buf *nb,
			    bool outgoing)
{
	struct smp_bt_user_data *ud = net_buf_user_data(nb);

	ud->conn = incoming_bridge_data.conn;
	ud->id = incoming_bridge_data.id;

	return smp_bt_tx_pkt(nb);
}
#endif

static void smp_bt_setup(void)
{
	int rc;

	next_id = 1;

#if defined(CONFIG_MCUMGR_TRANSPORT_BT_CONN_PARAM_CONTROL)
	conn_param_control_init();
#endif

	smp_bt_transport.functions.output = smp_bt_tx_pkt;
	TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);
	smp_bt_transport.functions.get_mtu = smp_bt_nb_get_mtu;
	TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_DEPRECATED_DECLARATIONS);
	smp_bt_transport.functions.ud_copy = smp_bt_ud_copy;
	smp_bt_transport.functions.ud_free = smp_bt_ud_free;
	smp_bt_transport.functions.query_valid_check = smp_bt_query_valid_check;

#if defined(CONFIG_MCUMGR_GRP_TRANSPORT) && defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT)
	smp_bt_transport.functions.bridge_connect = smp_bt_bridge_connect;
	smp_bt_transport.functions.bridge_disconnect = smp_bt_bridge_disconnect;
	smp_bt_transport.functions.bridge_output = smp_bt_bridge_tx;
#endif

	rc = smp_transport_init(&smp_bt_transport);

	if (IS_ENABLED(CONFIG_MCUMGR_TRANSPORT_BT_DYNAMIC_SVC_REGISTRATION) && rc == 0) {
		rc = smp_bt_register();
	}

	if (rc != 0) {
		LOG_ERR("Bluetooth SMP transport register failed (err %d)", rc);
	}
}

MCUMGR_HANDLER_DEFINE(smp_bt, smp_bt_setup);
