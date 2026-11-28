/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/bluetooth/decision.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "common/rpa.h"
#include "decision_internal.h"
#include "hci_core.h"

#define PRAND_LEN 3

#define PASS_IF_MASK                                                                               \
	(BT_LE_DECISION_PASS_IF_MATCH | BT_LE_DECISION_PASS_IF_NO_MATCH |                          \
	 BT_LE_DECISION_PASS_IF_ABSENT)

/* Serializes changes to the decision data of all advertising sets, so that a tag refresh
 * cannot send older decision data after the application has set new data.
 */
static K_MUTEX_DEFINE(decision_lock);

bool bt_le_decision_supported(void)
{
	return BT_FEAT_LE_DECISION_ADV_FILTER(bt_dev.le.features);
}

/* Reports whether the Controller supports both the DBAF feature and the given command.
 *
 * Both are checked because a Controller that supports DBAF only has to support the commands for the
 * roles it supports, advertising or scanning.
 */
static bool cmd_supported(enum bt_dev_cmd cmd)
{
	return bt_le_decision_supported() && bt_dev_cmd_supported(cmd);
}

int bt_le_decision_tag_generate(const uint8_t key[BT_LE_DECISION_KEY_SIZE], const uint8_t prand[3],
				uint8_t tag[BT_HCI_LE_DECISION_RESOLVABLE_TAG_LEN])
{
	int err;

	err = bt_rpa_ah(key, prand, tag);
	if (err != 0) {
		return err;
	}

	memcpy(&tag[BT_HCI_LE_DECISION_RESOLVABLE_TAG_LEN - PRAND_LEN], prand, PRAND_LEN);

	return 0;
}

/* Checks whether a Test_Field is one of the Arbitrary Data fields, which all take a mask and a
 * target value as parameters.
 */
static bool is_data_field(uint8_t field)
{
	return IN_RANGE(field, BT_LE_DECISION_FIELD_DATA_EXACT(1),
			BT_LE_DECISION_FIELD_DATA_EXACT(BT_LE_DECISION_TEST_DATA_LEN)) ||
	       IN_RANGE(field, BT_LE_DECISION_FIELD_DATA_AT_LEAST(1),
			BT_LE_DECISION_FIELD_DATA_AT_LEAST(BT_LE_DECISION_TEST_DATA_LEN)) ||
	       IN_RANGE(field, BT_LE_DECISION_FIELD_DATA_AT_MOST(1),
			BT_LE_DECISION_FIELD_DATA_AT_MOST(BT_LE_DECISION_TEST_DATA_LEN));
}

/* Encodes the parameters of an AdvA test.
 *
 * Addresses are required only when the check compares them and check_used is true, see
 * encode_params(). Each address must be public or random, the only types the parameters can hold.
 */
static int encode_adva(const struct bt_le_decision_test *test, bool check_used,
		       struct bt_hci_le_decision_params_adva *params)
{
	size_t required = 0;

	params->check = test->adva.check;

	if (check_used && test->adva.check == BT_LE_DECISION_ADVA_CHECK_ADDR) {
		required = 1;
	} else if (check_used && test->adva.check == BT_LE_DECISION_ADVA_CHECK_ADDR_ANY) {
		required = 2;
	}

	for (size_t i = 0; i < ARRAY_SIZE(test->adva.addr); i++) {
		const bt_addr_le_t *addr = test->adva.addr[i];

		if (addr == NULL) {
			if (i < required) {
				return -EINVAL;
			}

			continue;
		}

		if (addr->type != BT_ADDR_LE_PUBLIC && addr->type != BT_ADDR_LE_RANDOM) {
			return -EINVAL;
		}

		memcpy(&params->addr[i], addr, sizeof(params->addr[i]));
	}

	return 0;
}

/* Encodes the Test_Parameters of a test.
 *
 * check_used is true when the Controller will compare the parameters. When the test can only pass
 * on an absent field, the parameters are ignored, so a missing key or address is not an error.
 */
