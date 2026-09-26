/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/atomic.h>

/* Count down number of write commands after all PHY and connection updates */
#define COUNT_THROUGHPUT 1000U

/* Count down number of metrics intervals before performing a PHY update */
#define PHY_UPDATE_COUNTDOWN 5U
static uint32_t phy_update_countdown;

/* Current index of the parameters array to initiate PHY Update */
static uint8_t phy_param_idx;

/* Count down number of metrics intervals before performing a param update */
#define PARAM_UPDATE_COUNTDOWN     PHY_UPDATE_COUNTDOWN
static uint32_t param_update_countdown;

/* Current index of the parameters array to initiate Connection Update */
static uint8_t param_update_idx;

/* If testing PHY Update then perform one iteration of Connection Updates otherwise when testing
 * Connection Updates perform 20 iterations.
 */
#define PARAM_UPDATE_ITERATION_MAX COND_CODE_1(CONFIG_USE_PHY_UPDATE_ITERATION_ONCE, (1U), (20U))
static uint32_t param_update_iteration;

/* Total number of Connection Updates performed
 *
 * Used for logging purposes only
 */
static uint32_t param_update_count;

/* Calculate the Supervision Timeout to a Rounded up 10 ms unit
 *
 * Conform to required BT Specification defined minimum Supervision Timeout of 100 ms
 */
#define CONN_TIMEOUT(_timeout) \
	BT_GAP_US_TO_CONN_TIMEOUT(DIV_ROUND_UP(MAX(100U * USEC_PER_MSEC, (_timeout)), \
					       10U * USEC_PER_MSEC) * 10U * USEC_PER_MSEC)

/* Relaxed number of Connection Interval to set the Supervision Timeout.
 * Shall be >= 2U.
 *
 * Refer to BT Spec v6.1, Vol 6, Part B, Section 4.5.2 Supervision timeout
 *
 * `(1 + connPeripheralLatency) × connSubrateFactor × connInterval × 2`
 */
#define CONN_INTERVAL_MULTIPLIER (6U)

