/* Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_ADV_H_
#define ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_ADV_H_

#include <zephyr/bluetooth/bluetooth.h>

int bt_testlib_adv_conn(struct bt_conn **conn, int id, uint32_t adv_options,
			const struct bt_data *ad, size_t ad_len,
			const struct bt_data *sd, size_t sd_len);

#endif /* ZEPHYR_TESTS_BLUETOOTH_COMMON_TESTLIB_INCLUDE_TESTLIB_ADV_H_ */
