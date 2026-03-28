/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/pwm/renesas_rz_pwm.h>

LOG_MODULE_REGISTER(pwm_renesas_rza2m_gpt, CONFIG_PWM_LOG_LEVEL);

#define DT_DRV_COMPAT renesas_rza2m_gpt_pwm

#define RZA2M_CAPTURE_BOTH_FIRST_EVENT_IS_PULSE_CAPTURE   1
#define RZA2M_CAPTURE_BOTH_SECOND_EVENT_IS_PERIOD_CAPTURE 2

/* Enable action on the rising edge of GTIOCA input when GTIOCB input is 0 */
#define RZA2M_GT_ARBL BIT(8)
/* Enable action on the rising edge of GTIOCA input when GTIOCB input is 1 */
#define RZA2M_GT_ARBH BIT(9)
#define RZA2M_GT_AR   (RZA2M_GT_ARBL | RZA2M_GT_ARBH)
/* Enable action on the falling edge of GTIOCA input when GTIOCB input is 0 */
#define RZA2M_GT_AFBL BIT(10)
/* Enable action on the falling edge of GTIOCA input when GTIOCB input is 1 */
#define RZA2M_GT_AFBH BIT(11)
#define RZA2M_GT_AF   (RZA2M_GT_AFBL | RZA2M_GT_AFBH)
/* Enable action on the rising edge of GTIOCB input when GTIOCA input is 0 */
#define RZA2M_GT_BRAL BIT(12)
/* Enable action on the rising edge of GTIOCB input when GTIOCA input is 1 */
#define RZA2M_GT_BRAH BIT(13)
#define RZA2M_GT_BR   (RZA2M_GT_BRAL | RZA2M_GT_BRAH)
/* Enable action on the falling edge of GTIOCB input when GTIOCA input is 0 */
#define RZA2M_GT_BFAL BIT(14)
/* Enable action on the falling edge of GTIOCB input when GTIOCA input is 1 */
#define RZA2M_GT_BFAH BIT(15)
#define RZA2M_GT_BF   (RZA2M_GT_BFAL | RZA2M_GT_BFAH)

#define RZA2M_GTSSR_OFFSET 0x10 /* Start Source Select Register */
#define RZA2M_GTPSR_OFFSET 0x14 /* Stop Source Select Register */
#define RZA2M_GTCSR_OFFSET 0x18 /* Clear Source Select Register */

#define RZA2M_GTICASR_OFFSET 0x24 /* Input Capture Source Select Register A */
#define RZA2M_GTICBSR_OFFSET 0x28 /* Input Capture Source Select Register B */

#define RZA2M_GTCR_OFFSET        0x2c /* General PWM Timer Control Register */
#define RZA2M_GTCR_TPCS_SHIFT    24   /* Timer Prescaler Select */
#define RZA2M_GTCR_TPCS_MASK     0x7
#define RZA2M_GTCR_TPCS_MAX_VAL  5U
#define RZA2M_GTCR_GET_TPCS(reg) ((reg >> RZA2M_GTCR_TPCS_SHIFT) & RZA2M_GTCR_TPCS_MASK)
#define RZA2M_GTCR_SET_TPCS(reg, tpcs)                                                             \
	((reg & ~(RZA2M_GTCR_TPCS_MASK << RZA2M_GTCR_TPCS_SHIFT)) |                                \
	 ((tpcs & RZA2M_GTCR_TPCS_MASK) << RZA2M_GTCR_TPCS_SHIFT))
#define RZA2M_GTCR_MD_SHIFT 16
#define RZA2M_GTCR_MD_MASK  0x3
#define RZA2M_GTCR_SET_MD(reg, md)                                                                 \
	((reg & ~(RZA2M_GTCR_MD_MASK << RZA2M_GTCR_MD_SHIFT)) |                                    \
	 ((md & RZA2M_GTCR_MD_MASK) << RZA2M_GTCR_MD_SHIFT))
#define RZA2M_GTCR_MD_PWM_SAW_WAVE 0
#define RZA2M_GTCR_START_CNT       BIT(0)

#define RZA2M_GTUDDTYC_OFFSET 0x30   /* Count Direction and Duty Setting Register */
#define RZA2M_GTUDDTYC_UD     BIT(0) /* Count Direction Setting: counts up on GTCNT */

#define RZA2M_GTUDDTYC_OADTY_MASK (BIT(17) | BIT(16))
#define RZA2M_GTUDDTYC_OADTY_0    BIT(17)
#define RZA2M_GTUDDTYC_OADTY_100  (BIT(17) | BIT(16))

