/*
 * Xilinx Zynq-7000 (XC7Zxxx/XC7ZxxxS) PS7 clock control driver
 *
 * Copyright (c) 2024 Immo Birnbaum
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/syscon.h>

#include "clock_control_xlnx_ps7_clkc.h"

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "Enabled instance count for this device type must always be 1");
BUILD_ASSERT(CONFIG_SYSCON_INIT_PRIORITY < CONFIG_CLOCK_CONTROL_INIT_PRIORITY,
	     "syscon init priority must be higher than clkctrl init priority");

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control);

static struct xlnx_zynq_ps7_clkc_emio_clock_source_explicit explicit_config_emio_clock_data[] = {
	{
		.emio_clk_frequency = 0,
		.peripheral_clock_id = xlnx_zynq_clk_gem0,
		.emio_clk_name = "gem0_emio_clk_explicit"
	},
	{
		.emio_clk_frequency = 0,
		.peripheral_clock_id = xlnx_zynq_clk_gem1,
		.emio_clk_name = "gem1_emio_clk_explicit"
	},
	{
		.emio_clk_frequency = 0,
		.peripheral_clock_id = xlnx_zynq_clk_dbg_trc,
		.emio_clk_name = "dbg_trc_emio_clk_explicit"
	}
};

static bool xlnx_zynq_ps7_clkc_calculate_divisors(
	uint32_t source_pll_frequency,
	uint32_t target_frequency,
	uint32_t *resulting_frequency,
	uint32_t *divisor1,
	uint32_t *divisor0,
	bool force_even)
{
	/* No parameter checks - internal function */
	uint32_t freq_tmp = 0;
	uint32_t div0_tmp;
	uint32_t div1_tmp;

	*resulting_frequency = 0;

	LOG_DBG("convert source frequency %u to target frequency %u using %s",
		source_pll_frequency, target_frequency, (divisor1 != NULL) ?
		"both divisors" : "only divisor0");
	LOG_DBG("%s", (force_even) ? "divisor(s) must be even" : "both even and "
		"odd divisor(s) is/are valid");

	if (divisor1 != NULL) {
		/* use both divisors */
		*divisor1 = 1;
		*divisor0 = 1;

		for (div0_tmp = 1; div0_tmp < 64; div0_tmp++) {
			for (div1_tmp = 1; div1_tmp < 64; div1_tmp++) {
				freq_tmp = (source_pll_frequency / div0_tmp) / div1_tmp;
				if (freq_tmp >= (target_frequency - MAX_TARGET_DEVIATION) &&
				    freq_tmp <= (target_frequency + MAX_TARGET_DEVIATION)) {
					break;
				}
			}
			if (freq_tmp >= (target_frequency - MAX_TARGET_DEVIATION) &&
			    freq_tmp <= (target_frequency + MAX_TARGET_DEVIATION)) {
				LOG_DBG("%u / %u / %u = %u", source_pll_frequency, *divisor1,
					*divisor0, freq_tmp);
				*divisor1 = div1_tmp;
				*divisor0 = div0_tmp;
				*resulting_frequency = freq_tmp;
				return true;
			}
		}
	} else {
		/* target peripheral only supports divisor0 */
		*divisor0 = 1;

		for (div0_tmp = (force_even) ? 2 : 1; div0_tmp < 64;
		     (force_even) ? div0_tmp += 2 : div0_tmp++) {
			freq_tmp = (source_pll_frequency / div0_tmp);
			if (freq_tmp >= (target_frequency - MAX_TARGET_DEVIATION) &&
			    freq_tmp <= (target_frequency + MAX_TARGET_DEVIATION)) {
				LOG_DBG("%u / %u = %u", source_pll_frequency, *divisor0,
					freq_tmp);
				*divisor0 = div0_tmp;
				*resulting_frequency = freq_tmp;
				return true;
			}
		}
	}

	LOG_ERR("no suitable divisor%s found for conversion from frequency %u to %u",
		(divisor1 != NULL) ? "1/divisor0 tuple" : "0", source_pll_frequency,
		target_frequency);
	return false;
}

static bool xlnx_zynq_ps7_clkc_is_pll_driving_cpu(
	uint32_t cpu_source_pll,
	enum xlnx_zynq_ps7_clkc_clock_identifier clock_id)
{
	/* No parameter checks - internal function */
	if (cpu_source_pll == ARM_CLK_SOURCE_ARM_PLL ||
	    cpu_source_pll == ARM_CLK_SOURCE_ARM_PLL_ALT) {
		if (clock_id == xlnx_zynq_clk_armpll) {
			return true;
		}
	} else if (cpu_source_pll == ARM_CLK_SOURCE_DDR_PLL) {
		if (clock_id == xlnx_zynq_clk_ddrpll) {
			return true;
		}
	} else if (cpu_source_pll == ARM_CLK_SOURCE_IO_PLL) {
		if (clock_id == xlnx_zynq_clk_iopll) {
			return true;
		}
	}

	return false;
}

static void xlnx_zynq_ps7_clkc_get_register_offset(
	enum xlnx_zynq_ps7_clkc_clock_identifier clock_id,
	uint32_t *reg_offset,
	uint32_t *reg2_offset)
{
	/* No parameter checks - internal function */
	*reg_offset = 0;
	if (reg2_offset != NULL) {
		*reg2_offset = 0;
	}

	switch (clock_id) {
	case xlnx_zynq_clk_armpll:
		*reg_offset = ARM_PLL_CTRL_OFFSET;
		break;
	case xlnx_zynq_clk_ddrpll:
		*reg_offset = DDR_PLL_CTRL_OFFSET;
		break;
	case xlnx_zynq_clk_iopll:
		*reg_offset = IO_PLL_CTRL_OFFSET;
		break;
	case xlnx_zynq_clk_ddr2x:
	case xlnx_zynq_clk_ddr3x:
		*reg_offset = DDR_CLK_CTRL_OFFSET;
		break;
	case xlnx_zynq_clk_dci:
		*reg_offset = DCI_CLK_CTRL_OFFSET;
		break;
	case xlnx_zynq_clk_lqspi:
		*reg_offset = LQSPI_CLK_CTRL_OFFSET;
		break;
	case xlnx_zynq_clk_smc:
		*reg_offset = SMC_CLK_CTRL_OFFSET;
		break;
	case xlnx_zynq_clk_pcap:
		*reg_offset = PCAP_CLK_CTRL_OFFSET;
		break;
	case xlnx_zynq_clk_gem0:
		*reg_offset = GEM0_CLK_CTRL_OFFSET;
		if (reg2_offset != NULL) {
			*reg2_offset = GEM0_RCLK_CTRL_OFFSET;
		}
		break;
	case xlnx_zynq_clk_gem1:
		*reg_offset = GEM1_CLK_CTRL_OFFSET;
		if (reg2_offset != NULL) {
			*reg2_offset = GEM1_RCLK_CTRL_OFFSET;
		}
		break;
	case xlnx_zynq_clk_fclk0:
		*reg_offset = FPGA0_CLK_CTRL_OFFSET;
		break;
	case xlnx_zynq_clk_fclk1:
		*reg_offset = FPGA1_CLK_CTRL_OFFSET;
		break;
	case xlnx_zynq_clk_fclk2:
		*reg_offset = FPGA2_CLK_CTRL_OFFSET;
		break;
	case xlnx_zynq_clk_fclk3:
		*reg_offset = FPGA3_CLK_CTRL_OFFSET;
		break;
	case xlnx_zynq_clk_can0:
	case xlnx_zynq_clk_can1:
		*reg_offset = CAN_CLK_CTRL_OFFSET;
		if (reg2_offset != NULL) {
			*reg2_offset = CAN_MIOCLK_CTRL_OFFSET;
		}
		break;
	case xlnx_zynq_clk_sdio0:
	case xlnx_zynq_clk_sdio1:
		*reg_offset = SDIO_CLK_CTRL_OFFSET;
		break;
	case xlnx_zynq_clk_uart0:
	case xlnx_zynq_clk_uart1:
		*reg_offset = UART_CLK_CTRL_OFFSET;
		break;
	case xlnx_zynq_clk_spi0:
	case xlnx_zynq_clk_spi1:
		*reg_offset = SPI_CLK_CTRL_OFFSET;
		break;
	case xlnx_zynq_clk_dma:
	case xlnx_zynq_clk_usb0_aper:
	case xlnx_zynq_clk_usb1_aper:
	case xlnx_zynq_clk_gem0_aper:
	case xlnx_zynq_clk_gem1_aper:
	case xlnx_zynq_clk_sdio0_aper:
	case xlnx_zynq_clk_sdio1_aper:
	case xlnx_zynq_clk_spi0_aper:
	case xlnx_zynq_clk_spi1_aper:
	case xlnx_zynq_clk_can0_aper:
	case xlnx_zynq_clk_can1_aper:
	case xlnx_zynq_clk_i2c0_aper:
	case xlnx_zynq_clk_i2c1_aper:
	case xlnx_zynq_clk_uart0_aper:
	case xlnx_zynq_clk_uart1_aper:
	case xlnx_zynq_clk_gpio_aper:
	case xlnx_zynq_clk_lqspi_aper:
	case xlnx_zynq_clk_smc_aper:
		*reg_offset = APER_CLK_CTRL_OFFSET;
		break;
	case xlnx_zynq_clk_dbg_trc:
	case xlnx_zynq_clk_dbg_apb:
		*reg_offset = DBG_CLK_CTRL_OFFSET;
		break;
	default:
		break;
	}
}

static void xlnx_zynq_ps7_clkc_get_clk_ctrl_data(
	uint32_t clk_ctrl_reg,
	uint32_t *divisor1,
	uint32_t *divisor0,
	enum xlnx_zynq_ps7_clkc_clock_source_pll *source_pll,
	bool *active1,
	bool *active0)
{
	/* No parameter checks - internal function */

	/*
	 * The usual layout of a xxx_CLK_CTRL register is:
	 * - [25..20] DIVISOR1 (unavailable for a few peripherals)
	 * - [13..08] DIVISOR0
	 * - [06..04] SRCSEL - Source PLL identification
	 *            For some peripherals, this field may indicate EMIO as clock source.
	 *   [    01] CLKACT1 Clock active bit for instance 1 of peripherals that have
	 *            two instances but share the same source / divisor config (e.g. UART, CAN).
	 *   [    00] CLKACT0 (a.k.a. CLKACT if CLKACT1 is n/a)
	 */
	if (divisor1 != NULL) {
		*divisor1 = (clk_ctrl_reg >> PERIPH_CLK_DIVISOR1_SHIFT) &
			    PERIPH_CLK_DIVISOR_MASK;
	}
	if (divisor0 != NULL) {
		*divisor0 = (clk_ctrl_reg >> PERIPH_CLK_DIVISOR0_SHIFT) &
			    PERIPH_CLK_DIVISOR_MASK;
	}
	if (source_pll != NULL) {
		switch ((clk_ctrl_reg >> PERIPH_CLK_SRCSEL_SHIFT) & PERIPH_CLK_SRCSEL_MASK) {
		default:
		case 0:
		case 1:
			/* 00x = IO PLL */
			*source_pll = xlnx_zynq_clk_source_io_pll;
			break;
		case 2: /* 010 = ARM PLL */
			*source_pll = xlnx_zynq_clk_source_arm_pll;
			break;
		case 3: /* 011 = DDR PLL */
			*source_pll = xlnx_zynq_clk_source_ddr_pll;
			break;
		case 4:
		case 5:
		case 6:
		case 7: /* 1xx = EMIO */
			*source_pll = xlnx_zynq_clk_source_emio_clk;
			break;
		}
	}
	if (active1 != NULL) {
		*active1 = (clk_ctrl_reg & PERIPH_CLK_CLKACT1_BIT) ? true : false;
	}
	if (active0 != NULL) {
		*active0 = (clk_ctrl_reg & PERIPH_CLK_CLKACT0_BIT) ? true : false;
	}
}

static int xlnx_zynq_ps7_clkc_set_clk_ctrl_data(
	const struct device *dev,
	uint32_t clk_ctrl_reg,
	uint32_t *clk_ctrl_reg2,
	uint32_t *divisor1,
	uint32_t divisor0,
	enum xlnx_zynq_ps7_clkc_clock_source_pll source_pll)
{
	/* No parameter checks - internal function */
	const struct xlnx_zynq_ps7_clkc_clock_control_config *dev_cfg = dev->config;
	uint32_t reg_val;
	int err;

	err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + clk_ctrl_reg,
			      &reg_val);
	if (err != 0) {
		return -EIO;
	}

	reg_val &= (~(PERIPH_CLK_DIVISOR_MASK << PERIPH_CLK_DIVISOR1_SHIFT));
	reg_val &= (~(PERIPH_CLK_DIVISOR_MASK << PERIPH_CLK_DIVISOR0_SHIFT));
	reg_val &= (~(PERIPH_CLK_SRCSEL_MASK << PERIPH_CLK_SRCSEL_SHIFT));

	if (divisor1 != NULL) {
		reg_val |= ((*divisor1 & PERIPH_CLK_DIVISOR_MASK) << PERIPH_CLK_DIVISOR1_SHIFT);
	}
	reg_val |= ((divisor0 & PERIPH_CLK_DIVISOR_MASK) << PERIPH_CLK_DIVISOR0_SHIFT);
	reg_val |= ((source_pll & PERIPH_CLK_SRCSEL_MASK) << PERIPH_CLK_SRCSEL_SHIFT);

	err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + clk_ctrl_reg,
			       reg_val);
	if (err != 0) {
		return -EIO;
	}

	if (clk_ctrl_reg2 != NULL) {
		/* This is the case for the two GEMs only -> configure GEMx_RCLK */
		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + *clk_ctrl_reg2,
				      &reg_val);
		if (err != 0) {
			return -EIO;
		}

		/* GEMx_RCLK_CTRL.SRCSEL: 0 = MIO (standard ARM/DDR/IO PLL), 1 = EMIO */
		reg_val &= (~GEM_RCLK_SRCSEL_BIT);
		if (source_pll == xlnx_zynq_clk_source_emio_clk) {
			reg_val |= GEM_RCLK_SRCSEL_BIT;
		}

		err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + *clk_ctrl_reg2,
				       reg_val);
		if (err != 0) {
			return -EIO;
		}
	}

	return 0;
}

