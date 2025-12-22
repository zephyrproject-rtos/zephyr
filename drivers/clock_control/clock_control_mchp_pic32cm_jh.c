/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file clock_control_mchp_pic32cm_jh.c
 * @brief Clock control driver for pic32cm_jh family devices.
 */

#include <stdbool.h>
#include <string.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>

#define DT_DRV_COMPAT microchip_pic32cm_jh_clock

LOG_MODULE_REGISTER(clock_mchp_pic32cm_jh, CONFIG_CLOCK_CONTROL_LOG_LEVEL);
#define CLOCK_NODE    DT_NODELABEL(clock)
#define CLOCK_SUCCESS 0

/* Properties not exposed in binding file, initialize to values given below */
#define CLOCK_OSCCTRL_XOSCCTRL_GAIN_VALUE (4)

/* Frequency values */
#define FREQ_32KHZ 32768
#define FREQ_1KHZ  1024
#define FREQ_48MHZ 48000000
#define FREQ_96MHZ 96000000

/* timeout values in microseconds */
#define TIMEOUT_XOSC_RDY        1000000
#define TIMEOUT_OSC48M_RDY      1000000
#define TIMEOUT_FDPLL_LOCK_RDY  1000000
#define TIMEOUT_XOSC32KCTRL_RDY 1000000
#define TIMEOUT_OSC32KCTRL_RDY  1000000
#define TIMEOUT_REG_SYNC        1000

#define GCLK_IO_MAX                7
#define GCLK_PH_MAX                47
#define BIT_MASK_MAX               31
#define CLOCK_INIT_ITERATION_COUNT 3

/* maximum value for div, when div_select is clock source frequency divided by 2^(N+1) */
#define GCLKGEN_POWER_DIV_MAX 29

/* mclkbus Not Applicable for a clock subsystem ID */
#define MBUS_NA (0x3f)

/* mclkmaskbit Not Applicable for a clock subsystem ID */
#define MMASK_NA (0x3f)

/* gclkperiph Not Applicable for a clock subsystem ID */
#define GCLK_PH_NA (0x3f)

/* Clock subsystem Types */
#define SUBSYS_TYPE_XOSC       (0)
#define SUBSYS_TYPE_OSC48M     (1)
#define SUBSYS_TYPE_FDPLL      (2)
#define SUBSYS_TYPE_RTC        (3)
#define SUBSYS_TYPE_XOSC32K    (4)
#define SUBSYS_TYPE_OSC32K     (5)
#define SUBSYS_TYPE_GCLKGEN    (6)
#define SUBSYS_TYPE_GCLKPERIPH (7)
#define SUBSYS_TYPE_MCLKCPU    (8)
#define SUBSYS_TYPE_MCLKPERIPH (9)
#define SUBSYS_TYPE_MAX        (9)

/* mclk bus */
#define MBUS_AHB  (0)
#define MBUS_APBA (1)
#define MBUS_APBB (2)
#define MBUS_APBC (3)
#define MBUS_APBD (4)
#define MBUS_MAX  (4)

/* XOSC32K instances */
#define INST_XOSC32K_XOSC1K  0
#define INST_XOSC32K_XOSC32K 1

/* OSC32K instances */
#define INST_OSC32K_OSC1K  0
#define INST_OSC32K_OSC32K 1

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

#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP
/* XOSC initialization structure. */
struct clock_xosc_init {
	uint8_t startup_time;                        /* XOSCCTRL */
	uint8_t automatic_amplitude_gain_control_en; /* XOSCCTRL */
	uint8_t on_demand_en;                        /* XOSCCTRL */
	uint8_t run_in_standby_en;                   /* XOSCCTRL */
	uint8_t clock_failure_detection_en;          /* XOSCCTRL */
	uint8_t xtal_en;                             /* XOSCCTRL */
	uint8_t enable;                              /* XOSCCTRL */
	uint32_t frequency;
};

/* OSC48M initialization structure. */
struct clock_osc48m_init {
	uint8_t on_demand_en;      /* OSC48MCTRL */
	uint8_t run_in_standby_en; /* OSC48MCTRL */
	uint8_t enable;            /* OSC48MCTRL */
	uint8_t post_divider_freq; /* OSC48MDIV */
};

/* FDPLL initialization structure. */
struct clock_fdpll_init {
	uint8_t on_demand_en;        /* DPLLCTRLA */
	uint8_t run_in_standby_en;   /* DPLLCTRLA */
	uint8_t enable;              /* DPLLCTRLA */
	uint8_t divider_ratio_frac;  /* DPLLRATIO */
	uint16_t divider_ratio_int;  /* DPLLRATIO */
	uint16_t xosc_clock_divider; /* DPLLCTRLB */
	uint8_t lock_bypass_en;      /* DPLLCTRLB */
	uint8_t src;                 /* DPLLCTRLB */
	uint8_t wakeup_fast_en;      /* DPLLCTRLB */
	uint8_t low_power_en;        /* DPLLCTRLB */
	uint8_t pi_filter_type;      /* DPLLCTRLB */
	uint8_t output_prescalar;    /* DPLLPRESC */
};

/* XOSC32K initialization structure. */
struct clock_xosc32k_init {
	uint8_t startup_time;      /* XOSC32K */
	uint8_t on_demand_en;      /* XOSC32K */
	uint8_t run_in_standby_en; /* XOSC32K */
	uint8_t xosc32k_1khz_en;   /* XOSC32K */
	uint8_t xosc32k_32khz_en;  /* XOSC32K */
	uint8_t xtal_en;           /* XOSC32K */
	uint8_t enable;            /* XOSC32K */
	uint8_t cfd_en;            /* CFDCTRL */
};

/* OSC32K initialization structure. */
struct clock_osc32k_init {
	uint8_t startup_time;      /* OSC32K */
	uint8_t on_demand_en;      /* OSC32K */
	uint8_t run_in_standby_en; /* OSC32K */
	uint8_t osc32k_1khz_en;    /* OSC32K */
	uint8_t osc32k_32khz_en;   /* OSC32K */
	uint8_t enable;            /* OSC32K */
};

/* GCLKGEN initialization structure. */
struct clock_gclkgen_init {
	union clock_mchp_subsys subsys;
	uint16_t div_factor;           /* GENCTRLx */
	uint8_t run_in_standby_en;     /* GENCTRLx */
	uint8_t div_select;            /* GENCTRLx */
	uint8_t pin_output_en;         /* GENCTRLx */
	uint8_t pin_output_off_val;    /* GENCTRLx */
	uint8_t improve_duty_cycle_en; /* GENCTRLx */
	uint8_t enable;                /* GENCTRLx */
	uint8_t src;                   /* GENCTRLx */
	uint32_t pin_src_freq;
};
#endif /* CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP */

/* clock driver configuration structure. */
struct clock_mchp_config {
	oscctrl_registers_t *oscctrl_regs;
	osc32kctrl_registers_t *osc32kctrl_regs;
	gclk_registers_t *gclk_regs;
	mclk_registers_t *mclk_regs;

	/* Timeout in milliseconds to wait for clock to turn on */
	uint32_t on_timeout_ms;

	/* Number of wait states for a flash read operation */
	uint8_t flash_wait_states;
};

/* clock driver data structure. */
struct clock_mchp_data {
	uint32_t xosc_crystal_freq;
	uint32_t gclkpin_freq[GCLK_IO_MAX + 1];

	/*
	 * Use bit position as per enum enum clock_mchp_fdpll_src_clock, to show if the specified
	 * clock source to FDPLL is on.
	 */
	uint16_t fdpll_src_on_status;

	/*
	 * Use bit position as per enum enum clock_mchp_gclk_src_clock, to show if the specified
	 * clock source to gclk generator is on.
	 */
	uint16_t gclkgen_src_on_status;
	enum clock_mchp_gclk_src_clock gclk0_src;
};

#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE
static int clock_get_rate_osc48m(const struct device *dev, uint32_t *freq);
static int clock_get_rate_fdpll(const struct device *dev, uint32_t *freq);
#endif /* CONFIG_CLOCK_CONTROL_MCHP_GET_RATE */

static enum clock_control_status clock_mchp_get_status(const struct device *dev,
						       clock_control_subsys_t sys);

/* Internal helper functions */

/* check if subsystem type and id are valid. */
static int clock_check_subsys(union clock_mchp_subsys subsys)
{
	uint32_t inst_max = 0;
	uint32_t gclkperiph_max = GCLK_PH_NA;
	uint32_t mclkbus_max = MBUS_NA;
	uint32_t mclkmaskbit_max = MMASK_NA;

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

	case SUBSYS_TYPE_OSC48M:
		inst_max = CLOCK_MCHP_OSC48M_ID_MAX;
		break;

	case SUBSYS_TYPE_FDPLL:
		inst_max = CLOCK_MCHP_FDPLL_ID_MAX;
		gclkperiph_max = 0;
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
		return -EINVAL;
	}

	return CLOCK_SUCCESS;
}

/* get the address of mclk mask register. */
__IO uint32_t *get_mclkbus_mask_reg(mclk_registers_t *mclk_regs, uint32_t bus)
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
	case MBUS_APBD:
		reg32 = &mclk_regs->MCLK_APBDMASK;
		break;
	default:
		LOG_ERR("Unsupported mclkbus");
		break;
	}

	return reg32;
}

