/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Private functions for native posix ethernet driver.
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_NATIVE_POSIX_PRIV_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_NATIVE_POSIX_PRIV_H_

#define ETH_NATIVE_POSIX_DRV_NAME CONFIG_ETH_NATIVE_POSIX_DRV_NAME
#define ETH_NATIVE_POSIX_DEV_NAME CONFIG_ETH_NATIVE_POSIX_DEV_NAME

#if defined(CONFIG_ETH_NATIVE_POSIX_STARTUP_AUTOMATIC)
#define ETH_NATIVE_POSIX_SETUP_SCRIPT CONFIG_ETH_NATIVE_POSIX_SETUP_SCRIPT
#define ETH_NATIVE_POSIX_STARTUP_SCRIPT CONFIG_ETH_NATIVE_POSIX_STARTUP_SCRIPT
#define ETH_NATIVE_POSIX_STARTUP_SCRIPT_USER \
	CONFIG_ETH_NATIVE_POSIX_STARTUP_SCRIPT_USER
#else
#define ETH_NATIVE_POSIX_SETUP_SCRIPT ""
#define ETH_NATIVE_POSIX_STARTUP_SCRIPT ""
#define ETH_NATIVE_POSIX_STARTUP_SCRIPT_USER ""
#endif

int eth_iface_create(const char *if_name, bool tun_only);
int eth_iface_remove(int fd);
int eth_setup_host(const char *if_name);
int eth_start_script(const char *if_name);
int eth_wait_data(int fd);
ssize_t eth_read_data(int fd, void *buf, size_t buf_len);
ssize_t eth_write_data(int fd, void *buf, size_t buf_len);
int eth_if_up(const char *if_name);
int eth_if_down(const char *if_name);

#if defined(CONFIG_NET_GPTP)
int eth_clock_gettime(struct net_ptp_time *time);
#endif

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
int eth_promisc_mode(const char *if_name, bool enable);
#else
static inline int eth_promisc_mode(const char *if_name, bool enable)
{
	ARG_UNUSED(if_name);
	ARG_UNUSED(enable);

	return -ENOTSUP;
}
#endif

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_NATIVE_POSIX_PRIV_H_ */
