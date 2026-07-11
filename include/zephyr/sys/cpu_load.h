/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_CPU_LOAD_H_
#define ZEPHYR_INCLUDE_SYS_CPU_LOAD_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup cpu_load CPU load monitor
 *  @ingroup os_services
 *  @brief Module for monitoring CPU load.
 *
 *  The CPU load is the fraction of time the CPU spends outside of the idle
 *  state. Two measurement backends are available and selected through Kconfig:
 *
 *  - @kconfig{CONFIG_CPU_LOAD_BACKEND_RUNTIME_STATS} derives the load from the
 *    scheduler per-CPU runtime statistics. It is portable across architectures
 *    and supports multiple CPUs.
 *  - @kconfig{CONFIG_CPU_LOAD_BACKEND_IDLE_HOOK} measures the load using the
 *    architecture idle hooks. It has lower overhead and can use an optional
 *    hardware counter for higher precision, but is limited to a single CPU.
 *
 *  Regardless of the backend, the load is reported in per mille (0...1000).
 *  @{
 */

/** @brief Convert a per mille CPU load value to whole percent. */
#define CPU_LOAD_PERMILLE_TO_PERCENT(permille) ((permille) / 10)

/** @brief Get CPU load for a specific CPU.
 *
 * @param cpu_id CPU index for which to get the load. For the idle-hook backend
 *		 only the single supported CPU is measured and this argument is
 *		 ignored.
 * @param reset  Reset the measurement window after reading.
 *
 * @retval >=0 CPU load in per mille (0...1000).
 * @retval <0  Negative errno code on failure.
 */
int cpu_load_get_cpu(unsigned int cpu_id, bool reset);

/** @brief Get CPU load for the current CPU.
 *
 * @param reset Reset the measurement window after reading.
 *
 * @retval >=0 CPU load in per mille (0...1000).
 * @retval <0  Negative errno code on failure.
 */
int cpu_load_get(bool reset);

/** @brief Hook called on entering CPU idle.
 *
 * Only used by the idle-hook backend; called from the architecture idle path.
 */
void cpu_load_on_enter_idle(void);

/** @brief Hook called on exiting CPU idle.
 *
 * Only used by the idle-hook backend; called from the architecture idle path.
 */
void cpu_load_on_exit_idle(void);

/** @brief Control periodic CPU load report.
 *
 * Report logging is by default enabled.
 *
 * @param enable true to enable report logging and false to disable.
 */
void cpu_load_log_control(bool enable);

/* Optional callback for cpu_load_cb_reg
 *
 * This will be called from the k_timer expiry_fn used for periodic logging.
 * CONFIG_CPU_LOAD_LOG_PERIODICALLY must be configured to a positive value.
 * Time spent in this callback must be kept to a minimum.
 */
typedef void (*cpu_load_cb_t)(uint8_t percent);

/** @brief Optional registration of callback when load is greater or equal to the threshold.
 *
 * @param cb Pointer to the callback function. NULL will cancel the callback.
 * @param threshold_percent Threshold [0...100]. CPU load equal or greater that this
 * will trigger the callback.
 *
 * @retval 0 - Callback registered/cancelled.
 * @retval -EINVAL if the threshold is invalid.
 */
int cpu_load_cb_reg(cpu_load_cb_t cb, uint8_t threshold_percent);

/** @brief Get CPU load as a percentage for a specific CPU.
 *
 * @deprecated Use cpu_load_get_cpu() instead. Note that cpu_load_get_cpu()
 *	       returns the load in per mille rather than percent.
 *
 * @param cpu_id CPU index for which to get the load.
 *
 * @retval >=0 CPU load in percent (0...100).
 * @retval <0  Negative errno code on failure.
 */
__deprecated static inline int cpu_load_metric_get(int cpu_id)
{
	int load = cpu_load_get_cpu((unsigned int)cpu_id, true);

	return (load < 0) ? load : CPU_LOAD_PERMILLE_TO_PERCENT(load);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_CPU_LOAD_H_ */
