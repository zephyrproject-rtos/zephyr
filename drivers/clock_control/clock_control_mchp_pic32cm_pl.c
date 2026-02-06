/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>

#define DT_DRV_COMPAT microchip_pic32cm_pl_clock

LOG_MODULE_REGISTER(clock_mchp_pic32cm_pl, CONFIG_CLOCK_CONTROL_LOG_LEVEL);
#define CLOCK_NODE DT_NODELABEL(clock)

/* timeout values in microseconds */
#define TIMEOUT_XOSC32KCTRL_RDY  1000000
#define TIMEOUT_GCLKGEN_BUSY_RDY 1000000
#define TIMEOUT_OSC32KCTRL_RDY   1000000
#define TIMEOUT_REG_SYNC         1000
#define TIMEOUT_VALUE_US(n)      (n * 1000)
#define BIT_MASK_MAX             31

/* maximum value for div, when div_select is clock source frequency divided by 2^(N+1) */
#define GCLKGEN_POWER_DIV_MAX 29

/* mclkbus Not Applicable for a clock subsystem ID */
#define MCLK_BUS_NA (0x3f)

/* mclkmaskbit Not Applicable for a clock subsystem ID */
#define MCLK_MASK_NA (0x3f)

/* gclkperiph Not Applicable for a clock subsystem ID */
#define GCLK_PH_NA (0x3f)

/* Clock subsystem definition.
 *
 * Structure which can be used as a sys argument in the clock_control API.
 * Encode clock type, mclk bus, mclk mask bit, gclk pch and instance number,
 * to clock subsystem.
 *
 * - 00..07 (8 bits): inst
 * - 08..13 (6 bits): gclkperiph
 * - 14..19 (6 bits): mclkmaskbit
 * - 20..25 (6 bits): mclkbus
 * - 26..31 (6 bits): type
 */
union clock_mchp_subsys {
	uint32_t val;
	struct {
		uint32_t inst: 8;
		uint32_t gclkperiph: 6;
		uint32_t mclkmaskbit: 6;
		uint32_t mclkbus: 6;
		uint32_t type: 6;
	} bits;
};

/* clock driver configuration structure. */
struct clock_mchp_config {
	oscctrl_registers_t *oscctrl_regs;
	osc32kctrl_registers_t *osc32kctrl_regs;
	gclk_registers_t *gclk_regs;
	mclk_registers_t *mclk_regs;

	/* Timeout in milliseconds to wait for clock to turn on */
	uint32_t on_timeout_ms;
};

/* clock driver data structure. */
struct clock_mchp_data {
	uint32_t oschf_freq;
#ifdef CONFIG_CLOCK_CONTROL_MCHP_GCLK_PINOUT
	uint32_t gclkpin_freq[GCLK_IO_MAX + 1];
#endif /*CONFIG_CLOCK_CONTROL_MCHP_GCLK_PINOUT*/

	/*
	 * Use bit position as per enum enum clock_mchp_gclk_src_clock, to show if the specified
	 * clock source to gclk generator is on.
	 */
	uint16_t gclkgen_src_on_status;
	enum clock_mchp_gclk_src_clock gclk0_src;
};

static enum clock_control_status clock_mchp_get_status(const struct device *dev,
						       clock_control_subsys_t sys);

/* Internal helper functions */

/* check if subsystem type and id are valid. */
static int clock_check_subsys(union clock_mchp_subsys subsys)
{
	uint32_t inst_max = 0;
	uint32_t gclkperiph_max = GCLK_PH_NA;
	uint32_t mclkbus_max = MCLK_BUS_NA;
	uint32_t mclkmaskbit_max = MCLK_MASK_NA;

	/* Check if turning on all clocks is requested. */
	if (subsys.val == (uint32_t)CLOCK_CONTROL_SUBSYS_ALL) {
		return -EINVAL;
	}

	/* Check if the specified subsystem was found. */
	if (subsys.bits.type > SUBSYS_TYPE_MAX) {
		return -EINVAL;
	}

	switch (subsys.bits.type) {
	case SUBSYS_TYPE_OSCHF:
		inst_max = CLOCK_MCHP_OSCHF_ID_MAX;
		break;

	case SUBSYS_TYPE_RTC:
		inst_max = CLOCK_MCHP_RTC_ID_MAX;
		break;

	case SUBSYS_TYPE_XOSC32K:
		inst_max = CLOCK_MCHP_XOSC32K_ID_MAX;
		break;

	case SUBSYS_TYPE_OSC32K:
		inst_max = CLOCK_MCHP_OSC32K_ID_MAX;
		break;

	case SUBSYS_TYPE_GCLKGEN:
		inst_max = CLOCK_MCHP_GCLKGEN_ID_MAX;
		break;

	case SUBSYS_TYPE_GCLKPERIPH:
		inst_max = CLOCK_MCHP_GCLKPERIPH_ID_MAX;
		gclkperiph_max = GCLK_PH_MAX;
		break;

	case SUBSYS_TYPE_MCLKCPU:
		inst_max = CLOCK_MCHP_MCLKCPU_MAX;
		break;

	case SUBSYS_TYPE_MCLKPERIPH:
		inst_max = CLOCK_MCHP_MCLKPERIPH_ID_MAX;
		mclkbus_max = MBUS_MAX;
		mclkmaskbit_max = BIT_MASK_MAX;
		break;

	default:
		LOG_ERR("Unsupported SUBSYS_TYPE");
		return -EINVAL;
	}

	/* Check if the specified id is valid. */
	if ((subsys.bits.inst > inst_max) || (subsys.bits.gclkperiph > gclkperiph_max) ||
	    (subsys.bits.mclkbus > mclkbus_max) || (subsys.bits.mclkmaskbit > mclkmaskbit_max)) {
		LOG_ERR("Invalid SUBSYS ID");
		return -EINVAL;
	}

	return 0;
}

