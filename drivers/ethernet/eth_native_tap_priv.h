/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Private functions for native TAP ethernet driver.
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_NATIVE_TAP_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_NATIVE_TAP_PRIV_H_

int eth_iface_create(const char *dev_name, const char *if_name, bool tun_only);
int eth_iface_remove(int fd);
int eth_wait_data(int fd);
int eth_clock_gettime(uint64_t *second, uint32_t *nanosecond);
int eth_promisc_mode(const char *if_name, bool enable);

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_NATIVE_TAP_PRIV_H_ */
