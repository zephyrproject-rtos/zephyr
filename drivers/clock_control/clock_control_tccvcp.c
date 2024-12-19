/*
 * Copyright 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT tcc_ccu

#include <errno.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_tcc_ccu);

#include <zephyr/drivers/clock_control/clock_control_tcc_ccu.h>
#include <zephyr/drivers/clock_control/clock_control_tccvcp.h>

#include <soc_reg_phys.h>

/*
 * DEFINITIONS
 *
 */

struct clock_tcc_config {
	const struct device *syscon;
};

#define DEV_CFG(dev) ((const struct clock_tcc_config *const)(dev)->config)

/*
 * LOCAL VARIABLES
 *
 */

static uint32_t micom_clock_source[CLOCK_SRC_MAX_NUM];

/*
 * FUNCTION PROTOTYPES
 *
 */

static void clock_dev_write_pll(uint32_t uiReg, uint32_t en, uint32_t p, uint32_t m, uint32_t s);

static void clock_dev_write_pclk_ctrl(uint32_t uiReg, uint32_t md, uint32_t en, uint32_t sel,
				      uint32_t uiDiv, uint32_t uiType);

static void clock_dev_write_clk_ctrl(uint32_t uiReg, uint32_t en, uint32_t conf, uint32_t sel);

static long clock_dev_find_pms(struct clock_pms *psPll, uint32_t srcFreq);

static long clock_dev_set_pll_rate(uint32_t uiReg, uint32_t uiRate);

static uint32_t clock_dev_get_pll_rate(uint32_t uiReg);

static uint32_t clock_dev_get_pll_div(long iId);

static long clock_dev_find_clk_ctrl(struct clock_clk_ctrl *CLKCTRL);

static uint32_t clock_dev_cal_pclk_div(const struct clock_pclk_ctrl *psPclkCtrl,
				       uint32_t *puiClkDiv, const uint32_t srcClk,
				       uint32_t uiDivMax);

static long clock_dev_find_pclk(struct clock_pclk_ctrl *psPclkCtrl,
				       enum clock_pclk_ctrl_type eType);

static void clock_dev_reset_clk_src(long iId);

/* PLL Configuration Macro */
static void clock_dev_write_pll(uint32_t reg, uint32_t en, uint32_t p, uint32_t m, uint32_t s)
{
	const uint32_t timeout = 4000000000UL;
	uint32_t delay, try_count;

	delay = 0;
	try_count = 0;

	if (reg != 0UL) {
		if (en != 0UL) {
			sys_write32(0UL | (1UL << (uint32_t)CLOCK_PLL_LOCKEN_SHIFT) |
					    (2UL << (uint32_t)CLOCK_PLL_CHGPUMP_SHIFT) |
					    (((s) & (uint32_t)CLOCK_PLL_S_MASK)
					     << (uint32_t)CLOCK_PLL_S_SHIFT) |
					    (((m) & (uint32_t)CLOCK_PLL_M_MASK)
					     << (uint32_t)CLOCK_PLL_M_SHIFT) |
					    (((p) & (uint32_t)CLOCK_PLL_P_MASK)
					     << (uint32_t)CLOCK_PLL_P_SHIFT),
				    reg);

			for (delay = 100UL; delay > 0UL; delay--) {
				; /* if cpu clokc is 1HGz then loop 100. */
			}

			sys_write32(sys_read32(reg) |
					((en & 1UL) << (uint32_t)CLOCK_PLL_EN_SHIFT),
				    reg);

			while ((sys_read32(reg) & (1UL << (uint32_t)CLOCK_PLL_LOCKST_SHIFT)) ==
			       0UL) {
				try_count++;

				if (try_count > timeout) {
					break;
				}
			}
		} else {
			sys_write32((uint32_t)(sys_read32(reg)) &
					    (~(1UL << (uint32_t)CLOCK_PLL_EN_SHIFT)),
				    reg);
		}
	}
}

