/*
 * Copyright (c) 2016, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file PWM driver for Freescale K64 FlexTimer Module (FTM)
 *
 * This file implements Pulse Width Modulation using the Freescale FlexTimer
 * Module (FTM).  Basic functionality is implemented using edge-aligned PWM
 * mode.  More complex functionality such as non-zero phase is not supported
 * since combined mode operation is not implemented.
 *
 * The following configuration options are supported. ("x" can be one of the
 * following values: 0, 1, 2, or 3 representing one of the four FMT modules
 * FTM0, FTM1, FTM2, or FTM3.)
 *
 * - CONFIG_PWM_K64_FTM_x_DEV_NAME: string representing the device name
 * - CONFIG_PWM_K64_FTM_x_REG_BASE: the base address of FTM (FTMx_SC)
 * - CONFIG_PWM_K64_FTM_x_PRESCALE: the clock prescaler value
 * - CONFIG_PWM_K64_FTM_x_CLOCK_SOURCE: the clock source
 * - CONFIG_PWM_K64_FTM_DEBUG: enable debug log output for the driver
 * - CONFIG_STDOUT_CONSOLE: choose debug logs using printf of printk
 *
 * The following configuration options are not supported.  These are place
 * holders for future functionality
 *
 * - CONFIG_PWM_K64_FTM_x_PHASE_ENABLE_0 support non-zero phase on channel 0
 * - CONFIG_PWM_K64_FTM_x_PHASE_ENABLE_1 support non-zero phase on channel 1
 * - CONFIG_PWM_K64_FTM_x_PHASE_ENABLE_2 support non-zero phase on channel 2
 * - CONFIG_PWM_K64_FTM_x_PHASE_ENABLE_3 support non-zero phase on channel 3
 */

#include <nanokernel.h>

#include <board.h>
#include <k20_sim.h>
#include <pwm.h>

#include "pwm_k64_ftm.h"
#include <stdio.h>

/*
 * Non-zero phase is not supported because combine mode is not yet
 * implemented.
 */
#undef COMBINE_MODE_SUPPORT

#ifndef CONFIG_PWM_K64_FTM_DEBUG
#define DBG(...) do { } while ((0))
#else /* CONFIG_PWM_K64_FTM_DEBUG */
#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define DBG printf
#else
#include <misc/printk.h>
#define DBG printk
#endif /* CONFIG_STDOUT_CONSOLE */
#endif /* CONFIG_PWM_K64_FTM_DEBUG */

/* Maximum PWM outputs */
#define MAX_PWM_OUT		8

/**
 * @brief Enable the clock for the FTM subsystem
 *
 * This function must be called before writing to FTM registers.  Failure to
 * do so may result in bus fault.
 *
 * @param ftm_num index indicating which FTM
 *
 * @return DEV_OK if successful, failed otherwise
 */

static int pwm_ftm_clk_enable(uint8_t ftm_num)
{

	volatile struct K20_SIM *sim =
		(volatile struct K20_SIM *)PERIPH_ADDR_BASE_SIM; /* sys integ. ctl */

	if (ftm_num > 3) {
		DBG("ERROR: Illegal FTM number (%d).\n"
		    "  Cannot enable PWM clock\n", ftm_num);
		return DEV_INVALID_CONF;
	}

	/* enabling the FTM by setting one of the bits SIM_SCGC6[26:24] */

	sim->scgc6 |= 1 << (24 + ftm_num);

	return DEV_OK;
}



/**
 * @brief Initial FTM configuration
 *
 * Initialize the FTM hardware based on configuration options.
 *
 * @param dev Device struct
 * @param access_op Access operation (pin or port)
 * @param channel The pwm channel number
 * @param flags Device flags (unused)
 *
 * @return DEV_OK if successful, failed otherwise
 */

