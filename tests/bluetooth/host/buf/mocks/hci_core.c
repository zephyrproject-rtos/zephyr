/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/hci.h>
#include <host/hci_core.h>

struct bt_dev bt_dev = {
	.manufacturer = 0x1234,
};

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
void bt_hci_host_num_completed_packets(struct net_buf *buf)
{
}
#endif
