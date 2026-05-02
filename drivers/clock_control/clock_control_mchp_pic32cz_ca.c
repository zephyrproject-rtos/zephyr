/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <string.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>

#define DT_DRV_COMPAT microchip_pic32cz_ca_clock
LOG_MODULE_REGISTER(clock_mchp_pic32cz_ca, CONFIG_CLOCK_CONTROL_LOG_LEVEL);
#define CLOCK_NODE DT_NODELABEL(clock)

/* Frequency values */
#define FREQ_32KHZ   32768
#define FREQ_1KHZ    1024
#define FREQ_DFLL48M 48000000

/* timeout values in microseconds */
#define TIMEOUT_XOSC_RDY       1000000
#define TIMEOUT_DFLL48M_RDY    1000000
#define TIMEOUT_DPLL_LOCK      1000000
#define TIMEOUT_OSC32KCTRL_RDY 1000000
#define TIMEOUT_SUPC_REGRDY    1000000
#define TIMEOUT_MCLK_RDY       1000000
#define TIMEOUT_REG_SYNC       1000

#define GCLK_IO_MIN                2
#define GCLK_IO_MAX                7
#define GCLK_PH_MAX                47
#define BIT_MASK_MAX               31
#define CLOCK_INIT_ITERATION_COUNT 3

/* maximum value for div_val, when div_select is clock source frequency divided by 2^(N+1) */
#define GCLKGEN_POWER_DIV_MAX 29

/* mclkmaskreg Not Applicable for a clock subsystem ID */
#define MMASKREG_NA (0x3f)

/* mclkmaskbit Not Applicable for a clock subsystem ID */
#define MMASKBIT_NA (0x3f)

/* gclkperiph Not Applicable for a clock subsystem ID */
#define GCLK_PH_NA (0x3f)

/* OSCCTRL_PLL1 registers are at offset of 5 registers from OSCCTRL_PLL0 */
#define DPLLREG_OFFSET 5

#define PLLOUT_COUNT   4
#define PLLPOSTDIV_Msk 0x3F

/* bit positions of DPLL_OUT in POSTDIV register, are spanned at fixed intervals apart. */
#define PLLOUT_POSTDIV_SPAN 8

/* Clock subsystem Types */
#define SUBSYS_TYPE_XOSC       (0)
#define SUBSYS_TYPE_DFLL48M    (1)
#define SUBSYS_TYPE_DPLL       (2)
#define SUBSYS_TYPE_DPLL_OUT   (3)
#define SUBSYS_TYPE_RTC        (4)
#define SUBSYS_TYPE_XOSC32K    (5)
#define SUBSYS_TYPE_GCLKGEN    (6)
#define SUBSYS_TYPE_GCLKPERIPH (7)
#define SUBSYS_TYPE_MCLKDOMAIN (8)
#define SUBSYS_TYPE_MCLKPERIPH (9)
#define SUBSYS_TYPE_MAX        (9)

/* mclk bus */
#define MCLKMSK0     (0)
#define MCLKMSK1     (1)
#define MCLKMSK2     (2)
#define MMASKREG_MAX (2)

/* XOSC32K instances */
#define INST_XOSC32K_XOSC1K  0
#define INST_XOSC32K_XOSC32K 1

/* Clock subsystem definition.
 *
 * Structure which can be used as a sys argument in the clock_control API.
 * Encode clock type, mclk bus, mclk mask bit, gclk pch and instance number,
 * to clock subsystem.
 *
 * - 00..07 (8 bits): inst
 * - 08..13 (6 bits): gclkperiph
 * - 14..19 (6 bits): mclkmaskbit
 * - 20..25 (6 bits): mclkmaskreg
 * - 26..31 (6 bits): type
 */
union clock_mchp_subsys {
	uint32_t val;
	struct {
		uint32_t inst: 8;
		uint32_t gclkperiph: 6;
		uint32_t mclkmaskbit: 6;
		uint32_t mclkmaskreg: 6;
		uint32_t type: 6;
	} bits;
};

#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP

/* XOSC initialization structure. */
struct clock_xosc_init {
	uint8_t usb_ref_clock_div;          /* XOSCCTRLA */
	uint8_t startup_time;               /* XOSCCTRLA */
	uint8_t on_demand_en;               /* XOSCCTRLA */
	uint8_t clock_switch_back_en;       /* XOSCCTRLA */
	uint8_t clock_failure_detection_en; /* XOSCCTRLA */
	uint8_t xtal_en;                    /* XOSCCTRLA */
	uint8_t auto_gain_control_loop_en;  /* XOSCCTRLA */
	uint8_t enable;                     /* XOSCCTRLA */
	uint32_t frequency;
};

/* DFLL48M initialization structure. */
struct clock_dfll48m_init {
	uint8_t on_demand_en;     /* DFLLCTRLA */
	uint8_t enable;           /* DFLLCTRLA */
	uint8_t wait_lock_en;     /* DFLLCTRLB */
	uint8_t quick_lock_dis;   /* DFLLCTRLB */
	uint8_t chill_cycle_dis;  /* DFLLCTRLB */
	uint8_t lose_lock_en;     /* DFLLCTRLB */
	uint8_t stable_freq_en;   /* DFLLCTRLB */
	uint8_t closed_loop_en;   /* DFLLCTRLB */
	uint8_t tune_max_step;    /* DFLLMUL */
	uint16_t multiply_factor; /* DFLLMUL */
	uint8_t src_gclk;         /* PCHCTRL */
};

/* DPLL initialization structure. */
struct clock_dpll_init {
	union clock_mchp_subsys subsys;
	uint16_t feedback_divider_factor; /* PLLFBDIV */
	uint8_t ref_division_factor;      /* PLLREFDIV */
	uint8_t bandwidth_sel;            /* DPLLCTRL */
	uint8_t src;                      /* DPLLCTRL */
	uint8_t on_demand_en;             /* DPLLCTRL */
	uint8_t enable;                   /* DPLLCTRL */
};

/* DPLL OUT initialization structure. */
struct clock_dpll_out_init {
	union clock_mchp_subsys subsys;
	uint8_t output_en;              /* PLLPOSTDIVA */
	uint8_t output_division_factor; /* PLLPOSTDIVA */
	uint16_t fract_divider_int;     /* FRACDIV */
	uint16_t fract_divider_rem;     /* FRACDIV */
};

/* XOSC32K initialization structure. */
struct clock_xosc32k_init {
	uint8_t control_gain_mode; /* XOSC32K */
	uint8_t startup_time;      /* XOSC32K */
	uint8_t on_demand_en;      /* XOSC32K */
	uint8_t xtal_en;           /* XOSC32K */
	uint8_t enable;            /* XOSC32K */
	uint8_t cfd_prescaler;     /* CFDCTRL */
	uint8_t cfd_switchback_en; /* CFDCTRL */
	uint8_t cfd_en;            /* CFDCTRL */
};

/* GCLKGEN initialization structure. */
struct clock_gclkgen_init {
	union clock_mchp_subsys subsys;
	uint16_t div_factor;
	uint8_t run_in_standby_en;
	uint8_t div_select;
	uint8_t pin_output_en;
	uint8_t pin_output_off_val;
	uint8_t duty_50_50_en;
	uint8_t enable;
	uint8_t src;
	uint32_t pin_src_freq;
};

#endif /* CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP */

/* clock driver configuration structure. */
struct clock_mchp_config {
	oscctrl_registers_t *oscctrl_regs;
	osc32kctrl_registers_t *osc32kctrl_regs;
	gclk_registers_t *gclk_regs;
	mclk_registers_t *mclk_regs;
	supc_registers_t *supc_regs;

	/* Timeout in milliseconds to wait for clock to turn on */
	uint32_t on_timeout_ms;
};

/* clock driver data structure. */
struct clock_mchp_data {
	uint32_t xosc_crystal_freq;
	uint32_t gclkpin_freq[GCLK_IO_MAX - GCLK_IO_MIN + 1];

	/* Use bit position as per DPLL_TYPE subsystem ids, to show if specified dpll clock is on */
	uint8_t dpll_on_status;

	/* Use bit position as per DPLL_TYPE subsystem ids, to show if clock is requested for on */
	uint8_t dpll_on_request;

	/* Use bit position as per enum enum clock_mchp_dpll_src_clock, to show if the specified
	 * clock source to DPLL is on.
	 */
	uint32_t dpll_src_on_status;

	/* Use bit position as per enum enum clock_mchp_gclk_src_clock, to show if the specified
	 * clock source to gclk generator is on.
	 */
	uint16_t gclkgen_src_on_status;

	enum clock_mchp_gclk_src_clock gclk0_src;
};

#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE
static int clock_get_rate_dfll(const struct device *dev, uint32_t *freq);
static int clock_get_rate_dpll(const struct device *dev, uint8_t dpll_id, uint32_t *freq);
static int clock_get_rate_dpll_out(const struct device *dev, uint8_t dpll_out_id, uint32_t *freq);
#endif /* CONFIG_CLOCK_CONTROL_MCHP_GET_RATE */

static enum clock_control_status clock_mchp_get_status(const struct device *dev,
						       clock_control_subsys_t sys);

/* Internal helper functions */

/* check if subsystem type and id are valid. */
static int clock_check_subsys(union clock_mchp_subsys subsys)
{
	uint32_t inst_max = 0, gclkperiph_max = GCLK_PH_NA, mclkmaskreg_max = MMASKREG_NA,
		 mclkmaskbit_max = MMASKBIT_NA;

	/* Check if turning on all clocks is requested. */
	if (subsys.val == (uint32_t)CLOCK_CONTROL_SUBSYS_ALL) {
		return -EINVAL;
	}

	/* Check if the specified subsystem was found. */
	if (subsys.bits.type > SUBSYS_TYPE_MAX) {
		return -EINVAL;
	}

	switch (subsys.bits.type) {
	case SUBSYS_TYPE_XOSC:
		inst_max = CLOCK_MCHP_XOSC_ID_MAX;
		break;

	case SUBSYS_TYPE_DFLL48M:
		inst_max = CLOCK_MCHP_DFLL48M_ID_MAX;
		gclkperiph_max = CLOCK_MCHP_DFLL48M_ID_MAX;
		break;

	case SUBSYS_TYPE_DPLL:
		inst_max = CLOCK_MCHP_DPLL_ID_MAX;
		gclkperiph_max = CLOCK_MCHP_DPLL_ID_MAX;
		break;

	case SUBSYS_TYPE_DPLL_OUT:
		inst_max = CLOCK_MCHP_DPLL_OUT_ID_MAX;
		break;

	case SUBSYS_TYPE_RTC:
		inst_max = CLOCK_MCHP_RTC_ID_MAX;
		break;

	case SUBSYS_TYPE_XOSC32K:
		inst_max = CLOCK_MCHP_XOSC32K_ID_MAX;
		break;

	case SUBSYS_TYPE_GCLKGEN:
		inst_max = CLOCK_MCHP_GCLKGEN_ID_MAX;
		break;

	case SUBSYS_TYPE_GCLKPERIPH:
		inst_max = CLOCK_MCHP_GCLKPERIPH_ID_MAX;
		gclkperiph_max = GCLK_PH_MAX;
		break;

	case SUBSYS_TYPE_MCLKDOMAIN:
		inst_max = CLOCK_MCHP_MCLKDOMAIN_MAX;
		break;

	case SUBSYS_TYPE_MCLKPERIPH:
		inst_max = CLOCK_MCHP_MCLKPERIPH_ID_MAX;
		mclkmaskreg_max = MMASKREG_MAX;
		mclkmaskbit_max = BIT_MASK_MAX;
		break;

	default:
		LOG_ERR("%s: Unsupported SUBSYS_TYPE", __func__);
		return -EINVAL;
	}

	/* Check if the specified id is valid. */
	if ((subsys.bits.inst > inst_max) || (subsys.bits.gclkperiph > gclkperiph_max) ||
	    (subsys.bits.mclkmaskreg > mclkmaskreg_max) ||
	    (subsys.bits.mclkmaskbit > mclkmaskbit_max)) {
		LOG_ERR("%s: Unsupported SUBSYS_ID", __func__);
		return -EINVAL;
	}

	return 0;
}

