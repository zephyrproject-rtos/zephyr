/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32WBX_FLASH_REGISTERS_H_
#define _STM32WBX_FLASH_REGISTERS_H_

enum {
	STM32WBX_FLASH_LATENCY_0 = 0x0,
	STM32WBX_FLASH_LATENCY_1 = 0x1,
	STM32WBX_FLASH_LATENCY_2 = 0x2,
	STM32WBX_FLASH_LATENCY_3 = 0x3,
	STM32WBX_FLASH_LATENCY_4 = 0x4,
};

/* 3.7.1 FLASH_ACR */
union __ef_acr {
	u32_t val;
	struct {
		u32_t latency :3 __packed;
		u32_t rsvd__3_7 :5 __packed;
		u32_t prften :1 __packed;
		u32_t icen :1 __packed;
		u32_t dcen :1 __packed;
		u32_t icrst :1 __packed;
		u32_t dcrst :1 __packed;
		u32_t rsvd__13_14 :2 __packed;
		u32_t pes :1 __packed;
		u32_t empty :1 __packed;
		u32_t rsvd__17_31 :15 __packed;
	} bit;
};

/*  FLASH register map */
struct stm32wbx_flash {
	volatile union __ef_acr acr;
	volatile u32_t rsvd_0;
	volatile u32_t keyr;
	volatile u32_t optkeyr;
	volatile u32_t sr;
	volatile u32_t cr;
	volatile u32_t eccr;
	volatile u32_t rsvd_1;
	volatile u32_t optr;
	volatile u32_t pcrop1asr;
	volatile u32_t pcrop1aer;
	volatile u32_t wrp1ar;
	volatile u32_t wrp1br;
	volatile u32_t pcrop1bsr;
	volatile u32_t pcrop1ber;
	volatile u32_t ipccbr;
	volatile u32_t rsvd_2[8];
	volatile u32_t c2acr;
	volatile u32_t c2sr;
	volatile u32_t c2cr;
	volatile u32_t rsvd_3[7];
	volatile u32_t sfr;
	volatile u32_t srrvr;
};

#endif	/* _STM32WBX_FLASH_REGISTERS_H_ */
