/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/toolchain.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_assert.h>
#include <zephyr/ztest_test.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_adv_sid_test, CONFIG_BT_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Length of the command complete parameters used for commands without a dedicated handler.
 * The parameters are zeroed, and the size is large enough to hold the return parameters of any
 * command that the Host may send during initialization.
 */
#define UNHANDLED_CMD_RP_SIZE 64U

/* SID of the most recent LE Set Extended Advertising Parameters command. */
static uint8_t last_hci_sid;

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

	ccst = cmd_complete(evt, len, opcode);

	/* Fill any event parameters with zero */
	(void)memset(ccst, 0, len);

	ccst->status = BT_HCI_ERR_SUCCESS;
}

/* Handler for BT_HCI_OP_READ_LOCAL_FEATURES. */
static void read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	/* LE supported, and BR/EDR not supported */
	rp->features[4] = BIT(5) | BIT(6);
}

/* Handler for BT_HCI_OP_READ_SUPPORTED_COMMANDS. */
static void read_supported_commands(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				    uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
}

/* Handler for BT_HCI_OP_LE_READ_LOCAL_FEATURES. */
static void le_read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				   uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	/* Only report support for LE Extended Advertising */
	rp->features[BT_LE_FEAT_BIT_EXT_ADV / 8] = BIT(BT_LE_FEAT_BIT_EXT_ADV % 8);
}

/* Handler for BT_HCI_OP_LE_READ_SUPP_STATES. */
static void le_read_supp_states(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->le_states, 0xFF, sizeof(rp->le_states));
}

/* Handler for BT_HCI_OP_LE_READ_MAX_ADV_DATA_LEN. */
static void le_read_max_adv_data_len(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				     uint16_t opcode)
{
	struct bt_hci_rp_le_read_max_adv_data_len *rp;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->max_adv_data_len = sys_cpu_to_le16(BT_GAP_ADV_MAX_EXT_ADV_DATA_LEN);
}

/* Handler for BT_HCI_OP_LE_SET_EXT_ADV_PARAM. It stores the SID that the Host sent to the
 * Controller so that the test can verify it.
 */
static void le_set_ext_adv_param(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				 uint16_t opcode)
{
	const struct bt_hci_cp_le_set_ext_adv_param *cp = (void *)buf->data;
	struct bt_hci_rp_le_set_ext_adv_param *rp;

	zassert_true(buf->len >= sizeof(*cp), "Unexpected command length %u", buf->len);

	last_hci_sid = cp->sid;

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->tx_power = 0;
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
		BT_HCI_OP_LE_READ_LOCAL_FEATURES,
		sizeof(struct bt_hci_rp_le_read_local_features),
		le_read_local_features,
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
};