#define RZA2M_GTUDDTYC_OBDTY_MASK (BIT(25) | BIT(24))
#define RZA2M_GTUDDTYC_OBDTY_0    BIT(25)
#define RZA2M_GTUDDTYC_OBDTY_100  (BIT(25) | BIT(24))

#define RZA2M_GTIOR_OFFSET 0x34 /* I/O Control Register */

/* Levels of out on compare match A */
#define RZA2M_GTIOR_GTIOA_OUT_CYC_CMP_LOW  BIT(0)
#define RZA2M_GTIOR_GTIOA_OUT_CYC_CMP_HIGH BIT(1)

/* Levels of out on end of the cycle A */
#define RZA2M_GTIOR_GTIOA_OUT_CYC_END_LOW  BIT(2)
#define RZA2M_GTIOR_GTIOA_OUT_CYC_END_HIGH BIT(3)

/* Levels of out on compare match B */
#define RZA2M_GTIOR_GTIOB_OUT_CYC_CMP_LOW  BIT(16)
#define RZA2M_GTIOR_GTIOB_OUT_CYC_CMP_HIGH BIT(17)

/* Levels of out on end of the cycle B */
#define RZA2M_GTIOR_GTIOB_OUT_CYC_END_LOW  BIT(18)
#define RZA2M_GTIOR_GTIOB_OUT_CYC_END_HIGH BIT(19)

#define RZA2M_GTIOR_OAE BIT(8)  /* GTIOCA Pin Output Enable */
#define RZA2M_GTIOR_OBE BIT(24) /* GTIOCB Pin Output Enable */

#define RZA2M_GTINTAD_OFFSET  0x38   /* Interrupt Output Setting Register */
#define RZA2M_GTINTAD_GTINTA  BIT(0) /* GTCCRA Compare Match/InputCapture Interrupt Enable */
#define RZA2M_GTINTAD_GTINTB  BIT(1) /* GTCCRB Compare Match/InputCapture Interrupt Enable */
#define RZA2M_GTINTAD_GTINTPR BIT(6) /* Overflow Interrupt Enable */

#define RZA2M_GTST_OFFSET 0x3C   /* Status Register */
#define RZA2M_GTST_TCFA   BIT(0) /* Input capture/compare match of GTCCRA occurred */
#define RZA2M_GTST_TCFB   BIT(1) /* Input capture/compare match of GTCCRB occurred */
#define RZA2M_GTST_TCFPO  BIT(6) /* Overflow (crest) occurred */

#define RZA2M_GTBER_OFFSET     0x40      /* Buffer Enable Register */
#define RZA2M_GTBER_CCRA_1_BUF (BIT(16)) /* Single buffer operation for GTCCRA */
#define RZA2M_GTBER_CCRB_1_BUF (BIT(18)) /* Single buffer operation for GTCCRB */
#define RZA2M_GTBER_PR_1_BUF   (BIT(20)) /* Single buffer operation for GTPR */

#define RZA2M_GTBER_ADTTA_CREST (BIT(24))
#define RZA2M_GTBER_ADTTB_CREST (BIT(28))

#define RZA2M_GTBER_CCRA_1_BUF_EN (RZA2M_GTBER_CCRA_1_BUF | RZA2M_GTBER_ADTTA_CREST)
#define RZA2M_GTBER_CCRB_1_BUF_EN (RZA2M_GTBER_CCRB_1_BUF | RZA2M_GTBER_ADTTB_CREST)

/* Interrupt and A/D Converter Start Request Skipping Setting Register */
#define RZA2M_GTITC_OFFSET 0x44

#define RZA2M_GTCNT_OFFSET 0x48 /* Counter */

#define RZA2M_GTCCRA_OFFSET 0x4C /* Compare Capture Register A */
#define RZA2M_GTCCRB_OFFSET 0x50 /* Compare Capture Register B */
#define RZA2M_GTCCRC_OFFSET 0x54 /* Compare Capture Register C */
#define RZA2M_GTCCRE_OFFSET 0x58 /* Compare Capture Register E */

#define RZA2M_GTPR_OFFSET  0x64 /* Cycle Setting Register */
#define RZA2M_GTPBR_OFFSET 0x68 /* Cycle Setting Buffer Register */

