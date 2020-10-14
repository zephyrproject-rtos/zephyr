/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest_assert.h>
#include <bluetooth/buf.h>
#include <bluetooth/bluetooth.h>
#include <drivers/bluetooth/hci_driver.h>
#include "hci_mock.h"


/* @brief Function handles HCI commands received from
 * HOST and provides response events.
 *
 * Function uses mock of HCI command handler to process
 * HCI messages received from HOST. If handler provides
 * reposnse event, that event is send back to HOST.
 *
 * @param[in] buf	Pointer to command buffer
 */
static int cmd_handle(struct net_buf *buf)
{
	int err = 0;
	struct net_buf *evt;

	evt = hci_cmd_handle(buf);
	if (evt) {
		err = bt_recv_prio(evt);
		if (err) {
			printk("HCI Test driver - failed bt_recv_prio,"
			       " error %d", err);
		}
		return err;
	}
	return 0;
}

/* @brief HCI Test driver open function */
static int hci_driver_open(void)
{
	return 0;
}

/* @brief HCI Test driver mock of send function.
 *
 * @param[in] buf	Pointer to command to be handled
 *
 * @return 0 on success, negative error number on failure
 */
static int hci_driver_send(struct net_buf *buf)
{
	int err;

	err = cmd_handle(buf);
	net_buf_unref(buf);

	return err;
}

/* @brief Instance of a HCI test virtual driver. */
static const struct bt_hci_driver hci_test_drv = {
	.name = "hci_test_drv",
	.bus = BT_HCI_DRIVER_BUS_VIRTUAL,
	.open = hci_driver_open,
	.send = hci_driver_send,
	.quirks = BT_QUIRK_NO_RESET
};

int hci_init_test_driver(void)
{
	/* Register the test HCI driver */
	return bt_hci_driver_register(&hci_test_drv);
}
