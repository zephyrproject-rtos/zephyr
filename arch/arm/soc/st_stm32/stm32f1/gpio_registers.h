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

#ifndef _STM32F10X_GPIO_REGISTERS_H_
#define _STM32F10X_GPIO_REGISTERS_H_

/**
 * @brief
 *
 * Based on reference manual:
 *   STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 9: General-purpose and alternate-function I/Os
 *            (GPIOs and AFIOs)
 */

/* 9.2 GPIO registers - each GPIO port controls 16 pins */
struct stm32f10x_gpio {
	uint32_t crl;
	uint32_t crh;
	uint32_t idr;
	uint32_t odr;
	uint32_t bsrr;
	uint32_t brr;
	uint32_t lckr;
};

#endif /* _STM32F10X_GPIO_REGISTERS_H_ */