int clock_on_dpll(const struct device *dev, int inst)
{
	struct clock_mchp_data *data = dev->data;
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	uint32_t mask;

	*(&oscctrl_regs->OSCCTRL_PLL0CTRL + (inst * DPLLREG_OFFSET)) |= OSCCTRL_PLL0CTRL_ENABLE_Msk;

	mask = BIT(OSCCTRL_STATUS_PLL0LOCK_Pos + inst);
	if (WAIT_FOR(((oscctrl_regs->OSCCTRL_STATUS & mask) == mask), TIMEOUT_DPLL_LOCK, NULL) ==
	    false) {
		LOG_ERR("%s: DPLL[%d] lock timed out", __func__, inst);
		return -ETIMEDOUT;
	}
	/* Set DPLL clock status as on */
	data->dpll_on_status |= BIT(inst);

	return 0;
}

int clock_on_dpll_out(const struct device *dev, int inst)
{
	struct clock_mchp_data *data = dev->data;
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;

	uint32_t pos_en;
	int pll;

	pos_en = ((inst % PLLOUT_COUNT) + 1) * PLLOUT_POSTDIV_SPAN - 1;

	/* same macro for both DPLL0 and DPLL1 */
	pll = inst / PLLOUT_COUNT;
	*(&(oscctrl_regs->OSCCTRL_PLL0POSTDIVA) + (pll * DPLLREG_OFFSET)) |= BIT(pos_en);

	/* Set dpll_out status as on */
	data->gclkgen_src_on_status |= BIT(CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT0 + inst);

	/* Switch on corresponding DPLL, if not already on. */
	if (((data->dpll_on_status & BIT(pll)) == 0) && ((data->dpll_on_request & BIT(pll)) != 0)) {
		return clock_on_dpll(dev, pll);
	}

	return 0;
}

/* function to set clock subsystem enable bit. */
static int clock_on(const struct device *dev, const union clock_mchp_subsys subsys)
{
	struct clock_mchp_data *data = dev->data;
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	uint8_t inst = subsys.bits.inst;

	__IO uint32_t *reg32 = NULL;

	int ret_val = 0;

	switch (subsys.bits.type) {
	case SUBSYS_TYPE_XOSC:
		oscctrl_regs->OSCCTRL_XOSCCTRLA |= OSCCTRL_XOSCCTRLA_ENABLE_Msk;
		break;

	case SUBSYS_TYPE_DFLL48M:
		oscctrl_regs->OSCCTRL_DFLLCTRLA |= OSCCTRL_DFLLCTRLA_ENABLE_Msk;
		break;

	case SUBSYS_TYPE_DPLL:
		/* Switch on the PLL only if any of the PLL_OUT is on. Else, set request bit. */
		if ((data->gclkgen_src_on_status &
		     (0xF << (CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT0 + (inst * PLLOUT_COUNT)))) != 0) {
			ret_val = clock_on_dpll(dev, inst);
		} else {
			data->dpll_on_request |= BIT(inst);
		}
		break;

	case SUBSYS_TYPE_DPLL_OUT:
		ret_val = clock_on_dpll_out(dev, inst);
		break;

	case SUBSYS_TYPE_XOSC32K:
		osc32kctrl_regs->OSC32KCTRL_XOSC32K |= OSC32KCTRL_XOSC32K_ENABLE_Msk;
		break;

	case SUBSYS_TYPE_GCLKGEN:
		/* Check if the clock is GCLKGEN0, which is always on */
		if (inst == CLOCK_MCHP_GCLKGEN_GEN0) {
			/* GCLK GEN0 is always on */
			break;
		}

		/* Enable the clock generator by setting the GENEN bit */
		gclk_regs->GCLK_GENCTRL[inst] |= GCLK_GENCTRL_GENEN_Msk;
		break;

	case SUBSYS_TYPE_GCLKPERIPH:
		gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph] |= GCLK_PCHCTRL_CHEN_Msk;
		break;

	case SUBSYS_TYPE_MCLKPERIPH:
		reg32 = &(config->mclk_regs->MCLK_CLKMSK[subsys.bits.mclkmaskreg]);
		*reg32 |= BIT(subsys.bits.mclkmaskbit);
		break;

	default:
		ret_val = -ENOTSUP;
		break;
	}

	return ret_val;
}

#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE
/* get rate of gclk generator in Hz */
static int clock_get_rate_gclkgen(const struct device *dev, enum clock_mchp_gclkgen gclkgen_id,
				  enum clock_mchp_gclk_src_clock gclkgen_called_src, uint32_t *freq)
{
	const struct clock_mchp_config *config = dev->config;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	struct clock_mchp_data *data = dev->data;
	enum clock_mchp_gclk_src_clock gclkgen_src;
	uint32_t gclkgen_src_freq = 0;
	uint16_t gclkgen_div;
	int ret_val = 0;

	bool power_div = (((gclk_regs->GCLK_GENCTRL[gclkgen_id] & GCLK_GENCTRL_DIVSEL_Msk) >>
			   GCLK_GENCTRL_DIVSEL_Pos) == GCLK_GENCTRL_DIVSEL_DIV1_Val)
				 ? false
				 : true;

	/* Return rate as 0, if clock is not on */
	if (clock_mchp_get_status(dev, (clock_control_subsys_t)MCHP_CLOCK_DERIVE_ID(
					       SUBSYS_TYPE_GCLKGEN, MMASKREG_NA, MMASKBIT_NA,
					       GCLK_PH_NA, gclkgen_id)) !=
	    CLOCK_CONTROL_STATUS_ON) {
		*freq = 0;
		return 0;
	}

	/* get source for gclk generator from gclkgen registers */
	gclkgen_src = (gclk_regs->GCLK_GENCTRL[gclkgen_id] & GCLK_GENCTRL_SRC_Msk) >>
		      GCLK_GENCTRL_SRC_Pos;

	if (gclkgen_called_src == gclkgen_src) {
		LOG_ERR("%s: Recursive dependency detected", __func__);
		return -ENOTSUP;
	} else if (gclkgen_called_src == CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT0) {
		if ((CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT1 == gclkgen_src) ||
		    (CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT2 == gclkgen_src) ||
		    (CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT3 == gclkgen_src)) {
			return -ENOTSUP;
		}
	} else if (gclkgen_called_src == CLOCK_MCHP_GCLK_SRC_DPLL1_FRC_CLKOUT0) {
		if ((CLOCK_MCHP_GCLK_SRC_DPLL1_FRC_CLKOUT1 == gclkgen_src) ||
		    (CLOCK_MCHP_GCLK_SRC_DPLL1_CLKOUT2 == gclkgen_src) ||
		    (CLOCK_MCHP_GCLK_SRC_DPLL1_CLKOUT3 == gclkgen_src)) {
			return -ENOTSUP;
		}
	} else {
	}

	switch (gclkgen_src) {
	case CLOCK_MCHP_GCLK_SRC_XOSC:
		gclkgen_src_freq = data->xosc_crystal_freq;
		break;

	case CLOCK_MCHP_GCLK_SRC_GCLKPIN:
		if ((gclkgen_id <= GCLK_IO_MAX) && (gclkgen_id >= GCLK_IO_MIN)) {
			gclkgen_src_freq = data->gclkpin_freq[gclkgen_id - GCLK_IO_MIN];
		} else {
			ret_val = -ENOTSUP;
		}
		break;

	case CLOCK_MCHP_GCLK_SRC_GCLKGEN1:
		ret_val = (gclkgen_id == CLOCK_MCHP_GCLKGEN_GEN1)
				  ? -ELOOP
				  : clock_get_rate_gclkgen(dev, CLOCK_MCHP_GCLKGEN_GEN1,
							   CLOCK_MCHP_GCLK_SRC_MAX + 1,
							   &gclkgen_src_freq);
		break;

	case CLOCK_MCHP_GCLK_SRC_OSCULP32K:
	case CLOCK_MCHP_GCLK_SRC_XOSC32K:
		gclkgen_src_freq = FREQ_32KHZ;
		break;

	case CLOCK_MCHP_GCLK_SRC_DFLL48M:
		ret_val = clock_get_rate_dfll(dev, &gclkgen_src_freq);
		break;

	case CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT0:
	case CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT1:
	case CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT2:
	case CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT3:
	case CLOCK_MCHP_GCLK_SRC_DPLL1_FRC_CLKOUT0:
	case CLOCK_MCHP_GCLK_SRC_DPLL1_FRC_CLKOUT1:
	case CLOCK_MCHP_GCLK_SRC_DPLL1_CLKOUT2:
	case CLOCK_MCHP_GCLK_SRC_DPLL1_CLKOUT3:
		ret_val = clock_get_rate_dpll_out(
			dev, gclkgen_src - CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT0, &gclkgen_src_freq);
		break;

	default:
		break;
	}
	if (ret_val != 0) {
		return ret_val;
	}

	/* get gclk generator clock divider*/
	gclkgen_div = (gclk_regs->GCLK_GENCTRL[gclkgen_id] & GCLK_GENCTRL_DIV_Msk) >>
		      GCLK_GENCTRL_DIV_Pos;

	/*
	 * For gclk1, 8 division factor bits - DIV[7:0]
	 * others, 16 division factor bits - DIV[15:0]
	 */
	if (gclkgen_id != CLOCK_MCHP_GCLKGEN_GEN1) {
		gclkgen_div = gclkgen_div & 0xFF;
	}

	if (power_div == true) {
		if (gclkgen_div > GCLKGEN_POWER_DIV_MAX) {
			gclkgen_div = GCLKGEN_POWER_DIV_MAX;
		}
		gclkgen_div = BIT(gclkgen_div + 1);
	} else {
		gclkgen_div = (gclkgen_div == 0) ? 1 : gclkgen_div;
	}
	*freq = gclkgen_src_freq / gclkgen_div;

	return ret_val;
}

/* get rate of DFLL48M in Hz. */
static int clock_get_rate_dfll(const struct device *dev, uint32_t *freq)
{
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	uint32_t multiply_factor, gclkgen_freq;
	enum clock_mchp_gclkgen src_gclkgen;
	int ret_val = 0;

	if ((oscctrl_regs->OSCCTRL_STATUS & OSCCTRL_STATUS_DFLLRDY_Msk) == 0) {
		/* Return rate as 0, if clock is not on */
		*freq = 0;
		return 0;
	}

	if ((oscctrl_regs->OSCCTRL_DFLLCTRLB & OSCCTRL_DFLLCTRLB_LOOPEN_Msk) == 0) {
		/* in open loop mode*/
		*freq = FREQ_DFLL48M;
		return 0;
	}

	/* in closed loop mode*/
	multiply_factor = (oscctrl_regs->OSCCTRL_DFLLMUL & OSCCTRL_DFLLMUL_MUL_Msk) >>
			  OSCCTRL_DFLLMUL_MUL_Pos;

	/* PCHCTRL_0 is for DFLL48M*/
	src_gclkgen =
		(config->gclk_regs->GCLK_PCHCTRL[0] & GCLK_PCHCTRL_GEN_Msk) >> GCLK_PCHCTRL_GEN_Pos;

	ret_val = clock_get_rate_gclkgen(dev, src_gclkgen, CLOCK_MCHP_GCLK_SRC_DFLL48M,
					 &gclkgen_freq);
	if (ret_val == 0) {
		*freq = multiply_factor * gclkgen_freq;
	}

	return ret_val;
}

