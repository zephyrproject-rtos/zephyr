/*
 * Copyright (c) 2022 Microchip Technololgy Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_pwmbbled

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#ifdef CONFIG_SOC_SERIES_MEC172X
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#endif
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include <soc.h>

LOG_MODULE_REGISTER(pwmbbled_mchp_xec, CONFIG_PWM_LOG_LEVEL);

#define XEC_PWM_BBLED_MAX_FREQ_DIV	256U

/* We will choose frequency from Device Tree */
#define XEC_PWM_BBLED_INPUT_FREQ_HI	48000000
#define XEC_PWM_BBLED_INPUT_FREQ_LO	32768

/* Hardware blink mode equation is Fpwm = Fin / (256 * (LD + 1))
 * The maximum Fpwm is actually Fin / 256
 * LD in [0, 4095]
 */
#define XEC_PWM_BBLED_MAX_PWM_FREQ_HI	(XEC_PWM_BBLED_INPUT_FREQ_HI / \
		XEC_PWM_BBLED_MAX_FREQ_DIV)
#define XEC_PWM_BBLED_MAX_PWM_FREQ_LO	(XEC_PWM_BBLED_INPUT_FREQ_LO / \
		XEC_PWM_BBLED_MAX_FREQ_DIV)
#define XEC_PWM_BBLED_LD_MAX 4095
#define XEC_PWM_BBLED_DC_MIN 1u /* 0 full off */
#define XEC_PWM_BBLED_DC_MAX 254u /* 255 is full on */

/* BBLED PWM mode uses the duty cycle to set the PWM frequency:
 * Fpwm = Fclock / (256 * (LD + 1)) OR
 * Tpwm = (256 * (LD + 1)) / Fclock
 * Fclock is 48MHz or 32KHz
 * LD = Delay register, LOW_DELAY field: bits[11:0]
 * Pulse_ON_width = (1/Fpwm) * (duty_cycle/256) seconds
 * Puse_OFF_width = (1/Fpwm) * (256 - duty_cycle) seconds
 * where duty_cycle is an 8-bit value 0 to 255.
 * Prescale is derived from DELAY register LOW_DELAY 12-bit field
 * Duty cycle is derived from LIMITS register MINIMUM 8-bit field
 *
 * Fc in Hz, Tp in seconds
 * Fc / Fp = 256 * (LD+1)
 * Tp / Tc = 256 * (LD+1)
 *
 * API passes pulse period and pulse width in nanoseconds.
 * BBLED PWM mode duty cycle specified by 8-bit MIN field of the LIMITS register
 * MIN=0 is OFF, pin driven low
 * MIN=255 is ON, pin driven high
 */

/* Same BBLED hardware block in MEC15xx and MEC172x families
 * Config register
 */
#define XEC_PWM_BBLED_CFG_MSK			0x1ffffu
#define XEC_PWM_BBLED_CFG_MODE_POS		0
#define XEC_PWM_BBLED_CFG_MODE_MSK		0x3u
#define XEC_PWM_BBLED_CFG_MODE_OFF		0
#define XEC_PWM_BBLED_CFG_MODE_PWM		0x2u
#define XEC_PWM_BBLED_CFG_MODE_ALWAYS_ON	0x3u
#define XEC_PWM_BBLED_CFG_CLK_SRC_48M_POS	2
#define XEC_PWM_BBLED_CFG_EN_UPDATE_POS		6
#define XEC_PWM_BBLED_CFG_RST_PWM_POS		7
#define XEC_PWM_BBLED_CFG_WDT_RLD_POS		8
#define XEC_PWM_BBLED_CFG_WDT_RLD_MSK0		0xffu
#define XEC_PWM_BBLED_CFG_WDT_RLD_MSK		0xff00u
#define XEC_PWM_BBLED_CFG_WDT_RLD_DFLT		0x1400u

/* Limits register */
#define XEC_PWM_BBLED_LIM_MSK			0xffffu
#define XEC_PWM_BBLED_LIM_MIN_POS		0
#define XEC_PWM_BBLED_LIM_MIN_MSK		0xffu
#define XEC_PWM_BBLED_LIM_MAX_POS		8
#define XEC_PWM_BBLED_LIM_MAX_MSK		0xff00u

