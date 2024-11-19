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

/**
 * @brief System power off ready hook.
 *
 * This function needs to be implemented in the platform code. It must put the core
 * in the state which may lead to the power off of the system but it can return
 * if powering off is interrupted.
 */
void z_sys_poweroff_ready(void);

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

/**
 * @brief Put core in "ready for power off" state.
 *
 * Function can be used in multi-core platforms to put a core into a state which
 * may result in power off (if all other cores goes to the power off state) but
 * it may also return if system is waken up before power off is reached.
 *
 * @retval 0 if the core was waken up from power off ready state.
 * @retval negative if @ref z_sys_poweroff_prepare or @ref z_sys_poweroff_unprepare
 * returned error.
 */
int sys_poweroff_ready(void);

/**
 * @brief Prepare the power off.
 *
 * Platform specific code which can be executed before interrupts are locked and
 * power off function is called. Called when @kconfig{CONFIG_POWEROFF_PREPARE} is
 * enabled.
 *
 * @retval 0 if preparation was successful.
 * @retval negative if error occurred. Note that @ref sys_poweroff cannot return so error
 * can only interrupt powering off when @ref sys_poweroff_ready is used.
 */
int z_sys_poweroff_prepare(void);

/**
 * @brief Revert preparation to the power off.
 *
 * Platform specific code which can be executed if the core returns from @ref z_sys_poweroff_ready.
 * Called by @ref sys_poweroff_ready when @kconfig{CONFIG_POWEROFF_PREPARE} is enabled.
 *
 * @retval 0 if reverting was successful.
 * @retval negative if error occurred.
 */
int z_sys_poweroff_unprepare(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_POWEROFF_H_ */