/* get rate of DPLL in Hz. */
static int clock_get_rate_dpll(const struct device *dev, uint8_t dpll_id, uint32_t *freq)
{
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	struct clock_mchp_data *data = dev->data;
	uint32_t src_freq = 0;
	uint32_t refdiv, fbdiv;
	enum clock_mchp_gclkgen src_gclkgen;
	uint8_t ref_clk_type;
	int ret_val = 0;

	/* Return rate as 0, if clock is not on */
	if (clock_mchp_get_status(dev, (clock_control_subsys_t)MCHP_CLOCK_DERIVE_ID(
					       SUBSYS_TYPE_DPLL, MMASKREG_NA, MMASKBIT_NA,
					       dpll_id + 1, dpll_id)) != CLOCK_CONTROL_STATUS_ON) {
		*freq = 0;
		return 0;
	}

	ref_clk_type = (*(&oscctrl_regs->OSCCTRL_PLL0CTRL + (dpll_id * DPLLREG_OFFSET)) &
			OSCCTRL_PLL0CTRL_REFSEL_Msk) >>
		       OSCCTRL_PLL0CTRL_REFSEL_Pos;

	switch (ref_clk_type) {
	case OSCCTRL_PLL0CTRL_REFSEL_GCLK_Val:
		src_gclkgen =
			(config->gclk_regs->GCLK_PCHCTRL[dpll_id + 1] & GCLK_PCHCTRL_GEN_Msk) >>
			GCLK_PCHCTRL_GEN_Pos;
		ret_val = clock_get_rate_gclkgen(
			dev, src_gclkgen,
			CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT0 + (dpll_id * PLLOUT_COUNT), &src_freq);
		break;

	case OSCCTRL_PLL0CTRL_REFSEL_XOSC_Val:
		src_freq = data->xosc_crystal_freq;
		break;

	case OSCCTRL_PLL0CTRL_REFSEL_DFLL48M_Val:
		ret_val = clock_get_rate_dfll(dev, &src_freq);
		break;

	default:
		break;
	}

	if (ret_val != 0) {
		return ret_val;
	}

	refdiv = (*(&oscctrl_regs->OSCCTRL_PLL0REFDIV + (dpll_id * DPLLREG_OFFSET)) &
		  OSCCTRL_PLL0REFDIV_REFDIV_Msk) >>
		 OSCCTRL_PLL0REFDIV_REFDIV_Pos;
	if (refdiv != 0) {
		src_freq = src_freq / refdiv;
	}

	fbdiv = (*(&oscctrl_regs->OSCCTRL_PLL0FBDIV + (dpll_id * DPLLREG_OFFSET)) &
		 OSCCTRL_PLL0FBDIV_FBDIV_Msk) >>
		OSCCTRL_PLL0FBDIV_FBDIV_Pos;
	*freq = src_freq * fbdiv;

	return ret_val;
}

/* get rate of DPLL_OUT in Hz. */
static int clock_get_rate_dpll_out(const struct device *dev, uint8_t dpll_out_id, uint32_t *freq)
{
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	uint32_t src_freq = 0;
	uint32_t postdiv, pll;
	int ret_val;

	/* Return rate as 0, if clock is not on */
	if (clock_mchp_get_status(dev, (clock_control_subsys_t)MCHP_CLOCK_DERIVE_ID(
					       SUBSYS_TYPE_DPLL_OUT, MMASKREG_NA, MMASKBIT_NA,
					       GCLK_PH_NA, dpll_out_id)) !=
	    CLOCK_CONTROL_STATUS_ON) {
		*freq = 0;
		return 0;
	}

	pll = dpll_out_id / PLLOUT_COUNT;
	ret_val = clock_get_rate_dpll(dev, pll, &src_freq);
	if (ret_val == 0) {
		postdiv = *(&oscctrl_regs->OSCCTRL_PLL0POSTDIVA + (pll * DPLLREG_OFFSET)) &
			  (OSCCTRL_PLL0POSTDIVA_POSTDIV0_Msk
			   << ((dpll_out_id % PLLOUT_COUNT) * PLLOUT_POSTDIV_SPAN));
		if (postdiv != 0) {
			*freq = src_freq / postdiv;
		}
	}

	return ret_val;
}

/* get rate of RTC in Hz. */
static int clock_get_rate_rtc(const struct device *dev, uint32_t *freq)
{
	const struct clock_mchp_config *config = dev->config;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;
	uint8_t rtc_src;
	int ret_val = 0;

	/* get rtc source clock*/
	rtc_src = (osc32kctrl_regs->OSC32KCTRL_CLKSELCTRL & OSC32KCTRL_CLKSELCTRL_RTCSEL_Msk) >>
		  OSC32KCTRL_CLKSELCTRL_RTCSEL_Pos;

	switch (rtc_src) {
	case OSC32KCTRL_CLKSELCTRL_RTCSEL_ULP1K_Val:
		*freq = FREQ_1KHZ;
		break;

	case OSC32KCTRL_CLKSELCTRL_RTCSEL_ULP32K_Val:
		*freq = FREQ_32KHZ;
		break;

	case OSC32KCTRL_CLKSELCTRL_RTCSEL_XOSC1K_Val:
	case OSC32KCTRL_CLKSELCTRL_RTCSEL_XOSC32K_Val:
		if ((osc32kctrl_regs->OSC32KCTRL_XOSC32K & OSC32KCTRL_XOSC32K_ENABLE_Msk) != 0) {
			*freq = (rtc_src == OSC32KCTRL_CLKSELCTRL_RTCSEL_XOSC1K_Val) ? FREQ_1KHZ
										     : FREQ_32KHZ;
		} else {
			*freq = 0;
		}

	default:
		ret_val = -ENOTSUP;
	}

	return ret_val;
}

#endif /* CONFIG_CLOCK_CONTROL_MCHP_GET_RATE */

#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_RUNTIME
/* configure DFLL48M. */
static void clock_configure_dfll(const struct device *dev, void *req_config)
{
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	struct clock_mchp_subsys_dfll48m_config *dfll48m_config =
		(struct clock_mchp_subsys_dfll48m_config *)req_config;

	uint8_t val8;
	uint32_t val32;

	/* GCLK_PCHCTRL[0] is for DFLL48M input clock source */
	val32 = gclk_regs->GCLK_PCHCTRL[0] & ~(GCLK_PCHCTRL_GEN_Msk);
	val32 |= GCLK_PCHCTRL_GEN(dfll48m_config->src);
	gclk_regs->GCLK_PCHCTRL[0] = val32;

	if (dfll48m_config->closed_loop_en == true) {
		/* DFLLCTRLB */
		val8 = oscctrl_regs->OSCCTRL_DFLLCTRLB |= OSCCTRL_DFLLCTRLB_LOOPEN(1);
		oscctrl_regs->OSCCTRL_DFLLCTRLB = val8;
		if (WAIT_FOR((oscctrl_regs->OSCCTRL_SYNCBUSY == 0), TIMEOUT_REG_SYNC, NULL) ==
		    false) {
			LOG_ERR("%s: DFLL48MSYNC timeout on writing OSCCTRL_DFLLCTRLB", __func__);
			return;
		}

		/* DFLLMUL */
		val32 = oscctrl_regs->OSCCTRL_DFLLMUL & ~(OSCCTRL_DFLLMUL_MUL_Msk);
		val32 |= OSCCTRL_DFLLMUL_MUL(dfll48m_config->multiply_factor);
		oscctrl_regs->OSCCTRL_DFLLMUL = val32;
		if (WAIT_FOR((oscctrl_regs->OSCCTRL_SYNCBUSY == 0), TIMEOUT_REG_SYNC, NULL) ==
		    false) {
			LOG_ERR("%s: DFLL48MSYNC timeout on writing OSCCTRL_DFLLMUL", __func__);
			return;
		}
	}

	/* DFLLCTRLA */
	val8 = oscctrl_regs->OSCCTRL_DFLLCTRLA & ~(OSCCTRL_DFLLCTRLA_ONDEMAND_Msk);
	val8 |= OSCCTRL_DFLLCTRLA_ONDEMAND(dfll48m_config->on_demand_en);
	oscctrl_regs->OSCCTRL_DFLLCTRLA = val8;
	if (WAIT_FOR((oscctrl_regs->OSCCTRL_SYNCBUSY == 0), TIMEOUT_REG_SYNC, NULL) == false) {
		LOG_ERR("%s: DFLL48MSYNC timeout on writing OSCCTRL_DFLLCTRLA", __func__);
		return;
	}
}

/* configure DPLL. */
static void clock_configure_dpll(const struct device *dev, uint8_t inst, void *req_config)
{
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	struct clock_mchp_subsys_dpll_config *dpll_config =
		(struct clock_mchp_subsys_dpll_config *)req_config;
	uint32_t val32;
	int src;

	/* program gclkph if source is gclk & enable. */
	src = dpll_config->src;
	if (src <= CLOCK_MCHP_DPLL_SRC_GCLK15) {
		gclk_regs->GCLK_PCHCTRL[inst + 1] |=
			(GCLK_PCHCTRL_GEN(src) | GCLK_PCHCTRL_CHEN_Msk);
		if (WAIT_FOR(((gclk_regs->GCLK_PCHCTRL[inst + 1] & GCLK_PCHCTRL_CHEN_Msk) != 0),
			     TIMEOUT_REG_SYNC, NULL) == false) {
			LOG_ERR("%s: timeout on writing GCLK_PCHCTRL_CHEN_Msk", __func__);
			return;
		}
	}

	/* DPLLFBDIV, access DPLL0 or DPLL1 registers based on inst and offset */
	/* macro value is same for both DPLL0 and DPLL1 */
	val32 = *(&oscctrl_regs->OSCCTRL_PLL0FBDIV + (inst * DPLLREG_OFFSET));
	val32 &= ~(OSCCTRL_PLL0FBDIV_FBDIV_Msk);
	val32 |= OSCCTRL_PLL0FBDIV_FBDIV(dpll_config->feedback_divider_factor);
	*(&oscctrl_regs->OSCCTRL_PLL0FBDIV + (inst * DPLLREG_OFFSET)) = val32;

	/* DPLLREFDIV, access DPLL0 or DPLL1 registers based on inst and offset */
	/* macro is same for both DPLL0 and DPLL1 */
	val32 = *(&oscctrl_regs->OSCCTRL_PLL0REFDIV + (inst * DPLLREG_OFFSET));
	val32 &= ~(OSCCTRL_PLL0REFDIV_REFDIV_Msk);
	val32 |= OSCCTRL_PLL0REFDIV_REFDIV(dpll_config->ref_division_factor);
	*(&oscctrl_regs->OSCCTRL_PLL0REFDIV + (inst * DPLLREG_OFFSET)) = val32;

	/* DPLLCTRL, access DPLL0 or DPLL1 registers based on inst and offset */
	/* macro is same for both DPLL0 and DPLL1 */
	val32 = *(&oscctrl_regs->OSCCTRL_PLL0CTRL + (inst * DPLLREG_OFFSET));
	val32 &= ~(OSCCTRL_PLL0CTRL_REFSEL_Msk | OSCCTRL_PLL0CTRL_ONDEMAND_Msk);
	val32 |= OSCCTRL_PLL0CTRL_REFSEL(
		(src > CLOCK_MCHP_DPLL_SRC_GCLK15) ? (src - CLOCK_MCHP_DPLL_SRC_GCLK15) : 0);
	val32 |= OSCCTRL_PLL0CTRL_ONDEMAND(dpll_config->on_demand_en);
	*(&oscctrl_regs->OSCCTRL_PLL0CTRL + (inst * DPLLREG_OFFSET)) = val32;
}

