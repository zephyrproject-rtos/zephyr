/*
 * Copyright (c) 2026 ABOV Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_ABOV32_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_ABOV32_H_

/**
 * @brief Re-apply the board's cctl devicetree clock configuration to the SCU.
 *
 * DEEP-SLEEP wake-up on this SoC leaves MCLK on its HSI fallback with the
 * HCLK divider back at its power-on-reset default (A31C15x user's manual
 * section 4.7.5) -- everything clock_control_abov32_init() set up at boot
 * (MCLK source/dividers, PCLK divider, and any configured HSE/LSE/PLL
 * enables) needs to be redone. Call this from pm_state_exit_post_ops()
 * before anything that assumes CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC is
 * actually true again (notably SysTick).
 */
void clock_control_abov32_reinit(void);

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_ABOV32_H_ */