static void phy_update_iterate(struct bt_conn *conn)
{
	const struct bt_conn_le_phy_param phy_param[] = {
		/* List of 1M Tx with Rx on other PHYs */
		{
			.options = BT_CONN_LE_PHY_OPT_NONE,
			.pref_tx_phy = BT_GAP_LE_PHY_1M,
			.pref_rx_phy = BT_GAP_LE_PHY_1M,
		}, {
			.options = BT_CONN_LE_PHY_OPT_NONE,
			.pref_tx_phy = BT_GAP_LE_PHY_1M,
			.pref_rx_phy = BT_GAP_LE_PHY_2M,
		}, {
			.options = BT_CONN_LE_PHY_OPT_NONE,
			.pref_tx_phy = BT_GAP_LE_PHY_1M,
			.pref_rx_phy = BT_GAP_LE_PHY_CODED,
		},

		/* List of 2M Tx with Rx on other PHYs */
		{
			.options = BT_CONN_LE_PHY_OPT_NONE,
			.pref_tx_phy = BT_GAP_LE_PHY_2M,
			.pref_rx_phy = BT_GAP_LE_PHY_1M,
		}, {
			.options = BT_CONN_LE_PHY_OPT_NONE,
			.pref_tx_phy = BT_GAP_LE_PHY_2M,
			.pref_rx_phy = BT_GAP_LE_PHY_2M,
		}, {
			.options = BT_CONN_LE_PHY_OPT_NONE,
			.pref_tx_phy = BT_GAP_LE_PHY_2M,
			.pref_rx_phy = BT_GAP_LE_PHY_CODED,
		},

		/* List of Coded PHY S8 Tx with Rx on other PHYs */
		{
			.options = BT_CONN_LE_PHY_OPT_CODED_S8,
			.pref_tx_phy = BT_GAP_LE_PHY_CODED,
			.pref_rx_phy = BT_GAP_LE_PHY_1M,
		}, {
			.options = BT_CONN_LE_PHY_OPT_CODED_S8,
			.pref_tx_phy = BT_GAP_LE_PHY_CODED,
			.pref_rx_phy = BT_GAP_LE_PHY_2M,
		}, {
			.options = BT_CONN_LE_PHY_OPT_CODED_S8,
			.pref_tx_phy = BT_GAP_LE_PHY_CODED,
			.pref_rx_phy = BT_GAP_LE_PHY_CODED,
		},

		/* List of Coded PHY S2 Tx with Rx on other PHYs */
		{
			.options = BT_CONN_LE_PHY_OPT_CODED_S2,
			.pref_tx_phy = BT_GAP_LE_PHY_CODED,
			.pref_rx_phy = BT_GAP_LE_PHY_1M,
		}, {
			.options = BT_CONN_LE_PHY_OPT_CODED_S2,
			.pref_tx_phy = BT_GAP_LE_PHY_CODED,
			.pref_rx_phy = BT_GAP_LE_PHY_2M,
		}, {
			.options = BT_CONN_LE_PHY_OPT_CODED_S2,
			.pref_tx_phy = BT_GAP_LE_PHY_CODED,
			.pref_rx_phy = BT_GAP_LE_PHY_CODED,
		},

		/* Finally stop at 2M Tx with Rx on 2M */
		{
			.options = BT_CONN_LE_PHY_OPT_NONE,
			.pref_tx_phy = BT_GAP_LE_PHY_2M,
			.pref_rx_phy = BT_GAP_LE_PHY_2M,
		},
	};
	int err;

	if (phy_update_countdown--) {
		return;
	}

	phy_update_countdown = PHY_UPDATE_COUNTDOWN;

	if (phy_param_idx >= ARRAY_SIZE(phy_param)) {
		if (IS_ENABLED(CONFIG_USE_PHY_UPDATE_ITERATION_ONCE)) {
			/* No more PHY updates, stay at the last index */
			return;
		}

		/* Test PHY Update not enabled, lets continue with connection update iterations
		 * forever.
		 */
		phy_param_idx = 0U;
	}

	struct bt_conn_info conn_info;

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		printk("Failed to get connection info (%d).\n", err);
		return;
	}

	struct bt_conn_le_phy_param conn_phy_param;

	if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
		conn_phy_param.options = phy_param[phy_param_idx].options;
		conn_phy_param.pref_tx_phy = phy_param[phy_param_idx].pref_tx_phy;
		conn_phy_param.pref_rx_phy = phy_param[phy_param_idx].pref_rx_phy;
	} else {
		conn_phy_param.options = phy_param[phy_param_idx].options;
		conn_phy_param.pref_tx_phy = phy_param[phy_param_idx].pref_rx_phy;
		conn_phy_param.pref_rx_phy = phy_param[phy_param_idx].pref_tx_phy;
	}

	printk("%s: PHY Update requested %u %u (%u)\n", __func__,
	       conn_phy_param.pref_tx_phy,
	       conn_phy_param.pref_rx_phy,
	       conn_phy_param.options);

	err = bt_conn_le_phy_update(conn, &conn_phy_param);
	if (err) {
		printk("Failed to update PHY (%d).\n", err);
		return;
	}

	phy_param_idx++;
}

/* Interval between storing the measured write rate */
#define METRICS_INTERVAL 1U /* seconds */

static struct bt_gatt_exchange_params mtu_exchange_params;
static uint32_t write_count;
static uint32_t write_len;
static uint32_t write_rate;

/* Globals, reused by central_gatt_write and peripheral_gatt_write samples */
/* Connection context used by the Write Cmd calls */
struct bt_conn *conn_connected;
/* Stores the latest calculated write rate, bits per second */
uint32_t last_write_rate;
/* Number of Write Commands used to record the latest write rate.
 * Has to be large enough to be transmitting packets for METRICS_INTERVAL duration.
 * Assign 0 to continue calculating latest write rate, forever or until disconnection.
 */
uint32_t *write_countdown;
/* Function pointer used to restart scanning on ACL disconnect */
void (*start_scan_func)(void);

