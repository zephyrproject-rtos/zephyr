/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
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

#include <host/adv.h>
#include <host/hci_core.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_lim_adv_test, CONFIG_BT_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Length of the command complete parameters used for commands without a dedicated handler.
 * The parameters are zeroed, and the size is large enough to hold the return parameters of any
 * command that the Host may send.
 */
#define UNHANDLED_CMD_RP_SIZE 64U

#define TIMEOUT_MS (CONFIG_BT_LIM_ADV_TIMEOUT * MSEC_PER_SEC)

/* Long enough for the limited advertising timeout to expire and for the Host to act on it */
#define TIMEOUT_WAIT_MS (TIMEOUT_MS + 100)
#define TIMEOUT_WAIT    K_MSEC(TIMEOUT_WAIT_MS)

/* Advertising handle of the most recent LE Set Extended Advertising Parameters command. */
static uint8_t last_adv_handle;

/* Whether the test Controller advertises. */
static atomic_t adv_enabled;

/* Whether the test Controller rejects the commands that enable advertising. */
static atomic_t reject_adv_enable;

/* Time for which the test Controller holds the next command that sets advertising data. */
static atomic_t adv_data_delay_ms;

/* Time for which the test Controller stays busy after it completed the next command that enables
 * advertising.
 */
static atomic_t adv_enable_busy_us;

/* Error of the most recent connected callback. */
static uint8_t conn_err;

static K_SEM_DEFINE(connected_sem, 0, 1);

/* Command handler structure for cmd_handle(). */
struct cmd_handler {
	uint16_t opcode; /* HCI command opcode */
	uint8_t len;     /* HCI command response length */
	void (*handler)(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode);
};

/* Add event to net_buf. */
static void evt_create(struct net_buf *buf, uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;
}

/* Create a command complete event. */
static void *cmd_complete(struct net_buf **buf, uint8_t plen, uint16_t opcode)
{
	struct bt_hci_evt_cmd_complete *cc;

	*buf = bt_buf_get_evt(BT_HCI_EVT_CMD_COMPLETE, false, K_FOREVER);
	evt_create(*buf, BT_HCI_EVT_CMD_COMPLETE, sizeof(*cc) + plen);
	cc = net_buf_add(*buf, sizeof(*cc));
	cc->ncmd = 1U;
	cc->opcode = sys_cpu_to_le16(opcode);

	return net_buf_add(*buf, plen);
}

/* Generic command complete with success status. */
static void generic_success(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *ccst;

	ARG_UNUSED(buf);

	ccst = cmd_complete(evt, len, opcode);

	/* Fill any event parameters with zero */
	(void)memset(ccst, 0, len);

	ccst->status = BT_HCI_ERR_SUCCESS;
}

static void read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;

	ARGS_UNUSED(buf, len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	/* LE supported, and BR/EDR not supported */
	rp->features[4] = BIT(5) | BIT(6);
}

static void read_supported_commands(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				    uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;

	ARGS_UNUSED(buf, len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
}

static void read_bd_addr(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_rp_read_bd_addr *rp;

	ARGS_UNUSED(buf, len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(&rp->bdaddr, 0x11, sizeof(rp->bdaddr));
}

static void le_read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				   uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features *rp;

	ARGS_UNUSED(buf, len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));

	/* Extended advertising builds get a Controller with LE Extended Advertising */
	if (IS_ENABLED(CONFIG_BT_EXT_ADV)) {
		rp->features[BT_LE_FEAT_BIT_EXT_ADV / 8] = BIT(BT_LE_FEAT_BIT_EXT_ADV % 8);
	}
}