static int pwm_ftm_configure(struct device *dev, int access_op,
			     uint32_t channel, int flags)
{
	int return_val = DEV_OK;

	uint32_t clock_source;
	uint32_t prescale;
	uint32_t polarity;

	uint32_t reg_val;


	DBG("pwm_ftm_configure...\n");

	const struct pwm_ftm_config * const config =
		dev->config->config_info;

	ARG_UNUSED(access_op);
	ARG_UNUSED(flags);

	/* enable the clock for the FTM subsystem */
	pwm_ftm_clk_enable(config->ftm_num);

	/*
	 * Initialize:
	 *  clock source = x (system, fixed, external) from config
	 *  prescaler divide-by x=(1,2,4,8,16,32,64,128) from config
	 *  free-running count-up
	 *  edge-aligned PWM mode
	 *  pair: independent outputs
	 *  polarity +
	 *  no interrupt
	 */

	/*
	 * PS[2:0] = prescale
	 * MOD = pulse width
	 */

	clock_source = (config->clock_source & 0x3) << PWM_K64_FTM_SC_CLKS_SHIFT;

	if (clock_source == 0) {
		DBG("Warning: no clock source. PWM is disabled\n");
	}


	switch (config->prescale) {
	case PWM_K64_FTM_PRESCALE_1:
		prescale = PWM_K64_FTM_SC_PS_D1;
		break;

	case PWM_K64_FTM_PRESCALE_2:
		prescale = PWM_K64_FTM_SC_PS_D2;
		break;

	case PWM_K64_FTM_PRESCALE_4:
		prescale = PWM_K64_FTM_SC_PS_D4;
		break;

	case PWM_K64_FTM_PRESCALE_8:
		prescale = PWM_K64_FTM_SC_PS_D8;
		break;

	case PWM_K64_FTM_PRESCALE_16:
		prescale = PWM_K64_FTM_SC_PS_D16;
		break;

	case PWM_K64_FTM_PRESCALE_32:
		prescale = PWM_K64_FTM_SC_PS_D32;
		break;

	case PWM_K64_FTM_PRESCALE_64:
		prescale = PWM_K64_FTM_SC_PS_D64;
		break;

	case PWM_K64_FTM_PRESCALE_128:
		prescale = PWM_K64_FTM_SC_PS_D128;
		break;

	default:
		/* Illegal prescale value. Default to 1. */
		prescale = PWM_K64_FTM_SC_PS_D1;
		return_val = DEV_INVALID_OP;
		break;
	}


#ifdef COMBINE_MODE_SUPPORT
	/* Enable FTMEN=1 and set outputs to initial value */
	mode_reg_val = sys_read32(PWM_K64_FTM_MODE(config->reg_base));
	mode_reg_val |= PWM_K64_FTM_MODE_FTMEN | PWM_K64_FTM_MODE_INIT;

	DBG("pwm_ftm_configure sys_write32(0x%08x, 0x%08x)..\n",
	    mode_reg_val, PWM_K64_FTM_MODE(config->reg_base));

	sys_write32(mode_reg_val, PWM_K64_FTM_MODE(config->reg_base));

	/* Enable enhanced synchronization */

	DBG("pwm_ftm_configure sys_write32(0x%08x, 0x%08x)..\n",
		PWM_K64_FTM_SYNCONF_SYNCMODE|PWM_K64_FTM_SYNCONF_CNTINC,
		PWM_K64_FTM_SYNCONF(config->reg_base));

	sys_write32(PWM_K64_FTM_SYNCONF_SYNCMODE|PWM_K64_FTM_SYNCONF_CNTINC,
				PWM_K64_FTM_SYNCONF(config->reg_base));

#endif /*COMBINE_MODE_SUPPORT*/

	/* Configure: PS | CLKS | up-counter | disable TOF intr */
	reg_val = prescale | clock_source;

	DBG("pwm_ftm_configure sys_write32(0x%08x, 0x%08x)..\n",
	    reg_val, PWM_K64_FTM_SC(config->reg_base));

	sys_write32(reg_val, PWM_K64_FTM_SC(config->reg_base));

	DBG("pwm_ftm_configure sys_write32(0x%08x, 0x%08x)..\n",
	    config->period, PWM_K64_FTM_MOD(config->reg_base));

	/* set MOD to max */
	sys_write32(config->period, PWM_K64_FTM_MOD(config->reg_base));

	/* set channel control to edge-aligned */
	reg_val = PWM_K64_FTM_CNSC_MSB | PWM_K64_FTM_CNSC_ELSB;

	DBG("pwm_ftm_configure sys_write32(0x%08x, 0x%08x)..\n",
	    reg_val, PWM_K64_FTM_CNSC(config->reg_base, channel));

	sys_write32(reg_val, PWM_K64_FTM_CNSC(config->reg_base, channel));

	DBG("pwm_ftm_configure sys_read32 4..\n");

	/* set polarity high for this channel */
	polarity = sys_read32(PWM_K64_FTM_POL(config->reg_base));
	polarity &= ~(1<<channel);

	DBG("pwm_ftm_configure sys_write32(0x%08x, 0x%08x)..\n",
	    polarity, PWM_K64_FTM_POL(config->reg_base));

	sys_write32(polarity, PWM_K64_FTM_POL(config->reg_base));

	return return_val;
}