enum rza2m_gpt_event {
	RZA2M_GPT_EVENT_CYCLE_END, /* Requested timer delay has expired */
	RZA2M_GPT_EVENT_CAPTURE_A, /* A capture has occurred on signal A */
	RZA2M_GPT_EVENT_CAPTURE_B, /* A capture has occurred on signal B */
};

struct pwm_rza2m_config {
	DEVICE_MMIO_ROM; /* Must be first */
	uint32_t channel_id;
	uint32_t divider;

	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;

	const struct pinctrl_dev_config *pincfg;

	uint32_t ccmpa_irq;
	uint32_t ccmpb_irq;
	uint32_t cycle_end_irq;
};

struct pwm_rza2m_gpt_capture_data {
	pwm_capture_callback_handler_t callback;
	void *user_data;
	uint64_t period;
	uint64_t pulse;
	bool inverted;
	uint16_t type_flag;
	bool is_busy;
	uint32_t overflows;
	bool continuous;
	uint32_t capture_both_event_count;
	uint32_t capture_channel;

	uint32_t start_source;
	uint32_t stop_source;
	uint32_t capture_source;
	uint32_t clear_source;
};

struct pwm_rza2m_data {
	DEVICE_MMIO_RAM; /* Must be first */
	uint32_t clk_rate;
#ifdef CONFIG_PWM_CAPTURE
	struct pwm_rza2m_gpt_capture_data capture;
#endif
};

static uint32_t renesas_rza2m_pwm_read_32(const struct device *dev, uint32_t offset)
{
	return sys_read32(DEVICE_MMIO_GET(dev) + offset);
}

static void renesas_rza2m_pwm_write_32(const struct device *dev, uint32_t offset, uint32_t value)
{
	sys_write32(value, DEVICE_MMIO_GET(dev) + offset);
}

static inline void rza2m_pwm_set_duty_setting(const struct device *dev, uint32_t period_cycles,
					      uint32_t pulse_cycles, bool is_channel_b,
					      bool is_inverted)
{
	uint32_t reg;
	uint32_t new_reg;
	uint32_t mask;
	uint32_t duty_0;
	uint32_t duty_100;

	reg = renesas_rza2m_pwm_read_32(dev, RZA2M_GTUDDTYC_OFFSET);

	/* Select mask and values based on channel */
	if (is_channel_b) {
		mask = RZA2M_GTUDDTYC_OBDTY_MASK;
		duty_0 = RZA2M_GTUDDTYC_OBDTY_0;
		duty_100 = RZA2M_GTUDDTYC_OBDTY_100;
	} else {
		mask = RZA2M_GTUDDTYC_OADTY_MASK;
		duty_0 = RZA2M_GTUDDTYC_OADTY_0;
		duty_100 = RZA2M_GTUDDTYC_OADTY_100;
	}

	new_reg = reg & ~mask;

	/* Set duty based on period/pulse ratio and polarity */
	if (period_cycles == pulse_cycles) {
		/* 100% duty cycle */
		new_reg |= is_inverted ? duty_0 : duty_100;
	} else if (pulse_cycles == 0) {
		/* 0% duty cycle */
		new_reg |= is_inverted ? duty_100 : duty_0;
	} else {
		/* Do nothing */
	}

	if (new_reg != reg) {
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTUDDTYC_OFFSET, new_reg);
	}
}

#define RZA2M_CH_A_IO_FLAGS_NORMAL                                                                 \
	(RZA2M_GTIOR_OAE | RZA2M_GTIOR_GTIOA_OUT_CYC_END_HIGH | RZA2M_GTIOR_GTIOA_OUT_CYC_CMP_LOW)
#define RZA2M_CH_A_IO_FLAGS_INV                                                                    \
	(RZA2M_GTIOR_OAE | RZA2M_GTIOR_GTIOA_OUT_CYC_END_LOW | RZA2M_GTIOR_GTIOA_OUT_CYC_CMP_HIGH)
#define RZA2M_CH_B_IO_FLAGS_NORMAL                                                                 \
	(RZA2M_GTIOR_OBE | RZA2M_GTIOR_GTIOB_OUT_CYC_END_HIGH | RZA2M_GTIOR_GTIOB_OUT_CYC_CMP_LOW)
#define RZA2M_CH_B_IO_FLAGS_INV                                                                    \
	(RZA2M_GTIOR_OBE | RZA2M_GTIOR_GTIOB_OUT_CYC_END_LOW | RZA2M_GTIOR_GTIOB_OUT_CYC_CMP_HIGH)