static void clock_dev_write_pclk_ctrl(uint32_t reg, uint32_t md, uint32_t en, uint32_t sel,
				      uint32_t divider, uint32_t type)
{
	if (type == (uint32_t)CLOCK_PCLKCTRL_TYPE_XXX) {
		sys_write32(sys_read32(reg) & ~(1UL << (uint32_t)CLOCK_PCLKCTRL_OUTEN_SHIFT), reg);
		sys_write32(sys_read32(reg) & ~(1UL << (uint32_t)CLOCK_PCLKCTRL_EN_SHIFT), reg);
		sys_write32((sys_read32(reg) & ~((uint32_t)CLOCK_PCLKCTRL_SEL_MASK
						 << (uint32_t)CLOCK_PCLKCTRL_SEL_SHIFT)),
			    reg);
		sys_write32((sys_read32(reg) & ~((uint32_t)CLOCK_PCLKCTRL_DIV_XXX_MASK
						 << (uint32_t)CLOCK_PCLKCTRL_DIV_SHIFT)),
			    reg);

		sys_write32((sys_read32(reg) | ((divider & (uint32_t)CLOCK_PCLKCTRL_DIV_XXX_MASK)
						<< (uint32_t)CLOCK_PCLKCTRL_DIV_SHIFT)),
			    reg);
		sys_write32((sys_read32(reg) | ((sel & (uint32_t)CLOCK_PCLKCTRL_SEL_MASK)
						<< (uint32_t)CLOCK_PCLKCTRL_SEL_SHIFT)),
			    reg);
		sys_write32((sys_read32(reg) | ((en & 1UL) << (uint32_t)CLOCK_PCLKCTRL_EN_SHIFT)),
			    reg);
		sys_write32(
			(sys_read32(reg) | ((en & 1UL) << (uint32_t)CLOCK_PCLKCTRL_OUTEN_SHIFT)),
			reg);
	} else if (type == (uint32_t)CLOCK_PCLKCTRL_TYPE_YYY) {
		sys_write32(sys_read32(reg) & ~(1UL << (uint32_t)CLOCK_PCLKCTRL_EN_SHIFT), reg);
		sys_write32((sys_read32(reg) & ~((uint32_t)CLOCK_PCLKCTRL_DIV_YYY_MASK
						 << (uint32_t)CLOCK_PCLKCTRL_DIV_SHIFT)) |
				    ((divider & (uint32_t)CLOCK_PCLKCTRL_DIV_YYY_MASK)
				     << (uint32_t)CLOCK_PCLKCTRL_DIV_SHIFT),
			    reg);
		sys_write32((sys_read32(reg) & ~((uint32_t)CLOCK_PCLKCTRL_SEL_MASK
						 << (uint32_t)CLOCK_PCLKCTRL_SEL_SHIFT)) |
				    ((sel & (uint32_t)CLOCK_PCLKCTRL_SEL_MASK)
				     << (uint32_t)CLOCK_PCLKCTRL_SEL_SHIFT),
			    reg);
		sys_write32((sys_read32(reg) & ~(1UL << (uint32_t)CLOCK_PCLKCTRL_MD_SHIFT)) |
				    ((md & 1UL) << (uint32_t)CLOCK_PCLKCTRL_MD_SHIFT),
			    reg);
		sys_write32((sys_read32(reg) & ~(1UL << (uint32_t)CLOCK_PCLKCTRL_EN_SHIFT)) |
				    ((en & 1UL) << (uint32_t)CLOCK_PCLKCTRL_EN_SHIFT),
			    reg);
	}
}

static void clock_dev_write_clk_ctrl(uint32_t reg, uint32_t en, uint32_t conf, uint32_t sel)
{
	const uint32_t timeout = 4000000000;
	uint32_t cur_conf, try_count;

	cur_conf = (sys_read32(reg) >> (uint32_t)CLOCK_MCLKCTRL_CONFIG_SHIFT) &
		   (uint32_t)CLOCK_MCLKCTRL_CONFIG_MASK;

	if (conf >= cur_conf) {
		sys_write32((sys_read32(reg) & (~((uint32_t)CLOCK_MCLKCTRL_CONFIG_MASK
						  << (uint32_t)CLOCK_MCLKCTRL_CONFIG_SHIFT))) |
				    ((conf & (uint32_t)CLOCK_MCLKCTRL_CONFIG_MASK)
				     << (uint32_t)CLOCK_MCLKCTRL_CONFIG_SHIFT),
			    reg);
		try_count = 0;

		while ((sys_read32(reg) & (1UL << (uint32_t)CLOCK_MCLKCTRL_CLKCHG_SHIFT)) != 0UL) {
			try_count++;

			if (try_count > timeout) {
				break;
			}
		}

		sys_write32((sys_read32(reg) & (~((uint32_t)CLOCK_MCLKCTRL_SEL_MASK
						  << (uint32_t)CLOCK_MCLKCTRL_SEL_SHIFT))) |
				    ((sel & (uint32_t)CLOCK_MCLKCTRL_SEL_MASK)
				     << (uint32_t)CLOCK_MCLKCTRL_SEL_SHIFT),
			    reg);
		try_count = 0;

		while ((sys_read32(reg) & (1UL << (uint32_t)CLOCK_MCLKCTRL_CLKCHG_SHIFT)) != 0UL) {
			try_count++;

			if (try_count > timeout) {
				break;
			}
		}
	} else {
		sys_write32((sys_read32(reg) & (~((uint32_t)CLOCK_MCLKCTRL_SEL_MASK
						  << (uint32_t)CLOCK_MCLKCTRL_SEL_SHIFT))) |
				    ((sel & (uint32_t)CLOCK_MCLKCTRL_SEL_MASK)
				     << (uint32_t)CLOCK_MCLKCTRL_SEL_SHIFT),
			    reg);

		try_count = 0;
		while ((sys_read32(reg) & (1UL << (uint32_t)CLOCK_MCLKCTRL_CLKCHG_SHIFT)) != 0UL) {
			try_count++;

			if (try_count > timeout) {
				break;
			}
		}

		sys_write32((sys_read32(reg) & (~((uint32_t)CLOCK_MCLKCTRL_CONFIG_MASK
						  << (uint32_t)CLOCK_MCLKCTRL_CONFIG_SHIFT))) |
				    ((conf & (uint32_t)CLOCK_MCLKCTRL_CONFIG_MASK)
				     << (uint32_t)CLOCK_MCLKCTRL_CONFIG_SHIFT),
			    reg);

		try_count = 0;
		while ((sys_read32(reg) & (1UL << (uint32_t)CLOCK_MCLKCTRL_CLKCHG_SHIFT)) != 0UL) {
			try_count++;

			if (try_count > timeout) {
				break;
			}
		}
	}

	if (en != 0UL) {
		sys_write32((sys_read32(reg) & (~(1UL << (uint32_t)CLOCK_MCLKCTRL_EN_SHIFT))) |
				    ((en & 1UL) << (uint32_t)CLOCK_MCLKCTRL_EN_SHIFT),
			    reg);

		try_count = 0;
		while ((sys_read32(reg) & (1UL << (uint32_t)CLOCK_MCLKCTRL_DIVSTS_SHIFT)) != 0UL) {
			try_count++;

			if (try_count > timeout) {
				break;
			}
		}
	}
}

