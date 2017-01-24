/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _DRIVERS_DUALTIMER_CMSDK_AHB_
#define _DRIVERS_DUALTIMER_CMSDK_AHB_

#include <counter.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dualtimer_cmsdk_apb {
	/* Offset: 0x000 (R/W) Timer 1 Load */
	volatile uint32_t timer1load;
	/* Offset: 0x004 (R/ ) Timer 1 Counter Current Value */
	volatile uint32_t timer1value;
	/* Offset: 0x008 (R/W) Timer 1 Control */
	volatile uint32_t timer1ctrl;
	/* Offset: 0x00C ( /W) Timer 1 Interrupt Clear */
	volatile uint32_t timer1intclr;
	/* Offset: 0x010 (R/ ) Timer 1 Raw Interrupt Status */
	volatile uint32_t timer1ris;
	/* Offset: 0x014 (R/ ) Timer 1 Masked Interrupt Status */
	volatile uint32_t timer1mis;
	/* Offset: 0x018 (R/W) Background Load Register */
	volatile uint32_t timer1bgload;
	/* Reserved */
	volatile uint32_t reserved0;
	/* Offset: 0x020 (R/W) Timer 2 Load */
	volatile uint32_t timer2load;
	/* Offset: 0x024 (R/ ) Timer 2 Counter Current Value */
	volatile uint32_t timer2value;
	/* Offset: 0x028 (R/W) Timer 2 Control */
	volatile uint32_t timer2ctrl;
	/* Offset: 0x02C ( /W) Timer 2 Interrupt Clear */
	volatile uint32_t timer2intclr;
	/* Offset: 0x030 (R/ ) Timer 2 Raw Interrupt Status */
	volatile uint32_t timer2ris;
	/* Offset: 0x034 (R/ ) Timer 2 Masked Interrupt Status */
	volatile uint32_t timer2mis;
	/* Offset: 0x038 (R/W) Background Load Register */
	volatile uint32_t timer2bgload;
	/* Reserved */
	volatile uint32_t reserved1[945];
	/* Offset: 0xF00 (R/W) Integration Test Control Register */
	volatile uint32_t itcr;
	/* Offset: 0xF04 ( /W) Integration Test Output Set Register */
	volatile uint32_t itop;
};

#define DUALTIMER_CTRL_EN	(1 << 7)
#define DUALTIMER_CTRL_MODE	(1 << 6)
#define DUALTIMER_CTRL_INTEN	(1 << 5)
#define DUALTIMER_CTRL_PRESCALE	(3 << 2)
#define DUALTIMER_CTRL_SIZE_32	(1 << 1)
#define DUALTIMER_CTRL_ONESHOOT	(1 << 0)
#define DUALTIMER_INTCLR	(1 << 0)
#define DUALTIMER_RAWINTSTAT	(1 << 0)
#define DUALTIMER_MASKINTSTAT	(1 << 0)

#ifdef __cplusplus
}
#endif

#endif /* _DRIVERS_DUALTIMER_CMSDK_AHB_ */
