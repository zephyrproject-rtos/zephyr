/* test_hid_host.c - Bluetooth Classic HID Host board-to-board test application */

/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 * The test suite drives the boards entirely through the in-tree Bluetooth
 * shell modules, so the application itself only has to bring up the shell.
 * The HID Host role is provided by
 * subsys/bluetooth/host/classic/shell/hid_host.c and the HID Device role of
 * the second board by subsys/bluetooth/host/classic/shell/hid_device.c, both
 * enabled through CONFIG_BT_HID_HOST and CONFIG_BT_HID_DEVICE.
 */
int main(void)
{
	return 0;
}
