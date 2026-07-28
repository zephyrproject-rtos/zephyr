/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control helpers for Microchip XEC devices.
 * @ingroup clock_control_mchp_xec
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MCHP_XEC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MCHP_XEC_H_

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/mchp_xec_pcr.h>

/**
 * @defgroup clock_control_mchp_xec Microchip XEC
 * @ingroup clock_control_mchp
 * @{
 */

/** @cond INTERNAL_HIDDEN */

/*
 * Set/clear Microchip XEC peripheral sleep enable.
 * SoC layer contains the chip specific sleep index and positions
 */
int z_mchp_xec_pcr_periph_sleep(uint8_t slp_idx, uint8_t slp_pos,
				uint8_t slp_en);

int z_mchp_xec_pcr_periph_reset(uint8_t slp_idx, uint8_t slp_pos);

/** @endcond */

#if defined(CONFIG_PM)
/**
 * @brief Enable system sleep for the XEC clock controller.
 *
 * @param is_deep True to request deep sleep, false for light sleep.
 * @kconfig_dep{CONFIG_PM}
 */
void mchp_xec_clk_ctrl_sys_sleep_enable(bool is_deep);

/**
 * @brief Disable system sleep for the XEC clock controller.
 *
 * @kconfig_dep{CONFIG_PM}
 */
void mchp_xec_clk_ctrl_sys_sleep_disable(void);
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MCHP_XEC_H_ */
