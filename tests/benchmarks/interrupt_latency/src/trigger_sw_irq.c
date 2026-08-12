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
#elif defined(CONFIG_X86)
/*
 * x86 has no CONFIG_NUM_IRQS; IRQ lines are mapped to IDT vectors at
 * build time. Use the same line as tests/arch/common/interrupt, which
 * is known not to collide with the platform's own IRQ assignments.
 */
#define BENCH_IRQ_LINE 27
#elif defined(CONFIG_RISCV) && !defined(CONFIG_CLIC) && !defined(CONFIG_NRFX_CLIC)
/*
 * Without a CLIC, the only interrupt a RISC-V hart can raise on itself
 * is the machine software interrupt, asserted through the CLINT.
 */
#define BENCH_IRQ_LINE RISCV_IRQ_MSOFT
#else
#define BENCH_IRQ_LINE (CONFIG_NUM_IRQS - 1)
#endif

/*
 * Deassert a level triggered benchmark interrupt. Called at the very
 * start of the ISR, before the measurement handler, so that the
 * throughput scenario can reassert the interrupt from within its own
 * handler.
 */
static inline void bench_trigger_ack(void);

static bench_trigger_handler_t trigger_handler;

void bench_trigger_isr(const void *arg)
{
	ARG_UNUSED(arg);

	bench_trigger_ack();

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

#ifdef BENCH_HAS_ALT_LINE
void bench_trigger_alt(void)
{
#if defined(CONFIG_SOC_TI_LM3S6965_QEMU) || defined(CONFIG_CPU_CORTEX_M0) ||                       \
	defined(CONFIG_CPU_CORTEX_M0PLUS) || defined(CONFIG_CPU_CORTEX_M1) ||                      \
	defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	NVIC_SetPendingIRQ(BENCH_IRQ_LINE_ALT);
#else
	NVIC->STIR = BENCH_IRQ_LINE_ALT;
#endif
}
#endif

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

#ifdef BENCH_HAS_ALT_LINE
void bench_trigger_alt(void)
{
#if CONFIG_GIC_VER <= 2
	sys_write32(GICD_SGIR_TGTFILT_REQONLY | GICD_SGIR_SGIINTID(BENCH_IRQ_LINE_ALT), GICD_SGIR);
#else
	uint64_t mpidr = GET_MPIDR();
	uint8_t aff0 = MPIDR_AFFLVL(mpidr, 0);

	gic_raise_sgi(BENCH_IRQ_LINE_ALT, mpidr, BIT(aff0));
#endif
}
#endif

#elif defined(CONFIG_ARC)

void bench_trigger(void)
{
	z_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_HINT, BENCH_IRQ_LINE);
}

#ifdef BENCH_HAS_ALT_LINE
void bench_trigger_alt(void)
{
	z_arc_v2_aux_reg_write(_ARC_V2_AUX_IRQ_HINT, BENCH_IRQ_LINE_ALT);
}
#endif

#elif defined(CONFIG_X86)
#ifdef CONFIG_X2APIC
#include <zephyr/drivers/interrupt_controller/loapic.h>
#define VECTOR_MASK 0xFFU
#else
#include <zephyr/arch/arch_interface.h>
/*
 * LOAPIC ICR value for a self directed IPI: fixed delivery mode,
 * physical destination mode, level assert, edge triggered, no
 * destination shorthand.
 */
#define LOAPIC_ICR_IPI_TEST 0x00004000U
#endif

/*
 * The interrupt is emulated by sending an IPI from the local APIC to
 * the core itself. Note that unlike trigger_irq() in
 * <zephyr/interrupt_util.h>, no delay loop is added after the write:
 * the delay would be counted as interrupt entry latency. Callers spin
 * on a flag set by the ISR instead.
 */
static uint8_t bench_vector;
#ifdef BENCH_HAS_ALT_LINE
static uint8_t bench_vector_alt;
#endif

static inline void bench_send_self_ipi(uint8_t vector)
{
#ifdef CONFIG_X2APIC
	x86_write_x2apic(LOAPIC_SELF_IPI, vector & VECTOR_MASK);
#else
#ifdef CONFIG_SMP
	int cpu_id = arch_curr_cpu()->id;
#else
	int cpu_id = 0;
#endif
	z_loapic_ipi(cpu_id, LOAPIC_ICR_IPI_TEST, vector);
#endif /* CONFIG_X2APIC */
}

void bench_trigger(void)
{
	bench_send_self_ipi(bench_vector);
}

#ifdef BENCH_HAS_ALT_LINE
void bench_trigger_alt(void)
{
	bench_send_self_ipi(bench_vector_alt);
}
#endif

#elif defined(CONFIG_RISCV)
#if defined(CONFIG_CLIC) || defined(CONFIG_NRFX_CLIC)
void riscv_clic_irq_set_pending(uint32_t irq);

void bench_trigger(void)
{
	riscv_clic_irq_set_pending(BENCH_IRQ_LINE);
}

#else
#include <zephyr/arch/riscv/csr.h>

/*
 * Without a CLIC, a hart cannot make one of its own external
 * interrupts pending: PLIC pending bits are driven by the interrupt
 * gateways, and the machine software, timer and external pending bits
 * of the mip CSR are read only. The machine software interrupt is
 * therefore asserted the same way the SMP IPI code does it, by writing
 * this hart's MSIP register in the CLINT.
 *
 * MSIP is level triggered, so the ISR has to deassert it; see
 * bench_trigger_ack() below.
 *
 * Both the CLINT and mhartid are machine mode only, which is why
 * CONFIG_INT_BENCH_TRIGGER_SW_IRQ excludes CONFIG_RISCV_S_MODE.
 */
#define CLINT_NODE DT_NODELABEL(clint)
#if !DT_NODE_EXISTS(CLINT_NODE)
#error "No CLIC and no 'clint' devicetree node: cannot trigger an interrupt"
#endif

#define MSIP_BASE       DT_REG_ADDR_RAW(CLINT_NODE)
#define MSIP(hartid)    ((volatile uint32_t *)MSIP_BASE)[hartid]

#define BENCH_TRIGGER_LEVEL_ACK 1

void bench_trigger(void)
{
	MSIP(csr_read(mhartid)) = 1U;
}

static inline void bench_trigger_ack(void)
{
	MSIP(csr_read(mhartid)) = 0U;
}
#endif /* CONFIG_CLIC || CONFIG_NRFX_CLIC */

#else
#error "No software interrupt trigger available for this architecture"
#endif

#ifndef BENCH_TRIGGER_LEVEL_ACK
static inline void bench_trigger_ack(void)
{
	/* Edge triggered: nothing to deassert */
}
#endif

int bench_trigger_init(void)
{
	IRQ_CONNECT(BENCH_IRQ_LINE, CONFIG_INT_BENCH_IRQ_PRIO, bench_trigger_isr, NULL, 0);

#ifdef CONFIG_X86
	/*
	 * The local APIC is addressed by IDT vector rather than by IRQ
	 * line. Resolve the vector assigned to the benchmark line once,
	 * so that triggering costs no more than the APIC write itself.
	 */
	bench_vector = (uint8_t)Z_IRQ_TO_INTERRUPT_VECTOR(BENCH_IRQ_LINE);
#ifdef BENCH_HAS_ALT_LINE
	bench_vector_alt = (uint8_t)Z_IRQ_TO_INTERRUPT_VECTOR(BENCH_IRQ_LINE_ALT);
#endif
#endif

	irq_enable(BENCH_IRQ_LINE);

	return 0;
}