/**
 * @brief API call to set the on/off timer values
 *
 * @param dev Device struct
 * @param access_op Access operation (pin or port)
 * @param channel The pwm channel number
 * @param on Timer count value for the start of the pulse on each cycle
 *           (must be 0)
 * @param off Timer count value for the end of the pulse.  After this, the
 *            signal will be off (low if positive polarity) for the rest of
 *            the cycle.
 *
 * @return DEV_OK if successful, failed otherwise
 */

static int pwm_ftm_set_values(struct device *dev, int access_op,
			      uint32_t channel, uint32_t on, uint32_t off)
{
	const struct pwm_ftm_config * const config =
		dev->config->config_info;
	struct pwm_ftm_drv_data * const drv_data =
		(struct pwm_ftm_drv_data * const)dev->driver_data;

	DBG("pwm_ftm_set_values (on=%d, off=%d)\n", on, off);

	uint32_t pwm_pair;
	uint32_t combine;

	switch (access_op) {
	case PWM_ACCESS_BY_PIN:
		break;
	case PWM_ACCESS_ALL:
		return DEV_INVALID_OP;
	default:
		return DEV_INVALID_OP;
	}

	/* If either ON and/or OFF > max ticks, treat PWM as 100%.
	 * If OFF value == 0, treat it as 0%.
	 * Otherwise, populate registers accordingly.
	 */

	if ((on >= config->period) || (off >= config->period)) {
		/* Fully on. Set to 100% */

		DBG("pwm_ftm_set_values sys_write32(0x%08x, 0x%08x)..\n",
		    config->period, PWM_K64_FTM_CNV(config->reg_base, channel));

		/* CnV = pulse width */
		sys_write32(config->period, PWM_K64_FTM_CNV(config->reg_base, channel));

	} else if (off == 0) {
		/* Fully off. Set to 0% */

		DBG("pwm_ftm_set_values sys_write32(0x%08x, 0x%08x)..\n",
		    0, PWM_K64_FTM_CNV(config->reg_base, channel));

		/* CnV = 0 */
		sys_write32(0, PWM_K64_FTM_CNV(config->reg_base, channel));

	} else {


		/* if on != 0 then set to combine mode and pwm must be even */
		if (on != 0) {

#ifdef COMBINE_MODE_SUPPORT
			/* TODO should verify that the other channel is not in
			 * use in non-combine mode
			 */


			/* If phase != 0 enable combine mode */
			if (channel % 2 != 0) {
				DBG("If Phase is non-zero pwm must be 0, 2, 4, 6.\n");
				return DEV_INVALID_CONF;
			}

			DBG("Note: Enabling phase on pwm%d therefore "
			     "pwm%d is not valid for output\n", channel, channel+1);

			pwm_pair = channel / 2;

			/* verify that the pair is configured for non-zero phase */
			switch (pwm_pair) {
			case 0:
				if (!config->phase_enable0) {
					DBG("Error: Phase capability must be enabled on FTM0\n");
					return DEV_INVALID_CONF;
				}
				break;

			case 1:
				if (!config->phase_enable2) {
					DBG("Error: Phase capability must be enabled on FTM2\n");
					return DEV_INVALID_CONF;
				}
				drv_data->phase[1] = on;
				break;

			case 2:
				if (!config->phase_enable4) {
					DBG("Error: Phase capability must be enabled on FTM4\n");
					return DEV_INVALID_CONF;
				}
				break;

			case 3:
				if (!config->phase_enable6) {
					DBG("Error: Phase capability must be enabled on FTM0\n");
					return DEV_INVALID_CONF;
				}
				break;

			default:
				return DEV_INVALID_CONF;
			}

			drv_data->phase[pwm_pair] = on;

			combine =
				sys_read32(PWM_K64_FTM_COMBINE(config->reg_base));
			combine |= 1 << (pwm_pair * 8);

			DBG("pwm_ftm_set_values sys_write32(0x%08x, 0x%08x)..\n",
			    combine, PWM_K64_FTM_COMBINE(config->reg_base));

			sys_write32(combine, PWM_K64_FTM_COMBINE(config->reg_base));

			DBG("pwm_ftm_set_values sys_write32(0x%08x, 0x%08x)..\n",
			    on, PWM_K64_FTM_CNV(config->reg_base, channel));

			/* set the on value */
			sys_write32(on, PWM_K64_FTM_CNV(config->reg_base, channel));

			DBG("pwm_ftm_set_values sys_write32(0x%08x, 0x%08x)..\n",
			    off, PWM_K64_FTM_CNV(config->reg_base, channel+1));

			/* set the off value */
			sys_write32(off, PWM_K64_FTM_CNV(config->reg_base, channel+1));
#else /*COMBINE_MODE_SUPPORT*/
			DBG("Error: \"on\" value must be zero. Phase is not supported\n");
			return DEV_INVALID_CONF;
#endif /*COMBINE_MODE_SUPPORT*/

		} else {

			/* zero phase.  No need to combine two channels. */

			if (channel % 2 != 0) {
				pwm_pair = (channel - 1) / 2;
			} else {
				pwm_pair = channel / 2;
			}

			drv_data->phase[pwm_pair] = 0;

			combine =
				sys_read32(PWM_K64_FTM_COMBINE(config->reg_base));
			combine &= ~(1 << (pwm_pair * 8));

			DBG("pwm_ftm_set_values sys_write32(0x%08x, 0x%08x)..\n",
			    combine, PWM_K64_FTM_COMBINE(config->reg_base));

			sys_write32(combine, PWM_K64_FTM_COMBINE(config->reg_base));

			/* set the off value */

			DBG("pwm_ftm_set_values sys_write32(0x%08x, 0x%08x)..\n",
			    off, PWM_K64_FTM_CNV(config->reg_base, channel));

			sys_write32(off, PWM_K64_FTM_CNV(config->reg_base, channel));
		}

	}

	DBG("pwm_ftm_set_values done.\n");

	return DEV_OK;
}

