/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "host/att_internal.h"
#include "host/conn_internal.h"
#include "host/hci_core.h"
#include "host/l2cap_internal.h"

#define DT_DRV_COMPAT zephyr_bt_hci_test
#define SYNC_HANDLE 0x0100U
#define WAIT_TIMEOUT K_SECONDS(1)

struct connection_result {
	struct bt_conn *conn;
	bt_conn_state_t state;
	uint8_t err;
};

static struct connection_result results[CONFIG_BT_MAX_CONN];
static struct bt_le_ext_adv *advertisers[CONFIG_BT_EXT_ADV_MAX_ADV_SET];
static atomic_t connected_count;
static atomic_t phy_reads;
static atomic_t disconnect_in_phy;
static atomic_t rx_held;
static atomic_t pending_packets;
static uint16_t packet_handle;

static K_SEM_DEFINE(connected_sem, 0, CONFIG_BT_MAX_CONN);
static K_SEM_DEFINE(recycled_sem, 0, CONFIG_BT_MAX_CONN);
static K_SEM_DEFINE(rx_held_sem, 0, 1);
static K_SEM_DEFINE(rx_release_sem, 0, 1);
static K_SEM_DEFINE(rx_drained_sem, 0, 1);
static K_SEM_DEFINE(notification_sem, 0, 1);

static const bt_addr_le_t peer = {
	.type = BT_ADDR_LE_PUBLIC,
	.a.val = {0x11U, 0x22U, 0x33U, 0x44U, 0x55U, 0x66U},
};

BT_GATT_SERVICE_DEFINE(test_service,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_16(0xfff0)),
	BT_GATT_CHARACTERISTIC(BT_UUID_DECLARE_16(0xfff1), BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL)
);

static void event_header(struct net_buf *buf, uint8_t event, uint8_t length)
{
	struct bt_hci_evt_hdr *hdr = net_buf_add(buf, sizeof(*hdr));

	hdr->evt = event;
	hdr->len = length;
}

static void event_send(uint8_t event, const void *parameters, uint8_t length)
{
	struct net_buf *buf = bt_buf_get_evt(event, false, K_FOREVER);

	zassert_not_null(buf);
	event_header(buf, event, length);
	net_buf_add_mem(buf, parameters, length);
	zassert_ok(bt_hci_recv_err(DEVICE_DT_INST_GET(0), buf));
}

static void le_event_send(uint8_t subevent, const void *parameters, uint8_t length)
{
	struct net_buf *buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, false, K_FOREVER);

	zassert_not_null(buf);
	event_header(buf, BT_HCI_EVT_LE_META_EVENT, length + 1U);
	net_buf_add_u8(buf, subevent);
	net_buf_add_mem(buf, parameters, length);
	zassert_ok(bt_hci_recv_err(DEVICE_DT_INST_GET(0), buf));
}

static void disconnect_send(uint16_t handle, uint8_t reason)
{
	const struct bt_hci_evt_disconn_complete event = {
		.status = BT_HCI_ERR_SUCCESS,
		.handle = sys_cpu_to_le16(handle),
		.reason = reason,
	};

	event_send(BT_HCI_EVT_DISCONN_COMPLETE, &event, sizeof(event));
}

static void *command_complete(uint16_t opcode, uint8_t length, struct net_buf **buf)
{
	struct bt_hci_evt_cmd_complete *event;
	void *parameters;

	*buf = bt_buf_get_evt(BT_HCI_EVT_CMD_COMPLETE, false, K_FOREVER);
	zassert_not_null(*buf);
	event_header(*buf, BT_HCI_EVT_CMD_COMPLETE, sizeof(*event) + length);
	event = net_buf_add(*buf, sizeof(*event));
	event->ncmd = 1U;
	event->opcode = sys_cpu_to_le16(opcode);
	parameters = net_buf_add(*buf, length);
	memset(parameters, 0, length);

	return parameters;
}

