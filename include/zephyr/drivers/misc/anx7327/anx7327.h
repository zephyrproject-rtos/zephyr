/*
 * Copyright (c) 2025, Applied Materials
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the ANX7533 driver
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_MISC_ANX7327_H_
#define INCLUDE_ZEPHYR_DRIVERS_MISC_ANX7327_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get info for anx7533
 *
 *
 * @param dev the anx7533 device
 * @return EOK on success
 * @retval -EINVAL If @p dev is invalid
 */
__syscall int anx7327_get_info(const struct device *dev);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif /* INCLUDE_ZEPHYR_DRIVERS_MISC_ANX7533_H_ */
