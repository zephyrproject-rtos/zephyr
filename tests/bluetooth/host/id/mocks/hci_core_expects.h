/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when bt_unpair() is called
 *
 *  Expected behaviour:
 *   - bt_unpair() to be called once with correct parameters
 */
void expect_single_call_bt_unpair(uint8_t id, const bt_addr_le_t *addr);

/*
 *  Validate expected behaviour when bt_unpair() isn't called
 *
 *  Expected behaviour:
 *   - bt_unpair() isn't called at all
 */
void expect_not_called_bt_unpair(void);

/*
 *  Validate expected behaviour when bt_hci_cmd_alloc() is called
 *
 *  Expected behaviour:
 *   - bt_hci_cmd_alloc() to be called once with correct parameters
 */
void expect_single_call_bt_hci_cmd_alloc(void);

/*
 *  Validate expected behaviour when bt_hci_cmd_alloc() isn't called
 *
 *  Expected behaviour:
 *   - bt_hci_cmd_alloc() isn't called at all
 */
void expect_not_called_bt_hci_cmd_alloc(void);

/*
 *  Validate expected behaviour when bt_hci_cmd_send_sync() is called
 *
 *  Expected behaviour:
 *   - bt_hci_cmd_send_sync() to be called once with correct parameters
 */
void expect_single_call_bt_hci_cmd_send_sync(uint16_t opcode);

/*
 *  Validate expected behaviour when bt_hci_cmd_send_sync() isn't called
 *
 *  Expected behaviour:
 *   - bt_hci_cmd_send_sync() isn't called at all
 */
void expect_not_called_bt_hci_cmd_send_sync(void);
