/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Dev It Wise
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_BSIM_BLUETOOTH_HOST_SECURITY_LEVEL_ENFORCED_COMMON_H_
#define TESTS_BSIM_BLUETOOTH_HOST_SECURITY_LEVEL_ENFORCED_COMMON_H_

#include <zephyr/bluetooth/conn.h>

/* The connection, the flags and the reported outcome are private to common.c.
 * Both roles reach them only through the calls below, so there is one place
 * that decides what an elevation attempt is supposed to look like.
 */

void test_setup(void);
void wait_connected(void);
void wait_disconnected(void);
void clear_g_conn(void);
void scan_connect_to_first_result(void);
void advertise_connectable(void);
void disconnect_and_wait(void);
void forget_bonds(void);
void forget_security_result(void);
void expect_refused_elevation(bt_security_t level);
void expect_granted_elevation(bt_security_t level);
void expect_observed_elevation(bt_security_t level);

#endif /* TESTS_BSIM_BLUETOOTH_HOST_SECURITY_LEVEL_ENFORCED_COMMON_H_ */
