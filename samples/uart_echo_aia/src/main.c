/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * UART Echo Demo using APLIC → IMSIC MSI Delivery
 *
 * Platform-agnostic UART echo demo demonstrating AIA interrupt delivery.
 * Works on both QEMU and nSIM by reading configuration from device tree.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/interrupt_controller/riscv_aia.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/devicetree.h>

/* Get UART configuration from device tree */
#define UART_NODE      DT_CHOSEN(zephyr_console)
#define UART_BASE      DT_REG_ADDR(UART_NODE)
#define UART_REG_SHIFT DT_PROP_OR(UART_NODE, reg_shift, 0)

/* Get interrupt configuration from device tree */
#define UART_IRQ_NUM DT_IRQ_BY_IDX(UART_NODE, 0, irq)

/*
 * EIID (External Interrupt ID) to use for UART interrupt.
 * We use EIID 32, which is within the valid range (0-2047) for all AIA
 * implementations. The APLIC routes the UART interrupt source to this EIID.
 */
#define UART_EIID 32

/* 16550 UART register offsets (adjusted for reg-shift) */
#define UART_RBR (0x00 << UART_REG_SHIFT) /* Receiver Buffer Register (read) */
#define UART_THR (0x00 << UART_REG_SHIFT) /* Transmitter Holding Register (write) */
#define UART_IER (0x01 << UART_REG_SHIFT) /* Interrupt Enable Register */
#define UART_LSR (0x05 << UART_REG_SHIFT) /* Line Status Register */

/* UART_IER bits */
#define UART_IER_RDI 0x01 /* Receiver Data Interrupt */

/* UART_LSR bits */
#define UART_LSR_DR 0x01 /* Data Ready */

static inline uint8_t uart_read_reg(uint32_t offset)
{
	return sys_read8(UART_BASE + offset);
}

static inline void uart_write_reg(uint32_t offset, uint8_t value)
{
	sys_write8(value, UART_BASE + offset);
}

/* Buffer to collect characters */
#define BUFFER_SIZE 10
static char buffer[BUFFER_SIZE];
static volatile int buffer_count;
static volatile int isr_count;

/* UART ISR - buffers characters and prints when buffer is full */
static void uart_isr(const void *arg)
{
	ARG_UNUSED(arg);
	isr_count++;
	printk("[ISR #%d] UART interrupt triggered!\n", isr_count);

	/* Read all available characters */
	uint8_t lsr = uart_read_reg(UART_LSR);

	while (lsr & UART_LSR_DR) {
		uint8_t c = uart_read_reg(UART_RBR);

		/* Add to buffer */
		buffer[buffer_count++] = c;

		/* When buffer is full, print all characters */
		if (buffer_count >= BUFFER_SIZE) {
			printk("Received: ");
			for (int i = 0; i < BUFFER_SIZE; i++) {
				printk("%c", buffer[i]);
			}
			printk("\n");
			buffer_count = 0;
		}

		lsr = uart_read_reg(UART_LSR);
	}
}

int main(void)
{
	const struct device *dev = riscv_aia_get_dev();

	if (!dev) {
		printk("ERROR: AIA device not found\n");
		return -1;
	}

	/* Setup interrupt routing */
	IRQ_CONNECT(UART_EIID, 1, uart_isr, NULL, 0);
	irq_enable(UART_EIID);

	riscv_aia_config_source(UART_IRQ_NUM, APLIC_SM_LEVEL_HIGH);
	riscv_aia_route_to_hart(UART_IRQ_NUM, 0, UART_EIID);
	riscv_aia_enable_source(UART_IRQ_NUM);

	/* Enable UART RX interrupts */
	uart_write_reg(UART_IER, UART_IER_RDI);

	printk("Platform-agnostic UART Echo Demo\n");
	printk("UART base: 0x%08x, IRQ: %d, EIID: %d, reg-shift: %d\n", (uint32_t)UART_BASE,
	       UART_IRQ_NUM, UART_EIID, UART_REG_SHIFT);
	printk("APLIC configuration:\n");
	printk("  - Source %d configured for level-high\n", UART_IRQ_NUM);
	printk("  - Source %d routed to EIID %d\n", UART_IRQ_NUM, UART_EIID);
	printk("  - Source %d enabled\n", UART_IRQ_NUM);
	printk("  - EIID %d enabled in IMSIC\n", UART_EIID);
	printk("  - UART IER register set to 0x%02x\n", UART_IER_RDI);
	printk("UART echo ready - type 10 characters to see echo\n");

	/* Idle loop */
	while (1) {
		k_msleep(1000);
	}

	return 0;
}
