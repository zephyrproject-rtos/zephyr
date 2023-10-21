/*
 * Copyright (c) 2023 Bjarki Arge Andreasen
 * Copyright (c) 2023 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ATMEL_SAM_SOC_SUPC_H_
#define _ATMEL_SAM_SOC_SUPC_H_

#include <zephyr/types.h>

enum soc_supc_pin_wkup_type {
	SOC_SUPC_PIN_WKUP_TYPE_LOW,
	SOC_SUPC_PIN_WKUP_TYPE_HIGH,
};

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

/**
 * @brief Enable wakeup pin source
 */
void soc_supc_enable_wakeup_pin_source(uint32_t wkup_src,
				       enum soc_supc_pin_wkup_type wkup_type);

/**
 * @brief Disable wakeup pin source
 */
void soc_supc_disable_wakeup_pin_source(uint32_t wkup_src);

#endif /* _ATMEL_SAM_SOC_SUPC_H_ */
