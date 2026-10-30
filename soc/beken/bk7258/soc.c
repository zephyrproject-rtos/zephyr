/*
 * Copyright (c) 2026 Beken Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * BK7258 SoC-level initialisation.
 *
 * There is little to configure: there are no clock gating or power domain
 * registers to set up, interrupts go straight to the NVIC and are handled by
 * the generic Cortex-M code, and startup assembly, the vector table and the
 * system tick all come from Zephyr.
 */

#include <soc.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(soc_bk7258, CONFIG_SOC_LOG_LEVEL);

/*
 * Core cache control.
 *
 * The core has an instruction cache and it is off out of reset. With it off
 * every instruction fetch goes out to the XIP flash, which is far slower than
 * the core: the whole image runs at a fraction of its speed, and every sleep
 * and timeout wakes up several ticks late. The firmware the device ships with
 * turns the cache on, so an image that owns the chip has to do the same.
 *
 * These are architectural SCB registers at fixed addresses. The CMSIS header
 * used for this SoC describes the core as a plain Cortex-M33, which has no
 * cache and therefore no cache API to call.
 */
#define SOC_SCB_CCR_ADDR     (0xE000ED14U)
#define SOC_SCB_CLIDR_ADDR   (0xE000ED78U)
#define SOC_SCB_ICIALLU_ADDR (0xE000EF50U)

#define SOC_SCB_CLIDR_IC BIT(0)  /* an instruction cache is present */
#define SOC_SCB_CCR_IC   BIT(17) /* instruction cache enable */

static void soc_bk7258_icache_enable(void)
{
	if ((sys_read32(SOC_SCB_CLIDR_ADDR) & SOC_SCB_CLIDR_IC) == 0U) {
		return;
	}

	/* Already on when the firmware the device ships with started this image. */
	if ((sys_read32(SOC_SCB_CCR_ADDR) & SOC_SCB_CCR_IC) != 0U) {
		return;
	}

	__DSB();
	__ISB();
	sys_write32(0U, SOC_SCB_ICIALLU_ADDR);
	__DSB();
	__ISB();
	sys_set_bits(SOC_SCB_CCR_ADDR, SOC_SCB_CCR_IC);
	__DSB();
	__ISB();
}

/*
 * Clock tree.
 *
 * Out of reset both the core and the flash controller run straight off the
 * 26 MHz crystal. Every instruction fetch then waits on the XIP flash at that
 * clock, so the core spends most of its cycles waiting for code: a single tick
 * interrupt that walks the timeout list and switches threads takes milliseconds,
 * and sleeps and time slices wake up ticks late.
 *
 * The vendor boot flow instead runs the flash controller at 480/6 = 80 MHz and
 * the core at 480/4 = 120 MHz, both divided down from the 480 MHz PLL.
 * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC has to match the core frequency, and the
 * system tick runs off the core clock, so this has to happen before the timer
 * driver is initialised.
 */

/* Crude busy wait, for the delays that only need to be "long enough". */
static void soc_bk7258_delay(volatile uint32_t loops)
{
	while (loops-- != 0U) {
	}
}

/*
 * The boot ROM leaves the PLL configured but switched off and in its test
 * configuration. Turn it on, flip the VCO control bits the way the vendor boot
 * flow does, then calibrate.
 */
static void soc_bk7258_dpll_enable(void)
{
	uint32_t reg0;

	sys_set_bits(SOC_SYS_ANA_REG5_ADDR, SOC_SYS_ANA_REG5_EN_DPLL);

	reg0 = sys_read32(SOC_SYS_ANA_REG0_ADDR);
	reg0 &= ~SOC_SYS_ANA_REG0_DPLL_TSTEN;
	reg0 |= SOC_SYS_ANA_REG0_BAND1 | SOC_SYS_ANA_REG0_BANDMANUAL |
		SOC_SYS_ANA_REG0_VCTRL_DPLLLDO;
	sys_write32(reg0, SOC_SYS_ANA_REG0_ADDR);

	sys_set_bits(SOC_SYS_ANA_REG0_ADDR, SOC_SYS_ANA_REG0_DSPTRIG);
	sys_clear_bits(SOC_SYS_ANA_REG0_ADDR, SOC_SYS_ANA_REG0_DSPTRIG);

	sys_clear_bits(SOC_SYS_ANA_REG0_ADDR, SOC_SYS_ANA_REG0_SPITRIG);
	soc_bk7258_delay(120U);
	sys_set_bits(SOC_SYS_ANA_REG0_ADDR, SOC_SYS_ANA_REG0_SPITRIG);
	sys_clear_bits(SOC_SYS_ANA_REG0_ADDR, SOC_SYS_ANA_REG0_SPIDETEN);
	soc_bk7258_delay(3400U);
	sys_set_bits(SOC_SYS_ANA_REG0_ADDR, SOC_SYS_ANA_REG0_SPIDETEN);
	soc_bk7258_delay(3400U);
}

