/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include "clock_control_renesas_rza2m_cpg_lld.h"

static const struct rza2m_stb_module_info gs_stbcr[] = {
	{MODULE_CORESIGHT, STBCR2_OFFSET, CPG_STBCR2_MSTP20},
	{MODULE_OSTM0, STBCR3_OFFSET, CPG_STBCR3_MSTP36},
	{MODULE_OSTM1, STBCR3_OFFSET, CPG_STBCR3_MSTP35},
	{MODULE_OSTM2, STBCR3_OFFSET, CPG_STBCR3_MSTP34},
	{MODULE_MTU3, STBCR3_OFFSET, CPG_STBCR3_MSTP33},
	{MODULE_CANFD, STBCR3_OFFSET, CPG_STBCR3_MSTP32},
	{MODULE_ADC, STBCR5_OFFSET, CPG_STBCR5_MSTP57},
	{MODULE_GPT, STBCR3_OFFSET, CPG_STBCR3_MSTP30},
	{MODULE_SCIFA0, STBCR4_OFFSET, CPG_STBCR4_MSTP47},
	{MODULE_SCIFA1, STBCR4_OFFSET, CPG_STBCR4_MSTP46},
	{MODULE_SCIFA2, STBCR4_OFFSET, CPG_STBCR4_MSTP45},
	{MODULE_SCIFA3, STBCR4_OFFSET, CPG_STBCR4_MSTP44},
	{MODULE_SCIFA4, STBCR4_OFFSET, CPG_STBCR4_MSTP43},
	{MODULE_SCI0, STBCR4_OFFSET, CPG_STBCR4_MSTP42},
	{MODULE_SCI1, STBCR4_OFFSET, CPG_STBCR4_MSTP41},
	{MODULE_IrDA, STBCR4_OFFSET, CPG_STBCR4_MSTP40},
	{MODULE_CEU, STBCR5_OFFSET, CPG_STBCR5_MSTP56},
	{MODULE_RTC0, STBCR5_OFFSET, CPG_STBCR5_MSTP53},
	{MODULE_RTC1, STBCR5_OFFSET, CPG_STBCR5_MSTP52},
	{MODULE_JCU, STBCR5_OFFSET, CPG_STBCR5_MSTP51},
	{MODULE_VIN, STBCR6_OFFSET, CPG_STBCR6_MSTP66},
	{MODULE_ETHER, STBCR6_OFFSET,
	 CPG_STBCR6_MSTP65 | CPG_STBCR6_MSTP64 | CPG_STBCR6_MSTP63 | CPG_STBCR6_MSTP62},
	{MODULE_USB0, STBCR6_OFFSET, CPG_STBCR6_MSTP61},
	{MODULE_USB1, STBCR6_OFFSET, CPG_STBCR6_MSTP60},
	{MODULE_IMR2, STBCR7_OFFSET, CPG_STBCR7_MSTP77},
	{MODULE_DRW, STBCR7_OFFSET, CPG_STBCR7_MSTP76},
	{MODULE_MIPI, STBCR7_OFFSET, CPG_STBCR7_MSTP75},
	{MODULE_SSIF0, STBCR7_OFFSET, CPG_STBCR7_MSTP73},
	{MODULE_SSIF1, STBCR7_OFFSET, CPG_STBCR7_MSTP72},
	{MODULE_SSIF2, STBCR7_OFFSET, CPG_STBCR7_MSTP71},
	{MODULE_SSIF3, STBCR7_OFFSET, CPG_STBCR7_MSTP70},
	{MODULE_I2C0, STBCR8_OFFSET, CPG_STBCR8_MSTP87},
	{MODULE_I2C1, STBCR8_OFFSET, CPG_STBCR8_MSTP86},
	{MODULE_I2C2, STBCR8_OFFSET, CPG_STBCR8_MSTP85},
	{MODULE_I2C3, STBCR8_OFFSET, CPG_STBCR8_MSTP84},
	{MODULE_SPIBSC, STBCR8_OFFSET, CPG_STBCR8_MSTP83},
	{MODULE_VDC6, STBCR8_OFFSET, CPG_STBCR8_MSTP81},
	{MODULE_RSPI0, STBCR9_OFFSET, CPG_STBCR9_MSTP97},
	{MODULE_RSPI1, STBCR9_OFFSET, CPG_STBCR9_MSTP96},
	{MODULE_RSPI2, STBCR9_OFFSET, CPG_STBCR9_MSTP95},
	{MODULE_HYPERBUS, STBCR9_OFFSET, CPG_STBCR9_MSTP93},
	{MODULE_OCTAMEM, STBCR9_OFFSET, CPG_STBCR9_MSTP92},
	{MODULE_RSPDIF, STBCR9_OFFSET, CPG_STBCR9_MSTP91},
	{MODULE_DRP, STBCR9_OFFSET, CPG_STBCR9_MSTP90},
	{MODULE_TSIP, STBCR10_OFFSET, CPG_STBCR10_MSTP107},
	{MODULE_NAND, STBCR10_OFFSET, CPG_STBCR10_MSTP104},
	{MODULE_SDMMC0, STBCR10_OFFSET, CPG_STBCR10_MSTP103 | CPG_STBCR10_MSTP102},
	{MODULE_SDMMC1, STBCR10_OFFSET, CPG_STBCR10_MSTP101 | CPG_STBCR10_MSTP100},
};

