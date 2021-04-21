/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_COUNTER_TIMER_CMSDK_APB_H_
#define ZEPHYR_DRIVERS_COUNTER_TIMER_CMSDK_APB_H_

#include <drivers/counter.h>

#ifdef __cplusplus
extern "C" {
#endif

struct timer_cmsdk_apb {
	/* Offset: 0x000 (R/W) control register */
	volatile uint32_t ctrl;
	/* Offset: 0x004 (R/W) current value register */
	volatile uint32_t value;
	/* Offset: 0x008 (R/W) reload value register */
	volatile uint32_t reload;
	union {
		/* Offset: 0x00C (R/ ) interrupt status register */
		volatile uint32_t intstatus;
		/* Offset: 0x00C ( /W) interruptclear register */
		volatile uint32_t intclear;
	};
};

#define TIMER_CTRL_IRQ_EN       (1 << 3)
#define TIMER_CTRL_SEL_EXT_CLK  (1 << 2)
#define TIMER_CTRL_SEL_EXT_EN   (1 << 1)
#define TIMER_CTRL_EN           (1 << 0)
#define TIMER_CTRL_INT_CLEAR    (1 << 0)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_COUNTER_TIMER_CMSDK_APB_H_ */
