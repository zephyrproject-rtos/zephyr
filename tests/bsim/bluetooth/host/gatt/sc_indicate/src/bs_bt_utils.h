/**
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bstests.h"
#include "bs_tracing.h"

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

DECLARE_FLAG(flag_pairing_complete);
DECLARE_FLAG(flag_bonded);
DECLARE_FLAG(flag_not_bonded);

void scan_connect_to_first_result(void);
struct bt_conn *get_g_conn(void);
void clear_g_conn(void);
void disconnect(void);
void wait_connected(void);
void wait_disconnected(void);
void create_adv(struct bt_le_ext_adv **adv);
void start_adv(struct bt_le_ext_adv *adv);
void stop_adv(struct bt_le_ext_adv *adv);
void set_security(bt_security_t sec);
void pairing_complete(struct bt_conn *conn, bool bonded);
void pairing_failed(struct bt_conn *conn, enum bt_security_err err);