static const struct rza2m_stb_module_info gs_stbreq[] = {
	{MODULE_CORESIGHT, STBREQ1_OFFSET, CPG_STBREQ1_STBRQ15},
	{MODULE_CEU, STBREQ1_OFFSET, CPG_STBREQ1_STBRQ10},
	{MODULE_JCU, STBREQ1_OFFSET, CPG_STBREQ1_STBRQ13},
	{MODULE_VIN, STBREQ2_OFFSET, CPG_STBREQ2_STBRQ27},
	{MODULE_ETHER, STBREQ2_OFFSET, CPG_STBREQ2_STBRQ26},
	{MODULE_USB0, STBREQ3_OFFSET, CPG_STBREQ3_STBRQ31 | CPG_STBREQ3_STBRQ30},
	{MODULE_USB1, STBREQ3_OFFSET, CPG_STBREQ3_STBRQ33 | CPG_STBREQ3_STBRQ32},
	{MODULE_IMR2, STBREQ2_OFFSET, CPG_STBREQ2_STBRQ23},
	{MODULE_DRW, STBREQ2_OFFSET, CPG_STBREQ2_STBRQ21 | CPG_STBREQ2_STBRQ20},
	{MODULE_VDC6, STBREQ2_OFFSET, CPG_STBREQ2_STBRQ25},
	{MODULE_DRP, STBREQ2_OFFSET, CPG_STBREQ2_STBRQ24},
	{MODULE_NAND, STBREQ2_OFFSET, CPG_STBREQ2_STBRQ22},
	{MODULE_SDMMC0, STBREQ1_OFFSET, CPG_STBREQ1_STBRQ12},
	{MODULE_SDMMC1, STBREQ1_OFFSET, CPG_STBREQ1_STBRQ11},
};

static const struct rza2m_stb_module_info gs_stback[] = {
	{MODULE_CORESIGHT, STBACK1_OFFSET, CPG_STBACK1_STBAK15},
	{MODULE_CEU, STBACK1_OFFSET, CPG_STBACK1_STBAK10},
	{MODULE_JCU, STBACK1_OFFSET, CPG_STBACK1_STBAK13},
	{MODULE_VIN, STBACK2_OFFSET, CPG_STBACK2_STBAK27},
	{MODULE_ETHER, STBACK2_OFFSET, CPG_STBACK2_STBAK26},
	{MODULE_USB0, STBACK3_OFFSET, CPG_STBACK3_STBAK31 | CPG_STBACK3_STBAK30},
	{MODULE_USB1, STBACK3_OFFSET, CPG_STBACK3_STBAK33 | CPG_STBACK3_STBAK32},
	{MODULE_IMR2, STBACK2_OFFSET, CPG_STBACK2_STBAK23},
	{MODULE_DRW, STBACK2_OFFSET, CPG_STBACK2_STBAK21 | CPG_STBACK2_STBAK20},
	{MODULE_VDC6, STBACK2_OFFSET, CPG_STBACK2_STBAK25},
	{MODULE_DRP, STBACK2_OFFSET, CPG_STBACK2_STBAK24},
	{MODULE_NAND, STBACK2_OFFSET, CPG_STBACK2_STBAK22},
	{MODULE_SDMMC0, STBACK1_OFFSET, CPG_STBACK1_STBAK12},
	{MODULE_SDMMC1, STBACK1_OFFSET, CPG_STBACK1_STBAK11},
};