static long clock_dev_find_pms(struct clock_pms *pll_ptr, uint32_t src_freq)
{
	unsigned long long pll, src, fvco;
	unsigned long long srch_p, srch_m, temp, src_pll;
	uint32_t err, srch_err;
	long srch_s;

	if (pll_ptr == NULL) {
		return 0;
	}

	if (pll_ptr->fpll == 0UL) {
		pll_ptr->en = 0;
		return 0;
	}

	pll = (unsigned long long)pll_ptr->fpll;
	src = (unsigned long long)src_freq;

	err = 0xFFFFFFFFUL;
	srch_err = 0xFFFFFFFFUL;

	for (srch_s = (long)CLOCK_PLL_S_MIN; srch_s <= (long)CLOCK_PLL_S_MAX;
	     srch_s++) {
		fvco = pll << (unsigned long long)srch_s;

		if ((fvco >= (unsigned long long)CLOCK_PLL_VCO_MIN) &&
		    (fvco <= (unsigned long long)CLOCK_PLL_VCO_MAX)) {
			for (srch_p = (unsigned long long)CLOCK_PLL_P_MIN;
			     srch_p <= (unsigned long long)CLOCK_PLL_P_MAX; srch_p++) {
				srch_m = fvco * srch_p;
				soc_div64_to_32(&(srch_m), (uint32_t)src_freq, NULL);

				if ((srch_m < (unsigned long long)CLOCK_PLL_M_MIN) ||
				    (srch_m > (unsigned long long)CLOCK_PLL_M_MAX)) {
					continue;
				}

				temp = srch_m * src;
				soc_div64_to_32(&(temp), (uint32_t)srch_p, NULL);
				src_pll = (temp >> (unsigned long long)srch_s);

				if ((src_pll < (unsigned long long)CLOCK_PLL_MIN_RATE) ||
				    (src_pll > (unsigned long long)CLOCK_PLL_MAX_RATE)) {
					continue;
				}

				srch_err = (uint32_t)((src_pll > pll) ? (src_pll - pll)
								      : (pll - src_pll));

				if (srch_err < err) {
					err = srch_err;
					pll_ptr->p = (uint32_t)srch_p;
					pll_ptr->m = (uint32_t)srch_m;
					pll_ptr->s = (uint32_t)srch_s;
				}
			}
		}
	}

	if (err == 0xFFFFFFFFUL) {
		return -EIO;
	}

	temp = src * (unsigned long long)pll_ptr->m;
	soc_div64_to_32(&(temp), (uint32_t)(pll_ptr->p), NULL);
	pll_ptr->fpll = (uint32_t)(temp >> pll_ptr->s);
	pll_ptr->en = 1;

	return 0;
}

static long clock_dev_set_pll_rate(uint32_t reg, uint32_t rate)
{
	struct clock_pms pll;
	unsigned long long cal_m;
	long err;

	err = 0;
	pll.fpll = rate;

	if (clock_dev_find_pms(&pll, CLOCK_XIN_CLK_RATE) != 0L) {
		cal_m = (unsigned long long)CLOCK_PLL_P_MIN * (unsigned long long)CLOCK_PLL_VCO_MIN;
		cal_m += (unsigned long long)CLOCK_XIN_CLK_RATE;
		cal_m /= (unsigned long long)CLOCK_XIN_CLK_RATE;
		clock_dev_write_pll(reg, 0UL, (uint32_t)CLOCK_PLL_P_MIN, (uint32_t)cal_m,
				    (uint32_t)CLOCK_PLL_S_MIN);
		err = -EIO;
	} else {
		clock_dev_write_pll(reg, pll.en, pll.p, pll.m, pll.s);
	}

	return err;
}

static uint32_t clock_dev_get_pll_rate(uint32_t reg)
{
	uint32_t reg_val;
	struct clock_pms pll_cfg;
	unsigned long long temp;

	if (reg == 0UL) {
		return 0;
	}

	reg_val = sys_read32(reg);

	pll_cfg.p = (reg_val >> (uint32_t)CLOCK_PLL_P_SHIFT) & ((uint32_t)CLOCK_PLL_P_MASK);
	pll_cfg.m = (reg_val >> (uint32_t)CLOCK_PLL_M_SHIFT) & ((uint32_t)CLOCK_PLL_M_MASK);
	pll_cfg.s = (reg_val >> (uint32_t)CLOCK_PLL_S_SHIFT) & ((uint32_t)CLOCK_PLL_S_MASK);
	pll_cfg.en = (reg_val >> (uint32_t)CLOCK_PLL_EN_SHIFT) & (1UL);
	pll_cfg.src = (reg_val >> (uint32_t)CLOCK_PLL_SRC_SHIFT) & ((uint32_t)CLOCK_PLL_SRC_MASK);
	temp = (unsigned long long)CLOCK_XIN_CLK_RATE * (unsigned long long)pll_cfg.m;
	soc_div64_to_32(&temp, (uint32_t)(pll_cfg.p), NULL);

	return (uint32_t)((temp) >> pll_cfg.s);
}