static void rza2m_pwm_cfg_io(const struct device *dev, bool is_channel_b, bool is_inv)
{
	uint32_t gtior;

	if (is_channel_b) {
		if (is_inv) {
			gtior = RZA2M_CH_B_IO_FLAGS_INV;
		} else {
			gtior = RZA2M_CH_B_IO_FLAGS_NORMAL;
		}
	} else {
		if (is_inv) {
			gtior = RZA2M_CH_A_IO_FLAGS_INV;
		} else {
			gtior = RZA2M_CH_A_IO_FLAGS_NORMAL;
		}
	}
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTIOR_OFFSET, gtior);
}

static int pwm_rza2m_gpt_set_cycles(const struct device *dev, uint32_t channel, uint32_t period_cyc,
				    uint32_t pulse_cyc, pwm_flags_t flags)
{
	uint32_t reg;
	bool is_channel_b;
	bool is_inv;

	if (!(channel == RZ_PWM_GPT_IO_A || channel == RZ_PWM_GPT_IO_B)) {
		LOG_ERR("Valid only for RZ_PWM_GPT_IO_A and RZ_PWM_GPT_IO_B pins");
		return -EINVAL;
	}

	if (period_cyc == 0) {
		LOG_ERR("%s: period is equal to zero", dev->name);
		return -EINVAL;
	}

	if (period_cyc < pulse_cyc) {
		LOG_ERR("%s: period duration less than pulse duration", dev->name);
		return -EINVAL;
	}

	is_channel_b = (channel == RZ_PWM_GPT_IO_B);
	is_inv = ((flags & PWM_POLARITY_INVERTED) == PWM_POLARITY_INVERTED);

	/* Stop counter operation */
	reg = renesas_rza2m_pwm_read_32(dev, RZA2M_GTCR_OFFSET);
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTCR_OFFSET, reg & ~RZA2M_GTCR_START_CNT);

	/* Counter goes up */
	reg = renesas_rza2m_pwm_read_32(dev, RZA2M_GTUDDTYC_OFFSET);
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTUDDTYC_OFFSET, reg | RZA2M_GTUDDTYC_UD);

	rza2m_pwm_set_duty_setting(dev, period_cyc, pulse_cyc, is_channel_b, is_inv);

	/* Timer counter starts from zero */
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTCNT_OFFSET, 0);

	if (is_channel_b) {
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTCCRE_OFFSET, pulse_cyc);
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTCCRB_OFFSET, pulse_cyc);
	} else {
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTCCRC_OFFSET, pulse_cyc);
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTCCRA_OFFSET, pulse_cyc);
	}

	renesas_rza2m_pwm_write_32(dev, RZA2M_GTPR_OFFSET, period_cyc - 1);
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTPBR_OFFSET, period_cyc - 1);

	/* Enable bufferization for registers GTCCRA, GTCCRB and GTPR */
	if (is_channel_b) {
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTBER_OFFSET,
					   RZA2M_GTBER_CCRB_1_BUF_EN | RZA2M_GTBER_PR_1_BUF);
	} else {
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTBER_OFFSET,
					   RZA2M_GTBER_CCRA_1_BUF_EN | RZA2M_GTBER_PR_1_BUF);
	}

	rza2m_pwm_cfg_io(dev, is_channel_b, is_inv);

	/* Start counter operation */
	reg = renesas_rza2m_pwm_read_32(dev, RZA2M_GTCR_OFFSET);
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTCR_OFFSET, reg | RZA2M_GTCR_START_CNT);

	return 0;
};

static int pwm_rza2m_gpt_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					    uint64_t *cycles)
{
	struct pwm_rza2m_data *data = dev->data;
	uint32_t clk_divisor;

	/* We have the same clk_divisor for all in/out */
	clk_divisor = renesas_rza2m_pwm_read_32(dev, RZA2M_GTCR_OFFSET);
	clk_divisor = RZA2M_GTCR_GET_TPCS(clk_divisor);

	if (clk_divisor > RZA2M_GTCR_TPCS_MAX_VAL) {
		LOG_ERR("%s: clock clk_divisor hasn't be bigger than %u", dev->name,
			RZA2M_GTCR_TPCS_MAX_VAL);
		return -EINVAL;
	}

	clk_divisor <<= 1;
	*cycles = data->clk_rate >> clk_divisor;

	return 0;
};