void rza2m_pl310_set_standby_mode(void)
{
	uint32_t reg_val;

	/* Read current register value */
	reg_val = sys_read32(PL310_BASE_ADDR + PL310_PWR_CTRL_OFFSET);

	/* Set standby mode enable bit */
	reg_val |= BIT(PL310_PWR_CTRL_STANDBY_MODE_EN_SHIFT);

	sys_write32(reg_val, PL310_BASE_ADDR + PL310_PWR_CTRL_OFFSET);
	sys_read32(PL310_BASE_ADDR + PL310_PWR_CTRL_OFFSET);
}

void rza2m_cpg_calculate_pll_frequency(const struct device *dev)
{
	const struct rza2m_cpg_clock_config *config = dev->config;
	struct rza2m_cpg_clock_data *data = dev->data;

	data->cpg_extal_frequency_hz = config->cpg_extal_freq_hz_cfg;

	if ((data->cpg_extal_frequency_hz >= RZA2M_CPG_MHZ(10)) &&
	    (data->cpg_extal_frequency_hz <= RZA2M_CPG_MHZ(12))) {
		data->cpg_pll_frequency_hz = data->cpg_extal_frequency_hz * 88;
	} else if ((data->cpg_extal_frequency_hz >= RZA2M_CPG_MHZ(20)) &&
		   (data->cpg_extal_frequency_hz <= RZA2M_CPG_MHZ(24))) {
		data->cpg_pll_frequency_hz = data->cpg_extal_frequency_hz * 44;
	}
}

static void rza2m_cpg_change_reg_bits(mem_addr_t reg, uint8_t bitmask, bool enable)
{
	uint8_t reg_val;

	reg_val = sys_read8(reg);

	if (enable) {
		reg_val &= ~bitmask;
	} else {
		reg_val |= bitmask;
	}

	sys_write8(reg_val, reg);
	sys_read8(reg);
}

static const struct rza2m_stb_module_info *rza2m_cpg_get_stbcr_info(enum rza2m_stb_module module)
{
	size_t index;
	size_t count = ARRAY_SIZE(gs_stbcr);

	for (index = 0; index < count; index++) {
		if (module == gs_stbcr[index].module) {
			return &gs_stbcr[index];
		}
	}

	/* Return NULL if not found */
	return NULL;
}

static const struct rza2m_stb_module_info *rza2m_cpg_get_stbreq_info(enum rza2m_stb_module module)
{
	size_t index;
	size_t count = ARRAY_SIZE(gs_stbreq);

	for (index = 0; index < count; index++) {
		if (module == gs_stbreq[index].module) {
			return &gs_stbreq[index];
		}
	}

	/* Return NULL if not found */
	return NULL;
}

static const struct rza2m_stb_module_info *rza2m_cpg_get_stback_info(enum rza2m_stb_module module)
{
	size_t index;
	size_t count = ARRAY_SIZE(gs_stback);

	for (index = 0; index < count; index++) {
		if (module == gs_stback[index].module) {
			return &gs_stback[index];
		}
	}

	/* Return NULL if not found */
	return NULL;
}

static uint8_t rza2m_cpg_wait_bit_val(uint32_t reg_addr, uint8_t bit_mask, uint8_t bits_val,
				      int32_t us_wait)
{
	uint8_t reg_val;
	int32_t wait_cnt = (us_wait / 5);

	do {
		reg_val = sys_read8(reg_addr) & bit_mask;

		if (reg_val == bits_val) {
			break;
		}

		if (wait_cnt > 0) {
			k_busy_wait(5);
		}
	} while (wait_cnt-- > 0);

	return reg_val;
}

