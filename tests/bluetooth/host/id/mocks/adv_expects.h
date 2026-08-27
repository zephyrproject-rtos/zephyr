/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when bt_le_adv_lookup_legacy() is called
 *
 *  Expected behaviour:
 *   - bt_le_adv_lookup_legacy() to be called once with correct parameters
 */
void expect_single_call_bt_le_adv_lookup_legacy(void);

/*
 *  Validate expected behaviour when bt_le_adv_lookup_legacy() isn't called
 *
 *  Expected behaviour:
 *   - bt_le_adv_lookup_legacy() isn't called at all
 */
void expect_not_called_bt_le_adv_lookup_legacy(void);

/*
 *  Validate expected behaviour when bt_le_ext_adv_foreach() is called
 *
 *  Expected behaviour:
 *   - bt_le_ext_adv_foreach() to be called once with correct parameters
 */
void expect_single_call_bt_le_ext_adv_foreach(void);

/*
 *  Validate expected behaviour when bt_le_ext_adv_foreach() isn't called
 *
 *  Expected behaviour:
 *   - bt_le_ext_adv_foreach() isn't called at all
 */
void expect_not_called_bt_le_ext_adv_foreach(void);
