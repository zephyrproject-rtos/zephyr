/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_HISILICON_HI3861_REG_HI3861_TIMER_REGS_H_
#define ZEPHYR_SOC_HISILICON_HI3861_REG_HI3861_TIMER_REGS_H_

#include <stddef.h>
#include <stdint.h>
#include <zephyr/sys/util.h>

struct hi3861_timer_regs {
	volatile uint32_t load;
	volatile const uint32_t value;
	volatile uint32_t control;
	volatile const uint32_t eoi;
	volatile const uint32_t int_status;
};

#define TIMER_LOAD    offsetof(struct hi3861_timer_regs, load)
#define TIMER_VALUE   offsetof(struct hi3861_timer_regs, value)
#define TIMER_CONTROL offsetof(struct hi3861_timer_regs, control)
#define TIMER_EOI     offsetof(struct hi3861_timer_regs, eoi)

#define TIMER_ENABLE   BIT(0)
#define TIMER_PERIODIC BIT(1)
#define TIMER_MASK_IRQ BIT(2)
#define TIMER_SNAPSHOT BIT(3)

BUILD_ASSERT(TIMER_LOAD == 0x00 && TIMER_VALUE == 0x04);
BUILD_ASSERT(TIMER_CONTROL == 0x08 && TIMER_EOI == 0x0c);
BUILD_ASSERT(offsetof(struct hi3861_timer_regs, int_status) == 0x10);
BUILD_ASSERT(sizeof(struct hi3861_timer_regs) == 0x14);

#endif /* ZEPHYR_SOC_HISILICON_HI3861_REG_HI3861_TIMER_REGS_H_ */