/* get the address of mclk mask register. */
static __IO uint32_t *get_mclkbus_mask_reg(mclk_registers_t *mclk_regs, uint32_t bus)
{
	__IO uint32_t *reg32 = NULL;

	switch (bus) {
	case MBUS_AHB:
		reg32 = &mclk_regs->MCLK_AHBMASK;
		break;
	case MBUS_APBA:
		reg32 = &mclk_regs->MCLK_APBAMASK;
		break;
	case MBUS_APBB:
		reg32 = &mclk_regs->MCLK_APBBMASK;
		break;
	case MBUS_APBC:
		reg32 = &mclk_regs->MCLK_APBCMASK;
		break;
	default:
		LOG_ERR("Unsupported mclkbus");
		break;
	}

	return reg32;
}

/* get status of respective clocksubsystem. */
static enum clock_control_status clock_get_status(const struct device *dev,
						  clock_control_subsys_t sys)
{
	enum clock_control_status ret_status = CLOCK_CONTROL_STATUS_UNKNOWN;
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	union clock_mchp_subsys subsys = {.val = (uint32_t)sys};
	uint8_t inst;
	uint32_t mask;
	__IO uint32_t *reg32 = NULL;

	inst = subsys.bits.inst;
	switch (subsys.bits.type) {
	case SUBSYS_TYPE_OSCHF:
		/* It will be turned on by default. Check whether it is ready to be used as a
		 * clock source
		 */
		ret_status = ((oscctrl_regs->OSCCTRL_STATUS & OSCCTRL_STATUS_OSCHFRDY_Msk) == 0)
				     ? CLOCK_CONTROL_STATUS_STARTING
				     : CLOCK_CONTROL_STATUS_ON;
		break;

	case SUBSYS_TYPE_RTC:
		ret_status = CLOCK_CONTROL_STATUS_ON;
		break;

	case SUBSYS_TYPE_XOSC32K:
		ret_status = ((osc32kctrl_regs->OSC32KCTRL_STATUS &
			       OSC32KCTRL_STATUS_XOSC32KRDY_Msk) != 0)
				     ? CLOCK_CONTROL_STATUS_ON
				     : CLOCK_CONTROL_STATUS_OFF;
		break;

	case SUBSYS_TYPE_OSC32K:
		ret_status = ((osc32kctrl_regs->OSC32KCTRL_STATUS &
			       OSC32KCTRL_STATUS_OSC32KRDY_Msk) != 0)
				     ? CLOCK_CONTROL_STATUS_ON
				     : CLOCK_CONTROL_STATUS_OFF;
		break;

	case SUBSYS_TYPE_GCLKGEN:
		ret_status = CLOCK_CONTROL_STATUS_OFF;
		if ((gclk_regs->GCLK_GENCTRL[inst] & GCLK_GENCTRL_GENEN_Msk) != 0) {
			/* Generator is on,check if it's starting or fully on */
			ret_status = ((gclk_regs->GCLK_SYNCBUSY &
				       (BIT(GCLK_SYNCBUSY_GENCTRL_Pos + inst))) != 0)
					     ? CLOCK_CONTROL_STATUS_STARTING
					     : CLOCK_CONTROL_STATUS_ON;
		}
		break;

	case SUBSYS_TYPE_GCLKPERIPH:
		/* Check if the peripheral clock is enabled */
		ret_status = ((gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph] &
			       GCLK_PCHCTRL_CHEN_Msk) != 0)
				     ? CLOCK_CONTROL_STATUS_ON
				     : CLOCK_CONTROL_STATUS_OFF;
		break;

	case SUBSYS_TYPE_MCLKCPU:
		ret_status = CLOCK_CONTROL_STATUS_ON;
		break;

	case SUBSYS_TYPE_MCLKPERIPH:
		reg32 = get_mclkbus_mask_reg(config->mclk_regs, subsys.bits.mclkbus);
		if (reg32 != NULL) {
			mask = BIT(subsys.bits.mclkmaskbit);
			ret_status = ((*reg32 & mask) != 0) ? CLOCK_CONTROL_STATUS_ON
							    : CLOCK_CONTROL_STATUS_OFF;
		}
		break;

	default:
		break;
	}

	return ret_status;
}