static uint32_t xlnx_zynq_ps7_clkc_get_aper_clkact_mask(
	enum xlnx_zynq_ps7_clkc_clock_identifier clock_id)
{
	/* No parameter checks - internal function */
	switch (clock_id) {
	case xlnx_zynq_clk_dma:
		return APER_CLK_CTRL_DMA_CLKACT_BIT;
	case xlnx_zynq_clk_usb0_aper:
		return APER_CLK_CTRL_USB0_CLKACT_BIT;
	case xlnx_zynq_clk_usb1_aper:
		return APER_CLK_CTRL_USB1_CLKACT_BIT;
	case xlnx_zynq_clk_gem0_aper:
		return APER_CLK_CTRL_GEM0_CLKACT_BIT;
	case xlnx_zynq_clk_gem1_aper:
		return APER_CLK_CTRL_GEM1_CLKACT_BIT;
	case xlnx_zynq_clk_sdio0_aper:
		return APER_CLK_CTRL_SDI0_CLKACT_BIT;
	case xlnx_zynq_clk_sdio1_aper:
		return APER_CLK_CTRL_SDI1_CLKACT_BIT;
	case xlnx_zynq_clk_spi0_aper:
		return APER_CLK_CTRL_SPI0_CLKACT_BIT;
	case xlnx_zynq_clk_spi1_aper:
		return APER_CLK_CTRL_SPI1_CLKACT_BIT;
	case xlnx_zynq_clk_can0_aper:
		return APER_CLK_CTRL_CAN0_CLKACT_BIT;
	case xlnx_zynq_clk_can1_aper:
		return APER_CLK_CTRL_CAN1_CLKACT_BIT;
	case xlnx_zynq_clk_i2c0_aper:
		return APER_CLK_CTRL_I2C0_CLKACT_BIT;
	case xlnx_zynq_clk_i2c1_aper:
		return APER_CLK_CTRL_I2C1_CLKACT_BIT;
	case xlnx_zynq_clk_uart0_aper:
		return APER_CLK_CTRL_UART0_CLKACT_BIT;
	case xlnx_zynq_clk_uart1_aper:
		return APER_CLK_CTRL_UART1_CLKACT_BIT;
	case xlnx_zynq_clk_gpio_aper:
		return APER_CLK_CTRL_GPIO_CLKACT_BIT;
	case xlnx_zynq_clk_lqspi_aper:
		return APER_CLK_CTRL_LQSPI_CLKACT_BIT;
	case xlnx_zynq_clk_smc_aper:
		return APER_CLK_CTRL_SMC_CLKACT_BIT;
	default: /* Not an xxx_APER clock */
		break;
	}

	__ASSERT(0, "invalid zero CLKACT mask for clock ID %u", (uint32_t)clock_id);
	LOG_ERR("invalid zero CLKACT mask for clock ID %u", (uint32_t)clock_id);
	return 0;
}

static int xlnx_zynq_ps7_clkc_enable_pll(const struct device *dev,
					 enum xlnx_zynq_ps7_clkc_clock_identifier clock_id)
{
	/* No parameter checks - internal function */
	const struct xlnx_zynq_ps7_clkc_clock_control_config *dev_cfg = dev->config;
	struct xlnx_zynq_ps7_clkc_clock_control_data *dev_data = dev->data;

	enum xlnx_zynq_ps7_clkc_clock_source_pll source_pll;
	uint32_t reg_offset;
	uint32_t reg_val_ctrl;
	uint32_t reg_val_status = 0;
	uint32_t pll_locked_bit;
	uint32_t clock_iter;
	int err;

	if (clock_id == xlnx_zynq_clk_armpll) {
		source_pll = xlnx_zynq_clk_source_arm_pll;
		reg_offset = ARM_PLL_CTRL_OFFSET;
		pll_locked_bit = PLL_STATUS_ARM_PLL_LOCK_BIT;
	} else if (clock_id == xlnx_zynq_clk_ddrpll) {
		source_pll = xlnx_zynq_clk_source_ddr_pll;
		reg_offset = DDR_PLL_CTRL_OFFSET;
		pll_locked_bit = PLL_STATUS_DDR_PLL_LOCK_BIT;
	} else if (clock_id == xlnx_zynq_clk_iopll) {
		source_pll = xlnx_zynq_clk_source_io_pll;
		reg_offset = IO_PLL_CTRL_OFFSET;
		pll_locked_bit = PLL_STATUS_IO_PLL_LOCK_BIT;
	} else {
		return -EINVAL;
	}

	err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
			      &reg_val_ctrl);
	if (err != 0) {
		return -EIO;
	}

	reg_val_ctrl &= (~PLL_PWRDOWN_BIT);
	reg_val_ctrl |= PLL_BYPASS_FORCE_BIT | PLL_RESET_BIT;
	err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
			       reg_val_ctrl);
	if (err != 0) {
		return -EIO;
	}

	reg_val_ctrl &= (~PLL_RESET_BIT);
	err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
			       reg_val_ctrl);
	if (err != 0) {
		return -EIO;
	}

	while (!(reg_val_status & pll_locked_bit)) {
		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + PLL_STATUS_OFFSET,
				      &reg_val_status);
		if (err != 0) {
			return -EIO;
		}
	}

	reg_val_ctrl &= (~PLL_BYPASS_FORCE_BIT);
	err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
			       reg_val_ctrl);
	if (err != 0) {
		return -EIO;
	}

	for (clock_iter = 0; clock_iter <= xlnx_zynq_clk_dbg_apb; clock_iter++) {
		if (clock_iter == (uint32_t)clock_id) {
			dev_data->peripheral_clocks[clock_iter].active = true;
		} else {
			if (dev_data->peripheral_clocks[clock_iter].source_pll == source_pll) {
				dev_data->peripheral_clocks[clock_iter].parent_pll_stopped = false;
			}
		}
	}

	return 0;
}

static int xlnx_zynq_ps7_clkc_disable_pll(const struct device *dev,
					  enum xlnx_zynq_ps7_clkc_clock_identifier clock_id)
{
	/* No parameter checks - internal function */
	const struct xlnx_zynq_ps7_clkc_clock_control_config *dev_cfg = dev->config;
	struct xlnx_zynq_ps7_clkc_clock_control_data *dev_data = dev->data;

	enum xlnx_zynq_ps7_clkc_clock_source_pll source_pll;
	uint32_t reg_offset;
	uint32_t reg_val;
	uint32_t clock_iter;
	int err;

	if (clock_id == xlnx_zynq_clk_armpll) {
		source_pll = xlnx_zynq_clk_source_arm_pll;
		reg_offset = ARM_PLL_CTRL_OFFSET;
	} else if (clock_id == xlnx_zynq_clk_ddrpll) {
		source_pll = xlnx_zynq_clk_source_ddr_pll;
		reg_offset = DDR_PLL_CTRL_OFFSET;
	} else if (clock_id == xlnx_zynq_clk_iopll) {
		source_pll = xlnx_zynq_clk_source_io_pll;
		reg_offset = IO_PLL_CTRL_OFFSET;
	} else {
		return -EINVAL;
	}

	err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
			      &reg_val);
	if (err != 0) {
		return -EIO;
	}

	reg_val |= PLL_PWRDOWN_BIT | PLL_BYPASS_FORCE_BIT | PLL_RESET_BIT;
	err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
			       reg_val);
	if (err != 0) {
		return -EIO;
	}

	for (clock_iter = 0; clock_iter <= xlnx_zynq_clk_dbg_apb; clock_iter++) {
		if (clock_iter == (uint32_t)clock_id) {
			dev_data->peripheral_clocks[clock_iter].active = false;
		} else {
			if (dev_data->peripheral_clocks[clock_iter].source_pll == source_pll) {
				dev_data->peripheral_clocks[clock_iter].parent_pll_stopped = true;
			}
		}
	}

	return 0;
}

static int xlnx_zynq_ps7_clkc_read_current_config(
	const struct device *dev,
	enum xlnx_zynq_ps7_clkc_clock_identifier clock_id,
	struct xlnx_zynq_ps7_clkc_peripheral_clock *clock_data)
{
	/* No parameter checks - internal function */
	const struct xlnx_zynq_ps7_clkc_clock_control_config *dev_cfg = dev->config;
	struct xlnx_zynq_ps7_clkc_clock_control_data *dev_data = dev->data;

	int err;
	uint32_t reg_offset = 0;
	uint32_t reg2_offset = 0;
	uint32_t reg_val;
	uint32_t clkact_mask;
	uint32_t source_pll_frequency;
	uint32_t emio_iter;

	xlnx_zynq_ps7_clkc_get_register_offset(clock_id, &reg_offset, &reg2_offset);

	/*
	 * If the register offset is 0, the current clock ID refers to one of the
	 * non-peripheral related clocks which isn't read out here, but within the
	 * driver's init function instead. -> Just return.
	 */
	if (reg_offset == 0) {
		return 0;
	}

