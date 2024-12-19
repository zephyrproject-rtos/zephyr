/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_GATT_SETTINGS_CLEAR_SRC_BT_SETTINGS_HOOK_H_
#define ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_GATT_SETTINGS_CLEAR_SRC_BT_SETTINGS_HOOK_H_

void start_settings_record(void);
void stop_settings_record(void);
void settings_list_cleanup(void);
int get_settings_list_size(void);

#endif /* ZEPHYR_TESTS_BSIM_BLUETOOTH_HOST_GATT_SETTINGS_CLEAR_SRC_BT_SETTINGS_HOOK_H_ */