static void command_handle(const struct device *dev, struct net_buf *command)
{
	struct bt_hci_cmd_hdr *hdr = net_buf_pull_mem(command, sizeof(*hdr));
	uint16_t opcode = sys_le16_to_cpu(hdr->opcode);
	struct net_buf *reply;

	switch (opcode) {
	case BT_HCI_OP_READ_LOCAL_FEATURES: {
		struct bt_hci_rp_read_local_features *rp;

		rp = command_complete(opcode, sizeof(*rp), &reply);
		rp->features[4] = BIT(5) | BIT(6);
		break;
	}
	case BT_HCI_OP_READ_SUPPORTED_COMMANDS: {
		struct bt_hci_rp_read_supported_commands *rp;

		rp = command_complete(opcode, sizeof(*rp), &reply);
		memset(rp->commands, 0xff, sizeof(rp->commands));
		break;
	}
	case BT_HCI_OP_READ_BD_ADDR: {
		struct bt_hci_rp_read_bd_addr *rp;

		rp = command_complete(opcode, sizeof(*rp), &reply);
		memset(&rp->bdaddr, 0x77, sizeof(rp->bdaddr));
		break;
	}
	case BT_HCI_OP_LE_READ_LOCAL_FEATURES: {
		struct bt_hci_rp_le_read_local_features *rp;

		rp = command_complete(opcode, sizeof(*rp), &reply);
		/* Extended and periodic advertising. */
		rp->features[1] = BIT(4) | BIT(5);
		break;
	}
	case BT_HCI_OP_LE_READ_BUFFER_SIZE: {
		struct bt_hci_rp_le_read_buffer_size *rp;

		rp = command_complete(opcode, sizeof(*rp), &reply);
		rp->le_max_len = sys_cpu_to_le16(27U);
		rp->le_max_num = CONFIG_BT_BUF_ACL_TX_COUNT;
		break;
	}
	case BT_HCI_OP_LE_READ_SUPP_STATES: {
		struct bt_hci_rp_le_read_supp_states *rp;

		rp = command_complete(opcode, sizeof(*rp), &reply);
		memset(rp->le_states, 0xff, sizeof(rp->le_states));
		break;
	}
	case BT_HCI_OP_LE_READ_MAX_ADV_DATA_LEN: {
		struct bt_hci_rp_le_read_max_adv_data_len *rp;

		rp = command_complete(opcode, sizeof(*rp), &reply);
		rp->max_adv_data_len = sys_cpu_to_le16(BT_GAP_ADV_MAX_EXT_ADV_DATA_LEN);
		break;
	}
	case BT_HCI_OP_LE_READ_PHY: {
		const struct bt_hci_cp_le_read_phy *cp = (const void *)command->data;
		struct bt_hci_rp_le_read_phy *rp;
		uint16_t handle = sys_le16_to_cpu(cp->handle);

		(void)atomic_inc(&phy_reads);
		if (atomic_cas(&disconnect_in_phy, 1, 0)) {
			/* The priority path runs now; normal handling waits for this reply. */
			disconnect_send(handle, BT_HCI_ERR_CONN_TIMEOUT);
		}
		rp = command_complete(opcode, sizeof(*rp), &reply);
		rp->handle = sys_cpu_to_le16(handle);
		rp->tx_phy = BT_HCI_LE_PHY_1M;
		rp->rx_phy = BT_HCI_LE_PHY_1M;
		break;
	}
	case BT_HCI_OP_DISCONNECT:
	case BT_HCI_OP_LE_PER_ADV_CREATE_SYNC: {
		const struct bt_hci_evt_cmd_status status = {
			.status = BT_HCI_ERR_SUCCESS,
			.ncmd = 1U,
			.opcode = sys_cpu_to_le16(opcode),
		};

		event_send(BT_HCI_EVT_CMD_STATUS, &status, sizeof(status));
		return;
	}
	default:
		/* Initialization commands unrelated to the event sequences under test. */
		(void)command_complete(opcode, 64U, &reply);
		break;
	}

	zassert_ok(bt_hci_recv_err(dev, reply));
}