int rza2m_cpg_mstp_clock_endisable(const struct device *dev, enum rza2m_stb_module module,
				   bool enable)
{
	uint8_t reg_val = 0;
	const struct rza2m_stb_module_info *p_stbcr;
	const struct rza2m_stb_module_info *p_stbreq;
	const struct rza2m_stb_module_info *p_stback;

	/* Obtain the STBCR information */
	p_stbcr = rza2m_cpg_get_stbcr_info(module);

	/* Check if unsupported module */
	if (NULL != p_stbcr) {
		rza2m_cpg_change_reg_bits(CPG_REG_ADDR(p_stbcr->reg_offset), p_stbcr->mask, enable);
	} else {
		return -ENOTSUP;
	}

	p_stbreq = rza2m_cpg_get_stbreq_info(module);
	p_stback = rza2m_cpg_get_stback_info(module);

	if ((NULL != p_stback) && (NULL != p_stbreq)) {
		rza2m_cpg_change_reg_bits(CPG_REG_ADDR(p_stbreq->reg_offset), p_stbreq->mask,
					  enable);
		reg_val = rza2m_cpg_wait_bit_val(CPG_REG_ADDR(p_stback->reg_offset), p_stbreq->mask,
						 0, STBACK_REG_WAIT_US);
		if (reg_val) {
			return -EIO;
		}
	}

	return 0;
}