static uint32_t clock_dev_get_pll_div(long id)
{
	uint32_t ret;
	CLOCKMpll_t m_pll_id;
	uint32_t reg, offset, reg_val;

	ret = 0;
	m_pll_id = (CLOCKMpll_t)id;
	reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_CLKDIVC;
	reg_val = 0;

	switch (m_pll_id) {
	case CLOCK_MPLL_0:
	case CLOCK_MPLL_1:
		offset = (3UL - (uint32_t)id) * 8UL;
		break;
	case CLOCK_MPLL_XIN:
		offset = 8UL;
		break;
	default:
		offset = 0UL;
		break;
	}

	reg_val = sys_read32(reg);

	if (offset != 0UL) {
		ret = ((reg_val >> offset) & 0x3FUL);
	}

	return ret;
}

static uint32_t clock_dev_cal_pclk_div(const struct clock_pclk_ctrl *pclk_ctrl_ptr,
				       uint32_t *clk_div_ptr, const uint32_t src_clk,
				       uint32_t div_max)
{
	uint32_t clk_rate1, clk_rate2;
	uint32_t err1, err2;

	if (pclk_ctrl_ptr == NULL) {
		return 0;
	}

	if (clk_div_ptr == NULL) {
		return 0;
	}

	if (src_clk <= pclk_ctrl_ptr->freq) {
		*clk_div_ptr = 1UL;
	} else {
		*clk_div_ptr = src_clk / pclk_ctrl_ptr->freq;
	}

	if ((*clk_div_ptr) > div_max) {
		*clk_div_ptr = div_max;
	}

	clk_rate1 = (src_clk) / (*clk_div_ptr);
	clk_rate2 =
		(src_clk) / (((*clk_div_ptr) < div_max) ? ((*clk_div_ptr) + 1UL) : (*clk_div_ptr));
	err1 = (clk_rate1 > pclk_ctrl_ptr->freq) ? (clk_rate1 - pclk_ctrl_ptr->freq)
						 : (pclk_ctrl_ptr->freq - clk_rate1);
	err2 = (clk_rate2 > pclk_ctrl_ptr->freq) ? (clk_rate2 - pclk_ctrl_ptr->freq)
						 : (pclk_ctrl_ptr->freq - clk_rate2);

	if (err1 > err2) {
		*clk_div_ptr += 1UL;
	}

	return (err1 < err2) ? err1 : err2;
}

static long clock_dev_find_pclk(struct clock_pclk_ctrl *pclk_ctrl_ptr,
				       enum clock_pclk_ctrl_type type)
{
	long ret, last_idx, idx;
	uint32_t div_max, srch_src, err_div, md;
	uint32_t divider[CLOCK_SRC_MAX_NUM];
	uint32_t err[CLOCK_SRC_MAX_NUM];
	uint32_t div_div;
	enum clock_pclk_ctrl_mode pclk_sel;

	div_div = CLOCK_PCLKCTRL_DIV_MIN;
	pclk_sel = CLOCK_PCLKCTRL_MODE_DIVIDER;
	ret = 0;

	if (pclk_ctrl_ptr == NULL) {
		return -EINVAL;
	}

	pclk_ctrl_ptr->md = (uint32_t)pclk_sel;
	div_max = CLOCK_PCLKCTRL_DIV_XXX_MAX;

	memset((void *)divider, 0x00U, sizeof(divider));
	srch_src = 0xFFFFFFFFUL;
	last_idx = (long)CLOCK_SRC_MAX_NUM - 1L;

	for (idx = last_idx; idx >= 0L; idx--) {
		if (micom_clock_source[idx] == 0UL) {
			continue;
		}

		if ((micom_clock_source[idx] >= (uint32_t)CLOCK_PCLKCTRL_MAX_FCKS) &&
		    (type == CLOCK_PCLKCTRL_TYPE_XXX)) {
			continue;
		}

		/* divider mode */
		err_div = clock_dev_cal_pclk_div(
			pclk_ctrl_ptr, &div_div,
			micom_clock_source[idx], /*CLOCK_PCLKCTRL_DIV_MIN+1, \*/
			div_max + 1UL);

		err[idx] = err_div;
		divider[idx] = div_div;
		md = (uint32_t)pclk_sel;

		if (srch_src == 0xFFFFFFFFUL) {
			srch_src = (uint32_t)idx;
			pclk_ctrl_ptr->md = md;
		} else {
			/* find similar clock */
			if (err[idx] <= err[srch_src]) {
				srch_src = (uint32_t)idx;
				pclk_ctrl_ptr->md = md;
			}
		}
	}

	switch (srch_src) {
	case (uint32_t)CLOCK_MPLL_0:
		pclk_ctrl_ptr->sel = (uint32_t)CLOCK_MPCLKCTRL_SEL_PLL0;
		break;
	case (uint32_t)CLOCK_MPLL_1:
		pclk_ctrl_ptr->sel = (uint32_t)CLOCK_MPCLKCTRL_SEL_PLL1;
		break;
	case (uint32_t)CLOCK_MPLL_DIV_0:
		pclk_ctrl_ptr->sel = (uint32_t)CLOCK_MPCLKCTRL_SEL_PLL0DIV;
		break;
	case (uint32_t)CLOCK_MPLL_DIV_1:
		pclk_ctrl_ptr->sel = (uint32_t)CLOCK_MPCLKCTRL_SEL_PLL1DIV;
		break;
	case (uint32_t)CLOCK_MPLL_XIN:
		pclk_ctrl_ptr->sel = (uint32_t)CLOCK_MPCLKCTRL_SEL_XIN;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret != 0) {
		return ret;
	}

	pclk_ctrl_ptr->div_val = divider[srch_src];

