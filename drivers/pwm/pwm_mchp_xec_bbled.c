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

#include <soc.h>

LOG_MODULE_REGISTER(pwmbbled_mchp_xec, CONFIG_PWM_LOG_LEVEL);

/* We will choose frequency from Device Tree */
#define XEC_PWM_BBLED_INPUT_FREQ_HI	48000000
#define XEC_PWM_BBLED_INPUT_FREQ_LO	32768

#define XEC_PWM_BBLED_MAX_FREQ_DIV	256U
#define XEC_PWM_BBLED_MIN_FREQ_DIV	(256U * 4066U)

/* Maximum frequency BBLED-PWM can generate is scaled by
 * 256 * (LD+1) where LD is in [0, 4065].
 */
#define XEC_PWM_BBLED_MAX_PWM_FREQ_AHB_CLK	\
	(XEC_PWM_BBLED_INPUT_FREQ_HI / XEC_PWM_BBLED_MAX_FREQ_DIV)
#define XEC_PWM_BBLED_MAX_PWM_FREQ_32K_CLK	\
	(XEC_PWM_BBLED_INPUT_FREQ_LO / XEC_PWM_BBLED_MAX_FREQ_DIV)

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
#define XEC_PWM_BBLED_CLKSEL_PCR_SLOW	1
#define XEC_PWM_BBLED_CLKSEL_AHB_48M	2

#define XEC_PWM_BBLED_CLKSEL_0		XEC_PWM_BBLED_CLKSEL_32K
#define XEC_PWM_BBLED_CLKSEL_1		XEC_PWM_BBLED_CLKSEL_PCR_SLOW
#define XEC_PWM_BBLED_CLKSEL_2		XEC_PWM_BBLED_CLKSEL_AHB_48M

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
	uint8_t girq;
	uint8_t girq_pos;
	uint8_t pcr_idx;
	uint8_t pcr_pos;
	uint8_t clk_sel;
	const struct pinctrl_dev_config *pcfg;
};

/* Compute BBLED PWM delay factor to produce requested frequency.
 * Fpwm = Fclk / (256 * (LD+1)) where Fclk is 48MHz or 32KHz and
 * LD is a 12-bit value in [0, 4096].
 * We expect 256 <= pulse_cycles <= (256 * 4096)
 * period_cycles = (period * cycles_per_sec) / NSEC_PER_SEC;
 * period_cycles = (Tpwm * Fclk) = Fclk / Fpwm
 * period_cycles = Fclk * (256 * (LD+1)) / Fclk = (256 * (LD+1))
 * (LD+1) = period_cycles / 256
 */
static uint32_t xec_pwmbb_compute_ld(const struct device *dev, uint32_t period_cycles)
{
	uint32_t ld = 0;

	ld = period_cycles / 256U;
	if (ld > 0) {
		if (ld > 4096U) {
			ld = 4096U;
		}
		ld--;
	}

	return ld;
}

/* BBLED-PWM duty cycle set in 8-bit MINIMUM field of BBLED LIMITS register.
 * Limits.Minimum == 0 (alwyas off, output driven low)
 *                == 255 (always on, output driven high)
 * 1 <= Limits.Minimum <= 254 duty cycle
 */
static uint32_t xec_pwmbb_compute_dc(uint32_t period_cycles, uint32_t pulse_cycles)
{
	uint32_t dc;

	if (pulse_cycles >= period_cycles) {
		return 255U; /* always on */
	}

	if (period_cycles < 256U) {
		return 0; /* always off */
	}

	dc = (256U * pulse_cycles) / period_cycles;

	return dc;
}

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

	val = regs->delay & ~(XEC_PWM_BBLED_DLY_LO_MSK);
	val |= ((ld  << XEC_PWM_BBLED_DLY_LO_POS) & XEC_PWM_BBLED_DLY_LO_MSK);
	regs->delay = val;

	val = regs->limits & ~(XEC_PWM_BBLED_LIM_MIN_MSK);
	val |= ((dc << XEC_PWM_BBLED_LIM_MIN_POS) & XEC_PWM_BBLED_LIM_MIN_MSK);
	regs->limits = val;

	 /* transfer new delay value from holding register */
	regs->config |= BIT(XEC_PWM_BBLED_CFG_EN_UPDATE_POS);

	val = regs->config & ~(XEC_PWM_BBLED_CFG_MODE_MSK);
	val |= XEC_PWM_BBLED_CFG_MODE_PWM;
	regs->config = val;
}

