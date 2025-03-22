/*
 * Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ARCH_ARM64_TIMER_H_
#define ZEPHYR_INCLUDE_ARCH_ARM64_TIMER_H_

#ifndef _ASMLANGUAGE

#include <limits.h>

#include <zephyr/drivers/timer/arm_arch_timer.h>
#include <zephyr/types.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_ZVM) && defined(CONFIG_HAS_ARM_VHE)
#define ARM_ARCH_TIMER_IRQ	ARM_TIMER_HYP_IRQ
#define ARM_ARCH_TIMER_PRIO	ARM_TIMER_HYP_PRIO
#define ARM_ARCH_TIMER_FLAGS	ARM_TIMER_HYP_FLAGS

#define ARM_ARCH_VIRT_VTIMER_IRQ	ARM_TIMER_VIRTUAL_IRQ
#define ARM_ARCH_VIRT_VTIMER_PRIO	ARM_TIMER_VIRTUAL_PRIO
#define ARM_ARCH_VIRT_VTIMER_FLAGS	ARM_TIMER_VIRTUAL_FLAGS
#define ARM_ARCH_VIRT_PTIMER_IRQ	ARM_TIMER_NON_SECURE_IRQ
#define ARM_ARCH_VIRT_PTIMER_PRIO	ARM_TIMER_NON_SECURE_PRIO
#define ARM_ARCH_VIRT_PTIMER_FLAGS	ARM_TIMER_NON_SECURE_FLAGS

#define HOST_CYC_PER_TICK	((uint64_t)sys_clock_hw_cycles_per_sec() \
			/ (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC)

static ALWAYS_INLINE void arm_arch_timer_set_compare(uint64_t val)
{
	write_cntp_cval_el0(val);
}

static ALWAYS_INLINE void arm_arch_timer_enable(unsigned char enable)
{
	uint64_t cntp_ctl;

	cntp_ctl = read_cntp_ctl_el0();
	if (enable) {
		cntp_ctl |= CNTP_CTL_ENABLE_BIT;
	} else {
		cntp_ctl &= ~CNTP_CTL_ENABLE_BIT;
	}
	write_cntp_ctl_el0(cntp_ctl);
}

static ALWAYS_INLINE void arm_arch_timer_set_irq_mask(bool mask)
{
	uint64_t cntp_ctl;

	cntp_ctl = read_cntp_ctl_el0();
	if (mask) {
		cntp_ctl |= CNTP_CTL_IMASK_BIT;
	} else {
		cntp_ctl &= ~CNTP_CTL_IMASK_BIT;
	}
	write_cntp_ctl_el0(cntp_ctl);
}

static ALWAYS_INLINE uint64_t arm_arch_timer_count(void)
{
	return read_cntpct_el0();
}

#else
#define ARM_ARCH_TIMER_IRQ	ARM_TIMER_VIRTUAL_IRQ
#define ARM_ARCH_TIMER_PRIO	ARM_TIMER_VIRTUAL_PRIO
#define ARM_ARCH_TIMER_FLAGS	ARM_TIMER_VIRTUAL_FLAGS

static ALWAYS_INLINE void arm_arch_timer_set_compare(uint64_t val)
{
	write_cntv_cval_el0(val);
}

static ALWAYS_INLINE void arm_arch_timer_enable(unsigned char enable)
{
	uint64_t cntv_ctl;

	cntv_ctl = read_cntv_ctl_el0();

	if (enable) {
		cntv_ctl |= CNTV_CTL_ENABLE_BIT;
	} else {
		cntv_ctl &= ~CNTV_CTL_ENABLE_BIT;
	}

	write_cntv_ctl_el0(cntv_ctl);
}

static ALWAYS_INLINE void arm_arch_timer_set_irq_mask(bool mask)
{
	uint64_t cntv_ctl;

	cntv_ctl = read_cntv_ctl_el0();

	if (mask) {
		cntv_ctl |= CNTV_CTL_IMASK_BIT;
	} else {
		cntv_ctl &= ~CNTV_CTL_IMASK_BIT;
	}

	write_cntv_ctl_el0(cntv_ctl);
}

static ALWAYS_INLINE uint64_t arm_arch_timer_count(void)
{
	return read_cntvct_el0();
}
#endif	/* defined(CONFIG_ZVM) && defined(CONFIG_HAS_ARM_VHE) */

static ALWAYS_INLINE void arm_arch_timer_init(void)
{
#ifdef CONFIG_TIMER_READS_ITS_FREQUENCY_AT_RUNTIME
	extern int z_clock_hw_cycles_per_sec;
	uint64_t cntfrq_el0 = read_cntfrq_el0();

	__ASSERT(cntfrq_el0 < INT_MAX, "cntfrq_el0 cannot fit in system 'int'");
	z_clock_hw_cycles_per_sec = (int) cntfrq_el0;
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_INCLUDE_ARCH_ARM64_TIMER_H_ */
