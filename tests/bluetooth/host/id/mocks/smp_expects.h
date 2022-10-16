/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when bt_smp_le_oob_generate_sc_data() is called
 *
 *  Expected behaviour:
 *   - bt_smp_le_oob_generate_sc_data() to be called once with correct parameters
 */
void expect_single_call_bt_smp_le_oob_generate_sc_data(struct bt_le_oob_sc_data *le_sc_oob);

/*
 *  Validate expected behaviour when bt_smp_le_oob_generate_sc_data() isn't called
 *
 *  Expected behaviour:
 *   - bt_smp_le_oob_generate_sc_data() isn't called at all
 */
void expect_not_called_bt_smp_le_oob_generate_sc_data(void);

/*
 *  Validate expected behaviour when bt_smp_le_oob_set_tk() is called
 *
 *  Expected behaviour:
 *   - bt_smp_le_oob_set_tk() to be called once with correct parameters
 */
void expect_single_call_bt_smp_le_oob_set_tk(struct bt_conn *conn, const uint8_t *tk);
