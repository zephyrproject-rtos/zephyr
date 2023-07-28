/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_REBOOT_H_
#define ZEPHYR_INCLUDE_SYS_REBOOT_H_

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System reboot
 * @defgroup sys_reboot System reboot
 * @ingroup os_services
 * @{
 */

/**
 * @name Reboot types
 * @anchor REBOOT_TYPES
 * @{
 */

/** Warm reboot. */
#define SYS_REBOOT_WARM 0
/** Cold reboot. */
#define SYS_REBOOT_COLD 1

/** @} */

/**
 * @brief Reboot the system.
 *
 * @param type Reboot type (see @ref REBOOT_TYPES)
 */
FUNC_NORETURN void sys_reboot(int type);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_REBOOT_H_ */
