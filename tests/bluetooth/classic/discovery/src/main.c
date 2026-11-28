/*
 * Copyright 2026 Xiaomi Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/classic/classic.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of the commands the tests ignore */
#define GENERIC_RP_SIZE 64U

/* LMP features page 0, byte 4: LE Supported (Controller) set, and BR/EDR Not
 * Supported clear
 */
#define FEATURES_LE_BYTE 4U
#define FEATURES_LE_BIT  BIT(6)

#define ACL_MAX_LEN    1021U
#define ACL_MAX_NUM    4U
#define LE_ACL_MAX_LEN 27U
#define LE_ACL_MAX_NUM 3U

/* The longest inquiry, 61.44 s: no inquiry ends before a test ends it */
#define INQUIRY_LENGTH 0x30U

#define RSSI (-40)

/* How long a test waits for something the Host is to do */
#define WAIT K_SECONDS(1)

/* native_sim runs nothing but the test, the Bluetooth workqueue and the TX
 * thread, so a short sleep lets the Host handle all that has been queued to it.
 * Used where a test checks that something did not happen.
 */
#define SETTLE_TIME K_MSEC(10)

static const struct device *const hci_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_hci));

/* The emulated controller: what the Host asked for, and what Inquiry Cancel
 * returns
 */
static atomic_t inquiry_count;
static atomic_t name_request_count;
static atomic_t name_cancel_count;
static uint8_t inquiry_cancel_status;
static K_SEM_DEFINE(name_request_sem, 0, K_SEM_MAX_LIMIT);

static struct net_buf *evt_alloc(uint8_t evt, uint8_t len)
{
	struct net_buf *buf = bt_buf_get_evt(evt, false, K_FOREVER);
	struct bt_hci_evt_hdr *hdr = net_buf_add(buf, sizeof(*hdr));

	hdr->evt = evt;
	hdr->len = len;

	return buf;
}

static void *cmd_complete(struct net_buf **buf, uint8_t plen, uint16_t opcode)
{
	struct bt_hci_evt_cmd_complete *cc;

	*buf = evt_alloc(BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + plen);
	cc = net_buf_add(*buf, sizeof(*cc));
	cc->ncmd = 1U;
	cc->opcode = sys_cpu_to_le16(opcode);

	return net_buf_add(*buf, plen);
}

static struct net_buf *cmd_status(uint16_t opcode)
{
	struct bt_hci_evt_cmd_status *cs;
	struct net_buf *buf;

	buf = evt_alloc(BT_HCI_EVT_CMD_STATUS, sizeof(*cs));
	cs = net_buf_add(buf, sizeof(*cs));

	cs->status = BT_HCI_ERR_SUCCESS;
	cs->ncmd = 1U;
	cs->opcode = sys_cpu_to_le16(opcode);

	return buf;
}

