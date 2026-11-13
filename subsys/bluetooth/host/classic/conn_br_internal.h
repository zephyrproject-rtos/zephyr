/** @file
 *  @brief Internal APIs for Bluetooth classic connection handling.
 */

/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

int bt_hci_connect_br_cancel(struct bt_conn *conn);

void bt_br_acl_recv(struct bt_conn *conn, struct net_buf *buf, bool complete);

/* Write the Automatic Flush Timeout of the ACL link, in 0.625 ms units. 0 means
 * flushable PDUs are never flushed.
 */
int bt_conn_br_set_auto_flush_timeout(struct bt_conn *conn, uint16_t timeout);