/* function to set/clear clock subsystem enable bit. */
static int clock_on(const struct clock_mchp_config *config, const union clock_mchp_subsys subsys)
{
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	uint8_t inst = subsys.bits.inst;
	__IO uint32_t *reg32 = NULL;

	switch (subsys.bits.type) {
	case SUBSYS_TYPE_OSCHF:
	case SUBSYS_TYPE_OSC32K:
		/* Subsytem is on by default*/
		break;

	case SUBSYS_TYPE_XOSC32K:
		osc32kctrl_regs->OSC32KCTRL_XOSC32KCTRL |= OSC32KCTRL_XOSC32KCTRL_ENABLE_Msk;
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
		reg32 = get_mclkbus_mask_reg(config->mclk_regs, subsys.bits.mclkbus);
		if (reg32 != NULL) {
			*reg32 |= BIT(subsys.bits.mclkmaskbit);
		}
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

/* function to set/clear clock subsystem enable bit. */
static int clock_off(const struct clock_mchp_config *config, const union clock_mchp_subsys subsys)
{
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	uint8_t inst = subsys.bits.inst;
	__IO uint32_t *reg32 = NULL;
	int ret_status = 0;

	switch (subsys.bits.type) {
	case SUBSYS_TYPE_OSCHF:
	case SUBSYS_TYPE_OSC32K:
		/* Subsytem cannot be turned off */
		ret_status = -ENOTSUP;
		break;

	case SUBSYS_TYPE_XOSC32K:
		osc32kctrl_regs->OSC32KCTRL_XOSC32KCTRL &= ~OSC32KCTRL_XOSC32KCTRL_ENABLE_Msk;
		if (WAIT_FOR(((osc32kctrl_regs->OSC32KCTRL_STATUS &
			       OSC32KCTRL_STATUS_XOSC32KRDY_Msk) == 0),
			     TIMEOUT_XOSC32KCTRL_RDY,
			     (k_is_pre_kernel() ? k_busy_wait(1) : k_sleep(K_USEC(100)))) ==
		    false) {
			LOG_ERR("XOSC32KCTRL ready timed out");
			ret_status = -ETIMEDOUT;
		}
		break;

	case SUBSYS_TYPE_GCLKGEN:
		/* Check if the clock is GCLKGEN0, which is always on */
		if (inst == CLOCK_MCHP_GCLKGEN_GEN0) {
			/* GCLK GEN0 is always on */
			ret_status = -ENOTSUP;
			break;
		}

		/* Disable the clock generator by clearing the GENEN bit */
		gclk_regs->GCLK_GENCTRL[inst] &= ~(GCLK_GENCTRL_GENEN_Msk);
		if (WAIT_FOR(((gclk_regs->GCLK_SYNCBUSY &
			       (BIT(GCLK_SYNCBUSY_GENCTRL_Pos + inst))) == 0),
			     TIMEOUT_GCLKGEN_BUSY_RDY,
			     (k_is_pre_kernel() ? k_busy_wait(1) : k_sleep(K_USEC(100)))) ==
		    false) {
			LOG_ERR("GCLKGEN ready timed out");
			ret_status = -ETIMEDOUT;
		}
		break;

	case SUBSYS_TYPE_GCLKPERIPH:
		gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph] &= ~(GCLK_PCHCTRL_CHEN_Msk);
		break;

	case SUBSYS_TYPE_MCLKPERIPH:
		reg32 = get_mclkbus_mask_reg(config->mclk_regs, subsys.bits.mclkbus);
		if (reg32 != NULL) {
			*reg32 &= ~BIT(subsys.bits.mclkmaskbit);
		}
		break;

	default:
		ret_status = -ENOTSUP;
	}

	return ret_status;
}

#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE
/* get rate of gclk generator in Hz. */
static int clock_get_rate_gclkgen(const struct device *dev, enum clock_mchp_gclkgen gclkgen_id,
				  enum clock_mchp_gclk_src_clock gclkgen_called_src, uint32_t *freq)
{
	int ret_val = 0;
	const struct clock_mchp_config *config = dev->config;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;
	struct clock_mchp_data *data = dev->data;
	enum clock_mchp_gclk_src_clock gclkgen_src;
	uint32_t gclkgen_src_freq = 0;
	uint16_t gclkgen_div;
	bool power_div = (((gclk_regs->GCLK_GENCTRL[gclkgen_id] & GCLK_GENCTRL_DIVSEL_Msk) >>
			   GCLK_GENCTRL_DIVSEL_Pos) == GCLK_GENCTRL_DIVSEL_DIV1_Val)
				 ? false
				 : true;

	/* Return rate as 0, if clock is not on */
	if (clock_mchp_get_status(dev, (clock_control_subsys_t)MCHP_CLOCK_DERIVE_ID(
					       SUBSYS_TYPE_GCLKGEN, MCLK_BUS_NA, MCLK_MASK_NA,
					       GCLK_PH_NA, gclkgen_id)) !=
	    CLOCK_CONTROL_STATUS_ON) {
		*freq = 0;
		return 0;
	}

	/* get source for gclk generator from gclkgen registers */
	gclkgen_src = (gclk_regs->GCLK_GENCTRL[gclkgen_id] & GCLK_GENCTRL_SRC_Msk) >>
		      GCLK_GENCTRL_SRC_Pos;
	if (gclkgen_called_src == gclkgen_src) {
		return -ENOTSUP;
	}

	switch (gclkgen_src) {
	case CLOCK_MCHP_GCLK_SRC_OSCHF:
		gclkgen_src_freq = data->oschf_freq;
		break;

	case CLOCK_MCHP_GCLK_SRC_XOSC32K:
		gclkgen_src_freq = ((osc32kctrl_regs->OSC32KCTRL_XOSC32KCTRL &
				     OSC32KCTRL_XOSC32KCTRL_ENABLE_Msk) != 0)
					   ? CLOCK_FREQ_32KHZ
					   : 0;
		break;

	case CLOCK_MCHP_GCLK_SRC_OSC32K:
		/* OSC32K is turned on by default*/
		gclkgen_src_freq = CLOCK_FREQ_32KHZ;
		break;

	case CLOCK_MCHP_GCLK_SRC_GCLKPIN:
#ifdef CONFIG_CLOCK_CONTROL_MCHP_GCLK_PINOUT

		if (gclkgen_id <= GCLK_IO_MAX) {
			gclkgen_src_freq = data->gclkpin_freq[gclkgen_id];
		} else {
			ret_val = -ENOTSUP;
		}
#else
		ret_val = -ENOTSUP;
#endif /*CONFIG_CLOCK_CONTROL_MCHP_GCLK_PINOUT*/
		break;

	case CLOCK_MCHP_GCLK_SRC_GCLKGEN1:
		ret_val = (gclkgen_id == CLOCK_MCHP_GCLKGEN_GEN1)
				  ? -ELOOP
				  : clock_get_rate_gclkgen(dev, CLOCK_MCHP_GCLKGEN_GEN1,
							   CLOCK_MCHP_GCLK_SRC_COUNT,
							   &gclkgen_src_freq);
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
	 * For gclk1, 16 division factor bits - DIV[15:0]
	 * others, 8 division factor bits - DIV[7:0]
	 */
	if (gclkgen_id != CLOCK_MCHP_GCLKGEN_GEN1) {
		gclkgen_div = gclkgen_div & 0xFF;
	}

	if (power_div == true) {
		if (gclkgen_div > GCLKGEN_POWER_DIV_MAX) {
			gclkgen_div = GCLKGEN_POWER_DIV_MAX;
		}
		gclkgen_div = 1 << (gclkgen_div + 1);
	}

	/* if DIV value is 0, has same effect as DIV value 1 */
	if (gclkgen_div == 0) {
		gclkgen_div = 1;
	}
	*freq = gclkgen_src_freq / gclkgen_div;

	return ret_val;
}

/* get rate of RTC in Hz. */
static int clock_get_rate_rtc(const struct device *dev, uint32_t *freq)
{
	const struct clock_mchp_config *config = dev->config;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;

	uint8_t rtc_src;

	/* get rtc source clock*/
	rtc_src = (config->osc32kctrl_regs->OSC32KCTRL_RTCCTRL & OSC32KCTRL_RTCCTRL_RTCSEL_Msk) >>
		  OSC32KCTRL_RTCCTRL_RTCSEL_Pos;

	switch (rtc_src) {

	case OSC32KCTRL_RTCCTRL_RTCSEL_OSC1K_Val:
		*freq = CLOCK_FREQ_1KHZ;
		break;

	case OSC32KCTRL_RTCCTRL_RTCSEL_OSC32K_Val:
		*freq = CLOCK_FREQ_32KHZ;
		break;

	case OSC32KCTRL_RTCCTRL_RTCSEL_XOSC1K_Val:
		*freq = ((osc32kctrl_regs->OSC32KCTRL_XOSC32KCTRL &
			  OSC32KCTRL_XOSC32KCTRL_ENABLE_Msk) != 0)
				? CLOCK_FREQ_1KHZ
				: 0;
		break;

	case OSC32KCTRL_RTCCTRL_RTCSEL_XOSC32K_Val:
		*freq = ((osc32kctrl_regs->OSC32KCTRL_XOSC32KCTRL &
			  OSC32KCTRL_XOSC32KCTRL_ENABLE_Msk) != 0)
				? CLOCK_FREQ_32KHZ
				: 0;
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_CLOCK_CONTROL_MCHP_GET_RATE*/

static int wait_for_clock_on(const struct device *dev, clock_control_subsys_t sys)
{
	int ret_val;
	const struct clock_mchp_config *config = dev->config;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;
	union clock_mchp_subsys subsys = {.val = (uint32_t)sys};

	ret_val = 0;
	/* Wait until the clock state becomes ON. */
	switch (subsys.bits.type) {
	case SUBSYS_TYPE_XOSC32K:
		if (WAIT_FOR((((osc32kctrl_regs->OSC32KCTRL_STATUS &
				OSC32KCTRL_STATUS_XOSC32KRDY_Msk) != 0)),
			     TIMEOUT_VALUE_US(config->on_timeout_ms),
			     (k_is_pre_kernel() ? k_busy_wait(1) : k_sleep(K_USEC(100)))) ==
		    false) {
			LOG_ERR("clk on timeout :SUBSYS_TYPE_XOSC32K");
			ret_val = -ETIMEDOUT;
		}
		break;
	case SUBSYS_TYPE_OSC32K:
		if (WAIT_FOR((((osc32kctrl_regs->OSC32KCTRL_STATUS &
				OSC32KCTRL_STATUS_OSC32KRDY_Msk) != 0)),
			     TIMEOUT_VALUE_US(config->on_timeout_ms),
			     (k_is_pre_kernel() ? k_busy_wait(1) : k_sleep(K_USEC(100)))) ==
		    false) {
			LOG_ERR("clk on timeout : SUBSYS_TYPE_OSC32K");
			ret_val = -ETIMEDOUT;
		}
		break;
	default:
		if (WAIT_FOR((CLOCK_CONTROL_STATUS_ON == clock_mchp_get_status(dev, sys)),
			     TIMEOUT_VALUE_US(config->on_timeout_ms),
			     (k_is_pre_kernel() ? k_busy_wait(1) : k_sleep(K_USEC(100)))) ==
		    false) {
			LOG_ERR("clk on timeout  : %x", subsys.val);
			ret_val = -ETIMEDOUT;
		}
		break;
	}

	return ret_val;
}
/* API functions */

static int clock_mchp_on(const struct device *dev, clock_control_subsys_t sys)
{
	int ret_val = -ENOTSUP;
	const struct clock_mchp_config *config = dev->config;
	union clock_mchp_subsys subsys = {.val = (uint32_t)sys};
	enum clock_control_status status;

	/* Validate subsystem. */
	ret_val = clock_check_subsys(subsys);
	if (ret_val != 0) {
		return ret_val;
	}

	status = clock_mchp_get_status(dev, sys);
	if (status == CLOCK_CONTROL_STATUS_ON) {
		/* clock is already on.*/
		ret_val = -EALREADY;
	} else {
		if (clock_on(config, subsys) == 0) {
			/* clock on operation is successful. */
			ret_val = wait_for_clock_on(dev, sys);
		}
	}

	return ret_val;
}

static int clock_mchp_off(const struct device *dev, clock_control_subsys_t sys)
{
	const struct clock_mchp_config *config = dev->config;
	union clock_mchp_subsys subsys = {.val = (uint32_t)sys};

	/* Validate subsystem. */
	if (0 == clock_check_subsys(subsys)) {
		return clock_off(config, subsys);
	} else {
		return -ENOTSUP;
	}
}

static enum clock_control_status clock_mchp_get_status(const struct device *dev,
						       clock_control_subsys_t sys)
{
	union clock_mchp_subsys subsys = {.val = (uint32_t)sys};

	/* Validate subsystem. */
	if (0 == clock_check_subsys(subsys)) {
		return clock_get_status(dev, sys);
	} else {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}
}

#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE
static int clock_mchp_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *freq)
{
	int ret_val = 0;
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;
	union clock_mchp_subsys subsys = {.val = (uint32_t)sys};
	uint8_t inst = subsys.bits.inst;

	uint8_t cpu_div;
	uint32_t gclkgen_src_freq = 0;
	enum clock_mchp_gclkgen gclkperiph_src;

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
	case SUBSYS_TYPE_OSCHF:
		*freq = data->oschf_freq;
		break;

	case SUBSYS_TYPE_RTC:
		ret_val = clock_get_rate_rtc(dev, freq);
		break;

	case SUBSYS_TYPE_XOSC32K:
	case SUBSYS_TYPE_OSC32K:
		ret_val = -ENOTSUP;
		break;

	case SUBSYS_TYPE_GCLKGEN:
		ret_val = clock_get_rate_gclkgen(dev, inst, CLOCK_MCHP_GCLK_SRC_COUNT, freq);
		break;

	case SUBSYS_TYPE_GCLKPERIPH:
		gclkperiph_src = (config->gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph] &
				  GCLK_PCHCTRL_GEN_Msk) >>
				 GCLK_PCHCTRL_GEN_Pos;
		ret_val = clock_get_rate_gclkgen(dev, gclkperiph_src, CLOCK_MCHP_GCLK_SRC_COUNT,
						 freq);
		break;

	case SUBSYS_TYPE_MCLKCPU:
	case SUBSYS_TYPE_MCLKPERIPH:
		/* source for mclk is always gclk0 */
		ret_val = clock_get_rate_gclkgen(dev, 0, CLOCK_MCHP_GCLK_SRC_COUNT,
						 &gclkgen_src_freq);
		if (ret_val == 0) {
			cpu_div = (config->mclk_regs->MCLK_CPUDIV & MCLK_CPUDIV_CPUDIV_Msk) >>
				  MCLK_CPUDIV_CPUDIV_Pos;
			if (cpu_div != 0) {
				*freq = gclkgen_src_freq / cpu_div;
			}
		}
		break;

	default:
		ret_val = -ENOTSUP;
		break;
	}

	return ret_val;
}

#endif /*CONFIG_CLOCK_CONTROL_MCHP_GET_RATE */

#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP

/* Internal initialization functions */
static uint32_t find_oschf_frequency_bitpos(uint32_t freq)
{
	uint32_t freq_bitpos;

	switch (freq) {
	case CLOCK_FREQ_1MHZ:
		freq_bitpos = OSCCTRL_OSCHFCTRL_FRQSEL_1M_Val;
		break;
	case CLOCK_FREQ_2MHZ:
		freq_bitpos = OSCCTRL_OSCHFCTRL_FRQSEL_2M_Val;
		break;
	case CLOCK_FREQ_3MHZ:
		freq_bitpos = OSCCTRL_OSCHFCTRL_FRQSEL_3M_Val;
		break;
	case CLOCK_FREQ_4MHZ:
		freq_bitpos = OSCCTRL_OSCHFCTRL_FRQSEL_4M_Val;
		break;
	case CLOCK_FREQ_8MHZ:
		freq_bitpos = OSCCTRL_OSCHFCTRL_FRQSEL_8M_Val;
		break;
	case CLOCK_FREQ_12MHZ:
		freq_bitpos = OSCCTRL_OSCHFCTRL_FRQSEL_12M_Val;
		break;
	case CLOCK_FREQ_20MHZ:
		freq_bitpos = OSCCTRL_OSCHFCTRL_FRQSEL_20M_Val;
		break;
	case CLOCK_FREQ_24MHZ:
		freq_bitpos = OSCCTRL_OSCHFCTRL_FRQSEL_24M_Val;
		break;
	default:
		freq_bitpos = OSCCTRL_OSCHFCTRL_FRQSEL_4M_Val;
		LOG_ERR("unsupported frequency %d, setting default to 4MHz", __LINE__);
		break;
	}

	return freq_bitpos;
}

/* initialize rtc clock source from device tree node. */
static void clock_rtc_init(const struct device *dev, uint8_t rtc_src)
{
	const struct clock_mchp_config *config = dev->config;

	config->osc32kctrl_regs->OSC32KCTRL_RTCCTRL = OSC32KCTRL_RTCCTRL_RTCSEL(rtc_src);
}

/* initialize osc32k clocks from device tree node. */
static void clock_osc32k_init(const struct device *dev, bool on_demand)
{
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;

	/* Check if osc32k clock is already on */
	if ((data->gclkgen_src_on_status & BIT(CLOCK_MCHP_GCLK_SRC_OSC32K)) == 0) {
		/* OSC32K */
		osc32kctrl_regs->OSC32KCTRL_OSC32KCTRL = OSC32KCTRL_OSC32KCTRL_ONDEMAND(on_demand);

		if (WAIT_FOR(((osc32kctrl_regs->OSC32KCTRL_STATUS &
			       OSC32KCTRL_STATUS_OSC32KRDY_Msk) != 0),
			     TIMEOUT_OSC32KCTRL_RDY, NULL) == false) {
			LOG_ERR("OSC32KCTRL ready timed out");
			return;
		}
		data->gclkgen_src_on_status |= BIT(CLOCK_MCHP_GCLK_SRC_OSC32K);
	}
}

/* initialize peripheral gclk from device tree node. */
static void clock_gclkperiph_init(const struct device *dev, uint32_t subsys_val, uint8_t pch_src,
				  uint8_t enable)
{
	const struct clock_mchp_config *config = dev->config;
	union clock_mchp_subsys subsys;
	uint32_t val32;

	subsys.val = subsys_val;

	/* PCHCTRL */
	val32 = 0;
	val32 |= GCLK_PCHCTRL_CHEN(enable);
	val32 |= GCLK_PCHCTRL_GEN(pch_src);

	config->gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph] = val32;
}