static void le_read_buffer_size(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_buffer_size *rp;

	ARGS_UNUSED(buf, len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->le_max_len = sys_cpu_to_le16(27U);
	rp->le_max_num = 1U;
}

static void le_read_supp_states(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	ARGS_UNUSED(buf, len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->le_states, 0xFF, sizeof(rp->le_states));
}

static void le_read_max_adv_data_len(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				     uint16_t opcode)
{
	struct bt_hci_rp_le_read_max_adv_data_len *rp;

	ARGS_UNUSED(buf, len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->max_adv_data_len = sys_cpu_to_le16(BT_GAP_ADV_MAX_EXT_ADV_DATA_LEN);
}

/* Handler for BT_HCI_OP_LE_SET_EXT_ADV_PARAM. It stores the advertising handle, which the
 * LE Advertising Set Terminated event of the test Controller has to carry.
 */
static void le_set_ext_adv_param(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				 uint16_t opcode)
{
	const struct bt_hci_cp_le_set_ext_adv_param *cp = (void *)buf->data;
	struct bt_hci_rp_le_set_ext_adv_param *rp;

	ARG_UNUSED(len);

	zassert_true(buf->len >= sizeof(*cp), "Unexpected command length %u", buf->len);

	last_adv_handle = cp->handle;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->tx_power = 0;
}

/* Handler for BT_HCI_OP_LE_SET_ADV_ENABLE and BT_HCI_OP_LE_SET_EXT_ADV_ENABLE, which both start
 * with the Enable parameter. The tests use one advertising set at a time.
 */
static void le_set_adv_enable(struct net_buf *buf, struct net_buf **evt, uint8_t len,
			      uint16_t opcode)
{
	atomic_val_t busy_us;

	zassert_true(buf->len >= sizeof(uint8_t), "Unexpected command length %u", buf->len);

	if (buf->data[0] != 0U && atomic_get(&reject_adv_enable) != 0) {
		struct bt_hci_evt_cc_status *ccst;

		ccst = cmd_complete(evt, sizeof(*ccst), opcode);
		ccst->status = BT_HCI_ERR_CMD_DISALLOWED;

		return;
	}

	busy_us = (buf->data[0] != 0U) ? atomic_set(&adv_enable_busy_us, 0) : 0;

	(void)atomic_set(&adv_enabled, buf->data[0]);

	generic_success(buf, evt, len, opcode);

	if (busy_us != 0) {
		/* Complete the command, then keep the Host from running anything else */
		(void)bt_hci_recv(DEVICE_DT_INST_GET(0), *evt);
		*evt = NULL;
		k_busy_wait((uint32_t)busy_us);
	}
}

/* Handler for BT_HCI_OP_LE_SET_ADV_DATA and BT_HCI_OP_LE_SET_EXT_ADV_DATA. */
static void le_set_adv_data(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	atomic_val_t delay_ms = atomic_set(&adv_data_delay_ms, 0);

	if (delay_ms != 0) {
		(void)k_sleep(K_MSEC(delay_ms));
	}

	generic_success(buf, evt, len, opcode);
}

/* Handlers for the commands where the Host requires specific return parameters. Any other
 * command is accepted by cmd_handle() with zeroed return parameters.
 */
static const struct cmd_handler cmds[] = {
	{
		BT_HCI_OP_READ_LOCAL_FEATURES,
		sizeof(struct bt_hci_rp_read_local_features),
		read_local_features,
	},
	{
		BT_HCI_OP_READ_SUPPORTED_COMMANDS,
		sizeof(struct bt_hci_rp_read_supported_commands),
		read_supported_commands,
	},
	{
		BT_HCI_OP_READ_BD_ADDR,
		sizeof(struct bt_hci_rp_read_bd_addr),
		read_bd_addr,
	},
	{
		BT_HCI_OP_LE_READ_LOCAL_FEATURES,
		sizeof(struct bt_hci_rp_le_read_local_features),
		le_read_local_features,
	},
	{
		BT_HCI_OP_LE_READ_BUFFER_SIZE,
		sizeof(struct bt_hci_rp_le_read_buffer_size),
		le_read_buffer_size,
	},
	{
		BT_HCI_OP_LE_READ_SUPP_STATES,
		sizeof(struct bt_hci_rp_le_read_supp_states),
		le_read_supp_states,
	},
	{
		BT_HCI_OP_LE_READ_MAX_ADV_DATA_LEN,
		sizeof(struct bt_hci_rp_le_read_max_adv_data_len),
		le_read_max_adv_data_len,
	},
	{
		BT_HCI_OP_LE_SET_EXT_ADV_PARAM,
		sizeof(struct bt_hci_rp_le_set_ext_adv_param),
		le_set_ext_adv_param,
	},
	{
		BT_HCI_OP_LE_SET_ADV_DATA,
		sizeof(struct bt_hci_evt_cc_status),
		le_set_adv_data,
	},
	{
		BT_HCI_OP_LE_SET_EXT_ADV_DATA,
		sizeof(struct bt_hci_evt_cc_status),
		le_set_adv_data,
	},
	{
		BT_HCI_OP_LE_SET_ADV_ENABLE,
		sizeof(struct bt_hci_evt_cc_status),
		le_set_adv_enable,
	},
	{
		BT_HCI_OP_LE_SET_EXT_ADV_ENABLE,
		sizeof(struct bt_hci_evt_cc_status),
		le_set_adv_enable,
	},
};

/* Lookup the command opcode and invoke the handler. */
static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct net_buf *evt = NULL;
	struct bt_hci_cmd_hdr *chdr;
	uint16_t opcode;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

	ARRAY_FOR_EACH_PTR(cmds, handler) {
		if (handler->opcode == opcode) {
			handler->handler(cmd, &evt, handler->len, opcode);
			if (evt != NULL) {
				(void)bt_hci_recv(dev, evt);
			}

			return;
		}
	}

	LOG_DBG("Unhandled opcode 0x%04X", opcode);
	generic_success(cmd, &evt, UNHANDLED_CMD_RP_SIZE, opcode);
	(void)bt_hci_recv(dev, evt);
}

static int driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int driver_send(const struct device *dev, struct net_buf *buf)
{
	uint8_t type = net_buf_pull_u8(buf);

	zassert_equal(type, BT_HCI_H4_CMD, "Unexpected buffer type %u", type);

	cmd_handle(dev, buf);

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

static void *le_meta_event(struct net_buf **buf, uint8_t subevent, uint8_t len)
{
	struct bt_hci_evt_le_meta_event *meta;

	*buf = bt_buf_get_evt(BT_HCI_EVT_LE_META_EVENT, false, K_FOREVER);
	evt_create(*buf, BT_HCI_EVT_LE_META_EVENT, sizeof(*meta) + len);
	meta = net_buf_add(*buf, sizeof(*meta));
	meta->subevent = subevent;

	return net_buf_add(*buf, len);
}

/* Let the test Controller end high duty cycle directed advertising for want of a connection. Like
 * the Zephyr link layer, it sets no parameter of the failed connection besides the status.
 */
static void dir_adv_timeout(void)
{
	struct bt_hci_evt_le_enh_conn_complete *cc;
	struct net_buf *buf;

	(void)atomic_set(&adv_enabled, false);

	cc = le_meta_event(&buf, BT_HCI_EVT_LE_ENH_CONN_COMPLETE, sizeof(*cc));
	(void)memset(cc, 0, sizeof(*cc));
	cc->status = BT_HCI_ERR_ADV_TIMEOUT;
	(void)bt_hci_recv(DEVICE_DT_INST_GET(0), buf);

	if (IS_ENABLED(CONFIG_BT_EXT_ADV)) {
		struct bt_hci_evt_le_adv_set_terminated *term;

		term = le_meta_event(&buf, BT_HCI_EVT_LE_ADV_SET_TERMINATED, sizeof(*term));
		(void)memset(term, 0, sizeof(*term));
		term->status = BT_HCI_ERR_ADV_TIMEOUT;
		term->adv_handle = last_adv_handle;
		(void)bt_hci_recv(DEVICE_DT_INST_GET(0), buf);
	}
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(conn);

	conn_err = err;
	k_sem_give(&connected_sem);
}

static void *lim_adv_setup(void)
{
	static struct bt_conn_cb conn_cb = {
		.connected = connected,
	};
	int err;

	err = bt_enable(NULL);
	zassert_ok(err, "Bluetooth init failed: %d", err);

	err = bt_conn_cb_register(&conn_cb);
	zassert_ok(err, "Failed to register connection callbacks: %d", err);

	return NULL;
}

/* Runs after failed tests too: the next test starts with no advertising and no timeout */
static void lim_adv_after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (atomic_get(&adv_enabled) != 0) {
		int err;

		err = bt_le_adv_stop();
		zassert_ok(err, "Failed to stop advertising: %d", err);
	}

	(void)k_sleep(TIMEOUT_WAIT);
}

ZTEST_SUITE(bt_lim_adv, NULL, lim_adv_setup, NULL, lim_adv_after, NULL);

static const struct bt_data limited_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_LIMITED | BT_LE_AD_NO_BREDR),
};