/* configure DPLL_OUT. */
static void clock_configure_dpll_out(const struct device *dev, uint8_t inst, void *req_config)
{
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	struct clock_mchp_subsys_dpll_out_config *dpll_out_config =
		(struct clock_mchp_subsys_dpll_out_config *)req_config;

	uint32_t val32, pos_postdiv;
	int pll;

	pos_postdiv = (inst % PLLOUT_COUNT) * PLLOUT_POSTDIV_SPAN;

	/* same macro for both DPLL0 and DPLL1 */
	pll = inst / PLLOUT_COUNT;
	val32 = *(&oscctrl_regs->OSCCTRL_PLL0POSTDIVA + (pll * DPLLREG_OFFSET));
	val32 &= ~((0x3F << pos_postdiv));
	val32 |= (dpll_out_config->output_division_factor << pos_postdiv);
	*(&(oscctrl_regs->OSCCTRL_PLL0POSTDIVA) + (pll * DPLLREG_OFFSET)) = val32;
}

/* configure GCLKGEN. */
static void clock_configure_gclkgen(const struct device *dev, uint8_t inst, void *req_config)
{
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;
	uint32_t val32;

	struct clock_mchp_subsys_gclkgen_config *gclkgen_config =
		(struct clock_mchp_subsys_gclkgen_config *)req_config;

	/* GENCTRL */
	val32 = config->gclk_regs->GCLK_GENCTRL[inst] &
		~(GCLK_GENCTRL_DIV_Msk | GCLK_GENCTRL_RUNSTDBY_Msk | GCLK_GENCTRL_SRC_Msk);

	/* check range for div_factor, gclk1: 0 - 65535, others: 0 - 255 */
	if ((inst == 1) || (gclkgen_config->div_factor <= 0xFF)) {
		val32 |= GCLK_GENCTRL_DIV(gclkgen_config->div_factor);
	}
	val32 |= ((gclkgen_config->run_in_standby_en != 0) ? GCLK_GENCTRL_RUNSTDBY(1) : 0);
	val32 |= GCLK_GENCTRL_SRC(gclkgen_config->src);
	config->gclk_regs->GCLK_GENCTRL[inst] = val32;
	if (WAIT_FOR((config->gclk_regs->GCLK_SYNCBUSY == 0), TIMEOUT_REG_SYNC, NULL) == false) {
		LOG_ERR("%s: GCLK_SYNCBUSY timeout on writing GCLK_GENCTRL[%d]", __func__, inst);
		return;
	}

	/* To avoid changing dfll48m, while gclk0 is driven by it. Else will affect CPU */
	if (inst == CLOCK_MCHP_GCLKGEN_GEN0) {
		data->gclk0_src = gclkgen_config->src;
	}
}
#endif /* CONFIG_CLOCK_CONTROL_MCHP_CONFIG_RUNTIME */

/* API functions */

static int clock_mchp_on(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_mchp_config *config = dev->config;
	union clock_mchp_subsys subsys = {.val = (uint32_t)sys};
	enum clock_control_status status;
	uint32_t on_timeout_ms = 0;
	int ret_val = 0;

	/* Validate subsystem. */
	if (0 != clock_check_subsys(subsys)) {
		return -ENOTSUP;
	}

	status = clock_mchp_get_status(dev, sys);
	if (status == CLOCK_CONTROL_STATUS_ON) {
		/* clock is already on. */
		return -EALREADY;
	}

	/* Check if the clock on operation is successful. */
	if (clock_on(dev, subsys) != 0) {
		return -ENOTSUP;
	}

	/* Wait until the clock state becomes ON. */
	while (1) {
		/* For XOSC32K, need to wait for the oscillator to be on. get_status only
		 * return if EN1K or EN32K is on, which does not indicate the status of
		 * XOSC32K
		 */
		if (subsys.bits.type == SUBSYS_TYPE_XOSC32K) {
			osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;

			if ((osc32kctrl_regs->OSC32KCTRL_STATUS &
			     OSC32KCTRL_STATUS_XOSC32KRDY_Msk) != 0) {
				break;
			}
		} else {
			status = clock_mchp_get_status(dev, sys);
			if (status == CLOCK_CONTROL_STATUS_ON) {
				break;
			}
		}

		if (on_timeout_ms < config->on_timeout_ms) {
			/* Thread is not available while booting. */
			if ((k_is_pre_kernel() == false) && (k_current_get() != NULL)) {
				/* Sleep before checking again. */
				k_sleep(K_MSEC(1));
			} else {
				WAIT_FOR(0, 1000, NULL);
			}
			on_timeout_ms++;
		} else {
			/* Clock on timeout occurred */
			ret_val = -ETIMEDOUT;
			break;
		}
	}

	return ret_val;
}

static int clock_mchp_off(const struct device *dev, clock_control_subsys_t sys)
{
	struct clock_mchp_data *data = dev->data;
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	union clock_mchp_subsys subsys = {.val = (uint32_t)sys};
	uint8_t inst = subsys.bits.inst;
	__IO uint32_t *reg32 = NULL;
	uint32_t pos_en;
	int pll;

	/* Validate subsystem. */
	if (0 != clock_check_subsys(subsys)) {
		return -ENOTSUP;
	}

	switch (subsys.bits.type) {
	case SUBSYS_TYPE_XOSC:
		oscctrl_regs->OSCCTRL_XOSCCTRLA &= ~(OSCCTRL_XOSCCTRLA_ENABLE_Msk);
		break;

	case SUBSYS_TYPE_DFLL48M:
		oscctrl_regs->OSCCTRL_DFLLCTRLA &= ~(OSCCTRL_DFLLCTRLA_ENABLE_Msk);
		break;

	case SUBSYS_TYPE_DPLL:
		*(&oscctrl_regs->OSCCTRL_PLL0CTRL + (inst * DPLLREG_OFFSET)) &=
			~(OSCCTRL_PLL0CTRL_ENABLE_Msk);
		data->dpll_on_request &= ~BIT(inst);
		data->dpll_on_status &= ~BIT(inst);
		break;

	case SUBSYS_TYPE_DPLL_OUT:
		/* Find the bit position for the specified PLLOUT */
		pos_en = (((inst % PLLOUT_COUNT) + 1) * PLLOUT_POSTDIV_SPAN) - 1;

		/* same macro for both DPLL0 and DPLL1 */
		pll = inst / PLLOUT_COUNT;
		*(&(oscctrl_regs->OSCCTRL_PLL0POSTDIVA) + (pll * DPLLREG_OFFSET)) &= ~BIT(pos_en);

		/* Set dpll_out status as off */
		data->gclkgen_src_on_status &= ~BIT(CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT0 + inst);
		break;

	case SUBSYS_TYPE_XOSC32K:
		osc32kctrl_regs->OSC32KCTRL_XOSC32K &= ~OSC32KCTRL_XOSC32K_ENABLE_Msk;
		break;

	case SUBSYS_TYPE_GCLKGEN:
		/* Check if the clock is GCLKGEN0, which is always on */
		if (inst == CLOCK_MCHP_GCLKGEN_GEN0) {
			/* GCLK GEN0 is always on */
			return -ENOTSUP;
		}

		/* Disable the clock generator by setting the GENEN bit */
		gclk_regs->GCLK_GENCTRL[inst] &= ~(GCLK_GENCTRL_GENEN_Msk);
		break;

	case SUBSYS_TYPE_GCLKPERIPH:
		gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph] &= ~(GCLK_PCHCTRL_CHEN_Msk);
		break;

	case SUBSYS_TYPE_MCLKPERIPH:
		reg32 = &(config->mclk_regs->MCLK_CLKMSK[subsys.bits.mclkmaskreg]);
		*reg32 &= ~BIT(subsys.bits.mclkmaskbit);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

/* get status of respective clock subsystem. */
static enum clock_control_status clock_mchp_get_status(const struct device *dev,
						       clock_control_subsys_t sys)
{
	enum clock_control_status ret_status = CLOCK_CONTROL_STATUS_UNKNOWN;
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	union clock_mchp_subsys subsys = {.val = (uint32_t)sys};
	uint8_t inst = subsys.bits.inst;
	uint32_t mask, pll;

	__IO uint32_t *reg32 = NULL;

	/* Validate subsystem. */
	if (0 != clock_check_subsys(subsys)) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	switch (subsys.bits.type) {
	case SUBSYS_TYPE_XOSC:
		/* Check if XOSC is enabled */
		if ((oscctrl_regs->OSCCTRL_XOSCCTRLA & OSCCTRL_XOSCCTRLA_ENABLE_Msk) != 0) {
			/* Check if ready bit is set */
			ret_status =
				((oscctrl_regs->OSCCTRL_STATUS & OSCCTRL_STATUS_XOSCRDY_Msk) == 0)
					? CLOCK_CONTROL_STATUS_STARTING
					: CLOCK_CONTROL_STATUS_ON;
		} else {
			ret_status = CLOCK_CONTROL_STATUS_OFF;
		}
		break;

	case SUBSYS_TYPE_DFLL48M:
		/* Check if DFLL48M is enabled */
		if ((oscctrl_regs->OSCCTRL_DFLLCTRLA & OSCCTRL_DFLLCTRLA_ENABLE_Msk) != 0) {
			/* Check if sync is complete and ready bit is set */
			ret_status =
				((oscctrl_regs->OSCCTRL_SYNCBUSY != 0) ||
				 ((oscctrl_regs->OSCCTRL_STATUS & OSCCTRL_STATUS_DFLLRDY_Msk) == 0))
					? CLOCK_CONTROL_STATUS_STARTING
					: CLOCK_CONTROL_STATUS_ON;
		} else {
			ret_status = CLOCK_CONTROL_STATUS_OFF;
		}
		break;

	case SUBSYS_TYPE_DPLL:
		/* Check if DPLL is enabled */
		if ((*(&(oscctrl_regs->OSCCTRL_PLL0CTRL) + (inst * DPLLREG_OFFSET)) &
		     OSCCTRL_PLL0CTRL_ENABLE_Msk) != 0) {
			mask = BIT(OSCCTRL_STATUS_PLL0LOCK_Pos + inst);

			/* Check if DPLL is locked */
			ret_status = ((oscctrl_regs->OSCCTRL_STATUS & mask) != mask)
					     ? CLOCK_CONTROL_STATUS_STARTING
					     : CLOCK_CONTROL_STATUS_ON;
		} else {
			ret_status = CLOCK_CONTROL_STATUS_OFF;
		}
		break;

	case SUBSYS_TYPE_DPLL_OUT:
		/* Check if DPLL is enabled */
		pll = inst / PLLOUT_COUNT;
		if ((oscctrl_regs->OSCCTRL_STATUS & BIT(OSCCTRL_STATUS_PLL0LOCK_Pos + pll)) != 0) {
			/* Find the bit position for the specified PLLOUT */
			mask = BIT((((inst % PLLOUT_COUNT) + 1) * PLLOUT_POSTDIV_SPAN) - 1);
			ret_status =
				((*(&oscctrl_regs->OSCCTRL_PLL0POSTDIVA + (pll * DPLLREG_OFFSET)) &
				  mask) != 0)
					? CLOCK_CONTROL_STATUS_ON
					: CLOCK_CONTROL_STATUS_OFF;
		} else {
			ret_status = CLOCK_CONTROL_STATUS_OFF;
		}
		break;

	case SUBSYS_TYPE_RTC:
		ret_status = CLOCK_CONTROL_STATUS_ON;
		break;

	case SUBSYS_TYPE_XOSC32K:
		/* Check if XOSC32K is enabled */
		if ((osc32kctrl_regs->OSC32KCTRL_XOSC32K & OSC32KCTRL_XOSC32K_ENABLE_Msk) != 0) {
			/* Check if ready bit is set */
			ret_status = ((osc32kctrl_regs->OSC32KCTRL_STATUS &
				       OSC32KCTRL_STATUS_XOSC32KRDY_Msk) != 0)
					     ? CLOCK_CONTROL_STATUS_ON
					     : CLOCK_CONTROL_STATUS_STARTING;

		} else {
			ret_status = CLOCK_CONTROL_STATUS_OFF;
		}
		break;

	case SUBSYS_TYPE_GCLKGEN:
		if ((gclk_regs->GCLK_GENCTRL[inst] & GCLK_GENCTRL_GENEN_Msk) != 0) {
			/* Generator is on, check if it's starting or fully on */
			ret_status = ((gclk_regs->GCLK_SYNCBUSY &
				       BIT(GCLK_SYNCBUSY_GENCTRL_Pos + inst)) != 0)
					     ? CLOCK_CONTROL_STATUS_STARTING
					     : CLOCK_CONTROL_STATUS_ON;
		} else {
			ret_status = CLOCK_CONTROL_STATUS_OFF;
		}
		break;

	case SUBSYS_TYPE_GCLKPERIPH:
		/* Check if the peripheral clock is enabled */
		ret_status = ((gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph] &
			       GCLK_PCHCTRL_CHEN_Msk) != 0)
				     ? CLOCK_CONTROL_STATUS_ON
				     : CLOCK_CONTROL_STATUS_OFF;
		break;

	case SUBSYS_TYPE_MCLKDOMAIN:
		ret_status = CLOCK_CONTROL_STATUS_ON;
		break;

	case SUBSYS_TYPE_MCLKPERIPH:
		reg32 = &(config->mclk_regs->MCLK_CLKMSK[subsys.bits.mclkmaskreg]);
		mask = BIT(subsys.bits.mclkmaskbit);
		ret_status =
			((*reg32 & mask) != 0) ? CLOCK_CONTROL_STATUS_ON : CLOCK_CONTROL_STATUS_OFF;
		break;

	default:
		break;
	}

	/* Return the status of the clock for the specified subsystem. */
	return ret_status;
}

