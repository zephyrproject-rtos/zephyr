/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the Device Multiplexer driver
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_MISC_DEVMUX_H_
#define INCLUDE_ZEPHYR_DRIVERS_MISC_DEVMUX_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Devmux Driver APIs
 * @defgroup demux_interface Devmux Driver APIs
 * @ingroup misc_interfaces
 *
 * @details
 * Devmux operates as a device multiplexer, forwarding the characteristics of
 * the selected device.
 *
 * ```
 *            +----------+                            +----------+
 *            |  devmux  |                            |  devmux  |
 *            |          |                            |          |
 * dev0       |          |                 dev0       |          |
 * +---------->   \      |                 +---------->          |
 *            |    \     |                            |          |
 * dev1       |     \    |       dev0      dev1       |          |       dev2
 * +---------->      O   +---------->      +---------->      O   +---------->
 *            |          |                            |     /    |
 * dev2       |          |                 dev2       |    /     |
 * +---------->          |                 +---------->   /      |
 *            |          |                            |          |
 *            |          |                            |          |
 *            |          |                            |          |
 *            +-----^----+                            +-----^----+
 *                  |                                       |
 *   select == 0    |                       select == 2     |
 *   +--------------+                       +---------------+
 * ```
 * @{
 */

/**
 * @brief Get the current selection of a devmux device.
 *
 * Return the index of the currently selected device.
 *
 * @param dev the devmux device
 * @return The index (>= 0) of the currently active multiplexed device on success
 * @retval -EINVAL If @p dev is invalid
 */
__syscall ssize_t devmux_select_get(const struct device *dev);

/**
 * @brief Set the selection of a devmux device.
 *
 * Select the device at @p index.
 *
 * @param[in] dev the devmux device
 * @param index the index representing the desired selection
 * @retval 0 On success
 * @retval -EINVAL If @p dev is invalid
 * @retval -ENODEV If the multiplexed device at @p index is not ready
 */
__syscall int devmux_select_set(struct device *dev, size_t index);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <zephyr/syscalls/devmux.h>

#endif /* INCLUDE_ZEPHYR_DRIVERS_MISC_DEVMUX_H_ */