/**
 * @brief API call to set the duty cycle
 *
 * Duty cycle describes the percentage of time a signal is in the ON state.
 *
 * @param dev Device struct
 * @param access_op Access operation (pin or port)
 * @param channel The pwm channel number
 * @param duty Percentage of time signal is on (value between 0 and 100)
 *
 * @return DEV_OK if successful, failed otherwise
 */

static int pwm_ftm_set_duty_cycle(struct device *dev, int access_op,
				  uint32_t channel, uint8_t duty)
{
	uint32_t on, off;

	const struct pwm_ftm_config * const config =
		dev->config->config_info;
	struct pwm_ftm_drv_data * const drv_data =
		(struct pwm_ftm_drv_data * const)dev->driver_data;

	ARG_UNUSED(access_op);

	DBG("pwm_ftm_set_duty_cycle...\n");

	if (duty == 0) {
		/* Turn off PWM */
		on = 0;
		off = 0;
	} else if (duty >= 100) {
		/* Force PWM to be 100% */
		on = 0;
		off = config->period + 1;
	} else {

		on = 0;

		/*
		 * Set the "on" value to the phase offset if it was set by
		 * pwm_ftm_set_phase()
		 */

		switch (channel) {
		case 0:
			if (config->phase_enable0)
				on = drv_data->phase[0];
			break;

		case 2:
			if (config->phase_enable2)
				on = drv_data->phase[1];
			break;

		case 4:
			if (config->phase_enable4)
				on = drv_data->phase[2];
			break;

		case 6:
			if (config->phase_enable6)
				on = drv_data->phase[3];
			break;

		default:
			break;
		}


		/* Calculate the timer value for when to stop the pulse */

		off = on + config->period * duty / 100;

		DBG("pwm_ftm_set_duty_cycle on=%d, off=%d, "
		    "period=%d, duty=%d.\n",
		    on, off, config->period, duty);

		/* check for valid off value */
		if (off > config->period)
			return  DEV_INVALID_OP;
	}

	return pwm_ftm_set_values(dev, access_op, channel, on, off);

	DBG("pwm_ftm_set_duty_cycle done.\n");

}

