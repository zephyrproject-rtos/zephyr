/*
 * Copyright (c) 2025 Synopsys, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * UART Echo Demo - AIA with SMP
 *
 * Platform-agnostic UART echo demo demonstrating AIA interrupt delivery
 * in an SMP environment with multiple harts.
 *
 * This demo demonstrates:
 * 1. Both harts booting and running
 * 2. IMSIC per-hart interrupt files
 * 3. APLIC MSI delivery to specific harts
 * 4. GENMSI testing for each hart
 * 5. Dynamic interrupt routing between harts
 *
 * Works on both QEMU and nSIM by reading configuration from device tree.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/riscv_aia.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/arch/cpu.h>
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
#define UART_LSR_DR   0x01 /* Data Ready */
#define UART_LSR_THRE 0x20 /* Transmitter Holding Register Empty */

/* Per-hart statistics */
struct hart_stats {
	volatile uint32_t isr_count;
	volatile uint32_t rx_count;
	volatile uint32_t genmsi_count;
	volatile bool eiid_enabled; /* Track if this hart has enabled the EIID */
} __aligned(64);

static struct hart_stats stats[CONFIG_MP_MAX_NUM_CPUS];
static volatile uint32_t next_target_hart;

/* Semaphore to coordinate hart startup */
K_SEM_DEFINE(hart1_ready, 0, 1);

/* Per-hart heartbeat counters */
static volatile uint32_t hart_heartbeat[CONFIG_MP_MAX_NUM_CPUS];

/* UART helpers */
static inline uint8_t uart_read_reg(uint32_t offset)
{
	return sys_read8(UART_BASE + offset);
}

static inline void uart_write_reg(uint32_t offset, uint8_t value)
{
	sys_write8(value, UART_BASE + offset);
}

/* Thread for hart 1 interrupt initialization */
static struct k_thread hart1_thread_data;
static K_THREAD_STACK_DEFINE(hart1_stack, 4096);

/**
 * Heartbeat thread - runs on specific hart to prove it's executing
 * Pass hart_id as parameter to avoid arch_curr_cpu() issues early in SMP init
 */
static void hart_heartbeat_thread(void *p1, void *p2, void *p3)
{
	uint32_t hart_id = (uint32_t)(uintptr_t)p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	printk("[Hart %d] Heartbeat thread started\n", hart_id);

	/*
	 * The IMSIC driver already initialized this CPU during SMP boot via
	 * z_riscv_imsic_secondary_init(), which sets up EIDELIVERY, EITHRESHOLD,
	 * and enables MEXT. We just need to enable our specific EIID.
	 */
	printk("[Hart %d] Enabling UART EIID %d\n", hart_id, UART_EIID);
	irq_enable(UART_EIID);
	stats[hart_id].eiid_enabled = true;
	printk("[Hart %d] EIID %d enabled, ready\n", hart_id, UART_EIID);

	/* Signal that this hart is ready */
	k_sem_give(&hart1_ready);

	/* Heartbeat loop - increment counter and print periodically */
	while (1) {
		hart_heartbeat[hart_id]++;
		printk("[Hart %d] ♥ Heartbeat %d (ISR count=%d)\n", hart_id,
		       hart_heartbeat[hart_id], stats[hart_id].isr_count);
		k_msleep(2000);
	}
}

/**
 * UART ISR - handles both GENMSI and UART RX with hart-specific output
 */