/* get status of respective clock subsystem. */
static enum clock_control_status clock_get_status(const struct device *dev,
						  clock_control_subsys_t sys)
{
	enum clock_control_status ret_status = CLOCK_CONTROL_STATUS_UNKNOWN;
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	union clock_mchp_subsys subsys = {.val = (uint32_t)sys};
	uint8_t inst = subsys.bits.inst;
	uint32_t mask;
	__IO uint32_t *reg32 = NULL;

	switch (subsys.bits.type) {
	case SUBSYS_TYPE_XOSC:
		/* Check if XOSC is enabled */
		if ((oscctrl_regs->OSCCTRL_XOSCCTRL & OSCCTRL_XOSCCTRL_ENABLE_Msk) != 0) {
			/* Check if ready bit is set */
			ret_status =
				((oscctrl_regs->OSCCTRL_STATUS & OSCCTRL_STATUS_XOSCRDY_Msk) == 0)
					? CLOCK_CONTROL_STATUS_STARTING
					: CLOCK_CONTROL_STATUS_ON;
		} else {
			ret_status = CLOCK_CONTROL_STATUS_OFF;
		}
		break;

	case SUBSYS_TYPE_OSC48M:
		/* Check if OSC48M is enabled */
		if ((oscctrl_regs->OSCCTRL_OSC48MCTRL & OSCCTRL_OSC48MCTRL_ENABLE_Msk) != 0) {
			/* Check if ready bit is set */
			ret_status =
				((oscctrl_regs->OSCCTRL_STATUS & OSCCTRL_STATUS_OSC48MRDY_Msk) == 0)
					? CLOCK_CONTROL_STATUS_STARTING
					: CLOCK_CONTROL_STATUS_ON;
		} else {
			ret_status = CLOCK_CONTROL_STATUS_OFF;
		}
		break;

	case SUBSYS_TYPE_FDPLL:
		/* Check if DPLL is enabled */
		if ((oscctrl_regs->OSCCTRL_DPLLCTRLA & OSCCTRL_DPLLCTRLA_ENABLE_Msk) != 0) {
			mask = OSCCTRL_DPLLSTATUS_LOCK_Msk | OSCCTRL_DPLLSTATUS_CLKRDY_Msk;

			/* Check if sync is complete and ready bit is set */
			ret_status = ((oscctrl_regs->OSCCTRL_DPLLSYNCBUSY != 0) ||
				      ((oscctrl_regs->OSCCTRL_DPLLSTATUS & mask) != mask))
					     ? CLOCK_CONTROL_STATUS_STARTING
					     : CLOCK_CONTROL_STATUS_ON;
		} else {
			ret_status = CLOCK_CONTROL_STATUS_OFF;
		}
		break;

	case SUBSYS_TYPE_RTC:
		ret_status = CLOCK_CONTROL_STATUS_ON;
		break;

	case SUBSYS_TYPE_XOSC32K:
		/* For XOSC32K get_status return status of only EN1K or EN32K bits,
		 * which does not indicate the rdy status.
		 */
		if (inst == INST_XOSC32K_XOSC1K) {
			mask = OSC32KCTRL_XOSC32K_ENABLE_Msk | OSC32KCTRL_XOSC32K_EN1K_Msk;
			ret_status = ((osc32kctrl_regs->OSC32KCTRL_XOSC32K & mask) == mask)
					     ? CLOCK_CONTROL_STATUS_ON
					     : CLOCK_CONTROL_STATUS_OFF;
		} else {
			mask = OSC32KCTRL_XOSC32K_ENABLE_Msk | OSC32KCTRL_XOSC32K_EN32K_Msk;
			ret_status = ((osc32kctrl_regs->OSC32KCTRL_XOSC32K & mask) == mask)
					     ? CLOCK_CONTROL_STATUS_ON
					     : CLOCK_CONTROL_STATUS_OFF;
		}
		break;

	case SUBSYS_TYPE_OSC32K:
		/* For OSC32K get_status return status of only EN1K or EN32K bits,
		 * which does not indicate the rdy status.
		 */
		if (inst == INST_OSC32K_OSC1K) {
			mask = OSC32KCTRL_OSC32K_ENABLE_Msk | OSC32KCTRL_OSC32K_EN1K_Msk;
			ret_status = ((osc32kctrl_regs->OSC32KCTRL_OSC32K & mask) == mask)
					     ? CLOCK_CONTROL_STATUS_ON
					     : CLOCK_CONTROL_STATUS_OFF;
		} else {
			mask = OSC32KCTRL_OSC32K_ENABLE_Msk | OSC32KCTRL_OSC32K_EN32K_Msk;
			ret_status = ((osc32kctrl_regs->OSC32KCTRL_OSC32K & mask) == mask)
					     ? CLOCK_CONTROL_STATUS_ON
					     : CLOCK_CONTROL_STATUS_OFF;
		}
		break;

	case SUBSYS_TYPE_GCLKGEN:
		ret_status = CLOCK_CONTROL_STATUS_OFF;
		if ((gclk_regs->GCLK_GENCTRL[inst] & GCLK_GENCTRL_GENEN_Msk) != 0) {
			/* Generator is on, check if it's starting or fully on */
			ret_status = ((gclk_regs->GCLK_SYNCBUSY &
				       (1 << (GCLK_SYNCBUSY_GENCTRL_Pos + inst))) != 0)
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
		mask = 1 << subsys.bits.mclkmaskbit;
		ret_status =
			((*reg32 & mask) != 0) ? CLOCK_CONTROL_STATUS_ON : CLOCK_CONTROL_STATUS_OFF;
		break;

	default:
		break;
	}

	return ret_status;
}

/* function to set/clear clock subsystem enable bit. */
static int clock_on(const struct clock_mchp_config *config, const union clock_mchp_subsys subsys)
{
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	uint8_t inst = subsys.bits.inst;
	__IO uint32_t *reg32 = NULL;

	switch (subsys.bits.type) {
	case SUBSYS_TYPE_XOSC:
		oscctrl_regs->OSCCTRL_XOSCCTRL |= OSCCTRL_XOSCCTRL_ENABLE_Msk;
		break;

	case SUBSYS_TYPE_OSC48M:
		oscctrl_regs->OSCCTRL_OSC48MCTRL |= OSCCTRL_OSC48MCTRL_ENABLE_Msk;
		break;

	case SUBSYS_TYPE_FDPLL:
		oscctrl_regs->OSCCTRL_DPLLCTRLA |= OSCCTRL_DPLLCTRLA_ENABLE_Msk;
		break;

	case SUBSYS_TYPE_XOSC32K:
		if (inst == INST_XOSC32K_XOSC1K) {
			osc32kctrl_regs->OSC32KCTRL_XOSC32K |= OSC32KCTRL_XOSC32K_EN1K_Msk;
		} else {
			osc32kctrl_regs->OSC32KCTRL_XOSC32K |= OSC32KCTRL_XOSC32K_EN32K_Msk;
		}

		/* turn on XOSC32K if any of EN1K or EN32K is to be on */
		osc32kctrl_regs->OSC32KCTRL_XOSC32K |= OSC32KCTRL_XOSC32K_ENABLE_Msk;
		break;

	case SUBSYS_TYPE_OSC32K:
		if (inst == INST_OSC32K_OSC1K) {
			osc32kctrl_regs->OSC32KCTRL_OSC32K |= OSC32KCTRL_OSC32K_EN1K_Msk;
		} else {
			osc32kctrl_regs->OSC32KCTRL_OSC32K |= OSC32KCTRL_OSC32K_EN32K_Msk;
		}

		/* turn on OSC32K if any of EN1K or EN32K is to be on */
		osc32kctrl_regs->OSC32KCTRL_OSC32K |= OSC32KCTRL_OSC32K_ENABLE_Msk;
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
		*reg32 |= 1 << subsys.bits.mclkmaskbit;
		break;

	default:
		return -ENOTSUP;
	}

	return CLOCK_SUCCESS;
}

/* function to set/clear clock subsystem enable bit. */
static int clock_off(const struct clock_mchp_config *config, const union clock_mchp_subsys subsys)
{
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	uint8_t inst = subsys.bits.inst;
	__IO uint32_t *reg32 = NULL;

	switch (subsys.bits.type) {
	case SUBSYS_TYPE_XOSC:
		oscctrl_regs->OSCCTRL_XOSCCTRL &= ~(OSCCTRL_XOSCCTRL_ENABLE_Msk);
		break;

	case SUBSYS_TYPE_OSC48M:
		oscctrl_regs->OSCCTRL_OSC48MCTRL &= ~(OSCCTRL_OSC48MCTRL_ENABLE_Msk);
		break;

	case SUBSYS_TYPE_FDPLL:
		oscctrl_regs->OSCCTRL_DPLLCTRLA &= ~(OSCCTRL_DPLLCTRLA_ENABLE_Msk);
		break;

	case SUBSYS_TYPE_XOSC32K:
		if (inst == INST_XOSC32K_XOSC1K) {
			osc32kctrl_regs->OSC32KCTRL_XOSC32K &= ~(OSC32KCTRL_XOSC32K_EN1K_Msk);
		} else {
			osc32kctrl_regs->OSC32KCTRL_XOSC32K &= ~(OSC32KCTRL_XOSC32K_EN32K_Msk);
		}

		if ((osc32kctrl_regs->OSC32KCTRL_XOSC32K &
		     (OSC32KCTRL_XOSC32K_EN1K_Msk | OSC32KCTRL_XOSC32K_EN32K_Msk)) == 0) {
			/* turn off XOSC32K if both EN1K and EN32K are off */
			osc32kctrl_regs->OSC32KCTRL_XOSC32K &= ~OSC32KCTRL_XOSC32K_ENABLE_Msk;
		}
		break;

	case SUBSYS_TYPE_OSC32K:
		if (inst == INST_OSC32K_OSC1K) {
			osc32kctrl_regs->OSC32KCTRL_OSC32K &= ~(OSC32KCTRL_OSC32K_EN1K_Msk);
		} else {
			osc32kctrl_regs->OSC32KCTRL_OSC32K &= ~(OSC32KCTRL_OSC32K_EN32K_Msk);
		}

		if ((osc32kctrl_regs->OSC32KCTRL_OSC32K &
		     (OSC32KCTRL_OSC32K_EN1K_Msk | OSC32KCTRL_OSC32K_EN32K_Msk)) == 0) {
			/* turn off OSC32K if both EN1K and EN32K are off */
			osc32kctrl_regs->OSC32KCTRL_OSC32K &= ~OSC32KCTRL_OSC32K_ENABLE_Msk;
		}
		break;

	case SUBSYS_TYPE_GCLKGEN:
		/* Check if the clock is GCLKGEN0, which is always on */
		if (inst == CLOCK_MCHP_GCLKGEN_GEN0) {
			/* GCLK GEN0 is always on */
			break;
		}

		/* Disable the clock generator by clearing the GENEN bit */
		gclk_regs->GCLK_GENCTRL[inst] &= ~(GCLK_GENCTRL_GENEN_Msk);
		break;

	case SUBSYS_TYPE_GCLKPERIPH:
		gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph] &= ~(GCLK_PCHCTRL_CHEN_Msk);
		break;

	case SUBSYS_TYPE_MCLKPERIPH:
		reg32 = get_mclkbus_mask_reg(config->mclk_regs, subsys.bits.mclkbus);
		*reg32 &= ~(1 << subsys.bits.mclkmaskbit);
		break;

	default:
		return -ENOTSUP;
	}

	return CLOCK_SUCCESS;
}

