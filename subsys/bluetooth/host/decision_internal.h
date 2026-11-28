/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_HOST_DECISION_INTERNAL_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_HOST_DECISION_INTERNAL_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/decision.h>
#include <zephyr/bluetooth/hci_types.h>

struct bt_le_ext_adv;

/* Decision data set with bt_le_ext_adv_set_decision_data(). It is kept so that the
 * Resolvable Tag can be regenerated when the advertising address changes.
 */
struct bt_le_decision_state {
	uint8_t key[BT_LE_DECISION_KEY_SIZE];
	uint8_t data[BT_LE_DECISION_DATA_MAX_LEN];
	uint8_t data_len;
	bool has_key;
};

/* Check whether the Controller supports Decision-Based Advertising Filtering. */
bool bt_le_decision_supported(void);

/* Regenerate the Resolvable Tag of an advertising set with a new random value.
 *
 * Called after a new random address has been written for the set, while the set is
 * disabled, so that the new address and the new tag first appear in the same advertising
 * event. Does nothing if the set has no key.
 */
int bt_le_decision_data_refresh(struct bt_le_ext_adv *adv);

/* Clear the decision data of an advertising set, including its key. */
void bt_le_decision_data_clear(struct bt_le_ext_adv *adv);

/* Check whether the decision data of an advertising set includes a Resolvable Tag. */
bool bt_le_decision_has_key(struct bt_le_ext_adv *adv);

/* Generate a Resolvable Tag: the hash of the random value followed by the random value. */
int bt_le_decision_tag_generate(const uint8_t key[BT_LE_DECISION_KEY_SIZE], const uint8_t prand[3],
				uint8_t tag[BT_HCI_LE_DECISION_RESOLVABLE_TAG_LEN]);

/* Encode decision instructions into at most max_tests HCI tests.
 *
 * Returns the number of tests written, or -EINVAL if the instructions are invalid or do not
 * fit.
 */
int bt_le_decision_instructions_encode(const struct bt_le_decision_group *groups, size_t num_groups,
				       struct bt_hci_le_decision_test *tests, size_t max_tests);

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_DECISION_INTERNAL_H_ */
