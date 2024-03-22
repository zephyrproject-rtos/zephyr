/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int bt_ssp_start_security(struct bt_conn *conn);

int bt_ssp_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey);
int bt_ssp_auth_passkey_confirm(struct bt_conn *conn);
int bt_ssp_auth_cancel(struct bt_conn *conn);
int bt_ssp_auth_pairing_confirm(struct bt_conn *conn);
