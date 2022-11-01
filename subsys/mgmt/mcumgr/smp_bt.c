/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Bluetooth transport for the mcumgr SMP protocol.
 */

#include <errno.h>

#include <zephyr.h>
#include <init.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <mgmt/mcumgr/smp_bt.h>
#include <mgmt/mcumgr/buf.h>

#include <mgmt/mcumgr/smp.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(smp_bt, CONFIG_MCUMGR_LOG_LEVEL);

struct device;

struct smp_bt_user_data {
	struct bt_conn *conn;
	uint8_t id;
};

/* Verification of user data being able to fit */
BUILD_ASSERT(sizeof(struct smp_bt_user_data) <= CONFIG_MCUMGR_BUF_USER_DATA_SIZE,
	     "CONFIG_MCUMGR_BUF_USER_DATA_SIZE not large enough to fit Bluetooth user data");

struct conn_param_data {
	struct bt_conn *conn;
	uint8_t id;
};

static uint8_t next_id;
static struct zephyr_smp_transport smp_bt_transport;
static struct conn_param_data conn_data[CONFIG_BT_MAX_CONN];

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

/* Helper function that allocates conn_param_data for a conn. */
static struct conn_param_data *conn_param_data_alloc(struct bt_conn *conn)
{
	for (size_t i = 0; i < ARRAY_SIZE(conn_data); i++) {
		if (conn_data[i].conn == NULL) {
			bool valid = false;

			conn_data[i].conn = conn;

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

/**
 * Write handler for the SMP characteristic; processes an incoming SMP request.
 */
static ssize_t smp_bt_chr_write(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
	struct smp_bt_user_data *ud;
	struct net_buf *nb;
	struct conn_param_data *cpd = conn_param_data_get(conn);

	if (cpd == NULL) {
		printk("cpd is null");
		return len;
	}

	nb = mcumgr_buf_alloc();
	if (!nb) {
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}
	net_buf_add_mem(nb, buf, len);

	ud = net_buf_user_data(nb);
	ud->conn = conn;
	ud->id = cpd->id;

	zephyr_smp_rx_req(&smp_bt_transport, nb);

	return len;
}

static void smp_bt_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
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

/**
 * Transmits an SMP response over the specified Bluetooth connection.
 */
static int smp_bt_tx_rsp(struct bt_conn *conn, const void *data, uint16_t len)
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

	return ud->conn;
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

	/* Account for the three-byte notification header. */
	return mtu - 3;
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
static int smp_bt_tx_pkt(struct zephyr_smp_transport *zst, struct net_buf *nb)
{
	struct bt_conn *conn;
	struct smp_bt_user_data *ud = net_buf_user_data(nb);
	int rc;

	conn = smp_bt_conn_from_pkt(nb);
	if (conn == NULL) {
		rc = -1;
	} else {
		struct conn_param_data *cpd = conn_param_data_get(conn);

		if (cpd == NULL) {
			rc = -1;
		} else if (cpd->id == 0 || cpd->id != ud->id) {
			/* Connection has been lost or is a different device */
			rc = -1;
		} else {
			rc = smp_bt_tx_rsp(conn, nb->data, nb->len);
		}
	}

	smp_bt_ud_free(ud);
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
		(void)conn_param_data_alloc(conn);
	}
}

/* BT disconnected callback. */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct conn_param_data *cpd = conn_param_data_get(conn);

	/* Clear cpd. */
	if (cpd != NULL) {
		cpd->id = 0;
		cpd->conn = NULL;
	} else {
		LOG_ERR("Null cpd object for connection %p", (void *)conn);
	}
}

static int smp_bt_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	next_id = 1;

	/* Register BT callbacks */
	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
	};
	bt_conn_cb_register(&conn_callbacks);

	zephyr_smp_transport_init(&smp_bt_transport, smp_bt_tx_pkt,
				  smp_bt_get_mtu, smp_bt_ud_copy,
				  smp_bt_ud_free);
	return 0;
}

SYS_INIT(smp_bt_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
