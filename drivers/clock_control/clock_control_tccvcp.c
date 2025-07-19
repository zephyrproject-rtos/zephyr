/*
 * Copyright 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT tcc_ccu

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/sys/util.h>

#define LOG_LEVEL CONFIG_CLOCK_CONTROL_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(clock_control_tcc_ccu);

#include <zephyr/drivers/clock_control/clock_control_tcc_ccu.h>
#include <zephyr/drivers/clock_control/clock_control_tccvcp.h>

#include <soc_reg_phys.h>

struct clock_tcc_config {
	const struct device *syscon;
};

static uint32_t micom_clock_source[CLOCK_SRC_MAX_NUM];

static void clock_dev_write_pll(uint32_t uiReg, uint32_t en, uint32_t p, uint32_t m, uint32_t s);

static void clock_dev_write_pclk_ctrl(uint32_t uiReg, uint32_t md, uint32_t en, uint32_t sel,
				      uint32_t uiDiv, uint32_t uiType);

static void clock_dev_write_clk_ctrl(uint32_t uiReg, uint32_t en, uint32_t conf, uint32_t sel);

static int32_t clock_dev_find_pms(struct clock_pms *psPll, uint32_t srcFreq);

static int32_t clock_dev_set_pll_rate(uint32_t uiReg, uint32_t uiRate);

static uint32_t clock_dev_get_pll_rate(uint32_t uiReg);

static uint32_t clock_dev_get_pll_div(int32_t iId);

static int32_t clock_dev_find_clk_ctrl(struct clock_clk_ctrl *CLKCTRL);

static uint32_t clock_dev_cal_pclk_div(const struct clock_pclk_ctrl *psPclkCtrl,
				       uint32_t *puiClkDiv, const uint32_t srcClk,
				       uint32_t uiDivMax);

static int32_t clock_dev_find_pclk(struct clock_pclk_ctrl *psPclkCtrl,
				   enum clock_pclk_ctrl_type eType);

static void clock_dev_reset_clk_src(int32_t iId);

/* PLL Configuration Macro */
static void clock_dev_write_pll(uint32_t reg, uint32_t en, uint32_t p, uint32_t m, uint32_t s)
{
	const uint32_t timeout = 4000000000UL;
	uint32_t try_count = 0;

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

			sys_write32(sys_read32(reg) | ((en & 1UL) << (uint32_t)CLOCK_PLL_EN_SHIFT),
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
	} else {
		return;
	}
}

static void write_and_wait(uint32_t reg, uint32_t value, uint32_t mask, uint32_t shift,
			   uint32_t wait_shift, uint32_t timeout)
{
	sys_write32((sys_read32(reg) & ~(mask << shift)) | ((value & mask) << shift), reg);

	uint32_t try_count = 0;

	while ((sys_read32(reg) & (1UL << wait_shift)) != 0UL) {
		try_count++;
		if (try_count > timeout) {
			break;
		}
	}
}

static void clock_dev_write_clk_ctrl(uint32_t reg, uint32_t en, uint32_t conf, uint32_t sel)
{
	const uint32_t timeout = 4000000000;
	uint32_t cur_conf = (sys_read32(reg) >> (uint32_t)CLOCK_MCLKCTRL_CONFIG_SHIFT) &
			    (uint32_t)CLOCK_MCLKCTRL_CONFIG_MASK;

	if (conf >= cur_conf) {
		write_and_wait(reg, conf, CLOCK_MCLKCTRL_CONFIG_MASK, CLOCK_MCLKCTRL_CONFIG_SHIFT,
			       CLOCK_MCLKCTRL_CLKCHG_SHIFT, timeout);
		write_and_wait(reg, sel, CLOCK_MCLKCTRL_SEL_MASK, CLOCK_MCLKCTRL_SEL_SHIFT,
			       CLOCK_MCLKCTRL_CLKCHG_SHIFT, timeout);
	} else {
		write_and_wait(reg, sel, CLOCK_MCLKCTRL_SEL_MASK, CLOCK_MCLKCTRL_SEL_SHIFT,
			       CLOCK_MCLKCTRL_CLKCHG_SHIFT, timeout);
		write_and_wait(reg, conf, CLOCK_MCLKCTRL_CONFIG_MASK, CLOCK_MCLKCTRL_CONFIG_SHIFT,
			       CLOCK_MCLKCTRL_CLKCHG_SHIFT, timeout);
	}

	if (en != 0UL) {
		write_and_wait(reg, en & 1UL, 1UL, CLOCK_MCLKCTRL_EN_SHIFT,
			       CLOCK_MCLKCTRL_DIVSTS_SHIFT, timeout);
	}
}