#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE
/* get rate of gclk generator in Hz. */
static int clock_get_rate_gclkgen(const struct device *dev, enum clock_mchp_gclkgen gclkgen_id,
				  enum clock_mchp_gclk_src_clock gclkgen_called_src, uint32_t *freq)
{
	int ret_val = CLOCK_SUCCESS;
	const struct clock_mchp_config *config = dev->config;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;
	struct clock_mchp_data *data = dev->data;

	enum clock_mchp_gclk_src_clock gclkgen_src;
	uint32_t gclkgen_src_freq = 0;
	uint32_t mask;
	uint16_t gclkgen_div;

	bool power_div = (((gclk_regs->GCLK_GENCTRL[gclkgen_id] & GCLK_GENCTRL_DIVSEL_Msk) >>
			   GCLK_GENCTRL_DIVSEL_Pos) == GCLK_GENCTRL_DIVSEL_DIV1_Val)
				 ? false
				 : true;

	/* Return rate as 0, if clock is not on */
	if (clock_mchp_get_status(dev, (clock_control_subsys_t)MCHP_CLOCK_DERIVE_ID(
					       SUBSYS_TYPE_GCLKGEN, MBUS_NA, MMASK_NA, GCLK_PH_NA,
					       gclkgen_id)) != CLOCK_CONTROL_STATUS_ON) {
		*freq = 0;
		return CLOCK_SUCCESS;
	}

	/* get source for gclk generator from gclkgen registers */
	gclkgen_src = (gclk_regs->GCLK_GENCTRL[gclkgen_id] & GCLK_GENCTRL_SRC_Msk) >>
		      GCLK_GENCTRL_SRC_Pos;
	if (gclkgen_called_src == gclkgen_src) {
		return -ENOTSUP;
	}