	err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
			      &reg_val);
	if (err != 0) {
		return -EIO;
	}

	/*
	 * Raw config register data has been read -> evaluate it based on which clock
	 * we're looking at.
	 */

	switch (clock_id) {
	case xlnx_zynq_clk_lqspi: /* These single-instance peripherals only have one divisor */
	case xlnx_zynq_clk_smc:
	case xlnx_zynq_clk_pcap:
	case xlnx_zynq_clk_dbg_trc:
		clock_data->divisor1 = 1;
		xlnx_zynq_ps7_clkc_get_clk_ctrl_data(reg_val, NULL, &clock_data->divisor0,
			&clock_data->source_pll, NULL, &clock_data->active);
		break;
	case xlnx_zynq_clk_gem0: /* Both divisors, individual CLK_CTRL reg., CLKACT0 each */
	case xlnx_zynq_clk_gem1:
		xlnx_zynq_ps7_clkc_get_clk_ctrl_data(reg_val, &clock_data->divisor1,
			&clock_data->divisor0, &clock_data->source_pll, NULL, &clock_data->active);
		break;
	case xlnx_zynq_clk_fclk0: /* Both divisors, non-zero divs. AND fclk-enable bit */
	case xlnx_zynq_clk_fclk1: /* The FCLKs do not have a dedicated enable bit. */
	case xlnx_zynq_clk_fclk2: /* PS7Init generated code contains divs != 1 if enabled. */
	case xlnx_zynq_clk_fclk3:
		uint32_t fclk_enable_shift = (uint32_t)clock_id - (uint32_t)xlnx_zynq_clk_fclk0;

		xlnx_zynq_ps7_clkc_get_clk_ctrl_data(reg_val, &clock_data->divisor1,
			&clock_data->divisor0, &clock_data->source_pll, NULL, NULL);
		clock_data->active = (dev_cfg->fclk_enable >> fclk_enable_shift) & 0x1;
		break;
	case xlnx_zynq_clk_can0: /* Both divisors, shared CLK_CTRL reg., CLKACT1/0 */
		xlnx_zynq_ps7_clkc_get_clk_ctrl_data(reg_val, &clock_data->divisor1,
			&clock_data->divisor0, &clock_data->source_pll, NULL, &clock_data->active);
		break;
	case xlnx_zynq_clk_can1:
		xlnx_zynq_ps7_clkc_get_clk_ctrl_data(reg_val, &clock_data->divisor1,
						 &clock_data->divisor0, &clock_data->source_pll,
						 &clock_data->active, NULL);
		break;
	case xlnx_zynq_clk_sdio0: /* DIVISOR0 only, shared CLK_CTRL reg., CLKACT0 */
	case xlnx_zynq_clk_uart0:
	case xlnx_zynq_clk_spi0:
		clock_data->divisor1 = 1;
		xlnx_zynq_ps7_clkc_get_clk_ctrl_data(reg_val, NULL, &clock_data->divisor0,
			&clock_data->source_pll, NULL, &clock_data->active);
		break;
	case xlnx_zynq_clk_sdio1: /* DIVISOR0 only, shared CLK_CTRL reg., CLKACT1 */
	case xlnx_zynq_clk_uart1:
	case xlnx_zynq_clk_spi1:
		clock_data->divisor1 = 1;
		xlnx_zynq_ps7_clkc_get_clk_ctrl_data(reg_val, NULL, &clock_data->divisor0,
			&clock_data->source_pll, &clock_data->active, NULL);
		break;
	case xlnx_zynq_clk_dma: /* always driven by cpu_2x */
		if (dev_data->cpu_source_pll == ARM_CLK_SOURCE_DDR_PLL) {
			clock_data->source_pll = xlnx_zynq_clk_source_ddr_pll;
		} else if (dev_data->cpu_source_pll == ARM_CLK_SOURCE_IO_PLL) {
			clock_data->source_pll = xlnx_zynq_clk_source_io_pll;
		} else {
			clock_data->source_pll = xlnx_zynq_clk_source_arm_pll;
		}
		clock_data->clk_frequency = dev_data->cpu_2x_frequency;
		clkact_mask = xlnx_zynq_ps7_clkc_get_aper_clkact_mask(clock_id);
		clock_data->active = (reg_val & clkact_mask) ? true : false;
		break;
	case xlnx_zynq_clk_usb0_aper: /* all _APER clocks except DMA are driven by cpu_1x */
	case xlnx_zynq_clk_usb1_aper:
	case xlnx_zynq_clk_gem0_aper:
	case xlnx_zynq_clk_gem1_aper:
	case xlnx_zynq_clk_sdio0_aper:
	case xlnx_zynq_clk_sdio1_aper:
	case xlnx_zynq_clk_spi0_aper:
	case xlnx_zynq_clk_spi1_aper:
	case xlnx_zynq_clk_can0_aper:
	case xlnx_zynq_clk_can1_aper:
	case xlnx_zynq_clk_i2c0_aper:
	case xlnx_zynq_clk_i2c1_aper:
	case xlnx_zynq_clk_uart0_aper:
	case xlnx_zynq_clk_uart1_aper:
	case xlnx_zynq_clk_gpio_aper:
	case xlnx_zynq_clk_lqspi_aper:
	case xlnx_zynq_clk_smc_aper:
		if (dev_data->cpu_source_pll == ARM_CLK_SOURCE_DDR_PLL) {
			clock_data->source_pll = xlnx_zynq_clk_source_ddr_pll;
		} else if (dev_data->cpu_source_pll == ARM_CLK_SOURCE_IO_PLL) {
			clock_data->source_pll = xlnx_zynq_clk_source_io_pll;
		} else {
			clock_data->source_pll = xlnx_zynq_clk_source_arm_pll;
		}
		clock_data->clk_frequency = dev_data->cpu_1x_frequency;
		clkact_mask = xlnx_zynq_ps7_clkc_get_aper_clkact_mask(clock_id);
		clock_data->active = (reg_val & clkact_mask) ? true : false;
		break;
	case xlnx_zynq_clk_dbg_apb:
		if (dev_data->cpu_source_pll == ARM_CLK_SOURCE_DDR_PLL) {
			clock_data->source_pll = xlnx_zynq_clk_source_ddr_pll;
		} else if (dev_data->cpu_source_pll == ARM_CLK_SOURCE_IO_PLL) {
			clock_data->source_pll = xlnx_zynq_clk_source_io_pll;
		} else {
			clock_data->source_pll = xlnx_zynq_clk_source_arm_pll;
		}
		clock_data->clk_frequency = dev_data->cpu_1x_frequency;
		clock_data->active = (reg_val & DBG_APER_CLK_CLKACT_BIT) ? true : false;
		break;
	default:
		LOG_ERR("read current config not implemented for clock ID %u (%s)",
			(uint32_t)clock_id, clock_data->clk_name);
		return -EINVAL;
	}

	/*
	 * Determine if one or more of the peripherals supporting an EMIO clock source
	 * are to be switched over to EMIO regardless of the current register readout
	 * due to a matching fixed-clock node existing in the board device tree.
	 * -> If so, update the control register:
	 *    - set EMIO as (TX) clock source, in case of the GEMs also the RX clock,
	 *    - set divisors to 1/1,
	 *    - set the effective current clock frequency value to the EMIO source clock
	 *      frequency,
	 *    - write the modified control register.
	 * If the frequency of the EMIO clock source is to be reduced further for use
	 * by the respective peripheral, divisors will be calculated and applied in the
	 * set_rate function. Divisors 1/1 have to be assumed here as the standard fixed-
	 * clock doesn't allow for specifying anything like one or more divisor(s).
	 */

	if (clock_id == xlnx_zynq_clk_gem0 ||
	    clock_id == xlnx_zynq_clk_gem1 ||
	    clock_id == xlnx_zynq_clk_dbg_trc) {
		const struct xlnx_zynq_ps7_clkc_emio_clock_source_dt *emio_source = NULL;
		bool do_update = false;

		for (emio_iter = 0; emio_iter < dev_cfg->emio_clocks_count; emio_iter++) {
			do_update = false;
			emio_source = &dev_cfg->emio_clock_sources_dt[emio_iter];

			if (clock_id == xlnx_zynq_clk_gem0) {
				if (strcmp(emio_source->emio_clk_name, "gem0_emio_clk") == 0) {
					clock_data->emio_clock_source.dt_config = emio_source;
					LOG_DBG("EMIO clock source data found for clock %s",
						"gem0");
					do_update = true;
					break;
				}
			} else if (clock_id == xlnx_zynq_clk_gem1) {
				if (strcmp(emio_source->emio_clk_name, "gem1_emio_clk") == 0) {
					clock_data->emio_clock_source.dt_config = emio_source;
					LOG_DBG("EMIO clock source data found for clock %s",
						"gem1");
					do_update = true;
					break;
				}
			} else if (clock_id == xlnx_zynq_clk_dbg_trc) {
				if (strcmp(emio_source->emio_clk_name, "dbg_trc_emio_clk") == 0) {
					clock_data->emio_clock_source.dt_config = emio_source;
					LOG_DBG("EMIO clock source data found for clock %s",
						"dbg_trc");
					do_update = true;
					break;
				}
			}
		}

		if (do_update) {
			clock_data->divisor1 = 1;
			clock_data->divisor0 = 1;
			clock_data->clk_frequency = emio_source->emio_clk_frequency;
			clock_data->source_pll = xlnx_zynq_clk_source_emio_clk;

			err = xlnx_zynq_ps7_clkc_set_clk_ctrl_data(dev, reg_offset,
				(clock_id != xlnx_zynq_clk_dbg_trc) ? &reg2_offset : NULL,
				(clock_id != xlnx_zynq_clk_dbg_trc) ? &clock_data->divisor1 : NULL,
				clock_data->divisor0, clock_data->source_pll);
			if (err != 0) {
				LOG_ERR("failed to re-configure clock %u (%s) to clock source "
					"EMIO during initial enumeration", (uint32_t)clock_id,
					clock_data->clk_name);
				return err;
			}
		}
	}

	if (clock_data->source_pll != xlnx_zynq_clk_source_emio_clk &&
	    clock_data->clk_frequency == 0) {
		/*
		 * The clock frequency of the current peripheral is not
		 * fixed (e.g. cpu_1x, cpu_2x etc.) and the clock's source
		 * is one of the internal PLLs -> calculate the peripheral's
		 * current clock frequency based on the source PLL
		 */
		if (clock_data->source_pll == xlnx_zynq_clk_source_io_pll) {
			source_pll_frequency = dev_data->io_pll_frequency;
		} else if (clock_data->source_pll == xlnx_zynq_clk_source_ddr_pll) {
			source_pll_frequency = dev_data->ddr_pll_frequency;
		} else {
			source_pll_frequency = dev_data->arm_pll_frequency;
		}

		clock_data->clk_frequency = source_pll_frequency /
					    clock_data->divisor1 /
					    clock_data->divisor0;
	}

	if (clock_data->clk_frequency == 0) {
		LOG_ERR("failed to acquire the current clock frequency for clock ID %u (%s)",
			(uint32_t)clock_id, clock_data->clk_name);
		return -EIO;
	}

	if (clock_data->source_pll == xlnx_zynq_clk_source_io_pll &&
	    dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_iopll].active == false) {
		clock_data->parent_pll_stopped = true;
	} else if (clock_data->source_pll == xlnx_zynq_clk_source_ddr_pll &&
		   dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_ddrpll].active == false) {
		clock_data->parent_pll_stopped = true;
	} else if (clock_data->source_pll == xlnx_zynq_clk_source_arm_pll &&
		   dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_armpll].active == false) {
		clock_data->parent_pll_stopped = true;
	}

	return 0;
}

static int xlnx_zynq_ps7_clkc_clkctrl_on(const struct device *dev,
					 clock_control_subsys_t sys)
{
	int err;
	uint32_t reg_val;
	uint32_t reg_offset = 0;
	uint32_t reg2_val;
	uint32_t reg2_offset = 0;
	uint32_t clkact_shift;

	__ASSERT(dev != NULL, "device pointer is NULL");
	if (dev == NULL) {
		LOG_ERR("device pointer is NULL");
		return -EINVAL;
	}

	const struct xlnx_zynq_ps7_clkc_clock_control_config *dev_cfg = dev->config;
	struct xlnx_zynq_ps7_clkc_clock_control_data *dev_data = dev->data;
	enum xlnx_zynq_ps7_clkc_clock_identifier clock_id =
		(enum xlnx_zynq_ps7_clkc_clock_identifier)sys;

	__ASSERT(clock_id <= xlnx_zynq_clk_dbg_apb, "clock ID %u is out of range",
		(uint32_t)clock_id);
	if (clock_id > xlnx_zynq_clk_dbg_apb) {
		LOG_ERR("clock ID %u is out of range", (uint32_t)clock_id);
		return -EINVAL;
	}

	struct xlnx_zynq_ps7_clkc_peripheral_clock *clock_data =
		&dev_data->peripheral_clocks[(uint32_t)clock_id];

	__ASSERT(clock_id == clock_data->peripheral_clock_id,
		 "data inconsistency: clock ID %u resolves clock data struct for clock ID %u",
		 (uint32_t)clock_id, (uint32_t)clock_data->peripheral_clock_id);
	if (clock_id != clock_data->peripheral_clock_id) {
		LOG_ERR("data inconsistency: clock ID %u resolves clock data struct for clock "
			"ID %u", (uint32_t)clock_id, (uint32_t)clock_data->peripheral_clock_id);
		return -EINVAL;
	}

	/* Performed all parameter error checks -> break out if running in QEMU */
	if (IS_ENABLED(CONFIG_QEMU_TARGET)) {
		LOG_DBG("Skipping set clock on call: register space unavailable in QEMU");
		return 0;
	}

	if (clock_data->active) {
		return 0;
	}
	if (clock_data->parent_pll_stopped) {
		return -EAGAIN;
	}

	switch (clock_id) {
	case xlnx_zynq_clk_armpll:
	/* clk_ddrpll: invalid, can't be turned off (comp. comment in clkctrl_off) */
	case xlnx_zynq_clk_iopll:
		return xlnx_zynq_ps7_clkc_enable_pll(dev, clock_id);
	case xlnx_zynq_clk_cpu_6or4x:
	case xlnx_zynq_clk_cpu_3or2x:
	case xlnx_zynq_clk_cpu_2x:
	case xlnx_zynq_clk_cpu_1x: /* all controller via ARM_CLK_CTRL register */
		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + ARM_CLK_CTRL_OFFSET,
				      &reg_val);
		if (err != 0) {
			return -EIO;
		}
		clkact_shift = ((uint32_t)clock_id - (uint32_t)xlnx_zynq_clk_cpu_6or4x) +
				ARM_CPU6X4X_ACTIVE_SHIFT;
		reg_val |= BIT(clkact_shift);
		err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + ARM_CLK_CTRL_OFFSET,
				       reg_val);
		if (err != 0) {
			return -EIO;
		}
		break;
	case xlnx_zynq_clk_ddr2x:
	case xlnx_zynq_clk_ddr3x: /* both controlled via the DDR_CLK_CTRL register */
		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + DDR_CLK_CTRL_OFFSET,
				      &reg_val);
		if (err != 0) {
			return -EIO;
		}
		clkact_shift = ((uint32_t)clock_id - (uint32_t)xlnx_zynq_clk_ddr2x) ^ 1;
		reg_val |= BIT(clkact_shift);
		err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + DDR_CLK_CTRL_OFFSET,
				       reg_val);
		if (err != 0) {
			return -EIO;
		}
		break;
	case xlnx_zynq_clk_dci:
		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + DCI_CLK_CTRL_OFFSET,
				      &reg_val);
		if (err != 0) {
			return -EIO;
		}
		reg_val |= PERIPH_CLK_CLKACT0_BIT;
		err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + DCI_CLK_CTRL_OFFSET,
				       reg_val);
		if (err != 0) {
			return -EIO;
		}
		break;
	case xlnx_zynq_clk_gem0:
	case xlnx_zynq_clk_gem1:
		/*
		 * The two GEMs have individual CLKACT bits for their TX and RX clocks
		 * contained in two different registers. xlnx_zynq_ps7_clkc_get_register_offset()
		 * returns the TX clock register in reg_offset and the RX clock register in
		 * reg2_offset.
		 */
		xlnx_zynq_ps7_clkc_get_register_offset(clock_id, &reg_offset, &reg2_offset);
		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
				      &reg_val);
		err += syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + reg2_offset,
				       &reg2_val);
		if (err != 0) {
			return -EIO;
		}

		reg_val |= PERIPH_CLK_CLKACT0_BIT; /* TX clock enable */
		reg2_val |= PERIPH_CLK_CLKACT0_BIT; /* RX clock enable */

		/*
		 * Special GEM handling: set the RX clock source as MIO or EMIO based
		 * on the TX clock configuration. This hasn't been touched during initial
		 * driver init & initial current config acquisition. By the time the gem0/1
		 * clocks are first turned on, there might still be a config mismatch between
		 * the respective CLK_CTRL and RCLK_CTRL registers.
		 * GEMx_RCLK_CTRL[4] 0 = RX clock source is MIO, 1 = RX clock source is EMIO
		 */
		if (dev_data->peripheral_clocks[(uint32_t)clock_id].emio_clock_source.dt_config !=
		    NULL) {
			reg2_val |= BIT(4);
		} else {
			reg2_val &= (~BIT(4));
		}

		err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
				       reg_val);
		err += syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + reg2_offset,
					reg2_val);
		if (err != 0) {
			return -EIO;
		}
		break;
	case xlnx_zynq_clk_fclk0:
	case xlnx_zynq_clk_fclk1:
	case xlnx_zynq_clk_fclk2:
	case xlnx_zynq_clk_fclk3:
		/*
		 * The FCLKs cannot be explicitly turned on or off.
		 * If the respective FCLK is marked enabled via the fclk-enable word
		 * from the DT, confirm that it is on (active = true already set in
		 * xlnx_zynq_ps7_clkc_read_current_config). If the respective FCLK is
		 * disabled as indicated by the DT, it cannot be turned on,
		 * return -EINVAL.
		 */
		uint32_t fclk_enable_shift = (uint32_t)clock_id - (uint32_t)xlnx_zynq_clk_fclk0;
		bool fclk_enabled = (dev_cfg->fclk_enable >> fclk_enable_shift) & 0x1;

		if (!fclk_enabled) {
			return -EINVAL;
		}
		break;
	case xlnx_zynq_clk_lqspi:
	case xlnx_zynq_clk_smc:
	case xlnx_zynq_clk_pcap:
	case xlnx_zynq_clk_can0:
	case xlnx_zynq_clk_sdio0:
	case xlnx_zynq_clk_uart0:
	case xlnx_zynq_clk_spi0:
	case xlnx_zynq_clk_dbg_trc:
		xlnx_zynq_ps7_clkc_get_register_offset(clock_id, &reg_offset, NULL);
		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
				      &reg_val);
		if (err != 0) {
			return -EIO;
		}
		reg_val |= PERIPH_CLK_CLKACT0_BIT;
		err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
				       reg_val);
		if (err != 0) {
			return -EIO;
		}
		break;
	case xlnx_zynq_clk_can1:
	case xlnx_zynq_clk_sdio1:
	case xlnx_zynq_clk_uart1:
	case xlnx_zynq_clk_spi1:
	case xlnx_zynq_clk_dbg_apb:
		xlnx_zynq_ps7_clkc_get_register_offset(clock_id, &reg_offset, NULL);
		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
				      &reg_val);
		if (err != 0) {
			return -EIO;
		}
		reg_val |= PERIPH_CLK_CLKACT1_BIT;
		err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
				       reg_val);
		if (err != 0) {
			return -EIO;
		}
		break;
	case xlnx_zynq_clk_dma:
	case xlnx_zynq_clk_usb0_aper:
	case xlnx_zynq_clk_usb1_aper:
	case xlnx_zynq_clk_gem0_aper:
	case xlnx_zynq_clk_gem1_aper:
	case xlnx_zynq_clk_sdio0_aper:
	case xlnx_zynq_clk_sdio1_aper:
	case xlnx_zynq_clk_spi0_aper:
	case xlnx_zynq_clk_spi1_aper:
	case xlnx_zynq_clk_can0_aper:
	case xlnx_zynq_clk_can1_aper:
	case xlnx_zynq_clk_i2c0_aper:
	case xlnx_zynq_clk_i2c1_aper:
	case xlnx_zynq_clk_uart0_aper:
	case xlnx_zynq_clk_uart1_aper:
	case xlnx_zynq_clk_gpio_aper:
	case xlnx_zynq_clk_lqspi_aper:
	case xlnx_zynq_clk_smc_aper:
		/* All _aper clocks are controlled via the APER_CTK_CTRL register */
		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + APER_CLK_CTRL_OFFSET,
				      &reg_val);
		if (err != 0) {
			return -EIO;
		}
		err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + APER_CLK_CTRL_OFFSET,
			(reg_val | xlnx_zynq_ps7_clkc_get_aper_clkact_mask(clock_id)));
		if (err != 0) {
			return -EIO;
		}
		break;
	default:
		LOG_ERR("clkctrl_on not implemented for clock ID %u (%s)",
			(uint32_t)clock_id, clock_data->clk_name);
		return -EINVAL;
	}

	dev_data->peripheral_clocks[(uint32_t)clock_id].active = true;

	LOG_INF("clock ID %u (%s) is now on", (uint32_t)clock_id, clock_data->clk_name);
	return 0;
}

