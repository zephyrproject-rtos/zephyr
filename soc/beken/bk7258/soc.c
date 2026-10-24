/*
 * Copyright (c) 2026 Beken Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BK7258 SoC-level initialisation, deliberately kept minimal.
 *
 * Why there is almost no code here:
 *   1. This core has no peripheral clock-gating or power-domain registers of
 *      its own: the vendor power_ll_pwr_up_uart()/timer()/pwm() are all empty
 *      on the BK7258, so there is no "power up" logic to write.
 *   2. Interrupts go straight to the NVIC, which the generic Armv8-M port
 *      already handles; no SoC-level interrupt controller driver is needed.
 *   3. Startup assembly, the vector table and the system tick are all handled
 *      by the generic Zephyr code.
 *
 * This file therefore only provides a diagnostic interface so that
 * hello_world can print the actual clock register values at boot.
 */

#include <soc.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(soc_bk7258, CONFIG_SOC_BK7258_LOG_LEVEL);

uint32_t soc_bk7258_read_clk_div_mode1(void)
{
	return sys_read32(SOC_SYS_CPU_CLK_DIV_MODE1_ADDR);
}

void soc_bk7258_decode_uart0_clk(uint32_t reg, bool *use_apll, uint32_t *uart0_div)
{
	if (use_apll != NULL) {
		*use_apll = (reg & SOC_SYS_CLKSEL_UART0_BIT) != 0U;
	}
	if (uart0_div != NULL) {
		*uart0_div = (reg >> SOC_SYS_CLKDIV_UART0_SHIFT) & 0x3U;
	}
}

static int soc_bk7258_early_init(void)
{
	uint32_t reg = soc_bk7258_read_clk_div_mode1();

	/*
	 * The very first thing to do is tell the system controller that this core is up, before any
	 * of the printing below.
	 *
	 * The system controller waits only 2 s after releasing this core
	 * (mb_ipc_heartbeat.c:217) and then prints \"IPC retry to start core1\";
	 * 6 s without a heartbeat later
	 * it hits BK_ASSERT and resets the chip. Doing this on the first line of
	 * PRE_KERNEL_1 keeps it out of reach of any later init step (UART, log
	 * backend) that might stall and push it past the 2 s window.
	 *
	 * The heartbeat from then on comes from the k_timer in soc_ipc.c.
	 */
	if (IS_ENABLED(CONFIG_SOC_BK7258_IPC_HEARTBEAT)) {
		soc_bk7258_ipc_power_up();
	}

	/*
	 * Print only; no register is touched.
	 *
	 * Deliberate trade-off: the job here is to get the chain running, not to
	 * tune the clocks. If the core turns out not to run at 120 MHz, the fix is
	 * to configure the DPLL and the dividers explicitly, not to poke registers
	 * from a diagnostic.
	 */
	LOG_INF("SYS_CPU_CLK_DIV_MODE1(%#x) = %#010x", SOC_SYS_CPU_CLK_DIV_MODE1_ADDR, reg);
	LOG_INF("  clkdiv_core=%u cksel_core=%u clkdiv_bus=%u", reg & 0xFU, (reg >> 4) & 0x3U,
		(reg >> 6) & 0x1U);
	LOG_INF("  clkdiv_uart0=%u clksel_uart0=%u (0=XTAL26M 1=APLL)",
		(reg >> SOC_SYS_CLKDIV_UART0_SHIFT) & 0x3U,
		/* SOC_SYS_CLKSEL_UART0_BIT is BIT(10), i.e. 1UL, so the expression
		 * is unsigned long and -Wformat wants the cast.
		 */
		(unsigned int)((reg & SOC_SYS_CLKSEL_UART0_BIT) >> 10));
	LOG_INF("expected: clksel_uart0=0 (XTAL) and a CPU frequency that makes "
		"CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC=%u hold",
		CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	return 0;
}

SYS_INIT(soc_bk7258_early_init, PRE_KERNEL_1, 0);
