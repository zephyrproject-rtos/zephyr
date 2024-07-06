/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_POWEROFF_H_
#define ZEPHYR_INCLUDE_SYS_POWEROFF_H_

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup sys_poweroff System power off
 * @ingroup os_services
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/**
 * @brief System power off hook.
 *
 * This function needs to be implemented in platform code. It must only
 * perform an immediate power off of the system.
 */
FUNC_NORETURN void z_sys_poweroff(void);

/** @} */

/** @endcond */

/**
 * @brief Perform a system power off.
 *
 * This function will perform an immediate power off of the system. It is the
 * responsibility of the caller to ensure that the system is in a safe state to
 * be powered off. Any required wake up sources must be enabled before calling
 * this function.
 *
 * @kconfig{CONFIG_POWEROFF} needs to be enabled to use this API.
 */
FUNC_NORETURN void sys_poweroff(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_POWEROFF_H_ */