#ifdef CONFIG_PWM_CAPTURE
static int renesas_rza2m_pwm_configure_capture_flow(struct pwm_rza2m_gpt_capture_data *capture)
{
	uint32_t rising_edge;
	uint32_t falling_edge;

	/* Select channel-specific edge detection constants */
	if (capture->capture_channel == RZ_PWM_GPT_IO_B) {
		rising_edge = RZA2M_GT_BR;
		falling_edge = RZA2M_GT_BF;
	} else {
		rising_edge = RZA2M_GT_AR;
		falling_edge = RZA2M_GT_AF;
	}

	/* Configure start and capture sources based on type and polarity */
	capture->start_source = capture->inverted ? falling_edge : rising_edge;

	if (capture->type_flag == PWM_CAPTURE_TYPE_PERIOD) {
		capture->capture_source = capture->start_source;
	} else if (capture->type_flag == PWM_CAPTURE_TYPE_PULSE) {
		capture->capture_source = capture->inverted ? rising_edge : falling_edge;
	} else if (capture->type_flag == PWM_CAPTURE_TYPE_BOTH) {
		capture->capture_source = rising_edge | falling_edge;
	} else {
		return -EINVAL;
	}

	/* Configure continuous mode sources */
	capture->stop_source = 0;
	capture->clear_source = 0;

	if (capture->continuous) {
		if (capture->type_flag == PWM_CAPTURE_TYPE_BOTH) {
			capture->stop_source = 0;
		} else {
			capture->stop_source = capture->capture_source;
		}
		capture->clear_source = capture->start_source;
	}

	return 0;
}

static int pwm_rza2m_gpt_configure_capture(const struct device *dev, uint32_t channel,
					   pwm_flags_t flags, pwm_capture_callback_handler_t cb,
					   void *user_data)
{
	struct pwm_rza2m_data *data = dev->data;
	struct pwm_rza2m_gpt_capture_data *capture = &data->capture;
	uint32_t reg;
	int err;

	if (!(channel == RZ_PWM_GPT_IO_A || channel == RZ_PWM_GPT_IO_B)) {
		LOG_ERR("Valid only for RZ_PWM_GPT_IO_A and RZ_PWM_GPT_IO_B pins");
		return -EINVAL;
	}

	if (capture->is_busy) {
		LOG_ERR("%s: capture started, pls, stop before reconfigutration", dev->name);
		return -EBUSY;
	}

	/* Stop counter operation */
	reg = renesas_rza2m_pwm_read_32(dev, RZA2M_GTCR_OFFSET);
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTCR_OFFSET, reg & ~RZA2M_GTCR_START_CNT);

	/* Counter goes up */
	reg = renesas_rza2m_pwm_read_32(dev, RZA2M_GTUDDTYC_OFFSET);
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTUDDTYC_OFFSET, reg | RZA2M_GTUDDTYC_UD);

	/* Set maximum number cycles to 2^32 */
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTPR_OFFSET, UINT32_MAX);

	/* Disable interrupt skipping function */
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTITC_OFFSET, 0);

	capture->capture_channel = channel;
	capture->inverted = ((flags & PWM_POLARITY_INVERTED) == PWM_POLARITY_INVERTED);
	capture->type_flag = flags & PWM_CAPTURE_TYPE_MASK;
	capture->continuous = flags & PWM_CAPTURE_MODE_CONTINUOUS;

	if (capture->capture_channel == RZ_PWM_GPT_IO_B) {
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTBER_OFFSET, RZA2M_GTBER_CCRB_1_BUF);
	} else {
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTBER_OFFSET, RZA2M_GTBER_CCRA_1_BUF);
	}

	err = renesas_rza2m_pwm_configure_capture_flow(capture);
	if (err < 0) {
		return err;
	}

	capture->callback = cb;
	capture->user_data = user_data;

	return err;
}