#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE

static int clock_mchp_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *freq)
{
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;
	union clock_mchp_subsys subsys = {.val = (uint32_t)sys};
	uint8_t inst = subsys.bits.inst;
	uint8_t mclk_div;
	uint32_t gclkgen_src_freq = 0;
	enum clock_mchp_gclkgen gclkperiph_src;
	int ret_val = 0;

	/* Validate subsystem. */
	if (0 != clock_check_subsys(subsys)) {
		return -ENOTSUP;
	}

	/* Return rate as 0, if clock is not on */
	if (clock_mchp_get_status(dev, sys) != CLOCK_CONTROL_STATUS_ON) {
		*freq = 0;
		return 0;
	}

	switch (subsys.bits.type) {
	case SUBSYS_TYPE_XOSC:
		*freq = data->xosc_crystal_freq;
		break;

	case SUBSYS_TYPE_DFLL48M:
		ret_val = clock_get_rate_dfll(dev, freq);
		break;

	case SUBSYS_TYPE_DPLL:
		ret_val = clock_get_rate_dpll(dev, inst, freq);
		break;

	case SUBSYS_TYPE_DPLL_OUT:
		ret_val = clock_get_rate_dpll_out(dev, inst, freq);
		break;

	case SUBSYS_TYPE_RTC:
		ret_val = clock_get_rate_rtc(dev, freq);
		break;

	case SUBSYS_TYPE_XOSC32K:
		*freq = FREQ_32KHZ;
		break;

	case SUBSYS_TYPE_GCLKGEN:
		ret_val = clock_get_rate_gclkgen(dev, inst, CLOCK_MCHP_GCLK_SRC_MAX + 1, freq);
		break;

	case SUBSYS_TYPE_GCLKPERIPH:
		gclkperiph_src = (config->gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph] &
				  GCLK_PCHCTRL_GEN_Msk) >>
				 GCLK_PCHCTRL_GEN_Pos;
		ret_val = clock_get_rate_gclkgen(dev, gclkperiph_src, CLOCK_MCHP_GCLK_SRC_MAX + 1,
						 freq);
		break;

	case SUBSYS_TYPE_MCLKDOMAIN:
	case SUBSYS_TYPE_MCLKPERIPH:
		/* source for mclk is always gclk0 */
		ret_val = clock_get_rate_gclkgen(dev, 0, CLOCK_MCHP_GCLK_SRC_MAX + 1,
						 &gclkgen_src_freq);
		if (ret_val == 0) {
			mclk_div = (config->mclk_regs->MCLK_CLKDIV[inst] & MCLK_CLKDIV_DIV_Msk) >>
				   MCLK_CLKDIV_DIV_Pos;
			if (mclk_div != 0) {
				*freq = gclkgen_src_freq / mclk_div;
			}
		}
		break;

	default:
		ret_val = -ENOTSUP;
		break;
	}

	return ret_val;
}

#endif /* CONFIG_CLOCK_CONTROL_MCHP_GET_RATE */

#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_RUNTIME

static int clock_mchp_configure(const struct device *dev, clock_control_subsys_t sys,
				void *req_config)
{
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;
	union clock_mchp_subsys subsys;
	uint32_t val32;
	int inst;

	subsys.val = (uint32_t)sys;
	inst = subsys.bits.inst;

	if (req_config == NULL) {
		return -EINVAL;
	}

	/* Validate subsystem. */
	if (0 != clock_check_subsys(subsys)) {
		return -ENOTSUP;
	}

	switch (subsys.bits.type) {
	case SUBSYS_TYPE_XOSC:
		struct clock_mchp_subsys_xosc_config *xosc_config =
			(struct clock_mchp_subsys_xosc_config *)req_config;

		val32 = oscctrl_regs->OSCCTRL_XOSCCTRLA & ~(OSCCTRL_XOSCCTRLA_ONDEMAND_Msk);
		val32 |= OSCCTRL_XOSCCTRLA_ONDEMAND(xosc_config->on_demand_en);
		oscctrl_regs->OSCCTRL_XOSCCTRLA = val32;
		break;

	case SUBSYS_TYPE_DFLL48M:
		clock_configure_dfll(dev, req_config);
		break;

	case SUBSYS_TYPE_DPLL:
		clock_configure_dpll(dev, inst, req_config);
		break;

	case SUBSYS_TYPE_DPLL_OUT:
		clock_configure_dpll_out(dev, inst, req_config);
		break;

	case SUBSYS_TYPE_RTC:
		struct clock_mchp_subsys_rtc_config *rtc_config =
			(struct clock_mchp_subsys_rtc_config *)req_config;
		osc32kctrl_regs->OSC32KCTRL_CLKSELCTRL =
			OSC32KCTRL_CLKSELCTRL_RTCSEL(rtc_config->src);
		break;

	case SUBSYS_TYPE_XOSC32K:
		struct clock_mchp_subsys_xosc32k_config *xosc32k_config =
			(struct clock_mchp_subsys_xosc32k_config *)req_config;

		val32 = osc32kctrl_regs->OSC32KCTRL_XOSC32K & ~(OSC32KCTRL_XOSC32K_ONDEMAND_Msk);
		val32 |= OSC32KCTRL_XOSC32K_ONDEMAND(xosc32k_config->on_demand_en);
		osc32kctrl_regs->OSC32KCTRL_XOSC32K = val32;
		break;

	case SUBSYS_TYPE_GCLKGEN:
		clock_configure_gclkgen(dev, inst, req_config);
		break;

	case SUBSYS_TYPE_GCLKPERIPH:
		struct clock_mchp_subsys_gclkperiph_config *gclkperiph_config =
			(struct clock_mchp_subsys_gclkperiph_config *)req_config;

		val32 = config->gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph] &
			~(GCLK_PCHCTRL_GEN_Msk);
		val32 |= GCLK_PCHCTRL_GEN(gclkperiph_config->src);
		config->gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph] = val32;
		break;

	case SUBSYS_TYPE_MCLKDOMAIN:
		struct clock_mchp_subsys_mclkcpu_config *mclkcpu_config =
			(struct clock_mchp_subsys_mclkcpu_config *)req_config;

		val32 = config->mclk_regs->MCLK_CLKDIV[inst] & ~(MCLK_CLKDIV_DIV_Msk);
		val32 |= MCLK_CLKDIV_DIV(mclkcpu_config->division_factor);
		config->mclk_regs->MCLK_CLKDIV[inst] = val32;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_CLOCK_CONTROL_MCHP_CONFIG_RUNTIME */

#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP
/* Internal initialization functions */

/* initialize XOSC from device tree node. */
void clock_xosc_init(const struct device *dev, struct clock_xosc_init *xosc_init)
{
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	struct clock_mchp_data *data = dev->data;

	uint32_t val32;

	/* XOSCCTRLA */
	val32 = oscctrl_regs->OSCCTRL_XOSCCTRLA &
		~(OSCCTRL_XOSCCTRLA_USBHSDIV_Msk | OSCCTRL_XOSCCTRLA_STARTUP_Msk |
		  OSCCTRL_XOSCCTRLA_ONDEMAND_Msk | OSCCTRL_XOSCCTRLA_SWBEN_Msk |
		  OSCCTRL_XOSCCTRLA_CFDEN_Msk | OSCCTRL_XOSCCTRLA_XTALEN_Msk |
		  OSCCTRL_XOSCCTRLA_AGC_Msk | OSCCTRL_XOSCCTRLA_ENABLE_Msk);

	val32 |= OSCCTRL_XOSCCTRLA_USBHSDIV(xosc_init->usb_ref_clock_div);
	val32 |= OSCCTRL_XOSCCTRLA_STARTUP(xosc_init->startup_time);

	/* Important: Initializing it with 1, along with clock enabled, can lead to
	 * indefinite wait for the clock to be on, if there is no peripheral request
	 * for the clock in the sequence of clock Initialization. If required,
	 * better to turn on the clock using API, instead of enabling both
	 * (on_demand_en & enable) during startup.
	 */
	val32 |= OSCCTRL_XOSCCTRLA_ONDEMAND(xosc_init->on_demand_en);
	val32 |= OSCCTRL_XOSCCTRLA_SWBEN(xosc_init->clock_switch_back_en);
	val32 |= OSCCTRL_XOSCCTRLA_CFDEN(xosc_init->clock_failure_detection_en);
	val32 |= OSCCTRL_XOSCCTRLA_XTALEN(xosc_init->xtal_en);
	val32 |= OSCCTRL_XOSCCTRLA_AGC(xosc_init->auto_gain_control_loop_en);
	val32 |= OSCCTRL_XOSCCTRLA_ENABLE(xosc_init->enable);
	oscctrl_regs->OSCCTRL_XOSCCTRLA = val32;

	data->xosc_crystal_freq = xosc_init->frequency;

	if (xosc_init->enable != 0) {
		if (WAIT_FOR(((oscctrl_regs->OSCCTRL_STATUS & OSCCTRL_STATUS_XOSCRDY_Msk) != 0),
			     TIMEOUT_XOSC_RDY, NULL) == false) {
			LOG_ERR("%s: XOSC ready timed out", __func__);

		} else {
			/* Set xosc clock as on */
			data->dpll_src_on_status |= BIT(CLOCK_MCHP_DPLL_SRC_XOSC);
			data->gclkgen_src_on_status |= BIT(CLOCK_MCHP_GCLK_SRC_XOSC);
		}
	}
}