	switch (gclkgen_src) {
	case CLOCK_MCHP_GCLK_SRC_XOSC:
		gclkgen_src_freq = data->xosc_crystal_freq;
		break;

	case CLOCK_MCHP_GCLK_SRC_OSC48M:
		ret_val = clock_get_rate_osc48m(dev, &gclkgen_src_freq);
		break;

	case CLOCK_MCHP_GCLK_SRC_FDPLL:
		ret_val = clock_get_rate_fdpll(dev, &gclkgen_src_freq);
		break;

	case CLOCK_MCHP_GCLK_SRC_OSCULP32K:
		gclkgen_src_freq = FREQ_32KHZ;
		break;

	case CLOCK_MCHP_GCLK_SRC_XOSC32K:
		mask = OSC32KCTRL_XOSC32K_ENABLE_Msk | OSC32KCTRL_XOSC32K_EN32K_Msk;
		gclkgen_src_freq =
			((osc32kctrl_regs->OSC32KCTRL_XOSC32K & mask) == mask) ? FREQ_32KHZ : 0;
		break;

	case CLOCK_MCHP_GCLK_SRC_OSC32K:
		mask = OSC32KCTRL_OSC32K_ENABLE_Msk | OSC32KCTRL_OSC32K_EN32K_Msk;
		gclkgen_src_freq =
			((osc32kctrl_regs->OSC32KCTRL_OSC32K & mask) == mask) ? FREQ_32KHZ : 0;
		break;

	case CLOCK_MCHP_GCLK_SRC_GCLKPIN:
		if (gclkgen_id <= GCLK_IO_MAX) {
			gclkgen_src_freq = data->gclkpin_freq[gclkgen_id];
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

	default:
		break;
	}

	if (ret_val != CLOCK_SUCCESS) {
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
		gclkgen_div = 1 << (gclkgen_div + 1);
	}

	/* if DIV value is 0, has same effect as DIV value 1 */
	if (gclkgen_div == 0) {
		gclkgen_div = 1;
	}
	*freq = gclkgen_src_freq / gclkgen_div;

	return ret_val;
}

/* get rate of OSC48M in Hz. */
static int clock_get_rate_osc48m(const struct device *dev, uint32_t *freq)
{
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;

	uint8_t post_divider_freq;
	const uint32_t post_divider_freq_array[] = {
		48000000, 24000000, 16000000, 12000000, 9600000, 8000000, 6860000, 6000000,
		5330000,  4800000,  4360000,  4000000,  3690000, 3430000, 3200000, 3000000};

	if ((oscctrl_regs->OSCCTRL_STATUS & OSCCTRL_STATUS_OSC48MRDY_Msk) == 0) {
		/* Return rate as 0, if clock is not on */
		*freq = 0;
	} else {
		post_divider_freq = (oscctrl_regs->OSCCTRL_OSC48MDIV & OSCCTRL_OSC48MDIV_DIV_Msk) >>
				    OSCCTRL_OSC48MDIV_DIV_Pos;
		*freq = post_divider_freq_array[post_divider_freq];
	}

	return CLOCK_SUCCESS;
}

/* get rate of FDPLL in Hz. */
static int clock_get_rate_fdpll(const struct device *dev, uint32_t *freq)
{
	int ret_val = CLOCK_SUCCESS;
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	struct clock_mchp_data *data = dev->data;

	uint32_t src_freq = 0, xosc_div, mult_int, mult_frac, frac_mult_max;
	enum clock_mchp_gclkgen src_gclkgen;
	uint8_t ref_clk_type, output_prescalar;

	/* Return rate as 0, if clock is not on */
	if (clock_mchp_get_status(dev, (clock_control_subsys_t)MCHP_CLOCK_DERIVE_ID(
					       SUBSYS_TYPE_FDPLL, MBUS_NA, MMASK_NA, 0, 0)) !=
	    CLOCK_CONTROL_STATUS_ON) {
		*freq = 0;
		return CLOCK_SUCCESS;
	}

	/* Find the source clock */
	ref_clk_type = (oscctrl_regs->OSCCTRL_DPLLCTRLB & OSCCTRL_DPLLCTRLB_REFCLK_Msk) >>
		       OSCCTRL_DPLLCTRLB_REFCLK_Pos;

	/* Find the source clock frequency */
	switch (ref_clk_type) {
	case OSCCTRL_DPLLCTRLB_REFCLK_XOSC32K_Val:
		src_freq = FREQ_32KHZ;
		break;

	case OSCCTRL_DPLLCTRLB_REFCLK_XOSC_Val:
		xosc_div = (oscctrl_regs->OSCCTRL_DPLLCTRLB & OSCCTRL_DPLLCTRLB_DIV_Msk) >>
			   OSCCTRL_DPLLCTRLB_DIV_Pos;
		src_freq = data->xosc_crystal_freq / (2 * (xosc_div + 1));
		break;

	case OSCCTRL_DPLLCTRLB_REFCLK_GCLK_Val:
		src_gclkgen = (config->gclk_regs->GCLK_PCHCTRL[0] & GCLK_PCHCTRL_GEN_Msk) >>
			      GCLK_PCHCTRL_GEN_Pos;
		ret_val = clock_get_rate_gclkgen(dev, src_gclkgen, CLOCK_MCHP_GCLK_SRC_FDPLL,
						 &src_freq);
		break;

	default:
		break;
	}

	if (ret_val != CLOCK_SUCCESS) {
		return ret_val;
	}

	/* Multiply by integer & fractional part multipliers */
	mult_int = (oscctrl_regs->OSCCTRL_DPLLRATIO & OSCCTRL_DPLLRATIO_LDR_Msk) >>
		   OSCCTRL_DPLLRATIO_LDR_Pos;
	mult_frac = (oscctrl_regs->OSCCTRL_DPLLRATIO & OSCCTRL_DPLLRATIO_LDRFRAC_Msk) >>
		    OSCCTRL_DPLLRATIO_LDRFRAC_Pos;

	frac_mult_max = OSCCTRL_DPLLRATIO_LDRFRAC_Msk >> OSCCTRL_DPLLRATIO_LDRFRAC_Pos;
	*freq = (src_freq * (((mult_int + 1) * (frac_mult_max + 1)) + mult_frac)) /
		(frac_mult_max + 1);

	/* Divide by output prescalar value */
	output_prescalar = (oscctrl_regs->OSCCTRL_DPLLPRESC & OSCCTRL_DPLLPRESC_PRESC_Msk) >>
			   OSCCTRL_DPLLPRESC_PRESC_Pos;
	*freq = *freq / (1 << output_prescalar);

	return ret_val;
}

/* get rate of RTC in Hz. */
static int clock_get_rate_rtc(const struct device *dev, uint32_t *freq)
{
	const struct clock_mchp_config *config = dev->config;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;

	uint8_t rtc_src;
	uint32_t mask;

	/* get rtc source clock*/
	rtc_src = (config->osc32kctrl_regs->OSC32KCTRL_RTCCTRL & OSC32KCTRL_RTCCTRL_RTCSEL_Msk) >>
		  OSC32KCTRL_RTCCTRL_RTCSEL_Pos;

	switch (rtc_src) {
	case OSC32KCTRL_RTCCTRL_RTCSEL_ULP1K_Val:
		*freq = FREQ_1KHZ;
		break;

	case OSC32KCTRL_RTCCTRL_RTCSEL_ULP32K_Val:
		*freq = FREQ_32KHZ;
		break;

	case OSC32KCTRL_RTCCTRL_RTCSEL_OSC1K_Val:
		mask = OSC32KCTRL_OSC32K_ENABLE_Msk | OSC32KCTRL_OSC32K_EN1K_Msk;
		*freq = ((osc32kctrl_regs->OSC32KCTRL_OSC32K & mask) == mask) ? FREQ_1KHZ : 0;
		break;

	case OSC32KCTRL_RTCCTRL_RTCSEL_OSC32K_Val:
		mask = OSC32KCTRL_OSC32K_ENABLE_Msk | OSC32KCTRL_OSC32K_EN32K_Msk;
		*freq = ((osc32kctrl_regs->OSC32KCTRL_OSC32K & mask) == mask) ? FREQ_32KHZ : 0;
		break;

	case OSC32KCTRL_RTCCTRL_RTCSEL_XOSC1K_Val:
		mask = OSC32KCTRL_XOSC32K_ENABLE_Msk | OSC32KCTRL_XOSC32K_EN1K_Msk;
		*freq = ((osc32kctrl_regs->OSC32KCTRL_XOSC32K & mask) == mask) ? FREQ_1KHZ : 0;
		break;

	case OSC32KCTRL_RTCCTRL_RTCSEL_XOSC32K_Val:
		mask = OSC32KCTRL_XOSC32K_ENABLE_Msk | OSC32KCTRL_XOSC32K_EN32K_Msk;
		*freq = ((osc32kctrl_regs->OSC32KCTRL_XOSC32K & mask) == mask) ? FREQ_32KHZ : 0;
		break;

	default:
		return -ENOTSUP;
	}

	return CLOCK_SUCCESS;
}
#endif /* CONFIG_CLOCK_CONTROL_MCHP_GET_RATE */

#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_RUNTIME
/* configure OSC48M. */
static void clock_configure_osc48m(const struct device *dev, void *req_config)
{
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	uint8_t val8 = 0;
	struct clock_mchp_subsys_osc48m_config *osc48m_config =
		(struct clock_mchp_subsys_osc48m_config *)req_config;

	/* Configure on_demand_en and run_in_standby_en */
	val8 = oscctrl_regs->OSCCTRL_OSC48MCTRL;
	val8 &= ~(OSCCTRL_OSC48MCTRL_RUNSTDBY_Msk | OSCCTRL_OSC48MCTRL_ONDEMAND_Msk);
	val8 |= ((osc48m_config->run_in_standby_en != 0) ? OSCCTRL_OSC48MCTRL_RUNSTDBY(1) : 0);
	val8 |= ((osc48m_config->on_demand_en != 0) ? OSCCTRL_OSC48MCTRL_ONDEMAND(1) : 0);
	oscctrl_regs->OSCCTRL_OSC48MCTRL = val8;

	/* Configure post_divider_freq */
	if (osc48m_config->post_divider_freq <= CLOCK_MCHP_DIVIDER_3_MHZ) {
		val8 = oscctrl_regs->OSCCTRL_OSC48MDIV;
		val8 &= ~(OSCCTRL_OSC48MDIV_DIV_Msk);
		val8 |= osc48m_config->post_divider_freq;
		oscctrl_regs->OSCCTRL_OSC48MDIV = val8;
	} else {
		LOG_ERR("Unsupported OSC48M post_divider_freq");
	}
}

/* configure FDPLL. */
static void clock_configure_fdpll(const struct device *dev, void *req_config)
{
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	uint32_t val32 = 0;
	uint8_t val8 = 0;
	struct clock_mchp_subsys_fdpll_config *fdpll_config =
		(struct clock_mchp_subsys_fdpll_config *)req_config;

	val32 = oscctrl_regs->OSCCTRL_DPLLCTRLB;
	if (fdpll_config->src <= CLOCK_MCHP_FDPLL_SRC_XOSC) {
		val32 &= ~OSCCTRL_DPLLCTRLB_REFCLK_Msk;
		switch (fdpll_config->src) {
		case CLOCK_MCHP_FDPLL_SRC_XOSC32K:
			val32 |= OSCCTRL_DPLLCTRLB_REFCLK_XOSC32K;
			break;

		case CLOCK_MCHP_FDPLL_SRC_XOSC:
			val32 |= OSCCTRL_DPLLCTRLB_REFCLK_XOSC;
			break;

		default:
			val32 |= OSCCTRL_DPLLCTRLB_REFCLK_GCLK;
			/* source is gclk*/
			gclk_regs->GCLK_PCHCTRL[0] &= ~GCLK_PCHCTRL_GEN_Msk;
			gclk_regs->GCLK_PCHCTRL[0] |= GCLK_PCHCTRL_GEN(fdpll_config->src);
			break;
		}
	} else {
		LOG_ERR("Unsupported FDPLL source clock");
	}

	val32 &= ~OSCCTRL_DPLLCTRLB_DIV_Msk;
	val32 |= OSCCTRL_DPLLCTRLB_DIV(fdpll_config->xosc_clock_divider);
	oscctrl_regs->OSCCTRL_DPLLCTRLB = val32;

	/* DPLLRATIO */
	val32 = oscctrl_regs->OSCCTRL_DPLLRATIO;
	val32 &= ~(OSCCTRL_DPLLRATIO_LDRFRAC_Msk | OSCCTRL_DPLLRATIO_LDR_Msk);
	val32 |= OSCCTRL_DPLLRATIO_LDRFRAC(fdpll_config->divider_ratio_frac);
	val32 |= OSCCTRL_DPLLRATIO_LDR(fdpll_config->divider_ratio_int);
	oscctrl_regs->OSCCTRL_DPLLRATIO = val32;

	/* DPLLCTRLA */
	val8 = oscctrl_regs->OSCCTRL_DPLLCTRLA;
	val8 &= ~(OSCCTRL_DPLLCTRLA_RUNSTDBY_Msk | OSCCTRL_DPLLCTRLA_ONDEMAND_Msk);
	val8 |= ((fdpll_config->run_in_standby_en != 0) ? OSCCTRL_DPLLCTRLA_RUNSTDBY(1) : 0);
	val8 |= ((fdpll_config->on_demand_en != 0) ? OSCCTRL_DPLLCTRLA_ONDEMAND(1) : 0);
	oscctrl_regs->OSCCTRL_DPLLCTRLA = val8;
}

/* configure GCLKGEN. */
static void clock_configure_gclkgen(const struct device *dev, uint8_t inst, void *req_config)
{
	const struct clock_mchp_config *config = dev->config;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	uint32_t val32 = 0;
	struct clock_mchp_subsys_gclkgen_config *gclkgen_config =
		(struct clock_mchp_subsys_gclkgen_config *)req_config;

	val32 = gclk_regs->GCLK_GENCTRL[inst];
	val32 &= ~(GCLK_GENCTRL_RUNSTDBY_Msk | GCLK_GENCTRL_SRC_Msk | GCLK_GENCTRL_DIV_Msk);
	val32 |= ((gclkgen_config->run_in_standby_en != 0) ? GCLK_GENCTRL_RUNSTDBY(1) : 0);

	val32 |= GCLK_GENCTRL_SRC(gclkgen_config->src);
	/* check range for div_factor, gclk1: 0 - 65535, others: 0 - 255 */
	if ((inst == CLOCK_MCHP_GCLKGEN_GEN1) || (gclkgen_config->div_factor <= 0xFF)) {
		val32 |= GCLK_GENCTRL_DIV(gclkgen_config->div_factor);
	}

	gclk_regs->GCLK_GENCTRL[inst] = val32;
}
#endif /* CONFIG_CLOCK_CONTROL_MCHP_CONFIG_RUNTIME */

/* API functions */

static int clock_mchp_on(const struct device *dev, clock_control_subsys_t sys)
{
	int ret_val = -ENOTSUP;
	const struct clock_mchp_config *config = dev->config;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;
	union clock_mchp_subsys subsys = {.val = (uint32_t)sys};

	enum clock_control_status status;
	uint32_t on_timeout_ms = 0;
	bool is_wait = false;

	/* Validate subsystem. */
	if (CLOCK_SUCCESS == clock_check_subsys(subsys)) {
		status = clock_mchp_get_status(dev, sys);
		if (status == CLOCK_CONTROL_STATUS_ON) {
			/* clock is already on. */
			ret_val = -EALREADY;

		} else {
			if (clock_on(config, subsys) == CLOCK_SUCCESS) {
				/* clock on operation is successful. */
				is_wait = true;
			}
		}
	}

	/* Wait until the clock state becomes ON. */
	while (is_wait == true) {
		/* For OSC32K, need to wait for the oscillator to be on. get_status only
		 * return if EN1K or EN32K is on, which does not indicate the status of
		 * OSC32K
		 */
		if ((subsys.bits.type == SUBSYS_TYPE_XOSC32K) &&
		    ((osc32kctrl_regs->OSC32KCTRL_STATUS & OSC32KCTRL_STATUS_XOSC32KRDY_Msk) !=
		     0)) {
			/* Successfully turned on clock. */
			ret_val = CLOCK_SUCCESS;
		} else if ((subsys.bits.type == SUBSYS_TYPE_OSC32K) &&
			   ((osc32kctrl_regs->OSC32KCTRL_STATUS &
			     OSC32KCTRL_STATUS_OSC32KRDY_Msk) != 0)) {
			/* Successfully turned on clock. */
			ret_val = CLOCK_SUCCESS;
		} else {
			status = clock_mchp_get_status(dev, sys);
			if (status == CLOCK_CONTROL_STATUS_ON) {
				/* Successfully turned on clock. */
				ret_val = CLOCK_SUCCESS;
			}
		}

		if (ret_val == CLOCK_SUCCESS) {
			break;
		}

		if (on_timeout_ms < config->on_timeout_ms) {
			/* Thread is not available while booting. */
			if ((k_is_pre_kernel() == false) && (k_current_get() != NULL)) {
				/* Sleep before checking again. */
				k_sleep(K_MSEC(1));
				on_timeout_ms++;
			}
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
	const struct clock_mchp_config *config = dev->config;
	union clock_mchp_subsys subsys = {.val = (uint32_t)sys};

	/* Validate subsystem. */
	if (CLOCK_SUCCESS == clock_check_subsys(subsys)) {
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
	if (CLOCK_SUCCESS == clock_check_subsys(subsys)) {
		return clock_get_status(dev, sys);
	} else {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}
}

#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE
static int clock_mchp_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *freq)
{
	int ret_val = CLOCK_SUCCESS;
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;
	union clock_mchp_subsys subsys = {.val = (uint32_t)sys};
	uint8_t inst = subsys.bits.inst;

	uint8_t cpu_div;
	uint32_t gclkgen_src_freq = 0;
	enum clock_mchp_gclkgen gclkperiph_src;

	/* Validate subsystem. */
	if (CLOCK_SUCCESS != clock_check_subsys(subsys)) {
		return -ENOTSUP;
	}

	/* Return rate as 0, if clock is not on */
	if (clock_mchp_get_status(dev, sys) != CLOCK_CONTROL_STATUS_ON) {
		*freq = 0;
		return CLOCK_SUCCESS;
	}

	switch (subsys.bits.type) {
	case SUBSYS_TYPE_XOSC:
		*freq = data->xosc_crystal_freq;
		break;

	case SUBSYS_TYPE_OSC48M:
		ret_val = clock_get_rate_osc48m(dev, freq);
		break;

	case SUBSYS_TYPE_FDPLL:
		ret_val = clock_get_rate_fdpll(dev, freq);
		break;

	case SUBSYS_TYPE_RTC:
		ret_val = clock_get_rate_rtc(dev, freq);
		break;

	case SUBSYS_TYPE_XOSC32K:
		if (inst == INST_XOSC32K_XOSC1K) {
			*freq = FREQ_1KHZ;

		} else {
			*freq = FREQ_32KHZ;
		}
		break;

	case SUBSYS_TYPE_OSC32K:
		if (inst == INST_OSC32K_OSC1K) {
			*freq = FREQ_1KHZ;

		} else {
			*freq = FREQ_32KHZ;
		}
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

	case SUBSYS_TYPE_MCLKCPU:
	case SUBSYS_TYPE_MCLKPERIPH:
		/* source for mclk is always gclk0 */
		ret_val = clock_get_rate_gclkgen(dev, 0, CLOCK_MCHP_GCLK_SRC_MAX + 1,
						 &gclkgen_src_freq);
		if (ret_val == CLOCK_SUCCESS) {
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

#endif /* CONFIG_CLOCK_CONTROL_MCHP_GET_RATE */

#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_RUNTIME
static int clock_mchp_configure(const struct device *dev, clock_control_subsys_t sys,
				void *req_config)
{
	int ret_val = CLOCK_SUCCESS;
	const struct clock_mchp_config *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	union clock_mchp_subsys subsys;

	subsys.val = (uint32_t)sys;
	uint16_t inst = subsys.bits.inst;

	uint32_t val32 = 0;
	uint16_t val16 = 0;

	if (req_config == NULL) {
		return -EINVAL;
	}
	/* Validate subsystem. */
	if (CLOCK_SUCCESS != clock_check_subsys(subsys)) {
		return -ENOTSUP;
	}

	switch (subsys.bits.type) {
	case SUBSYS_TYPE_XOSC:
		struct clock_mchp_subsys_xosc_config *xosc_config =
			(struct clock_mchp_subsys_xosc_config *)req_config;
		val16 = oscctrl_regs->OSCCTRL_XOSCCTRL;
		val16 &= ~(OSCCTRL_XOSCCTRL_RUNSTDBY_Msk | OSCCTRL_XOSCCTRL_ONDEMAND_Msk);
		val16 |= ((xosc_config->run_in_standby_en != 0) ? OSCCTRL_XOSCCTRL_RUNSTDBY(1) : 0);
		val16 |= ((xosc_config->on_demand_en != 0) ? OSCCTRL_XOSCCTRL_ONDEMAND(1) : 0);
		oscctrl_regs->OSCCTRL_XOSCCTRL = val16;
		break;

	case SUBSYS_TYPE_OSC48M:
		clock_configure_osc48m(dev, req_config);
		break;

	case SUBSYS_TYPE_FDPLL:
		clock_configure_fdpll(dev, req_config);
		break;

	case SUBSYS_TYPE_RTC:
		struct clock_mchp_subsys_rtc_config *rtc_config =
			(struct clock_mchp_subsys_rtc_config *)req_config;
		osc32kctrl_regs->OSC32KCTRL_RTCCTRL = OSC32KCTRL_RTCCTRL_RTCSEL(rtc_config->src);
		break;

	case SUBSYS_TYPE_XOSC32K:
		struct clock_mchp_subsys_xosc32k_config *xosc32k_config =
			(struct clock_mchp_subsys_xosc32k_config *)req_config;

		val16 = osc32kctrl_regs->OSC32KCTRL_XOSC32K;
		val16 &= ~(OSC32KCTRL_XOSC32K_RUNSTDBY_Msk | OSC32KCTRL_XOSC32K_ONDEMAND_Msk);
		val16 |= ((xosc32k_config->run_in_standby_en != 0) ? OSC32KCTRL_XOSC32K_RUNSTDBY(1)
								   : 0);
		val16 |= ((xosc32k_config->on_demand_en != 0) ? OSC32KCTRL_XOSC32K_ONDEMAND(1) : 0);

		osc32kctrl_regs->OSC32KCTRL_XOSC32K = val16;
		break;

	case SUBSYS_TYPE_OSC32K:
		struct clock_mchp_subsys_osc32k_config *osc32k_config =
			(struct clock_mchp_subsys_osc32k_config *)req_config;

		val32 = osc32kctrl_regs->OSC32KCTRL_OSC32K;
		val32 &= ~(OSC32KCTRL_OSC32K_RUNSTDBY_Msk | OSC32KCTRL_OSC32K_ONDEMAND_Msk);
		val32 |= ((osc32k_config->run_in_standby_en != 0) ? OSC32KCTRL_OSC32K_RUNSTDBY(1)
								  : 0);
		val32 |= ((osc32k_config->on_demand_en != 0) ? OSC32KCTRL_OSC32K_ONDEMAND(1) : 0);
		osc32kctrl_regs->OSC32KCTRL_OSC32K = val32;
		break;

	case SUBSYS_TYPE_GCLKGEN:
		clock_configure_gclkgen(dev, inst, req_config);
		break;

	case SUBSYS_TYPE_GCLKPERIPH:
		struct clock_mchp_subsys_gclkperiph_config *gclkperiph_config =
			(struct clock_mchp_subsys_gclkperiph_config *)req_config;
		val32 = gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph];
		val32 &= ~GCLK_PCHCTRL_GEN_Msk;
		val32 |= GCLK_PCHCTRL_GEN(gclkperiph_config->src);
		gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph] = val32;
		break;

	case SUBSYS_TYPE_MCLKCPU:
		struct clock_mchp_subsys_mclkcpu_config *mclkcpu_config =
			(struct clock_mchp_subsys_mclkcpu_config *)req_config;
		config->mclk_regs->MCLK_CPUDIV =
			MCLK_CPUDIV_CPUDIV(mclkcpu_config->division_factor);
		break;

	default:
		ret_val = -ENOTSUP;
		break;
	}

	return ret_val;
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

	uint16_t val16;

	/* Check if xosc clock is already on */
	if ((data->fdpll_src_on_status & (1 << CLOCK_MCHP_FDPLL_SRC_XOSC)) == 0) {
		data->xosc_crystal_freq = xosc_init->frequency;

		/* XOSCCTRL */
		val16 = 0;
		val16 |= OSCCTRL_XOSCCTRL_STARTUP(xosc_init->startup_time);
		val16 |= ((xosc_init->automatic_amplitude_gain_control_en != 0)
				  ? OSCCTRL_XOSCCTRL_AMPGC(1)
				  : 0);
		/* Important: Initializing it with 1, along with clock enabled, can lead to
		 * indefinite wait for the clock to be on, if there is no peripheral request
		 * for the clock in the sequence of clock Initialization. If required,
		 * better to turn on the clock using API, instead of enabling both
		 * (on_demand_en & enable) during startup.
		 */
		val16 |= ((xosc_init->on_demand_en != 0) ? OSCCTRL_XOSCCTRL_ONDEMAND(1) : 0);
		val16 |= ((xosc_init->run_in_standby_en != 0) ? OSCCTRL_XOSCCTRL_RUNSTDBY(1) : 0);

		val16 |= ((xosc_init->clock_failure_detection_en != 0) ? OSCCTRL_XOSCCTRL_CFDEN(1)
								       : 0);
		val16 |= OSCCTRL_XOSCCTRL_GAIN(CLOCK_OSCCTRL_XOSCCTRL_GAIN_VALUE);

		val16 |= ((xosc_init->xtal_en != 0) ? OSCCTRL_XOSCCTRL_XTALEN(1) : 0);
		val16 |= ((xosc_init->enable != 0) ? OSCCTRL_XOSCCTRL_ENABLE(1) : 0);

		oscctrl_regs->OSCCTRL_XOSCCTRL = val16;
		if (xosc_init->enable != 0) {
			if (WAIT_FOR(((oscctrl_regs->OSCCTRL_STATUS & OSCCTRL_STATUS_XOSCRDY_Msk) !=
				      0),
				     TIMEOUT_XOSC_RDY, NULL) == false) {
				LOG_ERR("XOSC ready timed out");

			} else {
				/* Set xosc clock as on */
				data->fdpll_src_on_status |= 1 << CLOCK_MCHP_FDPLL_SRC_XOSC;
				data->gclkgen_src_on_status |= 1 << CLOCK_MCHP_GCLK_SRC_XOSC;
			}
		}
	}
}

/* initialize OSC48M from device tree node. */
void clock_osc48m_init(const struct device *dev, struct clock_osc48m_init *osc48m_init)
{
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;

	uint8_t val8;

	/* Check if osc48m clock is already on */
	if ((data->gclkgen_src_on_status & (1 << CLOCK_MCHP_GCLK_SRC_OSC48M)) != 0) {
		return;
	}

	/* To avoid changing osc48m, while gclk0 is driven by it. Else will affect CPU. */
	if (data->gclk0_src == CLOCK_MCHP_GCLK_SRC_OSC48M) {
		return;
	}

	/* OSC48MDIV */
	val8 = 0;
	val8 |= OSCCTRL_OSC48MDIV_DIV(osc48m_init->post_divider_freq);

	oscctrl_regs->OSCCTRL_OSC48MDIV = val8;
	if (WAIT_FOR((oscctrl_regs->OSCCTRL_OSC48MSYNCBUSY == 0), TIMEOUT_REG_SYNC, NULL) ==
	    false) {
		LOG_ERR("OSC48MSYNCBUSY timeout on writing OSCCTRL_OSC48MDIV");
		return;
	}

	/* OSC48MCTRL */
	val8 = 0;

	/* Important: Initializing it with 1, along with clock enabled, can lead to
	 * indefinite wait for the clock to be on, if there is no peripheral request
	 * for the clock in the sequence of clock Initialization. If required,
	 * better to turn on the clock using API, instead of enabling both
	 * (on_demand_en & enable) during startup.
	 */
	val8 |= ((osc48m_init->on_demand_en != 0) ? OSCCTRL_OSC48MCTRL_ONDEMAND(1) : 0);
	val8 |= ((osc48m_init->run_in_standby_en != 0) ? OSCCTRL_OSC48MCTRL_RUNSTDBY(1) : 0);
	val8 |= ((osc48m_init->enable != 0) ? OSCCTRL_OSC48MCTRL_ENABLE(1) : 0);

	oscctrl_regs->OSCCTRL_OSC48MCTRL = val8;
	if (osc48m_init->enable != 0) {
		if (WAIT_FOR(((oscctrl_regs->OSCCTRL_STATUS & OSCCTRL_STATUS_OSC48MRDY_Msk) != 0),
			     TIMEOUT_OSC48M_RDY, NULL) == false) {
			LOG_ERR("OSC48M ready timed out");
			return;
		}
		data->gclkgen_src_on_status |= (1 << CLOCK_MCHP_GCLK_SRC_OSC48M);
	}
}

/* initialize FDPLL from device tree node. */
void clock_fdpll_init(const struct device *dev, struct clock_fdpll_init *fdpll_init)
{
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;

	uint8_t val8;
	int src;
	uint32_t val32, mask;

	/* Check if fdpll clock is already on */
	if (data->gclkgen_src_on_status & (1 << (CLOCK_MCHP_GCLK_SRC_FDPLL))) {
		/* Early error handling return for code readability and maintainability */
		return;
	}

	/* Check if source clock of fdpll is off */
	src = fdpll_init->src;
	if ((data->fdpll_src_on_status & (1 << src)) == 0) {
		/* Early error handling return for code readability and maintainability */
		return;
	}

	/* program gclkph if source is gclk & enable */
	if (src <= CLOCK_MCHP_FDPLL_SRC_GCLK8) {
		gclk_regs->GCLK_PCHCTRL[0] |= (GCLK_PCHCTRL_GEN(src) | GCLK_PCHCTRL_CHEN_Msk);
		if (WAIT_FOR(((gclk_regs->GCLK_PCHCTRL[0] & GCLK_PCHCTRL_CHEN_Msk) != 0),
			     TIMEOUT_REG_SYNC, NULL) == false) {
			LOG_ERR("timeout on writing GCLK_PCHCTRL_CHEN_Msk");
			return;
		}
	}

	/* DPLLPRESC */
	val8 = oscctrl_regs->OSCCTRL_DPLLPRESC;
	val8 &= ~OSCCTRL_DPLLPRESC_PRESC_Msk;
	val8 |= OSCCTRL_DPLLPRESC_PRESC(fdpll_init->output_prescalar);
	oscctrl_regs->OSCCTRL_DPLLPRESC = val8;
	if (WAIT_FOR((oscctrl_regs->OSCCTRL_DPLLSYNCBUSY == 0), TIMEOUT_REG_SYNC, NULL) == false) {
		LOG_ERR("DPLLSYNCBUSY timeout on writing DPLLPRESC");
		return;
	}

	/* DPLLCTRLB */
	val32 = oscctrl_regs->OSCCTRL_DPLLCTRLB;
	val32 &= ~(OSCCTRL_DPLLCTRLB_DIV_Msk | OSCCTRL_DPLLCTRLB_LBYPASS_Msk |
		   OSCCTRL_DPLLCTRLB_REFCLK_Msk | OSCCTRL_DPLLCTRLB_WUF_Msk |
		   OSCCTRL_DPLLCTRLB_LPEN_Msk | OSCCTRL_DPLLCTRLB_FILTER_Msk);
	val32 |= OSCCTRL_DPLLCTRLB_DIV(fdpll_init->xosc_clock_divider);
	val32 |= ((fdpll_init->lock_bypass_en != 0) ? OSCCTRL_DPLLCTRLB_LBYPASS(1) : 0);
	if (src > CLOCK_MCHP_FDPLL_SRC_GCLK8) {
		val32 |= OSCCTRL_DPLLCTRLB_REFCLK((src == CLOCK_MCHP_FDPLL_SRC_XOSC32K)
							  ? OSCCTRL_DPLLCTRLB_REFCLK_XOSC32K_Val
							  : OSCCTRL_DPLLCTRLB_REFCLK_XOSC_Val);
	} else {
		val32 |= OSCCTRL_DPLLCTRLB_REFCLK(OSCCTRL_DPLLCTRLB_REFCLK_GCLK_Val);
	}
	val32 |= ((fdpll_init->wakeup_fast_en != 0) ? OSCCTRL_DPLLCTRLB_WUF(1) : 0);
	val32 |= ((fdpll_init->low_power_en != 0) ? OSCCTRL_DPLLCTRLB_LPEN(1) : 0);
	val32 |= OSCCTRL_DPLLCTRLB_FILTER(fdpll_init->pi_filter_type);
	oscctrl_regs->OSCCTRL_DPLLCTRLB = val32;

	/* DPLLRATIO */
	val32 = oscctrl_regs->OSCCTRL_DPLLRATIO;
	val32 &= ~(OSCCTRL_DPLLRATIO_LDRFRAC_Msk | OSCCTRL_DPLLRATIO_LDR_Msk);
	val32 |= OSCCTRL_DPLLRATIO_LDRFRAC(fdpll_init->divider_ratio_frac);
	val32 |= OSCCTRL_DPLLRATIO_LDR(fdpll_init->divider_ratio_int);
	oscctrl_regs->OSCCTRL_DPLLRATIO = val32;

	if (WAIT_FOR((oscctrl_regs->OSCCTRL_DPLLSYNCBUSY == 0), TIMEOUT_REG_SYNC, NULL) == false) {
		LOG_ERR("DPLLSYNCBUSY timeout on writing OSCCTRL_DPLLRATIO");
		return;
	}

	/* DPLLCTRLA */
	val8 = oscctrl_regs->OSCCTRL_DPLLCTRLA;
	val8 &= ~(OSCCTRL_DPLLCTRLA_ONDEMAND_Msk | OSCCTRL_DPLLCTRLA_RUNSTDBY_Msk |
		  OSCCTRL_DPLLCTRLA_ENABLE_Msk);

	/* Important: Initializing it with 1, along with clock enabled, can lead to
	 * indefinite wait for the clock to be on, if there is no peripheral request
	 * for the clock in the sequence of clock Initialization. If required,
	 * better to turn on the clock using API, instead of enabling both
	 * (on_demand_en & enable) during startup.
	 */
	val8 |= ((fdpll_init->on_demand_en != 0) ? OSCCTRL_DPLLCTRLA_ONDEMAND(1) : 0);
	val8 |= ((fdpll_init->run_in_standby_en != 0) ? OSCCTRL_DPLLCTRLA_RUNSTDBY(1) : 0);
	val8 |= ((fdpll_init->enable != 0) ? OSCCTRL_DPLLCTRLA_ENABLE(1) : 0);

	oscctrl_regs->OSCCTRL_DPLLCTRLA = val8;
	if (WAIT_FOR((oscctrl_regs->OSCCTRL_DPLLSYNCBUSY == 0), TIMEOUT_REG_SYNC, NULL) == false) {
		LOG_ERR("DPLLSYNCBUSY timeout on writing OSCCTRL_DPLLCTRLA");
		return;
	}
	if (fdpll_init->enable != 0) {
		mask = OSCCTRL_DPLLSTATUS_LOCK_Msk | OSCCTRL_DPLLSTATUS_CLKRDY_Msk;
		if (WAIT_FOR(((oscctrl_regs->OSCCTRL_DPLLSTATUS & mask) == mask),
			     TIMEOUT_FDPLL_LOCK_RDY, NULL) == false) {
			LOG_ERR("DPLL lock/ready timed out");
			return;
		}
		data->gclkgen_src_on_status |= (1 << CLOCK_MCHP_GCLK_SRC_FDPLL);
	}
}

/* initialize rtc clock source from device tree node. */
void clock_rtc_init(const struct device *dev, uint8_t rtc_src)
{
	const struct clock_mchp_config *config = dev->config;

	config->osc32kctrl_regs->OSC32KCTRL_RTCCTRL = OSC32KCTRL_RTCCTRL_RTCSEL(rtc_src);
}

/* initialize xosc32k clocks from device tree node. */
void clock_xosc32k_init(const struct device *dev, struct clock_xosc32k_init *xosc32k_init)
{
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;

	uint8_t val8;
	uint16_t val16;

	/* Check if xosc32k clock is already on */
	if ((data->gclkgen_src_on_status & (1 << CLOCK_MCHP_GCLK_SRC_XOSC32K)) == 0) {
		/* CFDCTRL */
		val8 = 0;
		val8 |= ((xosc32k_init->cfd_en != 0) ? OSC32KCTRL_CFDCTRL_CFDEN(1) : 0);

		osc32kctrl_regs->OSC32KCTRL_CFDCTRL = val8;

		/* XOSC32K */
		val16 = 0;
		val16 |= OSC32KCTRL_XOSC32K_STARTUP(xosc32k_init->startup_time);

		/* Important: Initializing it with 1, along with clock enabled, can lead to
		 * indefinite wait for the clock to be on, if there is no peripheral request
		 * for the clock in the sequence of clock Initialization. If required,
		 * better to turn on the clock using API, instead of enabling both
		 * (on_demand_en & enable) during startup.
		 */
		val16 |= ((xosc32k_init->on_demand_en != 0) ? OSC32KCTRL_XOSC32K_ONDEMAND(1) : 0);
		val16 |= ((xosc32k_init->run_in_standby_en != 0) ? OSC32KCTRL_XOSC32K_RUNSTDBY(1)
								 : 0);
		val16 |= ((xosc32k_init->xosc32k_1khz_en != 0) ? OSC32KCTRL_XOSC32K_EN1K(1) : 0);
		val16 |= ((xosc32k_init->xosc32k_32khz_en != 0) ? OSC32KCTRL_XOSC32K_EN32K(1) : 0);
		val16 |= ((xosc32k_init->xtal_en != 0) ? OSC32KCTRL_XOSC32K_XTALEN(1) : 0);
		val16 |= ((xosc32k_init->enable != 0) ? OSC32KCTRL_XOSC32K_ENABLE(1) : 0);

		osc32kctrl_regs->OSC32KCTRL_XOSC32K = val16;

		if (xosc32k_init->enable != 0) {
			if (WAIT_FOR(((osc32kctrl_regs->OSC32KCTRL_STATUS &
				       OSC32KCTRL_STATUS_XOSC32KRDY_Msk) != 0),
				     TIMEOUT_XOSC32KCTRL_RDY, NULL) == false) {
				LOG_ERR("XOSC32KCTRL ready timed out");
			} else {
				data->fdpll_src_on_status |= 1 << CLOCK_MCHP_FDPLL_SRC_XOSC32K;
				data->gclkgen_src_on_status |= 1 << CLOCK_MCHP_GCLK_SRC_XOSC32K;
			}
		}
	}
}

/* initialize osc32k clocks from device tree node. */
void clock_osc32k_init(const struct device *dev, struct clock_osc32k_init *osc32k_init)
{
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;
	osc32kctrl_registers_t *osc32kctrl_regs = config->osc32kctrl_regs;

	uint32_t val32;

	/* Check if osc32k clock is already on */
	if ((data->gclkgen_src_on_status & (1 << CLOCK_MCHP_GCLK_SRC_OSC32K)) == 0) {
		/* OSC32K */
		val32 = 0;
		val32 |= OSC32KCTRL_OSC32K_STARTUP(osc32k_init->startup_time);

		/* Important: Initializing it with 1, along with clock enabled, can lead to
		 * indefinite wait for the clock to be on, if there is no peripheral request
		 * for the clock in the sequence of clock Initialization. If required,
		 * better to turn on the clock using API, instead of enabling both
		 * (on_demand_en & enable) during startup.
		 */
		val32 |= ((osc32k_init->on_demand_en != 0) ? OSC32KCTRL_OSC32K_ONDEMAND(1) : 0);
		val32 |=
			((osc32k_init->run_in_standby_en != 0) ? OSC32KCTRL_OSC32K_RUNSTDBY(1) : 0);
		val32 |= ((osc32k_init->osc32k_1khz_en != 0) ? OSC32KCTRL_OSC32K_EN1K(1) : 0);
		val32 |= ((osc32k_init->osc32k_32khz_en != 0) ? OSC32KCTRL_OSC32K_EN32K(1) : 0);
		val32 |= ((osc32k_init->enable != 0) ? OSC32KCTRL_OSC32K_ENABLE(1) : 0);

		osc32kctrl_regs->OSC32KCTRL_OSC32K = val32;

		if (osc32k_init->enable != 0) {
			if ((osc32k_init->osc32k_32khz_en != 0) ||
			    (osc32k_init->osc32k_1khz_en != 0)) {
				if (WAIT_FOR(((osc32kctrl_regs->OSC32KCTRL_STATUS &
					       OSC32KCTRL_STATUS_OSC32KRDY_Msk) != 0),
					     TIMEOUT_OSC32KCTRL_RDY, NULL) == false) {
					LOG_ERR("OSC32KCTRL ready timed out");
					return;
				}
				data->gclkgen_src_on_status |= 1 << CLOCK_MCHP_GCLK_SRC_OSC32K;
			}
		}
	}
}

/* initialize gclk generator from device tree node. */
void clock_gclkgen_init(const struct device *dev, struct clock_gclkgen_init *gclkgen_init)
{
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;
	int inst = gclkgen_init->subsys.bits.inst;

	uint32_t val32;

	/* Check if gclkgen clock is already on */
	if ((data->fdpll_src_on_status & (1 << inst)) != 0) {
		/* Early error handling return for code readability and maintainability */
		return;
	}

	/* Check if source of gclk generator is off */
	if ((data->gclkgen_src_on_status & (1 << gclkgen_init->src)) == 0) {
		/* Early error handling return for code readability and maintainability */
		return;
	}

	if (inst <= GCLK_IO_MAX) {
		data->gclkpin_freq[inst] = gclkgen_init->pin_src_freq;
	}

	/* GENCTRL */
	val32 = 0;
	/* check range for div_factor, gclk1: 0 - 65535, others: 0 - 255 */
	if ((inst == 1) || (gclkgen_init->div_factor <= 0xFF)) {
		val32 |= GCLK_GENCTRL_DIV(gclkgen_init->div_factor);
	}
	val32 |= ((gclkgen_init->run_in_standby_en != 0) ? GCLK_GENCTRL_RUNSTDBY(1) : 0);
	if (gclkgen_init->div_select == 0) { /* div-factor */
		val32 |= GCLK_GENCTRL_DIVSEL(GCLK_GENCTRL_DIVSEL_DIV1_Val);
	} else { /* div-factor-power */
		val32 |= GCLK_GENCTRL_DIVSEL(GCLK_GENCTRL_DIVSEL_DIV2_Val);
	}
	val32 |= ((gclkgen_init->pin_output_en != 0) ? GCLK_GENCTRL_OE(1) : 0);
	val32 |= GCLK_GENCTRL_OOV(gclkgen_init->pin_output_off_val);
	val32 |= ((gclkgen_init->improve_duty_cycle_en != 0) ? GCLK_GENCTRL_IDC(1) : 0);
	val32 |= ((gclkgen_init->enable != 0) ? GCLK_GENCTRL_GENEN(1) : 0);
	val32 |= GCLK_GENCTRL_SRC(gclkgen_init->src);

	config->gclk_regs->GCLK_GENCTRL[inst] = val32;
	if (WAIT_FOR((config->gclk_regs->GCLK_SYNCBUSY == 0), TIMEOUT_REG_SYNC, NULL) == false) {
		LOG_ERR("GCLK_SYNCBUSY timeout on writing GCLK_GENCTRL[%d]", inst);
		return;
	}

	/* To avoid changing osc48m, while gclk0 is driven by it. Else will affect CPU */
	if (inst == CLOCK_MCHP_GCLKGEN_GEN0) {
		data->gclk0_src = gclkgen_init->src;
	}

	data->fdpll_src_on_status |= (1 << inst);
	if (inst == CLOCK_MCHP_GCLKGEN_GEN1) {
		data->gclkgen_src_on_status |= (1 << CLOCK_MCHP_GCLK_SRC_GCLKGEN1);
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
	val32 = 0;
	val32 |= ((enable != 0) ? GCLK_PCHCTRL_CHEN(1) : 0);
	val32 |= GCLK_PCHCTRL_GEN(pch_src);

	config->gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph] = val32;
}

/* initialize cpu mclk from device tree node. */
void clock_mclkcpu_init(const struct device *dev, uint8_t cpu_div)
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
	mask = 1 << subsys.bits.mclkmaskbit;
	mask_reg = get_mclkbus_mask_reg(config->mclk_regs, subsys.bits.mclkbus);

	if (mask_reg != NULL) {
		if (enable != 0) {
			*mask_reg |= mask;

		} else {
			*mask_reg &= ~mask;
		}
	}
}

#define CLOCK_MCHP_PROCESS_XOSC(node)                                                              \
	struct clock_xosc_init xosc_init = {0};                                                    \
	xosc_init.startup_time = DT_ENUM_IDX(node, xosc_startup_time);                             \
	xosc_init.automatic_amplitude_gain_control_en =                                            \
		DT_PROP(node, xosc_automatic_amplitude_gain_control_en);                           \
	xosc_init.on_demand_en = DT_PROP(node, xosc_on_demand_en);                                 \
	xosc_init.run_in_standby_en = DT_PROP(node, xosc_run_in_standby_en);                       \
	xosc_init.clock_failure_detection_en = DT_PROP(node, xosc_clock_failure_detection_en);     \
	xosc_init.xtal_en = DT_PROP(node, xosc_xtal_en);                                           \
	xosc_init.enable = DT_PROP(node, xosc_en);                                                 \
	xosc_init.frequency = DT_PROP(node, xosc_frequency);                                       \
	clock_xosc_init(dev, &xosc_init);

#define CLOCK_MCHP_PROCESS_OSC48M(node)                                                            \
	struct clock_osc48m_init osc48m_init = {0};                                                \
	osc48m_init.on_demand_en = DT_PROP(node, osc48m_on_demand_en);                             \
	osc48m_init.run_in_standby_en = DT_PROP(node, osc48m_run_in_standby_en);                   \
	osc48m_init.enable = DT_PROP(node, osc48m_en);                                             \
	osc48m_init.post_divider_freq = DT_ENUM_IDX(node, osc48m_post_divider_freq);               \
	clock_osc48m_init(dev, &osc48m_init);

#define CLOCK_MCHP_PROCESS_FDPLL(node)                                                             \
	struct clock_fdpll_init fdpll_init = {0};                                                  \
	fdpll_init.on_demand_en = DT_PROP(node, fdpll_on_demand_en);                               \
	fdpll_init.run_in_standby_en = DT_PROP(node, fdpll_run_in_standby_en);                     \
	fdpll_init.enable = DT_PROP(node, fdpll_en);                                               \
	fdpll_init.divider_ratio_frac = DT_PROP(node, fdpll_divider_ratio_frac);                   \
	fdpll_init.divider_ratio_int = DT_PROP(node, fdpll_divider_ratio_int);                     \
	fdpll_init.xosc_clock_divider = DT_PROP(node, fdpll_xosc_clock_divider);                   \
	fdpll_init.lock_bypass_en = DT_PROP(node, fdpll_lock_bypass_en);                           \
	fdpll_init.src = DT_ENUM_IDX(node, fdpll_src);                                             \
	fdpll_init.wakeup_fast_en = DT_PROP(node, fdpll_wakeup_fast_en);                           \
	fdpll_init.low_power_en = DT_PROP(node, fdpll_low_power_en);                               \
	fdpll_init.pi_filter_type = DT_ENUM_IDX(node, fdpll_pi_filter_type);                       \
	fdpll_init.output_prescalar = DT_ENUM_IDX(node, fdpll_output_prescalar);                   \
	clock_fdpll_init(dev, &fdpll_init);

#define CLOCK_MCHP_PROCESS_RTC(node) clock_rtc_init(dev, DT_ENUM_IDX(node, rtc_src));

#define CLOCK_MCHP_PROCESS_XOSC32K(node)                                                           \
	struct clock_xosc32k_init xosc32k_init = {0};                                              \
	xosc32k_init.startup_time = DT_ENUM_IDX(node, xosc32k_startup_time);                       \
	xosc32k_init.on_demand_en = DT_PROP(node, xosc32k_on_demand_en);                           \
	xosc32k_init.run_in_standby_en = DT_PROP(node, xosc32k_run_in_standby_en);                 \
	xosc32k_init.xosc32k_1khz_en = DT_PROP(node, xosc32k_1khz_en);                             \
	xosc32k_init.xosc32k_32khz_en = DT_PROP(node, xosc32k_32khz_en);                           \
	xosc32k_init.xtal_en = DT_PROP(node, xosc32k_xtal_en);                                     \
	xosc32k_init.enable = DT_PROP(node, xosc32k_en);                                           \
	xosc32k_init.cfd_en = DT_PROP(node, xosc32k_cfd_en);                                       \
	clock_xosc32k_init(dev, &xosc32k_init);

#define CLOCK_MCHP_PROCESS_OSC32K(node)                                                            \
	struct clock_osc32k_init osc32k_init = {0};                                                \
	osc32k_init.startup_time = DT_ENUM_IDX(node, osc32k_startup_time);                         \
	osc32k_init.on_demand_en = DT_PROP(node, osc32k_on_demand_en);                             \
	osc32k_init.run_in_standby_en = DT_PROP(node, osc32k_run_in_standby_en);                   \
	osc32k_init.osc32k_1khz_en = DT_PROP(node, osc32k_1khz_en);                                \
	osc32k_init.osc32k_32khz_en = DT_PROP(node, osc32k_32khz_en);                              \
	osc32k_init.enable = DT_PROP(node, osc32k_en);                                             \
	clock_osc32k_init(dev, &osc32k_init);

#define CLOCK_MCHP_ITERATE_GCLKGEN(child)                                                          \
	{                                                                                          \
		struct clock_gclkgen_init gclkgen_init = {0};                                      \
		gclkgen_init.subsys.val = DT_PROP(child, subsystem);                               \
		gclkgen_init.div_factor = DT_PROP(child, gclkgen_div_factor);                      \
		gclkgen_init.run_in_standby_en = DT_PROP(child, gclkgen_run_in_standby_en);        \
		gclkgen_init.div_select = DT_ENUM_IDX(child, gclkgen_div_select);                  \
		gclkgen_init.pin_output_en = DT_PROP(child, gclkgen_pin_output_en);                \
		gclkgen_init.pin_output_off_val = DT_ENUM_IDX(child, gclkgen_pin_output_off_val);  \
		gclkgen_init.improve_duty_cycle_en =                                               \
			DT_PROP(child, gclkgen_improve_duty_cycle_en);                             \
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

#define CLOCK_MCHP_PROCESS_MCLKCPU(node) clock_mclkcpu_init(dev, DT_PROP(node, mclk_cpu_div));

#define CLOCK_MCHP_ITERATE_MCLKPERIPH(child)                                                       \
	{                                                                                          \
		clock_mclkperiph_init(dev, DT_PROP(child, subsystem), DT_PROP(child, mclk_en));    \
	}

#endif /* CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP */

/* clock driver initialization function. */
static int clock_mchp_init(const struct device *dev)
{
	uint32_t val32;

#if CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP
	const struct clock_mchp_config *config = dev->config;
	struct clock_mchp_data *data = dev->data;

	/* Program flash wait states, before configuring clock frequencies */
	val32 = NVMCTRL_REGS->NVMCTRL_CTRLB;
	val32 &= ~NVMCTRL_CTRLB_RWS_Msk;
	val32 |= NVMCTRL_CTRLB_RWS(config->flash_wait_states);
	NVMCTRL_REGS->NVMCTRL_CTRLB = val32;

	/* iteration-1 */
	CLOCK_MCHP_PROCESS_OSC48M(DT_NODELABEL(osc48m));
	CLOCK_MCHP_PROCESS_XOSC(DT_NODELABEL(xosc));
	CLOCK_MCHP_PROCESS_XOSC32K(DT_NODELABEL(xosc32k));
	CLOCK_MCHP_PROCESS_OSC32K(DT_NODELABEL(osc32k));

	config->gclk_regs->GCLK_CTRLA = GCLK_CTRLA_SWRST(1);
	if (WAIT_FOR((config->gclk_regs->GCLK_SYNCBUSY == 0), TIMEOUT_REG_SYNC, NULL) == false) {
		LOG_ERR("GCLK_SYNCBUSY timeout on writing GCLK_CTRLA");
		return -ETIMEDOUT;
	}

	/* To avoid changing osc48m, while gclk0 is driven by it. Else will affect CPU */
	data->gclk0_src = CLOCK_MCHP_GCLK_SRC_OSC48M;

	for (int i = 0; i < CLOCK_INIT_ITERATION_COUNT; i++) {
		DT_FOREACH_CHILD(DT_NODELABEL(gclkgen), CLOCK_MCHP_ITERATE_GCLKGEN);
		CLOCK_MCHP_PROCESS_FDPLL(DT_NODELABEL(fdpll));
	}

	CLOCK_MCHP_PROCESS_RTC(DT_NODELABEL(rtcclock));
	DT_FOREACH_CHILD(DT_NODELABEL(gclkperiph), CLOCK_MCHP_ITERATE_GCLKPERIPH);
	DT_FOREACH_CHILD(DT_NODELABEL(mclkperiph), CLOCK_MCHP_ITERATE_MCLKPERIPH);

	CLOCK_MCHP_PROCESS_MCLKCPU(DT_NODELABEL(mclkcpu));
#endif /* CONFIG_CLOCK_CONTROL_MCHP_CONFIG_BOOTUP */

	/* Return CLOCK_SUCCESS indicating successful initialization. */
	return CLOCK_SUCCESS;
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
		.flash_wait_states = DT_PROP_OR(CLOCK_NODE, flash_wait_states, 3),                 \
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
