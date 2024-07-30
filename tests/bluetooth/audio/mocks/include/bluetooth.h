/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_BLUETOOTH_H_
#define MOCKS_BLUETOOTH_H_

struct bt_le_ext_adv {
	/* ID Address used for advertising */
	uint8_t id;

	/* Advertising handle */
	uint8_t handle;
};

#endif /* MOCKS_BLUETOOTH_H_ */
