/*
 * Copyright (c) 2022 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_xec_bbled

/**
 * @file
 * @brief Microchip Breathing-Blinking LED controller
 */

#include <soc.h>
#ifndef CONFIG_SOC_SERIES_MEC15XX
#include <zephyr/drivers/clock_control/mchp_xec_clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_mchp_xec_ecia.h>
#endif
#include <zephyr/drivers/led.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(led_xec, CONFIG_LED_LOG_LEVEL);

/* Same BBLED hardware block in MEC15xx and MEC172x families
 * Config register
 */
#define XEC_BBLED_CFG_MSK		0x1ffffu
#define XEC_BBLED_CFG_MODE_POS		0
#define XEC_BBLED_CFG_MODE_MSK		0x3u
#define XEC_BBLED_CFG_MODE_OFF		0
#define XEC_BBLED_CFG_MODE_BREATHING	0x1u
#define XEC_BBLED_CFG_MODE_PWM		0x2u
#define XEC_BBLED_CFG_MODE_ALWAYS_ON	0x3u
#define XEC_BBLED_CFG_CLK_SRC_48M_POS	2
#define XEC_BBLED_CFG_EN_UPDATE_POS	6
#define XEC_BBLED_CFG_RST_PWM_POS	7
#define XEC_BBLED_CFG_WDT_RLD_POS	8
#define XEC_BBLED_CFG_WDT_RLD_MSK0	0xffu
#define XEC_BBLED_CFG_WDT_RLD_MSK	0xff00u
#define XEC_BBLED_CFG_WDT_RLD_DFLT	0x1400u

/* Limits register */
#define XEC_BBLED_LIM_MSK		0xffffu
#define XEC_BBLED_LIM_MIN_POS		0
#define XEC_BBLED_LIM_MIN_MSK		0xffu
#define XEC_BBLED_LIM_MAX_POS		8
#define XEC_BBLED_LIM_MAX_MSK		0xff00u

/* Delay register */
#define XEC_BBLED_DLY_MSK		0xffffffu
#define XEC_BBLED_DLY_LO_POS		0
#define XEC_BBLED_DLY_LO_MSK		0xfffu
#define XEC_BBLED_DLY_HI_POS		12
#define XEC_BBLED_DLY_HI_MSK		0xfff000u

/* Update step size and update interval registers implement
 * eight 4-bit fields numbered 0 to 7
 */
#define XEC_BBLED_UPD_SSI_POS(n)	((uint32_t)(n) * 4u)
#define XEC_BBLED_UPD_SSI0_MSK(n)	((uint32_t)0xfu << XEC_BBLED_UPD_SSI_POS(n))

/* Output delay register: b[7:0] is delay in clock source units */
#define XEC_BBLED_OUT_DLY_MSK		0xffu

/* Delay.Lo register field */
#define XEC_BBLED_MAX_PRESCALER		4095u
/* Blink mode source frequency is 32768 Hz */
#define XEC_BBLED_BLINK_CLK_SRC_HZ	32768u
/* Fblink = 32768 / (256 * (prescaler+1))
 * prescaler is 12 bit.
 * Maximum Fblink = 128 Hz or 7.8125 ms
 * Minimum Fblink = 32.25 mHz or 32000 ms
 */
#define XEC_BBLED_BLINK_PERIOD_MAX_MS	32000u
#define XEC_BBLED_BLINK_PERIOD_MIN_MS	8u

struct xec_bbled_regs {
	volatile uint32_t config;
	volatile uint32_t limits;
	volatile uint32_t delay;
	volatile uint32_t update_step_size;
	volatile uint32_t update_interval;
	volatile uint32_t output_delay;
};

struct xec_bbled_config {
	struct xec_bbled_regs * const regs;
	const struct pinctrl_dev_config *pcfg;
	uint8_t pcr_id;
	uint8_t pcr_pos;
};

/* delay_on and delay_off are in milliseconds
 * (prescale+1) = (32768 * Tblink_ms) / (256 * 1000)
 * requires caller to limit delay_on and delay_off based
 * on BBLED 32KHz minimum/maximum values.
 */
static uint32_t calc_blink_32k_prescaler(uint32_t delay_on, uint32_t delay_off)
{
	uint32_t temp = ((delay_on + delay_off) * XEC_BBLED_BLINK_CLK_SRC_HZ) / (256U * 1000U);
	uint32_t prescaler = 0;

	if (temp) {
		temp--;
		if (temp > XEC_BBLED_MAX_PRESCALER) {
			prescaler = XEC_BBLED_MAX_PRESCALER;
		} else {
			prescaler = (uint32_t)temp;
		}
	}

	return prescaler;
}

/* return duty cycle scaled to [0, 255]
 * caller must insure delay_on and delay_off are in hardware range.
 */
static uint32_t calc_blink_duty_cycle(uint32_t delay_on, uint32_t delay_off)
{
	return (256U * delay_on) / (delay_on + delay_off);
}

/* Enable HW blinking of the LED.
 * delay_on = on time in milliseconds
 * delay_off = off time in milliseconds
 * BBLED blinking mode uses an 8-bit accumulator and an 8-bit duty cycle
 * register. The duty cycle register is programmed once and the
 * accumulator is used as an 8-bit up counter.
 * The counter uses the 32768 Hz clock and is pre-scaled by the delay
 * counter. Maximum blink rate is 128Hz to 32.25 mHz (7.8 ms to 32 seconds).
 * 8-bit duty cycle values: 0x00 = full off, 0xff = full on.
 * Fblink = 32768 / ((prescale + 1) * 256)
 * HiWidth (seconds) = (1/Fblink) * (duty_cycle / 256)
 * LoWidth (seconds) = (1/Fblink) * ((1 - duty_cycle) / 256)
 * duty_cycle in [0, 1]. Register value for duty cycle is
 * scaled to [0, 255].
 * prescale is delay register low delay field, bits[11:0]
 * duty_cycle is limits register minimum field, bits[7:0]
 */
