/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Private functions for native posix ethernet driver.
 */

#ifndef _ETH_NATIVE_POSIX_PRIV_H
#define _ETH_NATIVE_POSIX_PRIV_H

int eth_iface_create(const char *if_name, bool tun_only);
int eth_iface_remove(int fd);
int eth_setup_host(const char *if_name);
int eth_wait_data(int fd);
ssize_t eth_read_data(int fd, void *buf, size_t buf_len);
ssize_t eth_write_data(int fd, void *buf, size_t buf_len);

#if defined(CONFIG_NET_GPTP)
int eth_clock_gettime(struct net_ptp_time *time);
#endif

#endif /* _ETH_NATIVE_POSIX_PRIV_H */
