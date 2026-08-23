/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Trigger backend raising a real asynchronous interrupt through the
 * platform interrupt controller. The per-architecture trigger methods
 * are adapted from subsys/testsuite/include/zephyr/interrupt_util.h,
 * which is the mechanism proven by tests/arch/common/interrupt.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>

#include "trigger.h"

/*
 * Benchmark IRQ line selection. CONFIG_INT_BENCH_IRQ_LINE overrides;
 * otherwise pick a platform-appropriate default.
 */
#if CONFIG_INT_BENCH_IRQ_LINE >= 0
#define BENCH_IRQ_LINE CONFIG_INT_BENCH_IRQ_LINE
#elif defined(CONFIG_GIC)
/*
 * Use an SGI (software generated interrupt) line. SGI 0-2 are used by
 * Zephyr for SMP IPIs and SGI 8-15 may be inaccessible from the
 * Non-Secure state, so use SGI 6.
 */
#define BENCH_IRQ_LINE 6
#elif defined(CONFIG_BOARD_QEMU_CORTEX_M3)
/*
 * QEMU's NVIC model for this board does not implement all
 * CONFIG_NUM_IRQS lines; line 42 is implemented and unused (same line
 * used by tests/arch/common/interrupt).
 */
#define BENCH_IRQ_LINE 42
#else
#define BENCH_IRQ_LINE (CONFIG_NUM_IRQS - 1)
#endif

static bench_trigger_handler_t trigger_handler;

void bench_trigger_isr(const void *arg)
{
	ARG_UNUSED(arg);

	if (trigger_handler != NULL) {
		trigger_handler();
	}
}

void bench_trigger_set_handler(bench_trigger_handler_t handler)
{
	trigger_handler = handler;
}

unsigned int bench_trigger_irq_line(void)
{
	return BENCH_IRQ_LINE;
}

#if defined(CONFIG_CPU_CORTEX_M)
#include <cmsis_core.h>

void bench_trigger(void)
{
#if defined(CONFIG_SOC_TI_LM3S6965_QEMU) || defined(CONFIG_CPU_CORTEX_M0) ||                       \
	defined(CONFIG_CPU_CORTEX_M0PLUS) || defined(CONFIG_CPU_CORTEX_M1) ||                      \
	defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	/* QEMU does not simulate the STIR register: this is a workaround */
	NVIC_SetPendingIRQ(BENCH_IRQ_LINE);
#else
	NVIC->STIR = BENCH_IRQ_LINE;
#endif
}

#elif defined(CONFIG_GIC)
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/dt-bindings/interrupt-controller/arm-gic.h>

void bench_trigger(void)
{
#if CONFIG_GIC_VER <= 2
	sys_write32(GICD_SGIR_TGTFILT_REQONLY | GICD_SGIR_SGIINTID(BENCH_IRQ_LINE), GICD_SGIR);
#else
	uint64_t mpidr = GET_MPIDR();
	uint8_t aff0 = MPIDR_AFFLVL(mpidr, 0);

	gic_raise_sgi(BENCH_IRQ_LINE, mpidr, BIT(aff0));
#endif
}

#elif defined(CONFIG_ARC)

void bench_trigger(void)
{
	z_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_HINT, BENCH_IRQ_LINE);
}

#else
#error "No software interrupt trigger available for this architecture"
#endif

int bench_trigger_init(void)
{
	IRQ_CONNECT(BENCH_IRQ_LINE, CONFIG_INT_BENCH_IRQ_PRIO, bench_trigger_isr, NULL, 0);
	irq_enable(BENCH_IRQ_LINE);

	return 0;
}
