/*
 * SPDX-FileCopyrightText: Copyright Bavariamatic GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SmartFusion2 M2S010 early SoC initialization.
 */

#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/clock/microchip-smartfusion2-clock.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <soc.h>

#define SMARTFUSION2_CPU_CLOCK_FREQ DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency)

BUILD_ASSERT(SMARTFUSION2_CPU_CLOCK_FREQ ==
	     DT_PROP_BY_IDX(DT_NODELABEL(clkc), clock_frequencies, SMARTFUSION2_CLOCK_CPU),
	     "cpu0 clock-frequency must match the SmartFusion2 clock controller CPU output");

uint32_t SystemCoreClock = SMARTFUSION2_CPU_CLOCK_FREQ;

static inline mem_addr_t smartfusion2_sysreg_reg(uint32_t offset)
{
	return SMARTFUSION2_SYSREG_BASE + offset;
}

static void smartfusion2_init(void)
{
	uint32_t facc1_cr;

	SCB->CCR |= SCB_CCR_STKALIGN_Msk;

	facc1_cr = sys_read32(
		smartfusion2_sysreg_reg(SMARTFUSION2_SYSREG_MSSDDR_FACC1_CR_OFFSET));
	facc1_cr |= BIT(SMARTFUSION2_DDR_CLK_EN_SHIFT);
	facc1_cr &= ~SMARTFUSION2_CONTROLLER_PLL_INIT_MASK;
	sys_write32(facc1_cr,
		smartfusion2_sysreg_reg(SMARTFUSION2_SYSREG_MSSDDR_FACC1_CR_OFFSET));
}

static void smartfusion2_release_peripheral_resets(void)
{
	uint32_t reset_mask = 0U;

	if (!DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(watchdog))) {
		sys_write32(0U,
			smartfusion2_sysreg_reg(SMARTFUSION2_SYSREG_WDOG_CR_OFFSET));
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dma))) {
		reset_mask |= SMARTFUSION2_SYSREG_PDMA_SOFTRESET_MASK;
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(timers))) {
		reset_mask |= SMARTFUSION2_SYSREG_TIMER_SOFTRESET_MASK;
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(uart0))) {
		reset_mask |= SMARTFUSION2_SYSREG_MMUART0_SOFTRESET_MASK;
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(uart1))) {
		reset_mask |= SMARTFUSION2_SYSREG_MMUART1_SOFTRESET_MASK;
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(spi0))) {
		reset_mask |= SMARTFUSION2_SYSREG_SPI0_SOFTRESET_MASK;
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(spi1))) {
		reset_mask |= SMARTFUSION2_SYSREG_SPI1_SOFTRESET_MASK;
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(i2c0))) {
		reset_mask |= SMARTFUSION2_SYSREG_I2C0_SOFTRESET_MASK;
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(i2c1))) {
		reset_mask |= SMARTFUSION2_SYSREG_I2C1_SOFTRESET_MASK;
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(can0))) {
		reset_mask |= SMARTFUSION2_SYSREG_CAN_SOFTRESET_MASK;
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpio0))) {
		reset_mask |= SMARTFUSION2_SYSREG_GPIO_SOFTRESET_MASK |
			SMARTFUSION2_SYSREG_GPIO_7_0_SOFTRESET_MASK |
			SMARTFUSION2_SYSREG_GPIO_15_8_SOFTRESET_MASK |
			SMARTFUSION2_SYSREG_GPIO_23_16_SOFTRESET_MASK |
			SMARTFUSION2_SYSREG_GPIO_31_24_SOFTRESET_MASK;
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(eth0))) {
		reset_mask |= SMARTFUSION2_SYSREG_MAC_SOFTRESET_MASK;
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(usb))) {
		reset_mask |= SMARTFUSION2_SYSREG_USB_SOFTRESET_MASK;
	}

	sys_write32(
		sys_read32(smartfusion2_sysreg_reg(SMARTFUSION2_SYSREG_SOFT_RST_CR_OFFSET)) &
			~reset_mask,
		smartfusion2_sysreg_reg(SMARTFUSION2_SYSREG_SOFT_RST_CR_OFFSET));
}

void soc_early_init_hook(void)
{
	smartfusion2_init();
	smartfusion2_release_peripheral_resets();
}