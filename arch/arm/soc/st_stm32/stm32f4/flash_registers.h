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
	uint32_t val;
	struct {
		uint32_t latency :4 __packed;
		uint32_t rsvd__4_7 :4 __packed;
		uint32_t prften :1 __packed;
		uint32_t icen :1 __packed;
		uint32_t dcen :1 __packed;
		uint32_t icrst :1 __packed;
		uint32_t dcrst :1 __packed;
		uint32_t rsvd__13_31 :19 __packed;
	} bit;
};

/* 3.8.7 Embedded flash registers */
struct stm32f4x_flash {
	union __flash_acr acr;
	uint32_t key;
	uint32_t optkey;
	volatile uint32_t status;
	volatile uint32_t ctrl;
	uint32_t optctrl;
};

/**
 * @brief setup embedded flash controller
 *
 * Configure flash access time latency depending on SYSCLK.
 */
static inline void __setup_flash(void)
{
	volatile struct stm32f4x_flash *regs;
	uint32_t tmpreg = 0;

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
#else
	else {
		__ASSERT(0, "Flash latency not set");
	}
#endif

	/* Make sure latency was set */
	tmpreg = regs->acr.bit.latency;

}

#endif	/* _STM32F4X_FLASHREGISTERS_H_ */
