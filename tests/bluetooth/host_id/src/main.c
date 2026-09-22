/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#if defined(CONFIG_BT_PRIVACY)
#include <common/rpa.h>
#include <host/keys.h>
#endif

#define DT_DRV_COMPAT zephyr_bt_hci_test

/* Large enough for the return parameters of initialization commands that are
 * irrelevant to these tests.
 */
#define GENERIC_RP_SIZE 64U

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

static bool rand_failure;

static void le_rand(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_rp_le_rand *rp;
	static uint8_t rand_value;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	rp = cmd_complete(evt, sizeof(*rp), opcode);
	if (rand_failure) {
		rp->status = BT_HCI_ERR_HW_FAILURE;
		return;
	}

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

static void command_status(struct net_buf *buf, struct net_buf **evt, uint8_t len, uint16_t opcode)
{
	struct bt_hci_evt_cmd_status *cs;

	ARG_UNUSED(buf);
	ARG_UNUSED(len);

	*evt = bt_buf_get_evt(BT_HCI_EVT_CMD_STATUS, false, K_FOREVER);
	evt_create(*evt, BT_HCI_EVT_CMD_STATUS, sizeof(*cs));
	cs = net_buf_add(*evt, sizeof(*cs));
	cs->status = BT_HCI_ERR_SUCCESS;
	cs->ncmd = 1U;
	cs->opcode = sys_cpu_to_le16(opcode);
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
		BT_HCI_OP_LE_CREATE_CONN,
		0U,
		command_status,
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

static bt_addr_le_t test_addr(uint8_t value)
{
	bt_addr_le_t addr = {
		.type = BT_ADDR_LE_RANDOM,
		.a.val = {value, 0x22, 0x33, 0x44, 0x55, 0xC0},
	};

	return addr;
}

static size_t identity_count(void)
{
	size_t count = 0U;

	bt_id_get(NULL, &count);
	return count;
}

static void cleanup_secondary_identities(void)
{
	size_t count = identity_count();

	while (count > 1U) {
		const uint8_t id = (uint8_t)(count - 1U);
		int err;

		err = bt_id_delete(id);
		if (err == -EALREADY) {
			bt_addr_le_t addr = test_addr((uint8_t)(0x80U + id));

			/* Deleting the last identity shrinks the identity count, but an empty
			 * slot below it remains counted. Reclaim the slot before deleting it
			 * so cleanup can return to the default identity only.
			 */
			err = bt_id_reset(id, &addr, NULL);
			zassert_equal(err, id, "Failed to restore empty identity %u", id);
			err = bt_id_delete(id);
		}

		zassert_ok(err, "Failed to delete identity %u: %d", id, err);
		count = identity_count();
	}
}

static void *id_setup(void)
{
	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		bt_addr_le_t addr = test_addr(0x10U);
		uint8_t irk[BT_IRK_SIZE] = {1U};

		zassert_equal(bt_id_create(&addr, irk), BT_ID_DEFAULT,
			      "Failed to create the identity before enabling Bluetooth");
		zassert_equal(bt_id_reset_irk(BT_ID_DEFAULT, NULL), -EAGAIN,
			      "IRK reset was allowed before the stack was ready");
	}

	int err;

	err = bt_enable(NULL);
	zassert_ok(err, "Bluetooth init failed: %d", err);
	zassert_equal(identity_count(), 1U, "Expected exactly one default identity");

	return NULL;
}

static void id_before(void *fixture)
{
	ARG_UNUSED(fixture);

	cleanup_secondary_identities();
}

static void id_after(void *fixture)
{
	ARG_UNUSED(fixture);

	cleanup_secondary_identities();
}

ZTEST_SUITE(bt_id_public_api, NULL, id_setup, id_before, id_after, NULL);

ZTEST(bt_id_public_api, test_create_generated_addresses)
{
	bt_addr_le_t generated_addr = *BT_ADDR_LE_ANY;
	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	size_t count = ARRAY_SIZE(addrs);
	int id1;
	int id2;

	id1 = bt_id_create(NULL, NULL);
	zassert_equal(id1, 1, "Generated identity got id %d", id1);

	id2 = bt_id_create(&generated_addr, NULL);
	zassert_equal(id2, 2, "Generated identity with output address got id %d", id2);
	zassert_equal(generated_addr.type, BT_ADDR_LE_RANDOM,
		      "Generated identity address is not random");
	zassert_true(BT_ADDR_IS_STATIC(&generated_addr.a),
		     "Generated identity address is not static random");

	bt_id_get(addrs, &count);
	zassert_equal(count, 3U, "Unexpected identity count after generated identities");
	zassert_equal(addrs[id1].type, BT_ADDR_LE_RANDOM,
		      "NULL-address identity is not random");
	zassert_true(BT_ADDR_IS_STATIC(&addrs[id1].a),
		     "NULL-address identity is not static random");
	zassert_mem_equal(&addrs[id2], &generated_addr, sizeof(generated_addr),
			  "Generated address was not copied back to the caller");
	zassert_false(bt_addr_le_eq(&addrs[BT_ID_DEFAULT], &addrs[id1]),
		      "Generated identity duplicates the default identity");
	zassert_false(bt_addr_le_eq(&addrs[BT_ID_DEFAULT], &addrs[id2]),
		      "Generated identity duplicates the default identity");
	zassert_false(bt_addr_le_eq(&addrs[id1], &addrs[id2]),
		      "Generated identities have duplicate addresses");
}

ZTEST(bt_id_public_api, test_create_errors_and_capacity)
{
	bt_addr_le_t invalid_addr = test_addr(1U);
	bt_addr_le_t addr1 = test_addr(2U);
	bt_addr_le_t addr2 = test_addr(3U);
	bt_addr_le_t addr3 = test_addr(4U);
	bt_addr_le_t addr4 = test_addr(5U);
	bt_addr_le_t public_addr = test_addr(6U);
	uint8_t irk[BT_IRK_SIZE] = {1U};
	int id;

	invalid_addr.a.val[5] = 0x40U;
	public_addr.type = BT_ADDR_LE_PUBLIC;
	zassert_equal(bt_id_create(&invalid_addr, NULL), -EINVAL,
		      "Non-static random identity address was accepted");
	zassert_equal(bt_id_create(&public_addr, NULL), -EINVAL,
		      "Public identity address was accepted without controller support");
	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		zassert_equal(bt_id_create(&addr1, irk), 1, "IRK was not accepted");
	} else {
		zassert_equal(bt_id_create(&addr1, irk), -EINVAL,
			      "IRK was accepted while privacy is disabled");
	}

	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		id = 1;
	} else {
		id = bt_id_create(&addr1, NULL);
		zassert_equal(id, 1, "First secondary identity got id %d", id);
	}
	zassert_equal(bt_id_create(&addr1, NULL), -EALREADY,
		      "Duplicate identity address was accepted");

	id = bt_id_create(&addr2, NULL);
	zassert_equal(id, 2, "Second secondary identity got id %d", id);
	id = bt_id_create(&addr3, NULL);
	zassert_equal(id, 3, "Third secondary identity got id %d", id);

	zassert_equal(bt_id_create(&addr4, NULL), -ENOMEM,
		      "Identity was created after the identity pool was full");
	zassert_equal(identity_count(), CONFIG_BT_ID_MAX, "Unexpected identity count");
}