static void uart_eiid_isr(const void *arg)
{
	ARG_UNUSED(arg);

	uint32_t hart_id = arch_curr_cpu()->id;

	stats[hart_id].isr_count++;

	/* Check if this is a UART interrupt or GENMSI */
	uint8_t lsr = uart_read_reg(UART_LSR);

	if (!(lsr & UART_LSR_DR)) {
		/* No data ready - this was a GENMSI software interrupt */
		/* Print with hart-specific formatting */
		if (hart_id == 0) {
			printk("    [Hart 0] 🔔 GENMSI received! (Software interrupt)\n");
		} else {
			printk("    <Hart 1> 🔔 GENMSI received! (Software interrupt)\n");
		}
		return;
	}

	/* Handle UART RX data */
	while (lsr & UART_LSR_DR) {
		uint8_t c = uart_read_reg(UART_RBR);

		stats[hart_id].rx_count++;

		/* Echo with hart-specific formatting:
		 * Hart 0: [H0]char  (square brackets)
		 * Hart 1: <H1>char  (angle brackets)
		 */
		char buf[20];
		int len;

		if (hart_id == 0) {
			len = snprintk(buf, sizeof(buf), "[H0]%c", c);
		} else {
			len = snprintk(buf, sizeof(buf), "<H1>%c", c);
		}

		for (int i = 0; i < len; i++) {
			uart_write_reg(UART_THR, buf[i]);
			while (!(uart_read_reg(UART_LSR) & UART_LSR_THRE)) {
				/* Wait */
			}
		}

		/* Switch to next hart for next interrupt */
		uint32_t next = (hart_id + 1) % CONFIG_MP_MAX_NUM_CPUS;

		next_target_hart = next;

		riscv_aia_route_to_hart(UART_IRQ_NUM, next, UART_EIID);

		lsr = uart_read_reg(UART_LSR);
	}
}

