/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void bt_smp_connected(struct bt_conn *conn);
void bt_smp_disconnected(struct bt_conn *conn);
int bt_smp_init(void);

int bt_smp_auth_cancel(struct bt_conn *conn);
int bt_smp_auth_pairing_confirm(struct bt_conn *conn);
int bt_smp_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey);

int bt_smp_send_security_req(struct bt_conn *conn);
int bt_smp_send_pairing_req(struct bt_conn *conn);
