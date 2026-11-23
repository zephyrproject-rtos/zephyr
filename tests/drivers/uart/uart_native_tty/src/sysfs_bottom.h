/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_DRIVERS_UART_UART_NATIVE_TTY_SRC_SYSFS_BOTTOM_H_
#define ZEPHYR_TESTS_DRIVERS_UART_UART_NATIVE_TTY_SRC_SYSFS_BOTTOM_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create an empty fake sysfs tree in a new host temporary directory.
 *
 * Its tty class directory, the one the driver lists, is "class" under it.
 *
 * @param root	Receives the path of the tree.
 * @param size	Size of @p root.
 *
 * @return 0 if successful, -1 otherwise.
 */
int native_tty_test_sysfs_create(char *root, long size);

/**
 * @brief Add a tty to the fake sysfs tree.
 *
 * As in sysfs, the tty's device link points into the device tree, where the
 * USB device holding idVendor and idProduct is two levels above the tty.
 *
 * @param root		Path of the tree.
 * @param name		Name of the tty, such as "ttyUSB0".
 * @param vendor	USB vendor ID, or -1 for a tty with no USB device.
 * @param product	USB product ID.
 *
 * @return 0 if successful, -1 otherwise.
 */
int native_tty_test_sysfs_add_tty(const char *root, const char *name, int vendor, int product);

/** @brief Remove the fake sysfs tree. */
void native_tty_test_sysfs_remove(const char *root);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTS_DRIVERS_UART_UART_NATIVE_TTY_SRC_SYSFS_BOTTOM_H_ */
