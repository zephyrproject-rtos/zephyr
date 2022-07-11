/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/direction.h>
#include <host/hci_core.h>

static void init_work(struct k_work *work);

struct bt_dev bt_dev = {
	.init          = Z_WORK_INITIALIZER(init_work),
#if defined(CONFIG_BT_DEVICE_APPEARANCE_DYNAMIC)
	.appearance = CONFIG_BT_DEVICE_APPEARANCE,
#endif
};

static void init_work(struct k_work *work)
{
}

#if defined(CONFIG_BT_HCI_ACL_FLOW_CONTROL)
void bt_hci_host_num_completed_packets(struct net_buf *buf)
{
}
#endif