static void record_and_step(struct bt_conn *conn, uint16_t len)
{
	static uint32_t cycle_stamp;
	uint64_t delta;

	delta = k_cycle_get_32() - cycle_stamp;
	delta = k_cyc_to_ns_floor64(delta);

	if (delta == 0) {
		/* Skip division by zero */
		return;
	}

	/* if last data rx-ed was greater than 1 second in the past,
	 * reset the metrics.
	 */
	if (delta > (METRICS_INTERVAL * NSEC_PER_SEC)) {
		printk("%s: count= %u, len= %u, rate= %u bps.\n", __func__,
		       write_count, write_len, write_rate);

		last_write_rate = write_rate;

		write_count = 0U;
		write_len = 0U;
		write_rate = 0U;
		cycle_stamp = k_cycle_get_32();

		if (IS_ENABLED(CONFIG_BT_USER_PHY_UPDATE)) {
			phy_update_iterate(conn);
		}

		/* NOTE: Though minimum connection timeout permitted is 100 ms, to avoid supervision
		 *       timeout when observer role is enabled in the sample, keep the timeout for
		 *       smaller connection interval be large enough due to repeated overlaps by the
		 *       scan window.
		 */
		const struct bt_le_conn_param update_params[] = {{
				.interval_min = BT_GAP_US_TO_CONN_INTERVAL(51250U),
				.interval_max = BT_GAP_US_TO_CONN_INTERVAL(51250U),
				.latency = 0,
				.timeout = CONN_TIMEOUT(51250U * CONN_INTERVAL_MULTIPLIER),
			}, {
				.interval_min = BT_GAP_US_TO_CONN_INTERVAL(50000U),
				.interval_max = BT_GAP_US_TO_CONN_INTERVAL(50000U),
				.latency = 0,
				.timeout = CONN_TIMEOUT(50000U * CONN_INTERVAL_MULTIPLIER),
			}, {
				.interval_min = BT_GAP_US_TO_CONN_INTERVAL(8750U),
				.interval_max = BT_GAP_US_TO_CONN_INTERVAL(8750U),
				.latency = 0,
				.timeout = CONN_TIMEOUT(720000U),
			}, {
				.interval_min = BT_GAP_US_TO_CONN_INTERVAL(7500U),
				.interval_max = BT_GAP_US_TO_CONN_INTERVAL(7500U),
				.latency = 0,
				.timeout = CONN_TIMEOUT(720000U),
			}, {
				.interval_min = BT_GAP_US_TO_CONN_INTERVAL(50000U),
				.interval_max = BT_GAP_US_TO_CONN_INTERVAL(50000U),
				.latency = 0,
				.timeout = CONN_TIMEOUT(50000U * CONN_INTERVAL_MULTIPLIER),
			}, {
				.interval_min = BT_GAP_US_TO_CONN_INTERVAL(51250U),
				.interval_max = BT_GAP_US_TO_CONN_INTERVAL(51250U),
				.latency = 0,
				.timeout = CONN_TIMEOUT(51250U * CONN_INTERVAL_MULTIPLIER),
			}, {
				.interval_min = BT_GAP_US_TO_CONN_INTERVAL(7500U),
				.interval_max = BT_GAP_US_TO_CONN_INTERVAL(7500U),
				.latency = 0,
				.timeout = CONN_TIMEOUT(720000U),
			}, {
				.interval_min = BT_GAP_US_TO_CONN_INTERVAL(8750U),
				.interval_max = BT_GAP_US_TO_CONN_INTERVAL(8750U),
				.latency = 0,
				.timeout = CONN_TIMEOUT(720000U),
			}, {
				.interval_min = BT_GAP_US_TO_CONN_INTERVAL(50000U),
				.interval_max = BT_GAP_US_TO_CONN_INTERVAL(50000U),
				.latency = 0,
				.timeout = CONN_TIMEOUT(50000U * CONN_INTERVAL_MULTIPLIER),
			},
		};
		int err;

		if ((param_update_countdown--) != 0U) {
			return;
		}

		param_update_countdown = PARAM_UPDATE_COUNTDOWN;

		if (param_update_idx >= ARRAY_SIZE(update_params)) {
			if (IS_ENABLED(CONFIG_USE_CONN_UPDATE_ITERATION_ONCE) &&
			    (--param_update_iteration == 0U)) {
				/* No more conn updates, stay at the last index */
				param_update_iteration = 1U;

				/* As this function is re-used by the peripheral; on target, users
				 * can enable simultaneous (background) scanning but by default do
				 * not have the scanning enabled.
				 * If both Central plus Peripheral role is built together then
				 * Peripheral is scanning (on 1M and Coded PHY windows) while there
				 * is simultaneous Write Commands.
				 *
				 * We stop scanning if we are stopping after one iteration of
				 * Connection Updates.
				 */
				if (IS_ENABLED(CONFIG_BT_OBSERVER) &&
				    !IS_ENABLED(CONFIG_USE_PHY_UPDATE_ITERATION_ONCE)) {
					/* Stop scanning. We will keep calling on every complete
					 * countdown. This is ok, for implementation simplicity,
					 * i.e. not adding addition design.
					 */
					err = bt_le_scan_stop();
					if (err != 0) {
						printk("Failed to stop scanning (%d).\n", err);
					}

					printk("Scanning stopped.\n");

					*write_countdown = COUNT_THROUGHPUT;
				}

				return;
			}

			/* Test Connection Update not enabled, lets continue with connection update
			 * iterations forever.
			 */
			param_update_idx = 0U;
		}

		param_update_count++;

		printk("Parameter Update Count: %u. %u: 0x%x 0x%x %u %u\n", param_update_count,
		       param_update_idx,
		       update_params[param_update_idx].interval_min,
		       update_params[param_update_idx].interval_max,
		       update_params[param_update_idx].latency,
		       update_params[param_update_idx].timeout);
		err = bt_conn_le_param_update(conn, &update_params[param_update_idx]);
		if (err != 0) {
			printk("Parameter update failed (err %d)\n", err);
		}

		param_update_idx++;

	} else {
		write_count++;
		write_len += len;
		write_rate = ((uint64_t)write_len << 3) * (METRICS_INTERVAL * NSEC_PER_SEC) /
			     delta;
	}
}

