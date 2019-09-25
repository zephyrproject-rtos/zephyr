/*
 * Copyright (c) 2019 Philippe Retornaz <philippe@shapescale.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32G0X_FLASH_REGISTERS_H_
#define _STM32G0X_FLASH_REGISTERS_H_

#define FLASH_SR_BSY FLASH_SR_BSY1

enum {
	STM32_FLASH_LATENCY_0 = 0x0,
	STM32_FLASH_LATENCY_1 = 0x1,
	STM32_FLASH_LATENCY_2 = 0x2,
};

/* 3.7.1 FLASH_ACR */
union __ef_acr {
	u32_t val;
	struct {
		u32_t latency :3 __packed;
		u32_t rsvd__3_7 :5 __packed;
		u32_t prften :1 __packed;
		u32_t icen :1 __packed;
		u32_t rsvd__10 :1 __packed;
		u32_t icrst :1 __packed;
		u32_t rsvd__12_15 :4 __packed;
		u32_t empty :1 __packed;
		u32_t rsvd__17 :1 __packed;
		u32_t dbg_swend :1 __packed;
		u32_t rsvd__19_31 :13 __packed;
	} bit;
};

/*  FLASH register map */
struct stm32g0x_flash {
	volatile union __ef_acr acr;
	volatile u32_t rsvd__4;
	volatile u32_t keyr;
	volatile u32_t optkeyr;
	volatile u32_t sr;
	volatile u32_t cr;
	volatile u32_t eccr;
	volatile u32_t rsvd_0;
	volatile u32_t optr;
	volatile u32_t pcrop1sr;
	volatile u32_t pcrop1er;
	volatile u32_t wrp1ar;
	volatile u32_t wrp1br;
};

#endif	/* _STM32G0X_FLASH_REGISTERS_H_ */
