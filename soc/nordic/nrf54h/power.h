/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Common power.h include for Nordic SoCs.
 */

#ifndef _ZEPHYR_SOC_ARM_NORDIC_NRF_POWER_H_
#define _ZEPHYR_SOC_ARM_NORDIC_NRF_POWER_H_

/**
 * @brief Perform a power off.
 *
 * This function performs a power off of the core.
 */
void nrf_poweroff(void);

#endif /* _ZEPHYR_SOC_ARM_NORDIC_NRF_POWER_H_ */
