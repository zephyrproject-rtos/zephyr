/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

#include "babblekit/sync.h"
#include "babblekit/testcase.h"
#include "bstests.h"
#include "host/att_internal.h"
#include "audio.h"

#define SERVICE_UUID   BT_UUID_DECLARE_16(0x1234)
#define VALUE_UUID     BT_UUID_DECLARE_16(0x1235)
#define LONG_LEN       100
#define SHORT_LEN      10
#define PHASE_COUNT    8
#define MARKER         0xff
#define PHASE_ACK_BASE (2 + (IS_ENABLED(CONFIG_BT_AICS) ? AUDIO_PHASE_COUNT : 0))

static bool server_role;
static bool large_mtu;
static struct bt_conn *client_conn;
static struct bt_conn *server_conns[2];
static atomic_t connection_count;
static atomic_t acknowledged[2];
static atomic_t client_phase;
static atomic_t received;
static atomic_t confirmations;
static uint16_t value_handle;
static uint8_t payload[LONG_LEN];

K_SEM_DEFINE(connected_sem, 0, 2);
K_SEM_DEFINE(discovered_sem, 0, 1);
K_SEM_DEFINE(subscribed_sem, 0, 1);
K_SEM_DEFINE(exchanged_sem, 0, 1);
K_SEM_DEFINE(written_sem, 0, 1);
K_SEM_DEFINE(marker_sem, 0, 1);
K_SEM_DEFINE(destroyed_sem, 0, 1);
K_SEM_DEFINE(exhausted_sem, 0, 1);
K_SEM_DEFINE(sent_sem, 0, 2);

static void wait_sem(struct k_sem *sem)
{
	TEST_ASSERT(k_sem_take(sem, K_SECONDS(5)) == 0, "Timed out waiting for peer");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	TEST_ASSERT(err == 0, "Connection failed (%u)", err);
	if (server_role) {
		atomic_val_t index = atomic_get(&connection_count);

		TEST_ASSERT(index < ARRAY_SIZE(server_conns), "Too many connections");
		server_conns[index] = bt_conn_ref(conn);
		atomic_inc(&connection_count);
	}
	k_sem_give(&connected_sem);
}

#if defined(CONFIG_BT_SMP)
static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	ARG_UNUSED(conn);
	TEST_ASSERT(err == BT_SECURITY_ERR_SUCCESS && level >= BT_SECURITY_L2,
		    "Encryption failed (level %u, error %u)", level, err);
}
#endif

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
#if defined(CONFIG_BT_SMP)
	.security_changed = security_changed,
#endif
};

static ssize_t write_ack(struct bt_conn *conn, const struct bt_gatt_attr *attr, const void *buf,
			 uint16_t len, uint16_t offset, uint8_t flags)
{
	const uint8_t *ack = buf;

	ARG_UNUSED(attr);
	ARG_UNUSED(flags);
	TEST_ASSERT(server_role && len == 1 && offset == 0, "Invalid acknowledgment");
	for (size_t i = 0; i < ARRAY_SIZE(server_conns); i++) {
		if (server_conns[i] == conn) {
			TEST_ASSERT(*ack == atomic_get(&acknowledged[i]) + 1,
				    "Duplicate or out-of-order acknowledgment");
			atomic_set(&acknowledged[i], *ack);
			return len;
		}
	}
	TEST_FAIL("Acknowledgment from unexpected connection");
	return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
}

BT_GATT_SERVICE_DEFINE(test_svc,
	BT_GATT_PRIMARY_SERVICE(SERVICE_UUID),
	BT_GATT_CHARACTERISTIC(VALUE_UUID,
		BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_INDICATE | BT_GATT_CHRC_WRITE,
		BT_GATT_PERM_WRITE, NULL, write_ack, NULL),
	BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE));

static void wait_ack(size_t index, int value)
{
	int64_t deadline = k_uptime_get() + 5000;

	while (atomic_get(&acknowledged[index]) != value) {
		TEST_ASSERT(k_uptime_get() < deadline, "Missing acknowledgment %d from %zu", value,
			    index);
		k_sleep(K_MSEC(1));
	}
}