static int xlnx_zynq_ps7_clkc_clkctrl_off(const struct device *dev,
					  clock_control_subsys_t sys)
{
	int err;
	uint32_t reg_val;
	uint32_t reg_offset = 0;
	uint32_t reg2_val;
	uint32_t reg2_offset = 0;
	uint32_t clkact_shift;

	__ASSERT(dev != NULL, "device pointer is NULL");
	if (dev == NULL) {
		LOG_ERR("device pointer is NULL");
		return -EINVAL;
	}

	const struct xlnx_zynq_ps7_clkc_clock_control_config *dev_cfg = dev->config;
	struct xlnx_zynq_ps7_clkc_clock_control_data *dev_data = dev->data;
	enum xlnx_zynq_ps7_clkc_clock_identifier clock_id =
		(enum xlnx_zynq_ps7_clkc_clock_identifier)sys;

	__ASSERT(clock_id <= xlnx_zynq_clk_dbg_apb, "clock ID %u is out of range",
		(uint32_t)clock_id);
	if (clock_id > xlnx_zynq_clk_dbg_apb) {
		LOG_ERR("clock ID %u is out of range", (uint32_t)clock_id);
		return -EINVAL;
	}

	struct xlnx_zynq_ps7_clkc_peripheral_clock *clock_data =
		&dev_data->peripheral_clocks[(uint32_t)clock_id];

	__ASSERT(clock_id == clock_data->peripheral_clock_id,
		 "data inconsistency: clock ID %u resolves clock data struct for clock ID %u",
		 (uint32_t)clock_id, (uint32_t)clock_data->peripheral_clock_id);
	if (clock_id != clock_data->peripheral_clock_id) {
		LOG_ERR("data inconsistency: clock ID %u resolves clock data struct for clock "
			"ID %u", (uint32_t)clock_id, (uint32_t)clock_data->peripheral_clock_id);
		return -EINVAL;
	}

	/* Performed all parameter error checks -> break out if running in QEMU */
	if (IS_ENABLED(CONFIG_QEMU_TARGET)) {
		LOG_DBG("Skipping set clock off call: register space unavailable in QEMU");
		return 0;
	}

	if (!clock_data->active) {
		return 0;
	}

	switch (clock_id) {
	case xlnx_zynq_clk_armpll:
	/* clk_ddrpll: invalid, would kill the entire system unless running exclusively from OCM */
	case xlnx_zynq_clk_iopll:
		if (xlnx_zynq_ps7_clkc_is_pll_driving_cpu(dev_data->cpu_source_pll, clock_id)) {
			LOG_ERR("cannot turn off the PLL driving the CPU: clock ID %u (%s)!",
				(uint32_t)clock_id, clock_data->clk_name);
			return -EINVAL;
		}
		return xlnx_zynq_ps7_clkc_disable_pll(dev, clock_id);
	case xlnx_zynq_clk_cpu_6or4x:
	case xlnx_zynq_clk_cpu_3or2x:
	case xlnx_zynq_clk_cpu_2x:
	case xlnx_zynq_clk_cpu_1x: /* all controller via ARM_CLK_CTRL register */
		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + ARM_CLK_CTRL_OFFSET,
				      &reg_val);
		if (err != 0) {
			return -EIO;
		}
		clkact_shift = ((uint32_t)clock_id - (uint32_t)xlnx_zynq_clk_cpu_6or4x) +
				ARM_CPU6X4X_ACTIVE_SHIFT;
		reg_val &= (~BIT(clkact_shift));
		err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + ARM_CLK_CTRL_OFFSET,
				       reg_val);
		if (err != 0) {
			return -EIO;
		}
		break;
	case xlnx_zynq_clk_ddr2x:
	case xlnx_zynq_clk_ddr3x: /* both controlled via the DDR_CLK_CTRL register */
		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + DDR_CLK_CTRL_OFFSET,
				      &reg_val);
		if (err != 0) {
			return -EIO;
		}
		clkact_shift = ((uint32_t)clock_id - (uint32_t)xlnx_zynq_clk_ddr2x) ^ 1;
		reg_val &= (~BIT(clkact_shift));
		err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + DDR_CLK_CTRL_OFFSET,
				       reg_val);
		if (err != 0) {
			return -EIO;
		}
		break;
	case xlnx_zynq_clk_dci:
		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + DCI_CLK_CTRL_OFFSET,
				      &reg_val);
		if (err != 0) {
			return -EIO;
		}
		reg_val &= (~PERIPH_CLK_CLKACT0_BIT);
		err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + DCI_CLK_CTRL_OFFSET,
				       reg_val);
		if (err != 0) {
			return -EIO;
		}
		break;
	case xlnx_zynq_clk_gem0:
	case xlnx_zynq_clk_gem1:
		xlnx_zynq_ps7_clkc_get_register_offset(clock_id, &reg_offset, &reg2_offset);
		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
				      &reg_val);
		err += syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + reg2_offset,
				       &reg2_val);
		if (err != 0) {
			return -EIO;
		}
		reg_val &= (~PERIPH_CLK_CLKACT0_BIT); /* TX clock enable */
		reg2_val &= (~PERIPH_CLK_CLKACT0_BIT); /* RX clock enable */
		err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
				       reg_val);
		err += syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + reg2_offset,
					reg2_val);
		if (err != 0) {
			return -EIO;
		}
		break;
	case xlnx_zynq_clk_fclk0:
	case xlnx_zynq_clk_fclk1:
	case xlnx_zynq_clk_fclk2:
	case xlnx_zynq_clk_fclk3:
		uint32_t fclk_enable_shift = (uint32_t)clock_id - (uint32_t)xlnx_zynq_clk_fclk0;
		bool fclk_enabled = (dev_cfg->fclk_enable >> fclk_enable_shift) & 0x1;

		if (fclk_enabled) {
			/*
			 * This FCLK is defined as enabled in the DT and can therefore not be
			 * turned off.
			 */
			return -EAGAIN;
		}
		break;
	case xlnx_zynq_clk_lqspi:
	case xlnx_zynq_clk_smc:
	case xlnx_zynq_clk_pcap:
	case xlnx_zynq_clk_can0:
	case xlnx_zynq_clk_sdio0:
	case xlnx_zynq_clk_uart0:
	case xlnx_zynq_clk_spi0:
	case xlnx_zynq_clk_dbg_trc:
		xlnx_zynq_ps7_clkc_get_register_offset(clock_id, &reg_offset, NULL);
		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
				      &reg_val);
		if (err != 0) {
			return -EIO;
		}
		reg_val &= (~PERIPH_CLK_CLKACT0_BIT);
		err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
				       reg_val);
		if (err != 0) {
			return -EIO;
		}
		break;
	case xlnx_zynq_clk_can1:
	case xlnx_zynq_clk_sdio1:
	case xlnx_zynq_clk_uart1:
	case xlnx_zynq_clk_spi1:
	case xlnx_zynq_clk_dbg_apb:
		xlnx_zynq_ps7_clkc_get_register_offset(clock_id, &reg_offset, NULL);
		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
				      &reg_val);
		if (err != 0) {
			return -EIO;
		}
		reg_val &= (~PERIPH_CLK_CLKACT1_BIT);
		err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset,
				       reg_val);
		if (err != 0) {
			return -EIO;
		}
		break;
	case xlnx_zynq_clk_dma:
	case xlnx_zynq_clk_usb0_aper:
	case xlnx_zynq_clk_usb1_aper:
	case xlnx_zynq_clk_gem0_aper:
	case xlnx_zynq_clk_gem1_aper:
	case xlnx_zynq_clk_sdio0_aper:
	case xlnx_zynq_clk_sdio1_aper:
	case xlnx_zynq_clk_spi0_aper:
	case xlnx_zynq_clk_spi1_aper:
	case xlnx_zynq_clk_can0_aper:
	case xlnx_zynq_clk_can1_aper:
	case xlnx_zynq_clk_i2c0_aper:
	case xlnx_zynq_clk_i2c1_aper:
	case xlnx_zynq_clk_uart0_aper:
	case xlnx_zynq_clk_uart1_aper:
	case xlnx_zynq_clk_gpio_aper:
	case xlnx_zynq_clk_lqspi_aper:
	case xlnx_zynq_clk_smc_aper:
		/* All _aper clocks are controlled via the APER_CTK_CTRL register */
		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + APER_CLK_CTRL_OFFSET,
				      &reg_val);
		if (err != 0) {
			return -EIO;
		}
		err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + APER_CLK_CTRL_OFFSET,
			(reg_val & (~xlnx_zynq_ps7_clkc_get_aper_clkact_mask(clock_id))));
		if (err != 0) {
			return -EIO;
		}
		break;
	default:
		LOG_ERR("clkctrl_off not implemented for clock ID %u (%s)",
			(uint32_t)clock_id, clock_data->clk_name);
		return -EINVAL;
	}

	dev_data->peripheral_clocks[(uint32_t)clock_id].active = false;

	LOG_INF("clock ID %u (%s) is now off", (uint32_t)clock_id, clock_data->clk_name);
	return 0;
}