static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	const struct bt_hci_cmd_hdr *hdr = net_buf_pull_mem(cmd, sizeof(*hdr));
	uint16_t opcode = sys_le16_to_cpu(hdr->opcode);
	struct net_buf *evt;

	switch (opcode) {
	case BT_HCI_OP_INQUIRY:
		(void)atomic_inc(&inquiry_count);
		evt = cmd_status(opcode);
		break;
	case BT_HCI_OP_REMOTE_NAME_REQUEST:
		(void)atomic_inc(&name_request_count);
		evt = cmd_status(opcode);
		break;
	case BT_HCI_OP_INQUIRY_CANCEL: {
		struct bt_hci_evt_cc_status *rp = cmd_complete(&evt, sizeof(*rp), opcode);

		rp->status = inquiry_cancel_status;
		break;
	}
	case BT_HCI_OP_REMOTE_NAME_CANCEL: {
		const struct bt_hci_cp_remote_name_cancel *cp = (const void *)cmd->data;
		struct bt_hci_rp_remote_name_cancel *rp = cmd_complete(&evt, sizeof(*rp), opcode);

		(void)atomic_inc(&name_cancel_count);
		rp->status = BT_HCI_ERR_SUCCESS;
		bt_addr_copy(&rp->bdaddr, &cp->bdaddr);
		break;
	}
	case BT_HCI_OP_READ_LOCAL_FEATURES: {
		struct bt_hci_rp_read_local_features *rp = cmd_complete(&evt, sizeof(*rp), opcode);

		rp->status = BT_HCI_ERR_SUCCESS;
		(void)memset(rp->features, 0, sizeof(rp->features));
		rp->features[FEATURES_LE_BYTE] = FEATURES_LE_BIT;
		break;
	}
	case BT_HCI_OP_READ_SUPPORTED_COMMANDS: {
		struct bt_hci_rp_read_supported_commands *rp =
			cmd_complete(&evt, sizeof(*rp), opcode);

		rp->status = BT_HCI_ERR_SUCCESS;
		(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
		break;
	}
	case BT_HCI_OP_READ_BD_ADDR: {
		struct bt_hci_rp_read_bd_addr *rp = cmd_complete(&evt, sizeof(*rp), opcode);

		rp->status = BT_HCI_ERR_SUCCESS;
		(void)memset(&rp->bdaddr, 0x11, sizeof(rp->bdaddr));
		break;
	}
	case BT_HCI_OP_READ_BUFFER_SIZE: {
		struct bt_hci_rp_read_buffer_size *rp = cmd_complete(&evt, sizeof(*rp), opcode);

		(void)memset(rp, 0, sizeof(*rp));
		rp->status = BT_HCI_ERR_SUCCESS;
		rp->acl_max_len = sys_cpu_to_le16(ACL_MAX_LEN);
		rp->acl_max_num = sys_cpu_to_le16(ACL_MAX_NUM);
		break;
	}
	case BT_HCI_OP_LE_READ_BUFFER_SIZE: {
		struct bt_hci_rp_le_read_buffer_size *rp = cmd_complete(&evt, sizeof(*rp), opcode);

		rp->status = BT_HCI_ERR_SUCCESS;
		rp->le_max_len = sys_cpu_to_le16(LE_ACL_MAX_LEN);
		rp->le_max_num = LE_ACL_MAX_NUM;
		break;
	}
	case BT_HCI_OP_LE_READ_SUPP_STATES: {
		struct bt_hci_rp_le_read_supp_states *rp = cmd_complete(&evt, sizeof(*rp), opcode);

		rp->status = BT_HCI_ERR_SUCCESS;
		(void)memset(rp->le_states, 0xFF, sizeof(rp->le_states));
		break;
	}
	default: {
		struct bt_hci_evt_cc_status *rp = cmd_complete(&evt, GENERIC_RP_SIZE, opcode);

		(void)memset(rp, 0, GENERIC_RP_SIZE);
		rp->status = BT_HCI_ERR_SUCCESS;
		break;
	}
	}

	bt_hci_recv(dev, evt);

	if (opcode == BT_HCI_OP_REMOTE_NAME_REQUEST) {
		k_sem_give(&name_request_sem);
	}
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	if (net_buf_pull_u8(buf) == BT_HCI_H4_CMD) {
		cmd_handle(dev, buf);
	}

	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = {0};                                 \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

/* Remote devices. The Host reports M after it has handled what came before
 * it, so a test sends M to know that the Host is past an event.
 */
static const bt_addr_t addr_a = {{0x0a, 0x00, 0x00, 0x00, 0x00, 0x00}};
static const bt_addr_t addr_b = {{0x0b, 0x00, 0x00, 0x00, 0x00, 0x00}};
static const bt_addr_t addr_m = {{0x0d, 0x00, 0x00, 0x00, 0x00, 0x00}};

static struct bt_br_discovery_result results[4];

/* What the discovery callbacks have seen */
#define RECV_LOG_SIZE 8U

static bt_addr_t recv_log[RECV_LOG_SIZE];
static atomic_t recv_count;
static K_SEM_DEFINE(recv_sem, 0, K_SEM_MAX_LIMIT);
static atomic_t timeout_count;
static size_t timeout_result_count;
static K_SEM_DEFINE(timeout_sem, 0, K_SEM_MAX_LIMIT);

/* What the next recv or timeout callback runs, and what that returned */
typedef int (*action_func_t)(void);

static action_func_t recv_action;
static action_func_t timeout_action;
static int action_err;

static int start(void)
{
	static const struct bt_br_discovery_param param = {
		.length = INQUIRY_LENGTH,
		.limited = false,
	};

	return bt_br_discovery_start(&param, results, ARRAY_SIZE(results));
}

static int restart(void)
{
	int err = bt_br_discovery_stop();

	if (err != 0) {
		return err;
	}

	return start();
}

static void recv_cb(const struct bt_br_discovery_result *result)
{
	atomic_val_t n = atomic_inc(&recv_count);
	action_func_t action = recv_action;

	if (n < RECV_LOG_SIZE) {
		bt_addr_copy(&recv_log[n], &result->addr);
	}

	recv_action = NULL;
	if (action != NULL) {
		action_err = action();
	}

	k_sem_give(&recv_sem);
}

static void timeout_cb(const struct bt_br_discovery_result *found, size_t count)
{
	action_func_t action = timeout_action;

	ARG_UNUSED(found);

	(void)atomic_inc(&timeout_count);
	timeout_result_count = count;

	timeout_action = NULL;
	if (action != NULL) {
		action_err = action();
	}

	k_sem_give(&timeout_sem);
}

/* One Inquiry Result with RSSI event that reports two devices */
static void send_inquiry_results(const bt_addr_t *first, const bt_addr_t *second)
{
	const bt_addr_t *addrs[] = {first, second};
	struct bt_hci_evt_inquiry_result_with_rssi *rsp;
	struct net_buf *buf;

	buf = evt_alloc(BT_HCI_EVT_INQUIRY_RESULT_WITH_RSSI,
			sizeof(uint8_t) + (ARRAY_SIZE(addrs) * sizeof(*rsp)));
	(void)net_buf_add_u8(buf, (uint8_t)ARRAY_SIZE(addrs));
	rsp = net_buf_add(buf, ARRAY_SIZE(addrs) * sizeof(*rsp));
	(void)memset(rsp, 0, ARRAY_SIZE(addrs) * sizeof(*rsp));

	for (size_t i = 0U; i < ARRAY_SIZE(addrs); i++) {
		bt_addr_copy(&rsp[i].addr, addrs[i]);
		rsp[i].rssi = RSSI;
	}

	bt_hci_recv(hci_dev, buf);
}

/* One EIR structure: its length, its type, then the data */
struct eir_field {
	uint8_t len;
	uint8_t type;
	uint8_t data[];
} __packed;

/* An Extended Inquiry Result, with a name if @p name is not NULL */
static void send_eir(const bt_addr_t *addr, const char *name)
{
	struct bt_hci_evt_extended_inquiry_result *evt;
	struct net_buf *buf;

	buf = evt_alloc(BT_HCI_EVT_EXTENDED_INQUIRY_RESULT, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	evt->num_reports = 1U;
	bt_addr_copy(&evt->addr, addr);
	evt->rssi = RSSI;

	if (name != NULL) {
		struct eir_field *field = (void *)evt->eir;
		uint8_t len = (uint8_t)strlen(name);

		field->len = sizeof(field->type) + len;
		field->type = BT_DATA_NAME_COMPLETE;
		(void)memcpy(field->data, name, len);
	}

	bt_hci_recv(hci_dev, buf);
}

static void send_inquiry_complete(void)
{
	struct bt_hci_evt_inquiry_complete *evt;
	struct net_buf *buf;

	buf = evt_alloc(BT_HCI_EVT_INQUIRY_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;

	bt_hci_recv(hci_dev, buf);
}

static void send_name(const bt_addr_t *addr, const char *name)
{
	struct bt_hci_evt_remote_name_req_complete *evt;
	struct net_buf *buf;

	buf = evt_alloc(BT_HCI_EVT_REMOTE_NAME_REQ_COMPLETE, sizeof(*evt));
	evt = net_buf_add(buf, sizeof(*evt));
	(void)memset(evt, 0, sizeof(*evt));
	evt->status = BT_HCI_ERR_SUCCESS;
	bt_addr_copy(&evt->bdaddr, addr);
	(void)memcpy(evt->name, name, strlen(name));

	bt_hci_recv(hci_dev, buf);
}

/* Lets the Host handle what has been queued to it, see SETTLE_TIME */
static void settle(void)
{
	k_sleep(SETTLE_TIME);
}

static void *setup(void)
{
	static struct bt_br_discovery_cb discovery_cb = {
		.recv = recv_cb,
		.timeout = timeout_cb,
	};

	zassert_ok(bt_enable(NULL));
	bt_br_discovery_cb_register(&discovery_cb);

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)bt_br_discovery_stop();
	settle();

	recv_action = NULL;
	timeout_action = NULL;
	action_err = 0;
	inquiry_cancel_status = BT_HCI_ERR_SUCCESS;
	(void)atomic_clear(&recv_count);
	(void)atomic_clear(&timeout_count);
	(void)atomic_clear(&inquiry_count);
	(void)atomic_clear(&name_request_count);
	(void)atomic_clear(&name_cancel_count);
	k_sem_reset(&recv_sem);
	k_sem_reset(&timeout_sem);
	k_sem_reset(&name_request_sem);
}

ZTEST_SUITE(discovery, NULL, setup, before, NULL, NULL);

/* A recv callback stops and restarts discovery with the same storage: the
 * rest of the event belongs to the stopped discovery and is not reported.
 */
ZTEST(discovery, test_restart_from_recv)
{
	zassert_ok(start());

	recv_action = restart;
	send_inquiry_results(&addr_a, &addr_b);
	zassert_ok(k_sem_take(&recv_sem, WAIT));
	zassert_ok(action_err);

	send_eir(&addr_m, NULL);
	zassert_ok(k_sem_take(&recv_sem, WAIT));

	zassert_equal(atomic_get(&recv_count), 2, "the rest of the event was reported");
	zassert_true(bt_addr_eq(&recv_log[1], &addr_m));
	zassert_true(bt_addr_eq(&results[0].addr, &addr_m), "the new discovery kept an old result");
	zassert_equal(atomic_get(&inquiry_count), 2);
	zassert_equal(atomic_get(&timeout_count), 0);
}

/* A recv callback for a resolved name stops discovery: the stopped discovery
 * reports no results.
 */
ZTEST(discovery, test_stop_from_recv_while_resolving_names)
{
	zassert_ok(start());

	send_eir(&addr_a, NULL);
	zassert_ok(k_sem_take(&recv_sem, WAIT));

	/* A has no name in its EIR, so the Host asks for it */
	send_inquiry_complete();
	zassert_ok(k_sem_take(&name_request_sem, WAIT));

	recv_action = bt_br_discovery_stop;
	send_name(&addr_a, "a");
	zassert_ok(k_sem_take(&recv_sem, WAIT));
	zassert_ok(action_err);

	settle();
	zassert_equal(atomic_get(&timeout_count), 0, "a stopped discovery reported results");
}

/* A timeout callback starts a new discovery with the same storage: the new
 * discovery goes on once the callback returns.
 */
ZTEST(discovery, test_restart_from_timeout)
{
	zassert_ok(start());

	timeout_action = start;
	send_inquiry_complete();
	zassert_ok(k_sem_take(&timeout_sem, WAIT));
	zassert_ok(action_err);
	zassert_equal(timeout_result_count, 0);

	send_eir(&addr_m, NULL);
	zassert_ok(k_sem_take(&recv_sem, WAIT), "the new discovery was ended");
	zassert_equal(atomic_get(&inquiry_count), 2);
}

/* A name request cancelled by stop still completes, once a new discovery has
 * started: its result is not reported in the new discovery.
 */
ZTEST(discovery, test_stale_name_after_restart)
{
	zassert_ok(start());

	send_eir(&addr_a, NULL);
	zassert_ok(k_sem_take(&recv_sem, WAIT));
	send_inquiry_complete();
	zassert_ok(k_sem_take(&name_request_sem, WAIT));

	zassert_ok(bt_br_discovery_stop());
	zassert_equal(atomic_get(&name_cancel_count), 1);
	zassert_ok(start());

	send_name(&addr_a, "a");
	send_eir(&addr_m, NULL);
	zassert_ok(k_sem_take(&recv_sem, WAIT));

	zassert_equal(atomic_get(&recv_count), 2, "the old name was reported");
	zassert_true(bt_addr_eq(&recv_log[1], &addr_m));
	zassert_equal(atomic_get(&name_request_count), 1);
	zassert_equal(atomic_get(&timeout_count), 0);
}

/* Inquiry Cancel fails because the inquiry has just ended, and its Inquiry
 * Complete arrives once a new discovery has started: it does not end the new
 * discovery.
 */
ZTEST(discovery, test_stale_inquiry_complete_after_failed_cancel)
{
	zassert_ok(start());

	inquiry_cancel_status = BT_HCI_ERR_CMD_DISALLOWED;
	zassert_ok(bt_br_discovery_stop());
	inquiry_cancel_status = BT_HCI_ERR_SUCCESS;
	zassert_ok(start());

	send_inquiry_complete();
	send_eir(&addr_m, "m");
	zassert_ok(k_sem_take(&recv_sem, WAIT));
	zassert_equal(atomic_get(&timeout_count), 0, "the old inquiry ended the new discovery");

	/* M has a name, so the new discovery completes with its inquiry */
	send_inquiry_complete();
	zassert_ok(k_sem_take(&timeout_sem, WAIT));
	zassert_equal(timeout_result_count, 1);
}
