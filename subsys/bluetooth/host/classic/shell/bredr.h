/** @file
 *  @brief Bluetooth shell functions
 *
 *  This is not to be included by the application.
 */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BREDR_H
#define __BREDR_H
#include <stddef.h>
#include <stdint.h>

void role_changed(struct bt_conn *conn, uint8_t status);

#endif /* __BREDR_H */
