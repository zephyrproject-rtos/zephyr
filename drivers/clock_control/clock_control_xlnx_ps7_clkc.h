/*
 * Xilinx Zynq-7000 (XC7Zxxx/XC7ZxxxS) PS7 clock control driver
 *
 * Copyright (c) 2023 Immo Birnbaum
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_XLNX_PS7_CLKC_H_
#define ZEPHYR_DRIVERS_XLNX_PS7_CLKC_H_

#include <zephyr/drivers/clock_control/xlnx_ps7_clkc.h>

#define DT_DRV_COMPAT xlnx_ps7_clkc

/*
 * Offsets of the clock-related registers within the SLCR register space
 * Comp. Zynq-7000 TRM, chap. B.28.
 * Neither the abs. base address of the SLCR register space nor the 0x100
 * relative offset within the SLCR register space are required here, this
 * is all handled in the DT data due to the association with the SYSCON
 * driver.
 */

#define ARM_PLL_CTRL_OFFSET		0x00
#define DDR_PLL_CTRL_OFFSET		0x04
#define IO_PLL_CTRL_OFFSET		0x08
#define PLL_STATUS_OFFSET		0x0C
#define ARM_PLL_CFG_OFFSET		0x10
#define DDR_PLL_CFG_OFFSET		0x14
#define IO_PLL_CFG_OFFSET		0x18
#define ARM_CLK_CTRL_OFFSET		0x20
#define DDR_CLK_CTRL_OFFSET		0x24
#define DCI_CLK_CTRL_OFFSET		0x28
#define APER_CLK_CTRL_OFFSET		0x2C
#define USB0_CLK_CTRL_OFFSET		0x30
#define USB1_CLK_CTRL_OFFSET		0x34
#define GEM0_RCLK_CTRL_OFFSET		0x38
#define GEM1_RCLK_CTRL_OFFSET		0x3C
#define GEM0_CLK_CTRL_OFFSET		0x40
#define GEM1_CLK_CTRL_OFFSET		0x44
#define SMC_CLK_CTRL_OFFSET		0x48
#define LQSPI_CLK_CTRL_OFFSET		0x4C
#define SDIO_CLK_CTRL_OFFSET		0x50
#define UART_CLK_CTRL_OFFSET		0x54
#define SPI_CLK_CTRL_OFFSET		0x58
#define CAN_CLK_CTRL_OFFSET		0x5C
#define CAN_MIOCLK_CTRL_OFFSET		0x60
#define DBG_CLK_CTRL_OFFSET		0x64
#define PCAP_CLK_CTRL_OFFSET		0x68
#define TOPSW_CLK_CTRL_OFFSET		0x6C
#define FPGA0_CLK_CTRL_OFFSET		0x70
#define FPGA0_THR_CTRL_OFFSET		0x74
#define FPGA0_THR_CNT_OFFSET		0x78
#define FPGA0_THR_STA_OFFSET		0x7C
#define FPGA1_CLK_CTRL_OFFSET		0x80
#define FPGA1_THR_CTRL_OFFSET		0x84
#define FPGA1_THR_CNT_OFFSET		0x88
#define FPGA1_THR_STA_OFFSET		0x8C
#define FPGA2_CLK_CTRL_OFFSET		0x90
#define FPGA2_THR_CTRL_OFFSET		0x94
#define FPGA2_THR_CNT_OFFSET		0x98
#define FPGA2_THR_STA_OFFSET		0x9C
#define FPGA3_CLK_CTRL_OFFSET		0xA0
#define FPGA3_THR_CTRL_OFFSET		0xA4
#define FPGA3_THR_CNT_OFFSET		0xA8
#define FPGA3_THR_STA_OFFSET		0xAC
#define CLK_621_TRUE_OFFSET		0xC4

