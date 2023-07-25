/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when bt_conn_lookup_state_le() is called
 *
 *  Expected behaviour:
 *   - bt_conn_lookup_state_le() to be called once with correct parameters
 */
void expect_single_call_bt_conn_lookup_state_le(uint8_t id, const bt_addr_le_t *peer,
						const bt_conn_state_t state);

/*
 *  Validate expected behaviour when bt_conn_lookup_state_le() isn't called
 *
 *  Expected behaviour:
 *   - bt_conn_lookup_state_le() isn't called at all
 */
void expect_not_called_bt_conn_lookup_state_le(void);

/*
 *  Validate expected behaviour when bt_conn_unref() is called
 *
 *  Expected behaviour:
 *   - bt_conn_unref() to be called once with correct parameters
 */
void expect_single_call_bt_conn_unref(struct bt_conn *conn);

/*
 *  Validate expected behaviour when bt_conn_unref() isn't called
 *
 *  Expected behaviour:
 *   - bt_conn_unref() isn't called at all
 */
void expect_not_called_bt_conn_unref(void);
