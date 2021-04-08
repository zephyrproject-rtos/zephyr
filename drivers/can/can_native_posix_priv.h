/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Private functions for native posix canbus driver.
 */

#ifndef ZEPHYR_DRIVERS_CAN_NATIVE_POSIX_PRIV_H_
#define ZEPHYR_DRIVERS_CAN_NATIVE_POSIX_PRIV_H_

int canbus_np_iface_open(const char *if_name);
int canbus_np_iface_close(int fd);
int canbus_np_wait_data(int fd);
ssize_t canbus_np_read_data(int fd, void *buf, size_t buf_len);
ssize_t canbus_np_write_data(int fd, void *buf, size_t buf_len);
int canbus_np_setsockopt(int fd, int level, int optname,
			 const void *optval, socklen_t optlen);
int canbus_np_getsockopt(int fd, int level, int optname,
			 void *optval, socklen_t *optlen);

#endif /* ZEPHYR_DRIVERS_CAN_NATIVE_POSIX_PRIV_H_ */
