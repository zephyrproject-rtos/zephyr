/*
 * Copyright (c) 2016 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F3X_FLASH_REGISTERS_H_
#define _STM32F3X_FLASH_REGISTERS_H_

#include <zephyr/types.h>

/**
 * @brief
 *
 * Based on reference manual:
 *   STM32F101xx, STM32F102xx, STM32F103xx, STM32F105xx and STM32F107xx
 *   advanced ARM(r)-based 32-bit MCUs
 *   &
 *   STM32F334xx advanced ARM(r)-based 32-bit MCUs
 *
 * Chapter 3.3.3: Embedded Flash Memory
 */

enum {
	STM32_FLASH_LATENCY_0 = 0x0,
	STM32_FLASH_LATENCY_1 = 0x1,
	STM32_FLASH_LATENCY_2 = 0x2,
};

/* 3.3.3 FLASH_ACR */
union ef_acr {
	u32_t val;
	struct {
		u32_t latency :3 __packed;
		u32_t hlfcya :1 __packed;
		u32_t prftbe :1 __packed;
		u32_t prftbs :1 __packed;
		u32_t rsvd__6_31 :26 __packed;
	} bit;
};

/* 3.3.3 Embedded flash registers */
struct stm32f3x_flash {
	volatile union ef_acr acr;
	volatile u32_t keyr;
	volatile u32_t optkeyr;
	volatile u32_t sr;
	volatile u32_t cr;
	volatile u32_t ar;
	volatile u32_t rsvd;
	volatile u32_t obr;
	volatile u32_t wrpr;
};

/* list of device commands */
enum stm32_embedded_flash_cmd {
	STM32_FLASH_CMD_LATENCY_FOR_CLOCK_SET,
};

#endif /* _STM32F3X_FLASH_REGISTERS_H_ */