static void indicated(struct bt_conn *conn, struct bt_gatt_indicate_params *params, uint8_t err)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	TEST_ASSERT(err == 0, "Indication confirmation failed (%u)", err);
	atomic_inc(&confirmations);
}

static void destroyed(struct bt_gatt_indicate_params *params)
{
	ARG_UNUSED(params);
	k_sem_give(&destroyed_sem);
}

static void sent(struct bt_conn *conn, void *user_data)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(user_data);
	k_sem_give(&sent_sem);
}

static void audio_checkpoint(int phase)
{
	uint8_t marker[] = {AUDIO_MARKER, (uint8_t)phase};

	TEST_ASSERT(bt_gatt_notify(NULL, &test_svc.attrs[2], marker, sizeof(marker)) == 0,
		    "Audio marker failed");
	for (size_t i = 0; i < ARRAY_SIZE(server_conns); i++) {
		wait_ack(i, phase + 2);
	}
}

static void exhaust_resources(struct k_work *work)
{
	struct net_buf *buffers[CONFIG_BT_ATT_TX_COUNT];
	struct bt_att_req *requests[CONFIG_BT_ATT_TX_COUNT];
	struct bt_gatt_indicate_params ind = {
		.attr = &test_svc.attrs[2],
		.data = payload,
		.len = SHORT_LEN,
		.func = indicated,
		.destroy = destroyed,
	};
	size_t count = 0;

	ARG_UNUSED(work);
	/* The system workqueue makes allocation nonblocking, as in audio services. */
	while (count < ARRAY_SIZE(buffers)) {
		buffers[count] = bt_att_create_pdu(server_conns[0], BT_ATT_OP_NOTIFY, 0);
		if (buffers[count] == NULL) {
			break;
		}
		count++;
	}
	TEST_ASSERT(count > 0, "No buffers available before exhaustion");
	TEST_ASSERT(bt_gatt_notify(NULL, ind.attr, payload, SHORT_LEN) == -ENOMEM,
		    "Exhausted notification must fail");
	TEST_ASSERT(bt_gatt_indicate(NULL, &ind) == -ENOMEM && ind._ref == 0,
		    "Exhausted indication must fail without retaining parameters");
	for (size_t i = 0; i < count; i++) {
		net_buf_unref(buffers[i]);
	}

	count = 0;
	while (count < ARRAY_SIZE(requests)) {
		requests[count] = bt_att_req_alloc(K_NO_WAIT);
		if (requests[count] == NULL) {
			break;
		}
		count++;
	}
	TEST_ASSERT(count > 0, "No requests available before exhaustion");
	TEST_ASSERT(bt_gatt_indicate(NULL, &ind) == -ENOMEM && ind._ref == 0,
		    "Exhausted request pool must fail without retaining parameters");
	for (size_t i = 0; i < count; i++) {
		bt_att_req_free(requests[i]);
	}
	TEST_ASSERT(k_sem_take(&destroyed_sem, K_NO_WAIT) == -EBUSY,
		    "Failed indications must not invoke destroy");
	TEST_PRINT("ATT buffer and request exhaustion returned -ENOMEM");
	k_sem_give(&exhausted_sem);
}

