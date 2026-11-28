/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/decision.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>

#include "decision_internal.h"
#include "mocks/crypto.h"
#include "mocks/hci_core.h"
#include "mocks/kernel.h"
#include "mocks/net_buf.h"
#include "mocks/rpa.h"

DEFINE_FFF_GLOBALS;

/* Sample data for the random address hash function ah, from the Bluetooth Core Specification,
 * Vol 3, Part H, Appendix D.7, least significant octet first:
 *
 *   IRK   = ec0234a3 57c8ad05 341010a6 0a397d9b
 *   prand = 708194
 *   ah    = 0dfbaa
 */
static const uint8_t spec_key[BT_LE_DECISION_KEY_SIZE] = {
	0x9b, 0x7d, 0x39, 0x0a, 0xa6, 0x10, 0x10, 0x34,
	0x05, 0xad, 0xc8, 0x57, 0xa3, 0x34, 0x02, 0xec,
};
static const uint8_t spec_prand[3] = {0x94, 0x81, 0x70};
static const uint8_t spec_hash[3] = {0xaa, 0xfb, 0x0d};

/* Resolvable Tag for the sample data: the hash followed by prand (Vol 6, Part B, Figure 2.12) */
static const uint8_t spec_tag[BT_HCI_LE_DECISION_RESOLVABLE_TAG_LEN] = {
	0xaa, 0xfb, 0x0d, 0x94, 0x81, 0x70,
};

/* Hash returned by the fake ah for any input other than the sample data */
static const uint8_t other_hash[3] = {0x11, 0x22, 0x33};

static const uint8_t data[] = {'B', 'T'};

static uint8_t prand[3];
static uint8_t ah_key[BT_LE_DECISION_KEY_SIZE];
static uint8_t ah_prand[3];

static uint8_t cmd_data[UINT8_MAX];
static struct net_buf cmd_buf;
static uint8_t sent[UINT8_MAX];
static size_t sent_len;
static int send_err;

static struct bt_le_ext_adv adv;

static int rand_fake(void *buf, size_t len)
{
	zassert_equal(len, sizeof(prand));
	(void)memcpy(buf, prand, len);

	return 0;
}

static int ah_fake(const uint8_t *irk, const uint8_t *r, uint8_t *out)
{
	(void)memcpy(ah_key, irk, sizeof(ah_key));
	(void)memcpy(ah_prand, r, sizeof(ah_prand));

	if (memcmp(irk, spec_key, sizeof(spec_key)) == 0 &&
	    memcmp(r, spec_prand, sizeof(spec_prand)) == 0) {
		(void)memcpy(out, spec_hash, sizeof(spec_hash));
	} else {
		(void)memcpy(out, other_hash, sizeof(other_hash));
	}

	return 0;
}

static struct net_buf *cmd_alloc_fake(k_timeout_t timeout)
{
	ARG_UNUSED(timeout);

	net_buf_simple_init_with_data(&cmd_buf.b, cmd_data, sizeof(cmd_data));
	cmd_buf.len = 0U;

	return &cmd_buf;
}

static int cmd_send_sync_fake(uint16_t opcode, struct net_buf *buf, struct net_buf **rsp)
{
	ARG_UNUSED(rsp);

	zassert_equal(opcode, BT_HCI_OP_LE_SET_DECISION_DATA);
	(void)memcpy(sent, buf->data, buf->len);
	sent_len = buf->len;

	return send_err;
}

