/**
 * @file
 * @brief Chosen SRAM Devicetree macro public API header file.
 */

/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEVICETREE_SRAM_H_
#define ZEPHYR_INCLUDE_DEVICETREE_SRAM_H_

#ifdef CONFIG_SRAM_DEPRECATED_KCONFIG_SET
#include <zephyr/sys/util.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup devicetree-chosen-sram Devicetree SRAM API
 * @ingroup devicetree
 * @{
 */

/**
 * @brief Get chosen SRAM node address
 *
 * @return Absolute address of chosen SRAM node, if it exists
 */
#ifdef CONFIG_SRAM_DEPRECATED_KCONFIG_SET
#define DT_CHOSEN_SRAM_ADDR CONFIG_SRAM_BASE_ADDRESS
#else
#define DT_CHOSEN_SRAM_ADDR DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#endif

/**
 * @brief Get chosen SRAM node size
 *
 * @return Size of chosen SRAM node, if it exists
 */
#ifdef CONFIG_SRAM_DEPRECATED_KCONFIG_SET
#define DT_CHOSEN_SRAM_SIZE (CONFIG_SRAM_SIZE * 1024)
#else
#define DT_CHOSEN_SRAM_SIZE DT_REG_SIZE(DT_CHOSEN(zephyr_sram))
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DEVICETREE_SRAM_H_ */