static void server(void)
{
	struct k_work exhaust_work;
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	};
	const struct bt_gatt_attr *attr = &test_svc.attrs[2];
	struct bt_gatt_indicate_params ind = {
		.attr = attr,
		.data = payload,
		.len = LONG_LEN,
		.func = indicated,
		.destroy = destroyed,
	};
	struct bt_gatt_notify_params nfy = {
		.attr = attr,
		.data = payload,
		.len = LONG_LEN,
	};

	TEST_START("Notification and indication fanout");
	server_role = true;
	TEST_ASSERT(bt_enable(NULL) == 0, "Bluetooth initialization failed");
	audio_server_init();
	TEST_ASSERT(bt_gatt_notify_cb(NULL, &nfy) == -ENOTCONN, "Unexpected empty notify");
	TEST_ASSERT(bt_gatt_indicate(NULL, &ind) == -ENOTCONN, "Unexpected empty indicate");

	for (size_t i = 0; i < ARRAY_SIZE(server_conns); i++) {
		TEST_ASSERT(bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0) ==
				    0,
			    "Advertising failed");
		wait_sem(&connected_sem);
		wait_ack(i, 1);
		TEST_ASSERT(bt_le_adv_stop() == 0, "Stopping advertising failed");
	}

	audio_server_run(server_conns, audio_checkpoint);
	if (IS_ENABLED(CONFIG_BT_EATT)) {
		nfy.func = sent;
	}
	for (int phase = 0; phase < PHASE_COUNT; phase++) {
		uint16_t len = phase < 4 ? LONG_LEN : SHORT_LEN;
		uint8_t marker[] = {MARKER, (uint8_t)phase};
		int eligible = 0;
		int err;

		if (phase == 6) {
			k_work_init(&exhaust_work, exhaust_resources);
			TEST_ASSERT(k_work_submit(&exhaust_work) >= 0, "Work submission failed");
			wait_sem(&exhausted_sem);
		}
		for (size_t i = 0; i < ARRAY_SIZE(server_conns); i++) {
			uint16_t mtu = bt_gatt_get_mtu(server_conns[i]);

			TEST_ASSERT(mtu == 23 || mtu >= LONG_LEN + 3, "Unexpected MTU %u", mtu);
			if (IS_ENABLED(CONFIG_BT_EATT)) {
				TEST_ASSERT(bt_att_get_uatt_mtu(server_conns[i]) == 23,
					    "Long values must not fit on UATT");
				TEST_PRINT("Peer %zu: UATT MTU 23, max MTU %u, EATT %zu", i, mtu,
					   bt_eatt_count(server_conns[i]));
			}
			eligible += len + 3 <= mtu;
		}
		for (size_t i = 0; i < len; i++) {
			payload[i] = (uint8_t)(phase + i);
		}
		atomic_set(&confirmations, 0);
		if (phase == 3 || phase == 5 || phase == 7) {
			ind.len = len;
			err = bt_gatt_indicate(NULL, &ind);
		} else if (phase == 0 && !IS_ENABLED(CONFIG_BT_EATT)) {
			err = bt_gatt_notify(NULL, attr, payload, len);
		} else if (phase == 2 && !IS_ENABLED(CONFIG_BT_EATT)) {
			err = bt_gatt_notify_uuid(NULL, VALUE_UUID, test_svc.attrs, payload, len);
		} else {
			nfy.len = len;
			err = bt_gatt_notify_cb(NULL, &nfy);
		}
		TEST_PRINT("Phase %d: length %u, eligible %d, return %d", phase, len, eligible,
			   err);
		if ((phase == 3 || phase == 5 || phase == 7) && eligible > 0) {
			wait_sem(&destroyed_sem);
			TEST_ASSERT(atomic_get(&confirmations) == eligible,
				    "Incorrect indication completion count");
		} else if (IS_ENABLED(CONFIG_BT_EATT)) {
			for (int i = 0; i < eligible; i++) {
				wait_sem(&sent_sem);
			}
		}

		/* Wait for completion before markers can use another ATT bearer. */
		TEST_ASSERT(bt_gatt_notify(NULL, attr, marker, sizeof(marker)) == 0,
			    "Marker broadcast failed");
		for (size_t i = 0; i < ARRAY_SIZE(server_conns); i++) {
			wait_ack(i, phase + PHASE_ACK_BASE);
		}
		TEST_ASSERT(err == (eligible > 0 ? 0 : -ENOMEM),
			    "Phase %d returned %d for %d eligible peers", phase, err, eligible);
	}
	TEST_PASS("Fanout return values and indication lifetime passed");
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
			 struct net_buf_simple *ad)
{
	ARG_UNUSED(rssi);
	ARG_UNUSED(ad);
	if (client_conn != NULL || type != BT_HCI_ADV_IND) {
		return;
	}
	TEST_ASSERT(bt_le_scan_stop() == 0, "Scan stop failed");
	TEST_ASSERT(bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN, BT_LE_CONN_PARAM_DEFAULT,
				      &client_conn) == 0,
		    "Connect failed");
}

