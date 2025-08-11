/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file clock_mchp_sam_d5x_e5x.c
 * @brief Clock control driver for sam_d5x_e5x family devices.
 */

#include <stdbool.h>
#include <string.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/clock_control/mchp_clock_control.h>

/******************************************************************************
 * @brief Devicetree definitions
 *****************************************************************************/
#define DT_DRV_COMPAT microchip_sam_d5x_e5x_clock

/******************************************************************************
 * @brief Macro definitions
 *****************************************************************************/
LOG_MODULE_REGISTER(clock_mchp_sam_d5x_e5x, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define CLOCK_NODE DT_NODELABEL(clock)

#define CLOCK_SUCCESS 0

#define FREQ_32KHZ      32768
#define FREQ_1KHZ       1024
#define FREQ_DFLL_48MHZ 48000000

/* gclk peripheral channel max, 0 - 47 */
#define GPH_MAX 47

/* maximum value for mask bit position, 0 - 31 */
#define MMASK_MAX 31

/* maximum value for div, when div_select is clock source frequency divided by 2^(N+1) */
#define GCLKGEN_POWER_DIV_MAX 29

/* init iteration count, so that, the source clocks are initialized before executing init */
#define CLOCK_INIT_ITERATION_COUNT 3

/* mclkbus Not Applicable for a clock subsystem ID */
#define MBUS_NA (0x3f)

/* mclkmaskbit Not Applicable for a clock subsystem ID */
#define MMASK_NA (0x3f)

/* gclkperiph Not Applicable for a clock subsystem ID */
#define GPH_NA (0x3f)

/* Clock subsystem Types */
#define SUBSYS_TYPE_XOSC       (0)
#define SUBSYS_TYPE_DFLL       (1)
#define SUBSYS_TYPE_FDPLL      (2)
#define SUBSYS_TYPE_RTC        (3)
#define SUBSYS_TYPE_OSC32K     (4)
#define SUBSYS_TYPE_GCLKGEN    (5)
#define SUBSYS_TYPE_GCLKPERIPH (6)
#define SUBSYS_TYPE_MCLKCPU    (7)
#define SUBSYS_TYPE_MCLKPERIPH (8)
#define SUBSYS_TYPE_MAX        (8)

/* mclk bus */
#define MBUS_AHB  (0)
#define MBUS_APBA (1)
#define MBUS_APBB (2)
#define MBUS_APBC (3)
#define MBUS_APBD (4)
#define MBUS_MAX  (4)

/* XOSC instances */
#define INST_XOSC0 0
#define INST_XOSC1 1

/* FDPLL instances */
#define INST_FDPLL0 0
#define INST_FDPLL1 1

/* OSC32K instances */
#define INST_OSC32K_OSCULP1K  0
#define INST_OSC32K_OSCULP32K 1
#define INST_OSC32K_XOSC1K    2
#define INST_OSC32K_XOSC32K   3

/******************************************************************************
 * @brief Data type definitions
 *****************************************************************************/
/** @brief Clock subsystem definition.
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
typedef union {
	uint32_t val;
	struct {
		uint32_t inst: 8;
		uint32_t gclkperiph: 6;
		uint32_t mclkmaskbit: 6;
		uint32_t mclkbus: 6;
		uint32_t type: 6;
	} bits;
} clock_mchp_subsys_t;

/** @brief clock driver configuration structure. */
typedef struct {
	oscctrl_registers_t *oscctrl_regs;
	osc32kctrl_registers_t *osc32kctrl_regs;
	gclk_registers_t *gclk_regs;
	mclk_registers_t *mclk_regs;

	/* Timeout in milliseconds to wait for clock to turn on */
	uint32_t on_timeout_ms;
} clock_mchp_config_t;

/*
 * - 00..14 (15 bits): clock_mchp_gclkgen_t/clock_mchp_fdpll_src_clock_t
 * - 15..23 (9 bits): CLOCK_MCHP_FDPLL_SRC_MAX+1+clock_mchp_gclk_src_clock_t
 */
typedef enum {
	ON_BITPOS_GCLK0 = CLOCK_MCHP_FDPLL_SRC_GCLK0,
	ON_BITPOS_GCLK1 = CLOCK_MCHP_FDPLL_SRC_GCLK1,
	ON_BITPOS_GCLK2 = CLOCK_MCHP_FDPLL_SRC_GCLK2,
	ON_BITPOS_GCLK3 = CLOCK_MCHP_FDPLL_SRC_GCLK3,
	ON_BITPOS_GCLK4 = CLOCK_MCHP_FDPLL_SRC_GCLK4,
	ON_BITPOS_GCLK5 = CLOCK_MCHP_FDPLL_SRC_GCLK5,
	ON_BITPOS_GCLK6 = CLOCK_MCHP_FDPLL_SRC_GCLK6,
	ON_BITPOS_GCLK7 = CLOCK_MCHP_FDPLL_SRC_GCLK7,
	ON_BITPOS_GCLK8 = CLOCK_MCHP_FDPLL_SRC_GCLK8,
	ON_BITPOS_GCLK9 = CLOCK_MCHP_FDPLL_SRC_GCLK9,
	ON_BITPOS_GCLK10 = CLOCK_MCHP_FDPLL_SRC_GCLK10,
	ON_BITPOS_GCLK11 = CLOCK_MCHP_FDPLL_SRC_GCLK11,
	ON_BITPOS_XOSC32K_ = CLOCK_MCHP_FDPLL_SRC_XOSC32K,
	ON_BITPOS_XOSC0_ = CLOCK_MCHP_FDPLL_SRC_XOSC0,
	ON_BITPOS_XOSC1_ = CLOCK_MCHP_FDPLL_SRC_XOSC1,
	ON_BITPOS_XOSC0 = CLOCK_MCHP_FDPLL_SRC_MAX + 1 + CLOCK_MCHP_GCLK_SRC_XOSC0,
	ON_BITPOS_XOSC1 = CLOCK_MCHP_FDPLL_SRC_MAX + 1 + CLOCK_MCHP_GCLK_SRC_XOSC1,
	ON_BITPOS_GCLKPIN = CLOCK_MCHP_FDPLL_SRC_MAX + 1 + CLOCK_MCHP_GCLK_SRC_GCLKPIN,
	ON_BITPOS_GCLKGEN1 = CLOCK_MCHP_FDPLL_SRC_MAX + 1 + CLOCK_MCHP_GCLK_SRC_GCLKGEN1,
	ON_BITPOS_OSCULP32K = CLOCK_MCHP_FDPLL_SRC_MAX + 1 + CLOCK_MCHP_GCLK_SRC_OSCULP32K,
	ON_BITPOS_XOSC32K = CLOCK_MCHP_FDPLL_SRC_MAX + 1 + CLOCK_MCHP_GCLK_SRC_XOSC32K,
	ON_BITPOS_DFLL = CLOCK_MCHP_FDPLL_SRC_MAX + 1 + CLOCK_MCHP_GCLK_SRC_DFLL,
	ON_BITPOS_FDPLL0 = CLOCK_MCHP_FDPLL_SRC_MAX + 1 + CLOCK_MCHP_GCLK_SRC_FDPLL0,
	ON_BITPOS_FDPLL1 = CLOCK_MCHP_FDPLL_SRC_MAX + 1 + CLOCK_MCHP_GCLK_SRC_FDPLL1
} clock_mchp_on_bitpos_t;

/** @brief clock driver data structure. */
typedef struct {
	/*
	 * - 00..14 (15 bits): clock_mchp_gclkgen_t/clock_mchp_fdpll_src_clock_t
	 * - 15..23 (9 bits): CLOCK_MCHP_FDPLL_SRC_MAX+1+clock_mchp_gclk_src_clock_t
	 */
	uint32_t src_on_status;
} clock_mchp_data_t;

/******************************************************************************
 * @brief Function forward declarations
 *****************************************************************************/
#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE
static int clock_get_rate_dfll(const struct device *dev, uint32_t *freq);
#endif /* CONFIG_CLOCK_CONTROL_MCHP_GET_RATE */

static enum clock_control_status clock_mchp_get_status(const struct device *dev,
						       clock_control_subsys_t sys);

/******************************************************************************
 * @brief Helper functions
 *****************************************************************************/
/**
 * @brief check if subsystem type and id are valid.
 */
static int clock_check_subsys(clock_mchp_subsys_t subsys)
{
	int ret_val = -EINVAL;
	uint32_t inst_max = 0, gclkperiph_max = GPH_NA, mclkbus_max = MBUS_NA,
		 mclkmaskbit_max = MMASK_NA;

	do {
		/* Check if turning on all clocks is requested. */
		if (subsys.val == (uint32_t)CLOCK_CONTROL_SUBSYS_ALL) {
			break;
		}

		/* Check if the specified subsystem was found. */
		if (subsys.bits.type > SUBSYS_TYPE_MAX) {
			break;
		}

		switch (subsys.bits.type) {
		case SUBSYS_TYPE_XOSC:
			inst_max = CLOCK_MCHP_XOSC_ID_MAX;
			break;

		case SUBSYS_TYPE_DFLL:
			inst_max = CLOCK_MCHP_DFLL_MAX;
			gclkperiph_max = CLOCK_MCHP_DFLL_MAX;
			break;

		case SUBSYS_TYPE_FDPLL:
			inst_max = CLOCK_MCHP_FDPLL_ID_MAX;
			gclkperiph_max = CLOCK_MCHP_FDPLL_ID_MAX;
			break;

		case SUBSYS_TYPE_RTC:
			inst_max = CLOCK_MCHP_RTC_ID_MAX;
			break;

		case SUBSYS_TYPE_OSC32K:
			inst_max = CLOCK_MCHP_OSC32K_ID_MAX;
			break;

		case SUBSYS_TYPE_GCLKGEN:
			inst_max = CLOCK_MCHP_GCLKGEN_ID_MAX;
			break;

		case SUBSYS_TYPE_GCLKPERIPH:
			inst_max = CLOCK_MCHP_GCLKPERIPH_ID_MAX;
			gclkperiph_max = GPH_MAX;
			break;

		case SUBSYS_TYPE_MCLKCPU:
			inst_max = CLOCK_MCHP_MCLKCPU_MAX;
			break;

		case SUBSYS_TYPE_MCLKPERIPH:
			inst_max = CLOCK_MCHP_MCLKPERIPH_ID_MAX;
			mclkbus_max = MBUS_MAX;
			mclkmaskbit_max = MMASK_MAX;
			break;

		default:
			LOG_ERR("Unsupported SUBSYS_TYPE");
		}

		/* Check if the specified id is valid. */
		if ((subsys.bits.inst > inst_max) || (subsys.bits.gclkperiph > gclkperiph_max) ||
		    (subsys.bits.mclkbus > mclkbus_max) ||
		    (subsys.bits.mclkmaskbit > mclkmaskbit_max)) {
			break;
		}

		ret_val = CLOCK_SUCCESS;
	} while (0);

	return ret_val;
}

/**
 * @brief get the address of mclk mask register.
 */
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

/**
 * @brief function to set/clear clock subsystem enable bit.
 */
static int clock_on_off(const clock_mchp_config_t *config, const clock_mchp_subsys_t subsys,
			bool on)
{
	int ret_val = CLOCK_SUCCESS;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	__IO uint32_t *reg32 = NULL;
	uint32_t reg32_val = 0;

	switch (subsys.bits.type) {
	case SUBSYS_TYPE_GCLKPERIPH:
		reg32 = &gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph];
		reg32_val = GCLK_PCHCTRL_CHEN_Msk;
		break;
	case SUBSYS_TYPE_MCLKPERIPH:
		reg32 = get_mclkbus_mask_reg(config->mclk_regs, subsys.bits.mclkbus);
		reg32_val = 1 << subsys.bits.mclkmaskbit;
		break;
	default:
		ret_val = -ENOTSUP;
		break;
	}

	if ((ret_val == CLOCK_SUCCESS) && (reg32 != NULL)) {
		if (on == true) {
			*reg32 |= reg32_val;
		} else {
			*reg32 &= ~reg32_val;
		}
	}

	return ret_val;
}

