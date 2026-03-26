/* Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/** @brief Internal testing interfaces for Bluetooth
 *  @file
 *  @internal
 *
 *  The interfaces in this file are internal and not stable.
 */

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_TESTING_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_TESTING_H_

#include <zephyr/net_buf.h>

/** @brief Hook for `acl_in_pool.destroy`
 *
 *  Weak-function interface. The user can simply define this
 *  function, and it will automatically become the event
 *  listener.
 *
 *  @kconfig_dep{CONFIG_BT_TESTING}
 */
void bt_testing_trace_event_acl_pool_destroy(struct net_buf *buf);

/** @brief Trace when extended advertising report reassembly timed out
 *
 *  Weak-function interface called when the host reassembly timer expires for
 *  a fragmented extended advertising report chain.
 *
 *  @kconfig_dep{CONFIG_BT_TESTING}
 */
void bt_testing_trace_ext_adv_reassembly_timeout(void);

/** @brief Trace when a fragmented extended advertising report is delivered
 *
 *  Weak-function interface called when a fragmented extended advertising report
 *  chain has been fully reassembled and delivered to the application.
 *
 *  @kconfig_dep{CONFIG_BT_TESTING}
 */
void bt_testing_trace_ext_adv_reassembly_complete(void);

#endif /* ZEPHYR_INCLUDE_BLUETOOTH_TESTING_H_ */