static void packets_completed(struct k_work *work)
{
	struct {
		struct bt_hci_evt_num_completed_packets event;
		struct bt_hci_handle_count handle;
	} __packed parameters;

	ARG_UNUSED(work);
	parameters.event.num_handles = 1U;
	parameters.handle.handle = sys_cpu_to_le16(packet_handle);
	parameters.handle.count = sys_cpu_to_le16((uint16_t)atomic_set(&pending_packets, 0));
	event_send(BT_HCI_EVT_NUM_COMPLETED_PACKETS, &parameters, sizeof(parameters));
}

static K_WORK_DEFINE(packets_work, packets_completed);

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int driver_close(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	if (type == BT_HCI_H4_CMD) {
		command_handle(dev, buf);
	} else {
		struct bt_hci_acl_hdr *hdr;
		uint16_t length;

		zassert_equal(type, BT_HCI_H4_ACL);
		hdr = net_buf_pull_mem(buf, sizeof(*hdr));
		packet_handle = bt_acl_handle(sys_le16_to_cpu(hdr->handle));
		length = net_buf_pull_le16(buf);
		zassert_equal(length, buf->len - sizeof(uint16_t));
		zassert_equal(net_buf_pull_le16(buf), BT_L2CAP_CID_ATT);
		zassert_equal(net_buf_pull_u8(buf), BT_ATT_OP_NOTIFY);
		(void)atomic_inc(&pending_packets);
		zassert_true(k_work_submit(&packets_work) >= 0);
	}
	net_buf_unref(buf);
	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.close = driver_close,
	.send = driver_send,
};

static struct bt_hci_driver_data driver_data;
static const struct bt_hci_driver_config driver_config = BT_DT_HCI_DRIVER_CONFIG_INST_GET(0);

DEVICE_DT_INST_DEFINE(0, NULL, NULL, &driver_data, &driver_config, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api);

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct connection_result *result = &results[bt_conn_index(conn)];

	zassert_is_null(result->conn);
	result->conn = bt_conn_ref(conn);
	result->state = conn->state;
	result->err = err;
	(void)atomic_inc(&connected_count);
	k_sem_give(&connected_sem);
}

static void recycled(void)
{
	k_sem_give(&recycled_sem);
}

BT_CONN_CB_DEFINE(connection_callbacks) = {
	.connected = connected,
	.recycled = recycled,
};

static void rx_hold_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	(void)atomic_set(&rx_held, 1);
	k_sem_give(&rx_held_sem);
	zassert_ok(k_sem_take(&rx_release_sem, WAIT_TIMEOUT));
	(void)atomic_clear(&rx_held);
}

static K_WORK_DEFINE(rx_hold_work, rx_hold_handler);

static void rx_drain_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	k_sem_give(&rx_drained_sem);
}

static K_WORK_DEFINE(rx_drain_work, rx_drain_handler);

static void rx_drain(void)
{
	zassert_true(bt_work_submit(&rx_drain_work) >= 0);
	zassert_ok(k_sem_take(&rx_drained_sem, WAIT_TIMEOUT));
}

static void rx_hold(void)
{
	zassert_true(bt_work_submit(&rx_hold_work) >= 0);
	zassert_ok(k_sem_take(&rx_held_sem, WAIT_TIMEOUT));
}

static void connection_send(uint16_t handle, size_t advertiser)
{
	struct bt_hci_evt_le_enh_conn_complete complete = {
		.handle = sys_cpu_to_le16(handle),
		.role = BT_HCI_ROLE_PERIPHERAL,
		.interval = sys_cpu_to_le16(24U),
		.supv_timeout = sys_cpu_to_le16(100U),
	};
	struct bt_hci_evt_le_adv_set_terminated terminated = {
		.adv_handle = bt_le_ext_adv_get_index(advertisers[advertiser]),
		.conn_handle = sys_cpu_to_le16(handle),
	};

	bt_addr_le_copy(&complete.peer_addr, &peer);
	le_event_send(BT_HCI_EVT_LE_ENH_CONN_COMPLETE, &complete, sizeof(complete));
	le_event_send(BT_HCI_EVT_LE_ADV_SET_TERMINATED, &terminated, sizeof(terminated));
}

