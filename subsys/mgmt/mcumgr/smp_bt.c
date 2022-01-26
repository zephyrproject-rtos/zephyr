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

#define RESTORE_TIME	   COND_CODE_1(CONFIG_MCUMGR_SMP_BT_LATENCY_CONTROL, \
				(CONFIG_MCUMGR_SMP_BT_LATENCY_CONTROL_RESTORE_TIME), (0))
#define RESTORE_RETRY_TIME COND_CODE_1(CONFIG_MCUMGR_SMP_BT_LATENCY_CONTROL, \
				(CONFIG_MCUMGR_SMP_BT_LATENCY_CONTROL_RESTORE_RETRY_TIME), (0))
#define DEFAULT_LATENCY	   COND_CODE_1(CONFIG_MCUMGR_SMP_BT_LATENCY_CONTROL, \
				(CONFIG_MCUMGR_SMP_BT_LATENCY_CONTROL_DEFAULT_LATENCY), (0))


struct smp_bt_user_data {
	struct bt_conn *conn;
};

enum {
	CONN_LOW_LATENCY_ENABLED	= BIT(0),
	CONN_LOW_LATENCY_REQUIRED	= BIT(1),
};

struct conn_param_data {
	struct bt_conn *conn;
	struct k_work_delayable dwork;
	uint8_t latency_state;
};

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
static struct conn_param_data *alloc_conn_param_data(struct bt_conn *conn)
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
static struct conn_param_data *get_conn_param_data(const struct bt_conn *conn)
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
static void set_conn_latency(struct bt_conn *conn, bool low_latency)
{
	struct bt_le_conn_param params;
	struct conn_param_data *cpd;
	struct bt_conn_info info;
	int ret = 0;

	cpd = get_conn_param_data(conn);

	ret = bt_conn_get_info(conn, &info);
	__ASSERT_NO_MSG(!ret);

	if ((low_latency && (info.le.latency == 0)) ||
	    ((!low_latency) && (info.le.latency == DEFAULT_LATENCY))) {
		/* Already updated. */
		return;
	}

	params.interval_min = info.le.interval;
	params.interval_max = info.le.interval;
	params.latency = (low_latency) ? (0) : (DEFAULT_LATENCY);
	params.timeout = info.le.timeout;

	ret = bt_conn_le_param_update(cpd->conn, &params);
	if (ret && (ret != -EALREADY)) {
		if (!low_latency) {
			/* Try again to avoid stucking in low latency. */
			(void)k_work_reschedule(&cpd->dwork, K_MSEC(RESTORE_RETRY_TIME));
		}
	}
}


/* Work handler function for restoring the default latency for the connection. */
static void restore_default_latency(struct k_work *work)
{
	struct conn_param_data *cpd;

	cpd = CONTAINER_OF(work, struct conn_param_data, dwork);

	if (cpd->latency_state & CONN_LOW_LATENCY_REQUIRED) {
		cpd->latency_state &= ~CONN_LOW_LATENCY_REQUIRED;
		(void)k_work_reschedule(&cpd->dwork, K_MSEC(RESTORE_TIME));
	} else {
		set_conn_latency(cpd->conn, false);
	}
}

static void enable_low_latency(struct bt_conn *conn)
{
	struct conn_param_data *cpd = get_conn_param_data(conn);

	if (cpd->latency_state & CONN_LOW_LATENCY_ENABLED) {
		cpd->latency_state |= CONN_LOW_LATENCY_REQUIRED;
	} else {
		set_conn_latency(conn, true);
	}
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

	nb = mcumgr_buf_alloc();
	if (!nb) {
		return BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES);
	}
	net_buf_add_mem(nb, buf, len);

	ud = net_buf_user_data(nb);
	ud->conn = bt_conn_ref(conn);

	if (IS_ENABLED(CONFIG_MCUMGR_SMP_BT_LATENCY_CONTROL)) {
		enable_low_latency(conn);
	}

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
static int smp_bt_tx_pkt(struct zephyr_smp_transport *zst, struct net_buf *nb)
{
	struct bt_conn *conn;
	int rc;

	conn = smp_bt_conn_from_pkt(nb);
	if (conn == NULL) {
		rc = -1;
	} else {
		rc = smp_bt_notify(conn, nb->data, nb->len);
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
		alloc_conn_param_data(conn);
	}
}

/* BT disconnected callback. */
static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct conn_param_data *cpd = get_conn_param_data(conn);

	/* Cancel work if ongoing. */
	(void)k_work_cancel_delayable(&cpd->dwork);

	/* Clear cpd. */
	cpd->latency_state = 0;
	cpd->conn = NULL;
}

/* BT LE connection parameters updated callback. */
static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	struct conn_param_data *cpd = get_conn_param_data(conn);

	if (latency == 0) {
		cpd->latency_state |= CONN_LOW_LATENCY_ENABLED;
		cpd->latency_state &= ~CONN_LOW_LATENCY_REQUIRED;
		(void)k_work_reschedule(&cpd->dwork, K_MSEC(RESTORE_TIME));
	} else {
		cpd->latency_state &= ~CONN_LOW_LATENCY_ENABLED;
		(void)k_work_cancel_delayable(&cpd->dwork);
	}
}

static void init_latency_control_support(void)
{
	/* Register BT callbacks */
	static struct bt_conn_cb conn_callbacks = {
		.connected = connected,
		.disconnected = disconnected,
		.le_param_updated = le_param_updated,
	};
	bt_conn_cb_register(&conn_callbacks);

	for (size_t i = 0; i < ARRAY_SIZE(conn_data); i++) {
		k_work_init_delayable(&conn_data[i].dwork, restore_default_latency);
	}
}

static int smp_bt_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	if (IS_ENABLED(CONFIG_MCUMGR_SMP_BT_LATENCY_CONTROL)) {
		init_latency_control_support();
	}

	zephyr_smp_transport_init(&smp_bt_transport, smp_bt_tx_pkt,
				  smp_bt_get_mtu, smp_bt_ud_copy,
				  smp_bt_ud_free);
	return 0;
}

SYS_INIT(smp_bt_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
