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

#ifndef _STM32F4X_GPIO_REGISTERS_H_
#define _STM32F4X_GPIO_REGISTERS_H_

/**
 * @brief Driver for GPIO of STM32F4X family processor.
 *
 * Based on reference manual:
 *   RM0368 Reference manual STM32F401xB/C and STM32F401xD/E
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 8: General-purpose I/Os (GPIOs)
 */

/* 8.4 GPIO registers - each GPIO port controls 16 pins */
struct stm32f4x_gpio {
	uint32_t mode;
	uint32_t otype;
	uint32_t ospeed;
	uint32_t pupdr;
	uint32_t idr;
	uint32_t odr;
	uint32_t bsr;
	uint32_t lck;
	uint32_t afr[2];
};

union syscfg_exticr {
	uint32_t val;
	struct {
		uint16_t rsvd__16_31;
		uint16_t exti;
	} bit;
};

/* 7.2 SYSCFG registers */
struct stm32f4x_syscfg {
	uint32_t memrmp;
	uint32_t pmc;
	union syscfg_exticr exticr1;
	union syscfg_exticr exticr2;
	union syscfg_exticr exticr3;
	union syscfg_exticr exticr4;
	uint32_t cmpcr;
};

#endif /* _STM32F4X_GPIO_REGISTERS_H_ */