/* initialize DFLL48M from device tree node. */
void clock_dfll48m_init(const struct device *dev, struct clock_dfll48m_init *dfll48m_init)
{
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;

	int gclkgen_index;
	uint8_t val8;
	uint32_t val32;

	/* Check if DFLL48M is already initialized and on */
	if ((data->gclkgen_src_on_status & BIT(CLOCK_MCHP_GCLK_SRC_DFLL48M)) != 0) {
		LOG_INF("%s: skipping dfll48m_init, as DFLL is already on", __func__);
		return;
	}

	/* To avoid changing dfll48m, while gclk0 is driven by it. Else will affect CPU */
	if (data->gclk0_src != CLOCK_MCHP_GCLK_SRC_DFLL48M) {
		oscctrl_regs->OSCCTRL_DFLLCTRLA &= ~(OSCCTRL_DFLLCTRLA_ENABLE_Msk);
		if (dfll48m_init->closed_loop_en == true) {
			/* Check if the source gclkgen clock (driving DFLL48M) is initialized and
			 * on. Since the gclk generators are in order for dpll source, we can use
			 * the same here.
			 */
			gclkgen_index = dfll48m_init->src_gclk;
			if ((data->dpll_src_on_status & BIT(gclkgen_index)) == 0) {
				LOG_INF("%s: skipping dfll48m_init, as source gclk-%d is off. "
					"(Maximum init retry = %d)",
					__func__, gclkgen_index, CLOCK_INIT_ITERATION_COUNT);
				return;
			}

			/* GCLK_PCHCTRL[0] is for DFLL48M input clock source */
			val32 = gclk_regs->GCLK_PCHCTRL[0] & ~(GCLK_PCHCTRL_GEN_Msk);
			val32 |= (GCLK_PCHCTRL_GEN(gclkgen_index) | GCLK_PCHCTRL_CHEN_Msk);
			gclk_regs->GCLK_PCHCTRL[0] = val32;
			if (WAIT_FOR(((GCLK_REGS->GCLK_PCHCTRL[0] & GCLK_PCHCTRL_CHEN_Msk) != 0),
				     TIMEOUT_REG_SYNC, NULL) == false) {
				LOG_ERR("%s: DFLL48MSYNC timeout on writing GCLK_PCHCTRL",
					__func__);
				return;
			}

			/* DFLLMUL */
			val32 = oscctrl_regs->OSCCTRL_DFLLMUL &
				~(OSCCTRL_DFLLMUL_STEP_Msk | OSCCTRL_DFLLMUL_MUL_Msk);
			val32 |= OSCCTRL_DFLLMUL_STEP(dfll48m_init->tune_max_step);
			val32 |= OSCCTRL_DFLLMUL_MUL(dfll48m_init->multiply_factor);
			oscctrl_regs->OSCCTRL_DFLLMUL = val32;
			if (WAIT_FOR((oscctrl_regs->OSCCTRL_SYNCBUSY == 0), TIMEOUT_REG_SYNC,
				     NULL) == false) {
				LOG_ERR("%s: DFLL48MSYNC timeout on writing OSCCTRL_DFLLMUL",
					__func__);
				return;
			}

			/* DFLLCTRLB */
			val8 = oscctrl_regs->OSCCTRL_DFLLCTRLB &
			       ~(OSCCTRL_DFLLCTRLB_WAITLOCK_Msk | OSCCTRL_DFLLCTRLB_QLDIS_Msk |
				 OSCCTRL_DFLLCTRLB_CCDIS_Msk | OSCCTRL_DFLLCTRLB_LLAW_Msk |
				 OSCCTRL_DFLLCTRLB_STABLE_Msk);
			val8 |= OSCCTRL_DFLLCTRLB_WAITLOCK(dfll48m_init->wait_lock_en);
			val8 |= OSCCTRL_DFLLCTRLB_QLDIS(dfll48m_init->quick_lock_dis);
			val8 |= OSCCTRL_DFLLCTRLB_CCDIS(dfll48m_init->chill_cycle_dis);
			val8 |= OSCCTRL_DFLLCTRLB_LLAW(dfll48m_init->lose_lock_en);
			val8 |= OSCCTRL_DFLLCTRLB_STABLE(dfll48m_init->stable_freq_en);
			val8 |= OSCCTRL_DFLLCTRLB_LOOPEN(1);
			oscctrl_regs->OSCCTRL_DFLLCTRLB = val8;
			if (WAIT_FOR((oscctrl_regs->OSCCTRL_SYNCBUSY == 0), TIMEOUT_REG_SYNC,
				     NULL) == false) {
				LOG_ERR("%s: DFLL48MSYNC timeout on writing OSCCTRL_DFLLCTRLB",
					__func__);
				return;
			}
		}

		/* DFLLCTRLA */
		/* DFLLCTRLA.ONDEMAND must be written when DFLLCTRLA.ENABLE = 0 and DFLLSYNC.ENABLE
		 * = 0. Otherwise, the write of this bit is ignored.
		 */

		/* Important: Initializing it with 1, along with clock enabled, can lead to
		 * indefinite wait for the clock to be on, if there is no peripheral request
		 * for the clock in the sequence of clock Initialization. If required,
		 * better to turn on the clock using API, instead of enabling both
		 * (on_demand_en & enable) during startup.
		 */
		val8 = oscctrl_regs->OSCCTRL_DFLLCTRLA & ~(OSCCTRL_DFLLCTRLA_ONDEMAND_Msk);
		val8 |= OSCCTRL_DFLLCTRLA_ONDEMAND(dfll48m_init->on_demand_en);
		oscctrl_regs->OSCCTRL_DFLLCTRLA = val8;

		val8 = oscctrl_regs->OSCCTRL_DFLLCTRLA & ~(OSCCTRL_DFLLCTRLA_ENABLE_Msk);
		val8 |= OSCCTRL_DFLLCTRLA_ENABLE(dfll48m_init->enable);
		oscctrl_regs->OSCCTRL_DFLLCTRLA = val8;
		if (WAIT_FOR((oscctrl_regs->OSCCTRL_SYNCBUSY == 0), TIMEOUT_REG_SYNC, NULL) ==
		    false) {
			LOG_ERR("%s: DFLL48MSYNC timeout on writing OSCCTRL_DFLLCTRLA", __func__);
			return;
		}
		if (dfll48m_init->enable != 0) {
			if (WAIT_FOR(((oscctrl_regs->OSCCTRL_STATUS & OSCCTRL_STATUS_DFLLRDY_Msk) !=
				      0),
				     TIMEOUT_DFLL48M_RDY, NULL) == false) {
				LOG_ERR("%s: DFLL48M ready timed out", __func__);
				return;
			}
		}

	} else {
		LOG_INF("%s: skipping dfll48m_init, as DFLL is driving gclk0 (CPU)", __func__);
	}

	if (dfll48m_init->enable != 0) {
		/* Set DFLL48M clock as on */
		data->dpll_src_on_status |= BIT(CLOCK_MCHP_DPLL_SRC_DFLL48M);
		data->gclkgen_src_on_status |= BIT(CLOCK_MCHP_GCLK_SRC_DFLL48M);
	}
}

/* initialize DPLL from device tree node. */
void clock_dpll_init(const struct device *dev, struct clock_dpll_init *dpll_init)
{
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	supc_registers_t *supc_regs = config->supc_regs;

	uint32_t val32;
	int inst = dpll_init->subsys.bits.inst;
	int src;

	/* Check if the DPLL is already requested for on */
	if ((data->dpll_on_request & BIT(inst)) != 0) {
		return;
	}

	/* Check if the source clock (driving DPLL) is off. */
	src = dpll_init->src;
	if ((data->dpll_src_on_status & BIT(src)) == 0) {
		LOG_INF("%s: source clock (driving DPLL_%d) is off. (Maximum init retry = %d)",
			__func__, inst, CLOCK_INIT_ITERATION_COUNT);
		return;
	}

	/* program gclkph if source is gclk & enable. */
	if (src <= CLOCK_MCHP_DPLL_SRC_GCLK15) {
		gclk_regs->GCLK_PCHCTRL[inst + 1] |=
			(GCLK_PCHCTRL_GEN(src) | GCLK_PCHCTRL_CHEN_Msk);
		if (WAIT_FOR(((gclk_regs->GCLK_PCHCTRL[inst + 1] & GCLK_PCHCTRL_CHEN_Msk) != 0),
			     TIMEOUT_REG_SYNC, NULL) == false) {
			LOG_ERR("%s: timeout on writing GCLK_PCHCTRL_CHEN_Msk", __func__);
			return;
		}
	}

	/* DPLLFBDIV, access DPLL0 or DPLL1 registers based on inst and offset */
	/* macro value is same for both DPLL0 and DPLL1 */
	val32 = *(&oscctrl_regs->OSCCTRL_PLL0FBDIV + (inst * DPLLREG_OFFSET));
	val32 &= ~(OSCCTRL_PLL0FBDIV_FBDIV_Msk);
	val32 |= OSCCTRL_PLL0FBDIV_FBDIV(dpll_init->feedback_divider_factor);
	*(&oscctrl_regs->OSCCTRL_PLL0FBDIV + (inst * DPLLREG_OFFSET)) = val32;

	/* DPLLREFDIV, access DPLL0 or DPLL1 registers based on inst and offset */
	/* macro is same for both DPLL0 and DPLL1 */
	val32 = *(&oscctrl_regs->OSCCTRL_PLL0REFDIV + (inst * DPLLREG_OFFSET));
	val32 &= ~(OSCCTRL_PLL0REFDIV_REFDIV_Msk);
	val32 |= OSCCTRL_PLL0REFDIV_REFDIV(dpll_init->ref_division_factor);
	*(&oscctrl_regs->OSCCTRL_PLL0REFDIV + (inst * DPLLREG_OFFSET)) = val32;

	/* DPLLCTRL, access DPLL0 or DPLL1 registers based on inst and offset */
	/* macro is same for both DPLL0 and DPLL1 */
	val32 = *(&oscctrl_regs->OSCCTRL_PLL0CTRL + (inst * DPLLREG_OFFSET));
	val32 &= ~(OSCCTRL_PLL0CTRL_BWSEL_Msk | OSCCTRL_PLL0CTRL_REFSEL_Msk |
		   OSCCTRL_PLL0CTRL_ONDEMAND_Msk);
	val32 |= OSCCTRL_PLL0CTRL_BWSEL(dpll_init->bandwidth_sel);
	val32 |= OSCCTRL_PLL0CTRL_REFSEL(
		(src > CLOCK_MCHP_DPLL_SRC_GCLK15) ? (src - CLOCK_MCHP_DPLL_SRC_GCLK15) : 0);

	/* Important: Initializing it with 1, along with clock enabled, can lead to
	 * indefinite wait for the clock to be on, if there is no peripheral request
	 * for the clock in the sequence of clock Initialization. If required,
	 * better to turn on the clock using API, instead of enabling both
	 * (on_demand_en & enable) during startup.
	 */
	val32 |= OSCCTRL_PLL0CTRL_ONDEMAND(dpll_init->on_demand_en);
	*(&oscctrl_regs->OSCCTRL_PLL0CTRL + (inst * DPLLREG_OFFSET)) = val32;

	if (dpll_init->enable != 0) {
		/* Enable Additional Voltage Regulator */
		supc_regs->SUPC_VREGCTRL |= SUPC_VREGCTRL_AVREGEN_Msk;
		if (WAIT_FOR(((supc_regs->SUPC_STATUS & SUPC_STATUS_ADDVREGRDY_Msk) ==
			      SUPC_STATUS_ADDVREGRDY_Msk),
			     TIMEOUT_SUPC_REGRDY, NULL) == false) {
			LOG_ERR("%s: SUPC_STATUS timeout on writing SUPC_VREGCTRL", __func__);
			return;
		}

		/* Set DPLL clock request as on */
		data->dpll_on_request |= BIT(inst);
	}
}

