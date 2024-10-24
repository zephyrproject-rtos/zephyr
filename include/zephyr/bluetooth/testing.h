/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/** @brief Internal testing interfaces for Bluetooth
 *  @file
 *  @internal
 *
 *  The interfaces in this file are internal and not stable.
 */

#include <zerphy/net_buf.h>

/** @brief Hook for `acl_in_pool.destroy`
 *
 *  Weak-function interface. The user can simply define this
 *  function, and it will automatically become the event
 *  listener.
 *
 *  @warning This may be not invoked if
 *  @kconfig{CONFIG_BT_TESTING} is not set.
 */
void bt_testing_trace_event_acl_pool_destroy(struct net_buf *buf);