	if ((pclk_ctrl_ptr->div_val >= ((uint32_t)CLOCK_PCLKCTRL_DIV_MIN + 1UL)) &&
	    (pclk_ctrl_ptr->div_val <= (div_max + 1UL))) {
		pclk_ctrl_ptr->div_val -= 1UL;
	} else {
		return -EINVAL;
	}

	pclk_ctrl_ptr->freq = micom_clock_source[srch_src] / (pclk_ctrl_ptr->div_val + 1UL);

	return 0;
}

static void clock_dev_reset_clk_src(long id)
{
	if (id >= (long)CLOCK_SRC_MAX_NUM) {
		return;
	}

	if (id < (long)CLOCK_PLL_MAX_NUM) {
		micom_clock_source[id] = clock_get_pll_rate(id);
		micom_clock_source[CLOCK_PLL_MAX_NUM + id] =
			micom_clock_source[id] / (clock_dev_get_pll_div(id) + 1UL);
	}
}

void vcp_clock_init(void)
{
	static long initialized = -1;
	long idx;

	if (initialized == 1L) {
		return;
	}

	initialized = 1;

	for (idx = 0L; idx < ((long)CLOCK_PLL_MAX_NUM * 2L); idx++) {
		micom_clock_source[idx] = 0;
	}

	micom_clock_source[CLOCK_PLL_MAX_NUM * 2] = (uint32_t)CLOCK_XIN_CLK_RATE;
	micom_clock_source[(CLOCK_PLL_MAX_NUM * 2) + 1] = 0UL;

	for (idx = 0L; idx < (long)CLOCK_PLL_MAX_NUM; idx++) {
		clock_dev_reset_clk_src(idx);
	}
}

long clock_set_pll_div(long id, uint32_t pll_div)
{
	CLOCKMpll_t mpll_id;
	uint32_t reg, offset, reg_val, real_pll_div;

	reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_CLKDIVC;
	mpll_id = (CLOCKMpll_t)id;

	reg_val = 0;
	real_pll_div = ((pll_div > 1UL) ? (pll_div - 1UL) : (0UL));

	switch (mpll_id) {
	case CLOCK_MPLL_0:
		offset = (3UL - (uint32_t)id) * 8UL;
		break;
	case CLOCK_MPLL_1:
		offset = (3UL - (uint32_t)id) * 8UL;
		break;
	case CLOCK_MPLL_XIN:
		offset = 8UL;
		break;
	default:
		offset = 0;
		break;
	}

	if (offset == 0UL) {
		return -EINVAL;
	}

	reg_val = (uint32_t)(sys_read32(reg)) & (~(((uint32_t)0xFFUL << offset)));
	sys_write32(reg_val, reg);

	if (real_pll_div != 0UL) {
		reg_val |= (0x80UL | (real_pll_div & 0x3FUL)) << offset;
	} else {
		reg_val |= ((0x01UL) << offset);
	}

	sys_write32(reg_val, reg);

	return 0;
}

uint32_t clock_get_pll_rate(long id)
{
	uint32_t reg;
	CLOCKPll_t pll_id;

	reg = (uint32_t)0;
	pll_id = (CLOCKPll_t)id;

	if (pll_id == CLOCK_PLL_MICOM_0) {
		reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_PLL0PMS;
	} else if (pll_id == CLOCK_PLL_MICOM_1) {
		reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_PLL1PMS;
	} else {
		return 0;
	}

	return clock_dev_get_pll_rate(reg);
}

long clock_set_pll_rate(long id, uint32_t rate)
{
	uint32_t reg;
	CLOCKPll_t pll_id;
	enum clock_mpclk_ctrl_sel mpclk_sel;
	long idx;

	reg = (uint32_t)0;
	pll_id = (CLOCKPll_t)id;
	idx = -1;

	if (pll_id == CLOCK_PLL_MICOM_0) {
		reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_PLL0PMS;
		mpclk_sel = CLOCK_MPCLKCTRL_SEL_PLL0;
	} else if (pll_id == CLOCK_PLL_MICOM_1) {
		reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_PLL1PMS;
		mpclk_sel = CLOCK_MPCLKCTRL_SEL_PLL1;
	} else {
		return -EINVAL;
	}

	idx = (long)mpclk_sel;
	clock_dev_set_pll_rate(reg, rate);
	micom_clock_source[idx] = clock_dev_get_pll_rate(reg);

	return 0;
}

