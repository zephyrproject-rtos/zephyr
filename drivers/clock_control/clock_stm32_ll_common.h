/*
 *
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_STM32_LL_CLOCK_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_STM32_LL_CLOCK_H_

#include <stdint.h>

#include <zephyr/device.h>

#include <stm32_ll_utils.h>

/* Macros to fill up multiplication and division factors values */
#define z_pllm(v) LL_RCC_PLLM_DIV_ ## v
#define pllm(v) z_pllm(v)

#define z_pllp(v) LL_RCC_PLLP_DIV_ ## v
#define pllp(v) z_pllp(v)

#define z_pllq(v) LL_RCC_PLLQ_DIV_ ## v
#define pllq(v) z_pllq(v)

#define z_pllr(v) LL_RCC_PLLR_DIV_ ## v
#define pllr(v) z_pllr(v)

#define z_plli2s_m(v) LL_RCC_PLLI2SM_DIV_ ## v
#define plli2sm(v) z_plli2s_m(v)

#define z_plli2s_r(v) LL_RCC_PLLI2SR_DIV_ ## v
#define plli2sr(v) z_plli2s_r(v)

#ifdef __cplusplus
extern "C" {
#endif

#if defined(STM32_PLL_ENABLED)
void config_pll_sysclock(void);
uint32_t get_pllout_frequency(void);
uint32_t get_pllsrc_frequency(void);
#endif
#if defined(STM32_PLL2_ENABLED)
void config_pll2(void);
#endif
#if defined(STM32_PLLI2S_ENABLED)
void config_plli2s(void);
#endif
void config_enable_default_clocks(void);
void config_regulator_voltage(uint32_t hclk_freq);
int enabled_clock(uint32_t src_clk);

/* functions exported to the soc power.c */
int stm32_clock_control_init(const struct device *dev);
void stm32_clock_control_standby_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_STM32_LL_CLOCK_H_ */