/* Delay register */
#define XEC_PWM_BBLED_DLY_MSK			0xffffffu
#define XEC_PWM_BBLED_DLY_LO_POS		0
#define XEC_PWM_BBLED_DLY_LO_MSK		0xfffu
#define XEC_PWM_BBLED_DLY_HI_POS		12
#define XEC_PWM_BBLED_DLY_HI_MSK		0xfff000u

/* Output delay in clocks for initial enable and enable on resume from sleep
 * Clocks are either 48MHz or 32KHz selected in CONFIG register.
 */
#define XEC_PWM_BBLED_OUT_DLY_MSK		0xffu

/* DT enum values */
#define XEC_PWM_BBLED_CLKSEL_32K	0
#define XEC_PWM_BBLED_CLKSEL_AHB_48M	1

#define XEC_PWM_BBLED_CLKSEL_0		XEC_PWM_BBLED_CLKSEL_32K
#define XEC_PWM_BBLED_CLKSEL_1		XEC_PWM_BBLED_CLKSEL_AHB_48M


struct bbled_regs {
	volatile uint32_t config;
	volatile uint32_t limits;
	volatile uint32_t delay;
	volatile uint32_t update_step_size;
	volatile uint32_t update_interval;
	volatile uint32_t output_delay;
};

#define XEC_PWM_BBLED_CLK_SEL_48M	0
#define XEC_PWM_BBLED_CLK_SEL_32K	1

struct pwm_bbled_xec_config {
	struct bbled_regs * const regs;
	const struct pinctrl_dev_config *pcfg;
	uint8_t girq;
	uint8_t girq_pos;
	uint8_t pcr_idx;
	uint8_t pcr_pos;
	uint8_t clk_sel;
	bool enable_low_power_32K;
};

struct bbled_xec_data {
	uint32_t config;
};

/* Issue: two separate registers must be updated.
 * LIMITS.MIN = duty cycle = [1, 254]
 * LIMITS register update takes effect immediately.
 * DELAY.LO = pre-scaler = [0, 4095]
 * Writing DELAY stores value in an internal holding register.
 * Writing bit[6]=1 causes HW to update DELAY at the beginning of
 * the next HW PWM period.
 */
static void xec_pwmbb_progam_pwm(const struct device *dev, uint32_t ld, uint32_t dc)
{
	const struct pwm_bbled_xec_config * const cfg = dev->config;
	struct bbled_regs * const regs = cfg->regs;
	uint32_t val;

	val = regs->limits & ~(XEC_PWM_BBLED_LIM_MIN_MSK);
	val |= ((dc << XEC_PWM_BBLED_LIM_MIN_POS) & XEC_PWM_BBLED_LIM_MIN_MSK);
	regs->limits = val;

	val = regs->delay & ~(XEC_PWM_BBLED_DLY_LO_MSK);
	val |= ((ld  << XEC_PWM_BBLED_DLY_LO_POS) & XEC_PWM_BBLED_DLY_LO_MSK);
	regs->delay = val;

	 /* transfer new delay value from holding register */
	regs->config |= BIT(XEC_PWM_BBLED_CFG_EN_UPDATE_POS);

	val = regs->config & ~(XEC_PWM_BBLED_CFG_MODE_MSK);
	val |= XEC_PWM_BBLED_CFG_MODE_PWM;
	regs->config = val;
}

/* API implementation: Get the clock rate (cycles per second) for a single PWM output.
 * BBLED in PWM mode (same as blink mode) PWM frequency = Source Frequency / (256 * (LP + 1))
 * where Source Frequency is either 48 MHz or 32768 Hz and LP is the 12-bit low delay
 * field of the DELAY register. We return the maximum PWM frequency which is configured
 * hardware input frequency (32K or 48M) divided by 256.
 */
static int pwm_bbled_xec_get_cycles_per_sec(const struct device *dev,
					    uint32_t channel, uint64_t *cycles)
{
	const struct pwm_bbled_xec_config * const cfg = dev->config;
	struct bbled_regs * const regs = cfg->regs;

	if (channel > 0) {
		return -EIO;
	}

	if (cycles) {
		if (regs->config & BIT(XEC_PWM_BBLED_CFG_CLK_SRC_48M_POS)) {
			*cycles = XEC_PWM_BBLED_MAX_PWM_FREQ_HI; /* 187,500 Hz */
		} else {
			*cycles = XEC_PWM_BBLED_MAX_PWM_FREQ_LO; /* 128 Hz */
		}
	}

	return 0;
}