#define PLL_FDIV_SHIFT			12
#define PLL_FDIV_MASK			0x7F
#define PLL_RESET_BIT			BIT(0)
#define PLL_PWRDOWN_BIT			BIT(1)
#define PLL_BYPASS_FORCE_BIT		BIT(4)
#define CLK_SCHEME_621_SHIFT		0
#define CLK_SCHEME_621_MASK		0x1
#define ARM_CPU1X_ACTIVE_SHIFT		27
#define ARM_CPU2X_ACTIVE_SHIFT		26
#define ARM_CPU3X2X_ACTIVE_SHIFT	25
#define ARM_CPU6X4X_ACTIVE_SHIFT	24
#define ARM_CLK_ACTIVE_MASK		0x1
#define ARM_CLK_DIVISOR_SHIFT		8
#define ARM_CLK_DIVISOR_MASK		0x3F
#define ARM_CLK_SOURCE_SHIFT		4
#define ARM_CLK_SOURCE_MASK		0x3
#define ARM_CLK_SOURCE_ARM_PLL		0x0
#define ARM_CLK_SOURCE_ARM_PLL_ALT	0x1
#define ARM_CLK_SOURCE_DDR_PLL		0x2
#define ARM_CLK_SOURCE_IO_PLL		0x3

#define DDR_DDR2X_CLK_DIVISOR_SHIFT	26
#define DDR_DDR3X_CLK_DIVISOR_SHIFT	20
#define DDR_CLK_ACTIVE_MASK		0x1
#define DDR_DDR2X_ACTIVE_SHIFT		1
#define DDR_DDR3X_ACTIVE_SHIFT		0

#define PLL_STATUS_IO_PLL_STABLE_BIT	BIT(5)
#define PLL_STATUS_DDR_PLL_STABLE_BIT	BIT(4)
#define PLL_STATUS_ARM_PLL_STABLE_BIT	BIT(3)
#define PLL_STATUS_IO_PLL_LOCK_BIT	BIT(2)
#define PLL_STATUS_DDR_PLL_LOCK_BIT	BIT(1)
#define PLL_STATUS_ARM_PLL_LOCK_BIT	BIT(0)

#define APER_CLK_CTRL_SMC_CLKACT_BIT	BIT(24)
#define APER_CLK_CTRL_LQSPI_CLKACT_BIT	BIT(23)
#define APER_CLK_CTRL_GPIO_CLKACT_BIT	BIT(22)
#define APER_CLK_CTRL_UART1_CLKACT_BIT	BIT(21)
#define APER_CLK_CTRL_UART0_CLKACT_BIT	BIT(20)
#define APER_CLK_CTRL_I2C1_CLKACT_BIT	BIT(19)
#define APER_CLK_CTRL_I2C0_CLKACT_BIT	BIT(18)
#define APER_CLK_CTRL_CAN1_CLKACT_BIT	BIT(17)
#define APER_CLK_CTRL_CAN0_CLKACT_BIT	BIT(16)
#define APER_CLK_CTRL_SPI1_CLKACT_BIT	BIT(15)
#define APER_CLK_CTRL_SPI0_CLKACT_BIT	BIT(14)
#define APER_CLK_CTRL_SDI1_CLKACT_BIT	BIT(11)
#define APER_CLK_CTRL_SDI0_CLKACT_BIT	BIT(10)
#define APER_CLK_CTRL_GEM1_CLKACT_BIT	BIT(7)
#define APER_CLK_CTRL_GEM0_CLKACT_BIT	BIT(6)
#define APER_CLK_CTRL_USB1_CLKACT_BIT	BIT(3)
#define APER_CLK_CTRL_USB0_CLKACT_BIT	BIT(2)
#define APER_CLK_CTRL_DMA_CLKACT_BIT	BIT(0)