static void set_decision_data_before(void *fixture)
{
	ARG_UNUSED(fixture);

	CRYPTO_FFF_FAKES_LIST(RESET_FAKE);
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
	KERNEL_FFF_FAKES_LIST(RESET_FAKE);
	NET_BUF_FFF_FAKES_LIST(RESET_FAKE);
	RPA_FFF_FAKES_LIST(RESET_FAKE);

	bt_rand_fake.custom_fake = rand_fake;
	bt_rpa_ah_fake.custom_fake = ah_fake;
	bt_hci_cmd_alloc_fake.custom_fake = cmd_alloc_fake;
	bt_hci_cmd_send_sync_fake.custom_fake = cmd_send_sync_fake;

	(void)memset(&bt_dev, 0, sizeof(bt_dev));
	bt_dev.le.features[BT_LE_FEAT_BIT_DECISION_ADV_FILTER / 8U] |=
		BIT(BT_LE_FEAT_BIT_DECISION_ADV_FILTER % 8U);
	atomic_set_bit(bt_dev.supported_commands, BT_DEV_CMD_LE_SET_DECISION_DATA);

	(void)memset(&adv, 0, sizeof(adv));
	adv.handle = 3U;

	(void)memcpy(prand, spec_prand, sizeof(prand));
	(void)memset(sent, 0, sizeof(sent));
	sent_len = 0U;
	send_err = 0;
}

ZTEST_SUITE(bt_le_ext_adv_set_decision_data, NULL, NULL, set_decision_data_before, NULL, NULL);

static void expect_sent(uint8_t type_flags, const uint8_t *decision_data, size_t len)
{
	zassert_equal(sent_len, sizeof(struct bt_hci_cp_le_set_decision_data) + len,
		      "Command length %zu", sent_len);
	zassert_equal(sent[0], adv.handle, "Advertising_Handle 0x%02x", sent[0]);
	zassert_equal(sent[1], type_flags, "Decision_Type_Flags 0x%02x", sent[1]);
	zassert_equal(sent[2], len, "Decision_Data_Length %u", sent[2]);

	if (len > 0U) {
		zassert_mem_equal(&sent[3], decision_data, len, "Unexpected Decision_Data");
	}
}

/*
 *  Test tag generation with the sample data of the random address hash function
 *
 *  Expected behaviour:
 *   - The key and the random value reach ah unchanged
 *   - The tag is the hash followed by the random value
 *   - An error from ah is returned
 */
ZTEST(bt_le_ext_adv_set_decision_data, test_tag_generate)
{
	uint8_t tag[BT_HCI_LE_DECISION_RESOLVABLE_TAG_LEN];

	zassert_ok(bt_le_decision_tag_generate(spec_key, spec_prand, tag));
	zassert_mem_equal(ah_key, spec_key, sizeof(spec_key));
	zassert_mem_equal(ah_prand, spec_prand, sizeof(spec_prand));
	zassert_mem_equal(tag, spec_tag, sizeof(spec_tag));

	bt_rpa_ah_fake.custom_fake = NULL;
	bt_rpa_ah_fake.return_val = -EIO;
	zassert_equal(bt_le_decision_tag_generate(spec_key, spec_prand, tag), -EIO);
}

/*
 *  Test decision data with a key and arbitrary data
 *
 *  Expected behaviour:
 *   - The decision data is the tag followed by the arbitrary data
 *   - Decision_Type_Flags indicates the tag
 */
ZTEST(bt_le_ext_adv_set_decision_data, test_key_and_data)
{
	const struct bt_le_decision_data param = {
		.key = spec_key,
		.data = data,
		.data_len = sizeof(data),
	};
	const uint8_t expected[] = {0xaa, 0xfb, 0x0d, 0x94, 0x81, 0x70, 'B', 'T'};

	zassert_ok(bt_le_ext_adv_set_decision_data(&adv, &param));
	zassert_equal(bt_rand_fake.call_count, 1);
	expect_sent(BT_HCI_LE_DECISION_TYPE_RESOLVABLE_TAG, expected, sizeof(expected));
	zassert_true(bt_le_decision_has_key(&adv));
}

/*
 *  Test decision data without a key
 *
 *  Expected behaviour:
 *   - The decision data is the arbitrary data only, and no random value is generated
 *   - Without arbitrary data, the decision data is empty
 */
