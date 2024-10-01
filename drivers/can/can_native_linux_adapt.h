/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Private functions for native posix canbus driver.
 */

#ifndef ZEPHYR_DRIVERS_CAN_NATIVE_LINUX_ADAPT_H_
#define ZEPHYR_DRIVERS_CAN_NATIVE_LINUX_ADAPT_H_

int linux_socketcan_iface_open(const char *if_name);

int linux_socketcan_iface_close(int fd);

int linux_socketcan_poll_data(int fd);

int linux_socketcan_read_data(int fd, void *buf, size_t buf_len, bool *msg_confirm);

int linux_socketcan_set_mode_fd(int fd, bool mode_fd);

#endif /* ZEPHYR_DRIVERS_CAN_NATIVE_LINUX_ADAPT_H_ */