ZTEST(bt_id_public_api, test_reset_errors_and_updates_identity)
{
	bt_addr_le_t invalid_addr = test_addr(1U);
	bt_addr_le_t addr1 = test_addr(2U);
	bt_addr_le_t addr2 = test_addr(3U);
	bt_addr_le_t addr3 = test_addr(4U);
	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	uint8_t irk[BT_IRK_SIZE] = {1U};
	size_t count = ARRAY_SIZE(addrs);
	uint8_t first_unused_id;
	int id1;
	int id2;

	id1 = bt_id_create(&addr1, NULL);
	id2 = bt_id_create(&addr2, NULL);
	zassert_equal(id1, 1, "Unexpected first secondary identity id");
	zassert_equal(id2, 2, "Unexpected second secondary identity id");
	first_unused_id = (uint8_t)identity_count();

	invalid_addr.a.val[5] = 0x40U;
	zassert_equal(bt_id_reset((uint8_t)id1, &invalid_addr, NULL), -EINVAL,
		      "Non-static random reset address was accepted");
	if (!IS_ENABLED(CONFIG_BT_PRIVACY)) {
		zassert_equal(bt_id_reset((uint8_t)id1, &addr3, irk), -EINVAL,
			      "IRK was accepted while privacy is disabled");
	}
	zassert_equal(bt_id_reset(BT_ID_DEFAULT, &addr3, NULL), -EINVAL,
		      "Default identity was reset");
	zassert_equal(bt_id_reset(first_unused_id, &addr3, NULL), -EINVAL,
		      "First unused identity handle was reset");
	zassert_equal(bt_id_reset((uint8_t)id1, &addr2, NULL), -EALREADY,
		      "Identity was reset to an address already in use");

	zassert_equal(bt_id_reset((uint8_t)id1, &addr3, NULL), id1,
		      "Valid identity reset failed");
	bt_id_get(addrs, &count);
	zassert_equal(count, 3U, "Unexpected identity count after reset");
	zassert_mem_equal(&addrs[id1], &addr3, sizeof(addr3), "Identity address was not updated");
}