static uint8_t discovered(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			  struct bt_gatt_discover_params *params)
{
	const struct bt_gatt_chrc *chrc;

	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	TEST_ASSERT(attr != NULL, "Characteristic not found");
	chrc = attr->user_data;
	value_handle = chrc->value_handle;
	k_sem_give(&discovered_sem);
	return BT_GATT_ITER_STOP;
}

static void exchanged(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	TEST_ASSERT(err == 0, "MTU exchange failed (%u)", err);
	k_sem_give(&exchanged_sem);
}

static void subscribed(struct bt_conn *conn, uint8_t err, struct bt_gatt_subscribe_params *params)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	TEST_ASSERT(err == 0, "Subscribe failed (%u)", err);
	k_sem_give(&subscribed_sem);
}

static uint8_t notified(struct bt_conn *conn, struct bt_gatt_subscribe_params *params,
			const void *data, uint16_t len)
{
	const uint8_t *bytes = data;
	int phase = atomic_get(&client_phase);
	int expected_len = phase < 4 ? LONG_LEN : SHORT_LEN;
	int expected_count = large_mtu || phase >= 4;

	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	TEST_ASSERT(data != NULL && phase < PHASE_COUNT, "Unexpected late value or unsubscribe");
	if (len == 2 && bytes[0] == AUDIO_MARKER) {
		audio_client_check(bytes[1]);
		k_sem_give(&marker_sem);
		return BT_GATT_ITER_CONTINUE;
	}
	if (len == 2 && bytes[0] == MARKER) {
		TEST_ASSERT(bytes[1] == phase, "Out-of-order marker");
		TEST_ASSERT(atomic_get(&received) == expected_count,
			    "Phase %d: expected %d value, got %ld", phase, expected_count,
			    (long)atomic_get(&received));
		atomic_set(&received, 0);
		atomic_inc(&client_phase);
		k_sem_give(&marker_sem);
		return BT_GATT_ITER_CONTINUE;
	}
	TEST_ASSERT(len == expected_len && expected_count == 1, "Unexpected length %u", len);
	for (size_t i = 0; i < len; i++) {
		TEST_ASSERT(bytes[i] == (uint8_t)(phase + i), "Corrupted byte %zu", i);
	}
	TEST_ASSERT(atomic_inc(&received) == 0, "Duplicate value in phase %d", phase);
	return BT_GATT_ITER_CONTINUE;
}

static void written(struct bt_conn *conn, uint8_t err, struct bt_gatt_write_params *params)
{
	ARG_UNUSED(conn);
	ARG_UNUSED(params);
	TEST_ASSERT(err == 0, "Acknowledgment write failed (%u)", err);
	k_sem_give(&written_sem);
}

static void acknowledge(uint8_t value)
{
	struct bt_gatt_write_params params = {
		.func = written,
		.handle = value_handle,
		.data = &value,
		.length = sizeof(value),
	};

	TEST_ASSERT(bt_gatt_write(client_conn, &params) == 0, "Write could not be queued");
	wait_sem(&written_sem);
}

