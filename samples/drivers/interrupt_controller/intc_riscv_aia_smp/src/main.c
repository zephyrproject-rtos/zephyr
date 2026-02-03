/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * RISC-V AIA SMP Demo - Dynamic Interrupt Routing
 *
 * Demonstrates AIA's ability to dynamically route interrupts between
 * harts at runtime. Each character typed alternates between harts.
 *
 * Also demonstrates the distinction between:
 * - Local interrupts (timer): handled via mie/mip CSRs directly
 * - APLIC sources (UART): routed through APLIC+IMSIC
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/interrupt_controller/riscv_aia.h>
#include <zephyr/drivers/interrupt_controller/riscv_imsic.h>

/* Get UART configuration from device tree */
#define UART_NODE      DT_CHOSEN(zephyr_console)
#define UART_BASE      DT_REG_ADDR(UART_NODE)
#define UART_REG_SHIFT DT_PROP_OR(UART_NODE, reg_shift, 0)
#define UART_IRQ_NUM   DT_IRQ_BY_IDX(UART_NODE, 0, irq)
#define UART_IRQ_FLAGS DT_IRQ_BY_IDX(UART_NODE, 0, flags)

/* 16550 UART register offsets */
#define UART_RBR (0x00 << UART_REG_SHIFT)
#define UART_THR (0x00 << UART_REG_SHIFT)
#define UART_IER (0x01 << UART_REG_SHIFT)
#define UART_LSR (0x05 << UART_REG_SHIFT)

#define UART_IER_RDI  0x01
#define UART_LSR_DR   0x01
#define UART_LSR_THRE 0x20

static volatile uint32_t uart_isr_count[CONFIG_MP_MAX_NUM_CPUS];

/* Thread data for hart 1 */
static struct k_thread hart1_thread_data;
static K_THREAD_STACK_DEFINE(hart1_stack, 1024);
static K_SEM_DEFINE(hart1_ready, 0, 1);

static void uart_isr(const void *arg)
{
	ARG_UNUSED(arg);
	uint32_t hart = arch_curr_cpu()->id;

	uart_isr_count[hart]++;

	while (sys_read8(UART_BASE + UART_LSR) & UART_LSR_DR) {
		char c = sys_read8(UART_BASE + UART_RBR);

		/* Echo with hart tag */
		printk("[Hart %d] '%c'\n", hart, c);

		/* Route next interrupt to other hart */
		uint32_t next = (hart + 1) % CONFIG_MP_MAX_NUM_CPUS;

		riscv_aia_route_to_hart(UART_IRQ_NUM, next, UART_IRQ_NUM);
	}
}

static void hart1_init_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	/* Enable EIID on this hart's IMSIC */
	riscv_imsic_enable_eiid(UART_IRQ_NUM);
	printk("Hart 1: EIID %d enabled\n", UART_IRQ_NUM);

	k_sem_give(&hart1_ready);

	/* Keep thread alive */
	while (1) {
		k_msleep(5000);
	}
}

int main(void)
{
	printk("\nRISC-V AIA SMP Demo - Dynamic Interrupt Routing\n");
	printk("CPUs: %d, UART IRQ: %d\n\n", CONFIG_MP_MAX_NUM_CPUS, UART_IRQ_NUM);

	/* Standard IRQ setup on hart 0 */
	IRQ_CONNECT(UART_IRQ_NUM, 1, uart_isr, NULL, UART_IRQ_FLAGS);
	irq_enable(UART_IRQ_NUM);
	printk("Hart 0: IRQ %d enabled\n", UART_IRQ_NUM);

	/* Start thread on hart 1 to enable EIID there */
	k_tid_t tid = k_thread_create(&hart1_thread_data, hart1_stack,
				      K_THREAD_STACK_SIZEOF(hart1_stack), hart1_init_thread, NULL,
				      NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_FOREVER);
	k_thread_cpu_mask_clear(tid);
	k_thread_cpu_mask_enable(tid, 1);
	k_thread_start(tid);

	k_sem_take(&hart1_ready, K_FOREVER);

	/* Enable UART RX interrupts */
	sys_write8(UART_IER_RDI, UART_BASE + UART_IER);

	printk("\nType characters - they alternate between harts!\n\n");

	/* Test GENMSI injection to each hart */
	printk("Testing GENMSI injection...\n");
	for (int target_hart = 0; target_hart < CONFIG_MP_MAX_NUM_CPUS; target_hart++) {
		printk("Injecting MSI to hart %d (EIID %d)...\n", target_hart, UART_IRQ_NUM);
		riscv_aia_route_to_hart(UART_IRQ_NUM, target_hart, UART_IRQ_NUM);
		k_msleep(100); /* Let routing take effect */
		riscv_aia_inject_msi(target_hart, UART_IRQ_NUM);
		k_msleep(500); /* Wait for ISR */
		printk("  Hart 0 ISR count: %d, Hart 1 ISR count: %d\n", uart_isr_count[0],
		       uart_isr_count[1]);
	}
	printk("GENMSI test complete.\n\n");

	/*
	 * Demonstrate timer interrupts (local interrupts via mie CSR)
	 * Timer interrupts are handled locally on each hart - they don't
	 * go through APLIC/IMSIC. This shows the distinction between:
	 * - Local interrupts (timer, IRQ 7): mie/mip CSR bits
	 * - External interrupts (UART, via APLIC): routed through IMSIC
	 */
	printk("Monitoring system activity on each hart...\n");
	printk("(Timer IRQ 7 is a LOCAL interrupt - uses mie CSR, not APLIC/IMSIC)\n\n");

	/* Show system uptime advancing - proves timer interrupts are working */
	for (int i = 0; i < 5; i++) {
		int64_t uptime_ms = k_uptime_get();

		printk("Uptime: %lld ms - UART ISRs: Hart0=%d Hart1=%d\n", uptime_ms,
		       uart_isr_count[0], uart_isr_count[1]);
		k_msleep(1000);
	}

	printk("\nDemo complete. Timer interrupts work on all harts via mie CSR.\n");
	printk("UART interrupts route through APLIC+IMSIC with dynamic hart targeting.\n\n");

	while (1) {
		k_msleep(2000);
	}

	return 0;
}