static void soc_bk7258_clock_init(void)
{
	uint32_t mode1 = sys_read32(SOC_SYS_CPU_CLK_DIV_MODE1_ADDR);
	uint32_t mode2 = sys_read32(SOC_SYS_CPU_CLK_DIV_MODE2_ADDR);

	/* Anything but the crystal means someone else already set the tree up. */
	if ((mode1 & SOC_SYS_CLKSEL_CORE_MASK) != 0U) {
		return;
	}

	soc_bk7258_dpll_enable();

	/* Flash controller first: it is the fetch path this code runs from. */
	mode2 &= ~(SOC_SYS_CKDIV_FLASH_MASK | SOC_SYS_CKSEL_FLASH_MASK);
	mode2 |= SOC_SYS_CKDIV_FLASH_80M | SOC_SYS_CKSEL_FLASH_PLL;
	sys_write32(mode2, SOC_SYS_CPU_CLK_DIV_MODE2_ADDR);
	soc_bk7258_delay(2000U);

	/*
	 * Core: select the divider while still on the crystal, wait for it to
	 * settle, then switch the source.
	 */
	mode1 &= ~SOC_SYS_CLKDIV_CORE_MASK;
	mode1 |= SOC_SYS_CLKDIV_CORE_120M;
	sys_write32(mode1, SOC_SYS_CPU_CLK_DIV_MODE1_ADDR);
	soc_bk7258_delay(10000U);

	mode1 = sys_read32(SOC_SYS_CPU_CLK_DIV_MODE1_ADDR);
	mode1 &= ~SOC_SYS_CLKSEL_CORE_MASK;
	mode1 |= SOC_SYS_CLKSEL_CORE_PLL;
	sys_write32(mode1, SOC_SYS_CPU_CLK_DIV_MODE1_ADDR);

	/* Everything after this point is fetched at the new clock. */
	__DSB();
	__ISB();
	soc_bk7258_delay(10000U);
}

static int soc_bk7258_early_init(void)
{
	uint32_t reg;

	/* Before anything else, so that the rest of this runs with the cache on. */
	soc_bk7258_icache_enable();

	/* Then the clocks, so that everything runs at full speed. */
	soc_bk7258_clock_init();

	reg = sys_read32(SOC_SYS_CPU_CLK_DIV_MODE1_ADDR);

	/*
	 * The console port's clock gate is off when this image owns the chip:
	 * the firmware the device ships with turns it on. Enabling it here when
	 * it is already on changes nothing.
	 */
	if (!IS_ENABLED(CONFIG_SOC_BK7258_VENDOR_FIRMWARE)) {
		sys_set_bits(SOC_SYS_CPU_DEVICE_CLK_ENABLE_ADDR, SOC_SYS_UART0_CKEN_BIT);
	}

	/*
	 * Report that this image is up before anything else runs: the peer waits
	 * only 2 s for that indication after releasing it, so the call must not
	 * sit behind a later init step that could stall.
	 *
	 * The periodic heartbeat that follows comes from soc_ipc.c.
	 */
	if (IS_ENABLED(CONFIG_SOC_BK7258_IPC_HEARTBEAT)) {
		soc_bk7258_ipc_power_up();
	}

	/*
	 * Print only; no register is touched. If the frequency turns out to be
	 * something else, the DPLL and dividers are configured from here rather
	 * than poked in the log path, so that "does not run at all" and "clock
	 * configured wrong" stay distinguishable.
	 */
	LOG_INF("SYS_CPU_CLK_DIV_MODE1(%#x) = %#010x", SOC_SYS_CPU_CLK_DIV_MODE1_ADDR, reg);
	LOG_INF("  clkdiv_core=%u cksel_core=%u clkdiv_bus=%u", reg & 0xFU, (reg >> 4) & 0x3U,
		(reg >> 6) & 0x1U);
	LOG_INF("  clkdiv_uart0=%u clksel_uart0=%u (0=XTAL26M 1=APLL)",
		(reg >> SOC_SYS_CLKDIV_UART0_SHIFT) & 0x3U,
		/* BIT() is 1UL, so narrow explicitly: a bare %u trips -Wformat. */
		(unsigned int)((reg & SOC_SYS_CLKSEL_UART0_BIT) >> 10));
	LOG_INF("expected: clksel_uart0=0 (XTAL) and a CPU frequency that makes "
		"CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC=%u hold",
		CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC);

	return 0;
}

SYS_INIT(soc_bk7258_early_init, PRE_KERNEL_1, 0);
