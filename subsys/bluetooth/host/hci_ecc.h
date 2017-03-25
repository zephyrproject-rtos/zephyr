/* hci_ecc.h - HCI ECC emulation */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void bt_hci_ecc_init(void);
int bt_hci_ecc_send(struct net_buf *buf);