static int pwm_rza2m_gpt_enable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_rza2m_config *config = dev->config;
	struct pwm_rza2m_data *data = dev->data;
	struct pwm_rza2m_gpt_capture_data *capture = &data->capture;
	uint32_t intad, st, ssr;

	if (!(channel == RZ_PWM_GPT_IO_A || channel == RZ_PWM_GPT_IO_B)) {
		LOG_ERR("Valid only for RZ_PWM_GPT_IO_A and RZ_PWM_GPT_IO_B pins");
		return -EINVAL;
	}

	if (!capture->callback) {
		LOG_ERR("PWM capture not configured");
		return -EINVAL;
	}

	if (capture->is_busy) {
		LOG_ERR("Capture already active on this pin");
		return -EBUSY;
	}

	capture->capture_channel = channel;
	capture->is_busy = true;
	capture->overflows = 0;
	capture->capture_both_event_count = 0;

	renesas_rza2m_pwm_write_32(dev, RZA2M_GTCNT_OFFSET, 0);

	/* Unmask IRQ on capture for INT A/B */
	intad = renesas_rza2m_pwm_read_32(dev, RZA2M_GTINTAD_OFFSET);
	st = renesas_rza2m_pwm_read_32(dev, RZA2M_GTST_OFFSET);
	ssr = renesas_rza2m_pwm_read_32(dev, RZA2M_GTSSR_OFFSET);

	if (capture->capture_channel == RZ_PWM_GPT_IO_B) {
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTCCRB_OFFSET, 0);
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTCCRE_OFFSET, 0);

		intad |= RZA2M_GTINTAD_GTINTB;
		st &= ~RZA2M_GTST_TCFB;
		ssr &= ~(RZA2M_GT_BF | RZA2M_GT_BR);
	} else {
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTCCRA_OFFSET, 0);
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTCCRC_OFFSET, 0);

		intad |= RZA2M_GTINTAD_GTINTA;
		st &= ~RZA2M_GTST_TCFA;
		ssr &= ~(RZA2M_GT_AF | RZA2M_GT_AR);
	}

	renesas_rza2m_pwm_write_32(dev, RZA2M_GTSSR_OFFSET, capture->start_source | ssr);
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTPSR_OFFSET, capture->stop_source);
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTCSR_OFFSET, capture->clear_source);

	intad |= RZA2M_GTINTAD_GTINTPR;
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTINTAD_OFFSET, intad);
	st &= ~RZA2M_GTST_TCFPO;
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTST_OFFSET, st);

	if (capture->capture_channel == RZ_PWM_GPT_IO_B) {
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTICBSR_OFFSET, capture->capture_source);
		irq_enable(config->ccmpb_irq);
	} else {
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTICASR_OFFSET, capture->capture_source);
		irq_enable(config->ccmpa_irq);
	}
	irq_enable(config->cycle_end_irq);

	return 0;
}

static int pwm_rza2m_gpt_disable_capture(const struct device *dev, uint32_t channel)
{
	const struct pwm_rza2m_config *config = dev->config;
	struct pwm_rza2m_data *data = dev->data;
	struct pwm_rza2m_gpt_capture_data *capture = &data->capture;
	uint32_t reg, st;

	if (!(channel == RZ_PWM_GPT_IO_A || channel == RZ_PWM_GPT_IO_B)) {
		LOG_ERR("Valid only for RZ_PWM_GPT_IO_A and RZ_PWM_GPT_IO_B pins");
		return -EINVAL;
	}

	capture->capture_channel = channel;
	capture->is_busy = false;

	/* Disable auto start of cnt on input edges */
	reg = renesas_rza2m_pwm_read_32(dev, RZA2M_GTSSR_OFFSET);
	if (capture->capture_channel == RZ_PWM_GPT_IO_B) {
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTSSR_OFFSET,
					   reg & ~(RZA2M_GT_BR | RZA2M_GT_BF));
	} else {
		renesas_rza2m_pwm_write_32(dev, RZA2M_GTSSR_OFFSET,
					   reg & ~(RZA2M_GT_AR | RZA2M_GT_AF));
	}

	/* Stop counter operation */
	reg = renesas_rza2m_pwm_read_32(dev, RZA2M_GTCR_OFFSET);
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTCR_OFFSET, reg & ~RZA2M_GTCR_START_CNT);

	reg = renesas_rza2m_pwm_read_32(dev, RZA2M_GTINTAD_OFFSET);
	st = renesas_rza2m_pwm_read_32(dev, RZA2M_GTST_OFFSET);
	if (capture->capture_channel == RZ_PWM_GPT_IO_B) {
		reg &= ~RZA2M_GTINTAD_GTINTB;
		st &= ~RZA2M_GTST_TCFB;
	} else {
		reg &= ~RZA2M_GTINTAD_GTINTA;
		st &= ~RZA2M_GTST_TCFA;
	}

	reg &= ~RZA2M_GTINTAD_GTINTPR;
	st &= ~RZA2M_GTST_TCFPO;

	renesas_rza2m_pwm_write_32(dev, RZA2M_GTINTAD_OFFSET, reg);
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTST_OFFSET, st);
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTPSR_OFFSET, 0);

	irq_disable(config->cycle_end_irq);
	if (capture->capture_channel == RZ_PWM_GPT_IO_B) {
		irq_disable(config->ccmpb_irq);
	} else {
		irq_disable(config->ccmpa_irq);
	}

	return 0;
}
#endif /* CONFIG_PWM_CAPTURE */

