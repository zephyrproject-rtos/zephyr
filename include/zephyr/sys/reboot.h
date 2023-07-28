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

/** Reboot modes. */
enum sys_reboot_mode {
	/** Warm reboot. */
	SYS_REBOOT_WARM,
	/** Cold reboot. */
	SYS_REBOOT_COLD,
};

/**
 * @brief Reboot the system.
 *
 * @param mode Reboot mode.
 */
FUNC_NORETURN void sys_reboot(enum sys_reboot_mode mode);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_REBOOT_H_ */