static int xec_bbled_blink(const struct device *dev, uint32_t led,
			    uint32_t delay_on, uint32_t delay_off)
{
	const struct xec_bbled_config * const config = dev->config;
	struct xec_bbled_regs * const regs = config->regs;
	uint32_t period, prescaler, dcs;

	if (led) {
		return -EINVAL;
	}

	/* insure period will not overflow uin32_t */
	if ((delay_on > XEC_BBLED_BLINK_PERIOD_MAX_MS)
	    || (delay_off > XEC_BBLED_BLINK_PERIOD_MAX_MS)) {
		return -EINVAL;
	}

	period = delay_on + delay_off;
	if ((period < XEC_BBLED_BLINK_PERIOD_MIN_MS)
	    || (period > XEC_BBLED_BLINK_PERIOD_MAX_MS)) {
		return -EINVAL;
	}

	prescaler = calc_blink_32k_prescaler(delay_on, delay_off);
	dcs = calc_blink_duty_cycle(delay_on, delay_off);

	regs->config = (regs->config & ~(XEC_BBLED_CFG_MODE_MSK))
		       | XEC_BBLED_CFG_MODE_OFF;
	regs->delay = (regs->delay & ~(XEC_BBLED_DLY_LO_MSK))
		      | (prescaler & XEC_BBLED_DLY_LO_MSK);
	regs->limits = (regs->limits & ~(XEC_BBLED_LIM_MIN_MSK))
		       | (dcs & XEC_BBLED_LIM_MIN_MSK);
	regs->config = (regs->config & ~(XEC_BBLED_CFG_MODE_MSK))
		       | XEC_BBLED_CFG_MODE_PWM;
	regs->config |= BIT(XEC_BBLED_CFG_EN_UPDATE_POS);

	return 0;
}

static int xec_bbled_on(const struct device *dev, uint32_t led)
{
	const struct xec_bbled_config * const config = dev->config;
	struct xec_bbled_regs * const regs = config->regs;

	if (led) {
		return -EINVAL;
	}

	regs->config = (regs->config & ~(XEC_BBLED_CFG_MODE_MSK))
			| XEC_BBLED_CFG_MODE_ALWAYS_ON;
	return 0;
}

static int xec_bbled_off(const struct device *dev, uint32_t led)
{
	const struct xec_bbled_config * const config = dev->config;
	struct xec_bbled_regs * const regs = config->regs;

	if (led) {
		return -EINVAL;
	}

	regs->config = (regs->config & ~(XEC_BBLED_CFG_MODE_MSK))
			| XEC_BBLED_CFG_MODE_OFF;
	return 0;
}

#ifdef CONFIG_SOC_SERIES_MEC15XX
static inline void xec_bbled_slp_en_clr(const struct device *dev)
{
	const struct xec_bbled_config * const cfg = dev->config;
	enum pcr_id pcr_val = PCR_MAX_ID;

	switch (cfg->pcr_pos) {
	case MCHP_PCR3_LED0_POS:
		pcr_val = PCR_LED0;
		break;
	case MCHP_PCR3_LED1_POS:
		pcr_val = PCR_LED1;
		break;
	case MCHP_PCR3_LED2_POS:
		pcr_val = PCR_LED2;
		break;
	default:
		return;
	}

	mchp_pcr_periph_slp_ctrl(pcr_val, 0);
}
#else
static inline void xec_bbled_slp_en_clr(const struct device *dev)
{
	const struct xec_bbled_config * const cfg = dev->config;

	z_mchp_xec_pcr_periph_sleep(cfg->pcr_id, cfg->pcr_pos, 0);
}
#endif

static int xec_bbled_init(const struct device *dev)
{
	const struct xec_bbled_config * const config = dev->config;
	struct xec_bbled_regs * const regs = config->regs;
	int ret;

	xec_bbled_slp_en_clr(dev);

	/* soft reset, disable BBLED WDT, set clock source to default (32KHz domain) */
	regs->config |= BIT(XEC_BBLED_CFG_RST_PWM_POS);
	regs->config = XEC_BBLED_CFG_MODE_OFF;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("XEC BBLED pinctrl setup failed (%d)", ret);
	}

	return ret;
}

static const struct led_driver_api xec_bbled_api = {
	.on		= xec_bbled_on,
	.off		= xec_bbled_off,
	.blink		= xec_bbled_blink,
};

#define XEC_BBLED_PINCTRL_DEF(i) PINCTRL_DT_INST_DEFINE(i)

#define XEC_BBLED_CONFIG(i)						\
static struct xec_bbled_config xec_bbled_config_##i = {			\
	.regs = (struct xec_bbled_regs * const)DT_INST_REG_ADDR(i),	\
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(i),			\
	.pcr_id = (uint8_t)DT_INST_PROP_BY_IDX(i, pcrs, 0),		\
	.pcr_pos = (uint8_t)DT_INST_PROP_BY_IDX(i, pcrs, 1),		\
}

#define XEC_BBLED_DEVICE(i)						\
									\
XEC_BBLED_PINCTRL_DEF(i);						\
									\
XEC_BBLED_CONFIG(i);							\
									\
DEVICE_DT_INST_DEFINE(i, &xec_bbled_init, NULL,				\
		      NULL, &xec_bbled_config_##i,			\
		      POST_KERNEL, CONFIG_LED_INIT_PRIORITY,		\
		      &xec_bbled_api);

DT_INST_FOREACH_STATUS_OKAY(XEC_BBLED_DEVICE)