ZTEST(bt_id_public_api, test_delete_errors_and_empty_slot)
{
	bt_addr_le_t addr1 = test_addr(2U);
	bt_addr_le_t addr2 = test_addr(3U);
	bt_addr_le_t addr3 = test_addr(4U);
	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	size_t count = ARRAY_SIZE(addrs);
	uint8_t first_unused_id;
	int id1;
	int id2;

	id1 = bt_id_create(&addr1, NULL);
	id2 = bt_id_create(&addr2, NULL);
	zassert_equal(id1, 1, "Unexpected first secondary identity id");
	zassert_equal(id2, 2, "Unexpected second secondary identity id");
	first_unused_id = (uint8_t)identity_count();

	zassert_equal(bt_id_delete(BT_ID_DEFAULT), -EINVAL, "Default identity was deleted");
	zassert_equal(bt_id_delete(first_unused_id), -EINVAL,
		      "First unused identity handle was deleted");

	zassert_ok(bt_id_delete((uint8_t)id1), "Failed to delete non-tail identity");
	zassert_equal(identity_count(), 3U, "Deleting a non-tail identity changed identity count");
	zassert_equal(bt_id_delete((uint8_t)id1), -EALREADY,
		      "Deleting an empty identity slot did not return -EALREADY");

	zassert_equal(bt_id_reset((uint8_t)id1, &addr3, NULL), id1,
		      "Failed to reclaim deleted identity slot");
	bt_id_get(addrs, &count);
	zassert_equal(count, 3U, "Reclaiming an identity changed identity count");
	zassert_mem_equal(&addrs[id1], &addr3, sizeof(addr3),
			  "Reclaimed identity address was not updated");

	zassert_ok(bt_id_delete((uint8_t)id2), "Failed to delete tail identity");
	zassert_equal(identity_count(), 2U,
		      "Deleting the tail identity did not shrink identity count");
}

#if defined(CONFIG_BT_PRIVACY)
static bool is_zero_irk(const uint8_t *irk)
{
	for (size_t i = 0U; i < BT_IRK_SIZE; i++) {
		if (irk[i] != 0U) {
			return false;
		}
	}

	return true;
}

