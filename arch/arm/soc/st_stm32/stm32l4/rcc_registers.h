/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
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

#ifndef _STM32L4X_CLOCK_H_
#define _STM32L4X_CLOCK_H_

/**
 * @brief Driver for Reset & Clock Control of STM32L4x6 family processor.
 */

enum {
	STM32L4X_RCC_CFG_PLL_SRC_MSI     = 0x1,
	STM32L4X_RCC_CFG_PLL_SRC_HSI     = 0x2,
	STM32L4X_RCC_CFG_PLL_SRC_HSE     = 0x3,
};

enum {
	STM32L4X_RCC_CFG_PLL_Q_R_0  = 0x1,
	STM32L4X_RCC_CFG_PLL_Q_R_2  = 0x2,
};

enum {
	STM32L4X_RCC_CFG_SYSCLK_SRC_MSI   = 0x0,
	STM32L4X_RCC_CFG_SYSCLK_SRC_HSI   = 0x1,
	STM32L4X_RCC_CFG_SYSCLK_SRC_HSE   = 0x2,
	STM32L4X_RCC_CFG_SYSCLK_SRC_PLL   = 0x3,
};

enum {
	STM32L4X_RCC_CFG_HCLK_DIV_0  = 0x0,
	STM32L4X_RCC_CFG_HCLK_DIV_2  = 0x4,
	STM32L4X_RCC_CFG_HCLK_DIV_4  = 0x5,
	STM32L4X_RCC_CFG_HCLK_DIV_8  = 0x6,
	STM32L4X_RCC_CFG_HCLK_DIV_16 = 0x7,
};

enum {
	STM32L4X_RCC_CFG_SYSCLK_DIV_0   = 0x0,
	STM32L4X_RCC_CFG_SYSCLK_DIV_2   = 0x8,
	STM32L4X_RCC_CFG_SYSCLK_DIV_4   = 0x9,
	STM32L4X_RCC_CFG_SYSCLK_DIV_8   = 0xa,
	STM32L4X_RCC_CFG_SYSCLK_DIV_16  = 0xb,
	STM32L4X_RCC_CFG_SYSCLK_DIV_64  = 0xc,
	STM32L4X_RCC_CFG_SYSCLK_DIV_128 = 0xd,
	STM32L4X_RCC_CFG_SYSCLK_DIV_256 = 0xe,
	STM32L4X_RCC_CFG_SYSCLK_DIV_512 = 0xf,
};

enum {
	STM32L4X_RCC_CFG_MCO_DIV_0  = 0x0,
	STM32L4X_RCC_CFG_MCO_DIV_2  = 0x1,
	STM32L4X_RCC_CFG_MCO_DIV_4  = 0x2,
	STM32L4X_RCC_CFG_MCO_DIV_8  = 0x3,
	STM32L4X_RCC_CFG_MCO_DIV_16 = 0x4,
};

/**
 * @brief Reset and Clock Control
 */

union __rcc_cr {
	uint32_t val;
	struct {
		uint32_t msion :1 __packed;
		uint32_t msirdy :1 __packed;
		uint32_t msipllen :1 __packed;
		uint32_t msirgsel :1 __packed;
		uint32_t msirange :4 __packed;
		uint32_t hsion :1 __packed;
		uint32_t hsikeron :1 __packed;
		uint32_t hsirdy :1 __packed;
		uint32_t hsiasfs :1 __packed;
		uint32_t rsvd__12_15 :4 __packed;
		uint32_t hseon :1 __packed;
		uint32_t hserdy :1 __packed;
		uint32_t hsebyp :1 __packed;
		uint32_t csson :1 __packed;
		uint32_t rsvd__20_23 :4 __packed;
		uint32_t pllon :1 __packed;
		uint32_t pllrdy :1 __packed;
		uint32_t pllsai1on :1 __packed;
		uint32_t pllsai1rdy :1 __packed;

		/*
		 * SAI2 not present on L4x2, L431xx, STM32L433xx,
		 * and STM32L443xx.
		 */
		uint32_t pllsai2on :1 __packed;
		uint32_t pllsai2rdy :1 __packed;
		uint32_t rsvd__30_31 :2 __packed;
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
		uint32_t stopwuck :1 __packed;
		uint32_t rsvd__16_23 :8 __packed;
		uint32_t mcosel :3 __packed; /* 2 bits long on L4x{1,5,6} */
		uint32_t mcopre :3 __packed;
		uint32_t rsvd__31 :1 __packed;
	} bit;
};

union __rcc_pllcfgr {
	uint32_t val;
	struct {
		uint32_t pllsrc :2 __packed;
		uint32_t rsvd__2_3 :2 __packed;
		uint32_t pllm :3 __packed;
		uint32_t rsvd__7 :1 __packed;
		uint32_t plln :7 __packed;
		uint32_t rsvd__15 :1 __packed;
		uint32_t pllpen :1 __packed;
		uint32_t pllp :1 __packed;
		uint32_t rsvd__18_19 :2 __packed;
		uint32_t pllqen :1 __packed;
		uint32_t pllq :2 __packed;
		uint32_t rsvd__23 :1 __packed;
		uint32_t pllren :1 __packed;
		uint32_t pllr :2 __packed;
		uint32_t pllpdiv :5 __packed; /* Not present on L4x{1,5,6} */
	} bit;
};

struct stm32l4x_rcc {
	union __rcc_cr cr;
	uint32_t icscr;
	union __rcc_cfgr cfgr;
	union __rcc_pllcfgr pllcfgr;
	uint32_t pllsai1cfgr;
	uint32_t pllsai2cfgr;
	uint32_t cier;
	uint32_t cifr;
	uint32_t cicr;
	uint32_t rsvd_0;
	uint32_t ahb1rstr;
	uint32_t ahb2rstr;
	uint32_t ahb3rstr;
	uint32_t rsvd_1;
	uint32_t apb1rstr1;
	uint32_t apb1rstr2;
	uint32_t apb2rstr;
	uint32_t rsvd_2;
	uint32_t ahb1enr;
	uint32_t ahb2enr;
	uint32_t ahb3enr;
	uint32_t rsvd_3;
	uint32_t apb1enr1;
	uint32_t apb1enr2;
	uint32_t apb2enr;
	uint32_t rsvd_4;
	uint32_t ahb1smenr;
	uint32_t ahb2smenr;
	uint32_t ahb3smenr;
	uint32_t rsvd_5;
	uint32_t apb1smenr1;
	uint32_t apb1smenr2;
	uint32_t apb2smenr;
	uint32_t rsvd_6;
	uint32_t ccipr;
	uint32_t rsvd_7;
	uint32_t bdcr;
	uint32_t csr;
};

#endif /* _STM32L4X_CLOCK_H_ */
