/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when bt_hci_le_rand() is called
 *
 *  Expected behaviour:
 *   - bt_hci_le_rand() to be called once with correct parameters
 */
void expect_call_count_bt_hci_le_rand(int call_count, uint8_t args_history[]);

/*
 *  Validate expected behaviour when bt_hci_le_rand() isn't called
 *
 *  Expected behaviour:
 *   - bt_hci_le_rand() isn't called at all
 */
void expect_not_called_bt_hci_le_rand(void);
