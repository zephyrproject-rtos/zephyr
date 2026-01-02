/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * UART Echo Demo using RISC-V AIA (APLIC + IMSIC)
 *
 * Demonstrates that AIA interrupt delivery works transparently with
 * standard Zephyr IRQ APIs. The AIA driver handles all APLIC/IMSIC
 * configuration automatically.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/devicetree.h>

/* Get UART configuration from device tree */
#define UART_NODE      DT_CHOSEN(zephyr_console)
#define UART_BASE      DT_REG_ADDR(UART_NODE)
#define UART_REG_SHIFT DT_PROP_OR(UART_NODE, reg_shift, 0)
#define UART_IRQ_NUM   DT_IRQ_BY_IDX(UART_NODE, 0, irq)
#define UART_IRQ_FLAGS DT_IRQ_BY_IDX(UART_NODE, 0, flags)

/* 16550 UART register offsets */
#define UART_RBR (0x00 << UART_REG_SHIFT)
#define UART_IER (0x01 << UART_REG_SHIFT)
#define UART_LSR (0x05 << UART_REG_SHIFT)

#define UART_IER_RDI 0x01 /* Receiver Data Interrupt */
#define UART_LSR_DR  0x01 /* Data Ready */

static volatile int isr_count;

static void uart_isr(const void *arg)
{
	ARG_UNUSED(arg);
	isr_count++;

	while (sys_read8(UART_BASE + UART_LSR) & UART_LSR_DR) {
		char c = sys_read8(UART_BASE + UART_RBR);

		printk("[ISR #%d] Received: '%c'\n", isr_count, c);
	}
}

int main(void)
{
	printk("RISC-V AIA UART Echo Demo\n");
	printk("UART IRQ %d - using standard Zephyr IRQ APIs\n", UART_IRQ_NUM);

	/* Standard Zephyr IRQ setup - AIA driver handles APLIC/IMSIC config */
	IRQ_CONNECT(UART_IRQ_NUM, 1, uart_isr, NULL, UART_IRQ_FLAGS);
	irq_enable(UART_IRQ_NUM);

	/* Enable UART RX interrupts */
	sys_write8(UART_IER_RDI, UART_BASE + UART_IER);

	printk("Type characters to see interrupt-driven echo\n");

	while (1) {
		k_msleep(1000);
	}

	return 0;
}