/* API PWM set cycles:
 * pulse == 0 -> pin should be constant inactive level
 * pulse >= period -> pin should be constant active level
 * hardware PWM (blink) mode: Fpwm = Fin_actual / (LD + 1)
 * Fin_actual = XEC_PWM_BBLED_MAX_PWM_FREQ_HI or XEC_PWM_BBLED_MAX_PWM_FREQ_LO.
 *  period cycles and pulse cycles both zero is OFF
 *  pulse cycles == 0 is OFF
 *  pulse cycles > 0 and period cycles == 0 is OFF
 *  otherwise
 *    compute duty cycle = 256 * (pulse_cycles / period_cycles).
 *    compute (LD + 1) = Fin_actual / Fpwm
 *    program LD into bits[11:0] of Delay register
 *    program duty cycle info bits[7:0] of Limits register
 * NOTE: flags parameter is currently used for pin invert and PWM capture.
 * The BBLED HW does not support pin invert or PWM capture.
 * NOTE 2: Pin invert is possible by using the MCHP function invert feature
 * of the GPIO pin. This property can be set using PINCTRL at build time.
 */
static int pwm_bbled_xec_set_cycles(const struct device *dev, uint32_t channel,
				    uint32_t period_cycles, uint32_t pulse_cycles,
				    pwm_flags_t flags)
{
	const struct pwm_bbled_xec_config * const cfg = dev->config;
	struct bbled_regs * const regs = cfg->regs;
	uint32_t dc, ld;

	if (channel > 0) {
		LOG_ERR("Invalid channel: %u", channel);
		return -EIO;
	}

	if (flags) {
		return -ENOTSUP;
	}

	LOG_DBG("period_cycles = %u  pulse_cycles = %u", period_cycles, pulse_cycles);

	if (pulse_cycles == 0u) {
		/* drive pin to inactive state */
		regs->config = (regs->config & ~XEC_PWM_BBLED_CFG_MODE_MSK)
			       | XEC_PWM_BBLED_CFG_MODE_OFF;
		regs->limits &= ~XEC_PWM_BBLED_LIM_MIN_MSK;
		regs->delay &= ~(XEC_PWM_BBLED_DLY_LO_MSK);
	} else if (pulse_cycles >= period_cycles) {
		/* drive pin to active state */
		regs->config = (regs->config & ~XEC_PWM_BBLED_CFG_MODE_MSK)
			       | XEC_PWM_BBLED_CFG_MODE_ALWAYS_ON;
		regs->limits &= ~XEC_PWM_BBLED_LIM_MIN_MSK;
		regs->delay &= ~(XEC_PWM_BBLED_DLY_LO_MSK);
	} else {
		ld = period_cycles;
		if (ld) {
			ld--;
			if (ld > XEC_PWM_BBLED_LD_MAX) {
				ld = XEC_PWM_BBLED_LD_MAX;
			}
		}

		dc = ((XEC_PWM_BBLED_DC_MAX + 1) * pulse_cycles / period_cycles);
		if (dc < XEC_PWM_BBLED_DC_MIN) {
			dc = XEC_PWM_BBLED_DC_MIN;
		} else if (dc > XEC_PWM_BBLED_DC_MAX) {
			dc = XEC_PWM_BBLED_DC_MAX;
		}

		LOG_DBG("Program: ld = 0x%0x  dc = 0x%0x", ld, dc);

		xec_pwmbb_progam_pwm(dev, ld, dc);
	}

	return 0;
}


