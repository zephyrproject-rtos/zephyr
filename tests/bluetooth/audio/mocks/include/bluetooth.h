/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_BLUETOOTH_H_
#define MOCKS_BLUETOOTH_H_
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>

struct bt_le_ext_adv {
	/* ID Address used for advertising */
	uint8_t id;

	/* Advertising handle */
	uint8_t handle;

	/** Extended advertising state */
	enum bt_le_ext_adv_state ext_adv_state;

	/** Periodic advertising state */
	enum bt_le_per_adv_state per_adv_state;
};

#endif /* MOCKS_BLUETOOTH_H_ */