#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE
/**
 * @brief get rate of gclk generator in Hz.
 */
static int clock_get_rate_gclkgen(const struct device *dev, clock_mchp_gclkgen_t gclkgen_id,
				  clock_mchp_gclk_src_clock_t gclkgen_called_src, uint32_t *freq)
{
	int ret_val = CLOCK_SUCCESS;
	const clock_mchp_config_t *config = dev->config;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	clock_mchp_gclk_src_clock_t gclkgen_src;
	uint32_t gclkgen_src_freq;
	uint16_t gclkgen_div;

	bool power_div = (((gclk_regs->GCLK_GENCTRL[gclkgen_id] & GCLK_GENCTRL_DIVSEL_Msk) >>
			   GCLK_GENCTRL_DIVSEL_Pos) == GCLK_GENCTRL_DIVSEL_DIV1_Val)
				 ? false
				 : true;

	do {
		/* Return rate as 0, if clock is not on */
		if (clock_mchp_get_status(dev, (clock_control_subsys_t)MCHP_CLOCK_DERIVE_ID(
						       SUBSYS_TYPE_GCLKGEN, MBUS_NA, MMASK_NA,
						       GPH_NA, gclkgen_id)) !=
		    CLOCK_CONTROL_STATUS_ON) {
			*freq = 0;
			break;
		}

		/* get source for gclk generator from gclkgen registers */
		gclkgen_src = (gclk_regs->GCLK_GENCTRL[gclkgen_id] & GCLK_GENCTRL_SRC_Msk) >>
			      GCLK_GENCTRL_SRC_Pos;
		if (gclkgen_called_src == gclkgen_src) {
			ret_val = -ENOTSUP;
			break;
		}

		if (gclkgen_src == CLOCK_MCHP_GCLK_SRC_DFLL) {
			ret_val = clock_get_rate_dfll(dev, &gclkgen_src_freq);
		} else {
			ret_val = -ENOTSUP;
		}
		if (ret_val != CLOCK_SUCCESS) {
			break;
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
		} else {
			/* if DIV value is 0, has same effect as DIV value 1 */
			if (gclkgen_div == 0) {
				gclkgen_div = 1;
			}
		}
		*freq = gclkgen_src_freq / gclkgen_div;
	} while (0);

	return ret_val;
}

