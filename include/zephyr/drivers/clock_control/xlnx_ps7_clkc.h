/*
 * Xilinx Zynq-7000 (XC7Zxxx/XC7ZxxxS) PS7 clock control driver
 *
 * Copyright (c) 2023 Immo Birnbaum
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_XLNX_PS7_CLKC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_XLNX_PS7_CLKC_H_

#include <stdint.h>

/**
 * @brief Clock source PLL enumeration.
 *
 * Enumeration type containing the supported clock sources for peripherals
 * within the Zynq-7000's Processor System. The values must match those of
 * the SRCSEL bits in the peripherals' configuration registers. For the IO
 * PLL and EMIO, alternate bit masks exists. Whenever those are encountered
 * during a register read, the driver converts them to the primary value,
 * therefore, those alternate values are unsupported within this enum.
 */
enum xlnx_zynq_ps7_clkc_clock_source_pll {
	xlnx_zynq_clk_source_io_pll = 0,
	xlnx_zynq_clk_source_arm_pll = 2,
	xlnx_zynq_clk_source_ddr_pll = 3,
	xlnx_zynq_clk_source_emio_clk = 4
};

/**
 * @brief Clock identification enumeration.
 *
 * Enumeration type containing the all the clocks that are supported by the
 * Zynq SLCR Clock Control driver. The clock names and order must match those
 * contained in the clock-output-names array in the Zynq-7000's DTSI file.
 */
enum xlnx_zynq_ps7_clkc_clock_identifier {
	xlnx_zynq_clk_armpll = 0,
	xlnx_zynq_clk_ddrpll,
	xlnx_zynq_clk_iopll,
	xlnx_zynq_clk_cpu_6or4x,
	xlnx_zynq_clk_cpu_3or2x,
	xlnx_zynq_clk_cpu_2x,
	xlnx_zynq_clk_cpu_1x,
	xlnx_zynq_clk_ddr2x,
	xlnx_zynq_clk_ddr3x,
	xlnx_zynq_clk_dci,
	xlnx_zynq_clk_lqspi,
	xlnx_zynq_clk_smc,
	xlnx_zynq_clk_pcap,
	xlnx_zynq_clk_gem0,
	xlnx_zynq_clk_gem1,
	xlnx_zynq_clk_fclk0,
	xlnx_zynq_clk_fclk1,
	xlnx_zynq_clk_fclk2,
	xlnx_zynq_clk_fclk3,
	xlnx_zynq_clk_can0,
	xlnx_zynq_clk_can1,
	xlnx_zynq_clk_sdio0,
	xlnx_zynq_clk_sdio1,
	xlnx_zynq_clk_uart0,
	xlnx_zynq_clk_uart1,
	xlnx_zynq_clk_spi0,
	xlnx_zynq_clk_spi1,
	xlnx_zynq_clk_dma,
	xlnx_zynq_clk_usb0_aper,
	xlnx_zynq_clk_usb1_aper,
	xlnx_zynq_clk_gem0_aper,
	xlnx_zynq_clk_gem1_aper,
	xlnx_zynq_clk_sdio0_aper,
	xlnx_zynq_clk_sdio1_aper,
	xlnx_zynq_clk_spi0_aper,
	xlnx_zynq_clk_spi1_aper,
	xlnx_zynq_clk_can0_aper,
	xlnx_zynq_clk_can1_aper,
	xlnx_zynq_clk_i2c0_aper,
	xlnx_zynq_clk_i2c1_aper,
	xlnx_zynq_clk_uart0_aper,
	xlnx_zynq_clk_uart1_aper,
	xlnx_zynq_clk_gpio_aper,
	xlnx_zynq_clk_lqspi_aper,
	xlnx_zynq_clk_smc_aper,
	xlnx_zynq_clk_swdt,
	xlnx_zynq_clk_dbg_trc,
	xlnx_zynq_clk_dbg_apb
};

/**
 * @brief Clock configuration specification for a Zynq-7000 Processor System peripheral.
 *
 * Contains all the configuration data required to perform a full peripheral clock
 * setup via the SLCR registers within the configure function of the driver's API.
 */
struct xlnx_zynq_ps7_clkc_clock_control_configuration {
	enum xlnx_zynq_ps7_clkc_clock_source_pll source_pll;
	uint32_t emio_clock_frequency;
	uint32_t divisor1;
	uint32_t divisor0;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_XLNX_PS7_CLKC_H_ */
