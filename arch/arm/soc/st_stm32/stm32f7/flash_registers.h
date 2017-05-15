/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2017 BayLibre, SAS
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
 * Chapter 3: Embedded Flash Memory (FLASH)
 */

union __flash_acr {
	u32_t val;
	struct {
		u32_t latency :4 __packed;
		u32_t rsvd__4_7 :4 __packed;
		u32_t prften :1 __packed;
		u32_t arten :1 __packed;
		u32_t rsvd_10_10 :1 __packed;
		u32_t artrst :1 __packed;
		u32_t rsvd__12_31 :20 __packed;
	} bit;
};

/* 3.8.7 Embedded flash registers */
struct stm32f7x_flash {
	volatile union __flash_acr acr;
	volatile u32_t key;
	volatile u32_t optkey;
	volatile u32_t status;
	volatile u32_t ctrl;
	volatile u32_t optctrl;
	volatile u32_t optctrl1;
	volatile u32_t optctrl2;
};

#endif	/* _STM32F7X_FLASHREGISTERS_H_ */
