/*
 * Copyright (c) 2021 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_A_R_TIMER_H_
#define ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_A_R_TIMER_H_

#ifdef CONFIG_ARM_ARCH_TIMER

#ifndef _ASMLANGUAGE

#include <drivers/timer/arm_arch_timer.h>
#include <sys/device_mmio.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ARM_ARCH_TIMER_BASE     DT_REG_ADDR_BY_IDX(ARM_TIMER_NODE, 0)
#define ARM_ARCH_TIMER_IRQ      ARM_TIMER_VIRTUAL_IRQ
#define ARM_ARCH_TIMER_PRIO     ARM_TIMER_VIRTUAL_PRIO
#define ARM_ARCH_TIMER_FLAGS    ARM_TIMER_VIRTUAL_FLAGS

#define TIMER_CNT_LOWER         0x00
#define TIMER_CNT_UPPER         0x04
#define TIMER_CTRL              0x08
#define TIMER_ISR               0x0c
#define TIMER_CMP_LOWER         0x10
#define TIMER_CMP_UPPER         0x14

#define TIMER_IRQ_ENABLE        BIT(2)
#define TIMER_COMP_ENABLE       BIT(1)
#define TIMER_ENABLE            BIT(0)

DEVICE_MMIO_TOPLEVEL_STATIC(timer_regs, ARM_TIMER_NODE);

#define TIMER_REG_GET(offs) (DEVICE_MMIO_TOPLEVEL_GET(timer_regs) + offs)

static ALWAYS_INLINE void arm_arch_timer_init(void)
{
	DEVICE_MMIO_TOPLEVEL_MAP(timer_regs, K_MEM_CACHE_NONE);
}

static ALWAYS_INLINE void arm_arch_timer_set_compare(uint64_t val)
{
	uint32_t lower = (uint32_t)val;
	uint32_t upper = (uint32_t)(val >> 32);
	uint32_t ctrl;

	/* Disable IRQ and comparator */
	ctrl = sys_read32(TIMER_REG_GET(TIMER_CTRL));
	ctrl &= ~(TIMER_COMP_ENABLE | TIMER_IRQ_ENABLE);
	sys_write32(ctrl, TIMER_REG_GET(TIMER_CTRL));

	sys_write32(lower, TIMER_REG_GET(TIMER_CMP_LOWER));
	sys_write32(upper, TIMER_REG_GET(TIMER_CMP_UPPER));

	/* enable comparator back, let set_irq_mask enabling the IRQ again */
	ctrl |= TIMER_COMP_ENABLE;
	sys_write32(ctrl, TIMER_REG_GET(TIMER_CTRL));
}

static ALWAYS_INLINE void arm_arch_timer_enable(bool enable)
{
	uint32_t ctrl;

	ctrl = sys_read32(TIMER_REG_GET(TIMER_CTRL));
	if (enable) {
		ctrl |= TIMER_ENABLE;
	} else {
		ctrl &= ~TIMER_ENABLE;
	}

	sys_write32(ctrl, TIMER_REG_GET(TIMER_CTRL));
}

static ALWAYS_INLINE void arm_arch_timer_set_irq_mask(bool mask)
{
	uint32_t ctrl;

	ctrl = sys_read32(TIMER_REG_GET(TIMER_CTRL));
	if (mask) {
		ctrl &= ~TIMER_IRQ_ENABLE;
	} else {
		ctrl |= TIMER_IRQ_ENABLE;
		sys_write32(1, TIMER_REG_GET(TIMER_ISR));
	}
	sys_write32(ctrl, TIMER_REG_GET(TIMER_CTRL));
}

static ALWAYS_INLINE uint64_t arm_arch_timer_count(void)
{
	uint32_t lower;
	uint32_t upper, upper_saved;

	/* To get the value from the Global Timer Counter register proceed
	 * as follows:
	 * 1. Read the upper 32-bit timer counter register.
	 * 2. Read the lower 32-bit timer counter register.
	 * 3. Read the upper 32-bit timer counter register again. If the value
	 * is different to the 32-bit upper value read previously,
	 * go back to step 2.
	 * Otherwise the 64-bit timer counter value is correct.
	 */
	upper = sys_read32(TIMER_REG_GET(TIMER_CNT_UPPER));
	do {
		upper_saved = upper;
		lower = sys_read32(TIMER_REG_GET(TIMER_CNT_LOWER));
		upper = sys_read32(TIMER_REG_GET(TIMER_CNT_UPPER));
	} while (upper != upper_saved);

	return ((uint64_t)upper) << 32 | lower;
}

#ifdef __cplusplus
}
#endif

#endif  /* _ASMLANGUAGE */

#endif /* CONFIG_ARM_ARCH_TIMER */

#endif  /* ZEPHYR_INCLUDE_ARCH_ARM_AARCH32_CORTEX_A_R_TIMER_H_ */