static void write_cmd_cb(struct bt_conn *conn, void *user_data)
{
	/* The 16-bit data length is stored in user_data */
	record_and_step(conn, (uint32_t)user_data & 0xFFFF);
}

#if defined(CONFIG_USE_NOTIFY)
/* Vendor specific Throughput Measurement service and Notify characteristic */
#define NOTIFY_SVC_UUID_BYTES \
	0xCC, 0x7B, 0xCB, 0x32, 0x07, 0x08, 0x17, 0xAF, \
	0xD3, 0x43, 0x1E, 0x5D, 0x20, 0x0D, 0xEC, 0x1A
#define NOTIFY_CHRC_UUID_BYTES \
	0x1E, 0x25, 0x21, 0x59, 0x67, 0x84, 0x78, 0x9E, \
	0x30, 0x4D, 0xE9, 0x91, 0x81, 0x13, 0xB0, 0xF7

static struct bt_uuid_128 notify_svc_uuid = BT_UUID_INIT_128(NOTIFY_SVC_UUID_BYTES);
static struct bt_uuid_128 notify_chrc_uuid = BT_UUID_INIT_128(NOTIFY_CHRC_UUID_BYTES);

/* Set by the peripheral's CCC handler when the peer subscribes */
static uint8_t notify_enabled;

/* Number of notifications received by the central (subscriber) */
static atomic_t notify_rx_count;

static void notify_ccc_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	ARG_UNUSED(attr);

	notify_enabled = (value == BT_GATT_CCC_NOTIFY);
	printk("%s: notify %s\n", __func__, notify_enabled ? "enabled" : "disabled");
}