static const struct bt_data general_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
};

static const struct bt_le_adv_param scan_param = BT_LE_ADV_PARAM_INIT(
	BT_LE_ADV_OPT_SCANNABLE, BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL);

/* Encoded, one byte longer than the scan response data of legacy advertising can be */
static const uint8_t long_data[BT_GAP_ADV_MAX_ADV_DATA_LEN - 1];

static const struct bt_data long_sd[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, long_data, sizeof(long_data)),
};

static const bt_addr_le_t dir_peer = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { { 0x01, 0x02, 0x03, 0x04, 0x05, 0xc6 } },
};

/* The timeout of limited discoverable advertising runs from the start that succeeded */
static ZTEST(bt_lim_adv, test_second_start)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_NCONN, limited_ad, ARRAY_SIZE(limited_ad), NULL, 0);
	zassert_ok(err, "Failed to start advertising: %d", err);

	(void)k_sleep(K_MSEC(TIMEOUT_WAIT_MS / 2));

	err = bt_le_adv_start(BT_LE_ADV_NCONN, limited_ad, ARRAY_SIZE(limited_ad), NULL, 0);
	zassert_equal(err, -EALREADY, "Unexpected second start result: %d", err);

	(void)k_sleep(K_MSEC(TIMEOUT_WAIT_MS / 2));

	zassert_equal(atomic_get(&adv_enabled), 0, "Advertising not ended by the timeout");
}

