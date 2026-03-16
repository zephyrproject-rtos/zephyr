/*
 * SPDX-FileCopyrightText: Copyright Bavariamatic GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SmartFusion2 M2S010 early SoC initialization.
 */

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>
#include <soc.h>

uint32_t SystemCoreClock = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

/**
 * @brief Compute a system register address from its block-relative offset.
 *
 * @param offset Register offset from the SYSREG base.
 *
 * @return Absolute memory-mapped register address.
 */
static inline mem_addr_t smartfusion2_sysreg_reg(uint32_t offset)
{
	return SMARTFUSION2_SYSREG_BASE + offset;
}

/**
 * @brief Perform SmartFusion2 device-specific early initialization.
 */
void SystemInit(void)
{
	uint32_t facc1_cr;
	uint32_t device_version;

	SCB->CCR |= SCB_CCR_STKALIGN_Msk;

	device_version = sys_read32(
		smartfusion2_sysreg_reg(SMARTFUSION2_SYSREG_DEVICE_VERSION_OFFSET));
	if (device_version != SMARTFUSION2_M2S050_REV_A_DEVICE_VERSION) {
		return;
	}

	facc1_cr = sys_read32(
		smartfusion2_sysreg_reg(SMARTFUSION2_SYSREG_MSSDDR_FACC1_CR_OFFSET));
	facc1_cr |= BIT(SMARTFUSION2_DDR_CLK_EN_SHIFT);
	facc1_cr &= ~SMARTFUSION2_CONTROLLER_PLL_INIT_MASK;
	sys_write32(facc1_cr,
		smartfusion2_sysreg_reg(SMARTFUSION2_SYSREG_MSSDDR_FACC1_CR_OFFSET));
}

/**
 * @brief Update the exported system core clock value.
 */
void SystemCoreClockUpdate(void)
{
	SystemCoreClock = CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;
}

/**
 * @brief Release resets for peripherals enabled in the devicetree.
 */
static void smartfusion2_release_peripheral_resets(void)
{
	uint32_t reset_mask = 0U;

	sys_write32(0U,
		smartfusion2_sysreg_reg(SMARTFUSION2_SYSREG_WDOG_CR_OFFSET));

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

/**
 * @brief Run the Zephyr SoC early initialization hook.
 */
void soc_early_init_hook(void)
{
	SystemInit();
	SystemCoreClockUpdate();
	smartfusion2_release_peripheral_resets();
}