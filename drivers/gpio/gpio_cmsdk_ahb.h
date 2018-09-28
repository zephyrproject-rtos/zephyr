/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_GPIO_GPIO_CMSDK_AHB_H_
#define ZEPHYR_DRIVERS_GPIO_GPIO_CMSDK_AHB_H_

#include <gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ARM LTD CMSDK AHB General Purpose Input/Output (GPIO) */
struct gpio_cmsdk_ahb {
	/* Offset: 0x000 (r/w) data register */
	volatile u32_t data;
	/* Offset: 0x004 (r/w) data output latch register */
	volatile u32_t dataout;
	volatile u32_t reserved0[2];
	/* Offset: 0x010 (r/w) output enable set register */
	volatile u32_t outenableset;
	/* Offset: 0x014 (r/w) output enable clear register */
	volatile u32_t outenableclr;
	/* Offset: 0x018 (r/w) alternate function set register */
	volatile u32_t altfuncset;
	/* Offset: 0x01c (r/w) alternate function clear register */
	volatile u32_t altfuncclr;
	/* Offset: 0x020 (r/w) interrupt enable set register */
	volatile u32_t intenset;
	/* Offset: 0x024 (r/w) interrupt enable clear register */
	volatile u32_t intenclr;
	/* Offset: 0x028 (r/w) interrupt type set register */
	volatile u32_t inttypeset;
	/* Offset: 0x02c (r/w) interrupt type clear register */
	volatile u32_t inttypeclr;
	/* Offset: 0x030 (r/w) interrupt polarity set register */
	volatile u32_t intpolset;
	/* Offset: 0x034 (r/w) interrupt polarity clear register */
	volatile u32_t intpolclr;
	union {
		/* Offset: 0x038 (r/ ) interrupt status register */
		volatile u32_t intstatus;
		/* Offset: 0x038 ( /w) interrupt clear register */
		volatile u32_t  intclear;
	};
	volatile u32_t reserved1[241];
	/* Offset: 0x400 - 0x7fc lower byte masked access register (r/w) */
	volatile u32_t lb_masked[256];
	/* Offset: 0x800 - 0xbfc upper byte masked access register (r/w) */
	volatile u32_t ub_masked[256];
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_GPIO_GPIO_CMSDK_AHB_H_ */
