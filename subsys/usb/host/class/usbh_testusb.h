/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_USB_HOST_CLASS_USBH_TESTUSB_H
#define ZEPHYR_SUBSYS_USB_HOST_CLASS_USBH_TESTUSB_H

#include <stdint.h>

/* Parameters of a test run, modeled after the Linux usbtest usbtest_param. */
struct usbh_testusb_param {
	/* Test case to run, see testusb_exec() */
	uint32_t test;
	/* Length of a single transfer in bytes */
	uint32_t length;
	/* Number of transfers (test iterations) */
	uint32_t count;
	/* Per-transfer length variation, like the Linux usbtest vary. */
	uint32_t vary;
};

/*
 * Run a test case against a connected test device. addr is the address of the
 * device to test, or 0 to use any connected test device. The test numbers
 * mirror a subset of the Linux usbtest driver:
 *
 *    0: NOP                 1: bulk OUT            2: bulk IN
 *    3: bulk OUT (vary)     4: bulk IN (vary)
 *   13: set/clear halt     14: control loopback
 *   25: interrupt OUT      26: interrupt IN
 *
 * The function is the common entry point for in-tree test tools.
 * It returns 0 on success, -ENODEV if no matching device is connected, -ENOSYS
 * if the test case is not implemented or a negative value on failure.
 */
int usbh_testusb_exec(const uint8_t addr, const struct usbh_testusb_param *const param);

#endif /* ZEPHYR_SUBSYS_USB_HOST_CLASS_USBH_TESTUSB_H */
