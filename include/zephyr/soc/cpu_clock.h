/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Generic CPU clock frequency publication API.
 *
 * Provides a standard internal mechanism for SoC code to publish
 * the current CPU core clock frequency after clock initialization
 * or after a runtime frequency change.
 *
 * @note This is an internal SoC/platform API. Application code must not
 *       call these functions directly.
 */

#ifndef ZEPHYR_INCLUDE_SOC_CPU_CLOCK_H_
#define ZEPHYR_INCLUDE_SOC_CPU_CLOCK_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Publish the current CPU core clock frequency.
 *
 * Called by SoC code after clock initialization has completed (or after
 * a runtime frequency change) to update @c SystemCoreClock.
 *
 * On platforms where the system timer clock is derived from the CPU clock
 * and @kconfig{CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE} is
 * enabled, this also updates the kernel runtime system timer frequency.
 *
 * @param hz The applied CPU core clock frequency in Hz. A value of 0 is
 *           ignored.
 */
void z_soc_cpu_clock_hz_publish(uint32_t hz);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SOC_CPU_CLOCK_H_ */