static int rza2m_cpg_modify_frqcr(const struct device *dev, enum rza2m_cp_sub_clock clk_sub_src,
				  uint32_t sub_clk_frequency_hz, uint16_t *p_frqcr)
{
	struct rza2m_cpg_clock_data *data = dev->data;
	uint16_t div_d;
	uint16_t fc;

	/* Avoid divide by zero */
	if (sub_clk_frequency_hz == 0) {
		return -EINVAL;
	}

	div_d = data->cpg_pll_frequency_hz / sub_clk_frequency_hz;

	if (CPG_SUB_CLOCK_ICLK == clk_sub_src) {
		if (div_d == 2) {
			fc = 0;
		} else if (div_d == 4) {
			fc = 1;
		} else if (div_d == 8) {
			fc = 2;
		} else if (div_d == 16) {
			fc = 3;
		} else {
			return -EINVAL;
		}
		/* Clear IFC bit */
		(*p_frqcr) &= (uint16_t)(~CPG_FRQCR_IFC);

		/* Modify IFC bit */
		(*p_frqcr) |= (uint16_t)(fc << CPG_FRQCR_IFC_SHIFT);
	} else if (CPG_SUB_CLOCK_BCLK == clk_sub_src) {
		if (div_d == 8) {
			fc = 1;
		} else if (div_d == 16) {
			fc = 2;
		} else if (div_d == 32) {
			fc = 3;
		} else {
			return -EINVAL;
		}

		/* Clear BFC bit */
		(*p_frqcr) &= (uint16_t)(~CPG_FRQCR_BFC);

		/* Modify BFC bit */
		(*p_frqcr) |= (uint16_t)(fc << CPG_FRQCR_BFC_SHIFT);
	} else if (CPG_SUB_CLOCK_P1CLK == clk_sub_src) {
		if (div_d == 16) {
			fc = 2;
		} else if (div_d == 32) {
			fc = 3;
		} else {
			return -EINVAL;
		}

		/* Clear PFC bit */
		(*p_frqcr) &= (uint16_t)(~CPG_FRQCR_PFC);

		/* Modify PFC bit */
		(*p_frqcr) |= (uint16_t)(fc << CPG_FRQCR_PFC_SHIFT);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

int rza2m_cpg_set_sub_clock_divider(const struct device *dev, enum rza2m_cp_sub_clock clk_sub_src,
				    uint32_t sub_clk_frequency_hz)
{
	struct rza2m_cpg_clock_data *data = dev->data;
	uint16_t frqcr;
	uint16_t check_frqcr;
	int ret;

	/* Read previous settings */
	frqcr = sys_read16(CPG_REG_ADDR(CPG_FRQCR_OFFSET));

	ret = rza2m_cpg_modify_frqcr(dev, clk_sub_src, sub_clk_frequency_hz, &frqcr);

	if (ret < 0) {
		return ret;
	}

	/* Check unless valid combination */
	check_frqcr = frqcr & (CPG_FRQCR_IFC | CPG_FRQCR_BFC | CPG_FRQCR_PFC);
	switch (check_frqcr) {
	case 0x012: /* "VALID":do nothing, fall through */
	case 0x112: /* "VALID":do nothing, fall through */
	case 0x212: /* "VALID":do nothing, fall through */
	case 0x322: /* "VALID":do nothing, fall through */
	case 0x333: /* "VALID":do nothing, fall through */
		break;
	default:
		return !EINVAL;
	}

	/* Update local divisor variables based on the clock type */
	switch (clk_sub_src) {
	case CPG_SUB_CLOCK_ICLK:
		switch ((frqcr & CPG_FRQCR_IFC) >> CPG_FRQCR_IFC_SHIFT) {
		case 0:
			data->cpg_iclk_divisor = 2;
			break;
		case 1:
			data->cpg_iclk_divisor = 4;
			break;
		case 2:
			data->cpg_iclk_divisor = 8;
			break;
		case 3:
			data->cpg_iclk_divisor = 16;
			break;
		default:
			break;
		}
		data->cpg_iclk_frequency_hz = data->cpg_pll_frequency_hz / data->cpg_iclk_divisor;
		break;

	case CPG_SUB_CLOCK_BCLK:
		switch ((frqcr & CPG_FRQCR_BFC) >> CPG_FRQCR_BFC_SHIFT) {
		case 1:
			data->cpg_bclk_divisor = 8;
			break;
		case 2:
			data->cpg_bclk_divisor = 16;
			break;
		case 3:
			data->cpg_bclk_divisor = 32;
			break;
		default:
			break;
		}
		data->cpg_bclk_frequency_hz = data->cpg_pll_frequency_hz / data->cpg_bclk_divisor;
		break;

	case CPG_SUB_CLOCK_P1CLK:
		switch ((frqcr & CPG_FRQCR_PFC) >> CPG_FRQCR_PFC_SHIFT) {
		case 2:
			data->cpg_p1clk_divisor = 16;
			break;
		case 3:
			data->cpg_p1clk_divisor = 32;
			break;
		default:
			break;
		}
		data->cpg_p1clk_frequency_hz = data->cpg_pll_frequency_hz / data->cpg_p1clk_divisor;
		break;

	default:
		return -ENOTSUP;
	}

	rza2m_pl310_set_standby_mode();
	sys_write16(frqcr, CPG_REG_ADDR(CPG_FRQCR_OFFSET));
	sys_read16(CPG_REG_ADDR(CPG_FRQCR_OFFSET));

	return 0;
}

int rza2m_cpg_get_clock(const struct device *dev, enum rza2m_cpg_get_freq_src src, uint32_t *p_freq)
{
	struct rza2m_cpg_clock_data *data = dev->data;

	if (NULL == p_freq) {
		return -EINVAL;
	}

	switch (src) {
	case CPG_FREQ_EXTAL:
		*p_freq = data->cpg_extal_frequency_hz;
		break;
	case CPG_FREQ_ICLK:
		*p_freq = data->cpg_iclk_frequency_hz;
		break;
	case CPG_FREQ_GCLK:
		*p_freq = (data->cpg_pll_frequency_hz * 2) / data->cpg_bclk_divisor;
		break;
	case CPG_FREQ_BCLK:
		*p_freq = data->cpg_bclk_frequency_hz;
		break;
	case CPG_FREQ_P1CLK:
		*p_freq = data->cpg_p1clk_frequency_hz;
		break;
	case CPG_FREQ_P0CLK:
		*p_freq = data->cpg_pll_frequency_hz / 32;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