ZTEST(bt_le_ext_adv_set_decision_data, test_without_key)
{
	const struct bt_le_decision_data param = {.data = data, .data_len = sizeof(data)};
	const struct bt_le_decision_data empty = {0};

	zassert_ok(bt_le_ext_adv_set_decision_data(&adv, &param));
	expect_sent(0U, data, sizeof(data));

	zassert_ok(bt_le_ext_adv_set_decision_data(&adv, &empty));
	expect_sent(0U, NULL, 0U);

	zassert_equal(bt_rand_fake.call_count, 0);
	zassert_false(bt_le_decision_has_key(&adv));
}

/*
 *  Test the length limit of the decision data
 *
 *  Expected behaviour:
 *   - BT_LE_DECISION_DATA_MAX_LEN octets in total are accepted
 *   - One octet more gives -EINVAL and no command is sent
 */
ZTEST(bt_le_ext_adv_set_decision_data, test_too_long)
{
	const uint8_t max_data[BT_LE_DECISION_DATA_MAX_LEN + 1] = {0};
	const size_t tag_len = BT_HCI_LE_DECISION_RESOLVABLE_TAG_LEN;
	struct bt_le_decision_data param = {.data = max_data};

	param.data_len = BT_LE_DECISION_DATA_MAX_LEN;
	zassert_ok(bt_le_ext_adv_set_decision_data(&adv, &param));

	param.key = spec_key;
	param.data_len = BT_LE_DECISION_DATA_MAX_LEN - tag_len;
	zassert_ok(bt_le_ext_adv_set_decision_data(&adv, &param));

	bt_hci_cmd_alloc_fake.call_count = 0;

	param.data_len = BT_LE_DECISION_DATA_MAX_LEN - tag_len + 1U;
	zassert_equal(bt_le_ext_adv_set_decision_data(&adv, &param), -EINVAL);

	param.key = NULL;
	param.data_len = BT_LE_DECISION_DATA_MAX_LEN + 1U;
	zassert_equal(bt_le_ext_adv_set_decision_data(&adv, &param), -EINVAL);

	zassert_equal(bt_hci_cmd_alloc_fake.call_count, 0);
}

/*
 *  Test missing arguments
 *
 *  Expected behaviour:
 *   - No advertising set, no parameters, or a data length without data gives -EINVAL
 */
ZTEST(bt_le_ext_adv_set_decision_data, test_invalid_arguments)
{
	const struct bt_le_decision_data param = {.data = data, .data_len = sizeof(data)};
	const struct bt_le_decision_data no_data = {.data_len = sizeof(data)};

	zassert_equal(bt_le_ext_adv_set_decision_data(NULL, &param), -EINVAL);
	zassert_equal(bt_le_ext_adv_set_decision_data(&adv, NULL), -EINVAL);
	zassert_equal(bt_le_ext_adv_set_decision_data(&adv, &no_data), -EINVAL);
	zassert_equal(bt_hci_cmd_alloc_fake.call_count, 0);
}

/*
 *  Test a Controller without support
 *
 *  Expected behaviour:
 *   - Without the feature or without the command, -ENOTSUP is returned
 */
ZTEST(bt_le_ext_adv_set_decision_data, test_not_supported)
{
	const struct bt_le_decision_data param = {.data = data, .data_len = sizeof(data)};

	atomic_clear_bit(bt_dev.supported_commands, BT_DEV_CMD_LE_SET_DECISION_DATA);
	zassert_equal(bt_le_ext_adv_set_decision_data(&adv, &param), -ENOTSUP);

	atomic_set_bit(bt_dev.supported_commands, BT_DEV_CMD_LE_SET_DECISION_DATA);
	(void)memset(bt_dev.le.features, 0, sizeof(bt_dev.le.features));
	zassert_equal(bt_le_ext_adv_set_decision_data(&adv, &param), -ENOTSUP);

	zassert_equal(bt_hci_cmd_alloc_fake.call_count, 0);
}

/*
 *  Test an advertising set whose address the Controller generates
 *
 *  Expected behaviour:
 *   - A key gives -ENOTSUP, because the tag could not change with the address
 *   - Arbitrary data alone is accepted
 */