static struct connection_result *connection_wait(uint16_t handle, bt_conn_state_t state,
						 uint8_t err)
{
	zassert_ok(k_sem_take(&connected_sem, WAIT_TIMEOUT));
	for (size_t i = 0U; i < ARRAY_SIZE(results); i++) {
		if (results[i].conn != NULL && results[i].conn->handle == handle) {
			zassert_equal(results[i].state, state);
			zassert_equal(results[i].err, err);
			return &results[i];
		}
	}
	zassert_unreachable("No callback for handle %u", handle);
	return NULL;
}

static void connection_release(struct connection_result *result)
{
	rx_drain();
	zassert_equal(result->conn->state, BT_CONN_DISCONNECTED);
	k_sem_reset(&recycled_sem);
	bt_conn_unref(result->conn);
	result->conn = NULL;
	zassert_ok(k_sem_take(&recycled_sem, WAIT_TIMEOUT));
}

static void notification_sent(struct bt_conn *conn, void *user_data)
{
	ARGS_UNUSED(conn, user_data);
	k_sem_give(&notification_sem);
}

static void verify_data(struct connection_result *result)
{
	uint8_t value = 0x5aU;
	struct bt_gatt_notify_params params = {
		.attr = &test_service.attrs[2],
		.data = &value,
		.len = sizeof(value),
		.func = notification_sent,
	};

	zassert_ok(bt_gatt_notify_cb(result->conn, &params));
	zassert_ok(k_sem_take(&notification_sem, WAIT_TIMEOUT));
	zassert_equal(packet_handle, result->conn->handle);
}