static ZTEST(bt_id_public_api, test_reset_irk_updates_key)
{
	bt_addr_le_t before;
	bt_addr_le_t after;
	struct bt_le_oob old_oob = {0};
	struct bt_le_oob new_oob = {0};
	uint8_t old_irk[BT_IRK_SIZE] = {1U};
	const uint8_t expected_irk[BT_IRK_SIZE] = {2U};
	uint8_t supplied_irk[BT_IRK_SIZE] = {2U};
	uint8_t generated_irk[BT_IRK_SIZE] = {0};
	size_t count = 1U;

	bt_id_get(&before, &count);
	zassert_equal(count, 1U, "Unexpected identity count");

	zassert_ok(bt_id_reset_irk(BT_ID_DEFAULT, old_irk), "Failed to set the old IRK");
	zassert_ok(bt_le_oob_get_local(BT_ID_DEFAULT, &old_oob), "Failed to get the old RPA");
	zassert_true(bt_addr_le_is_rpa(&old_oob.addr), "The old address is not an RPA");
	zassert_true(bt_rpa_irk_matches(old_irk, &old_oob.addr.a),
		     "The old RPA does not match the old IRK");

	zassert_ok(bt_id_reset_irk(BT_ID_DEFAULT, supplied_irk), "Failed to set the supplied IRK");
	zassert_mem_equal(supplied_irk, expected_irk, sizeof(supplied_irk),
			  "The supplied IRK was modified");

	count = 1U;
	bt_id_get(&after, &count);
	zassert_equal(count, 1U, "Unexpected identity count after IRK reset");
	zassert_mem_equal(&before, &after, sizeof(before), "Identity address changed");

	zassert_ok(bt_le_oob_get_local(BT_ID_DEFAULT, &new_oob), "Failed to get the new RPA");
	zassert_true(bt_addr_le_is_rpa(&new_oob.addr), "The new address is not an RPA");
	zassert_true(bt_rpa_irk_matches(supplied_irk, &new_oob.addr.a),
		     "The new RPA does not match the supplied IRK");
	zassert_false(bt_rpa_irk_matches(old_irk, &new_oob.addr.a),
		      "The new RPA still matches the old IRK");
	zassert_false(bt_addr_le_eq(&old_oob.addr, &new_oob.addr),
		      "The RPA did not change after the IRK reset");

	zassert_ok(bt_id_reset_irk(BT_ID_DEFAULT, generated_irk), "Failed to generate a new IRK");
	zassert_false(is_zero_irk(generated_irk), "Generated IRK is all zeroes");
}

#if defined(CONFIG_BT_RPA_SHARING)
static ZTEST(bt_id_public_api, test_reset_irk_invalidates_cached_rpa)
{
	struct bt_le_ext_adv *adv;
	struct bt_le_ext_adv_info info;
	bt_addr_le_t old_rpa;
	bt_addr_le_t new_rpa;
	uint8_t old_irk[BT_IRK_SIZE] = {1U};
	uint8_t new_irk[BT_IRK_SIZE] = {2U};
	int err;

	zassert_ok(bt_id_reset_irk(BT_ID_DEFAULT, old_irk), "Failed to set the old IRK");

	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, &adv);
	zassert_ok(err, "Failed to create the old advertising set");
	err = bt_le_ext_adv_get_info(adv, &info);
	zassert_ok(err, "Failed to get the old advertising set info");
	bt_addr_le_copy(&old_rpa, info.addr);
	zassert_true(bt_rpa_irk_matches(old_irk, &old_rpa.a),
		     "The cached RPA does not match the old IRK");
	zassert_ok(bt_le_ext_adv_delete(adv), "Failed to delete the old advertising set");

	zassert_ok(bt_id_reset_irk(BT_ID_DEFAULT, new_irk), "Failed to set the new IRK");
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, &adv);
	zassert_ok(err, "Failed to create the new advertising set");
	err = bt_le_ext_adv_get_info(adv, &info);
	zassert_ok(err, "Failed to get the new advertising set info");
	bt_addr_le_copy(&new_rpa, info.addr);
	zassert_true(bt_rpa_irk_matches(new_irk, &new_rpa.a),
		     "The refreshed cached RPA does not match the new IRK");
	zassert_false(bt_rpa_irk_matches(old_irk, &new_rpa.a),
		      "The refreshed cached RPA still matches the old IRK");
	zassert_false(bt_addr_le_eq(&old_rpa, &new_rpa),
		      "The cached RPA did not change after the IRK reset");
	zassert_ok(bt_le_ext_adv_delete(adv), "Failed to delete the new advertising set");
}
#endif /* defined(CONFIG_BT_RPA_SHARING) */

