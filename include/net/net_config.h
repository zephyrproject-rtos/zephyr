/** @file
 * @brief Routines for network subsystem initialization.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_CONFIG_H_
#define ZEPHYR_INCLUDE_NET_NET_CONFIG_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network configuration library
 * @defgroup net_config Network Configuration Library
 * @ingroup networking
 * @{
 */

/** Flags that tell what kind of functionality is needed by the client. */
#define NET_CONFIG_NEED_ROUTER 0x00000001
#define NET_CONFIG_NEED_IPV6   0x00000002
#define NET_CONFIG_NEED_IPV4   0x00000004

/**
 * @brief Initialize this network application.
 *
 * @param app_info String describing this application.
 * @param flags Flags related to services needed by the client.
 * @param timeout How long to wait the network setup before continuing
 * the startup.
 *
 * @return 0 if ok, <0 if error.
 */
int net_config_init(const char *app_info, u32_t flags, s32_t timeout);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_NET_CONFIG_H_ */
