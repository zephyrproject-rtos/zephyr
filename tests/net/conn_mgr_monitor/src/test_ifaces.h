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
extern struct net_if *if_simp_a;
extern struct net_if *if_simp_b;
extern struct net_if *if_conn_a;
extern struct net_if *if_conn_b;
extern struct net_if *if_dummy_eth;

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_TEST_IFACES_H_ */