int main(void)
{
	printk("\n");
	printk("╔════════════════════════════════════════════════╗\n");
	printk("║      UART Echo - AIA SMP Demo (2 Harts)      ║\n");
	printk("║   Interrupts alternate between both harts!    ║\n");
	printk("╚════════════════════════════════════════════════╝\n");
	printk("\n");

	printk("Main thread running on CPU %u\n", arch_curr_cpu()->id);
	printk("  CPUs configured: %d\n", CONFIG_MP_MAX_NUM_CPUS);
	printk("  UART base: 0x%08x, IRQ: %d, EIID: %d, reg-shift: %d\n", (uint32_t)UART_BASE,
	       UART_IRQ_NUM, UART_EIID, UART_REG_SHIFT);
	printk("\n");

	const struct device *dev = riscv_aia_get_dev();

	if (!dev) {
		printk("ERROR: AIA not found\n");
		return -1;
	}

	/* Register ISR (compile-time, all harts share same table) */
	IRQ_CONNECT(UART_EIID, 1, uart_eiid_isr, NULL, 0);

	/* Enable EIID on Hart 0 (this hart) */
	printk("Enabling UART EIID %d on Hart 0...\n", UART_EIID);
	irq_enable(UART_EIID);
	stats[0].eiid_enabled = true;
	printk("✓ Hart 0 ready\n\n");

	printk("═══════════════════════════════════════════════\n");
	printk("  Starting Heartbeat Threads\n");
	printk("═══════════════════════════════════════════════\n\n");

	/* Create heartbeat thread for hart 1 (will enable EIID) */
	printk("Creating heartbeat thread for hart 1...\n");
	k_tid_t hart1_tid = k_thread_create(
		&hart1_thread_data, hart1_stack, K_THREAD_STACK_SIZEOF(hart1_stack),
		hart_heartbeat_thread, (void *)(uintptr_t)1, /* Pass hart_id=1 as parameter */
		NULL, NULL, K_LOWEST_APPLICATION_THREAD_PRIO, 0, K_FOREVER);

	/* Set CPU affinity to CPU 1 only BEFORE starting */
	k_thread_cpu_mask_clear(hart1_tid);
	k_thread_cpu_mask_enable(hart1_tid, 1);

	/* Now start the thread */
	k_thread_start(hart1_tid);
	printk("✓ Thread created and pinned to CPU 1\n");

	/* Wait for hart 1 thread to be ready */
	printk("Waiting for hart 1 to enable EIID...\n");
	k_sem_take(&hart1_ready, K_FOREVER);
	printk("✓ Hart 1 ready\n\n");

	printk("═══════════════════════════════════════════════\n");
	printk("  AIA SMP Configuration\n");
	printk("═══════════════════════════════════════════════\n");
	printk("\n");

	/* Configure AIA routing - initially target Hart 1 */
	printk("Step 1: Configure AIA for UART (source %d → Hart 1, EIID %d)\n", UART_IRQ_NUM,
	       UART_EIID);
	riscv_aia_config_source(UART_IRQ_NUM, APLIC_SM_LEVEL_HIGH);
	riscv_aia_route_to_hart(UART_IRQ_NUM, 1, UART_EIID); /* Route to Hart 1 */
	riscv_aia_enable_source(UART_IRQ_NUM);
	printk("  ✓ Configured\n\n");

	/* Enable UART interrupts */
	printk("Step 2: Enable UART RX interrupts\n");
	uart_write_reg(UART_IER, UART_IER_RDI);
	printk("  ✓ Enabled (IER=0x%02x)\n\n", uart_read_reg(UART_IER));

	/* Step 3: Test GENMSI injection to both harts with showcase */
	printk("Step 3: GENMSI Injection Showcase - Testing both harts\n");
	printk("  Each hart will print a unique message when it handles GENMSI\n");
	printk("\n");

	printk("  → Injecting GENMSI to Hart 0...\n");
	uint32_t pre_isr_h0 = stats[0].isr_count;

	riscv_aia_inject_msi(0, UART_EIID); /* Hart 0, EIID 32 */
	k_msleep(10);

	if (stats[0].isr_count > pre_isr_h0) {
		printk("    ✓ Hart 0 handled GENMSI! [ISR count=%d]\n", stats[0].isr_count);
	} else {
		printk("    ✗ Hart 0 did not handle GENMSI\n");
	}

	printk("\n");
	printk("  → Injecting GENMSI to Hart 1...\n");
	uint32_t pre_isr_h1 = stats[1].isr_count;

	riscv_aia_inject_msi(1, UART_EIID); /* Hart 1, EIID 32 */
	k_msleep(10);

	if (stats[1].isr_count > pre_isr_h1) {
		printk("    ✓ Hart 1 handled GENMSI! <ISR count=%d>\n", stats[1].isr_count);
	} else {
		printk("    ✗ Hart 1 did not handle GENMSI\n");
	}

	printk("\n");
	printk("  💡 Notice: Each hart shows its count with different brackets!\n");
	printk("     Hart 0 uses [square], Hart 1 uses <angle>\n");
	printk("\n");

	/* Start by routing to Hart 0 */
	printk("Step 4: Initial UART routing to Hart 0\n");
	riscv_aia_route_to_hart(UART_IRQ_NUM, 0, UART_EIID);
	printk("  ✓ UART interrupts will start on Hart 0\n");
	printk("\n");
	next_target_hart = 0;

	printk("\n");
	printk("═══════════════════════════════════════════════\n");
	printk("  Ready - Interrupt Flow\n");
	printk("═══════════════════════════════════════════════\n");
	printk("\n");
	printk("  UART RX → APLIC (source %d) → MSI → IMSIC (Hart X)\n", UART_IRQ_NUM);
	printk("  → MEXT → ISR on Hart X\n");
	printk("  → Echo with hart-specific tag:\n");
	printk("     • Hart 0: [H0]char  (square brackets)\n");
	printk("     • Hart 1: <H1>char  (angle brackets)\n");
	printk("  → Route next interrupt to alternate hart\n");
	printk("\n");
	printk("  Type characters - watch them alternate!\n");
	printk("  First char → Hart 0 [H0], Second char → Hart 1 <H1>, etc.\n");
	printk("\n");

	/* Main loop - Hart 0 heartbeat */
	while (1) {
		k_msleep(2000);

		/* Hart 0 heartbeat */
		hart_heartbeat[0]++;
		printk("[Hart 0] ♥ Heartbeat %d (ISR count=%d)\n", hart_heartbeat[0],
		       stats[0].isr_count);
	}

	return 0;
}