#define PERIPH_CLK_DIVISOR1_SHIFT	20
#define PERIPH_CLK_DIVISOR0_SHIFT	8
#define PERIPH_CLK_DIVISOR_MASK		0x3F
#define PERIPH_CLK_SRCSEL_SHIFT		4
#define PERIPH_CLK_SRCSEL_MASK		0x7
#define PERIPH_CLK_CLKACT1_BIT		BIT(1)
#define PERIPH_CLK_CLKACT0_BIT		BIT(0)

#define GEM_RCLK_SRCSEL_BIT		BIT(4)
#define DBG_APER_CLK_CLKACT_BIT		BIT(1)

#define MAX_TARGET_DEVIATION		20
/*
 * This deviation value is reasonable for the base PLLs' PLL_FDIV value calculation:
 * in real numbers: 33.3 MHz * PLL_FDIV 26 = 866.6 MHz (ARM PLL POR value)
 * in integer arithmetic: 33333333 * 26 = 866666658 delta 8 to ideal int value
 * in real numbers: 33.3 MHz * PLL_FDIV 40 = 1333.3 MHz (ARM PLL Vivado default config)
 * in integer arithmetic: 33333333 * 40 = 1333333320 delta 13 to ideal int value
 * in real numbers: 33.3 MHz * PLL_FDIV 50 = 1666.6 MHz
 * in integer arithmetic: 33333333 * 50 = 1666666650 delta 16 to ideal int value
 */

union xlnx_zynq_ps7_clkc_emio_clock_source {
	struct xlnx_zynq_ps7_clkc_emio_clock_source_explicit {
		uint32_t emio_clk_frequency;
		const enum xlnx_zynq_ps7_clkc_clock_identifier peripheral_clock_id;
		const char *const emio_clk_name;
	} *explicit_config;
	const struct xlnx_zynq_ps7_clkc_emio_clock_source_dt {
		const uint32_t emio_clk_frequency;
		const enum xlnx_zynq_ps7_clkc_clock_identifier peripheral_clock_id;
		const char *const emio_clk_name;
	} *dt_config;
};

struct xlnx_zynq_ps7_clkc_peripheral_clock {
	bool active;
	bool parent_pll_stopped;
	uint32_t divisor1;
	uint32_t divisor0;
	enum xlnx_zynq_ps7_clkc_clock_source_pll source_pll;
	uint32_t clk_frequency;
	const enum xlnx_zynq_ps7_clkc_clock_identifier peripheral_clock_id;
	const char *const clk_name;
	union xlnx_zynq_ps7_clkc_emio_clock_source emio_clock_source;
};

struct xlnx_zynq_ps7_clkc_clock_control_config {
	const struct device *const slcr;
	const mm_reg_t base_address;
	const uint32_t ps_clk_frequency;
	const uint32_t fclk_enable;
	const uint32_t emio_clocks_count;
	const struct xlnx_zynq_ps7_clkc_emio_clock_source_dt emio_clock_sources_dt[];
};

struct xlnx_zynq_ps7_clkc_clock_control_data {
	struct xlnx_zynq_ps7_clkc_peripheral_clock peripheral_clocks[48];
	uint32_t arm_pll_multiplier;
	uint32_t arm_pll_frequency;
	uint32_t ddr_pll_multiplier;
	uint32_t ddr_pll_frequency;
	uint32_t io_pll_multiplier;
	uint32_t io_pll_frequency;
	bool clk_scheme_621;
	bool cpu_1x_active;
	bool cpu_2x_active;
	bool cpu_6x4x_active;
	bool cpu_3x2x_active;
	uint32_t cpu_divisor;
	uint32_t cpu_source_pll;
	uint32_t cpu_6x4x_frequency;
	uint32_t cpu_3x2x_frequency;
	uint32_t cpu_2x_frequency;
	uint32_t cpu_1x_frequency;
	bool ddr_2x_active;
	bool ddr_3x_active;
	uint32_t ddr_2x_frequency;
	uint32_t ddr_3x_frequency;
};

#endif /* ZEPHYR_DRIVERS_XLNX_PS7_CLKC_H_ */
