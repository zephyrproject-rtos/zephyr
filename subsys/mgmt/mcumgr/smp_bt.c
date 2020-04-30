/*
 * Copyright Runtime.io 2018. All rights reserved.
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

struct device;

struct smp_bt_user_data {
	struct bt_conn *conn;
};

static struct zephyr_smp_transport smp_bt_transport;

/* SMP service.
 * {8D53DC1D-1DB7-4CD3-868B-8A527460AA84}
 */
static struct bt_uuid_128 smp_bt_svc_uuid = BT_UUID_INIT_128(
	0x84, 0xaa, 0x60, 0x74, 0x52, 0x8a, 0x8b, 0x86,
	0xd3, 0x4c, 0xb7, 0x1d, 0x1d, 0xdc, 0x53, 0x8d);

/* SMP characteristic; used for both requests and responses.
 * {DA2E7828-FBCE-4E01-AE9E-261174997C48}
 */
static struct bt_uuid_128 smp_bt_chr_uuid = BT_UUID_INIT_128(
	0x48, 0x7c, 0x99, 0x74, 0x11, 0x26, 0x9e, 0xae,
	0x01, 0x4e, 0xce, 0xfb, 0x28, 0x78, 0x2e, 0xda);

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

	nb = mcumgr_buf_alloc();
	net_buf_add_mem(nb, buf, len);

	ud = net_buf_user_data(nb);
	ud->conn = bt_conn_ref(conn);

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
static int smp_bt_tx_pkt(struct zephyr_smp_transport *zst, struct net_buf *nb)
{
	struct bt_conn *conn;
	int rc;

	conn = smp_bt_conn_from_pkt(nb);
	if (conn == NULL) {
		rc = -1;
	} else {
		rc = smp_bt_tx_rsp(conn, nb->data, nb->len);
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

static int smp_bt_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	zephyr_smp_transport_init(&smp_bt_transport, smp_bt_tx_pkt,
				  smp_bt_get_mtu, smp_bt_ud_copy,
				  smp_bt_ud_free);
	return 0;
}

SYS_INIT(smp_bt_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