/**
 * @brief get rate of DFLL in Hz.
 */
static int clock_get_rate_dfll(const struct device *dev, uint32_t *freq)
{
	int ret_val = CLOCK_SUCCESS;
	const clock_mchp_config_t *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;

	if ((oscctrl_regs->OSCCTRL_STATUS & OSCCTRL_STATUS_DFLLRDY_Msk) == 0) {
		/* Return rate as 0, if clock is not on */
		*freq = 0;
	} else if ((oscctrl_regs->OSCCTRL_DFLLCTRLB & OSCCTRL_DFLLCTRLB_MODE_Msk) == 0) {
		/* in open loop mode*/
		*freq = FREQ_DFLL_48MHZ;
	} else {
		/* in closed loop mode*/
		ret_val = -ENOTSUP;
	}

	return ret_val;
}
#endif /* CONFIG_CLOCK_CONTROL_MCHP_GET_RATE */

/******************************************************************************
 * @brief API functions
 *****************************************************************************/

/**
 * @brief Turn on the clock for a specified subsystem, can be blocking.
 *
 * @param dev Pointer to the clock device structure
 * @param sys Clock subsystem
 *
 * @return 0 if the clock is successfully turned on
 * @return -ENOTSUP If the requested operation is not supported.
 * @return -ETIMEDOUT If the requested operation is timedout.
 * @return -EALREADY If clock is already on.
 */