static DEVICE_API(pwm, pwm_rza2m_gpt_driver_api) = {
	.get_cycles_per_sec = pwm_rza2m_gpt_get_cycles_per_sec,
	.set_cycles = pwm_rza2m_gpt_set_cycles,
#ifdef CONFIG_PWM_CAPTURE
	.configure_capture = pwm_rza2m_gpt_configure_capture,
	.enable_capture = pwm_rza2m_gpt_enable_capture,
	.disable_capture = pwm_rza2m_gpt_disable_capture,
#endif /* CONFIG_PWM_CAPTURE */
};

static int pwm_rza2m_gpt_init(const struct device *dev)
{
	const struct pwm_rza2m_config *config = dev->config;
	struct pwm_rza2m_data *data = dev->data;
	int err;
	uint32_t tpcs;
	uint32_t reg;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("Failed to configure pins for PWM (%d)", err);
		return err;
	}

	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)config->clock_subsys);
	if (err < 0) {
		return err;
	}

	err = clock_control_get_rate(config->clock_dev,
				     (clock_control_subsys_t)config->clock_subsys, &data->clk_rate);
	if (err < 0) {
		return err;
	}

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	/* Stop counter operation */
	reg = renesas_rza2m_pwm_read_32(dev, RZA2M_GTCR_OFFSET);
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTCR_OFFSET, reg & ~RZA2M_GTCR_START_CNT);

	/* Set saw-wave mode */
	reg = RZA2M_GTCR_SET_MD(reg, RZA2M_GTCR_MD_PWM_SAW_WAVE);

	/* Set divider */
	tpcs = find_lsb_set(config->divider) >> 1;
	reg = RZA2M_GTCR_SET_TPCS(reg, tpcs);
	renesas_rza2m_pwm_write_32(dev, RZA2M_GTCR_OFFSET, reg);

	return 0;
}

#define RZA2M_GPT(idx) DT_INST_PARENT(idx)

#ifdef CONFIG_PWM_CAPTURE
static void renesas_rza2m_pwm_ccmp_handler(const struct device *dev, enum rza2m_gpt_event event)
{
	struct pwm_rza2m_data *data = dev->data;
	struct pwm_rza2m_gpt_capture_data *capture = &data->capture;
	uint32_t reg;
	uint32_t gtccr_offset;
	uint64_t period_cyc;

	period_cyc = renesas_rza2m_pwm_read_32(dev, RZA2M_GTPR_OFFSET) + 1;

	if (event == RZA2M_GPT_EVENT_CAPTURE_A) {
		gtccr_offset = RZA2M_GTCCRA_OFFSET;
	} else {
		gtccr_offset = RZA2M_GTCCRB_OFFSET;
	}

	reg = renesas_rza2m_pwm_read_32(dev, gtccr_offset);
	if (reg == 0) {
		return;
	}

	if (capture->type_flag == PWM_CAPTURE_TYPE_BOTH) {
		capture->capture_both_event_count++;
		if (capture->capture_both_event_count ==
		    RZA2M_CAPTURE_BOTH_FIRST_EVENT_IS_PULSE_CAPTURE) {
			capture->pulse = (capture->overflows * period_cyc) + reg;

			return;
		}
		if (capture->capture_both_event_count ==
		    RZA2M_CAPTURE_BOTH_SECOND_EVENT_IS_PERIOD_CAPTURE) {
			capture->capture_both_event_count = 0;
			capture->period = (capture->overflows * period_cyc) + reg;
			capture->callback(dev, capture->capture_channel, capture->period,
					  capture->pulse, 0, capture->user_data);
		}
	} else if (capture->type_flag == PWM_CAPTURE_TYPE_PULSE) {
		capture->pulse = (capture->overflows * period_cyc) + reg;
		capture->callback(dev, capture->capture_channel, 0, capture->pulse, 0,
				  capture->user_data);
	} else {
		capture->period = (capture->overflows * period_cyc) + reg;
		capture->callback(dev, capture->capture_channel, capture->period, 0, 0,
				  capture->user_data);
	}

	capture->overflows = 0;

	if (!capture->continuous) {
		pwm_rza2m_gpt_disable_capture(dev, capture->capture_channel);
	}
}

