/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_KERNEL_BANNER_H
#define ZEPHYR_INCLUDE_KERNEL_BANNER_H

/**
 * @brief Boot banner API
 * @ingroup kernel_apis
 * @defgroup boot_banner_api Boot banner API
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Output boot banner
 *
 * Output boot banner at device boot-up. This should output the boot banner using e.g. `printk`
 * calls, if @kconfig{CONFIG_BOOT_BANNER_EXTERNAL_IMPLEMENTATION} is enabled then this function
 * should be implemented in application/module code.
 */
void boot_banner(void);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_KERNEL_BANNER_H */