static int clock_mchp_on(const struct device *dev, clock_control_subsys_t sys)
{
	int ret_val = -ENOTSUP;
	const clock_mchp_config_t *config = dev->config;
	clock_mchp_subsys_t subsys = {.val = (uint32_t)sys};
	enum clock_control_status status;
	bool is_wait = false;
	uint32_t on_timeout_ms = 0;

	do {
		/* Validate subsystem. */
		if (CLOCK_SUCCESS != clock_check_subsys(subsys)) {
			break;
		}

		status = clock_mchp_get_status(dev, sys);
		if (status == CLOCK_CONTROL_STATUS_ON) {
			/* clock is already on. */
			ret_val = -EALREADY;
			break;
		}

		/* Check if the clock on operation is successful. */
		if (clock_on_off(config, subsys, true) == CLOCK_SUCCESS) {
			is_wait = true;
		}
	} while (0);

	/* Wait until the clock state becomes ON. */
	while (is_wait == true) {
		status = clock_mchp_get_status(dev, sys);
		if (status == CLOCK_CONTROL_STATUS_ON) {
			/* Successfully turned on clock. */
			ret_val = CLOCK_SUCCESS;
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

/**
 * @brief Turn off the clock for a specified subsystem.
 *
 * @param dev Pointer to the clock device structure
 * @param sys Clock subsystem
 *
 * @return 0 if the clock is successfully turned off
 * @return -ENOTSUP If the requested operation is not supported.
 * @return -EALREADY If clock is already off.
 */
static int clock_mchp_off(const struct device *dev, clock_control_subsys_t sys)
{
	int ret_val = -ENOTSUP;
	const clock_mchp_config_t *config = dev->config;
	clock_mchp_subsys_t subsys = {.val = (uint32_t)sys};

	do {
		/* Validate subsystem. */
		if (CLOCK_SUCCESS != clock_check_subsys(subsys)) {
			break;
		}

		ret_val = clock_on_off(config, subsys, false);
	} while (0);

	return ret_val;
}

/**
 * @brief Get the status of clock, for a specified subsystem.
 *
 * This function retrieves the current status of the clock for a given subsystem.
 *
 * @param dev Pointer to the clock device structure.
 * @param sys The clock subsystem.
 *
 * @return The current status of clock for the subsystem (e.g., off, on, starting, or unknown).
 */
static enum clock_control_status clock_mchp_get_status(const struct device *dev,
						       clock_control_subsys_t sys)
{
	enum clock_control_status ret_status = CLOCK_CONTROL_STATUS_UNKNOWN;
	const clock_mchp_config_t *config = dev->config;
	oscctrl_registers_t *oscctrl_regs = config->oscctrl_regs;
	gclk_registers_t *gclk_regs = config->gclk_regs;
	clock_mchp_subsys_t subsys = {.val = (uint32_t)sys};
	uint32_t mask;
	uint8_t inst;
	__IO uint32_t *reg32;

	do {
		/* Validate subsystem. */
		if (CLOCK_SUCCESS != clock_check_subsys(subsys)) {
			break;
		}

		inst = subsys.bits.inst;

		switch (subsys.bits.type) {
		case SUBSYS_TYPE_DFLL:
			/* Check if DFLL is enabled */
			if ((oscctrl_regs->OSCCTRL_DFLLCTRLA & OSCCTRL_DFLLCTRLA_ENABLE_Msk) != 0) {
				/* Check if sync is complete and ready bit is set */
				ret_status = ((oscctrl_regs->OSCCTRL_DFLLSYNC != 0) ||
					      ((oscctrl_regs->OSCCTRL_STATUS &
						OSCCTRL_STATUS_DFLLRDY_Msk) == 0))
						     ? CLOCK_CONTROL_STATUS_STARTING
						     : CLOCK_CONTROL_STATUS_ON;
			} else {
				ret_status = CLOCK_CONTROL_STATUS_OFF;
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
			ret_status = ((*reg32 & mask) != 0) ? CLOCK_CONTROL_STATUS_ON
							    : CLOCK_CONTROL_STATUS_OFF;

			break;
		default:
			break;
		}

	} while (0);

	/* Return the status of the clock for the specified subsystem. */
	return ret_status;
}

#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE
/**
 * @brief Get the rate of the clock for a specified subsystem.
 *
 * This function retrieves the clock frequency (Hz) for the given subsystem.
 *
 * @param dev Pointer to clock device structure.
 * @param sys The clock subsystem.
 * @param frequency Pointer to store the retrieved clock rate.
 *
 * @return 0 if the rate is successfully retrieved.
 * @return -ENOTSUP If the requested operation is not supported.
 */
static int clock_mchp_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *freq)
{
	int ret_val = CLOCK_SUCCESS;
	const clock_mchp_config_t *config = dev->config;
	clock_mchp_subsys_t subsys = {.val = (uint32_t)sys};
	uint8_t cpu_div;
	uint32_t gclkgen_src_freq;
	clock_mchp_gclkgen_t gclkperiph_src;

	do {
		/* Validate subsystem. */
		if (CLOCK_SUCCESS != clock_check_subsys(subsys)) {
			ret_val = -ENOTSUP;
			break;
		}

		/* Return rate as 0, if clock is not on */
		if (clock_mchp_get_status(dev, sys) != CLOCK_CONTROL_STATUS_ON) {
			*freq = 0;
			break;
		}

		switch (subsys.bits.type) {
		case SUBSYS_TYPE_GCLKPERIPH:
			gclkperiph_src = (config->gclk_regs->GCLK_PCHCTRL[subsys.bits.gclkperiph] &
					  GCLK_PCHCTRL_GEN_Msk) >>
					 GCLK_PCHCTRL_GEN_Pos;
			ret_val = clock_get_rate_gclkgen(dev, gclkperiph_src,
							 CLOCK_MCHP_GCLK_SRC_MAX + 1, freq);
			break;

		case SUBSYS_TYPE_MCLKCPU:
		case SUBSYS_TYPE_MCLKPERIPH:
			/* source for mclk is always gclk0 */
			ret_val = clock_get_rate_gclkgen(dev, 0, CLOCK_MCHP_GCLK_SRC_MAX + 1,
							 &gclkgen_src_freq);
			if (ret_val == CLOCK_SUCCESS) {
				cpu_div = (config->mclk_regs->MCLK_CPUDIV & MCLK_CPUDIV_DIV_Msk) >>
					  MCLK_CPUDIV_DIV_Pos;
				if (cpu_div != 0) {
					*freq = gclkgen_src_freq / cpu_div;
				}
			}
			break;
		default:
			ret_val = -ENOTSUP;
			break;
		}

	} while (0);

	return ret_val;
}

#endif /* CONFIG_CLOCK_CONTROL_MCHP_GET_RATE */

/**
 * @brief clock driver initialization function.
 */
static int clock_mchp_init(const struct device *dev)
{
	/* Return CLOCK_SUCCESS indicating successful initialization. */
	return CLOCK_SUCCESS;
}

/******************************************************************************
 * @brief Zephyr driver instance creation
 *****************************************************************************/
static DEVICE_API(clock_control, clock_mchp_driver_api) = {
	.on = clock_mchp_on,
	.off = clock_mchp_off,
	.get_status = clock_mchp_get_status,
#if CONFIG_CLOCK_CONTROL_MCHP_GET_RATE
	.get_rate = clock_mchp_get_rate,
#endif /* CONFIG_CLOCK_CONTROL_MCHP_GET_RATE */
};

#define CLOCK_MCHP_CONFIG_DEFN()                                                                   \
	static const clock_mchp_config_t clock_mchp_config = {                                     \
		.on_timeout_ms = DT_PROP_OR(CLOCK_NODE, on_timeout_ms, 5),                         \
		.mclk_regs = (mclk_registers_t *)DT_REG_ADDR_BY_NAME(CLOCK_NODE, mclk),            \
		.oscctrl_regs = (oscctrl_registers_t *)DT_REG_ADDR_BY_NAME(CLOCK_NODE, oscctrl),   \
		.osc32kctrl_regs =                                                                 \
			(osc32kctrl_registers_t *)DT_REG_ADDR_BY_NAME(CLOCK_NODE, osc32kctrl),     \
		.gclk_regs = (gclk_registers_t *)DT_REG_ADDR_BY_NAME(CLOCK_NODE, gclk)}

#define CLOCK_MCHP_DATA_DEFN() static clock_mchp_data_t clock_mchp_data;

#define CLOCK_MCHP_DEVICE_INIT(n)                                                                  \
	CLOCK_MCHP_CONFIG_DEFN();                                                                  \
	CLOCK_MCHP_DATA_DEFN();                                                                    \
	DEVICE_DT_INST_DEFINE(n, clock_mchp_init, NULL, &clock_mchp_data, &clock_mchp_config,      \
			      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY,                    \
			      &clock_mchp_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CLOCK_MCHP_DEVICE_INIT)