/* API implementation: Set the period and pulse width for a single PWM.
 * channel must be 0 as each PWM instance implements one output.
 * period in clock cycles of currently configured clock.
 * pulse width in clock cycles of currently configured clock.
 * flags b[7:0] defined by zephyr. b[15:8] can be SoC specific.
 * Bit[0] = 1 inverted, bits[7:1] specify capture features not implemented in XEC PWM.
 * Note: macro PWM_MSEC() and others convert from other units to nanoseconds.
 * BBLED output state is Active High. If Active low is required the GPIO pin invert
 * bit must be set. The XEC PWM block also defaults to Active High but it has a bit
 * to select Active Low.
 * PWM API exposes this function as pwm_set_cycles and has wrapper API defined in
 * pwm.h, named pwm_set which:
 * Calls pwm_get_cycles_per_second to get current maximum HW frequency as cycles_per_sec
 * Computes period_cycles = (period * cycles_per_sec) / NSEC_PER_SEC
 *          pulse_cycles = (pulse * cycles_per_sec) / NSEC_PER_SEC
 * Call pwm_set_cycles passing period_cycles and pulse_cycles.
 *
 * BBLED PWM input frequency is 32KHz (POR default) or 48MHz selected by device tree
 * at application build time.
 * BBLED Fpwm = Fin / (256 * (LD + 1)) where LD = [0, 4095]
 * This equation tells use the maximum number of cycles of Fin the hardware can
 * generate is 256 whereas the mininum number of cycles is 256 * 4096.
 *
 * Fin = 32KHz
 * Fpwm-min = 32768 / (256 * 4096) = 31.25 mHz = 31250000 nHz = 0x01DC_D650 nHz
 * Fpwm-max = 32768 / 256 = 128 Hz = 128e9 nHz = 0x1D_CD65_0000 nHz
 * Tpwm-min = 32e9 ns    = 0x0007_7359_4000 ns
 * Tpmw-max = 7812500 ns = 0x0077_3594 ns
 *
 * Fin = 48MHz
 * Fpwm-min = 48e6 / (256 * 4096) = 45.7763 Hz = 45776367188 nHz = 0x000A_A87B_EE53 nHz
 * Fpwm-max = 48e6 / 256 = 187500 = 1.875e14 = 0xAA87_BEE5_3800 nHz
 * Tpwm-min = 5334 ns = 0x14D6 ns
 * Tpwm-max = 21845333 ns = 0x014D_5555 ns
 */
static int pwm_bbled_xec_check_cycles(uint32_t period_cycles, uint32_t pulse_cycles)
{
	if ((period_cycles < 256U) || (period_cycles > (4096U * 256U))) {
		return -EINVAL;
	}

	if ((pulse_cycles < 256U) || (pulse_cycles > (4096U * 256U))) {
		return -EINVAL;
	}

	return 0;
}

static int pwm_bbled_xec_set_cycles(const struct device *dev, uint32_t channel,
				    uint32_t period_cycles, uint32_t pulse_cycles,
				    pwm_flags_t flags)
{
	const struct pwm_bbled_xec_config * const cfg = dev->config;
	struct bbled_regs * const regs = cfg->regs;
	uint32_t dc, ld;
	int ret;

	if (channel > 0) {
		return -EIO;
	}

	if (flags) {
		/* PWM polarity not supported (yet?) */
		return -ENOTSUP;
	}

	if ((pulse_cycles == 0U) && (period_cycles == 0U)) { /* Controller off, clocks gated */
		regs->config = (regs->config & ~XEC_PWM_BBLED_CFG_MODE_MSK)
			       | XEC_PWM_BBLED_CFG_MODE_OFF;
	} else if ((pulse_cycles == 0U) && (period_cycles > 0U)) {
		/* PWM mode: Limits minimum duty cycle == 0 -> LED output is fully OFF */
		regs->limits &= ~XEC_PWM_BBLED_LIM_MIN_MSK;
	} else if ((pulse_cycles > 0U) && (period_cycles == 0U)) {
		/* PWM mode: Limits minimum duty cycle == full value -> LED output is fully ON */
		regs->limits |= XEC_PWM_BBLED_LIM_MIN_MSK;
	} else {
		ret = pwm_bbled_xec_check_cycles(period_cycles, pulse_cycles);
		if (ret) {
			LOG_DBG("Target frequency out of range");
			return ret;
		}

		ld = xec_pwmbb_compute_ld(dev, period_cycles);
		dc = xec_pwmbb_compute_dc(period_cycles, pulse_cycles);
		xec_pwmbb_progam_pwm(dev, ld, dc);
	}

	return 0;
}

/* API implementation: Get the clock rate (cycles per second) for a single PWM output.
 * BBLED in PWM mode (same as blink mode) PWM frequency = Source Frequency / (256 * (LP + 1))
 * where Source Frequency is either 48 MHz or 32768 Hz and LP is the 12-bit low delay
 * field of the DELAY register.
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
			*cycles = XEC_PWM_BBLED_INPUT_FREQ_HI;
		} else {
			*cycles = XEC_PWM_BBLED_INPUT_FREQ_LO;
		}
	}

	return 0;
}

static const struct pwm_driver_api pwm_bbled_xec_driver_api = {
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
		.clk_sel = UTIL_CAT(XEC_PWM_BBLED_CLKSEL_, XEC_PWM_BBLED_CLKSEL(n)),	\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
	};

#define XEC_PWM_BBLED_DEVICE_INIT(index)				\
									\
	PINCTRL_DT_INST_DEFINE(index);					\
									\
	XEC_PWM_BBLED_CONFIG(index);					\
									\
	DEVICE_DT_INST_DEFINE(index, &pwm_bbled_xec_init,		\
			      NULL,					\
			      NULL,					\
			      &pwm_bbled_xec_config_##index, POST_KERNEL,	\
			      CONFIG_PWM_INIT_PRIORITY,			\
			      &pwm_bbled_xec_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XEC_PWM_BBLED_DEVICE_INIT)