/* Lookup the command opcode and invoke the handler. */
static void cmd_handle(const struct device *dev, struct net_buf *cmd)
{
	struct net_buf *evt = NULL;
	struct bt_hci_cmd_hdr *chdr;
	uint16_t opcode;

	chdr = net_buf_pull_mem(cmd, sizeof(*chdr));
	opcode = sys_le16_to_cpu(chdr->opcode);

	for (size_t i = 0U; i < ARRAY_SIZE(cmds); i++) {
		if (cmds[i].opcode == opcode) {
			cmds[i].handler(cmd, &evt, cmds[i].len, opcode);
			bt_hci_recv(dev, evt);

			return;
		}
	}

	/* The Host may send other commands during initialization. Those are all accepted with
	 * zeroed return parameters, as their content is not relevant for this test.
	 */
	LOG_DBG("Unhandled opcode 0x%04X", opcode);
	generic_success(cmd, &evt, UNHANDLED_CMD_RP_SIZE, opcode);
	bt_hci_recv(dev, evt);
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

static struct bt_le_adv_param adv_param_get(uint8_t sid, bool ext_adv)
{
	struct bt_le_adv_param param = {
		.id = BT_ID_DEFAULT,
		.sid = sid,
		.secondary_max_skip = 0U,
		.options = ext_adv ? BT_LE_ADV_OPT_EXT_ADV : BT_LE_ADV_OPT_NONE,
		.interval_min = BT_GAP_ADV_SLOW_INT_MIN,
		.interval_max = BT_GAP_ADV_SLOW_INT_MAX,
		.peer = NULL,
	};

	return param;
}

static uint8_t adv_sid_get(struct bt_le_ext_adv *adv)
{
	struct bt_le_ext_adv_info info;
	int err;

	err = bt_le_ext_adv_get_info(adv, &info);
	zassert_ok(err, "Failed to get advertising set info: %d", err);

	return info.sid;
}

static void *adv_sid_setup(void)
{
	int err;

	err = bt_enable(NULL);
	zassert_ok(err, "Bluetooth init failed: %d", err);

	return NULL;
}

static void adv_sid_before(void *f)
{
	ARG_UNUSED(f);

	last_hci_sid = BT_HCI_LE_EXT_ADV_SID_INVALID;
}

ZTEST_SUITE(bt_adv_sid, NULL, adv_sid_setup, adv_sid_before, NULL, NULL);

static ZTEST(bt_adv_sid, test_ext_adv_uses_param_sid)
{
	const uint8_t sids[] = {BT_GAP_SID_MIN, 5U, BT_GAP_SID_MAX};

	for (size_t i = 0U; i < ARRAY_SIZE(sids); i++) {
		struct bt_le_adv_param param = adv_param_get(sids[i], true);
		struct bt_le_ext_adv *adv;
		int err;

		err = bt_le_ext_adv_create(&param, NULL, &adv);
		zassert_ok(err, "Failed to create advertising set with SID %u: %d", sids[i], err);

		zassert_equal(adv_sid_get(adv), sids[i],
			      "SID %u was not set in the advertising set", sids[i]);
		zassert_equal(last_hci_sid, sids[i], "SID %u was not sent to the controller",
			      sids[i]);

		err = bt_le_ext_adv_delete(adv);
		zassert_ok(err, "Failed to delete advertising set: %d", err);
	}
}

static ZTEST(bt_adv_sid, test_legacy_adv_sid_is_invalid)
{
	struct bt_le_adv_param param = adv_param_get(BT_GAP_SID_MAX + 1U, false);
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_le_ext_adv_create(&param, NULL, &adv);
	zassert_ok(err, "Failed to create legacy advertising set: %d", err);

	zassert_equal(adv_sid_get(adv), BT_GAP_SID_INVALID,
		      "SID of a legacy advertising set shall be BT_GAP_SID_INVALID");
	zassert_equal(last_hci_sid, 0U, "SID sent to the controller shall be 0 for legacy");

	err = bt_le_ext_adv_delete(adv);
	zassert_ok(err, "Failed to delete advertising set: %d", err);
}

static ZTEST(bt_adv_sid, test_ext_adv_invalid_sid_rejected)
{
	const uint8_t sids[] = {BT_GAP_SID_MAX + 1U, BT_GAP_SID_INVALID};

	for (size_t i = 0U; i < ARRAY_SIZE(sids); i++) {
		struct bt_le_adv_param param = adv_param_get(sids[i], true);
		struct bt_le_ext_adv *adv;
		int err;

		err = bt_le_ext_adv_create(&param, NULL, &adv);
		zassert_equal(err, -EINVAL, "Creating advertising set with SID %u returned %d",
			      sids[i], err);
	}
}

static ZTEST(bt_adv_sid, test_update_param_updates_sid)
{
	struct bt_le_adv_param param = adv_param_get(3U, true);
	struct bt_le_ext_adv *adv;
	int err;

	err = bt_le_ext_adv_create(&param, NULL, &adv);
	zassert_ok(err, "Failed to create advertising set: %d", err);
	zassert_equal(adv_sid_get(adv), 3U, "SID was not set in the advertising set");

	/* Updating with legacy parameters shall invalidate the SID */
	param = adv_param_get(BT_GAP_SID_MAX + 1U, false);
	err = bt_le_ext_adv_update_param(adv, &param);
	zassert_ok(err, "Failed to update advertising parameters: %d", err);
	zassert_equal(adv_sid_get(adv), BT_GAP_SID_INVALID,
		      "SID of a legacy advertising set shall be BT_GAP_SID_INVALID");

	/* Updating with extended parameters shall set the SID again */
	param = adv_param_get(7U, true);
	err = bt_le_ext_adv_update_param(adv, &param);
	zassert_ok(err, "Failed to update advertising parameters: %d", err);
	zassert_equal(adv_sid_get(adv), 7U, "SID was not set in the advertising set");

	err = bt_le_ext_adv_delete(adv);
	zassert_ok(err, "Failed to delete advertising set: %d", err);
}
