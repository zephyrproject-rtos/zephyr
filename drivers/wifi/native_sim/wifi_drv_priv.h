/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Private functions for native_sim wifi driver.
 */

#ifndef ZEPHYR_DRIVERS_WIFI_NATIVE_SIM_PRIV_H_
#define ZEPHYR_DRIVERS_WIFI_NATIVE_SIM_PRIV_H_

int eth_iface_create(const char *dev_name, const char *if_name, bool tun_only);
int eth_iface_remove(int fd);
int eth_wait_data(int fd);
ssize_t eth_read_data(int fd, void *buf, size_t buf_len);
ssize_t eth_write_data(int fd, void *buf, size_t buf_len);
int eth_promisc_mode(const char *if_name, bool enable);

#endif /* ZEPHYR_DRIVERS_WIFI_NATIVE_SIM_PRIV_H_ */
