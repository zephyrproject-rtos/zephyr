/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when bt_id_del() is called
 *
 *  Expected behaviour:
 *   - bt_id_del() to be called once with correct parameters
 */
void expect_single_call_bt_id_del(struct bt_keys *keys);

/*
 *  Validate expected behaviour when bt_id_del() isn't called
 *
 *  Expected behaviour:
 *   - bt_id_del() isn't called at all
 */
void expect_not_called_bt_id_del(void);
