/* common.h - Common test code */

/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/types.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/att.h>

extern volatile int num_eatt_channels;
extern struct bt_conn *default_conn;

void central_setup_and_connect(void);
void peripheral_setup_and_connect(void);
void wait_for_disconnect(void);
void disconnect(void);