/* initialize cpu mclk from device tree node.*/
static void clock_mclkcpu_init(const struct device *dev, uint8_t cpu_div)
{
	const struct clock_mchp_config *config = dev->config;

	config->mclk_regs->MCLK_CPUDIV = MCLK_CPUDIV_CPUDIV(cpu_div);
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
	mask_reg = get_mclkbus_mask_reg(config->mclk_regs, subsys.bits.mclkbus);

	if (mask_reg != NULL) {
		if (enable != 0) {
			*mask_reg |= mask;

		} else {
			*mask_reg &= ~mask;
		}
	}
}

#define CLOCK_MCHP_PROCESS_OSCHF(node)                                                             \
	{                                                                                          \
		oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;                          \
		uint32_t val32;                                                                    \
		uint8_t freq_sel_val;                                                              \
		freq_sel_val = find_oschf_frequency_bitpos(DT_PROP(node, oschf_frequency));        \
		data->oschf_freq = DT_PROP(node, oschf_frequency);                                 \
		val32 = OSCCTRL_OSCHFCTRL_FRQSEL(freq_sel_val) |                                   \
			OSCCTRL_OSCHFCTRL_AUTOTUNE(DT_PROP(node, oschf_autotune_en)) |             \
			OSCCTRL_OSCHFCTRL_ONDEMAND(DT_PROP(node, oschf_on_demand_en));             \
		oscctrl_regs->OSCCTRL_OSCHFCTRL = val32;                                           \
	}