/* initialize DPLL_OUT from device tree node. */
void clock_dpll_out_init(const struct device *dev, struct clock_dpll_out_init *dpll_out_init)
{
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;

	uint32_t val32, pos_postdiv;
	int inst = dpll_out_init->subsys.bits.inst;
	int pll;

	/* Check if the DPLL_OUT is already on */
	if ((data->gclkgen_src_on_status & BIT(CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT0 + inst)) != 0) {
		LOG_INF("%s: skipping dpll_%d_out_%d_init, as it is already on", __func__,
			inst / PLLOUT_COUNT, inst % PLLOUT_COUNT);
		return;
	}

	pos_postdiv = (inst % PLLOUT_COUNT) * PLLOUT_POSTDIV_SPAN;

	/* same macro for both DPLL0 and DPLL1 */
	pll = inst / PLLOUT_COUNT;
	val32 = *(&oscctrl_regs->OSCCTRL_PLL0POSTDIVA + (pll * DPLLREG_OFFSET)) &
		~(PLLPOSTDIV_Msk << pos_postdiv);
	val32 |= (dpll_out_init->output_division_factor << pos_postdiv);
	*(&(oscctrl_regs->OSCCTRL_PLL0POSTDIVA) + (pll * DPLLREG_OFFSET)) = val32;

	/* Check if the driving DPLL is not requested for on. Also if out is not enabled */
	if ((dpll_out_init->output_en == 0) || (clock_on_dpll_out(dev, inst) != 0)) {
		LOG_INF("%s: skipping dpll_%d_out_%d_init, as driving DPLL is off", __func__,
			inst / PLLOUT_COUNT, inst % PLLOUT_COUNT);
		return;
	}

	/* Check if fractional dividers to be configured. */
	if ((inst == CLOCK_MCHP_DPLL1_ID_OUT0) || (inst == CLOCK_MCHP_DPLL1_ID_OUT1)) {
		/* same macro for both DPLL0 and DPLL1 */
		val32 = *(&oscctrl_regs->OSCCTRL_FRACDIV0 + (pll * 2));
		val32 &= ~(OSCCTRL_FRACDIV0_INTDIV_Msk | OSCCTRL_FRACDIV0_REMDIV_Msk);
		val32 |= OSCCTRL_FRACDIV0_INTDIV(dpll_out_init->fract_divider_int) |
			 OSCCTRL_FRACDIV0_REMDIV(dpll_out_init->fract_divider_rem);
		*(&oscctrl_regs->OSCCTRL_FRACDIV0 + (pll * 2)) = val32;
		if (WAIT_FOR(((oscctrl_regs->OSCCTRL_SYNCBUSY &
			       BIT(OSCCTRL_SYNCBUSY_FRACDIV0_Pos + pll)) == 0),
			     TIMEOUT_REG_SYNC, NULL) == false) {
			LOG_ERR("%s: timeout on writing fractional divider", __func__);
			return;
		}
	}

	/* Set dpll_out status as on */
	data->gclkgen_src_on_status |= BIT(CLOCK_MCHP_GCLK_SRC_DPLL0_CLKOUT0 + inst);
}

/* initialize rtc clock source from device tree node. */
void clock_rtc_init(const struct device *dev, uint8_t rtc_src)
{
	const struct clock_mchp_config *config = dev->config;

	config->osc32kctrl_regs->OSC32KCTRL_CLKSELCTRL = OSC32KCTRL_CLKSELCTRL_RTCSEL(rtc_src);
}

/* initialize XOSC32K clocks from device tree node. */
void clock_xosc32k_init(const struct device *dev, struct clock_xosc32k_init *xosc32k_init)
{
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;

	uint32_t val32;

	/* Check if XOSC32K clock is already on */
	if ((data->gclkgen_src_on_status & BIT(CLOCK_MCHP_GCLK_SRC_XOSC32K)) != 0) {
		LOG_INF("%s: skipping xosc32k_init, as it is already on", __func__);
		return;
	}

	/* CFDCTRL */
	val32 = osc32kctrl_regs->OSC32KCTRL_CFDCTRL &
		~(OSC32KCTRL_CFDCTRL_CFDPRESC_Msk | OSC32KCTRL_CFDCTRL_SWBACK_Msk |
		  OSC32KCTRL_CFDCTRL_CFDEN_Msk);
	val32 |= OSC32KCTRL_CFDCTRL_CFDPRESC(xosc32k_init->cfd_prescaler);
	val32 |= OSC32KCTRL_CFDCTRL_SWBACK(xosc32k_init->cfd_switchback_en);
	val32 |= OSC32KCTRL_CFDCTRL_CFDEN(xosc32k_init->cfd_en);
	osc32kctrl_regs->OSC32KCTRL_CFDCTRL = val32;

	/* XOSC32K */
	val32 = osc32kctrl_regs->OSC32KCTRL_XOSC32K &
		~(OSC32KCTRL_XOSC32K_CGM_Msk | OSC32KCTRL_XOSC32K_STARTUP_Msk |
		  OSC32KCTRL_XOSC32K_ONDEMAND_Msk | OSC32KCTRL_XOSC32K_XTALEN_Msk |
		  OSC32KCTRL_XOSC32K_ENABLE_Msk);
	val32 |= OSC32KCTRL_XOSC32K_CGM(xosc32k_init->control_gain_mode);
	val32 |= OSC32KCTRL_XOSC32K_STARTUP(xosc32k_init->startup_time);

	/* Important: Initializing it with 1, along with clock enabled, can lead to
	 * indefinite wait for the clock to be on, if there is no peripheral request
	 * for the clock in the sequence of clock Initialization. If required,
	 * better to turn on the clock using API, instead of enabling both
	 * (on_demand_en & enable) during startup.
	 */
	val32 |= OSC32KCTRL_XOSC32K_ONDEMAND(xosc32k_init->on_demand_en);
	val32 |= OSC32KCTRL_XOSC32K_XTALEN(xosc32k_init->xtal_en);
	val32 |= OSC32KCTRL_XOSC32K_ENABLE(xosc32k_init->enable);
	osc32kctrl_regs->OSC32KCTRL_XOSC32K = val32;

	if (xosc32k_init->enable != 0) {
		if (WAIT_FOR(((osc32kctrl_regs->OSC32KCTRL_STATUS &
			       OSC32KCTRL_STATUS_XOSC32KRDY_Msk) != 0),
			     TIMEOUT_OSC32KCTRL_RDY, NULL) == false) {
			LOG_ERR("%s: OSC32KCTRL ready timed out", __func__);
			return;
		}

		/* Set XOSC32K clock as on */
		data->gclkgen_src_on_status |= BIT(CLOCK_MCHP_GCLK_SRC_XOSC32K);
	}
}

/* initialize gclk generator from device tree node. */
void clock_gclkgen_init(const struct device *dev, struct clock_gclkgen_init *gclkgen_init)
{
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;

	uint32_t val32;
	int inst = gclkgen_init->subsys.bits.inst;

	/* Check if gclkgen clock is already on */
	if ((data->dpll_src_on_status & BIT(inst)) != 0) {
		LOG_INF("%s: skipping gclkgen-%d_init, as it is already on", __func__, inst);
		return;
	}

	/* Check if source of gclk generator is off */
	if ((data->gclkgen_src_on_status & BIT(gclkgen_init->src)) == 0) {
		LOG_INF("%s: skipping gclkgen_init, as source of gclk-%d, is off. (Maximum init "
			"retry = %d)",
			__func__, inst, CLOCK_INIT_ITERATION_COUNT);
		return;
	}

	/* GENCTRL */
	val32 = config->gclk_regs->GCLK_GENCTRL[inst] &
		~(GCLK_GENCTRL_DIV_Msk | GCLK_GENCTRL_GENEN_Msk | GCLK_GENCTRL_OOV_Msk |
		  GCLK_GENCTRL_DIVSEL_Msk | GCLK_GENCTRL_OE_Msk | GCLK_GENCTRL_RUNSTDBY_Msk |
		  GCLK_GENCTRL_SRC_Msk);

	/* check range for div_factor, gclk1: 0 - 65535, others: 0 - 255 */
	if ((inst == 1) || (gclkgen_init->div_factor <= 0xFF)) {
		val32 |= GCLK_GENCTRL_DIV(gclkgen_init->div_factor);
	}

	val32 |= ((gclkgen_init->run_in_standby_en != 0) ? GCLK_GENCTRL_RUNSTDBY(1) : 0);
	if (gclkgen_init->div_select == 0) {
		/* div-factor */
		val32 |= GCLK_GENCTRL_DIVSEL(GCLK_GENCTRL_DIVSEL_DIV1_Val);
	} else {
		/* div-factor-power */
		val32 |= GCLK_GENCTRL_DIVSEL(GCLK_GENCTRL_DIVSEL_DIV2_Val);
	}
	val32 |= GCLK_GENCTRL_OE(gclkgen_init->pin_output_en);
	val32 |= GCLK_GENCTRL_OOV(gclkgen_init->pin_output_off_val);
	val32 |= GCLK_GENCTRL_IDC(gclkgen_init->duty_50_50_en);
	val32 |= GCLK_GENCTRL_GENEN(gclkgen_init->enable);
	val32 |= GCLK_GENCTRL_SRC(gclkgen_init->src);
	config->gclk_regs->GCLK_GENCTRL[inst] = val32;

	if (WAIT_FOR((config->gclk_regs->GCLK_SYNCBUSY == 0), TIMEOUT_REG_SYNC, NULL) == false) {
		LOG_ERR("%s: GCLK_SYNCBUSY timeout on writing GCLK_GENCTRL[%d]", __func__, inst);
		return;
	}

	if ((inst <= GCLK_IO_MAX) && (inst >= GCLK_IO_MIN)) {
		data->gclkpin_freq[inst - GCLK_IO_MIN] = gclkgen_init->pin_src_freq;
	}

	/* To avoid changing dfll48m, while gclk0 is driven by it. Else will affect CPU */
	if (inst == CLOCK_MCHP_GCLKGEN_GEN0) {
		data->gclk0_src = gclkgen_init->src;
	}

	/* Set gclkgen clock as on */
	data->dpll_src_on_status |= BIT(inst);
	if (inst == CLOCK_MCHP_GCLKGEN_GEN1) {
		data->gclkgen_src_on_status |= BIT(CLOCK_MCHP_GCLKGEN_GEN1);
	}
}

/* initialize peripheral gclk from device tree node. */
void clock_gclkperiph_init(const struct device *dev, uint32_t subsys_val, uint8_t pch_src,
			   uint8_t enable)
{
	const struct clock_mchp_config *config = dev->config;
	union clock_mchp_subsys subsys;
	uint32_t val32;

	subsys.val = subsys_val;

	/* PCHCTRL */
	val32 = config->gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph] &
		~(GCLK_PCHCTRL_CHEN_Msk | GCLK_PCHCTRL_GEN_Msk);
	val32 |= GCLK_PCHCTRL_CHEN(enable) | GCLK_PCHCTRL_GEN(pch_src);
	config->gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph] = val32;
}

/* initialize mclk domains from device tree node. */
void clock_mclkdomain_init(const struct device *dev, uint32_t subsys_val, uint8_t mclk_div)
{
	const struct clock_mchp_config *config = dev->config;
	union clock_mchp_subsys subsys;
	uint32_t val32, inst;

	subsys.val = subsys_val;
	inst = subsys.bits.inst;

	val32 = config->mclk_regs->MCLK_CLKDIV[inst] & ~(MCLK_CLKDIV_DIV_Msk);
	val32 |= MCLK_CLKDIV_DIV(mclk_div);
	/* config->mclk_regs->MCLK_CLKDIV[inst] = val32;
	 *todo, system halts if we program MCLK_CLKDIV register. To resolve..
	 */

	/* Wait for the Main Clock to be Ready */
	if (WAIT_FOR(((MCLK_REGS->MCLK_INTFLAG & MCLK_INTFLAG_CKRDY_Msk) == MCLK_INTFLAG_CKRDY_Msk),
		     TIMEOUT_MCLK_RDY, NULL) == false) {
		LOG_ERR("%s: MCLK_INTFLAG RDY timeout on writing MCLK_CLKDIV[%d]", __func__, inst);
		return;
	}
}

/* initialize peripheral mclk from device tree node. */
void clock_mclkperiph_init(const struct device *dev, uint32_t subsys_val, uint8_t enable)
{
	const struct clock_mchp_config *config = dev->config;
	union clock_mchp_subsys subsys;

	uint32_t mask;
	__IO uint32_t *mask_reg;

	subsys.val = subsys_val;
	mask = BIT(subsys.bits.mclkmaskbit);
	mask_reg = &(config->mclk_regs->MCLK_CLKMSK[subsys.bits.mclkmaskreg]);

	if (enable == true) {
		*mask_reg |= mask;

	} else {
		*mask_reg &= ~mask;
	}
}

