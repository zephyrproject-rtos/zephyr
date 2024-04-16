/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TEST_IFACES_H_
#define ZEPHYR_INCLUDE_TEST_IFACES_H_

#include <zephyr/net/net_if.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Public accessors for static iface structs */

extern struct net_if *ifa1;
extern struct net_if *ifa2;
extern struct net_if *ifb;
extern struct net_if *ifni;
extern struct net_if *ifnull;
extern struct net_if *ifnone;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_TEST_IFACES_H_ */