#define CLOCK_MCHP_PROCESS_RTC(node) clock_rtc_init(dev, DT_ENUM_IDX(node, rtc_src));

#define CLOCK_MCHP_PROCESS_XOSC32K(node)                                                           \
	{                                                                                          \
		osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;                 \
		uint32_t val32;                                                                    \
                                                                                                   \
		/* Check if xosc32k clock is already on */                                         \
		if ((data->gclkgen_src_on_status & BIT(CLOCK_MCHP_GCLK_SRC_XOSC32K)) == 0) {       \
			/* CFDCTRL */                                                              \
			val32 = 0;                                                                 \
			val32 |= OSC32KCTRL_CFDCTRL_CFDEN(DT_PROP(node, xosc32k_cfd_en));          \
			osc32kctrl_regs->OSC32KCTRL_CFDCTRL = val32;                               \
			/* XOSC32K */                                                              \
			val32 = 0;                                                                 \
			val32 |= OSC32KCTRL_XOSC32KCTRL_CSUT(                                      \
				DT_ENUM_IDX(node, xosc32k_startup_time));                          \
			val32 |= OSC32KCTRL_XOSC32KCTRL_ONDEMAND(                                  \
				DT_PROP(node, xosc32k_on_demand_en));                              \
			val32 |= OSC32KCTRL_XOSC32KCTRL_XTALEN(DT_PROP(node, xosc32k_xtal_en));    \
			val32 |= OSC32KCTRL_XOSC32KCTRL_ENABLE(DT_PROP(node, xosc32k_en));         \
			val32 |= OSC32KCTRL_XOSC32KCTRL_LPMODE(DT_PROP(node, low_power_en));       \
			osc32kctrl_regs->OSC32KCTRL_XOSC32KCTRL = val32;                           \
			if (DT_PROP(node, xosc32k_en) != 0) {                                      \
				if (WAIT_FOR(((osc32kctrl_regs->OSC32KCTRL_STATUS &                \
					       OSC32KCTRL_STATUS_XOSC32KRDY_Msk) != 0),            \
					     TIMEOUT_XOSC32KCTRL_RDY, NULL) == false) {            \
					LOG_ERR("XOSC32KCTRL ready timed out");                    \
				} else {                                                           \
					data->gclkgen_src_on_status |=                             \
						BIT(CLOCK_MCHP_GCLK_SRC_XOSC32K);                  \
				}                                                                  \
			}                                                                          \
		}                                                                                  \
	}