static void pwm_rza2m_gpt_ccmpa_isr(const struct device *dev)
{
	renesas_rza2m_pwm_ccmp_handler(dev, RZA2M_GPT_EVENT_CAPTURE_A);
}

static void pwm_rza2m_gpt_ccmpb_isr(const struct device *dev)
{
	renesas_rza2m_pwm_ccmp_handler(dev, RZA2M_GPT_EVENT_CAPTURE_B);
}

static void pwm_rza2m_gpt_ovf_isr(const struct device *dev)
{
	struct pwm_rza2m_data *data = dev->data;

	data->capture.overflows++;
}

#define GPT_GET_IRQ_FLAGS(idx, irq_name) DT_IRQ_BY_NAME(RZA2M_GPT(idx), irq_name, flags)

#define PWM_RZA2M_IRQ_CONFIG_INIT(inst)                                                            \
	do {                                                                                       \
		IRQ_CONNECT(DT_IRQ_BY_NAME(RZA2M_GPT(inst), ccmpa, irq) - GIC_SPI_INT_BASE,        \
			    DT_IRQ_BY_NAME(RZA2M_GPT(inst), ccmpa, priority),                      \
			    pwm_rza2m_gpt_ccmpa_isr, DEVICE_DT_INST_GET(inst),                     \
			    GPT_GET_IRQ_FLAGS(inst, ccmpa));                                       \
		IRQ_CONNECT(DT_IRQ_BY_NAME(RZA2M_GPT(inst), ccmpb, irq) - GIC_SPI_INT_BASE,        \
			    DT_IRQ_BY_NAME(RZA2M_GPT(inst), ccmpb, priority),                      \
			    pwm_rza2m_gpt_ccmpb_isr, DEVICE_DT_INST_GET(inst),                     \
			    GPT_GET_IRQ_FLAGS(inst, ccmpb));                                       \
		IRQ_CONNECT(DT_IRQ_BY_NAME(RZA2M_GPT(inst), ovf, irq) - GIC_SPI_INT_BASE,          \
			    DT_IRQ_BY_NAME(RZA2M_GPT(inst), ovf, priority), pwm_rza2m_gpt_ovf_isr, \
			    DEVICE_DT_INST_GET(inst), GPT_GET_IRQ_FLAGS(inst, ovf));               \
	} while (0)
#endif /* CONFIG_PWM_CAPTURE */

#define PWM_RZA2M_INIT(inst)                                                                       \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	uint32_t pwm_clock_subsys##inst = DT_CLOCKS_CELL(RZA2M_GPT(inst), clk_id);                 \
	static const struct pwm_rza2m_config pwm_rza2m_gpt_config_##inst = {                       \
		DEVICE_MMIO_ROM_INIT(DT_PARENT(DT_DRV_INST(inst))),                                \
		.channel_id = DT_PROP(RZA2M_GPT(inst), channel),                                   \
		.divider = DT_PROP(RZA2M_GPT(inst), prescaler),                                    \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(RZA2M_GPT(inst))),                       \
		.clock_subsys = (clock_control_subsys_t)(&pwm_clock_subsys##inst),                 \
		.ccmpa_irq = DT_IRQ_BY_NAME(RZA2M_GPT(inst), ccmpa, irq) - GIC_SPI_INT_BASE,       \
		.ccmpb_irq = DT_IRQ_BY_NAME(RZA2M_GPT(inst), ccmpb, irq) - GIC_SPI_INT_BASE,       \
		.cycle_end_irq = DT_IRQ_BY_NAME(RZA2M_GPT(inst), ovf, irq) - GIC_SPI_INT_BASE,     \
	};                                                                                         \
	static struct pwm_rza2m_data pwm_rza2m_data_##inst = {};                                   \
	static int pwm_rza2m_gpt_init_##inst(const struct device *dev)                             \
	{                                                                                          \
		IF_ENABLED(CONFIG_PWM_CAPTURE,                                  \
			    (PWM_RZA2M_IRQ_CONFIG_INIT(inst);))                  \
		int err = pwm_rza2m_gpt_init(dev);                                                 \
		if (err < 0) {                                                                     \
			return err;                                                                \
		}                                                                                  \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(inst, pwm_rza2m_gpt_init_##inst, NULL, &pwm_rza2m_data_##inst,       \
			      &pwm_rza2m_gpt_config_##inst, POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, \
			      &pwm_rza2m_gpt_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_RZA2M_INIT);
