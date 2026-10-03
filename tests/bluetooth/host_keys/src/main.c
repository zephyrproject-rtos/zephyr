/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/keys.h>

#define DT_DRV_COMPAT zephyr_bt_hci_test

#define GENERIC_RP_SIZE 64U

#define NUM_PEERS 2U

static const bt_addr_le_t peers[NUM_PEERS] = {
	{
		.type = BT_ADDR_LE_PUBLIC,
		.a.val = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06},
	},
	{
		.type = BT_ADDR_LE_PUBLIC,
		.a.val = {0x11, 0x12, 0x13, 0x14, 0x15, 0x16},
	},
};

static uint8_t rl_adds[NUM_PEERS];
static uint8_t rl_rejects[NUM_PEERS];

struct cmd_handler {
	uint16_t opcode;
	uint8_t len;
	void (*handler)(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode);
};

static void evt_create(struct net_buf *buf, uint8_t evt, uint8_t len)
{
	struct bt_hci_evt_hdr *hdr;

	hdr = net_buf_add(buf, sizeof(*hdr));
	hdr->evt = evt;
	hdr->len = len;
}

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

static void generic_success(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cc_status *rp;

	ARG_UNUSED(buf);

	rp = cmd_complete(evt, len, opcode);
	(void)memset(rp, 0, len);
	rp->status = BT_HCI_ERR_SUCCESS;
}

static void le_rand(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_rp_le_rand *rp;
	static uint8_t rand_value;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;

	for (size_t i = 0U; i < ARRAY_SIZE(rp->rand); i++) {
		rp->rand[i] = rand_value++;
	}
}

static void read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_read_local_features *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	/* LE supported and BR/EDR not supported. */
	rp->features[4] = BIT(5) | BIT(6);
}

static void read_supported_commands(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				    uint16_t opcode)
{
	struct bt_hci_rp_read_supported_commands *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->commands, 0xFF, sizeof(rp->commands));
}

static void read_bd_addr(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_rp_read_bd_addr *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(&rp->bdaddr, 0x11, sizeof(rp->bdaddr));
}

static void le_read_local_features(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				   uint16_t opcode)
{
	struct bt_hci_rp_le_read_local_features *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->features, 0, sizeof(rp->features));
	rp->features[0] = BIT(BT_LE_FEAT_BIT_PRIVACY);
}

static void le_read_supp_states(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_supp_states *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	(void)memset(rp->le_states, 0xFF, sizeof(rp->le_states));
}

static void le_read_buffer_size(struct net_buf *buf, struct net_buf **evt, uint8_t len,
				uint16_t opcode)
{
	struct bt_hci_rp_le_read_buffer_size *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->le_max_len = sys_cpu_to_le16(27U);
	rp->le_max_num = 1U;
}

static void le_read_rl_size(struct net_buf *buf, struct net_buf **evt, uint8_t len,
			    uint16_t opcode)
{
	struct bt_hci_rp_le_read_rl_size *rp;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;
	rp->rl_size = CONFIG_TEST_RL_SIZE;
}

static void le_add_dev_to_rl(struct net_buf *buf, struct net_buf **evt, uint8_t len,
			     uint16_t opcode)
{
	struct bt_hci_cp_le_add_dev_to_rl *cp = (void *)buf->data;
	struct bt_hci_evt_cc_status *rp;

	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	rp->status = BT_HCI_ERR_SUCCESS;

	for (size_t i = 0U; i < NUM_PEERS; i++) {
		if (!bt_addr_le_eq(&cp->peer_id_addr, &peers[i])) {
			continue;
		}

		rl_adds[i]++;

		/* The Controller rejects an entry that is already in the list. */
		if (rl_adds[i] > 1U) {
			rl_rejects[i]++;
			rp->status = BT_HCI_ERR_INVALID_PARAM;
		}
	}
}

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
		BT_HCI_OP_LE_RAND,
		sizeof(struct bt_hci_rp_le_rand),
		le_rand,
	},
	{
		BT_HCI_OP_LE_READ_RL_SIZE,
		sizeof(struct bt_hci_rp_le_read_rl_size),
		le_read_rl_size,
	},
	{
		BT_HCI_OP_LE_ADD_DEV_TO_RL,
		sizeof(struct bt_hci_evt_cc_status),
		le_add_dev_to_rl,
	},
};

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

	generic_success(cmd, &evt, GENERIC_RP_SIZE, opcode);
	bt_hci_recv(dev, evt);
}

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

	zassert_equal(type, BT_HCI_H4_CMD, "Unexpected buffer type %u", type);
	cmd_handle(dev, buf);
	net_buf_unref(buf);

	return 0;
}

static DEVICE_API(bt_hci, driver_api) = {
	.open = driver_open,
	.close = driver_close,
	.send = driver_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data driver_data_##inst = {0};                                 \
	static const struct bt_hci_driver_config driver_config_##inst =                            \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &driver_data_##inst, &driver_config_##inst,        \
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

static void store_bond(size_t idx)
{
	struct bt_keys *keys;

	keys = bt_keys_get_type(BT_KEYS_IRK, BT_ID_DEFAULT, &peers[idx]);
	zassert_not_null(keys, "No room for peer %zu's keys", idx);
	(void)memset(keys->irk.val, 0x5A + idx, sizeof(keys->irk.val));
	zassert_ok(bt_keys_store(keys), "Failed to store peer %zu's keys", idx);
}

static void *keys_setup(void)
{
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	store_bond(0U);
	/* Drop the in-RAM copy so that the first settings_load() has to restore it. */
	bt_keys_reset();

	return NULL;
}

static void load_and_advertise(void)
{
	zassert_ok(settings_load(), "settings_load() failed");
	zassert_ok(bt_le_adv_start(BT_LE_ADV_NCONN, NULL, 0, NULL, 0), "Advertising failed");
	zassert_ok(bt_le_adv_stop(), "Stopping advertising failed");
}

ZTEST_SUITE(bt_keys_commit, NULL, keys_setup, NULL, NULL, NULL);

ZTEST(bt_keys_commit, test_settings_reload_does_not_add_irk_twice)
{
	const uint8_t expected_adds = (CONFIG_TEST_RL_SIZE > 0) ? 1U : 0U;
	struct bt_keys *keys;

	zassert_is_null(bt_keys_find(BT_KEYS_IRK, BT_ID_DEFAULT, &peers[0]),
			"The peer's keys survived bt_keys_reset()");

	load_and_advertise();

	keys = bt_keys_find(BT_KEYS_IRK, BT_ID_DEFAULT, &peers[0]);
	zassert_not_null(keys, "The peer's keys were not restored");
	zassert_true((keys->state & BT_KEYS_ID_ADDED) != 0U, "The peer's IRK was never added");
	zassert_equal(bt_dev.le.rl_entries, 1U, "rl_entries %u after the first load",
		      bt_dev.le.rl_entries);
	zassert_equal(rl_adds[0], expected_adds, "%u adds after the first load", rl_adds[0]);

	store_bond(1U);

	load_and_advertise();

	zassert_equal(rl_adds[0], expected_adds, "%u adds of the first peer after the second load",
		      rl_adds[0]);
	zassert_equal(rl_adds[1], expected_adds, "%u adds of the second peer after the second load",
		      rl_adds[1]);
	zassert_equal(rl_rejects[0], 0U, "%u adds of the first peer rejected", rl_rejects[0]);
	zassert_equal(rl_rejects[1], 0U, "%u adds of the second peer rejected", rl_rejects[1]);
	zassert_equal(bt_dev.le.rl_entries, 2U, "rl_entries %u after the second load",
		      bt_dev.le.rl_entries);
}
