/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F4X_FLASH_REGISTERS_H_
#define _STM32F4X_FLASH_REGISTERS_H_

/**
 * @brief
 *
 * Based on reference manual:
 *
 * Chapter 3.4: Embedded Flash Memory
 */

enum {
	STM32F4X_FLASH_LATENCY_0 = 0x0,
	STM32F4X_FLASH_LATENCY_1 = 0x1,
	STM32F4X_FLASH_LATENCY_2 = 0x2,
	STM32F4X_FLASH_LATENCY_3 = 0x3,
	STM32F4X_FLASH_LATENCY_4 = 0x4,
	STM32F4X_FLASH_LATENCY_5 = 0x5,
};

union __flash_acr {
	u32_t val;
	struct {
		u32_t latency :4 __packed;
		u32_t rsvd__4_7 :4 __packed;
		u32_t prften :1 __packed;
		u32_t icen :1 __packed;
		u32_t dcen :1 __packed;
		u32_t icrst :1 __packed;
		u32_t dcrst :1 __packed;
		u32_t rsvd__13_31 :19 __packed;
	} bit;
};

/* 3.8.7 Embedded flash registers */
struct stm32f4x_flash {
	volatile union __flash_acr acr;
	volatile u32_t key;
	volatile u32_t optkey;
	volatile u32_t status;
	volatile u32_t ctrl;
	volatile u32_t optctrl;
};

/**
 * @brief setup embedded flash controller
 *
 * Configure flash access time latency (wait states) depending on
 * SYSCLK.  This code assumes that we're using a supply voltage of
 * 2.7V or higher, for lower voltages this code must be changed.
 *
 * The following tables show the required latency value required for a
 * certain CPU frequency (HCLK) and supply voltage.  See the section
 * "Relation between CPU clock frequency and Flash memory read time"
 * in the reference manual for more information.
 *
 * Note that the highest frequency might be limited for other reaasons
 * than wait states, for example the STM32F405xx is limited to 168MHz
 * even with 5 wait states and the highest supply voltage.
 *
 * STM32F401xx:
 *
 * LATENCY | 2.7V - 3.6V | 2.4V - 2.7V | 2.1V - 2.4V | 1.8V - 2.1V
 * ------- | ----------- | ----------- | ----------- | -----------
 *    0    |    30 MHz   |    24 MHz   |    18 MHz   |    16 MHz
 *    1    |    60 MHz   |    48 MHz   |    36 MHz   |    32 MHz
 *    2    |    84 MHz   |    72 MHz   |    54 MHz   |    48 MHz
 *    3    |             |    84 MHz   |    72 MHz   |    64 MHz
 *    4    |             |             |    84 MHz   |    80 MHz
 *    5    |             |             |             |    84 MHz
 *
 * STM32F405xx/407xx/415xx/417xx/42xxx/43xxx:
 *
 * LATENCY | 2.7V - 3.6V | 2.4V - 2.7V | 2.1V - 2.4V | 1.8V - 2.1V
 * ------- | ----------- | ----------- | ----------- | -----------
 *    0    |    30 MHz   |    24 MHz   |    22 MHz   |    20 MHz
 *    1    |    60 MHz   |    48 MHz   |    44 MHz   |    40 MHz
 *    2    |    90 MHz   |    72 MHz   |    66 MHz   |    60 MHz
 *    3    |   120 MHz   |    96 MHz   |    88 MHz   |    80 MHz
 *    4    |   150 MHz   |   120 MHz   |   110 MHz   |   100 MHz
 *    5    |   180 MHz   |   144 MHz   |   132 MHz   |   120 MHz
 *    6    |             |   168 MHz   |   154 MHz   |   140 MHz
 *    7    |             |   180 MHz   |   176 MHz   |   160 MHz
 *    8    |             |             |   180 MHz   |   168 MHz
 *
 * STM32F411x:
 *
 * LATENCY | 2.7V - 3.6V | 2.4V - 2.7V | 2.1V - 2.4V | 1.7V - 2.1V
 * ------- | ----------- | ----------- | ----------- | -----------
 *    0    |    30 MHz   |    24 MHz   |    18 MHz   |    16 MHz
 *    1    |    64 MHz   |    48 MHz   |    36 MHz   |    32 MHz
 *    2    |    90 MHz   |    72 MHz   |    54 MHz   |    48 MHz
 *    3    |   100 MHz   |    96 MHz   |    72 MHz   |    64 MHz
 *    4    |             |   100 MHz   |    90 MHz   |    80 MHz
 *    5    |             |             |   100 MHz   |    96 MHz
 *    6    |             |             |             |   100 MHz
 */
static inline void __setup_flash(void)
{
	volatile struct stm32f4x_flash *regs;
	u32_t tmpreg = 0;

	regs = (struct stm32f4x_flash *) FLASH_R_BASE;

	if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 30000000) {
		regs->acr.bit.latency = STM32F4X_FLASH_LATENCY_0;
	}
#ifdef CONFIG_SOC_STM32F401XE
	else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 60000000) {
		regs->acr.bit.latency = STM32F4X_FLASH_LATENCY_1;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 84000000) {
		regs->acr.bit.latency = STM32F4X_FLASH_LATENCY_2;
	}
#elif CONFIG_SOC_STM32F411XE
	else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 64000000) {
		regs->acr.bit.latency = STM32F4X_FLASH_LATENCY_1;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 90000000) {
		regs->acr.bit.latency = STM32F4X_FLASH_LATENCY_2;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 100000000) {
		regs->acr.bit.latency = STM32F4X_FLASH_LATENCY_3;
	}
#elif defined(CONFIG_SOC_STM32F407XX) || defined(CONFIG_SOC_STM32F429XX)
	else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 60000000) {
		regs->acr.bit.latency = STM32F4X_FLASH_LATENCY_1;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 90000000) {
		regs->acr.bit.latency = STM32F4X_FLASH_LATENCY_2;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 120000000) {
		regs->acr.bit.latency = STM32F4X_FLASH_LATENCY_3;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 150000000) {
		regs->acr.bit.latency = STM32F4X_FLASH_LATENCY_4;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 180000000) {
		regs->acr.bit.latency = STM32F4X_FLASH_LATENCY_5;
	}
#else
#error Flash latency configuration for MCU model is missing
#endif

	/* Make sure latency was set */
	tmpreg = regs->acr.bit.latency;

}

#endif	/* _STM32F4X_FLASHREGISTERS_H_ */
