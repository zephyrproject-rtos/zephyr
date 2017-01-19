/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32L4X_FLASH_REGISTERS_H_
#define _STM32L4X_FLASH_REGISTERS_H_

enum {
	STM32L4X_FLASH_LATENCY_0 = 0x0,
	STM32L4X_FLASH_LATENCY_1 = 0x1,
	STM32L4X_FLASH_LATENCY_2 = 0x2,
	STM32L4X_FLASH_LATENCY_3 = 0x3,
	STM32L4X_FLASH_LATENCY_4 = 0x4,
};

/* 3.7.1 FLASH_ACR */
union __ef_acr {
	uint32_t val;
	struct {
		uint32_t latency :3 __packed;
		uint32_t rsvd__3_7 :5 __packed;
		uint32_t prften :1 __packed;
		uint32_t icen :1 __packed;
		uint32_t dcen :1 __packed;
		uint32_t icrst :1 __packed;
		uint32_t dcrst :1 __packed;
		uint32_t run_pd :1 __packed;
		uint32_t sleep_pd :1 __packed;
		uint32_t rsvd__16_31 :17 __packed;
	} bit;
};

/*  FLASH register map */
struct stm32l4x_flash {
	union __ef_acr acr;
	uint32_t pdkeyr;
	uint32_t keyr;
	uint32_t optkeyr;
	uint32_t sr;
	uint32_t cr;
	uint32_t eccr;
	uint32_t rsvd_0;
	uint32_t optr;
	uint32_t pcrop1sr;
	uint32_t pcrop1er;
	uint32_t wrp1ar;
	uint32_t wrp1br;
	uint32_t rsvd_2[4];

	/*
	 * The registers below are only present on STM32L4x2, STM32L4x5,
	 * STM32L4x6.
	 */
	uint32_t pcrop2sr;
	uint32_t pcrop2er;
	uint32_t wrp2ar;
	uint32_t wrp2br;
};

#endif	/* _STM32L4X_FLASH_REGISTERS_H_ */
