/*
 * Copyright (c) 2016 Linaro Limited.
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
	STM32F7X_FLASH_LATENCY_0 = 0x0,
	STM32F7X_FLASH_LATENCY_1 = 0x1,
	STM32F7X_FLASH_LATENCY_2 = 0x2,
	STM32F7X_FLASH_LATENCY_3 = 0x3,
	STM32F7X_FLASH_LATENCY_4 = 0x4,
	STM32F7X_FLASH_LATENCY_5 = 0x5,
	STM32F7X_FLASH_LATENCY_6 = 0x6,
	STM32F7X_FLASH_LATENCY_7 = 0x7,
	STM32F7X_FLASH_LATENCY_8 = 0x8,
	STM32F7X_FLASH_LATENCY_9 = 0x9,
};

union __flash_acr {
	uint32_t val;
	struct {
		uint32_t latency :4 __packed;
		uint32_t rsvd__4_7 :4 __packed;
		uint32_t prften :1 __packed;
		uint32_t arten :1 __packed;
		uint32_t rsvd__10 :1 __packed;
		uint32_t artrst :1 __packed;
		uint32_t rsvd__12_31 :20 __packed;
	} bit;
};

/* 3.7 Embedded flash registers */
struct stm32f7x_flash {
	union __flash_acr acr;
	uint32_t key;
	uint32_t optkey;
	volatile uint32_t status;
	volatile uint32_t ctrl;
	uint32_t optctrl;
	uint32_t optctrl1;

};

/**
 * @brief setup embedded flash controller
 *
 * Configure flash access time latency depending on SYSCLK.
 */
static inline void __setup_flash(void)
{
	volatile struct stm32f7x_flash *regs;
	uint32_t tmpreg = 0;

	regs = (struct stm32f7x_flash *) FLASH_R_BASE;

	if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 30000000) {
		regs->acr.bit.latency = STM32F7X_FLASH_LATENCY_0;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 60000000) {
		regs->acr.bit.latency = STM32F7X_FLASH_LATENCY_1;
	} else if (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC <= 90000000) {
		regs->acr.bit.latency = STM32F7X_FLASH_LATENCY_2;
	}

	/* Make sure latency was set */
	tmpreg = regs->acr.bit.latency;
}

#endif	/* _STM32F7X_FLASHREGISTERS_H_ */
