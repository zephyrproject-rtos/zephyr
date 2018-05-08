/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _STM32F7X_FLASH_REGISTERS_H_
#define _STM32F7X_FLASH_REGISTERS_H_

/**
 * @brief
 *
 * Based on reference manual:
 *
 * Chapter 3.4: Embedded Flash Memory
 */

union __flash_acr {
	u32_t val;
	struct {
		u32_t latency :4 __packed;
		u32_t rsvd__4_7 :4 __packed;
		u32_t prften :1 __packed;
		u32_t arten :1 __packed;
		u32_t rsvd__10 :1 __packed;
		u32_t artrst :1 __packed;
		u32_t rsvd__12_31 :20 __packed;
	} bit;
};

/* 3.8.7 Embedded flash registers */
struct stm32f7x_flash {
	volatile union __flash_acr acr;
	volatile u32_t keyr;
	volatile u32_t optkeyr;
	volatile u32_t sr;
	volatile u32_t cr;
	volatile u32_t optcr;
	volatile u32_t optcr1;
};

#endif	/* _STM32F7X_FLASHREGISTERS_H_ */
