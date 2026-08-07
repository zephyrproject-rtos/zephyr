/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_INCL_NSI_CPU_CTRL_H
#define NSI_COMMON_SRC_INCL_NSI_CPU_CTRL_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Define if a CPU should automatically start at boot or not
 *
 * @param[in] cpu_n:      Which CPU
 * @param[in] auto_start: If true, it will autostart on its own,
 *                        if 0, it won't
 */
void nsi_cpu_set_auto_start(int cpu_n, bool auto_start);

bool nsi_cpu_get_auto_start(int cpu_n);

/**
 * @brief Boot CPU <cpu_n>
 *
 * Note: This API may only be called if that CPU was not stared before
 *
 * @param[in] cpu_n:      Which CPU
 */
void nsi_cpu_boot(int cpu_n);

/*
 * Internal native simulator runner API.
 * Boot all CPUs which are configured to boot automatically
 */
void nsi_cpu_auto_boot(void);

#ifdef __cplusplus
}
#endif

#endif /* NSI_COMMON_SRC_INCL_NSI_CPU_CTRL_H */