#ifdef CONFIG_PM_DEVICE
static int pwm_bbled_xec_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct pwm_bbled_xec_config *const devcfg = dev->config;
	struct bbled_regs * const regs = devcfg->regs;
	struct bbled_xec_data * const data = dev->data;
	int ret = 0;

	/* 32K core clock is not gated by PCR in sleep, so BBLED can blink the LED even
	 * in sleep, if it is configured to use 32K clock. If we want to control it
	 * we shall use flag "enable_low_power_32K".
	 * This flag dont have effect on 48M clock. Since it is gated by PCR in sleep, BBLED
	 * will not get clock during sleep.
	 */
	if ((!devcfg->enable_low_power_32K) &&
			(!(regs->config & BIT(XEC_PWM_BBLED_CFG_CLK_SRC_48M_POS)))) {
		return ret;
	}

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_DEFAULT);
		if (ret != 0) {
			LOG_ERR("XEC BBLED pinctrl setup failed (%d)", ret);
		}

		/* Turn on BBLED only if it is ON before sleep */
		if ((data->config & XEC_PWM_BBLED_CFG_MODE_MSK) != XEC_PWM_BBLED_CFG_MODE_OFF) {

			regs->config |= (data->config & XEC_PWM_BBLED_CFG_MODE_MSK);
			regs->config |= BIT(XEC_PWM_BBLED_CFG_EN_UPDATE_POS);

			data->config = XEC_PWM_BBLED_CFG_MODE_OFF;
		}
	break;
	case PM_DEVICE_ACTION_SUSPEND:
		if ((regs->config & XEC_PWM_BBLED_CFG_MODE_MSK) != XEC_PWM_BBLED_CFG_MODE_OFF) {
			/* Do copy first, then clear mode. */
			data->config = regs->config;

			regs->config &= ~(XEC_PWM_BBLED_CFG_MODE_MSK);
		}

		ret = pinctrl_apply_state(devcfg->pcfg, PINCTRL_STATE_SLEEP);
		/* pinctrl-1 does not exist. */
		if (ret == -ENOENT) {
			ret = 0;
		}
	break;
	default:
	ret = -ENOTSUP;
	}
	return ret;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(pwm, pwm_bbled_xec_driver_api) = {
	.set_cycles = pwm_bbled_xec_set_cycles,
	.get_cycles_per_sec = pwm_bbled_xec_get_cycles_per_sec,
};

static int pwm_bbled_xec_init(const struct device *dev)
{
	const struct pwm_bbled_xec_config * const cfg = dev->config;
	struct bbled_regs * const regs = cfg->regs;
	int ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);

	if (ret != 0) {
		LOG_ERR("XEC PWM-BBLED pinctrl init failed (%d)", ret);
		return ret;
	}

	/* BBLED PWM WDT is enabled by default. Disable it and select 32KHz */
	regs->config = BIT(XEC_PWM_BBLED_CFG_RST_PWM_POS);
	regs->config = 0U;
	if (cfg->clk_sel == XEC_PWM_BBLED_CLKSEL_AHB_48M) {
		regs->config |= BIT(XEC_PWM_BBLED_CFG_CLK_SRC_48M_POS);
	}

	return 0;
}

#define XEC_PWM_BBLED_CLKSEL(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, clock_select),			\
		    (DT_INST_ENUM_IDX(n, clock_select)), (0))

#define XEC_PWM_BBLED_CONFIG(inst)						\
	static struct pwm_bbled_xec_config pwm_bbled_xec_config_##inst = {	\
		.regs = (struct bbled_regs * const)DT_INST_REG_ADDR(inst),	\
		.girq = (uint8_t)(DT_INST_PROP_BY_IDX(0, girqs, 0)),		\
		.girq_pos = (uint8_t)(DT_INST_PROP_BY_IDX(0, girqs, 1)),	\
		.pcr_idx = (uint8_t)DT_INST_PROP_BY_IDX(inst, pcrs, 0),		\
		.pcr_pos = (uint8_t)DT_INST_PROP_BY_IDX(inst, pcrs, 1),		\
		.clk_sel = UTIL_CAT(XEC_PWM_BBLED_CLKSEL_, XEC_PWM_BBLED_CLKSEL(inst)),	\
		.enable_low_power_32K = DT_INST_PROP(inst, enable_low_power_32k),\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
	};

#define XEC_PWM_BBLED_DEVICE_INIT(index)				\
									\
	static struct bbled_xec_data bbled_xec_data_##index;	\
									\
	PINCTRL_DT_INST_DEFINE(index);					\
									\
	XEC_PWM_BBLED_CONFIG(index);					\
									\
	PM_DEVICE_DT_INST_DEFINE(index, pwm_bbled_xec_pm_action);	\
									\
	DEVICE_DT_INST_DEFINE(index, &pwm_bbled_xec_init,		\
			      PM_DEVICE_DT_INST_GET(index),		\
			      &bbled_xec_data_##index,			\
			      &pwm_bbled_xec_config_##index, POST_KERNEL,	\
			      CONFIG_PWM_INIT_PRIORITY,			\
			      &pwm_bbled_xec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XEC_PWM_BBLED_DEVICE_INIT)
