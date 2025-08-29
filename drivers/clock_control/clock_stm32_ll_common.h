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

#if defined(RCC_PLLI2SCFGR_PLLI2SM)
/* Some stm32F4 devices have a dedicated PLL I2S with M divider */
#define z_plli2s_m(v) LL_RCC_PLLI2SM_DIV_ ## v
#else
/* Some stm32F4 devices (typ. stm32F401) have a dedicated PLL I2S with PLL M divider */
#define z_plli2s_m(v) LL_RCC_PLLM_DIV_ ## v
#endif /* RCC_PLLI2SCFGR_PLLI2SM */
#define plli2sm(v) z_plli2s_m(v)

#define z_plli2s_q(v) LL_RCC_PLLI2SQ_DIV_ ## v
#define plli2sq(v) z_plli2s_q(v)

#define z_plli2s_r(v) LL_RCC_PLLI2SR_DIV_ ## v
#define plli2sr(v) z_plli2s_r(v)

#define z_pllsai_m(v) LL_RCC_PLLM_DIV_ ## v
#define pllsaim(v) z_pllsai_m(v)

#define z_pllsai_p(v) LL_RCC_PLLSAIP_DIV_ ## v
#define pllsaip(v) z_pllsai_p(v)

#define z_pllsai_q(v) LL_RCC_PLLSAIQ_DIV_ ## v
#define pllsaiq(v) z_pllsai_q(v)

#define z_pllsai_divq(v) LL_RCC_PLLSAIDIVQ_DIV_ ## v
#define pllsaidivq(v) z_pllsai_divq(v)

#define z_pllsai_r(v) LL_RCC_PLLSAIR_DIV_ ## v
#define pllsair(v) z_pllsai_r(v)

#define z_pllsai_divr(v) LL_RCC_PLLSAIDIVR_DIV_ ## v
#define pllsaidivr(v) z_pllsai_divr(v)

#define z_pllsai1_p(v) LL_RCC_PLLSAI1P_DIV_ ## v
#define pllsai1p(v) z_pllsai1_p(v)

#define z_pllsai1_q(v) LL_RCC_PLLSAI1Q_DIV_ ## v
#define pllsai1q(v) z_pllsai1_q(v)

#define z_pllsai1_r(v) LL_RCC_PLLSAI1R_DIV_ ## v
#define pllsai1r(v) z_pllsai1_r(v)

#if defined(RCC_PLLSAI2M_DIV_1_16_SUPPORT)
#define z_pllsai2_m(v) LL_RCC_PLLSAI2M_DIV_ ## v
#else
#define z_pllsai2_m(v) LL_RCC_PLLM_DIV_ ## v
#endif
#define pllsai2m(v) z_pllsai2_m(v)

#define z_pllsai2_p(v) LL_RCC_PLLSAI2P_DIV_ ## v
#define pllsai2p(v) z_pllsai2_p(v)

#define z_pllsai2_q(v) LL_RCC_PLLSAI2Q_DIV_ ## v
#define pllsai2q(v) z_pllsai2_q(v)

#define z_pllsai2_r(v) LL_RCC_PLLSAI2R_DIV_ ## v
#define pllsai2r(v) z_pllsai2_r(v)

#define z_pllsai2_divr(v) LL_RCC_PLLSAI2DIVR_DIV_ ## v
#define pllsai2divr(v) z_pllsai2_divr(v)

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
#if defined(STM32_PLLSAI_ENABLED)
uint32_t get_pllsaisrc_frequency(void);
void config_pllsai(void);
#endif
#if defined(STM32_PLLSAI1_ENABLED)
uint32_t get_pllsai1src_frequency(void);
void config_pllsai1(void);
#endif
#if defined(STM32_PLLSAI2_ENABLED)
uint32_t get_pllsai2src_frequency(void);
void config_pllsai2(void);
#endif
void config_enable_default_clocks(void);
void config_regulator_voltage(uint32_t hclk_freq);
int enabled_clock(uint32_t src_clk);

#if defined(STM32_CK48_ENABLED)
uint32_t get_ck48_frequency(void);
#endif

/* functions exported to the soc power.c */
int stm32_clock_control_init(const struct device *dev);
void stm32_clock_control_standby_exit(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_STM32_LL_CLOCK_H_ */