BT_GATT_SERVICE_DEFINE(notify_svc,
	BT_GATT_PRIMARY_SERVICE(&notify_svc_uuid),
	BT_GATT_CHARACTERISTIC(&notify_chrc_uuid.uuid, BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(notify_ccc_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* attrs[1] is the Notify characteristic value attribute */
#define NOTIFY_ATTR (&notify_svc.attrs[1])

static void notify_sent_cb(struct bt_conn *conn, void *user_data)
{
	record_and_step(conn, (uint32_t)user_data & 0xFFFF);
}

/* Peripheral (notifier) transmit path.
 *
 * Returns 0 when a notification was queued, -EAGAIN if the peer has not yet
 * subscribed, or a negative error code otherwise.
 */
int notify_data(struct bt_conn *conn)
{
	static uint8_t data[BT_ATT_MAX_ATTRIBUTE_LEN];
	struct bt_gatt_notify_params params = {0};
	uint16_t len;
	int err;

	if (!notify_enabled) {
		return -EAGAIN;
	}

	len = bt_gatt_get_mtu(conn) - 3U;
	if (len > sizeof(data)) {
		len = sizeof(data);
	}

	params.attr = NOTIFY_ATTR;
	params.data = data;
	params.len = len;
	params.func = notify_sent_cb;
	params.user_data = (void *)((uint32_t)len);

	err = bt_gatt_notify_cb(conn, &params);
	if (err && err != -ENOMEM) {
		printk("%s: Notify failed (%d).\n", __func__, err);
	}

	return err;
}

static uint8_t notify_recv_cb(struct bt_conn *conn,
			      struct bt_gatt_subscribe_params *params,
			      const void *data, uint16_t length)
{
	ARG_UNUSED(params);

	if (data == NULL) {
		/* Unsubscribed */
		return BT_GATT_ITER_STOP;
	}

	atomic_inc(&notify_rx_count);
	record_and_step(conn, length);

	return BT_GATT_ITER_CONTINUE;
}

static struct bt_gatt_discover_params notify_discover_params;
static struct bt_gatt_subscribe_params notify_subscribe_params;

static uint8_t notify_discover_cb(struct bt_conn *conn, const struct bt_gatt_attr *attr,
				  struct bt_gatt_discover_params *params)
{
	int err;

	if (attr == NULL) {
		printk("%s: Notify characteristic not found.\n", __func__);
		return BT_GATT_ITER_STOP;
	}

	if (params->type == BT_GATT_DISCOVER_CHARACTERISTIC) {
		uint16_t value_handle = bt_gatt_attr_value_handle(attr);

		notify_subscribe_params.notify = notify_recv_cb;
		notify_subscribe_params.value = BT_GATT_CCC_NOTIFY;
		notify_subscribe_params.value_handle = value_handle;
		/* CCC descriptor immediately follows the value handle */
		notify_subscribe_params.ccc_handle = value_handle + 1U;

		err = bt_gatt_subscribe(conn, &notify_subscribe_params);
		if (err && err != -EALREADY) {
			printk("%s: Subscribe failed (%d).\n", __func__, err);
		} else {
			printk("%s: Subscribed for notifications.\n", __func__);
		}
	}

	return BT_GATT_ITER_STOP;
}

void notify_subscribe(struct bt_conn *conn)
{
	int err;

	notify_discover_params.uuid = &notify_chrc_uuid.uuid;
	notify_discover_params.func = notify_discover_cb;
	notify_discover_params.start_handle = 0x0001;
	notify_discover_params.end_handle = 0xffff;
	notify_discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

	err = bt_gatt_discover(conn, &notify_discover_params);
	if (err) {
		printk("%s: Discover failed (%d).\n", __func__, err);
	}
}

uint32_t notify_rx_get(void)
{
	return (uint32_t)atomic_get(&notify_rx_count);
}
#endif /* CONFIG_USE_NOTIFY */

static void mtu_exchange_cb(struct bt_conn *conn, uint8_t err,
			    struct bt_gatt_exchange_params *params)
{
	printk("%s: MTU exchange %s (%u)\n", __func__,
	       err == 0U ? "successful" : "failed",
	       bt_gatt_get_mtu(conn));
}

static int mtu_exchange(struct bt_conn *conn)
{
	int err;

	printk("%s: Current MTU = %u\n", __func__, bt_gatt_get_mtu(conn));

	mtu_exchange_params.func = mtu_exchange_cb;

	printk("%s: Exchange MTU...\n", __func__);
	err = bt_gatt_exchange_mtu(conn, &mtu_exchange_params);
	if (err) {
		printk("%s: MTU exchange failed (err %d)", __func__, err);
	}

	return err;
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	struct bt_conn_info conn_info;
	int err;

	if (conn_err) {
		printk("%s: Failed to connect to %s (0x%02x)\n", __func__, bt_conn_dst_str(conn),
		       conn_err);
		return;
	}

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		printk("Failed to get connection info (%d).\n", err);
		return;
	}

	printk("%s: %s role %u\n", __func__, bt_conn_dst_str(conn), conn_info.role);

	conn_connected = bt_conn_ref(conn);

	(void)mtu_exchange(conn);

#if defined(CONFIG_BT_SMP)
	if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
		err = bt_conn_set_security(conn, BT_SECURITY_L2);
		if (err) {
			printk("Failed to set security (%d).\n", err);
		}
	}
#endif

#if defined(CONFIG_USE_NOTIFY)
	if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
		/* Central is the subscriber: discover and subscribe to the
		 * peripheral's notify characteristic.
		 */
		notify_subscribe(conn);
	}
#endif

	if (IS_ENABLED(CONFIG_BT_USER_PHY_UPDATE)) {
		phy_update_countdown = PHY_UPDATE_COUNTDOWN;
		phy_param_idx = 0U;
	}

	/* Every 1 second the acknowledged total GATT Write without Response data size is used for
	 * the throughput calculation.
	 * PHY update is performed in reference to this calculation interval, and connection update
	 * is offset by 1 of this interval so that connection update is initiated one such interval
	 * after PHY update was requested.
	 */
	param_update_countdown = PARAM_UPDATE_COUNTDOWN + 1U;
	param_update_iteration = PARAM_UPDATE_ITERATION_MAX;
	param_update_count = 0U;
	param_update_idx = 0U;
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	struct bt_conn_info conn_info;
	int err;

	err = bt_conn_get_info(conn, &conn_info);
	if (err) {
		printk("Failed to get connection info (%d).\n", err);
		return;
	}

	printk("%s: %s role %u, reason 0x%02x %s\n", __func__, bt_conn_dst_str(conn),
	       conn_info.role, reason, bt_hci_err_to_str(reason));

	conn_connected = NULL;

	bt_conn_unref(conn);

	if (conn_info.role == BT_CONN_ROLE_CENTRAL) {
		start_scan_func();
	}
}