static ZTEST(bt_id_public_api, test_reset_irk_rejects_random_failure)
{
	uint8_t supplied_irk[BT_IRK_SIZE] = {1U};
	uint8_t failed_irk[BT_IRK_SIZE] = {0};
	int err;

	zassert_ok(bt_id_reset_irk(BT_ID_DEFAULT, supplied_irk), "Failed to set the supplied IRK");

	rand_failure = true;
	err = bt_id_reset_irk(BT_ID_DEFAULT, failed_irk);
	rand_failure = false;

	zassert_equal(err, -EIO, "Unexpected error from random IRK generation: %d", err);
	zassert_true(is_zero_irk(failed_irk), "Failed IRK output buffer was modified");
}

#if defined(CONFIG_BT_SMP)
static ZTEST(bt_id_public_api, test_reset_irk_rejects_bond)
{
	bt_addr_le_t remote = test_addr(0x42U);
	struct bt_keys *keys;

	keys = bt_keys_get_type(BT_KEYS_IRK, BT_ID_DEFAULT, &remote);
	zassert_not_null(keys, "Failed to create a test bond");
	zassert_equal(bt_id_reset_irk(BT_ID_DEFAULT, NULL), -ENOTEMPTY,
		      "IRK reset succeeded while a bond existed");
	zassert_ok(bt_unpair(BT_ID_DEFAULT, NULL), "Failed to remove the test bond");
	zassert_ok(bt_id_reset_irk(BT_ID_DEFAULT, NULL), "Failed to reset IRK after unpair");
}
#endif

static ZTEST(bt_id_public_api, test_reset_irk_rejects_empty_slot)
{
	bt_addr_le_t addr1 = test_addr(0x44U);
	bt_addr_le_t addr2 = test_addr(0x45U);
	int id1;
	int id2;

	id1 = bt_id_create(&addr1, NULL);
	id2 = bt_id_create(&addr2, NULL);
	zassert_equal(id1, 1, "Unexpected first secondary identity id");
	zassert_equal(id2, 2, "Unexpected second secondary identity id");

	zassert_ok(bt_id_delete((uint8_t)id1), "Failed to delete non-tail identity");
	zassert_equal(bt_id_reset_irk((uint8_t)id1, NULL), -EALREADY,
		      "IRK reset was allowed for an empty identity slot");
}

static ZTEST(bt_id_public_api, test_reset_irk_rejects_active_roles)
{
	int err;

	if (IS_ENABLED(CONFIG_BT_BROADCASTER)) {
		bt_addr_le_t secondary_addr = test_addr(0x46U);
		int secondary_id;

		secondary_id = bt_id_create(&secondary_addr, NULL);
		zassert_equal(secondary_id, 1, "Failed to create secondary identity");

		err = bt_le_adv_start(BT_LE_ADV_NCONN, NULL, 0U, NULL, 0U);
		zassert_ok(err, "Failed to start advertising");
		zassert_equal(bt_id_reset_irk(BT_ID_DEFAULT, NULL), -EBUSY,
			      "IRK reset was allowed while advertising");
		zassert_equal(bt_id_reset_irk((uint8_t)secondary_id, NULL), -EBUSY,
			      "Secondary IRK reset was allowed while advertising");
		zassert_ok(bt_le_adv_stop(), "Failed to stop advertising");
	}

	if (IS_ENABLED(CONFIG_BT_OBSERVER)) {
		err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
		zassert_ok(err, "Failed to start scanning");
		zassert_equal(bt_id_reset_irk(BT_ID_DEFAULT, NULL), -EBUSY,
			      "IRK reset was allowed while scanning");
		zassert_ok(bt_le_scan_stop(), "Failed to stop scanning");
	}

	if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
		bt_addr_le_t remote = test_addr(0x43U);
		struct bt_conn *conn = NULL;

		err = bt_conn_le_create(&remote, BT_CONN_LE_CREATE_CONN,
					BT_LE_CONN_PARAM_DEFAULT, &conn);
		zassert_ok(err, "Failed to start initiating");
		zassert_equal(bt_id_reset_irk(BT_ID_DEFAULT, NULL), -EBUSY,
			      "IRK reset was allowed while initiating");
		zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
			   "Failed to cancel initiating");
		bt_conn_unref(conn);
	}

	err = bt_id_reset_irk(BT_ID_DEFAULT, NULL);
	zassert_ok(err, "IRK reset failed after active roles stopped");
}
#endif /* defined(CONFIG_BT_PRIVACY) */