/* A start that fails leaves no timeout behind to end the advertising started next */
static ZTEST(bt_lim_adv, test_failed_start)
{
	/* The minimum interval is above the maximum */
	const struct bt_le_adv_param invalid_param = BT_LE_ADV_PARAM_INIT(
		BT_LE_ADV_OPT_NONE, BT_GAP_ADV_FAST_INT_MAX_2, BT_GAP_ADV_FAST_INT_MIN_2, NULL);
	struct bt_le_ext_adv *adv;
	int err;

	/* The only advertising set, which every start takes */
	err = bt_le_adv_start(BT_LE_ADV_NCONN, general_ad, ARRAY_SIZE(general_ad), NULL, 0);
	zassert_ok(err, "Failed to start advertising: %d", err);
	adv = bt_le_adv_lookup_legacy();
	zassert_not_null(adv, "No advertising set");
	err = bt_le_adv_stop();
	zassert_ok(err, "Failed to stop advertising: %d", err);

	err = bt_le_adv_start(&invalid_param, limited_ad, ARRAY_SIZE(limited_ad), NULL, 0);
	zassert_equal(err, -EINVAL, "Unexpected start result: %d", err);

	/* Checked before the next start, which cancels a timeout left behind */
	zassert_false(k_work_delayable_is_pending(&adv->lim_adv_timeout_work),
		      "Timeout armed by the failed start");

	err = bt_le_adv_start(BT_LE_ADV_NCONN, general_ad, ARRAY_SIZE(general_ad), NULL, 0);
	zassert_ok(err, "Failed to start advertising: %d", err);

	(void)k_sleep(TIMEOUT_WAIT);

	zassert_equal(atomic_get(&adv_enabled), 1, "Advertising ended by the timeout");
}

/* Data that gets the limited flag starts the timeout */
static ZTEST(bt_lim_adv, test_update_to_limited)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_NCONN, general_ad, ARRAY_SIZE(general_ad), NULL, 0);
	zassert_ok(err, "Failed to start advertising: %d", err);

	err = bt_le_adv_update_data(limited_ad, ARRAY_SIZE(limited_ad), NULL, 0);
	zassert_ok(err, "Failed to update advertising data: %d", err);

	(void)k_sleep(TIMEOUT_WAIT);

	zassert_equal(atomic_get(&adv_enabled), 0, "Advertising not ended by the timeout");
}