/**
 * @brief API call to set the phase
 *
 * Phase describes number of clock ticks of delay before the start of the
 * pulse.  The maximum count of the FTM timer is 65536 so the phase value is
 * an integer from 0 to 65536.
 *
 * A non-zero phase value requires the timer pair to be set to combined mode
 * so the odd-numbered (n+1) channel is not available for output
 *
 * Note: non-zero phase is not supported in this implementation
 *
 * @param dev Device struct
 * @param access_op Access operation (pin or port)
 * @param channel The pwm channel number
 * @param phase Clock ticks of delay before start of the pulse (must be 0)
 *
 * @return DEV_OK if successful, failed otherwise
 */

static int pwm_ftm_set_phase(struct device *dev, int access_op,
			     uint32_t channel, uint8_t phase)
{

#ifdef COMBINE_MODE_SUPPORT
	const struct pwm_ftm_config * const config =
		dev->config->config_info;
	struct pwm_ftm_drv_data * const drv_data =
		(struct pwm_ftm_drv_data * const)dev->driver_data;

	ARG_UNUSED(access_op);

	DBG("pwm_ftm_set_phase...\n");

	if ((phase < 0) || (phase > config->period))
		return DEV_INVALID_OP;

	switch (channel) {
	case 0:
		if (!config->phase_enable0)
			return DEV_INVALID_OP;
		drv_data->phase[0] = phase;
		break;

	case 2:
		if (!config->phase_enable2)
			return DEV_INVALID_OP;
		drv_data->phase[1] = phase;
		break;

	case 4:
		if (!config->phase_enable4)
			return DEV_INVALID_OP;
		drv_data->phase[2] = phase;
		break;

	case 6:
		if (!config->phase_enable6)
			return DEV_INVALID_OP;
		drv_data->phase[3] = phase;
		break;

	default:
		/* channel must be 0, 2, 4, or 6 */
		return DEV_INVALID_OP;
	}

	DBG("pwm_ftm_set_phase done.\n");

	return DEV_OK;
#else /*COMBINE_MODE_SUPPORT*/

	ARG_UNUSED(dev);
	ARG_UNUSED(access_op);
	ARG_UNUSED(channel);
	ARG_UNUSED(phase);

	DBG("ERROR: non-zero phase is not supported.\n");

	return DEV_INVALID_OP;
#endif /*COMBINE_MODE_SUPPORT*/
}

/**
 * @brief API call to disable FTM
 *
 * This function simply sets the clock source to "no clock selected" thus
 * disabling the FTM
 *
 * @param dev Device struct
 *
 * @return DEV_OK if successful, failed otherwise
 */

static int pwm_ftm_suspend(struct device *dev)
{
	uint32_t reg_val;

	const struct pwm_ftm_config * const config =
		dev->config->config_info;

	DBG("pwm_ftm_suspend...\n");

	/* set clock source to "no clock selected" */

	reg_val = sys_read32(PWM_K64_FTM_SC(config->reg_base));

	reg_val &= ~PWM_K64_FTM_SC_CLKS_MASK;

	reg_val |= PWM_K64_FTM_SC_CLKS_DISABLE;

	sys_write32(reg_val, PWM_K64_FTM_SC(config->reg_base));

	DBG("pwm_ftm_suspend done.\n");

	return DEV_OK;

}

