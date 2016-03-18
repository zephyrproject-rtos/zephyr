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

#ifndef _STM32F10X_FLASH_REGISTERS_H_
#define _STM32F10X_FLASH_REGISTERS_H_

/**
 * @brief
 *
 * Based on reference manual:
 *   STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx
 *   advanced ARM ® -based 32-bit MCUs
 *
 * Chapter 3.3.3: Embedded Flash Memory
 */

enum {
	STM32F10X_FLASH_LATENCY_0 = 0x0,
	STM32F10X_FLASH_LATENCY_1 = 0x1,
	STM32F10X_FLASH_LATENCY_2 = 0x2,
};

/* 3.3.3 FLASH_ACR */
union __ef_acr {
	uint32_t val;
	struct {
		uint32_t latency :3 __packed;
		uint32_t hlfcya :1 __packed;
		uint32_t prftbe :1 __packed;
		uint32_t prftbs :1 __packed;
		uint32_t rsvd__6_31 :26 __packed;
	} bit;
};

/* 3.3.3 Embedded flash registers */
struct stm32f10x_flash {
	union __ef_acr acr;
	uint32_t keyr;
	uint32_t optkeyr;
	uint32_t sr;
	uint32_t cr;
	uint32_t ar;
	uint32_t rsvd;
	uint32_t obr;
	uint32_t wrpr;
};

#endif	/* _STM32F10X_FLASHREGISTERS_H_ */