/* Data that keeps the limited flag keeps the timeout of the start */
static ZTEST(bt_lim_adv, test_update_keeps_limited)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_NCONN, limited_ad, ARRAY_SIZE(limited_ad), NULL, 0);
	zassert_ok(err, "Failed to start advertising: %d", err);

	(void)k_sleep(K_MSEC(TIMEOUT_WAIT_MS / 2));

	err = bt_le_adv_update_data(limited_ad, ARRAY_SIZE(limited_ad), NULL, 0);
	zassert_ok(err, "Failed to update advertising data: %d", err);

	(void)k_sleep(K_MSEC(TIMEOUT_WAIT_MS / 2));

	zassert_equal(atomic_get(&adv_enabled), 0, "Advertising not ended by the timeout");
}

/* Data that loses the limited flag leaves no timeout to end the advertising */
static ZTEST(bt_lim_adv, test_update_to_general)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_NCONN, limited_ad, ARRAY_SIZE(limited_ad), NULL, 0);
	zassert_ok(err, "Failed to start advertising: %d", err);

	err = bt_le_adv_update_data(general_ad, ARRAY_SIZE(general_ad), NULL, 0);
	zassert_ok(err, "Failed to update advertising data: %d", err);

	(void)k_sleep(TIMEOUT_WAIT);

	zassert_equal(atomic_get(&adv_enabled), 1, "Advertising ended by the timeout");
}

static void update_limited(void *p1, void *p2, void *p3)
{
	int *err = p1;

	ARGS_UNUSED(p2, p3);

	*err = bt_le_adv_update_data(limited_ad, ARRAY_SIZE(limited_ad), NULL, 0);
}

/* An update that returns after the timeout ended the advertising leaves no timeout behind to end
 * the advertising started next
 */
static ZTEST(bt_lim_adv, test_update_during_timeout)
{
	static K_THREAD_STACK_DEFINE(update_stack, 2048);
	static struct k_thread update_thread;
	k_tid_t tid;
	int update_err;
	int err;

	err = bt_le_adv_start(BT_LE_ADV_NCONN, limited_ad, ARRAY_SIZE(limited_ad), NULL, 0);
	zassert_ok(err, "Failed to start advertising: %d", err);

	(void)k_sleep(K_MSEC(TIMEOUT_MS - 50));

	/* The timeout expires while the test Controller holds the data command, and the Host
	 * completes the stop before the lower priority thread of the update runs again.
	 */
	(void)atomic_set(&adv_data_delay_ms, 100);
	tid = k_thread_create(&update_thread, update_stack, K_THREAD_STACK_SIZEOF(update_stack),
			      update_limited, &update_err, NULL, NULL, K_PRIO_PREEMPT(5), 0,
			      K_NO_WAIT);
	(void)k_thread_join(tid, K_FOREVER);
	zassert_ok(update_err, "Failed to update advertising data: %d", update_err);
	zassert_equal(atomic_get(&adv_enabled), 0, "Advertising not ended by the timeout");

	/* A timeout that the update armed would expire after the next start enabled the
	 * advertising and before the start returned.
	 */
	(void)k_sleep(K_MSEC(TIMEOUT_MS - 50));
	(void)atomic_set(&adv_enable_busy_us, 100 * USEC_PER_MSEC);

	err = bt_le_adv_start(BT_LE_ADV_NCONN, general_ad, ARRAY_SIZE(general_ad), NULL, 0);
	zassert_ok(err, "Failed to start advertising: %d", err);

	(void)k_sleep(TIMEOUT_WAIT);

	zassert_equal(atomic_get(&adv_enabled), 1, "Advertising ended by the timeout");
}

/* The advertising data of an update whose scan response fails still starts the timeout */
static ZTEST(bt_lim_adv, test_update_to_limited_sd_fails)
{
	int err;

	err = bt_le_adv_start(&scan_param, general_ad, ARRAY_SIZE(general_ad), NULL, 0);
	zassert_ok(err, "Failed to start advertising: %d", err);

	err = bt_le_adv_update_data(limited_ad, ARRAY_SIZE(limited_ad), long_sd,
				    ARRAY_SIZE(long_sd));
	zassert_equal(err, -EINVAL, "Unexpected update result: %d", err);

	(void)k_sleep(TIMEOUT_WAIT);

	zassert_equal(atomic_get(&adv_enabled), 0, "Advertising not ended by the timeout");
}

