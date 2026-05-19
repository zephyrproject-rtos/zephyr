/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Realtek Ameba clock control interface.
 *
 * This header selects the SoC-specific clock Devicetree bindings for
 * Ameba family SoCs based on the active SoC series configuration.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_AMEBA_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_AMEBA_CLOCK_CONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Ameba SoC clock Devicetree bindings
 * @{
 */

/*
 * Select the appropriate Ameba clock Devicetree binding header
 * according to the SoC series in use.
 */
#if defined(CONFIG_SOC_SERIES_AMEBADPLUS)
#include <zephyr/dt-bindings/clock/amebadplus_clock.h>
#elif defined(CONFIG_SOC_SERIES_AMEBAD)
#include <zephyr/dt-bindings/clock/amebad_clock.h>
#elif defined(CONFIG_SOC_SERIES_AMEBAG2)
#include <zephyr/dt-bindings/clock/amebag2_clock.h>
#else
#error "Please select the correct Ameba SoC series."
#endif

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_AMEBA_CLOCK_CONTROL_H_ */