#define CLOCK_MCHP_PROCESS_OSC32K(node) clock_osc32k_init(dev, DT_PROP(node, osc32k_on_demand_en));

#ifdef CONFIG_CLOCK_CONTROL_MCHP_GCLK_PINOUT
#define GET_PIN_SRC_FREQ(child)                                                                    \
	if (inst <= GCLK_IO_MAX) {                                                                 \
		data->gclkpin_freq[inst] = DT_PROP(child, gclkgen_pin_src_freq);                   \
	}
#else
#define GET_PIN_SRC_FREQ(child)
#endif /*CONFIG_CLOCK_CONTROL_MCHP_GCLK_PINOUT*/

#define CLOCK_MCHP_ITERATE_GCLKGEN(child)                                                          \
	{                                                                                          \
		do {                                                                               \
			union clock_mchp_subsys subsys;                                            \
			subsys.val = DT_PROP(child, subsystem);                                    \
			int inst = subsys.bits.inst;                                               \
			uint32_t val32 = 0;                                                        \
			if ((data->gclkgen_src_on_status &                                         \
			     BIT(DT_ENUM_IDX(child, gclkgen_src))) != 0) {                         \
				LOG_ERR("glckgen_src of subsys %x not turned on", subsys.val);     \
				break;                                                             \
			}                                                                          \
			GET_PIN_SRC_FREQ(child)                                                    \
			if ((inst == CLOCK_MCHP_GCLKGEN_GEN1) ||                                   \
			    (DT_PROP(child, gclkgen_div_factor) <= 0xFF)) {                        \
				val32 |= GCLK_GENCTRL_DIV(DT_PROP(child, gclkgen_div_factor));     \
			}                                                                          \
			val32 |= GCLK_GENCTRL_RUNSTDBY(DT_PROP(child, gclkgen_run_in_standby_en)); \
			if (DT_ENUM_IDX(child, gclkgen_div_select) == 0) { /* div-factor */        \
				val32 |= GCLK_GENCTRL_DIVSEL(GCLK_GENCTRL_DIVSEL_DIV1_Val);        \
			} else { /* div-factor-power */                                            \
				val32 |= GCLK_GENCTRL_DIVSEL(GCLK_GENCTRL_DIVSEL_DIV2_Val);        \
			}                                                                          \
			val32 |= GCLK_GENCTRL_OE(DT_PROP(child, gclkgen_pin_output_en));           \
			val32 |= GCLK_GENCTRL_OOV(DT_ENUM_IDX(child, gclkgen_pin_output_off_val)); \
			val32 |= GCLK_GENCTRL_IDC(DT_PROP(child, gclkgen_improve_duty_cycle_en));  \
			val32 |= GCLK_GENCTRL_GENEN(DT_PROP(child, gclkgen_en));                   \
			val32 |= GCLK_GENCTRL_SRC(DT_ENUM_IDX(child, gclkgen_src));                \
			config->gclk_regs->GCLK_GENCTRL[inst] = val32;                             \
			if (WAIT_FOR((config->gclk_regs->GCLK_SYNCBUSY == 0), TIMEOUT_REG_SYNC,    \
				     NULL) == false) {                                             \
				LOG_ERR("GCLK_SYNCBUSY timeout on writing GCLK_GENCTRL[%d]",       \
					inst);                                                     \
				break;                                                             \
			}                                                                          \
			/* To avoid changing oschf, while gclk0 drives it. Else will affect CPU */ \
			if (inst == CLOCK_MCHP_GCLKGEN_GEN0) {                                     \
				data->gclk0_src = DT_ENUM_IDX(child, gclkgen_src);                 \
			}                                                                          \
			if (inst == CLOCK_MCHP_GCLKGEN_GEN1) {                                     \
				data->gclkgen_src_on_status |= BIT(CLOCK_MCHP_GCLK_SRC_GCLKGEN1);  \
			}                                                                          \
		} while (0);                                                                       \
	}