#define CLOCK_MCHP_PROCESS_XOSC(node)                                                              \
	struct clock_xosc_init xosc_init = {0};                                                    \
	xosc_init.usb_ref_clock_div = DT_ENUM_IDX(node, xosc_usb_ref_clock_div);                   \
	xosc_init.startup_time = DT_ENUM_IDX(node, xosc_startup_time);                             \
	xosc_init.on_demand_en = DT_PROP(node, xosc_on_demand_en);                                 \
	xosc_init.clock_switch_back_en = DT_PROP(node, xosc_clock_switch_back_en);                 \
	xosc_init.clock_failure_detection_en = DT_PROP(node, xosc_clock_failure_detection_en);     \
	xosc_init.xtal_en = DT_PROP(node, xosc_xtal_en);                                           \
	xosc_init.auto_gain_control_loop_en = DT_PROP(node, xosc_auto_gain_control_loop_en);       \
	xosc_init.enable = DT_PROP(node, xosc_en);                                                 \
	xosc_init.frequency = DT_PROP(node, xosc_frequency);                                       \
	clock_xosc_init(dev, &xosc_init);

#define CLOCK_MCHP_PROCESS_DFLL48M(node)                                                           \
	struct clock_dfll48m_init dfll48m_init = {0};                                              \
	dfll48m_init.on_demand_en = DT_PROP(node, dfll48m_on_demand_en);                           \
	dfll48m_init.enable = DT_PROP(node, dfll48m_en);                                           \
	dfll48m_init.wait_lock_en = DT_PROP(node, dfll48m_wait_lock_en);                           \
	dfll48m_init.quick_lock_dis = DT_PROP(node, dfll48m_quick_lock_dis);                       \
	dfll48m_init.chill_cycle_dis = DT_PROP(node, dfll48m_chill_cycle_dis);                     \
	dfll48m_init.lose_lock_en = DT_PROP(node, dfll48m_lose_lock_en);                           \
	dfll48m_init.stable_freq_en = DT_PROP(node, dfll48m_stable_freq_en);                       \
	dfll48m_init.closed_loop_en = DT_PROP(node, dfll48m_closed_loop_en);                       \
	dfll48m_init.tune_max_step = DT_PROP(node, dfll48m_tune_max_step);                         \
	dfll48m_init.multiply_factor = DT_PROP(node, dfll48m_multiply_factor);                     \
	dfll48m_init.src_gclk = DT_ENUM_IDX(node, dfll48m_src_gclk);                               \
	clock_dfll48m_init(dev, &dfll48m_init);

#define CLOCK_MCHP_ITERATE_DPLL_OUT(subchild)                                                      \
	{                                                                                          \
		struct clock_dpll_out_init dpll_out_init = {0};                                    \
		dpll_out_init.subsys.val = DT_PROP(subchild, subsystem);                           \
		dpll_out_init.output_en = DT_PROP(subchild, dpll_output_en);                       \
		dpll_out_init.output_division_factor =                                             \
			DT_PROP(subchild, dpll_output_division_factor);                            \
		dpll_out_init.fract_divider_int = DT_PROP(subchild, dpll_fractional_divider_int);  \
		dpll_out_init.fract_divider_rem = DT_PROP(subchild, dpll_fractional_divider_rem);  \
		clock_dpll_out_init(dev, &dpll_out_init);                                          \
	}

#define CLOCK_MCHP_ITERATE_DPLL(child)                                                             \
	{                                                                                          \
		struct clock_dpll_init dpll_init = {0};                                            \
		dpll_init.subsys.val = DT_PROP(child, subsystem);                                  \
		dpll_init.feedback_divider_factor = DT_PROP(child, dpll_feedback_divider_factor);  \
		dpll_init.ref_division_factor = DT_PROP(child, dpll_ref_division_factor);          \
		dpll_init.bandwidth_sel = DT_ENUM_IDX(child, dpll_bandwidth_sel);                  \
		dpll_init.src = DT_ENUM_IDX(child, dpll_src);                                      \
		dpll_init.on_demand_en = DT_PROP(child, dpll_on_demand_en);                        \
		dpll_init.enable = DT_PROP(child, dpll_en);                                        \
		clock_dpll_init(dev, &dpll_init);                                                  \
	}

#define CLOCK_MCHP_PROCESS_RTC(node) clock_rtc_init(dev, DT_PROP(node, rtc_src));

#define CLOCK_MCHP_PROCESS_XOSC32K(node)                                                           \
	struct clock_xosc32k_init xosc32k_init = {0};                                              \
	xosc32k_init.control_gain_mode = DT_ENUM_IDX(node, xosc32k_control_gain_mode);             \
	xosc32k_init.startup_time = DT_ENUM_IDX(node, xosc32k_startup_time);                       \
	xosc32k_init.on_demand_en = DT_PROP(node, xosc32k_on_demand_en);                           \
	xosc32k_init.xtal_en = DT_PROP(node, xosc32k_xtal_en);                                     \
	xosc32k_init.enable = DT_PROP(node, xosc32k_en);                                           \
	xosc32k_init.cfd_prescaler = DT_PROP(node, xosc32k_cfd_prescaler);                         \
	xosc32k_init.cfd_switchback_en = DT_PROP(node, xosc32k_cfd_switchback_en);                 \
	xosc32k_init.cfd_en = DT_PROP(node, xosc32k_cfd_en);                                       \
	clock_xosc32k_init(dev, &xosc32k_init);

#define CLOCK_MCHP_ITERATE_GCLKGEN(child)                                                          \
	{                                                                                          \
		struct clock_gclkgen_init gclkgen_init = {0};                                      \
		gclkgen_init.subsys.val = DT_PROP(child, subsystem);                               \
		gclkgen_init.div_factor = DT_PROP(child, gclkgen_div_factor);                      \
		gclkgen_init.run_in_standby_en = DT_PROP(child, gclkgen_run_in_standby_en);        \
		gclkgen_init.div_select = DT_ENUM_IDX(child, gclkgen_div_select);                  \
		gclkgen_init.pin_output_en = DT_PROP(child, gclkgen_pin_output_en);                \
		gclkgen_init.pin_output_off_val = DT_ENUM_IDX(child, gclkgen_pin_output_off_val);  \
		gclkgen_init.duty_50_50_en = DT_PROP(child, gclkgen_duty_50_50_en);                \
		gclkgen_init.enable = DT_PROP(child, gclkgen_en);                                  \
		gclkgen_init.src = DT_ENUM_IDX(child, gclkgen_src);                                \
		gclkgen_init.pin_src_freq = DT_PROP(child, gclkgen_pin_src_freq);                  \
		clock_gclkgen_init(dev, &gclkgen_init);                                            \
	}

#define CLOCK_MCHP_ITERATE_GCLKPERIPH(child)                                                       \
	{                                                                                          \
		clock_gclkperiph_init(dev, DT_PROP(child, subsystem),                              \
				      DT_ENUM_IDX(child, gclkperiph_src),                          \
				      DT_PROP(child, gclkperiph_en));                              \
	}

#define CLOCK_MCHP_ITERATE_MCLKDOMAIN(child)                                                       \
	{                                                                                          \
		clock_mclkdomain_init(dev, DT_PROP(child, subsystem), DT_PROP(child, mclk_div));   \
	}

#define CLOCK_MCHP_ITERATE_MCLKPERIPH(child)                                                       \
	{                                                                                          \
		clock_mclkperiph_init(dev, DT_PROP(child, subsystem), DT_PROP(child, mclk_en));    \
	}

#endif /* CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP */

/* clock driver initialization function. */
static int clock_mchp_init(const struct device *dev)
{
#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;

	DT_FOREACH_CHILD(DT_NODELABEL(mclkdomain), CLOCK_MCHP_ITERATE_MCLKDOMAIN);

	/* iteration-1 */
	CLOCK_MCHP_PROCESS_XOSC(DT_NODELABEL(xosc));
	CLOCK_MCHP_PROCESS_XOSC32K(DT_NODELABEL(xosc32k));

	config->gclk_regs->GCLK_CTRLA = GCLK_CTRLA_SWRST(1);
	if (WAIT_FOR((config->gclk_regs->GCLK_SYNCBUSY == 0), TIMEOUT_REG_SYNC, NULL) == false) {
		LOG_ERR("%s: GCLK_SYNCBUSY timeout on writing GCLK_CTRLA", __func__);
		return -ETIMEDOUT;
	}

	/* To avoid changing dfll48m, while gclk0 is driven by it. Else will affect CPU */
	data->gclk0_src = CLOCK_MCHP_GCLK_SRC_DFLL48M;
	for (int i = 0; i < CLOCK_INIT_ITERATION_COUNT; i++) {
		DT_FOREACH_CHILD(DT_NODELABEL(gclkgen), CLOCK_MCHP_ITERATE_GCLKGEN);
		CLOCK_MCHP_PROCESS_DFLL48M(DT_NODELABEL(dfll48m));
		DT_FOREACH_CHILD(DT_NODELABEL(dpll), CLOCK_MCHP_ITERATE_DPLL);
		DT_FOREACH_CHILD(DT_NODELABEL(dpll0), CLOCK_MCHP_ITERATE_DPLL_OUT);
		DT_FOREACH_CHILD(DT_NODELABEL(dpll1), CLOCK_MCHP_ITERATE_DPLL_OUT);
	}

	CLOCK_MCHP_PROCESS_RTC(DT_NODELABEL(rtcclock));
	DT_FOREACH_CHILD(DT_NODELABEL(gclkperiph), CLOCK_MCHP_ITERATE_GCLKPERIPH);
	DT_FOREACH_CHILD(DT_NODELABEL(mclkperiph), CLOCK_MCHP_ITERATE_MCLKPERIPH);

#endif /* CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP */

	return 0;
}

static DEVICE_API(clock_control, clock_mchp_driver_api) = {
	.on = clock_mchp_on,
	.off = clock_mchp_off,
	.get_status = clock_mchp_get_status,

#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE
	.get_rate = clock_mchp_get_rate,
#endif /* CONFIG_CLOCK_CONTROL_MCHP_GET_RATE */

#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_RUNTIME
	.configure = clock_mchp_configure,
#endif /* CONFIG_CLOCK_CONTROL_MCHP_CONFIG_RUNTIME */
};

#define CLOCK_MCHP_CONFIG_DEFN()                                                                   \
	static const struct clock_mchp_config clock_config = {                                     \
		.on_timeout_ms = DT_PROP_OR(CLOCK_NODE, on_timeout_ms, 5),                         \
		.mclk_regs = (mclk_registers_t *)DT_REG_ADDR_BY_NAME(CLOCK_NODE, mclk),            \
		.oscctrl_regs = (oscctrl_registers_t *)DT_REG_ADDR_BY_NAME(CLOCK_NODE, oscctrl),   \
		.osc32kctrl_regs =                                                                 \
			(osc32kctrl_registers_t *)DT_REG_ADDR_BY_NAME(CLOCK_NODE, osc32kctrl),     \
		.gclk_regs = (gclk_registers_t *)DT_REG_ADDR_BY_NAME(CLOCK_NODE, gclk),            \
		.supc_regs = (supc_registers_t *)DT_REG_ADDR_BY_NAME(CLOCK_NODE, supc)}

#define CLOCK_MCHP_DATA_DEFN() static struct clock_mchp_data clock_data;

#define CLOCK_MCHP_DEVICE_INIT(n)                                                                  \
	CLOCK_MCHP_CONFIG_DEFN();                                                                  \
	CLOCK_MCHP_DATA_DEFN();                                                                    \
	DEVICE_DT_INST_DEFINE(n, clock_mchp_init, NULL, &clock_data, &clock_config, PRE_KERNEL_1,  \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_mchp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CLOCK_MCHP_DEVICE_INIT)
