/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ATMEL_SAM_SOC_SUPC_H_
#define _ATMEL_SAM_SOC_SUPC_H_

#include <zephyr/types.h>

/**
 * @brief Enable the clock of specified peripheral module.
 */
void soc_supc_core_voltage_regulator_off(void);

/**
 * @brief Switch slow clock source to external crystal oscillator
 */
void soc_supc_slow_clock_select_crystal_osc(void);

/**
 * @brief Enable wakeup source
 */
void soc_supc_enable_wakeup_source(uint32_t wakeup_source_id);

#endif /* _ATMEL_SAM_SOC_SUPC_H_ */