static int encode_params(const struct bt_le_decision_test *test, bool check_used,
			 uint8_t params[BT_HCI_LE_DECISION_TEST_PARAMS_LEN])
{
	(void)memset(params, 0, BT_HCI_LE_DECISION_TEST_PARAMS_LEN);

	switch (test->field) {
	case BT_LE_DECISION_FIELD_TAG:
		if (test->key == NULL) {
			return check_used ? -EINVAL : 0;
		}

		memcpy(params, test->key, BT_LE_DECISION_KEY_SIZE);
		return 0;
	case BT_LE_DECISION_FIELD_ADV_MODE:
		params[0] = test->adv_modes;
		return 0;
	case BT_LE_DECISION_FIELD_RSSI: {
		struct bt_hci_le_decision_params_rssi *rssi = (void *)params;

		rssi->min = test->rssi.min;
		rssi->max = test->rssi.max;
		return 0;
	}
	case BT_LE_DECISION_FIELD_PATH_LOSS: {
		struct bt_hci_le_decision_params_path_loss *path_loss = (void *)params;

		path_loss->min = test->path_loss.min;
		path_loss->max = test->path_loss.max;
		return 0;
	}
	case BT_LE_DECISION_FIELD_ADVA:
		return encode_adva(test, check_used, (void *)params);
	default:
		break;
	}

	if (is_data_field(test->field)) {
		struct bt_hci_le_decision_params_data *data = (void *)params;

		memcpy(data->mask, test->data.mask, sizeof(data->mask));
		memcpy(data->target, test->data.target, sizeof(data->target));
	} else {
		/* Reserved and vendor-specific fields are left to the Controller to accept or
		 * reject.
		 */
		memcpy(params, test->raw, BT_HCI_LE_DECISION_TEST_PARAMS_LEN);
	}

	return 0;
}

/* Encodes a test into its HCI form. new_group marks the first test of a group.
 *
 * If a test has no pass condition, BT_LE_DECISION_PASS_IF_MATCH is set. Otherwise all adv
 * decision pdus would be rejected. To reject all adv decision pdus set num_groups to 0.
 */
static int encode_test(const struct bt_le_decision_test *test, bool new_group,
		       struct bt_hci_le_decision_test *out)
{
	uint8_t flags = test->flags;
	bool check_used;

	if ((flags & PASS_IF_MASK) == 0U) {
		flags |= BT_LE_DECISION_PASS_IF_MATCH;
	}

	check_used =
		(flags & (BT_LE_DECISION_PASS_IF_MATCH | BT_LE_DECISION_PASS_IF_NO_MATCH)) != 0U;

	/* The application flags map to Test_Flags bits 1 to 7, because the host sets bit 0 */
	out->flags = (uint8_t)(flags << 1);
	if (new_group) {
		out->flags |= BT_HCI_LE_DECISION_TEST_FLAG_NEW_GROUP;
	}

	out->field = test->field;

	return encode_params(test, check_used, out->params);
}

int bt_le_decision_instructions_encode(const struct bt_le_decision_group *groups, size_t num_groups,
				       struct bt_hci_le_decision_test *tests, size_t max_tests)
{
	size_t num_tests = 0;

	max_tests = MIN(max_tests, BT_LE_DECISION_TESTS_MAX);

	if (num_groups == 0U) {
		/* The command requires at least one test. A test with none of the pass
		 * conditions never passes, so every decision PDU is rejected.
		 */
		if (max_tests == 0U) {
			return -EINVAL;
		}

		(void)memset(&tests[0], 0, sizeof(tests[0]));
		tests[0].flags = BT_HCI_LE_DECISION_TEST_FLAG_NEW_GROUP;
		tests[0].field = BT_HCI_LE_DECISION_TEST_FIELD_ADV_MODE;

		return 1;
	}

	if (groups == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0; i < num_groups; i++) {
		const struct bt_le_decision_group *group = &groups[i];

		if (group->tests == NULL || group->num_tests == 0U) {
			return -EINVAL;
		}

		for (size_t j = 0; j < group->num_tests; j++) {
			int err;

			if (num_tests >= max_tests) {
				return -EINVAL;
			}

			err = encode_test(&group->tests[j], j == 0U, &tests[num_tests]);
			if (err != 0) {
				return err;
			}

			num_tests++;
		}
	}

	return (int)num_tests;
}

#if defined(CONFIG_BT_BROADCASTER)
/* Sends the LE Set Decision Data command for an advertising set. */
static int decision_data_send(struct bt_le_ext_adv *adv, uint8_t type_flags, const uint8_t *data,
			      uint8_t data_len)
{
	struct bt_hci_cp_le_set_decision_data *cp;
	struct net_buf *buf;

	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));
	cp->handle = adv->handle;
	cp->type_flags = type_flags;
	cp->len = data_len;

	if (data_len > 0U) {
		net_buf_add_mem(buf, data, data_len);
	}

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_DECISION_DATA, buf, NULL);
}

/* Builds the decision data from the stored state of an advertising set and sends it.
 *
 * Each call generates the tag from a new random value, so that the tag changes together with the
 * advertising address and cannot be used to track the device. Subfields are placed in the order of
 * their Decision_Type_Flags bits with the arbitrary data last, as the decision PDU format requires.
 */