static inline int is_valid_fvco(uint64_t fvco)
{
	return (fvco >= (uint64_t)CLOCK_PLL_VCO_MIN) && (fvco <= (uint64_t)CLOCK_PLL_VCO_MAX);
}

static inline int is_valid_m(uint64_t m)
{
	return (m >= (uint64_t)CLOCK_PLL_M_MIN) && (m <= (uint64_t)CLOCK_PLL_M_MAX);
}

static inline int is_valid_src_pll(uint64_t src_pll)
{
	return (src_pll >= (uint64_t)CLOCK_PLL_MIN_RATE) &&
	       (src_pll <= (uint64_t)CLOCK_PLL_MAX_RATE);
}

static void update_best_err(uint32_t *err, uint32_t srch_err, struct clock_pms *pll_ptr, uint32_t p,
			    uint32_t m, uint32_t s)
{
	if (srch_err < *err) {
		*err = srch_err;
		pll_ptr->p = p;
		pll_ptr->m = m;
		pll_ptr->s = s;
	}
}

static int32_t clock_dev_find_pms(struct clock_pms *pll_ptr, uint32_t src_freq)
{
	uint64_t pll, src, fvco;
	uint64_t srch_m, temp, src_pll;
	uint32_t err, srch_err;

	if (!pll_ptr) {
		return 0;
	}
	if (pll_ptr->fpll == 0UL) {
		pll_ptr->en = 0;
		return 0;
	}

	pll = (uint64_t)pll_ptr->fpll;
	src = (uint64_t)src_freq;
	err = 0xFFFFFFFFUL;

	for (int32_t srch_s = CLOCK_PLL_S_MIN; srch_s <= CLOCK_PLL_S_MAX; srch_s++) {
		fvco = pll << srch_s;
		if (!is_valid_fvco(fvco)) {
			continue;
		}

		for (uint64_t srch_p = CLOCK_PLL_P_MIN; srch_p <= CLOCK_PLL_P_MAX; srch_p++) {
			srch_m = fvco * srch_p;
			soc_div64_to_32(&srch_m, src_freq, NULL);
			if (!is_valid_m(srch_m)) {
				continue;
			}

			temp = srch_m * src;
			soc_div64_to_32(&temp, srch_p, NULL);
			src_pll = (temp >> srch_s);
			if (!is_valid_src_pll(src_pll)) {
				continue;
			}

			srch_err = (uint32_t)((src_pll > pll) ? (src_pll - pll) : (pll - src_pll));
			update_best_err(&err, srch_err, pll_ptr, (uint32_t)srch_p, (uint32_t)srch_m,
					(uint32_t)srch_s);
		}
	}

	if (err == 0xFFFFFFFFUL) {
		return -EIO;
	}

	temp = src * (uint64_t)pll_ptr->m;
	soc_div64_to_32(&temp, pll_ptr->p, NULL);
	pll_ptr->fpll = (uint32_t)(temp >> pll_ptr->s);
	pll_ptr->en = 1;

	return 0;
}

