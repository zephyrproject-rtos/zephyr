/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F4X_CLOCK_H_
#define _STM32F4X_CLOCK_H_

/**
 * @brief Driver for Reset & Clock Control of STM32F4X family processor.
 *
 * Based on reference manual:
 *   RM0368 Reference manual STM32F401xB/C and STM32F401xD/E
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 6. Reset and Clock control (RCC) for STM43F401xB/C and STM32F401xD/E
 */

/* 6.3.1 Clock control register (RCC_CR) */
enum {
	STM32F4X_RCC_CFG_PLL_SRC_HSI = 0x0,
	STM32F4X_RCC_CFG_PLL_SRC_HSE = 0x1,
};

enum {
	STM32F4X_RCC_CFG_SYSCLK_SRC_HSI = 0x0,
	STM32F4X_RCC_CFG_SYSCLK_SRC_HSE = 0x1,
	STM32F4X_RCC_CFG_SYSCLK_SRC_PLL = 0x2,
};

enum {
	STM32F4X_RCC_CFG_PLLP_DIV_2 = 0x0,
	STM32F4X_RCC_CFG_PLLP_DIV_4 = 0x1,
	STM32F4X_RCC_CFG_PLLP_DIV_6 = 0x2,
	STM32F4X_RCC_CFG_PLLP_DIV_8 = 0x3,
};

enum {
	STM32F4X_RCC_CFG_HCLK_DIV_0  = 0x0,
	STM32F4X_RCC_CFG_HCLK_DIV_2  = 0x4,
	STM32F4X_RCC_CFG_HCLK_DIV_4  = 0x5,
	STM32F4X_RCC_CFG_HCLK_DIV_8  = 0x6,
	STM32F4X_RCC_CFG_HCLK_DIV_16 = 0x7,
};

enum {
	STM32F4X_RCC_CFG_SYSCLK_DIV_0	 = 0x0,
	STM32F4X_RCC_CFG_SYSCLK_DIV_2	 = 0x8,
	STM32F4X_RCC_CFG_SYSCLK_DIV_4	 = 0x9,
	STM32F4X_RCC_CFG_SYSCLK_DIV_8	 = 0xa,
	STM32F4X_RCC_CFG_SYSCLK_DIV_16	 = 0xb,
	STM32F4X_RCC_CFG_SYSCLK_DIV_64	 = 0xc,
	STM32F4X_RCC_CFG_SYSCLK_DIV_128	 = 0xd,
	STM32F4X_RCC_CFG_SYSCLK_DIV_256	 = 0xe,
	STM32F4X_RCC_CFG_SYSCLK_DIV_512	 = 0xf,
};

/**
  * @brief Reset and Clock Control
  */

/* Helpers */
enum {
	STM32F4X_RCC_APB1ENR_PWREN = 0x10000000U,
};

union __rcc_cr {
	u32_t val;
	struct {
		u32_t hsion :1 __packed;
		u32_t hsirdy :1 __packed;
		u32_t rsvd__2 :1 __packed;
		u32_t hsitrim :5 __packed;
		u32_t hsical :8 __packed;
		u32_t hseon :1 __packed;
		u32_t hserdy :1 __packed;
		u32_t hsebyp :1 __packed;
		u32_t csson :1 __packed;
		u32_t rsvd__20_23 :4 __packed;
		u32_t pllon :1 __packed;
		u32_t pllrdy :1 __packed;
		u32_t plli2son :1 __packed;
		u32_t plli2srdy :1 __packed;
		u32_t pllsaion :1 __packed;
		u32_t pllsairdy :1 __packed;
		u32_t rsvd__30_31 :2 __packed;
	} bit;
};

union __rcc_pllcfgr {
	u32_t val;
	struct {
		u32_t pllm :6 __packed;
		u32_t plln :9 __packed;
		u32_t rsvd__15 :1 __packed;
		u32_t pllp :2 __packed;
		u32_t rsvd__18_21 :4 __packed;
		u32_t pllsrc :1 __packed;
		u32_t rsvd__23 :1 __packed;
		u32_t pllq :4 __packed;
		u32_t rsvd__28_31 :4 __packed;
	} bit;
};

union __rcc_cfgr {
	u32_t val;
	struct {
		u32_t sw :2 __packed;
		u32_t sws :2 __packed;
		u32_t hpre :4 __packed;
		u32_t rsvd__8_9 :2 __packed;
		u32_t ppre1 :3 __packed;
		u32_t ppre2 :3 __packed;
		u32_t rtcpre :5 __packed;
		u32_t mco1 :2 __packed;
		u32_t i2sscr :1 __packed;
		u32_t mco1pre :3 __packed;
		u32_t mco2pre :3 __packed;
		u32_t mco2 :2 __packed;
	} bit;
};

struct stm32f4x_rcc {
	union __rcc_cr cr;
	union __rcc_pllcfgr pllcfgr;
	union __rcc_cfgr cfgr;
	u32_t cir;
	u32_t ahb1rstr;
	u32_t ahb2rstr;
	u32_t ahb3rstr;
	u32_t rsvd0;
	u32_t apb1rstr;
	u32_t apb2rstr;
	u32_t rsvd1[2];
	u32_t ahb1enr;
	u32_t ahb2enr;
	u32_t ahb3enr;
	u32_t rsvd2;
	u32_t apb1enr;
	u32_t apb2enr;
	u32_t rsvd3[2];
	u32_t ahb1lpenr;
	u32_t ahb2lpenr;
	u32_t ahb3lpenr;
	u32_t rsvd4;
	u32_t apb1lpenr;
	u32_t apb2lpenr;
	u32_t rsvd5[2];
	u32_t bdcr;
	u32_t csr;
	u32_t rsvd6[2];
	u32_t sscgr;
	u32_t plli2scfgr;
	u32_t rsvd7;
	u32_t dckcfgr;
};

#endif /* _STM32F4X_CLOCK_H_ */
