/*
 * Copyright (c) 2023 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief mDNS responder API
 *
 * This file contains the mDNS responder API. These APIs are used by the
 * to register mDNS records.
 */

#ifndef ZEPHYR_INCLUDE_NET_MDNS_RESPONDER_H_
#define ZEPHYR_INCLUDE_NET_MDNS_RESPONDER_H_

#include <stddef.h>
#include <errno.h>
#include <zephyr/net/dns_sd.h>
#include <zephyr/net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register continuous memory of @ref dns_sd_rec records.
 *
 * mDNS responder will start with iteration over mDNS records registered using
 * @ref DNS_SD_REGISTER_SERVICE (if any) and then go over external records.
 *
 * @param records A pointer to an array of mDNS records. It is stored internally
 *                without copying the content so it must be kept valid. It can
 *                be set to NULL, e.g. before freeing the memory block.
 * @param count The number of elements
 * @return 0 for OK; -EINVAL for invalid parameters.
 */
int mdns_responder_set_ext_records(const struct dns_sd_rec *records, size_t count);

#if defined(CONFIG_MDNS_RESPONDER_RUNTIME_IFACE_CONTROL) || defined(__DOXYGEN__)
/**
 * @brief Enable the mDNS responder on a specific network interface at runtime.
 *
 * The runtime setting overrides the build-time interface policy
 * (@kconfig{CONFIG_MDNS_RESPONDER_IFACE_POLICY_ALL} and friends) for the given
 * interface. The responder listener sockets and multicast group memberships
 * are reconfigured so that queries arriving on the interface are answered.
 *
 * @kconfig_dep{CONFIG_MDNS_RESPONDER_RUNTIME_IFACE_CONTROL}
 *
 * @param iface Network interface to enable the responder on.
 *
 * @return 0 for OK; -EINVAL if @p iface is NULL; -ERANGE if the interface
 *         index is out of range; -ENOTSUP if runtime control is not enabled.
 */
int mdns_responder_enable_iface(struct net_if *iface);

/**
 * @brief Disable the mDNS responder on a specific network interface at runtime.
 *
 * The runtime setting overrides the build-time interface policy for the given
 * interface. The listener socket bound to the interface is closed and its mDNS
 * multicast group memberships are left, so no further queries are answered on
 * that interface.
 *
 * @kconfig_dep{CONFIG_MDNS_RESPONDER_RUNTIME_IFACE_CONTROL}
 *
 * @param iface Network interface to disable the responder on.
 *
 * @return 0 for OK; -EINVAL if @p iface is NULL; -ERANGE if the interface
 *         index is out of range; -ENOTSUP if runtime control is not enabled.
 */
int mdns_responder_disable_iface(struct net_if *iface);

#else /* CONFIG_MDNS_RESPONDER_RUNTIME_IFACE_CONTROL */

static inline int mdns_responder_enable_iface(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return -ENOTSUP;
}

static inline int mdns_responder_disable_iface(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return -ENOTSUP;
}

#endif /* CONFIG_MDNS_RESPONDER_RUNTIME_IFACE_CONTROL || __DOXYGEN__ */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_MDNS_RESPONDER_H_ */