/* The advertising data of an update whose scan response fails still cancels the timeout */
static ZTEST(bt_lim_adv, test_update_to_general_sd_fails)
{
	int err;

	err = bt_le_adv_start(&scan_param, limited_ad, ARRAY_SIZE(limited_ad), NULL, 0);
	zassert_ok(err, "Failed to start advertising: %d", err);

	err = bt_le_adv_update_data(general_ad, ARRAY_SIZE(general_ad), long_sd,
				    ARRAY_SIZE(long_sd));
	zassert_equal(err, -EINVAL, "Unexpected update result: %d", err);

	(void)k_sleep(TIMEOUT_WAIT);

	zassert_equal(atomic_get(&adv_enabled), 1, "Advertising ended by the timeout");
}

/* Directed advertising ignores the data, so updated flags start no timeout either */
static ZTEST(bt_lim_adv, test_update_directed)
{
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN_DIR_LOW_DUTY(&dir_peer), NULL, 0, NULL, 0);
	zassert_ok(err, "Failed to start directed advertising: %d", err);

	err = bt_le_adv_update_data(limited_ad, ARRAY_SIZE(limited_ad), NULL, 0);
	zassert_ok(err, "Failed to update advertising data: %d", err);

	(void)k_sleep(TIMEOUT_WAIT);

	zassert_equal(atomic_get(&adv_enabled), 1, "Advertising ended by the timeout");
}

#if defined(CONFIG_BT_EXT_ADV)
/* An extended advertising start that fails leaves no limited set for the RPA timeout to skip */
static ZTEST(bt_lim_adv, test_failed_ext_start)
{
	const struct bt_le_ext_adv_start_param start_param = {
		.timeout = 100U, /* 1 s, in units of 10 ms */
	};
	struct bt_le_ext_adv *adv;
	bool limited;
	int err;

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, &adv);
	zassert_ok(err, "Failed to create advertising set: %d", err);

	(void)atomic_set(&reject_adv_enable, true);
	err = bt_le_ext_adv_start(adv, &start_param);
	(void)atomic_set(&reject_adv_enable, false);
	zassert_equal(err, -EACCES, "Unexpected start result: %d", err);

	limited = atomic_test_bit(adv->flags, BT_ADV_LIMITED);

	/* Checked once the set is deleted, so that a failure leaves no set to the next test */
	err = bt_le_ext_adv_delete(adv);
	zassert_ok(err, "Failed to delete advertising set: %d", err);

	zassert_false(limited, "Set left limited by the failed start");
}
#endif /* CONFIG_BT_EXT_ADV */

/* Directed advertising ignores the data, so its flags start no timeout */
static ZTEST(bt_lim_adv, test_directed_start)
{
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_le_adv_start(BT_LE_ADV_CONN_DIR(&dir_peer), limited_ad, ARRAY_SIZE(limited_ad),
			      NULL, 0);
	zassert_ok(err, "Failed to start directed advertising: %d", err);
	adv = bt_le_adv_lookup_legacy();
	zassert_not_null(adv, "No advertising set");

	dir_adv_timeout();

	err = k_sem_take(&connected_sem, K_SECONDS(1));
	zassert_ok(err, "No connected callback");
	zassert_equal(conn_err, BT_HCI_ERR_ADV_TIMEOUT, "Unexpected error 0x%02x", conn_err);

	/* Checked before the next start, which cancels a timeout left behind */
	zassert_false(k_work_delayable_is_pending(&adv->lim_adv_timeout_work),
		      "Timeout armed by the directed start");

	err = bt_le_adv_start(BT_LE_ADV_NCONN, general_ad, ARRAY_SIZE(general_ad), NULL, 0);
	zassert_ok(err, "Failed to start advertising: %d", err);

	(void)k_sleep(TIMEOUT_WAIT);

	zassert_equal(atomic_get(&adv_enabled), 1, "Advertising ended by the timeout");
}