static int decision_state_send(struct bt_le_ext_adv *adv, const struct bt_le_decision_state *state)
{
	uint8_t data[BT_LE_DECISION_DATA_MAX_LEN];
	uint8_t type_flags = 0U;
	uint8_t len = 0U;
	int err;

	if (state->has_key) {
		uint8_t prand[PRAND_LEN];

		err = bt_rand(prand, sizeof(prand));
		if (err != 0) {
			return err;
		}

		err = bt_le_decision_tag_generate(state->key, prand, &data[len]);
		if (err != 0) {
			return err;
		}

		type_flags |= BT_HCI_LE_DECISION_TYPE_RESOLVABLE_TAG;
		len += BT_HCI_LE_DECISION_RESOLVABLE_TAG_LEN;
	}

	memcpy(&data[len], state->data, state->data_len);
	len += state->data_len;

	return decision_data_send(adv, type_flags, data, len);
}

int bt_le_ext_adv_set_decision_data(struct bt_le_ext_adv *adv,
				    const struct bt_le_decision_data *param)
{
	struct bt_le_decision_state state = {0};
	size_t len;
	int err;

	if (adv == NULL || param == NULL || (param->data_len > 0U && param->data == NULL)) {
		return -EINVAL;
	}

	if (!cmd_supported(BT_DEV_CMD_LE_SET_DECISION_DATA)) {
		return -ENOTSUP;
	}

	len = param->data_len;
	if (param->key != NULL) {
		len += BT_HCI_LE_DECISION_RESOLVABLE_TAG_LEN;
	}

	if (len > BT_LE_DECISION_DATA_MAX_LEN) {
		return -EINVAL;
	}

	if (param->key != NULL) {
		/* With this option the Controller generates the advertising address and changes it
		 * without notifying the host. The tag would then stay the same across addresses,
		 * which lets observers link changed addresses to the same device.
		 */
		if ((adv->options & BT_LE_ADV_OPT_DIR_ADDR_RPA) != 0U) {
			return -ENOTSUP;
		}

		memcpy(state.key, param->key, sizeof(state.key));
		state.has_key = true;
	}

	if (param->data_len > 0U) {
		memcpy(state.data, param->data, param->data_len);
	}

	state.data_len = param->data_len;

	k_mutex_lock(&decision_lock, K_FOREVER);

	err = decision_state_send(adv, &state);
	if (err == 0) {
		adv->decision = state;
	}

	k_mutex_unlock(&decision_lock);

	return err;
}

int bt_le_decision_data_refresh(struct bt_le_ext_adv *adv)
{
	int err = 0;

	k_mutex_lock(&decision_lock, K_FOREVER);

	if (adv->decision.has_key) {
		err = decision_state_send(adv, &adv->decision);
	}

	k_mutex_unlock(&decision_lock);

	return err;
}

void bt_le_decision_data_clear(struct bt_le_ext_adv *adv)
{
	k_mutex_lock(&decision_lock, K_FOREVER);
	(void)memset(&adv->decision, 0, sizeof(adv->decision));
	k_mutex_unlock(&decision_lock);
}

bool bt_le_decision_has_key(struct bt_le_ext_adv *adv)
{
	bool has_key;

	k_mutex_lock(&decision_lock, K_FOREVER);
	has_key = adv->decision.has_key;
	k_mutex_unlock(&decision_lock);

	return has_key;
}
#endif /* CONFIG_BT_BROADCASTER */

#if defined(CONFIG_BT_OBSERVER)
int bt_le_scan_set_decision_instructions(const struct bt_le_decision_group *groups,
					 size_t num_groups)
{
	struct bt_hci_cp_le_set_decision_instructions *cp;
	struct net_buf *buf;
	int num_tests;

	if (!atomic_test_bit(bt_dev.flags, BT_DEV_READY)) {
		return -EAGAIN;
	}

	if (!cmd_supported(BT_DEV_CMD_LE_SET_DECISION_INSTRUCTIONS)) {
		return -ENOTSUP;
	}

	buf = bt_hci_cmd_alloc(K_FOREVER);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	cp = net_buf_add(buf, sizeof(*cp));

	num_tests = bt_le_decision_instructions_encode(
		groups, num_groups, (struct bt_hci_le_decision_test *)net_buf_tail(buf),
		net_buf_tailroom(buf) / sizeof(struct bt_hci_le_decision_test));
	if (num_tests < 0) {
		net_buf_unref(buf);
		return num_tests;
	}

	net_buf_add(buf, num_tests * sizeof(struct bt_hci_le_decision_test));
	cp->num_tests = (uint8_t)num_tests;

	return bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_DECISION_INSTRUCTIONS, buf, NULL);
}
#endif /* CONFIG_BT_OBSERVER */