static int32_t clock_dev_set_pll_rate(uint32_t reg, uint32_t rate)
{
	struct clock_pms pll;
	uint64_t cal_m;
	int32_t err;

	err = 0;
	memset(&pll, 0, sizeof(pll));
	pll.fpll = rate;

	if (clock_dev_find_pms(&pll, CLOCK_XIN_CLK_RATE) != 0L) {
		cal_m = (uint64_t)CLOCK_PLL_P_MIN * (uint64_t)CLOCK_PLL_VCO_MIN;
		cal_m += (uint64_t)CLOCK_XIN_CLK_RATE;
		cal_m /= (uint64_t)CLOCK_XIN_CLK_RATE;
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
	uint64_t temp;

	if (reg == 0UL) {
		return 0;
	}

	reg_val = sys_read32(reg);

	pll_cfg.p = (reg_val >> (uint32_t)CLOCK_PLL_P_SHIFT) & ((uint32_t)CLOCK_PLL_P_MASK);
	pll_cfg.m = (reg_val >> (uint32_t)CLOCK_PLL_M_SHIFT) & ((uint32_t)CLOCK_PLL_M_MASK);
	pll_cfg.s = (reg_val >> (uint32_t)CLOCK_PLL_S_SHIFT) & ((uint32_t)CLOCK_PLL_S_MASK);
	pll_cfg.en = (reg_val >> (uint32_t)CLOCK_PLL_EN_SHIFT) & (1UL);
	pll_cfg.src = (reg_val >> (uint32_t)CLOCK_PLL_SRC_SHIFT) & ((uint32_t)CLOCK_PLL_SRC_MASK);
	temp = (uint64_t)CLOCK_XIN_CLK_RATE * (uint64_t)pll_cfg.m;
	soc_div64_to_32(&temp, (uint32_t)(pll_cfg.p), NULL);

	return (uint32_t)((temp) >> pll_cfg.s);
}

static uint32_t clock_dev_get_pll_div(int32_t id)
{
	uint32_t ret;
	CLOCKMpll_t m_pll_id;
	uint32_t reg, offset, reg_val;

	ret = 0;
	m_pll_id = (CLOCKMpll_t)id;
	reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_CLKDIVC;

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

	if (*clk_div_ptr == 0) {
		return 0;
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

static int32_t clock_dev_find_pclk(struct clock_pclk_ctrl *pclk_ctrl_ptr,
				   enum clock_pclk_ctrl_type type)
{
	int32_t ret, last_idx;
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
	last_idx = (int32_t)CLOCK_SRC_MAX_NUM - 1L;

	for (int32_t idx = last_idx; idx >= 0L; idx--) {
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

static void clock_dev_reset_clk_src(int32_t id)
{
	if ((id < 0) || (id >= (int32_t)CLOCK_SRC_MAX_NUM)) {
		return;
	}

	if (id < (int32_t)CLOCK_PLL_MAX_NUM) {
		micom_clock_source[id] = clock_get_pll_rate(id);
		micom_clock_source[CLOCK_PLL_MAX_NUM + id] =
			micom_clock_source[id] / (clock_dev_get_pll_div(id) + 1UL);
	}
}

void vcp_clock_init(void)
{
	static int32_t initialized = -1;
	int32_t idx;

	if (initialized == 1L) {
		return;
	}

	initialized = 1;

	for (idx = 0L; idx < ((int32_t)CLOCK_PLL_MAX_NUM * 2L); idx++) {
		micom_clock_source[idx] = 0;
	}

	micom_clock_source[CLOCK_PLL_MAX_NUM * 2] = (uint32_t)CLOCK_XIN_CLK_RATE;
	micom_clock_source[(CLOCK_PLL_MAX_NUM * 2) + 1] = 0UL;

	for (idx = 0L; idx < (int32_t)CLOCK_PLL_MAX_NUM; idx++) {
		clock_dev_reset_clk_src(idx);
	}
}

int32_t clock_set_pll_div(int32_t id, uint32_t pll_div)
{
	CLOCKMpll_t mpll_id;
	uint32_t reg, offset, reg_val, real_pll_div;

	reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_CLKDIVC;
	mpll_id = (CLOCKMpll_t)id;

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

uint32_t clock_get_pll_rate(int32_t id)
{
	uint32_t reg;
	CLOCKPll_t pll_id;

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

int32_t clock_set_pll_rate(int32_t id, uint32_t rate)
{
	uint32_t reg;
	CLOCKPll_t pll_id;
	enum clock_mpclk_ctrl_sel mpclk_sel;
	int32_t idx;

	pll_id = (CLOCKPll_t)id;

	if (pll_id == CLOCK_PLL_MICOM_0) {
		reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_PLL0PMS;
		mpclk_sel = CLOCK_MPCLKCTRL_SEL_PLL0;
	} else if (pll_id == CLOCK_PLL_MICOM_1) {
		reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_PLL1PMS;
		mpclk_sel = CLOCK_MPCLKCTRL_SEL_PLL1;
	} else {
		return -EINVAL;
	}

	idx = (int32_t)mpclk_sel;
	clock_dev_set_pll_rate(reg, rate);
	micom_clock_source[idx] = clock_dev_get_pll_rate(reg);

	return 0;
}

static uint32_t calc_div(uint32_t src, uint32_t freq)
{
	uint32_t div_val = (src + (freq - 1UL)) / freq;

	if (div_val > CLOCK_MCLKCTRL_CONFIG_MAX + 1UL) {
		return CLOCK_MCLKCTRL_CONFIG_MAX + 1UL;
	}
	if (div_val < CLOCK_MCLKCTRL_CONFIG_MIN + 1UL) {
		return CLOCK_MCLKCTRL_CONFIG_MIN + 1UL;
	}
	return div_val;
}

static void update_best_src(uint32_t idx, uint32_t err, uint32_t div, uint32_t *srch_src,
			    uint32_t *best_err, uint32_t *div_table, uint32_t *err_table)
{
	if (*srch_src == 0xFFFFFFFFUL || err < *best_err ||
	    (err == *best_err && (div % 2UL == 0UL))) {
		*srch_src = idx;
		*best_err = err;
	}
	err_table[idx] = err;
	div_table[idx] = div;
}

static int32_t clock_dev_find_clk_ctrl(struct clock_clk_ctrl *clk_ctrl)
{
	if (!clk_ctrl) {
		return -EINVAL;
	}

	uint32_t xin_freq = (clk_ctrl->en != 0UL) ? (CLOCK_XIN_CLK_RATE / 2UL) : CLOCK_XIN_CLK_RATE;

	if (clk_ctrl->freq <= xin_freq) {
		clk_ctrl->sel = CLOCK_MCLKCTRL_SEL_XIN;
		clk_ctrl->freq = xin_freq;
		clk_ctrl->conf = (clk_ctrl->en != 0UL) ? 1 : 0;
		return 0;
	}

	uint32_t div_table[CLOCK_SRC_MAX_NUM] = {0};
	uint32_t err_table[CLOCK_SRC_MAX_NUM] = {0};
	uint32_t srch_src = 0xFFFFFFFFUL;
	uint32_t best_err = 0xFFFFFFFFUL;

	for (uint32_t idx = 0; idx < CLOCK_SRC_MAX_NUM; idx++) {
		uint32_t src = micom_clock_source[idx];

		if (src == 0UL) {
			continue;
		}

		uint32_t clk_rate, div_val = 1;

		if (clk_ctrl->en != 0UL) {
			div_val = calc_div(src, clk_ctrl->freq);
			clk_rate = src / div_val;
		} else {
			clk_rate = src;
		}

		if (clk_ctrl->freq < clk_rate) {
			continue;
		}

		uint32_t err = clk_ctrl->freq - clk_rate;

		update_best_src(idx, err, div_val, &srch_src, &best_err, div_table, err_table);

		if (best_err == 0UL) {
			break;
		}
	}

	if (srch_src == 0xFFFFFFFFUL) {
		return -EIO;
	}

	static const uint32_t sel_table[CLOCK_SRC_MAX_NUM] = {
		[CLOCK_MPLL_0] = CLOCK_MCLKCTRL_SEL_PLL0,
		[CLOCK_MPLL_1] = CLOCK_MCLKCTRL_SEL_PLL1,
		[CLOCK_MPLL_DIV_0] = CLOCK_MCLKCTRL_SEL_PLL0DIV,
		[CLOCK_MPLL_DIV_1] = CLOCK_MCLKCTRL_SEL_PLL1DIV,
		[CLOCK_MPLL_XIN] = CLOCK_MCLKCTRL_SEL_XIN,
	};

	if (srch_src >= CLOCK_SRC_MAX_NUM || sel_table[srch_src] == 0) {
		return -EINVAL;
	}

	clk_ctrl->sel = sel_table[srch_src];

	if (clk_ctrl->en != 0UL) {
		uint32_t div_tbl = div_table[srch_src];

		if (div_tbl > CLOCK_MCLKCTRL_CONFIG_MAX + 1UL) {
			div_tbl = CLOCK_MCLKCTRL_CONFIG_MAX + 1UL;
		}
		if (div_tbl <= CLOCK_MCLKCTRL_CONFIG_MIN) {
			div_tbl = CLOCK_MCLKCTRL_CONFIG_MIN + 1UL;
		}
		clk_ctrl->freq = micom_clock_source[srch_src] / div_tbl;
		clk_ctrl->conf = div_tbl - 1UL;
	} else {
		clk_ctrl->freq = micom_clock_source[srch_src];
		clk_ctrl->conf = 0;
	}

	return 0;
}

int32_t clock_set_clk_ctrl_rate(int32_t id, uint32_t rate)
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

uint32_t clock_get_clk_ctrl_rate(int32_t id)
{
	struct clock_clk_ctrl clk_ctrl;
	enum clock_mclk_ctrl_sel mclk_sel;
	uint32_t reg, reg_val, src_freq, ret;

	reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_CLKCTRL + ((uint32_t)id * 4UL);
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
		src_freq = clock_get_pll_rate((int32_t)CLOCK_PLL_MICOM_0);
		break;
	case CLOCK_MCLKCTRL_SEL_PLL1:
		src_freq = clock_get_pll_rate((int32_t)CLOCK_PLL_MICOM_1);
		break;
	case CLOCK_MCLKCTRL_SEL_PLL0DIV:
		src_freq = clock_get_pll_rate((int32_t)CLOCK_PLL_MICOM_0) /
			   (clock_dev_get_pll_div((int32_t)CLOCK_PLL_MICOM_0) + 1UL);
		break;
	case CLOCK_MCLKCTRL_SEL_PLL1DIV:
		src_freq = clock_get_pll_rate((int32_t)CLOCK_PLL_MICOM_1) /
			   (clock_dev_get_pll_div((int32_t)CLOCK_PLL_MICOM_1) + 1UL);
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

int32_t clock_is_peri_enabled(int32_t id)
{
	CLOCKPeri_t peri_offset;
	uint32_t reg;

	peri_offset = CLOCK_PERI_SFMC;
	reg = ((uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_PCLKCTRL) +
	      (((uint32_t)id - (uint32_t)peri_offset) * 4UL);

	return ((sys_read32(reg) & (1UL << (uint32_t)CLOCK_PCLKCTRL_EN_SHIFT)) != 0UL) ? 1L : 0L;
}

int32_t clock_enable_peri(int32_t id)
{
	CLOCKPeri_t peri_offset;
	uint32_t reg;

	peri_offset = CLOCK_PERI_SFMC;
	reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_PCLKCTRL +
	      (((uint32_t)id - (uint32_t)peri_offset) * 4UL);
	sys_write32((sys_read32(reg) | (1UL << (uint32_t)CLOCK_PCLKCTRL_EN_SHIFT)), reg);

	return 0;
}

int32_t clock_disable_peri(int32_t id)
{
	CLOCKPeri_t peri_offset;
	uint32_t reg;

	peri_offset = CLOCK_PERI_SFMC;
	reg = (uint32_t)CLOCK_BASE_ADDR + (uint32_t)CLOCK_MCKC_PCLKCTRL +
	      (((uint32_t)id - (uint32_t)peri_offset) * 4UL);

	sys_write32(sys_read32(reg) & ~(1UL << (uint32_t)CLOCK_PCLKCTRL_EN_SHIFT), reg);

	return 0;
}

uint32_t clock_get_peri_rate(int32_t id)
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
		src_freq = clock_get_pll_rate((int32_t)CLOCK_PLL_MICOM_0);
		break;
	case CLOCK_PCLKCTRL_SEL_PLL1:
		src_freq = clock_get_pll_rate((int32_t)CLOCK_PLL_MICOM_1);
		break;
	case CLOCK_PCLKCTRL_SEL_PLL0DIV:
		src_freq = clock_get_pll_rate((int32_t)CLOCK_PLL_MICOM_0) /
			   (clock_dev_get_pll_div((int32_t)CLOCK_PLL_MICOM_0) + 1UL);
		break;
	case CLOCK_PCLKCTRL_SEL_PLL1DIV:
		src_freq = clock_get_pll_rate((int32_t)CLOCK_PLL_MICOM_1) /
			   (clock_dev_get_pll_div((int32_t)CLOCK_PLL_MICOM_1) + 1UL);
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

int32_t clock_set_peri_rate(int32_t id, uint32_t rate)
{
	CLOCKPeri_t peri_offset;
	uint32_t reg;
	int32_t err;
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
					  (uint32_t)false, (uint32_t)CLOCK_MPCLKCTRL_SEL_XIN, 1UL,
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

int32_t clock_is_iobus_pwdn(int32_t id)
{
	uint32_t reg;
	int32_t rest;
	CLOCKIobus_t iobus;

	iobus = (CLOCKIobus_t)id;

	if ((int32_t)iobus < (32L * 1L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLK0;
	} else if ((int32_t)iobus < (32L * 2L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLK1;
	} else if ((int32_t)iobus < (32L * 3L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLK2;
	} else {
		return -EINVAL;
	}

	rest = (int32_t)iobus % 32L;

	return ((sys_read32(reg) & (1UL << (uint32_t)rest)) != 0UL) ? 0L : 1L;
}

int32_t clock_enable_iobus(int32_t id, bool en)
{
	int32_t ret;

	if (en == true) {
		if (clock_set_iobus_pwdn(id, false) == 0L) {
			ret = clock_set_sw_reset(id, false);
		} else {
			ret = -EIO;
		}
	} else {
		if (clock_set_sw_reset(id, true) == 0L) {
			ret = clock_set_iobus_pwdn(id, true);
		} else {
			ret = -EIO;
		}
	}

	return ret;
}

int32_t clock_set_iobus_pwdn(int32_t id, bool en)
{
	uint32_t reg;
	int32_t rest;
	CLOCKIobus_t iobus;

	iobus = (CLOCKIobus_t)id;

	if ((int32_t)iobus < (32L * 1L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLK0;
	} else if ((int32_t)iobus < (32L * 2L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLK1;
	} else if ((int32_t)iobus < (32L * 3L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLK2;
	} else {
		return -EINVAL;
	}

	rest = (int32_t)iobus % 32;

	if (en == true) {
		sys_write32(sys_read32(reg) & ~((uint32_t)1UL << (uint32_t)rest), reg);
	} else {
		sys_write32(sys_read32(reg) | ((uint32_t)1UL << (uint32_t)rest), reg);
	}

	return 0;
}

int32_t clock_set_sw_reset(int32_t id, bool reset)
{
	uint32_t reg;
	int32_t rest;
	CLOCKIobus_t iobus;

	iobus = (CLOCKIobus_t)id;

	if ((int32_t)iobus < (32L * 1L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLKSWR0;
	} else if ((int32_t)iobus < (32L * 2L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLKSWR1;
	} else if ((int32_t)iobus < (32L * 3L)) {
		reg = (uint32_t)MCU_BSP_SUBSYS_BASE + (uint32_t)CLOCK_MCKC_HCLKSWR2;
	} else {
		return -EINVAL;
	}

	rest = (int32_t)iobus % 32;

	if (reset == true) {
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
