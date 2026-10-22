/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef BT_SETTINGS_COMMIT_H_
#define BT_SETTINGS_COMMIT_H_

/**
 * @brief Bluetooth Settings Commit Priorities
 *
 * Enum of commit priorities for Bluetooth settings handlers.
 * Lower values indicate higher priority.
 */
enum bt_settings_commit_priority {
	BT_SETTINGS_CPRIO_0,
	BT_SETTINGS_CPRIO_1,
	BT_SETTINGS_CPRIO_2,
};

#endif /* BT_SETTINGS_COMMIT_H_ */