#define CLOCK_MCHP_ITERATE_GCLKPERIPH(child)                                                       \
	{                                                                                          \
		clock_gclkperiph_init(dev, DT_PROP(child, subsystem),                              \
				      DT_ENUM_IDX(child, gclkperiph_src),                          \
				      DT_PROP(child, gclkperiph_en));                              \
	}

#define CLOCK_MCHP_PROCESS_MCLKCPU(node) clock_mclkcpu_init(dev, DT_PROP(node, mclk_cpu_div));

#define CLOCK_MCHP_ITERATE_MCLKPERIPH(child)                                                       \
	{                                                                                          \
		clock_mclkperiph_init(dev, DT_PROP(child, subsystem), DT_PROP(child, mclk_en));    \
	}

#endif /* CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP */

/* clock driver initialization function. */
static int clock_mchp_init(const struct device *dev)
{
#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP
	struct clock_mchp_data *data = dev->data;
	const struct clock_mchp_config *config = dev->config;

	/* iteration-1 */
	CLOCK_MCHP_PROCESS_OSCHF(DT_NODELABEL(oschf));
	CLOCK_MCHP_PROCESS_OSC32K(DT_NODELABEL(osc32k));
	CLOCK_MCHP_PROCESS_XOSC32K(DT_NODELABEL(xosc32k));
	CLOCK_MCHP_PROCESS_RTC(DT_NODELABEL(rtcclock));

	/* To avoid changing oschf, while gclk0 is driven by it. Else will affect CPU */
	data->gclk0_src = CLOCK_MCHP_GCLK_SRC_OSCHF;

	DT_FOREACH_CHILD(DT_NODELABEL(gclkgen), CLOCK_MCHP_ITERATE_GCLKGEN);

	DT_FOREACH_CHILD(DT_NODELABEL(gclkperiph), CLOCK_MCHP_ITERATE_GCLKPERIPH);
	DT_FOREACH_CHILD(DT_NODELABEL(mclkperiph), CLOCK_MCHP_ITERATE_MCLKPERIPH);

	CLOCK_MCHP_PROCESS_MCLKCPU(DT_NODELABEL(mclkcpu));
#endif /* CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP */

	return 0;
}