static void verify_handle_reuse(uint16_t handle)
{
	struct connection_result *result;

	zassert_ok(bt_le_ext_adv_start(advertisers[0], BT_LE_EXT_ADV_START_DEFAULT));
	connection_send(handle, 0U);
	result = connection_wait(handle, BT_CONN_CONNECTED, BT_HCI_ERR_SUCCESS);
	verify_data(result);
	disconnect_send(handle, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	connection_release(result);
}

static void reject_handle(uint16_t handle)
{
	atomic_val_t callbacks = atomic_get(&connected_count);

	zassert_ok(bt_le_ext_adv_start(advertisers[0], BT_LE_EXT_ADV_START_DEFAULT));
	k_sem_reset(&recycled_sem);
	zassert_ok(bt_le_ext_adv_stop(advertisers[0]));
	zassert_ok(k_sem_take(&recycled_sem, WAIT_TIMEOUT));

	/* No reservation remains when the connection events are processed. */
	connection_send(handle, 0U);
	rx_drain();
	zassert_equal(atomic_get(&connected_count), callbacks);
	disconnect_send(handle, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	rx_drain();
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	(void)atomic_clear(&connected_count);
	(void)atomic_clear(&phy_reads);
	(void)atomic_clear(&disconnect_in_phy);
	k_sem_reset(&connected_sem);
	k_sem_reset(&recycled_sem);
	k_sem_reset(&notification_sem);
	zassert_ok(bt_enable(NULL));

	for (size_t i = 0U; i < ARRAY_SIZE(advertisers); i++) {
		zassert_ok(bt_le_ext_adv_create(
			BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_EXT_ADV,
					BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL),
			NULL, &advertisers[i]));
	}
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	if (atomic_get(&rx_held) != 0) {
		k_sem_give(&rx_release_sem);
	}
	rx_drain();
	for (size_t i = 0U; i < ARRAY_SIZE(advertisers); i++) {
		zassert_ok(bt_le_ext_adv_stop(advertisers[i]));
		zassert_ok(bt_le_ext_adv_delete(advertisers[i]));
	}
	for (size_t i = 0U; i < ARRAY_SIZE(results); i++) {
		if (results[i].conn != NULL) {
			/* An assertion can leave a connection waiting for normal disconnect. */
			if (results[i].conn->state != BT_CONN_DISCONNECTED) {
				disconnect_send(results[i].conn->handle,
						BT_HCI_ERR_REMOTE_USER_TERM_CONN);
				rx_drain();
			}
			bt_conn_unref(results[i].conn);
			results[i].conn = NULL;
		}
	}
	zassert_ok(bt_disable());
}

ZTEST_SUITE(disconnected_handles, NULL, NULL, before, after, NULL);

static ZTEST(disconnected_handles, test_normal_connection)
{
	verify_handle_reuse(1U);
	zassert_equal(atomic_get(&phy_reads), 1);
}

static ZTEST(disconnected_handles, test_rejected_handle_reuse)
{
	reject_handle(2U);
	verify_handle_reuse(2U);
}

static ZTEST(disconnected_handles, test_rejected_handle_zero_reuse)
{
	reject_handle(0U);
	verify_handle_reuse(0U);
}

static ZTEST(disconnected_handles, test_disconnect_after_connected)
{
	verify_handle_reuse(3U);
	verify_handle_reuse(3U);
}

static ZTEST(disconnected_handles, test_early_disconnect)
{
	struct connection_result *result;

	zassert_ok(bt_le_ext_adv_start(advertisers[0], BT_LE_EXT_ADV_START_DEFAULT));
	rx_hold();
	connection_send(4U, 0U);
	disconnect_send(4U, BT_HCI_ERR_CONN_TIMEOUT);
	k_sem_give(&rx_release_sem);
	result = connection_wait(4U, BT_CONN_DISCONNECT_COMPLETE, BT_HCI_ERR_CONN_TIMEOUT);
	connection_release(result);
	verify_handle_reuse(4U);
}

static ZTEST(disconnected_handles, test_disconnect_during_phy)
{
	struct connection_result *result;

	zassert_ok(bt_le_ext_adv_start(advertisers[0], BT_LE_EXT_ADV_START_DEFAULT));
	(void)atomic_set(&disconnect_in_phy, 1);
	connection_send(5U, 0U);
	result = connection_wait(5U, BT_CONN_DISCONNECT_COMPLETE, BT_HCI_ERR_CONN_TIMEOUT);
	zassert_equal(atomic_get(&phy_reads), 1);
	connection_release(result);
	verify_handle_reuse(5U);
}

static ZTEST(disconnected_handles, test_rejections_do_not_exhaust_markers)
{
	struct connection_result *result;

	for (uint16_t handle = 6U; handle < 6U + CONFIG_BT_MAX_CONN + 1U; handle++) {
		reject_handle(handle);
	}
	zassert_ok(bt_le_ext_adv_start(advertisers[0], BT_LE_EXT_ADV_START_DEFAULT));
	rx_hold();
	connection_send(10U, 0U);
	disconnect_send(10U, BT_HCI_ERR_CONN_TIMEOUT);
	k_sem_give(&rx_release_sem);
	result = connection_wait(10U, BT_CONN_DISCONNECT_COMPLETE, BT_HCI_ERR_CONN_TIMEOUT);
	connection_release(result);
	verify_handle_reuse(10U);
}

#if CONFIG_BT_EXT_ADV_MAX_ADV_SET > 1
static ZTEST(disconnected_handles, test_two_early_disconnects)
{
	struct connection_result *first;
	struct connection_result *second;

	zassert_ok(bt_le_ext_adv_start(advertisers[0], BT_LE_EXT_ADV_START_DEFAULT));
	zassert_ok(bt_le_ext_adv_start(advertisers[1], BT_LE_EXT_ADV_START_DEFAULT));
	rx_hold();
	connection_send(11U, 0U);
	connection_send(12U, 1U);
	disconnect_send(11U, BT_HCI_ERR_CONN_TIMEOUT);
	disconnect_send(12U, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	k_sem_give(&rx_release_sem);
	first = connection_wait(11U, BT_CONN_DISCONNECT_COMPLETE, BT_HCI_ERR_CONN_TIMEOUT);
	second = connection_wait(12U, BT_CONN_DISCONNECT_COMPLETE,
				 BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	connection_release(first);
	connection_release(second);
	verify_handle_reuse(11U);
	verify_handle_reuse(12U);
}
#endif

#if defined(CONFIG_BT_PER_ADV_SYNC_RSP)
static struct bt_le_per_adv_sync *sync_create(void)
{
	struct bt_le_per_adv_sync_param param = {
		.sid = 1U,
		.timeout = 100U,
	};
	struct bt_le_per_adv_sync *sync;
	struct bt_hci_evt_le_per_adv_sync_established_v2 event = {
		.handle = sys_cpu_to_le16(SYNC_HANDLE),
		.sid = 1U,
		.phy = BT_HCI_LE_PHY_1M,
		.interval = sys_cpu_to_le16(32U),
		.num_subevents = 1U,
	};

	bt_addr_le_copy(&param.addr, &peer);
	bt_addr_le_copy(&event.adv_addr, &peer);
	zassert_ok(bt_le_per_adv_sync_create(&param, &sync));
	le_event_send(BT_HCI_EVT_LE_PER_ADV_SYNC_ESTABLISHED_V2, &event, sizeof(event));
	rx_drain();
	return sync;
}

static void sync_connection_send(uint16_t handle)
{
	struct bt_hci_evt_le_enh_conn_complete_v2 event = {
		.handle = sys_cpu_to_le16(handle),
		.role = BT_HCI_ROLE_PERIPHERAL,
		.interval = sys_cpu_to_le16(24U),
		.supv_timeout = sys_cpu_to_le16(100U),
		.adv_handle = BT_HCI_ADV_HANDLE_INVALID,
		.sync_handle = sys_cpu_to_le16(SYNC_HANDLE),
	};

	bt_addr_le_copy(&event.peer_addr, &peer);
	le_event_send(BT_HCI_EVT_LE_ENH_CONN_COMPLETE_V2, &event, sizeof(event));
}

static ZTEST(disconnected_handles, test_sync_early_disconnect)
{
	struct bt_le_per_adv_sync *sync = sync_create();
	struct connection_result *result;

	rx_hold();
	sync_connection_send(13U);
	disconnect_send(13U, BT_HCI_ERR_CONN_TIMEOUT);
	k_sem_give(&rx_release_sem);
	result = connection_wait(13U, BT_CONN_DISCONNECT_COMPLETE, BT_HCI_ERR_CONN_TIMEOUT);
	connection_release(result);
	sync_connection_send(13U);
	result = connection_wait(13U, BT_CONN_CONNECTED, BT_HCI_ERR_SUCCESS);
	verify_data(result);
	disconnect_send(13U, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	connection_release(result);
	zassert_ok(bt_le_per_adv_sync_delete(sync));
}

static ZTEST(disconnected_handles, test_sync_rejected_handle_reuse)
{
	struct bt_le_per_adv_sync *sync = sync_create();
	struct bt_conn *reservations[CONFIG_BT_MAX_CONN];
	struct connection_result *result;

	/* Occupy the connection pool without assigning any controller handles. */
	for (size_t i = 0U; i < ARRAY_SIZE(reservations); i++) {
		reservations[i] = bt_conn_add_le(BT_ID_DEFAULT, BT_ADDR_LE_NONE);
		zassert_not_null(reservations[i]);
	}
	sync_connection_send(14U);
	rx_drain();
	zassert_equal(atomic_get(&connected_count), 0);
	disconnect_send(14U, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	rx_drain();
	for (size_t i = 0U; i < ARRAY_SIZE(reservations); i++) {
		k_sem_reset(&recycled_sem);
		bt_conn_unref(reservations[i]);
		zassert_ok(k_sem_take(&recycled_sem, WAIT_TIMEOUT));
	}
	sync_connection_send(14U);
	result = connection_wait(14U, BT_CONN_CONNECTED, BT_HCI_ERR_SUCCESS);
	verify_data(result);
	disconnect_send(14U, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	connection_release(result);
	zassert_ok(bt_le_per_adv_sync_delete(sync));
}
#endif
