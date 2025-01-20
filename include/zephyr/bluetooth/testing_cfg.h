/* Copyright (c) 2024 Nordic Semiconductor
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief Runtime configuration for Bluetooth testing purposes.
 *
 *  This exists to make it possible to compile a single binary
 *  that can be configured at boot time. This allows compiling
 *  tests with conflicting Kconfig configurations into one
 *  image, both reducing build time, and making code analysis in
 *  IDEs better by reducing if-defined-out code.
 */

#include <stdbool.h>

#include <zephyr/sys/util_macro.h>

#ifndef ZEPHYR_INCLUDE_BLUETOOTH_TESTING_CFG_H_
#define ZEPHYR_INCLUDE_BLUETOOTH_TESTING_CFG_H_

/** @brief Bluetooth boot-time configuration.
 */
struct bt_testing_cfg {
	bool l2cap_ecred;
	bool l2cap_seg_recv;
};

/* Select between hard-coded config, and runtime variable.
 */
#if defined(CONFIG_BT_TESTING_CFG)
/** @brief Boot-time Bluetooth configuration.
 *
 *  This variable is used to configure Bluetooth at boot time,
 *  before calling any Bluetooth APIs.
 *
 *  It starts out all zero, which means that whenever
 *  @kconfig{CONFIG_BT_TESTING_CFG} is enabled, features
 *  otherwise enabled by just their Kconfig may also have to be
 *  enabled here.
 */
extern struct bt_testing_cfg bt_testing_cfg_val;
#else
#define bt_testing_cfg_val _bt_testing_cfg_all_on
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wmissing-field-initializers"
/** Boilerplate configuration with all fields enabled.
 *
 *  When the testing configuration is not enabled, all fields
 *  must be enabled.
 *
 *  Do not use designated initializers `.foo = true,` here! That
 *  unfortunately suppresses the missing-field-initializers
 *  error.
 * TODO: use memset or other initialization method in .c file, for scalability.
 */
static const struct bt_testing_cfg _bt_testing_cfg_all_on = {
	true,
	true,
};
#pragma GCC diagnostic pop

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_BT_TESTING_CFG_H_ */