static bool le_param_req(struct bt_conn *conn, struct bt_le_conn_param *param)
{
	printk("%s: int (0x%04x, 0x%04x) lat %u to %u\n", __func__,
	       param->interval_min, param->interval_max, param->latency,
	       param->timeout);

	return true;
}

static void le_param_updated(struct bt_conn *conn, uint16_t interval,
			     uint16_t latency, uint16_t timeout)
{
	printk("%s: int 0x%04x lat %u to %u\n", __func__, interval,
	       latency, timeout);
}

#if defined(CONFIG_BT_SMP)
static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	printk("%s: to level %u, err %s(%u)\n", __func__, level, bt_security_err_to_str(err), err);
}
#endif

#if defined(CONFIG_BT_USER_PHY_UPDATE)
static void le_phy_updated(struct bt_conn *conn,
			   struct bt_conn_le_phy_info *param)
{
	printk("LE PHY Updated: %s Tx 0x%x, Rx 0x%x\n", bt_conn_dst_str(conn), param->tx_phy,
	       param->rx_phy);
}
#endif /* CONFIG_BT_USER_PHY_UPDATE */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
static void le_data_len_updated(struct bt_conn *conn,
				struct bt_conn_le_data_len_info *info)
{
	printk("Data length updated: %s max tx %u (%u us) max rx %u (%u us)\n",
	       bt_conn_dst_str(conn), info->tx_max_len, info->tx_max_time, info->rx_max_len,
	       info->rx_max_time);
}
#endif /* CONFIG_BT_USER_DATA_LEN_UPDATE */

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.le_param_req = le_param_req,
	.le_param_updated = le_param_updated,

#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif

#if defined(CONFIG_BT_USER_PHY_UPDATE)
	.le_phy_updated = le_phy_updated,
#endif /* CONFIG_BT_USER_PHY_UPDATE */

#if defined(CONFIG_BT_USER_DATA_LEN_UPDATE)
	.le_data_len_updated = le_data_len_updated,
#endif /* CONFIG_BT_USER_DATA_LEN_UPDATE */
};

int write_cmd(struct bt_conn *conn)
{
	static uint8_t data[BT_ATT_MAX_ATTRIBUTE_LEN] = {0, };
	static uint16_t data_len;
	uint16_t data_len_max;
	int err;

	data_len_max = bt_gatt_get_mtu(conn) - 3;
	if (data_len_max > BT_ATT_MAX_ATTRIBUTE_LEN) {
		data_len_max = BT_ATT_MAX_ATTRIBUTE_LEN;
	}

	if (IS_ENABLED(CONFIG_USE_VARIABLE_LENGTH_DATA)) {
		/* Use incremental length data for every write command */
		/* TODO: Include test case in BabbleSim tests */
		static bool decrement;

		if (decrement) {
			data_len--;
			if (data_len <= 1) {
				data_len = 1;
				decrement = false;
			}
		} else {
			data_len++;
			if (data_len >= data_len_max) {
				data_len = data_len_max;
				decrement = true;
			}
		}
	} else {
		/* Use fixed length data for every write command */
		data_len = data_len_max;
	}

	/* Pass the 16-bit data length value (instead of reference) in
	 * user_data so that unique value is pass for each write callback.
	 * Using handle 0x0001, we do not care if it is writable, we just want
	 * to transmit the data across.
	 */
	err = bt_gatt_write_without_response_cb(conn, 0x0001, data, data_len,
						false, write_cmd_cb,
						(void *)((uint32_t)data_len));
	if (err) {
		printk("%s: Write cmd failed (%d).\n", __func__, err);
	}

	return err;
}