static long clock_dev_find_clk_ctrl(struct clock_clk_ctrl *clk_ctrl)
{
	long ret;
	uint32_t div_table[CLOCK_SRC_MAX_NUM];
	uint32_t err[CLOCK_SRC_MAX_NUM];
	uint32_t idx, srch_src, clk_rate, xin_freq;

	ret = 0;
	xin_freq = 0;

	if (clk_ctrl == NULL) {
		return -EINVAL;
	}

	if (clk_ctrl->en != 0UL) {
		xin_freq = ((uint32_t)CLOCK_XIN_CLK_RATE / 2UL);
	} else {
		xin_freq = (uint32_t)CLOCK_XIN_CLK_RATE;
	}

	srch_src = 0xFFFFFFFFUL;

	if (clk_ctrl->freq <= xin_freq) {
		clk_ctrl->sel = (uint32_t)CLOCK_MCLKCTRL_SEL_XIN;
		clk_ctrl->freq = xin_freq;

		if (clk_ctrl->en != 0UL) {
			clk_ctrl->conf = 1;
		} else {
			clk_ctrl->conf = 0;
		}
	} else {
		memset((void *)div_table, 0x00U, sizeof(div_table));

		for (idx = 0UL; idx < (uint32_t)CLOCK_SRC_MAX_NUM; idx++) {
			if (micom_clock_source[idx] == 0UL) {
				continue;
			}

			if (clk_ctrl->en != 0UL) {
				div_table[idx] =
					(micom_clock_source[idx] + (clk_ctrl->freq - 1UL)) /
					clk_ctrl->freq;
				if (div_table[idx] > ((uint32_t)CLOCK_MCLKCTRL_CONFIG_MAX + 1UL)) {
					div_table[idx] = (uint32_t)CLOCK_MCLKCTRL_CONFIG_MAX + 1UL;
				} else if (div_table[idx] <
					   ((uint32_t)CLOCK_MCLKCTRL_CONFIG_MIN + 1UL)) {
					div_table[idx] = (uint32_t)CLOCK_MCLKCTRL_CONFIG_MIN + 1UL;
				}

				clk_rate = micom_clock_source[idx] / div_table[idx];
			} else {
				clk_rate = micom_clock_source[idx];
			}

			if (clk_ctrl->freq < clk_rate) {
				continue;
			}

			err[idx] = clk_ctrl->freq - clk_rate;

			if (srch_src == 0xFFFFFFFFUL) {
				srch_src = idx;
			} else {
				/* find similar clock */
				if (err[idx] < err[srch_src]) {
					srch_src = idx;
				}
				/* find even division vlaue */
				else if (err[idx] == err[srch_src]) {
					if ((div_table[idx] % 2UL) == 0UL) {
						srch_src = idx;
					}
				} else {
					srch_src = 0xFFFFFFFFUL;
				}
			}

			if (err[srch_src] == 0UL) {
				break;
			}
		}

		if (srch_src == 0xFFFFFFFFUL) {
			return -EIO;
		}

		switch (srch_src) {
		case (uint32_t)CLOCK_MPLL_0:
			clk_ctrl->sel = (uint32_t)CLOCK_MCLKCTRL_SEL_PLL0;
			break;
		case (uint32_t)CLOCK_MPLL_1:
			clk_ctrl->sel = (uint32_t)CLOCK_MCLKCTRL_SEL_PLL1;
			break;
		case (uint32_t)CLOCK_MPLL_DIV_0:
			clk_ctrl->sel = (uint32_t)CLOCK_MCLKCTRL_SEL_PLL0DIV;
			break;
		case (uint32_t)CLOCK_MPLL_DIV_1:
			clk_ctrl->sel = (uint32_t)CLOCK_MCLKCTRL_SEL_PLL1DIV;
			break;
		case (uint32_t)CLOCK_MPLL_XIN:
			clk_ctrl->sel = (uint32_t)CLOCK_MCLKCTRL_SEL_XIN;
			break;
		default:
			ret = -EINVAL;
			break;
		}

		if (ret != 0L) {
			return ret;
		}

		if (clk_ctrl->en != 0UL) {
			if (div_table[srch_src] > ((uint32_t)CLOCK_MCLKCTRL_CONFIG_MAX + 1UL)) {
				div_table[srch_src] = (uint32_t)CLOCK_MCLKCTRL_CONFIG_MAX + 1UL;
			} else if (div_table[srch_src] <= (uint32_t)CLOCK_MCLKCTRL_CONFIG_MIN) {
				div_table[srch_src] = (uint32_t)CLOCK_MCLKCTRL_CONFIG_MIN + 1UL;
			}

			clk_ctrl->freq = micom_clock_source[srch_src] / div_table[srch_src];
			clk_ctrl->conf = div_table[srch_src] - 1UL;
		} else {
			clk_ctrl->freq = micom_clock_source[srch_src];
			clk_ctrl->conf = 0;
		}
	}

	return 0;
}