static void client(bool first, bool large_peer)
{
	struct bt_gatt_exchange_params exchange = {.func = exchanged};
	struct bt_gatt_discover_params discover = {
		.uuid = VALUE_UUID,
		.func = discovered,
		.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE,
		.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
		.type = BT_GATT_DISCOVER_CHARACTERISTIC,
	};
	static struct bt_gatt_discover_params ccc_discover;
	static struct bt_gatt_subscribe_params subscription = {
		.notify = notified,
		.subscribe = subscribed,
		.ccc_handle = BT_GATT_AUTO_DISCOVER_CCC_HANDLE,
		.disc_params = &ccc_discover,
		.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE,
		.value = BT_GATT_CCC_NOTIFY | BT_GATT_CCC_INDICATE,
	};

	TEST_START("Client %s, %s MTU", first ? "first" : "second", large_peer ? "large" : "small");
	large_mtu = large_peer;
	TEST_ASSERT(bk_sync_init() == 0, "Backchannel initialization failed");
	TEST_ASSERT(bt_enable(NULL) == 0, "Bluetooth initialization failed");
	if (!first) {
		bk_sync_wait();
	}
	TEST_ASSERT(bt_le_scan_start(BT_LE_SCAN_PASSIVE, device_found) == 0, "Scan failed");
	wait_sem(&connected_sem);
	if (IS_ENABLED(CONFIG_BT_SMP)) {
		int64_t deadline = k_uptime_get() + 5000;

		TEST_ASSERT(bt_conn_set_security(client_conn, BT_SECURITY_L2) == 0,
			    "Security request failed");
		while (bt_conn_get_security(client_conn) < BT_SECURITY_L2) {
			TEST_ASSERT(k_uptime_get() < deadline, "Encryption timed out");
			k_sleep(K_MSEC(1));
		}
	}
	if (large_peer && IS_ENABLED(CONFIG_BT_EATT)) {
		int64_t deadline = k_uptime_get() + 5000;

		TEST_ASSERT(bt_eatt_connect(client_conn, 1) == 0, "EATT connect failed");
		while (bt_eatt_count(client_conn) != 1) {
			TEST_ASSERT(k_uptime_get() < deadline, "EATT connection timed out");
			k_sleep(K_MSEC(1));
		}
	} else if (large_peer) {
		TEST_ASSERT(bt_gatt_exchange_mtu(client_conn, &exchange) == 0,
			    "MTU request failed");
		wait_sem(&exchanged_sem);
	}
	TEST_ASSERT(large_peer ? bt_gatt_get_mtu(client_conn) >= LONG_LEN + 3
			       : bt_gatt_get_mtu(client_conn) == 23,
		    "Incorrect MTU");
	TEST_ASSERT(bt_gatt_discover(client_conn, &discover) == 0, "Discovery failed");
	wait_sem(&discovered_sem);
	subscription.value_handle = value_handle;
	TEST_ASSERT(bt_gatt_subscribe(client_conn, &subscription) == 0, "Subscription failed");
	wait_sem(&subscribed_sem);
	audio_client_subscribe(client_conn, large_peer);
	acknowledge(1);
	if (first) {
		/* The second peer cannot connect or subscribe before the first peer is ready. */
		bk_sync_send();
	}
	if (IS_ENABLED(CONFIG_BT_AICS)) {
		for (int phase = 0; phase < AUDIO_PHASE_COUNT; phase++) {
			wait_sem(&marker_sem);
			acknowledge(phase + 2);
		}
	}
	for (int phase = 0; phase < PHASE_COUNT; phase++) {
		wait_sem(&marker_sem);
		acknowledge(phase + PHASE_ACK_BASE);
	}
	TEST_PASS("Client received each eligible value exactly once");
}

static void small_first(void)
{
	client(true, false);
}

static void small_second(void)
{
	client(false, false);
}

static void large_first(void)
{
	client(true, true);
}

static void large_second(void)
{
	client(false, true);
}

static const struct bst_test_instance tests[] = {
	{.test_id = "server", .test_main_f = server},
	{.test_id = "small_first", .test_main_f = small_first},
	{.test_id = "small_second", .test_main_f = small_second},
	{.test_id = "large_first", .test_main_f = large_first},
	{.test_id = "large_second", .test_main_f = large_second},
	BSTEST_END_MARKER,
};

static struct bst_test_list *install(struct bst_test_list *list)
{
	return bst_add_tests(list, tests);
}

bst_test_install_t test_installers[] = {install, NULL};

int main(void)
{
	bst_main();
	return 0;
}