/**
 * @brief API call to reenable FTM
 *
 * This function simply sets the clock source to the configuration value with
 * the assumption that FTM was previously disabled by setting the clock source
 * to "no clock selected" due to a call to pwm_ftm_suspend.
 *
 * @param dev Device struct
 *
 * @return DEV_OK if successful, failed otherwise
 */

static int pwm_ftm_resume(struct device *dev)
{
	uint32_t clock_source;
	uint32_t reg_val;

	/* set clock source to config value */

	const struct pwm_ftm_config * const config =
		dev->config->config_info;

	DBG("pwm_ftm_resume...\n");

	clock_source = (config->clock_source << PWM_K64_FTM_SC_CLKS_SHIFT) &&
		PWM_K64_FTM_SC_CLKS_MASK;

	reg_val = sys_read32(PWM_K64_FTM_SC(config->reg_base));

	reg_val &= ~PWM_K64_FTM_SC_CLKS_MASK;

	reg_val |= clock_source;

	sys_write32(reg_val, PWM_K64_FTM_SC(config->reg_base));

	DBG("pwm_ftm_resume done.\n");

	return DEV_OK;
}

static struct pwm_driver_api pwm_ftm_drv_api_funcs = {
	.config = pwm_ftm_configure,
	.set_values = pwm_ftm_set_values,
	.set_duty_cycle = pwm_ftm_set_duty_cycle,
	.set_phase = pwm_ftm_set_phase,
	.suspend = pwm_ftm_suspend,
	.resume = pwm_ftm_resume,
};

/**
 * @brief Initialization function of FTM
 *
 * @param dev Device struct
 * @return DEV_OK if successful, failed otherwise.
 */
int pwm_ftm_init(struct device *dev)
{

	DBG("pwm_ftm_init...\n");

	dev->driver_api = &pwm_ftm_drv_api_funcs;

	return DEV_OK;
}

/* Initialization for PWM_K64_FTM_0 */
#ifdef CONFIG_PWM_K64_FTM_0
#include <device.h>
#include <init.h>

static struct pwm_ftm_config pwm_ftm_0_cfg = {
	.ftm_num       = 0,
	.reg_base      = CONFIG_PWM_K64_FTM_0_REG_BASE,
	.prescale      = CONFIG_PWM_K64_FTM_0_PRESCALE,
	.clock_source  = CONFIG_PWM_K64_FTM_0_CLOCK_SOURCE,

#ifdef CONFIG_PWM_K64_FTM_0_PHASE_ENABLE_0
	.phase_enable0 = 1,
#else
	.phase_enable0 = 0,
#endif

#ifdef CONFIG_PWM_K64_FTM_0_PHASE_ENABLE_2
	.phase_enable2 = 1,
#else
	.phase_enable2 = 0,
#endif

#ifdef CONFIG_PWM_K64_FTM_0_PHASE_ENABLE_4
	.phase_enable4 = 1,
#else
	.phase_enable4 = 0,
#endif

#ifdef CONFIG_PWM_K64_FTM_0_PHASE_ENABLE_6
	.phase_enable6 = 1,
#else
	.phase_enable6 = 0,
#endif

	.period = CONFIG_PWM_K64_FTM_0_PERIOD,
};

static struct pwm_ftm_drv_data pwm_ftm_0_drvdata;

