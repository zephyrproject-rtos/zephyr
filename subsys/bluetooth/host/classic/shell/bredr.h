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

void br_role_changed(struct bt_conn *conn, uint8_t status);
void br_packet_type_changed(struct bt_conn *conn, uint8_t status, uint16_t packet_type);

#endif /* __BREDR_H */
