/*
 * Copyright (c) 2021 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MCHP_XEC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MCHP_XEC_H_

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/mchp_xec_pcr.h>

/*
 * Set/clear Microchip XEC peripheral sleep enable.
 * SoC layer contains the chip specific sleep index and positions
 */
int z_mchp_xec_pcr_periph_sleep(uint8_t slp_idx, uint8_t slp_pos,
				uint8_t slp_en);

#if defined(CONFIG_PM)
void mchp_xec_clk_ctrl_sys_sleep_enable(bool is_deep);
void mchp_xec_clk_ctrl_sys_sleep_disable(void);
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_LPC11U6X_CLOCK_CONTROL_H_ */