static int xlnx_zynq_ps7_clkc_clkctrl_get_rate(const struct device *dev,
					       clock_control_subsys_t sys,
					       uint32_t *rate)
{
	__ASSERT(dev != NULL, "device pointer is NULL");
	__ASSERT(rate != NULL, "frequency output pointer is NULL");
	if (dev == NULL) {
		LOG_ERR("device pointer is NULL");
		return -EINVAL;
	}
	if (rate == NULL) {
		LOG_ERR("frequency output pointer is NULL");
		return -EINVAL;
	}

	struct xlnx_zynq_ps7_clkc_clock_control_data *dev_data = dev->data;
	enum xlnx_zynq_ps7_clkc_clock_identifier clock_id =
		(enum xlnx_zynq_ps7_clkc_clock_identifier)sys;

	__ASSERT(clock_id <= xlnx_zynq_clk_dbg_apb, "clock ID %u is out of range",
		(uint32_t)clock_id);
	if (clock_id > xlnx_zynq_clk_dbg_apb) {
		LOG_ERR("clock ID %u is out of range", (uint32_t)clock_id);
		return -EINVAL;
	}

	__ASSERT(clock_id == dev_data->peripheral_clocks[(uint32_t)clock_id].peripheral_clock_id,
		 "data inconsistency: clock ID %u resolves clock data struct for clock ID %u",
		 (uint32_t)clock_id, (uint32_t)
		 dev_data->peripheral_clocks[(uint32_t)clock_id].peripheral_clock_id);
	if (clock_id != dev_data->peripheral_clocks[(uint32_t)clock_id].peripheral_clock_id) {
		LOG_ERR("data inconsistency: clock ID %u resolves clock data struct for clock "
			"ID %u", (uint32_t)clock_id, (uint32_t)
			dev_data->peripheral_clocks[(uint32_t)clock_id].peripheral_clock_id);
		return -EINVAL;
	}

	/* Performed all parameter error checks -> break out if running in QEMU */
	if (IS_ENABLED(CONFIG_QEMU_TARGET)) {
		LOG_DBG("Skipping get clock rate call: register space unavailable in QEMU");
		return 100000000; /* assume 100 MHz clock, regardless of what we're looking at */
	}

	if (dev_data->peripheral_clocks[(uint32_t)clock_id].active == false ||
	    dev_data->peripheral_clocks[(uint32_t)clock_id].parent_pll_stopped == true) {
		return -EAGAIN;
	}

	*rate = dev_data->peripheral_clocks[(uint32_t)clock_id].clk_frequency;
	return 0;
}

static enum clock_control_status xlnx_zynq_ps7_clkc_clkctrl_get_status(
	const struct device *dev,
	clock_control_subsys_t sys)
{
	__ASSERT(dev != NULL, "device pointer is NULL");
	if (dev == NULL) {
		LOG_ERR("device pointer is NULL");
		return -EINVAL;
	}

	struct xlnx_zynq_ps7_clkc_clock_control_data *dev_data = dev->data;
	enum xlnx_zynq_ps7_clkc_clock_identifier clock_id =
		(enum xlnx_zynq_ps7_clkc_clock_identifier)sys;

	__ASSERT(clock_id <= xlnx_zynq_clk_dbg_apb, "clock ID %u is out of range",
		(uint32_t)clock_id);
	if (clock_id > xlnx_zynq_clk_dbg_apb) {
		LOG_ERR("clock ID %u is out of range", (uint32_t)clock_id);
		return -EINVAL;
	}

	__ASSERT(clock_id == dev_data->peripheral_clocks[(uint32_t)clock_id].peripheral_clock_id,
		 "data inconsistency: clock ID %u resolves clock data struct for clock ID %u",
		 (uint32_t)clock_id, (uint32_t)
		 dev_data->peripheral_clocks[(uint32_t)clock_id].peripheral_clock_id);
	if (clock_id != dev_data->peripheral_clocks[(uint32_t)clock_id].peripheral_clock_id) {
		LOG_ERR("data inconsistency: clock ID %u resolves clock data struct for clock "
			"ID %u", (uint32_t)clock_id, (uint32_t)
			dev_data->peripheral_clocks[(uint32_t)clock_id].peripheral_clock_id);
		return CLOCK_CONTROL_STATUS_OFF;
	}

	/* Performed all parameter error checks -> break out if running in QEMU */
	if (IS_ENABLED(CONFIG_QEMU_TARGET)) {
		LOG_DBG("Skipping get clock status call: register space unavailable in QEMU");
		return CLOCK_CONTROL_STATUS_ON;
	}

	return (dev_data->peripheral_clocks[(uint32_t)clock_id].active &&
		!dev_data->peripheral_clocks[(uint32_t)clock_id].parent_pll_stopped) ?
		CLOCK_CONTROL_STATUS_ON : CLOCK_CONTROL_STATUS_OFF;
}

