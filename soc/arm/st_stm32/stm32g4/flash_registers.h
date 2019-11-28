/*
 * Copyright (c) 2019 Richard Osterloh <richard.osterloh@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32G4X_FLASH_REGISTERS_H_
#define _STM32G4X_FLASH_REGISTERS_H_

enum {
	STM32G4X_FLASH_LATENCY_0 = 0x0,
	STM32G4X_FLASH_LATENCY_1 = 0x1,
	STM32G4X_FLASH_LATENCY_2 = 0x2,
	STM32G4X_FLASH_LATENCY_3 = 0x3,
	STM32G4X_FLASH_LATENCY_4 = 0x4,
	STM32G4X_FLASH_LATENCY_5 = 0x5,
	STM32G4X_FLASH_LATENCY_6 = 0x6,
	STM32G4X_FLASH_LATENCY_7 = 0x7,
	STM32G4X_FLASH_LATENCY_8 = 0x8,
	STM32G4X_FLASH_LATENCY_9 = 0x9,
	STM32G4X_FLASH_LATENCY_10 = 0x10,
	STM32G4X_FLASH_LATENCY_11 = 0x11,
	STM32G4X_FLASH_LATENCY_12 = 0x12,
	STM32G4X_FLASH_LATENCY_13 = 0x13,
	STM32G4X_FLASH_LATENCY_14 = 0x14,
};

/* 3.7.1 FLASH_ACR */
union __ef_acr {
	u32_t val;
	struct {
		u32_t latency :4 __packed;
		u32_t rsvd__4_7 :4 __packed;
		u32_t prften :1 __packed;
		u32_t icen :1 __packed;
		u32_t dcen :1 __packed;
		u32_t icrst :1 __packed;
		u32_t dcrst :1 __packed;
		u32_t run_pd :1 __packed;
		u32_t sleep_pd :1 __packed;
		u32_t rsvd__15_17 :3 __packed;
		u32_t dbg_swend :1 __packed;
		u32_t rsvd__19_31 :13 __packed;
	} bit;
};

/* 3.7.13 Embedded flash registers */
struct stm32g4x_flash {
	volatile union __ef_acr acr;
	volatile u32_t pdkeyr;
	volatile u32_t keyr;
	volatile u32_t optkeyr;
	volatile u32_t sr;
	volatile u32_t cr;
	volatile u32_t eccr;
	volatile u32_t optr;
	volatile u32_t pcrop1sr;
	volatile u32_t pcrop1er;
	volatile u32_t wrp1ar;
	volatile u32_t wrp1br;
	volatile u32_t pcrop2sr;
	volatile u32_t pcrop2er;
	volatile u32_t wrp2ar;
	volatile u32_t wrp2br;
};

#endif	/* _STM32G4X_FLASH_REGISTERS_H_ */
