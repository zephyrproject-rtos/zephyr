/*
 * Copyright (c) 2016 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family Power Management Controller (PMC) module
 * HAL driver.
 */

#ifndef _ATMEL_SAM_SOC_PMC_H_
#define _ATMEL_SAM_SOC_PMC_H_

#include <zephyr/types.h>

/**
 * @brief Enable the clock of specified peripheral module.
 *
 * @param id   peripheral module id, as defined in data sheet.
 */
void soc_pmc_peripheral_enable(uint32_t id);

/**
 * @brief Disable the clock of specified peripheral module.
 *
 * @param id   peripheral module id, as defined in data sheet.
 */
void soc_pmc_peripheral_disable(uint32_t id);

/**
 * @brief Check if specified peripheral module is enabled.
 *
 * @param id   peripheral module id, as defined in data sheet.
 * @return     1 if peripheral is enabled, 0 otherwise
 */
uint32_t soc_pmc_peripheral_is_enabled(uint32_t id);

#endif /* _ATMEL_SAM_SOC_PMC_H_ */