DEVICE_INIT(pwm_ftm_0, CONFIG_PWM_K64_FTM_0_DEV_NAME, pwm_ftm_init,
			&pwm_ftm_0_drvdata, &pwm_ftm_0_cfg,
			SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_PWM_K64_FTM_0 */

/* Initialization for PWM_K64_FTM_1 */
#ifdef CONFIG_PWM_K64_FTM_1
#include <device.h>
#include <init.h>

static struct pwm_ftm_config pwm_ftm_1_cfg = {
	.ftm_num       = 1,
	.reg_base      = CONFIG_PWM_K64_FTM_1_REG_BASE,
	.prescale      = CONFIG_PWM_K64_FTM_1_PRESCALE,
	.clock_source  = CONFIG_PWM_K64_FTM_1_CLOCK_SOURCE,

#ifdef CONFIG_PWM_K64_FTM_1_PHASE_ENABLE_0
	.phase_enable0 = 1,
#else
	.phase_enable0 = 0,
#endif

#ifdef CONFIG_PWM_K64_FTM_1_PHASE_ENABLE_2
	.phase_enable2 = 1,
#else
	.phase_enable2 = 0,
#endif

#ifdef CONFIG_PWM_K64_FTM_1_PHASE_ENABLE_4
	.phase_enable4 = 1,
#else
	.phase_enable4 = 0,
#endif

#ifdef CONFIG_PWM_K64_FTM_1_PHASE_ENABLE_6
	.phase_enable6 = 1,
#else
	.phase_enable6 = 0,
#endif

};

static struct pwm_ftm_drv_data pwm_ftm_1_drvdata;

DEVICE_INIT(pwm_ftm_1, CONFIG_PWM_K64_FTM_1_DEV_NAME, pwm_ftm_init,
			&pwm_ftm_1_drvdata, &pwm_ftm_1_cfg,
			SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_PWM_K64_FTM_1 */


/* Initialization for PWM_K64_FTM_2 */
#ifdef CONFIG_PWM_K64_FTM_2
#include <device.h>
#include <init.h>

static struct pwm_ftm_config pwm_ftm_2_cfg = {
	.ftm_num       = 2,
	.reg_base      = CONFIG_PWM_K64_FTM_2_REG_BASE,
	.prescale      = CONFIG_PWM_K64_FTM_2_PRESCALE,
	.clock_source  = CONFIG_PWM_K64_FTM_2_CLOCK_SOURCE,

#ifdef CONFIG_PWM_K64_FTM_2_PHASE_ENABLE_0
	.phase_enable0 = 1,
#else
	.phase_enable0 = 0,
#endif

#ifdef CONFIG_PWM_K64_FTM_2_PHASE_ENABLE_2
	.phase_enable2 = 1,
#else
	.phase_enable2 = 0,
#endif

#ifdef CONFIG_PWM_K64_FTM_2_PHASE_ENABLE_4
	.phase_enable4 = 1,
#else
	.phase_enable4 = 0,
#endif

#ifdef CONFIG_PWM_K64_FTM_2_PHASE_ENABLE_6
	.phase_enable6 = 1,
#else
	.phase_enable6 = 0,
#endif

};

static struct pwm_ftm_drv_data pwm_ftm_2_drvdata;

DEVICE_INIT(pwm_ftm_2, CONFIG_PWM_K64_FTM_2_DEV_NAME, pwm_ftm_init,
			&pwm_ftm_2_drvdata, &pwm_ftm_2_cfg,
			SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_PWM_K64_FTM_2 */


/* Initialization for PWM_K64_FTM_3 */
#ifdef CONFIG_PWM_K64_FTM_3
#include <device.h>
#include <init.h>

static struct pwm_ftm_config pwm_ftm_3_cfg = {
	.ftm_num       = 3,
	.reg_base      = CONFIG_PWM_K64_FTM_3_REG_BASE,
	.prescale      = CONFIG_PWM_K64_FTM_3_PRESCALE,
	.clock_source  = CONFIG_PWM_K64_FTM_3_CLOCK_SOURCE,

#ifdef CONFIG_PWM_K64_FTM_3_PHASE_ENABLE_0
	.phase_enable0 = 1,
#else
	.phase_enable0 = 0,
#endif

#ifdef CONFIG_PWM_K64_FTM_3_PHASE_ENABLE_2
	.phase_enable2 = 1,
#else
	.phase_enable2 = 0,
#endif

#ifdef CONFIG_PWM_K64_FTM_3_PHASE_ENABLE_4
	.phase_enable4 = 1,
#else
	.phase_enable4 = 0,
#endif

#ifdef CONFIG_PWM_K64_FTM_3_PHASE_ENABLE_6
	.phase_enable6 = 1,
#else
	.phase_enable6 = 0,
#endif

};

static struct pwm_ftm_drv_data pwm_ftm_3_drvdata;

DEVICE_INIT(pwm_ftm_3, CONFIG_PWM_K64_FTM_3_DEV_NAME, pwm_ftm_init,
			&pwm_ftm_3_drvdata, &pwm_ftm_3_cfg,
			SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_PWM_K64_FTM_3 */
