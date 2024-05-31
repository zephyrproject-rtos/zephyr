/*
 * Copyright (c) 2022, Carlo Caione <ccaione@baylibre.com>
 */

/**
 * @file
 *
 * @brief public S2RAM APIs.
 * @defgroup pm_s2ram S2RAM APIs
 * @{
 */

#ifndef ZEPHYR_INCLUDE_ARCH_COMMON_PM_S2RAM_H_
#define ZEPHYR_INCLUDE_ARCH_COMMON_PM_S2RAM_H_

#ifdef _ASMLANGUAGE
GTEXT(arch_pm_s2ram_suspend);
#else

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief System off function
 *
 * This function is passed as argument and called by @ref arch_pm_s2ram_suspend
 * to power the system off after the CPU context has been saved.
 *
 * This function never returns if the system is powered off. If the operation
 * cannot be performed a proper value is returned and the code must take care
 * of restoring the system in a fully operational state before returning.
 *
 * @retval none		The system is powered off.
 * @retval -EBUSY	The system is busy and cannot be powered off at this time.
 * @retval -errno	Other error codes.
 */
typedef int (*pm_s2ram_system_off_fn_t)(void);

/**
 * @brief Save CPU context on suspend
 *
 * This function is used on suspend-to-RAM (S2RAM) to save the CPU context in
 * (retained) RAM before powering the system off using the provided function.
 * This function is usually called from the PM subsystem / hooks.
 *
 * The CPU context is usually the minimum set of CPU registers which content
 * must be restored on resume to let the platform resume its execution from the
 * point it left at the time of suspension.
 *
 * @param system_off	Function to power off the system.
 *
 * @retval 0		The CPU context was successfully saved and restored.
 * @retval -EBUSY	The system is busy and cannot be suspended at this time.
 * @retval -errno	Negative errno code in case of failure.
 */
int arch_pm_s2ram_suspend(pm_s2ram_system_off_fn_t system_off);

/**
 * @brief Mark that core is entering suspend-to-RAM state.
 *
 * Function is called when system state is stored to RAM, just before going to system
 * off.
 *
 * Default implementation is setting a magic word in RAM. CONFIG_PM_S2RAM_CUSTOM_MARKING
 * allows custom implementation.
 */
void pm_s2ram_mark_set(void);

/**
 * @brief Check suspend-to-RAM marking and clear its state.
 *
 * Function is used to determine if resuming after suspend-to-RAM shall be performed
 * or standard boot code shall be executed.
 *
 * Default implementation is checking a magic word in RAM. CONFIG_PM_S2RAM_CUSTOM_MARKING
 * allows custom implementation.
 *
 * @retval true if marking is found which indicates resuming after suspend-to-RAM.
 * @retval false if marking is not found which indicates standard boot.
 */
bool pm_s2ram_mark_check_and_clear(void);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_COMMON_PM_S2RAM_H_ */
