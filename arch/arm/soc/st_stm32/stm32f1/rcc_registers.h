/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _STM32F10X_CLOCK_H_
#define _STM32F10X_CLOCK_H_

/**
 * @brief Driver for Reset & Clock Control of STM32F10x family processor.
 *
 * Based on reference manual:
 *   STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 7: Low-, medium-, high- and XL-density reset and
 *            clock control
 */

/* 8.3.1 Clock control register (RCC_CR) */
enum {
	STM32F10X_RCC_CFG_PLL_SRC_HSI	  = 0x0,
	STM32F10X_RCC_CFG_PLL_SRC_HSE     = 0x1,
};

enum {
	STM32F10X_RCC_CFG_PLL_XTPRE_DIV_0  = 0x0,
	STM32F10X_RCC_CFG_PLL_XTPRE_DIV_2  = 0x1,
};

enum {
	STM32F10X_RCC_CFG_SYSCLK_SRC_HSI = 0x0,
	STM32F10X_RCC_CFG_SYSCLK_SRC_HSE = 0x1,
	STM32F10X_RCC_CFG_SYSCLK_SRC_PLL = 0x2,
};

enum {
	STM32F10X_RCC_CFG_HCLK_DIV_0  = 0x0,
	STM32F10X_RCC_CFG_HCLK_DIV_2  = 0x4,
	STM32F10X_RCC_CFG_HCLK_DIV_4  = 0x5,
	STM32F10X_RCC_CFG_HCLK_DIV_8  = 0x6,
	STM32F10X_RCC_CFG_HCLK_DIV_16 = 0x7,
};

enum {
	STM32F10X_RCC_CFG_SYSCLK_DIV_0	 = 0x0,
	STM32F10X_RCC_CFG_SYSCLK_DIV_2	 = 0x8,
	STM32F10X_RCC_CFG_SYSCLK_DIV_4	 = 0x9,
	STM32F10X_RCC_CFG_SYSCLK_DIV_8	 = 0xa,
	STM32F10X_RCC_CFG_SYSCLK_DIV_16	 = 0xb,
	STM32F10X_RCC_CFG_SYSCLK_DIV_64	 = 0xc,
	STM32F10X_RCC_CFG_SYSCLK_DIV_128 = 0xd,
	STM32F10X_RCC_CFG_SYSCLK_DIV_256 = 0xe,
	STM32F10X_RCC_CFG_SYSCLK_DIV_512 = 0xf,
};

/**
  * @brief Reset and Clock Control
  */

union __rcc_cr {
	uint32_t val;
	struct {
		uint32_t hsion :1 __packed;
		uint32_t hsirdy :1 __packed;
		uint32_t rsvd__2 :1 __packed;
		uint32_t hsitrim :5 __packed;
		uint32_t hsical :8 __packed;
		uint32_t hseon :1 __packed;
		uint32_t hserdy :1 __packed;
		uint32_t hsebyp :1 __packed;
		uint32_t csson :1 __packed;
		uint32_t rsvd__20_23 :4 __packed;
		uint32_t pllon :1 __packed;
		uint32_t pllrdy :1 __packed;
		uint32_t rsvd__26_31 :6 __packed;
	} bit;
};

union __rcc_cfgr {
	uint32_t val;
	struct {
		uint32_t sw :2 __packed;
		uint32_t sws :2 __packed;
		uint32_t hpre :4 __packed;
		uint32_t ppre1 :3 __packed;
		uint32_t ppre2 :3 __packed;
		uint32_t adcpre :2 __packed;
		uint32_t pllsrc :1 __packed;
		uint32_t pllxtpre :1 __packed;
		uint32_t pllmul :4 __packed;
		uint32_t usbpre :1 __packed;
		uint32_t rsvd__23 :1 __packed;
		uint32_t mco :3 __packed;
		uint32_t rsvd__27_31 :5 __packed;
	} bit;
};

struct stm32f10x_rcc {
	union __rcc_cr cr;
	union __rcc_cfgr cfgr;
	uint32_t cir;
	uint32_t apb2rstr;
	uint32_t apb1rstr;
	uint32_t ahbenr;
	uint32_t apb2enr;
	uint32_t apb1enr;
	uint32_t bdcr;
	uint32_t csr;
};

#endif /* _STM32F10X_CLOCK_H_ */