static DEVICE_API(clock_control, clock_mchp_driver_api) = {
	.on = clock_mchp_on,
	.off = clock_mchp_off,
	.get_status = clock_mchp_get_status,
#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE
	.get_rate = clock_mchp_get_rate,
#endif /* CONFIG_CLOCK_CONTROL_MCHP_GET_RATE*/
};

#define CLOCK_MCHP_CONFIG_DEFN()                                                                   \
	static const struct clock_mchp_config clock_config = {                                     \
		.on_timeout_ms = DT_PROP_OR(CLOCK_NODE, on_timeout_ms, 5),                         \
		.mclk_regs = (mclk_registers_t *)DT_REG_ADDR_BY_NAME(CLOCK_NODE, mclk),            \
		.oscctrl_regs = (oscctrl_registers_t *)DT_REG_ADDR_BY_NAME(CLOCK_NODE, oscctrl),   \
		.osc32kctrl_regs =                                                                 \
			(osc32kctrl_registers_t *)DT_REG_ADDR_BY_NAME(CLOCK_NODE, osc32kctrl),     \
		.gclk_regs = (gclk_registers_t *)DT_REG_ADDR_BY_NAME(CLOCK_NODE, gclk)}

#define CLOCK_MCHP_DATA_DEFN() static struct clock_mchp_data clock_data;

#define CLOCK_MCHP_DEVICE_INIT(n)                                                                  \
	CLOCK_MCHP_CONFIG_DEFN();                                                                  \
	CLOCK_MCHP_DATA_DEFN();                                                                    \
	DEVICE_DT_INST_DEFINE(n, clock_mchp_init, NULL, &clock_data, &clock_config, PRE_KERNEL_1,  \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_mchp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CLOCK_MCHP_DEVICE_INIT)
