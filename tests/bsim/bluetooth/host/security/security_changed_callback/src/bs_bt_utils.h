/**
 * Common functions and helpers for BSIM tests
 *
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

DECLARE_FLAG(flag_pairing_complete);
DECLARE_FLAG(flag_bonded);
DECLARE_FLAG(flag_not_bonded);

extern struct bt_conn *g_conn;
void wait_connected(void);
void wait_disconnected(void);
void clear_g_conn(void);
void bs_bt_utils_setup(void);
void scan_connect_to_first_result(void);
void set_security(bt_security_t sec);
void advertise_connectable(int id, bt_addr_le_t *directed_dst);
void set_bondable(bool enable);
