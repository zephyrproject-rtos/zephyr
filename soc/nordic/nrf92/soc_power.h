/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Common power management for Nordic nRF92 SoCs.
 */

#ifndef ZEPHYR_SOC_NORDIC_NRF92_SOC_POWER_H_
#define ZEPHYR_SOC_NORDIC_NRF92_SOC_POWER_H_

/**
 * @brief Power domain early init.
 *
 */
void nrf_power_domain_init(void);

#if defined(CONFIG_PM) || defined(CONFIG_POWEROFF)
/**
 * @brief Perform a power off.
 *
 * This function performs a power off of the core.
 */
void nrf_poweroff(void);

/**
 * @brief Power up and enable instruction and data cache.
 */
void nrf_power_up_cache(void);
#endif /* defined(CONFIG_PM) || defined(CONFIG_POWEROFF) */

#endif /* ZEPHYR_SOC_NORDIC_NRF92_SOC_POWER_H_ */