static int xlnx_zynq_ps7_clkc_clkctrl_set_rate(const struct device *dev,
					       clock_control_subsys_t sys,
					       clock_control_subsys_rate_t rate)
{
	uint32_t target_frequency = (uint32_t)rate;
	uint32_t pll_frequency;
	uint32_t divisor1 = 1;
	uint32_t divisor0 = 1;
	uint32_t resulting_frequency = 0;
	uint32_t clock_iter;

	uint32_t reg_offset;
	uint32_t reg_val;
	int err;

	__ASSERT(dev != NULL, "device pointer is NULL");
	if (dev == NULL) {
		LOG_ERR("device pointer is NULL");
		return -EINVAL;
	}

	const struct xlnx_zynq_ps7_clkc_clock_control_config *dev_cfg = dev->config;
	struct xlnx_zynq_ps7_clkc_clock_control_data *dev_data = dev->data;
	enum xlnx_zynq_ps7_clkc_clock_identifier clock_id =
		(enum xlnx_zynq_ps7_clkc_clock_identifier)sys;

	__ASSERT(clock_id <= xlnx_zynq_clk_dbg_apb, "clock ID %u is out of range",
		(uint32_t)clock_id);
	if (clock_id > xlnx_zynq_clk_dbg_apb) {
		LOG_ERR("clock ID %u is out of range", (uint32_t)clock_id);
		return -EINVAL;
	}

	__ASSERT(target_frequency != 0, "target frequency for clock ID %u must not be 0",
		(uint32_t)clock_id);
	if (target_frequency == 0) {
		LOG_ERR("target frequency for clock ID %u must not be 0",
			(uint32_t)clock_id);
		return -EINVAL;
	}

	struct xlnx_zynq_ps7_clkc_peripheral_clock *clock_data =
		&dev_data->peripheral_clocks[(uint32_t)clock_id];

	__ASSERT(clock_id == clock_data->peripheral_clock_id,
		 "data inconsistency: clock ID %u resolves clock data struct for clock ID %u",
			(uint32_t)clock_id, (uint32_t)clock_data->peripheral_clock_id);
	if (clock_id != clock_data->peripheral_clock_id) {
		LOG_ERR("data inconsistency: clock ID %u resolves clock data struct for clock "
			"ID %u", (uint32_t)clock_id, (uint32_t)clock_data->peripheral_clock_id);
		return -EINVAL;
	}

	/* Performed all parameter error checks -> break out if running in QEMU */
	if (IS_ENABLED(CONFIG_QEMU_TARGET)) {
		LOG_DBG("Skipping set clock rate call: register space unavailable in QEMU");
		return 0;
	}

	/*
	 * Determine the frequency of the PLL driving the respective peripheral.
	 * The peripheral's clock divisor(s) will be applied to this frequency.
	 * For a few peripherals, the clock source may also be an EMIO clock.
	 */
	switch (clock_data->source_pll) {
	case xlnx_zynq_clk_source_io_pll:
		pll_frequency =
			dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_iopll].clk_frequency;
		break;
	case xlnx_zynq_clk_source_ddr_pll:
		pll_frequency =
			dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_ddrpll].clk_frequency;
		break;
	case xlnx_zynq_clk_source_arm_pll:
		pll_frequency =
			dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_armpll].clk_frequency;
		break;
	case xlnx_zynq_clk_source_emio_clk:
		__ASSERT(clock_data->emio_clock_source.dt_config != NULL,
			 "clock ID %u (%s) source EMIO clock data unavailable",
			 (uint32_t)clock_id, clock_data->clk_name);
		if (clock_data->emio_clock_source.dt_config == NULL) {
			LOG_ERR("clock ID %u (%s) source EMIO clock data unavailable",
				(uint32_t)clock_id, clock_data->clk_name);
			return -EINVAL;
		}
		pll_frequency = (clock_data->emio_clock_source.dt_config)->emio_clk_frequency;
		break;
	default:
		__ASSERT(0, "invalid source PLL or EMIO clock entry for clock %u (%s)",
			(uint32_t)clock_id, clock_data->clk_name);
		LOG_ERR("invalid source PLL or EMIO clock entry for clock %u (%s)",
			(uint32_t)clock_id, clock_data->clk_name);
		return -EINVAL;
	}

	/*
	 * Calculate either DIVISOR1 and DIVISOR0 or DIVISOR0 only for the input to
	 * target frequency reduction. For certain clocks, this information might not
	 * even be useful/applicable (all cpu_..., all ..._aper, dma, dbg_apb), but for
	 * those clocks, it is at least determined at this point if source and target
	 * frequencies are divisible at all.
	 */

	switch (clock_id) {
	case xlnx_zynq_clk_armpll:
	case xlnx_zynq_clk_ddrpll:
	case xlnx_zynq_clk_iopll:
		/* Skip the 3 base PLLs. Calculation of their PLL_FDIV value below. */
		break;
	case xlnx_zynq_clk_gem0:
	case xlnx_zynq_clk_gem1:
	case xlnx_zynq_clk_fclk0:
	case xlnx_zynq_clk_fclk1:
	case xlnx_zynq_clk_fclk2:
	case xlnx_zynq_clk_fclk3:
	case xlnx_zynq_clk_can0:
	case xlnx_zynq_clk_can1:
		/* These peripherals support DIVISOR1 and DIVISOR0 */
		if (!xlnx_zynq_ps7_clkc_calculate_divisors(pll_frequency, target_frequency,
			&resulting_frequency, &divisor1, &divisor0, false)) {
			LOG_ERR("divisor0/1 calculation failed for clock ID %u (%s)",
				(uint32_t)clock_id, clock_data->clk_name);
			return -EAGAIN;
		}
		break;
	default:
		/* Everything else just supports DIVISOR0, must be even for ddr_3x */
		if (!xlnx_zynq_ps7_clkc_calculate_divisors(pll_frequency, target_frequency,
			&resulting_frequency, NULL, &divisor0,
			(clock_id != xlnx_zynq_clk_ddr3x) ? false : true)) {
			LOG_ERR("divisor0 calculation failed for clock ID %u (%s)",
				(uint32_t)clock_id, clock_data->clk_name);
			return -EAGAIN;
		}
		break;
	}

	/*
	 * Read the current contents of the respective peripheral's control register
	 * -> Will be altered based on which peripheral is being configured.
	 */
	xlnx_zynq_ps7_clkc_get_register_offset(clock_id, &reg_offset, NULL);
	err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset, &reg_val);
	if (err != 0) {
		LOG_ERR("read control register failed for clock ID %u (%s)",
			(uint32_t)clock_id, clock_data->clk_name);
		return -EIO;
	}

	switch (clock_id) {
	case xlnx_zynq_clk_armpll:
	case xlnx_zynq_clk_ddrpll:
	case xlnx_zynq_clk_iopll:
		/*
		 * Special case: changing the frequency of one of the three main PLLs.
		 * This may not be the PLL driving the CPU itself, for obvious reasons.
		 * If one of the other two PLLs is modified, all peripherals driven by
		 * this PLL must be updated, as they now apply their respective divisors
		 * to a changed PLL frequency.
		 */
		if (xlnx_zynq_ps7_clkc_is_pll_driving_cpu(dev_data->cpu_source_pll, clock_id)) {
			LOG_ERR("cannot change the frequency of the PLL driving the CPU: "
				"clock ID %u (%s)!", (uint32_t)clock_id, clock_data->clk_name);
			return -EINVAL;
		}

		uint32_t pll_fdiv = 0; /* PLL_FDIV is [18..12] in the respective ctrl. reg */
				       /* -> 7 bits, valid range 1 .. 127 */
		uint32_t pll_fdiv_tmp;
		uint32_t pll_freq_tmp;

		for (pll_fdiv_tmp = 1; pll_fdiv_tmp <= 127; pll_fdiv_tmp++) {
			pll_freq_tmp = dev_cfg->ps_clk_frequency * pll_fdiv_tmp;
			if (pll_freq_tmp >= (target_frequency - MAX_TARGET_DEVIATION) &&
			    pll_freq_tmp <= (target_frequency + MAX_TARGET_DEVIATION)) {
				pll_fdiv = pll_fdiv_tmp;
				resulting_frequency = (dev_cfg->ps_clk_frequency * pll_fdiv_tmp);
				break;
			}
		}

		if (pll_fdiv == 0) {
			LOG_ERR("could not compute a suitable PLL_FDIV value to generate the "
				"target frequency %u from the ps_clk_frequency value %u for the"
				" base PLL %s", target_frequency, dev_cfg->ps_clk_frequency,
				clock_data->clk_name);
			return -EAGAIN;
		}

		/*
		 * Observe the proper reset/override/re-configure/re-enable sequence when
		 * re-configuring one of the base PLLs -> comp. PS7Init report.
		 */
		xlnx_zynq_ps7_clkc_clkctrl_off(dev, sys);

		/*
		 * The control register must be re-read in its current state ->
		 * xlnx_zynq_ps7_clkc_clkctrl_off changed the reset and bypass bits
		 */

		err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset, &reg_val);
		if (err != 0) {
			LOG_ERR("read control register failed for clock ID %u (%s)",
				(uint32_t)clock_id, clock_data->clk_name);
			return -EIO;
		}
		reg_val &= ~(PLL_FDIV_MASK << PLL_FDIV_SHIFT);
		err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset, reg_val);
		if (err != 0) {
			LOG_ERR("write control register failed for clock ID %u (%s)",
				(uint32_t)clock_id, clock_data->clk_name);
			return -EIO;
		}

		xlnx_zynq_ps7_clkc_clkctrl_on(dev, sys);

		clock_data->clk_frequency = resulting_frequency;
		enum xlnx_zynq_ps7_clkc_clock_source_pll source_pll = xlnx_zynq_clk_source_arm_pll;

		if (clock_id == xlnx_zynq_clk_armpll) {
			dev_data->arm_pll_multiplier = pll_fdiv;
			dev_data->arm_pll_frequency = resulting_frequency;
			source_pll = xlnx_zynq_clk_source_arm_pll;
		} else if (clock_id == xlnx_zynq_clk_ddrpll) {
			dev_data->ddr_pll_multiplier = pll_fdiv;
			dev_data->ddr_pll_frequency = resulting_frequency;
			source_pll = xlnx_zynq_clk_source_ddr_pll;
		} else if (clock_id == xlnx_zynq_clk_iopll) {
			dev_data->io_pll_multiplier = pll_fdiv;
			dev_data->io_pll_frequency = resulting_frequency;
			source_pll = xlnx_zynq_clk_source_io_pll;
		}

		/*
		 * Now that the specified PLL has been re-configured, all peripheral clocks driven
		 * by this PLL must be re-calculated -> apply the current divisor(s) to the new
		 * PLL clock frequency
		 */
		for (clock_iter = 0; clock_iter <= xlnx_zynq_clk_dbg_apb; clock_iter++) {
			if (dev_data->peripheral_clocks[clock_iter].source_pll == source_pll) {
				LOG_INF("due to clock ID %u (%s) update: updating dependent "
					"clock %u (%s) as well", (uint32_t)clock_id,
					clock_data->clk_name, clock_iter,
					dev_data->peripheral_clocks[clock_iter].clk_name);
				dev_data->peripheral_clocks[clock_iter].clk_frequency =
					resulting_frequency /
					dev_data->peripheral_clocks[clock_iter].divisor1 /
					dev_data->peripheral_clocks[clock_iter].divisor0;
				LOG_INF("new frequency of clock ID %u (%s): %u div1 %u "
					"div0 %u = %u", (uint32_t)clock_iter,
					dev_data->peripheral_clocks[clock_iter].clk_name,
					resulting_frequency,
					dev_data->peripheral_clocks[clock_iter].divisor1,
					dev_data->peripheral_clocks[clock_iter].divisor0,
					dev_data->peripheral_clocks[clock_iter].clk_frequency);
			}
		}

		return 0; /* skip the standard case register write behind the switch (clock_id) */
	case xlnx_zynq_clk_fclk0:
	case xlnx_zynq_clk_fclk1:
	case xlnx_zynq_clk_fclk2:
	case xlnx_zynq_clk_fclk3:
		/*
		 * Special case: FCLK[0..3] -> modify divisors only if the respective FCLK
		 * is specified as enabled in the device tree.
		 */
		uint32_t fclk_enable_shift = (uint32_t)clock_id - (uint32_t)xlnx_zynq_clk_fclk0;
		bool fclk_enabled = (dev_cfg->fclk_enable >> fclk_enable_shift) & 0x1;

		if (!fclk_enabled) {
			LOG_ERR("clock ID %u (%s) is not enabled via the device tree's fclk-"
				"enable bit mask", (uint32_t)clock_id, clock_data->clk_name);
			return -EINVAL;
		}

		reg_val &= ~(PERIPH_CLK_DIVISOR_MASK << PERIPH_CLK_DIVISOR1_SHIFT);
		reg_val &= ~(PERIPH_CLK_DIVISOR_MASK << PERIPH_CLK_DIVISOR0_SHIFT);
		reg_val |= (divisor1 & PERIPH_CLK_DIVISOR_MASK) << PERIPH_CLK_DIVISOR1_SHIFT;
		reg_val |= (divisor0 & PERIPH_CLK_DIVISOR_MASK) << PERIPH_CLK_DIVISOR0_SHIFT;
		break;
	case xlnx_zynq_clk_ddr2x:
	case xlnx_zynq_clk_ddr3x:
		/* DDR_CLK_CTRL has a different register layout, 2x DIV0 in one register */
		reg_val &= ~(PERIPH_CLK_DIVISOR_MASK << ((clock_id == xlnx_zynq_clk_ddr2x) ?
			   DDR_DDR2X_CLK_DIVISOR_SHIFT : DDR_DDR3X_CLK_DIVISOR_SHIFT));
		reg_val |= ((divisor0 & PERIPH_CLK_DIVISOR_MASK) << ((clock_id ==
			   xlnx_zynq_clk_ddr2x) ? DDR_DDR2X_CLK_DIVISOR_SHIFT :
			   DDR_DDR3X_CLK_DIVISOR_SHIFT));
		break;
	case xlnx_zynq_clk_dci:
	case xlnx_zynq_clk_gem0:
	case xlnx_zynq_clk_gem1:
	case xlnx_zynq_clk_can0:
	case xlnx_zynq_clk_can1:
		reg_val &= ~(PERIPH_CLK_DIVISOR_MASK << PERIPH_CLK_DIVISOR1_SHIFT);
		reg_val &= ~(PERIPH_CLK_DIVISOR_MASK << PERIPH_CLK_DIVISOR0_SHIFT);
		reg_val |= ((divisor1 & PERIPH_CLK_DIVISOR_MASK) << PERIPH_CLK_DIVISOR1_SHIFT);
		reg_val |= ((divisor0 & PERIPH_CLK_DIVISOR_MASK) << PERIPH_CLK_DIVISOR0_SHIFT);
		break;
	case xlnx_zynq_clk_lqspi:
	case xlnx_zynq_clk_smc:
	case xlnx_zynq_clk_pcap:
	case xlnx_zynq_clk_sdio0:
	case xlnx_zynq_clk_sdio1:
	case xlnx_zynq_clk_uart0:
	case xlnx_zynq_clk_uart1:
	case xlnx_zynq_clk_spi0:
	case xlnx_zynq_clk_spi1:
	case xlnx_zynq_clk_dbg_trc:
		reg_val &= ~(PERIPH_CLK_DIVISOR_MASK << PERIPH_CLK_DIVISOR0_SHIFT);
		reg_val |= ((divisor0 & PERIPH_CLK_DIVISOR_MASK) << PERIPH_CLK_DIVISOR0_SHIFT);
		break;
	default:
		/*
		 * Applies to: cpu_6or4x, cpu_3or2x, cpu_2x, cpu_1x, dma, all ..._aper, dbg_apb:
		 * -> these clocks are either directly derived from the frequency of the
		 * PLL driving the CPU or are driven by one of the scaled-down CPU clocks
		 * (dma = cpu_2x, ..._aper = cpu_1x) and therefore cannot be configured
		 * individually.
		 */
		LOG_ERR("clock ID %u (%s) is not supported by this function",
			(uint32_t)clock_id, clock_data->clk_name);
		return -EINVAL;
	}

	err = xlnx_zynq_ps7_clkc_clkctrl_off(dev, sys);
	if (err != 0) {
		LOG_ERR("disable clock %u ID (%s) prior to divisor adjustment failed",
			(uint32_t)clock_id, clock_data->clk_name);
		return err;
	}

	err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + reg_offset, reg_val);
	if (err != 0) {
		LOG_ERR("write control register failed for clock ID %u (%s)",
			(uint32_t)clock_id, clock_data->clk_name);
		return -EIO;
	}

	clock_data->divisor1 = divisor1;
	clock_data->divisor0 = divisor0;
	clock_data->clk_frequency = resulting_frequency;

	if (clock_id >= xlnx_zynq_clk_can0 && clock_id <= xlnx_zynq_clk_spi1) {
		struct xlnx_zynq_ps7_clkc_peripheral_clock *other_inst_clock_data;

		LOG_WRN("changed the divisor(s) for clock ID %u (%s) - this also affects the "
			"other instance of the same peripheral!", (uint32_t)clock_id,
			clock_data->clk_name);

		/*
		 * If the current clock ID is divisible by 2 only with remainder, the clock ID
		 * provided to this function refers to instance 0 of the current peripheral,
		 * e.g. can0. can1 now has the same divisors and clock frequency post control
		 * register update triggered by the modification of can0. If the clock ID is
		 * divisible without remainder, instance 1 is current and instance 0 must be
		 * updated here. -> based on: xlnx_zynq_clk_can0 = clock ID 19.
		 */
		other_inst_clock_data = &dev_data->peripheral_clocks[(clock_id % 2 != 0) ?
			(uint32_t)clock_id + 1 : (uint32_t)clock_id - 1];
		other_inst_clock_data->divisor1 = divisor1;
		other_inst_clock_data->divisor0 = divisor0;
		other_inst_clock_data->clk_frequency = resulting_frequency;
	}

	err = xlnx_zynq_ps7_clkc_clkctrl_on(dev, sys);
	if (err != 0) {
		LOG_ERR("re-enable clock ID %u (%s) post divisor adjustment failed",
			(uint32_t)clock_id, clock_data->clk_name);
		return err;
	}

	LOG_INF("set clock ID %u (%s) to frequency %u OK", (uint32_t)clock_id,
		clock_data->clk_name, target_frequency);

	return 0;
}