long clock_set_clk_ctrl_rate(long id, uint32_t rate)
{
	uint32_t reg;
	struct clock_clk_ctrl clk_ctrl;

	reg = (uint32_t)((uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_CLKCTRL +
			 ((uint32_t)id * 4UL));

	if ((sys_read32(reg) & (1UL << (uint32_t)CLOCK_MCLKCTRL_EN_SHIFT)) != 0UL) {
		clk_ctrl.en = 1;
	} else {
		clk_ctrl.en = 0;
	}

	clk_ctrl.freq = rate;

	if (clock_dev_find_clk_ctrl(&clk_ctrl) != 0L) {
		return -EIO;
	}

	clock_dev_write_clk_ctrl(reg, clk_ctrl.en, clk_ctrl.conf, clk_ctrl.sel);

	return 0;
}

uint32_t clock_get_clk_ctrl_rate(long id)
{
	struct clock_clk_ctrl clk_ctrl;
	enum clock_mclk_ctrl_sel mclk_sel;
	uint32_t reg, reg_val, src_freq, ret;

	reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_CLKCTRL + ((uint32_t)id * 4UL);
	reg_val = 0;
	ret = 0;
	reg_val = sys_read32(reg);

	clk_ctrl.sel = (reg_val & ((uint32_t)CLOCK_MCLKCTRL_SEL_MASK
				   << (uint32_t)CLOCK_MCLKCTRL_SEL_SHIFT)) >>
		       (uint32_t)CLOCK_MCLKCTRL_SEL_SHIFT;
	mclk_sel = (enum clock_mclk_ctrl_sel)clk_ctrl.sel;

	switch (mclk_sel) {
	case CLOCK_MCLKCTRL_SEL_XIN:
		src_freq = (uint32_t)CLOCK_XIN_CLK_RATE;
		break;
	case CLOCK_MCLKCTRL_SEL_PLL0:
		src_freq = clock_get_pll_rate((long)CLOCK_PLL_MICOM_0);
		break;
	case CLOCK_MCLKCTRL_SEL_PLL1:
		src_freq = clock_get_pll_rate((long)CLOCK_PLL_MICOM_1);
		break;
	case CLOCK_MCLKCTRL_SEL_PLL0DIV:
		src_freq = clock_get_pll_rate((long)CLOCK_PLL_MICOM_0) /
			   (clock_dev_get_pll_div((long)CLOCK_PLL_MICOM_0) + 1UL);
		break;
	case CLOCK_MCLKCTRL_SEL_PLL1DIV:
		src_freq = clock_get_pll_rate((long)CLOCK_PLL_MICOM_1) /
			   (clock_dev_get_pll_div((long)CLOCK_PLL_MICOM_1) + 1UL);
		break;
	default:
		src_freq = 0UL;
		break;
	}

	if (src_freq > 0UL) {
		clk_ctrl.conf = (reg_val & ((uint32_t)CLOCK_MCLKCTRL_CONFIG_MASK
					    << (uint32_t)CLOCK_MCLKCTRL_CONFIG_SHIFT)) >>
				(uint32_t)CLOCK_MCLKCTRL_CONFIG_SHIFT;

		clk_ctrl.freq = src_freq / (clk_ctrl.conf + 1UL);
		ret = (uint32_t)clk_ctrl.freq;
	}

	return ret;
}

long clock_is_peri_enabled(long id)
{
	CLOCKPeri_t peri_offset;
	uint32_t reg;

	peri_offset = CLOCK_PERI_SFMC;
	reg = ((uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_PCLKCTRL) +
	      (((uint32_t)id - (uint32_t)peri_offset) * 4UL);

	return ((sys_read32(reg) & (1UL << (uint32_t)CLOCK_PCLKCTRL_EN_SHIFT)) != 0UL) ? 1L : 0L;
}

long clock_enable_peri(long id)
{
	CLOCKPeri_t peri_offset;
	uint32_t reg;

	peri_offset = CLOCK_PERI_SFMC;
	reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_PCLKCTRL +
	      (((uint32_t)id - (uint32_t)peri_offset) * 4UL);
	sys_write32((sys_read32(reg) | (1UL << (uint32_t)CLOCK_PCLKCTRL_EN_SHIFT)), reg);

	return 0;
}

long clock_disable_peri(long id)
{
	CLOCKPeri_t peri_offset;
	uint32_t reg;

	peri_offset = CLOCK_PERI_SFMC;
	reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_PCLKCTRL +
	      (((uint32_t)id - (uint32_t)peri_offset) * 4UL);

	sys_write32(sys_read32(reg) & ~(1UL << (uint32_t)CLOCK_PCLKCTRL_EN_SHIFT), reg);

	return 0;
}

uint32_t clock_get_peri_rate(long id)
{
	CLOCKPeri_t peri_offset;
	uint32_t reg, reg_va, src_freq, ret;
	struct clock_pclk_ctrl pclk_ctrl;
	CLOCKPclkctrlSel_t pclk_sel;

	peri_offset = CLOCK_PERI_SFMC;
	reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_PCLKCTRL +
	      (((uint32_t)id - (uint32_t)peri_offset) * 4UL);
	reg_va = sys_read32(reg);
	ret = 0;
	pclk_ctrl.sel = (reg_va & ((uint32_t)CLOCK_PCLKCTRL_SEL_MASK
				   << (uint32_t)CLOCK_PCLKCTRL_SEL_SHIFT)) >>
			(uint32_t)CLOCK_PCLKCTRL_SEL_SHIFT;

	pclk_sel = (CLOCKPclkctrlSel_t)(pclk_ctrl.sel);

	switch (pclk_sel) {
	case CLOCK_PCLKCTRL_SEL_PLL0:
		src_freq = clock_get_pll_rate((long)CLOCK_PLL_MICOM_0);
		break;
	case CLOCK_PCLKCTRL_SEL_PLL1:
		src_freq = clock_get_pll_rate((long)CLOCK_PLL_MICOM_1);
		break;
	case CLOCK_PCLKCTRL_SEL_PLL0DIV:
		src_freq = clock_get_pll_rate((long)CLOCK_PLL_MICOM_0) /
			   (clock_dev_get_pll_div((long)CLOCK_PLL_MICOM_0) + 1UL);
		break;
	case CLOCK_PCLKCTRL_SEL_PLL1DIV:
		src_freq = clock_get_pll_rate((long)CLOCK_PLL_MICOM_1) /
			   (clock_dev_get_pll_div((long)CLOCK_PLL_MICOM_1) + 1UL);
		break;
	case CLOCK_PCLKCTRL_SEL_XIN:
		src_freq = (uint32_t)CLOCK_XIN_CLK_RATE;
		break;
	default:
		src_freq = 0UL;
		break;
	}

	if (src_freq > 0UL) {
		pclk_ctrl.freq = 0;
		pclk_ctrl.div_val = (reg_va & ((uint32_t)CLOCK_PCLKCTRL_DIV_XXX_MASK
					       << (uint32_t)CLOCK_PCLKCTRL_DIV_SHIFT)) >>
				    (uint32_t)CLOCK_PCLKCTRL_DIV_SHIFT;
		pclk_ctrl.freq = src_freq / (pclk_ctrl.div_val + 1UL);
		ret = (uint32_t)pclk_ctrl.freq;
	}

	return ret;
}

long clock_set_peri_rate(long id, uint32_t rate)
{
	CLOCKPeri_t peri_offset;
	uint32_t reg;
	long err;
	struct clock_pclk_ctrl pclk_ctrl;

	peri_offset = CLOCK_PERI_SFMC;
	reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_PCLKCTRL +
	      (((uint32_t)id - (uint32_t)peri_offset) * 4UL);
	err = 0;
	pclk_ctrl.freq = rate;
	pclk_ctrl.peri_name = (uint32_t)id;
	pclk_ctrl.div_val = 0;
	pclk_ctrl.md = (uint32_t)CLOCK_PCLKCTRL_MODE_DIVIDER;
	pclk_ctrl.sel = (uint32_t)CLOCK_MPCLKCTRL_SEL_XIN;

	if (clock_dev_find_pclk(&pclk_ctrl, CLOCK_PCLKCTRL_TYPE_XXX) != 0L) {
		clock_dev_write_pclk_ctrl(reg, (uint32_t)CLOCK_PCLKCTRL_MODE_DIVIDER,
					  (uint32_t)FALSE, (uint32_t)CLOCK_MPCLKCTRL_SEL_XIN, 1UL,
					  (uint32_t)CLOCK_PCLKCTRL_TYPE_XXX);
		err = -EIO;
	} else {
		if ((sys_read32(reg) & (1UL << (uint32_t)CLOCK_PCLKCTRL_EN_SHIFT)) != 0UL) {
			pclk_ctrl.en = 1;
		} else {
			pclk_ctrl.en = 0;
		}

		clock_dev_write_pclk_ctrl(reg, pclk_ctrl.md, pclk_ctrl.en, pclk_ctrl.sel,
					  pclk_ctrl.div_val, (uint32_t)CLOCK_PCLKCTRL_TYPE_XXX);
	}

	return err;
}

long clock_is_iobus_pwdn(long id)
{
	uint32_t reg;
	long rest;
	CLOCKIobus_t iobus;

	iobus = (CLOCKIobus_t)id;

	if ((long)iobus < (32L * 1L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLK0;
	} else if ((long)iobus < (32L * 2L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLK1;
	} else if ((long)iobus < (32L * 3L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLK2;
	} else {
		return -EINVAL;
	}

	rest = (long)iobus % 32L;

	return ((sys_read32(reg) & (1UL << (uint32_t)rest)) != 0UL) ? 0L : 1L;
}

long clock_enable_iobus(long id, boolean en)
{
	long ret;

	if (en == TRUE) {
		if (clock_set_iobus_pwdn(id, FALSE) == 0L) {
			ret = clock_set_sw_reset(id, FALSE);
		} else {
			ret = -EIO;
		}
	} else {
		if (clock_set_sw_reset(id, TRUE) == 0L) {
			ret = clock_set_iobus_pwdn(id, TRUE);
		} else {
			ret = -EIO;
		}
	}

	return ret;
}

long clock_set_iobus_pwdn(long id, boolean en)
{
	uint32_t reg;
	long rest;
	CLOCKIobus_t iobus;

	iobus = (CLOCKIobus_t)id;

	if ((long)iobus < (32L * 1L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLK0;
	} else if ((long)iobus < (32L * 2L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLK1;
	} else if ((long)iobus < (32L * 3L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLK2;
	} else {
		return -EINVAL;
	}

	rest = (long)iobus % 32;

	if (en == TRUE) {
		sys_write32(sys_read32(reg) & ~((uint32_t)1UL << (uint32_t)rest), reg);
	} else {
		sys_write32(sys_read32(reg) | ((uint32_t)1UL << (uint32_t)rest), reg);
	}

	return 0;
}

long clock_set_sw_reset(long id, boolean reset)
{
	uint32_t reg;
	long rest;
	CLOCKIobus_t iobus;

	iobus = (CLOCKIobus_t)id;

	if ((long)iobus < (32L * 1L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLKSWR0;
	} else if ((long)iobus < (32L * 2L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLKSWR1;
	} else if ((long)iobus < (32L * 3L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLKSWR2;
	} else {
		return -EINVAL;
	}

	rest = (long)iobus % 32;

	if (reset == TRUE) {
		sys_write32(sys_read32(reg) & ~((uint32_t)1UL << (uint32_t)rest), reg);
	} else {
		sys_write32(sys_read32(reg) | ((uint32_t)1UL << (uint32_t)rest), reg);
	}

	return 0;
}

static int tcc_clock_control_get_rate(const struct device *dev, clock_control_subsys_t sys,
				      uint32_t *rate)
{
	/*	const struct device *syscon = DEV_CFG(dev)->syscon; */
	uint32_t clk_id = (uint32_t)sys;

	*rate = clock_get_clk_ctrl_rate(clk_id);

	return 0;
}

static int clock_control_tcc_vcp_init(const struct device *dev)
{
	vcp_clock_init();

	return 0;
}

static const struct clock_control_driver_api tcc_clk_api = {
	.get_rate = tcc_clock_control_get_rate,
};

#define TCC_CLOCK_INIT(n)                                                                          \
	static const struct clock_tcc_config clock_tcc_cfg_##n = {                                 \
		.syscon = DEVICE_DT_GET(DT_NODELABEL(syscon)),                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, clock_control_tcc_vcp_init, NULL, NULL, &clock_tcc_cfg_##n,       \
			      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &tcc_clk_api);

DT_INST_FOREACH_STATUS_OKAY(TCC_CLOCK_INIT)