ZTEST(bt_le_ext_adv_set_decision_data, test_controller_generated_address)
{
	const struct bt_le_decision_data with_key = {.key = spec_key};
	const struct bt_le_decision_data without_key = {.data = data, .data_len = sizeof(data)};

	adv.options = BT_LE_ADV_OPT_DIR_ADDR_RPA;

	zassert_equal(bt_le_ext_adv_set_decision_data(&adv, &with_key), -ENOTSUP);
	zassert_ok(bt_le_ext_adv_set_decision_data(&adv, &without_key));
}

/*
 *  Test a refresh after the advertising address has changed
 *
 *  Expected behaviour:
 *   - The tag is generated again from the stored key and a new random value
 *   - The arbitrary data is unchanged
 */
ZTEST(bt_le_ext_adv_set_decision_data, test_refresh)
{
	const struct bt_le_decision_data param = {
		.key = spec_key,
		.data = data,
		.data_len = sizeof(data),
	};
	const uint8_t new_prand[3] = {0x01, 0x02, 0x03};
	const uint8_t expected[] = {0x11, 0x22, 0x33, 0x01, 0x02, 0x03, 'B', 'T'};

	zassert_ok(bt_le_ext_adv_set_decision_data(&adv, &param));

	(void)memcpy(prand, new_prand, sizeof(prand));
	zassert_ok(bt_le_decision_data_refresh(&adv));

	zassert_equal(bt_rand_fake.call_count, 2);
	zassert_mem_equal(ah_key, spec_key, sizeof(spec_key));
	zassert_mem_equal(ah_prand, new_prand, sizeof(new_prand));
	expect_sent(BT_HCI_LE_DECISION_TYPE_RESOLVABLE_TAG, expected, sizeof(expected));
}

/*
 *  Test a refresh of decision data without a key
 *
 *  Expected behaviour:
 *   - No command is sent, because the decision data does not depend on the address
 */
ZTEST(bt_le_ext_adv_set_decision_data, test_refresh_without_key)
{
	const struct bt_le_decision_data param = {.data = data, .data_len = sizeof(data)};

	zassert_ok(bt_le_ext_adv_set_decision_data(&adv, &param));
	zassert_ok(bt_le_decision_data_refresh(&adv));
	zassert_equal(bt_hci_cmd_send_sync_fake.call_count, 1);
}

/*
 *  Test decision data that the Controller rejects
 *
 *  Expected behaviour:
 *   - The error is returned and the previous decision data stays in use
 */
ZTEST(bt_le_ext_adv_set_decision_data, test_send_error)
{
	const struct bt_le_decision_data with_key = {.key = spec_key};
	const struct bt_le_decision_data without_key = {.data = data, .data_len = sizeof(data)};

	zassert_ok(bt_le_ext_adv_set_decision_data(&adv, &with_key));

	send_err = -EIO;
	zassert_equal(bt_le_ext_adv_set_decision_data(&adv, &without_key), -EIO);
	zassert_true(bt_le_decision_has_key(&adv));

	send_err = 0;
	zassert_ok(bt_le_decision_data_refresh(&adv));
	expect_sent(BT_HCI_LE_DECISION_TYPE_RESOLVABLE_TAG, spec_tag, sizeof(spec_tag));
}

/*
 *  Test clearing the decision data of a deleted advertising set
 *
 *  Expected behaviour:
 *   - The key is removed, so a later refresh sends nothing
 */
ZTEST(bt_le_ext_adv_set_decision_data, test_clear)
{
	const struct bt_le_decision_data param = {.key = spec_key};

	zassert_ok(bt_le_ext_adv_set_decision_data(&adv, &param));
	bt_le_decision_data_clear(&adv);

	zassert_false(bt_le_decision_has_key(&adv));
	zassert_ok(bt_le_decision_data_refresh(&adv));
	zassert_equal(bt_hci_cmd_send_sync_fake.call_count, 1);
}