static int xlnx_zynq_ps7_clkc_clock_control_configure(const struct device *dev,
						      clock_control_subsys_t sys,
						      void *data)
{
	struct xlnx_zynq_ps7_clkc_peripheral_clock *other_inst_clock_data = NULL;
	bool active_pre = false;
	bool active_pre_other_inst = false;
	uint32_t reg_offset;
	uint32_t reg2_offset;
	uint32_t pll_frequency;
	int err;

	__ASSERT(dev != NULL, "device pointer is NULL");
	if (dev == NULL) {
		LOG_ERR("device pointer is NULL");
		return -EINVAL;
	}

	struct xlnx_zynq_ps7_clkc_clock_control_data *dev_data = dev->data;
	struct xlnx_zynq_ps7_clkc_clock_control_configuration *clock_configuration_data =
		(struct xlnx_zynq_ps7_clkc_clock_control_configuration *)data;
	enum xlnx_zynq_ps7_clkc_clock_identifier clock_id =
		(enum xlnx_zynq_ps7_clkc_clock_identifier)sys;

	__ASSERT(clock_id <= xlnx_zynq_clk_dbg_apb, "clock ID %u is out of range",
		(uint32_t)clock_id);
	if (clock_id > xlnx_zynq_clk_dbg_apb) {
		LOG_ERR("clock ID %u is out of range", (uint32_t)clock_id);
		return -EINVAL;
	}

	struct xlnx_zynq_ps7_clkc_peripheral_clock *clock_data =
		&dev_data->peripheral_clocks[(uint32_t)clock_id];

	__ASSERT(clock_id == clock_data->peripheral_clock_id,
		 "data inconsistency: clock ID %u resolves clock data struct for clock ID %u",
		 (uint32_t)clock_id, (uint32_t)clock_data->peripheral_clock_id);
	if (clock_id != clock_data->peripheral_clock_id) {
		LOG_ERR("data inconsistency: clock ID %u resolves clock data struct for clock "
			"ID %u", (uint32_t)clock_id, (uint32_t)clock_data->peripheral_clock_id);
		return -EINVAL;
	}

	__ASSERT(clock_configuration_data->divisor1 >= 1 &&
		clock_configuration_data->divisor1 <= PERIPH_CLK_DIVISOR_MASK,
		"divisor1 value %u for clock ID %u (%s) is out of range",
		clock_configuration_data->divisor1, (uint32_t)clock_id, clock_data->clk_name);
	if (clock_configuration_data->divisor1 < 1 ||
	    clock_configuration_data->divisor1 > PERIPH_CLK_DIVISOR_MASK) {
		LOG_ERR("divisor1 value %u for clock ID %u (%s) is out of range",
			clock_configuration_data->divisor1, (uint32_t)clock_id,
			clock_data->clk_name);
		return -EINVAL;
	}
	__ASSERT(clock_configuration_data->divisor0 >= 1 &&
		clock_configuration_data->divisor0 <= PERIPH_CLK_DIVISOR_MASK,
		"divisor0 value %u for clock ID %u (%s) is out of range",
		clock_configuration_data->divisor0, (uint32_t)clock_id,	clock_data->clk_name);
	if (clock_configuration_data->divisor0 < 1 ||
	    clock_configuration_data->divisor0 > PERIPH_CLK_DIVISOR_MASK) {
		LOG_ERR("divisor0 value %u for clock ID %u (%s) is out of range",
			clock_configuration_data->divisor0, (uint32_t)clock_id,
			clock_data->clk_name);
		return -EINVAL;
	}

	/* Performed all parameter error checks -> break out if running in QEMU */
	if (IS_ENABLED(CONFIG_QEMU_TARGET)) {
		LOG_DBG("Skipping clock configure call: register space unavailable in QEMU");
		return 0;
	}

	if (clock_configuration_data->source_pll == xlnx_zynq_clk_source_emio_clk) {
		__ASSERT(clock_configuration_data->emio_clock_frequency != 0,
			 "clock ID %u (%s) is to be re-configured to source = EMIO clock, but "
			 "the EMIO clock frequency value is zero", (uint32_t)clock_id,
			 clock_data->clk_name);
		if (clock_configuration_data->emio_clock_frequency == 0) {
			LOG_ERR("clock ID %u (%s) is to be re-configured to source = EMIO clock, "
				"but the EMIO clock frequency value is zero", (uint32_t)clock_id,
				clock_data->clk_name);
			return -EINVAL;
		}

		struct xlnx_zynq_ps7_clkc_emio_clock_source_explicit *emio_clock_configuration =
			(clock_id == xlnx_zynq_clk_gem0) ? &explicit_config_emio_clock_data[0] :
			(clock_id == xlnx_zynq_clk_gem1) ? &explicit_config_emio_clock_data[1] :
			(clock_id == xlnx_zynq_clk_dbg_trc) ? &explicit_config_emio_clock_data[2] :
			NULL;

		__ASSERT(emio_clock_configuration != NULL,
			"failed to allocate struct for clock ID %u (%s) EMIO clock data",
			(uint32_t)clock_id, clock_data->clk_name);
		if (emio_clock_configuration == NULL) {
			LOG_ERR("failed to allocate struct for clock ID %u (%s) EMIO clock data",
				(uint32_t)clock_id, clock_data->clk_name);
			return -EIO;
		}

		emio_clock_configuration->emio_clk_frequency =
			clock_configuration_data->emio_clock_frequency;

		clock_data->emio_clock_source.explicit_config = emio_clock_configuration;
	} else {
		/*
		 * Clear EMIO reference in case of, for example, gem0 having been initially
		 * configured for clock source EMIO based on a fixed-clock device tree node,
		 * but is now being re-configured to be driven by one of the ARM/DDR/IO PLLs.
		 */
		clock_data->emio_clock_source.explicit_config = NULL;
	}

	/*
	 * Break out if the specified clock cannot be re-configured:
	 * - the specified clock is one of the three base PLLs
	 * - the specified clock has a fixed source PLL, e.g. the ddr_... clocks
	 * - the specified clock is tied to one specific clock, e.g. all the _aper clocks.
	 */
	switch (clock_id) {
	case xlnx_zynq_clk_armpll:
	case xlnx_zynq_clk_ddrpll:
	case xlnx_zynq_clk_iopll:
	case xlnx_zynq_clk_cpu_6or4x:
	case xlnx_zynq_clk_cpu_3or2x:
	case xlnx_zynq_clk_cpu_2x:
	case xlnx_zynq_clk_cpu_1x:
	case xlnx_zynq_clk_ddr2x:
	case xlnx_zynq_clk_ddr3x:
	case xlnx_zynq_clk_dci:
	case xlnx_zynq_clk_dma:
	case xlnx_zynq_clk_usb0_aper:
	case xlnx_zynq_clk_usb1_aper:
	case xlnx_zynq_clk_gem0_aper:
	case xlnx_zynq_clk_gem1_aper:
	case xlnx_zynq_clk_sdio0_aper:
	case xlnx_zynq_clk_sdio1_aper:
	case xlnx_zynq_clk_spi0_aper:
	case xlnx_zynq_clk_spi1_aper:
	case xlnx_zynq_clk_can0_aper:
	case xlnx_zynq_clk_can1_aper:
	case xlnx_zynq_clk_i2c0_aper:
	case xlnx_zynq_clk_i2c1_aper:
	case xlnx_zynq_clk_uart0_aper:
	case xlnx_zynq_clk_uart1_aper:
	case xlnx_zynq_clk_gpio_aper:
	case xlnx_zynq_clk_lqspi_aper:
	case xlnx_zynq_clk_smc_aper:
	case xlnx_zynq_clk_dbg_apb:
		LOG_ERR("source PLL of clock %u (%s) is not re-configurable or re-configuration "
			"would prevent the system from running", (uint32_t)clock_id,
			clock_data->clk_name);
		return -EINVAL;
	default:
		break;
	}

	xlnx_zynq_ps7_clkc_get_register_offset(clock_id, &reg_offset, &reg2_offset);

	if (clock_id >= xlnx_zynq_clk_can0 && clock_id <= xlnx_zynq_clk_spi1) {
		/*
		 * Store a pointer to the 2nd instance of the same peripheral if both instances
		 * share the source PLL and divisor configuration. If so, the 2nd instance must
		 * equally be disabled prior to the re-configuration and re-enabled post the re-
		 * configuration, and the updated source PLL, divisor and clock frequency
		 * information must be stored in the 2nd instance configuration data as well.
		 * Comp. xlnx_zynq_ps7_clkc_clkctrl_set_rate(), where the logic of the 2nd
		 * instance addressing is explained.
		 */
		other_inst_clock_data = &dev_data->peripheral_clocks[(clock_id % 2 != 0) ?
			(uint32_t)clock_id + 1 : (uint32_t)clock_id - 1];
	}

	/*
	 * Turn off the clock during re-config, unless it's one of the FCLKs which have no
	 * explicit enable bits.
	 */
	if (dev_data->peripheral_clocks[(uint32_t)clock_id].active &&
	    (!(clock_id >= xlnx_zynq_clk_fclk0 && clock_id <= xlnx_zynq_clk_fclk3))) {
		active_pre = true;
		err = xlnx_zynq_ps7_clkc_clkctrl_off(dev, sys);
		if (err != 0) {
			LOG_ERR("disable clock %u (%s) prior to PLL and divisor adjustment failed",
				(uint32_t)clock_id, clock_data->clk_name);
			return err;
		}
	}
	if (other_inst_clock_data != NULL && other_inst_clock_data->active) {
		/* The 2nd instance issue doesn't apply to FCLKx -> skip above check */
		active_pre_other_inst = true;
		clock_control_subsys_t other_sys =
			(clock_control_subsys_t)other_inst_clock_data->peripheral_clock_id;
		err = xlnx_zynq_ps7_clkc_clkctrl_off(dev, other_sys);
		if (err != 0) {
			LOG_ERR("disable dependent clock %u (%s) prior to PLL and divisor "
				"adjustment failed",
				(uint32_t)other_inst_clock_data->peripheral_clock_id,
				other_inst_clock_data->clk_name);
			return err;
		}
	}

	switch (clock_id) {
	case xlnx_zynq_clk_fclk0:
	case xlnx_zynq_clk_fclk1:
	case xlnx_zynq_clk_fclk2:
	case xlnx_zynq_clk_fclk3:
	case xlnx_zynq_clk_dci:
	case xlnx_zynq_clk_gem0:
	case xlnx_zynq_clk_gem1:
	case xlnx_zynq_clk_can0:
	case xlnx_zynq_clk_can1:
		/* These peripherals support DIVISOR1 and DIVISOR0 */
		err = xlnx_zynq_ps7_clkc_set_clk_ctrl_data(dev, reg_offset,
			(clock_id == xlnx_zynq_clk_gem0 || clock_id == xlnx_zynq_clk_gem1) ?
			&reg2_offset : NULL, &clock_configuration_data->divisor1,
			clock_configuration_data->divisor0, clock_configuration_data->source_pll);
		if (err != 0) {
			LOG_ERR("failed to re-configure clock %u (%s)", (uint32_t)clock_id,
				clock_data->clk_name);
			return err;
		}

		clock_data->divisor1 = clock_configuration_data->divisor1;
		clock_data->divisor0 = clock_configuration_data->divisor0;
		break;
	default:
		/* All other supported peripherals only support DIVISOR0 */
		err = xlnx_zynq_ps7_clkc_set_clk_ctrl_data(dev, reg_offset, NULL,
			NULL, clock_configuration_data->divisor0,
			clock_configuration_data->source_pll);
		if (err != 0) {
			LOG_ERR("failed to re-configure clock %u (%s)", (uint32_t)clock_id,
				clock_data->clk_name);
			return err;
		}

		clock_data->divisor1 = 1;
		clock_data->divisor0 = clock_configuration_data->divisor0;

		if (clock_configuration_data->divisor1 != 1) {
			LOG_WRN("clock %u (%s) supports only divisor0, so divisor1 should be "
				"set to 1 when calling this function. Current value %u is being "
				"overridden", (uint32_t)clock_id, clock_data->clk_name,
				clock_configuration_data->divisor1);
		}

		break;
	}

	clock_data->source_pll = clock_configuration_data->source_pll;

	if (clock_data->emio_clock_source.dt_config == NULL) {
		pll_frequency = (clock_data->source_pll == xlnx_zynq_clk_source_arm_pll) ?
				dev_data->arm_pll_frequency : (clock_data->source_pll ==
				xlnx_zynq_clk_source_ddr_pll) ? dev_data->ddr_pll_frequency :
				dev_data->io_pll_frequency;
		clock_data->clk_frequency = pll_frequency / clock_data->divisor1 /
					    clock_data->divisor0;
	} else {
		clock_data->clk_frequency =
			clock_data->emio_clock_source.dt_config->emio_clk_frequency /
			clock_data->divisor1 / clock_data->divisor0;
	}

	if (other_inst_clock_data != NULL) {
		other_inst_clock_data->source_pll = clock_data->source_pll;
		other_inst_clock_data->divisor1 = clock_data->divisor1;
		other_inst_clock_data->divisor0 = clock_data->divisor0;
		other_inst_clock_data->clk_frequency = clock_data->clk_frequency;
	}

	if (active_pre &&
	    (!(clock_id >= xlnx_zynq_clk_fclk0 && clock_id <= xlnx_zynq_clk_fclk3))) {
		err = xlnx_zynq_ps7_clkc_clkctrl_on(dev, sys);
		if (err != 0) {
			LOG_ERR("re-enable clock %u (%s) post PLL and divisor adjustment failed",
				(uint32_t)clock_id, clock_data->clk_name);
			return err;
		}
	}
	if (other_inst_clock_data != NULL && active_pre_other_inst) {
		clock_control_subsys_t other_sys =
			(clock_control_subsys_t)other_inst_clock_data->peripheral_clock_id;
		err = xlnx_zynq_ps7_clkc_clkctrl_on(dev, other_sys);
		if (err != 0) {
			LOG_ERR("re-enable dependent clock %u (%s) post PLL and divisor adjustment "
				"failed", (uint32_t)other_inst_clock_data->peripheral_clock_id,
				other_inst_clock_data->clk_name);
			return err;
		}
	}

	LOG_INF("set clock ID %u (%s) to source %s OK", (uint32_t)clock_id, clock_data->clk_name,
		(clock_data->source_pll == xlnx_zynq_clk_source_arm_pll) ? "ARM PLL" :
		(clock_data->source_pll == xlnx_zynq_clk_source_ddr_pll) ? "DDR PLL" :
		(clock_data->source_pll == xlnx_zynq_clk_source_io_pll) ? "I/O PLL" :
		"EMIO");
	if (clock_data->source_pll == xlnx_zynq_clk_source_emio_clk) {
		LOG_INF("EMIO clock frequency %u", clock_configuration_data->emio_clock_frequency);
	}
	LOG_INF("divisor1 = %u, divisor0 = %u", clock_data->divisor1, clock_data->divisor0);

	return 0;
}

#define PERIPHERAL_CLOCK_ENTRY(node_id, prop, idx) \
{ \
	.active = false, \
	.parent_pll_stopped = false, \
	.divisor1 = 1, \
	.divisor0 = 1, \
	.source_pll = xlnx_zynq_clk_source_io_pll, \
	.clk_frequency = 0, \
	.peripheral_clock_id = (enum xlnx_zynq_ps7_clkc_clock_identifier)idx, \
	.clk_name = STRINGIFY(DT_STRING_TOKEN_BY_IDX(node_id, clock_output_names, idx)), \
	.emio_clock_source.dt_config = NULL, \
},

#define EMIO_CLK_ENTRY(node_id, prop, idx) \
{ \
	.emio_clk_frequency = DT_PROP(DT_PHANDLE_BY_IDX(node_id, clocks, idx), \
			      clock_frequency), \
	.peripheral_clock_id = (enum xlnx_zynq_ps7_clkc_clock_identifier)DT_DEP_ORD( \
			       DT_PHANDLE_BY_IDX(node_id, clocks, idx)),\
	.emio_clk_name = STRINGIFY(DT_STRING_TOKEN_BY_IDX(node_id, clock_names, idx)), \
},

static const struct xlnx_zynq_ps7_clkc_clock_control_config xlnx_zynq_ps7_clkc_clkctrl0_cfg = {
	.slcr = DEVICE_DT_GET(DT_INST_PHANDLE(0, syscon)),
	.base_address = DT_INST_REG_ADDR(0),
	.ps_clk_frequency = DT_INST_PROP(0, ps_clk_frequency),
	.fclk_enable = DT_INST_PROP_OR(0, fclk_enable, 0),
#if DT_INST_PROP_HAS_IDX(0, clocks, 0)
	.emio_clocks_count = DT_INST_PROP_LEN(0, clocks),
	.emio_clock_sources_dt = {DT_INST_FOREACH_PROP_ELEM(0, clocks, EMIO_CLK_ENTRY)},
#else
	.emio_clocks_count = 0,
	.emio_clock_sources_dt = {},
#endif
};

static struct xlnx_zynq_ps7_clkc_clock_control_data xlnx_zynq_ps7_clkc_clkctrl0_data = {
	.peripheral_clocks = {DT_INST_FOREACH_PROP_ELEM(0, clock_output_names,
			      PERIPHERAL_CLOCK_ENTRY)},
	.arm_pll_multiplier = 0,
	.arm_pll_frequency = 0,
	.ddr_pll_multiplier = 0,
	.ddr_pll_frequency = 0,
	.io_pll_multiplier = 0,
	.io_pll_frequency = 0,
	.clk_scheme_621 = false,
	.cpu_1x_active = false,
	.cpu_2x_active = false,
	.cpu_6x4x_active = false,
	.cpu_3x2x_active = false,
	.cpu_divisor = 0,
	.cpu_6x4x_frequency = 0,
	.cpu_3x2x_frequency = 0,
	.cpu_2x_frequency = 0,
	.cpu_1x_frequency = 0,
	.ddr_2x_active = false,
	.ddr_3x_active = false,
	.ddr_2x_frequency = 0,
	.ddr_3x_frequency = 0
};

static int xlnx_zynq_ps7_clkc_clkctrl_init(const struct device *dev)
{
	const struct xlnx_zynq_ps7_clkc_clock_control_config *dev_cfg = dev->config;
	struct xlnx_zynq_ps7_clkc_clock_control_data *dev_data = dev->data;
	uint32_t reg_val;
	uint32_t pll_frequency;
	int err;
	uint32_t clock_iter;

	if (IS_ENABLED(CONFIG_QEMU_TARGET)) {
		LOG_DBG("Skipping driver initialization: register space unavailable in QEMU");
		return 0;
	}

	if (!device_is_ready(dev_cfg->slcr)) {
		LOG_ERR("SLCR syscon device not ready");
		return -ENODEV;
	}

	/*
	 * Acquire the FBDIV values applied to fOSC for the 3 base PLLs: ARM, DDR, IO.
	 * Calculate & store the resulting frequencies
	 */
	err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + ARM_PLL_CTRL_OFFSET,
			      &reg_val);
	if (err != 0) {
		return -EIO;
	}

	dev_data->arm_pll_multiplier = (reg_val >> PLL_FDIV_SHIFT) & PLL_FDIV_MASK;
	dev_data->arm_pll_frequency = dev_cfg->ps_clk_frequency * dev_data->arm_pll_multiplier;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_armpll].active = true;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_armpll].source_pll =
		xlnx_zynq_clk_source_arm_pll;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_armpll].clk_frequency =
		dev_data->arm_pll_frequency;

	err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + DDR_PLL_CTRL_OFFSET,
			      &reg_val);
	if (err != 0) {
		return -EIO;
	}

	dev_data->ddr_pll_multiplier = (reg_val >> PLL_FDIV_SHIFT) & PLL_FDIV_MASK;
	dev_data->ddr_pll_frequency = dev_cfg->ps_clk_frequency * dev_data->ddr_pll_multiplier;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_ddrpll].active = true;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_ddrpll].source_pll =
		xlnx_zynq_clk_source_ddr_pll;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_ddrpll].clk_frequency =
		dev_data->ddr_pll_frequency;

	err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + IO_PLL_CTRL_OFFSET,
			      &reg_val);
	if (err != 0) {
		return -EIO;
	}

	dev_data->io_pll_multiplier = (reg_val >> PLL_FDIV_SHIFT) & PLL_FDIV_MASK;
	dev_data->io_pll_frequency = dev_cfg->ps_clk_frequency * dev_data->io_pll_multiplier;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_iopll].active = true;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_iopll].source_pll =
		xlnx_zynq_clk_source_io_pll;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_iopll].clk_frequency =
		dev_data->io_pll_frequency;

	/* Get the active CPU clock divisor scheme */
	err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + CLK_621_TRUE_OFFSET,
			      &reg_val);
	if (err != 0) {
		return -EIO;
	}

	reg_val = (reg_val >> CLK_SCHEME_621_SHIFT) & CLK_SCHEME_621_MASK;
	dev_data->clk_scheme_621 = reg_val ? true : false;
	if (dev_data->clk_scheme_621) {
		dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_6or4x].divisor1 = 6;
		dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_3or2x].divisor1 = 3;
	} else {
		dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_6or4x].divisor1 = 4;
		dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_3or2x].divisor1 = 2;
	}

	/* Acquire the active CPU clock configuration */
	err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + ARM_CLK_CTRL_OFFSET,
			      &reg_val);
	if (err != 0) {
		return -EIO;
	}

	dev_data->cpu_6x4x_active = ((reg_val >> ARM_CPU6X4X_ACTIVE_SHIFT) & ARM_CLK_ACTIVE_MASK)
				    ? true : false;
	dev_data->cpu_3x2x_active = ((reg_val >> ARM_CPU3X2X_ACTIVE_SHIFT) & ARM_CLK_ACTIVE_MASK)
				    ? true : false;
	dev_data->cpu_2x_active = ((reg_val >> ARM_CPU2X_ACTIVE_SHIFT) & ARM_CLK_ACTIVE_MASK)
				  ? true : false;
	dev_data->cpu_1x_active = ((reg_val >> ARM_CPU1X_ACTIVE_SHIFT) & ARM_CLK_ACTIVE_MASK)
				  ? true : false;

	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_6or4x].active =
		dev_data->cpu_6x4x_active;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_3or2x].active =
		dev_data->cpu_3x2x_active;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_2x].active =
		dev_data->cpu_2x_active;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_1x].active =
		dev_data->cpu_1x_active;

	dev_data->cpu_divisor = (reg_val >> ARM_CLK_DIVISOR_SHIFT) & ARM_CLK_DIVISOR_MASK;
	dev_data->cpu_source_pll = (reg_val >> ARM_CLK_SOURCE_SHIFT) & ARM_CLK_SOURCE_MASK;

	/*
	 * Store the information which PLL drives cpu_6x4x, cpu_3x2x, cpu_2x and cpu_1x.
	 * As a few of the clocks, namely all of the AMBA peripheral clocks (xxx_aper bindings),
	 * the DMA engine and the Debug APB clock are driven by either cpu_2x or cpu_1x,
	 * store the source PLL information for them as well.
	 */
	if (dev_data->cpu_source_pll == ARM_CLK_SOURCE_DDR_PLL) {
		dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_6or4x].source_pll =
		dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_3or2x].source_pll =
		dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_2x].source_pll =
		dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_1x].source_pll =
			xlnx_zynq_clk_source_ddr_pll;
		pll_frequency = dev_data->ddr_pll_frequency;
	} else if (dev_data->cpu_source_pll == ARM_CLK_SOURCE_IO_PLL) {
		dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_6or4x].source_pll =
		dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_3or2x].source_pll =
		dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_2x].source_pll =
		dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_1x].source_pll =
			xlnx_zynq_clk_source_io_pll;
		pll_frequency = dev_data->io_pll_frequency;
	} else {
		dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_6or4x].source_pll =
		dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_3or2x].source_pll =
		dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_2x].source_pll =
		dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_1x].source_pll =
			xlnx_zynq_clk_source_arm_pll;
		pll_frequency = dev_data->arm_pll_frequency;
	}

	dev_data->cpu_6x4x_frequency = pll_frequency / dev_data->cpu_divisor;
	dev_data->cpu_3x2x_frequency = dev_data->cpu_6x4x_frequency / 2;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_6or4x].clk_frequency =
		dev_data->cpu_6x4x_frequency;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_3or2x].clk_frequency =
		dev_data->cpu_3x2x_frequency;

	if (dev_data->clk_scheme_621) {
		dev_data->cpu_1x_frequency = dev_data->cpu_6x4x_frequency / 6;
		dev_data->cpu_2x_frequency = dev_data->cpu_6x4x_frequency / 3;
	} else {
		dev_data->cpu_1x_frequency = dev_data->cpu_6x4x_frequency / 4;
		dev_data->cpu_2x_frequency = dev_data->cpu_6x4x_frequency / 2;
	}
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_2x].clk_frequency =
		dev_data->cpu_2x_frequency;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_cpu_1x].clk_frequency =
		dev_data->cpu_1x_frequency;

	/* Get the active DDR2X/DDR3X clock configuration -> always driven by DDR PLL */
	err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + DDR_CLK_CTRL_OFFSET,
			      &reg_val);
	if (err != 0) {
		return -EIO;
	}

	dev_data->ddr_2x_active = ((reg_val >> DDR_DDR2X_ACTIVE_SHIFT) & DDR_CLK_ACTIVE_MASK);
	dev_data->ddr_3x_active = ((reg_val >> DDR_DDR3X_ACTIVE_SHIFT) & DDR_CLK_ACTIVE_MASK);

	dev_data->ddr_2x_frequency = dev_data->ddr_pll_frequency /
		((reg_val >> DDR_DDR2X_CLK_DIVISOR_SHIFT) & PERIPH_CLK_DIVISOR_MASK);
	dev_data->ddr_3x_frequency = dev_data->ddr_pll_frequency /
		((reg_val >> DDR_DDR3X_CLK_DIVISOR_SHIFT) & PERIPH_CLK_DIVISOR_MASK);

	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_ddr2x].source_pll =
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_ddr3x].source_pll =
		xlnx_zynq_clk_source_ddr_pll;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_ddr2x].clk_frequency =
		dev_data->ddr_2x_frequency;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_ddr3x].clk_frequency =
		dev_data->ddr_3x_frequency;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_ddr2x].active =
		dev_data->ddr_2x_active;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_ddr3x].active =
		dev_data->ddr_3x_active;

	/* Get the active DDR DCI clock configuration -> always driven by DDR PLL */
	err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + DCI_CLK_CTRL_OFFSET,
			      &reg_val);
	if (err != 0) {
		return -EIO;
	}

	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_dci].source_pll =
		xlnx_zynq_clk_source_ddr_pll;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_dci].divisor1 =
		(reg_val >> PERIPH_CLK_DIVISOR1_SHIFT) & PERIPH_CLK_DIVISOR_MASK;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_dci].divisor0 =
		(reg_val >> PERIPH_CLK_DIVISOR0_SHIFT) & PERIPH_CLK_DIVISOR_MASK;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_dci].clk_frequency =
		dev_data->ddr_pll_frequency
		/ dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_dci].divisor1
		/ dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_dci].divisor0;
	dev_data->peripheral_clocks[(uint32_t)xlnx_zynq_clk_dci].active =
		(reg_val & PERIPH_CLK_CLKACT0_BIT);

	/*
	 * Set the respective enable bits in the APER_CLK_CTRL (AMBA Peripheral
	 * Clock Control) register for all supported peripherals that are enabled
	 * for the current target via the device tree. If the AMBA clock is not
	 * enabled for the respective peripheral, any access to its register space
	 * from within the respective device driver will cause an exception.
	 *
	 * Also controlled via the APER_CLK_CTRL register is the AMBA clock for
	 * the DMA controller, which is always driven by cpu_2x. The source PLL and
	 * frequency information for the DMA clock have already been set above.
	 * -> Just read this clock's current state.
	 */
	err = syscon_read_reg(dev_cfg->slcr, dev_cfg->base_address + APER_CLK_CTRL_OFFSET,
			      &reg_val);

	if (err != 0) {
		return -EIO;
	}

	/* Add further enable bits here once the corresponding device drivers exist */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(psgpio), okay)
	reg_val |= APER_CLK_CTRL_GPIO_CLKACT_BIT;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay)
	reg_val |= APER_CLK_CTRL_UART1_CLKACT_BIT;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay)
	reg_val |= APER_CLK_CTRL_UART0_CLKACT_BIT;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gem1), okay)
	reg_val |= APER_CLK_CTRL_GEM1_CLKACT_BIT;
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gem0), okay)
	reg_val |= APER_CLK_CTRL_GEM0_CLKACT_BIT;
#endif

	err = syscon_write_reg(dev_cfg->slcr, dev_cfg->base_address + APER_CLK_CTRL_OFFSET,
			       reg_val);
	if (err != 0) {
		return -EIO;
	}

	/* Populate the peripheral clocks (incl. _APER) array with the current configuration */
	for (clock_iter = (uint32_t)xlnx_zynq_clk_lqspi; clock_iter <=
	     (uint32_t)xlnx_zynq_clk_dbg_apb; clock_iter++) {
		err = xlnx_zynq_ps7_clkc_read_current_config(dev,
			(enum xlnx_zynq_ps7_clkc_clock_identifier)clock_iter,
			&dev_data->peripheral_clocks[clock_iter]);
		if (err != 0) {
			return err;
		}
	}

	return 0;
}

static const struct clock_control_driver_api xlnx_zynq_ps7_clkc_clkctrl_api = {
	.on = xlnx_zynq_ps7_clkc_clkctrl_on,
	.off = xlnx_zynq_ps7_clkc_clkctrl_off,
	.get_rate = xlnx_zynq_ps7_clkc_clkctrl_get_rate,
	.get_status = xlnx_zynq_ps7_clkc_clkctrl_get_status,
	.set_rate = xlnx_zynq_ps7_clkc_clkctrl_set_rate,
	.configure = xlnx_zynq_ps7_clkc_clock_control_configure
};

DEVICE_DT_DEFINE(DT_NODELABEL(clkctrl0), &xlnx_zynq_ps7_clkc_clkctrl_init, NULL,
	&xlnx_zynq_ps7_clkc_clkctrl0_data, &xlnx_zynq_ps7_clkc_clkctrl0_cfg, PRE_KERNEL_1,
	CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &xlnx_zynq_ps7_clkc_clkctrl_api);
