/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>

#define DT_DRV_COMPAT microchip_tc_g1_pwm

LOG_MODULE_REGISTER(pwm_mchp_tc_g1, CONFIG_PWM_LOG_LEVEL);

#define PWM_MODE8(pwm_reg)  ((tc_count8_registers_t *)&(((tc_registers_t *)(pwm_reg))->COUNT8))
#define PWM_MODE16(pwm_reg) ((tc_count16_registers_t *)&(((tc_registers_t *)(pwm_reg))->COUNT16))
#define PWM_MODE32(pwm_reg) ((tc_count32_registers_t *)&(((tc_registers_t *)(pwm_reg))->COUNT32))

#define MCHP_PWM_SUCCESS 0

#define MCHP_PWM_LOCK_TIMEOUT K_MSEC(10)

#define TIMEOUT_VALUE_US 5000000
#define DELAY_US         2

enum pwm_counter_modes {
	BIT_MODE_8 = 8,
	BIT_MODE_16 = 16,
	BIT_MODE_24 = 24,
	BIT_MODE_32 = 32,
};

enum pwm_prescale_modes {
	PWM_PRESCALE_1 = 1,
	PWM_PRESCALE_2 = 2,
	PWM_PRESCALE_4 = 4,
	PWM_PRESCALE_8 = 8,
	PWM_PRESCALE_16 = 16,
	PWM_PRESCALE_32 = 32,
	PWM_PRESCALE_64 = 64,
	PWM_PRESCALE_128 = 128,
	PWM_PRESCALE_256 = 256,
	PWM_PRESCALE_512 = 512,
	PWM_PRESCALE_1024 = 1024
};

struct pwm_mchp_data {
	struct k_mutex lock;
};

struct mchp_pwm_clock {
	const struct device *clock_dev;
	clock_control_subsys_t host_mclk;
	clock_control_subsys_t client_mclk;
	clock_control_subsys_t host_gclk;
};

struct pwm_mchp_config {
	void *regs;             /*Pointer to PWM peripheral registers */
	uint32_t max_bit_width; /* Used for finding the mode of tc peripheral */
	struct mchp_pwm_clock pwm_clock;
	const struct pinctrl_dev_config *pinctrl_config;
	uint16_t prescaler;
	uint8_t channels; /* Number of PWM channels */
	uint32_t freq;    /* Frequency of the PWM signal */
};

/*
 *This function maps a given prescaler constant to its corresponding numerical
 *value. If the prescaler does not match any predefined constants, it returns 0.
 */
static uint32_t tc_get_prescale_val(uint32_t prescaler)
{
	uint32_t prescaler_val;

	switch (prescaler) {
	case PWM_PRESCALE_1:
		prescaler_val = TC_CTRLA_PRESCALER_DIV1;
		break;
	case PWM_PRESCALE_2:
		prescaler_val = TC_CTRLA_PRESCALER_DIV2;
		break;
	case PWM_PRESCALE_4:
		prescaler_val = TC_CTRLA_PRESCALER_DIV4;
		break;
	case PWM_PRESCALE_8:
		prescaler_val = TC_CTRLA_PRESCALER_DIV8;
		break;
	case PWM_PRESCALE_16:
		prescaler_val = TC_CTRLA_PRESCALER_DIV16;
		break;
	case PWM_PRESCALE_64:
		prescaler_val = TC_CTRLA_PRESCALER_DIV64;
		break;
	case PWM_PRESCALE_256:
		prescaler_val = TC_CTRLA_PRESCALER_DIV256;
		break;
	case PWM_PRESCALE_1024:
		prescaler_val = TC_CTRLA_PRESCALER_DIV1024;
		break;
	default:
		prescaler_val = TC_CTRLA_PRESCALER_DIV1;
		LOG_ERR("Unsupported prescaler specified in dts. Initialising with default "
			"prescaler of DIV1");
		break;
	}

	return prescaler_val;
}

/*
 * This function will check whether the tc peripheral is in slave mode or not.
 * This is for ensuring that the tc peripheral will not be configured if it is chained to a host tc
 * peripheral to achieve 32 bit mode.
 */
static bool check_slave_status(const void *pwm_reg)
{
	bool ret;

	ret = ((PWM_MODE8(pwm_reg)->TC_STATUS & TC_STATUS_SLAVE_Msk) != 0) ? true : false;
	LOG_DBG("%s", (ret ? "tc is a slave" : "tc is not a slave"));

	return ret;
}

static void tc_sync_wait(const void *pwm_reg, const uint32_t max_bit_width)
{
	switch (max_bit_width) {
	case BIT_MODE_8:
		if ((WAIT_FOR((0 == (PWM_MODE8(pwm_reg)->TC_SYNCBUSY)), TIMEOUT_VALUE_US,
			      k_busy_wait(DELAY_US))) == false) {
			LOG_ERR("TC_SYNCBUSY8 reset timed out");
		}
		break;

	case BIT_MODE_16:
		if ((WAIT_FOR((0 == (PWM_MODE16(pwm_reg)->TC_SYNCBUSY)), TIMEOUT_VALUE_US,
			      k_busy_wait(DELAY_US))) == false) {
			LOG_ERR("TC_SYNCBUSY16 reset timed out");
		}
		break;

	case BIT_MODE_32:
		if ((WAIT_FOR((0 == (PWM_MODE32(pwm_reg)->TC_SYNCBUSY)), TIMEOUT_VALUE_US,
			      k_busy_wait(DELAY_US))) == false) {
			LOG_ERR("TC_SYNCBUSY32 reset timed out");
		}
		break;

	default:
		LOG_ERR("%s : Unsupported PWM mode %d", __func__, max_bit_width);
		break;
	}
}

/*
 *This function resets the TC registers for the given PWM.
 *It sets the TC_CTRLA register to initiate a software reset based on the
 *max_bit_width. After setting the reset, it waits for synchronization to
 *complete.
 */
static int tc_reset_regs(const void *pwm_reg, const uint32_t max_bit_width)
{
	bool slave_mode = check_slave_status(pwm_reg);

	if (slave_mode == true) {
		LOG_ERR("tc is in slave mode");
		return -EBUSY;
	}

	switch (max_bit_width) {
	case BIT_MODE_8:
		PWM_MODE8(pwm_reg)->TC_CTRLA = TC_CTRLA_SWRST(1);
		break;

	case BIT_MODE_16:
		PWM_MODE16(pwm_reg)->TC_CTRLA = TC_CTRLA_SWRST(1);
		break;

	case BIT_MODE_32:
		PWM_MODE32(pwm_reg)->TC_CTRLA = TC_CTRLA_SWRST(1);
		break;

	default:
		LOG_ERR("%s : Unsupported PWM mode %d", __func__, max_bit_width);
		return -ENOTSUP;
	}
	LOG_DBG("%s invoked %d", __func__, max_bit_width);
	tc_sync_wait(pwm_reg, max_bit_width);

	return MCHP_PWM_SUCCESS;
}

/*
 * This function enables or disables the TC based on the enable parameter.
 * It sets or clears the TC_CTRLA_ENABLE bit in the TC_CTRLA register based on
 * the max_bit_width. After setting or clearing the enable bit, it waits for
 * synchronization to complete.
 */
static int32_t tc_enable(const void *pwm_reg, const uint32_t max_bit_width, bool enable)
{
	bool slave_mode = check_slave_status(pwm_reg);

	if (slave_mode == true) {
		LOG_ERR("tc is in slave mode");
		return -EBUSY;
	}

	switch (max_bit_width) {
	case BIT_MODE_8:
		if (enable != 0) {
			PWM_MODE8(pwm_reg)->TC_CTRLA |= TC_CTRLA_ENABLE(1);
		} else {
			PWM_MODE8(pwm_reg)->TC_CTRLA &= ~TC_CTRLA_ENABLE(1);
		}
		LOG_DBG("%s %d invoked 0x%x", __func__, enable, PWM_MODE8(pwm_reg)->TC_CTRLA);
		break;

	case BIT_MODE_16:
		if (enable != 0) {
			PWM_MODE16(pwm_reg)->TC_CTRLA |= TC_CTRLA_ENABLE(1);
		} else {
			PWM_MODE16(pwm_reg)->TC_CTRLA &= ~TC_CTRLA_ENABLE(1);
		}
		break;

	case BIT_MODE_32:
		if (enable != 0) {
			PWM_MODE32(pwm_reg)->TC_CTRLA |= TC_CTRLA_ENABLE(1);
		} else {
			PWM_MODE32(pwm_reg)->TC_CTRLA &= ~TC_CTRLA_ENABLE(1);
		}
		break;

	default:
		LOG_ERR("%s : Unsupported PWM mode %d", __func__, max_bit_width);
		return -ENOTSUP;
	}
	tc_sync_wait(pwm_reg, max_bit_width);

	return MCHP_PWM_SUCCESS;
}

/*
 * This function sets the mode of the TC based on the max_bit_width.
 * It clears the current mode bits in the TC_CTRLA register and sets the
 * appropriate mode bits. After setting the mode, it waits for synchronization to
 * complete. Returns 0 on success, or -1 if the max_bit_width is unsupported.
 */
static int32_t tc_set_mode(const void *pwm_reg, const uint32_t max_bit_width)
{
	uint32_t reg_val;
	bool slave_mode = check_slave_status(pwm_reg);

	if (slave_mode == true) {
		LOG_ERR("tc is in slave mode");
		return -EBUSY;
	}

	switch (max_bit_width) {
	case BIT_MODE_8:
		reg_val = PWM_MODE8(pwm_reg)->TC_CTRLA;
		reg_val &= (~TC_CTRLA_MODE_Msk);
		reg_val |= TC_CTRLA_MODE(TC_CTRLA_MODE_COUNT8_Val);
		PWM_MODE8(pwm_reg)->TC_CTRLA = (uint8_t)reg_val;
		LOG_DBG("CTRLA = 0x%x\n", PWM_MODE8(pwm_reg)->TC_CTRLA);
		break;

	case BIT_MODE_16:
		reg_val = PWM_MODE16(pwm_reg)->TC_CTRLA;
		reg_val &= (~TC_CTRLA_MODE_Msk);
		reg_val |= TC_CTRLA_MODE(TC_CTRLA_MODE_COUNT16_Val);
		PWM_MODE16(pwm_reg)->TC_CTRLA = (uint16_t)reg_val;
		break;

	case BIT_MODE_32:
		reg_val = PWM_MODE32(pwm_reg)->TC_CTRLA;
		reg_val &= (~TC_CTRLA_MODE_Msk);
		reg_val |= TC_CTRLA_MODE(TC_CTRLA_MODE_COUNT32_Val);
		PWM_MODE32(pwm_reg)->TC_CTRLA = reg_val;
		break;

	default:
		LOG_ERR("%s : Unsupported PWM mode %d", __func__, max_bit_width);
		return -ENOTSUP;
	}
	tc_sync_wait(pwm_reg, max_bit_width);
	LOG_DBG("Mode set = %x\n", TC_CTRLA_MODE(TC_CTRLA_MODE_COUNT8_Val));

	return MCHP_PWM_SUCCESS;
}

/*
 * This function sets the pulse width for the specified channel based on the
 * max_bit_width. It writes the pulse value to the appropriate TC_CCBUF register.
 * Logs the pulse value for debugging purposes.
 *
 * In 16-bit/32-bit mode, the pulse value is written to CCBUF[1]. This is because they are in MPWM
 * mode. In MPWM mode, the wave output will can be observed in WO[1] and a negative spike can  be
 * observed in each overflow of the counter(at the beginning of each period). By default double
 * buffering is enabled to prevent wraparound issues.
 */
static int32_t tc_set_pulse_buf(const void *pwm_reg, uint32_t max_bit_width, uint32_t channel,
				uint32_t pulse)
{
	bool slave_mode = check_slave_status(pwm_reg);

	if (slave_mode == true) {
		LOG_ERR("tc is in slave mode");
		return -EBUSY;
	}

	switch (max_bit_width) {
	case BIT_MODE_8:
		PWM_MODE8(pwm_reg)->TC_CCBUF[channel] = TC_COUNT8_CCBUF_CCBUF(pulse);
		LOG_DBG("m_tc_set_pulse invoked 8: 0x%x", TC_COUNT8_CCBUF_CCBUF(pulse));
		break;

	case BIT_MODE_16:
		PWM_MODE16(pwm_reg)->TC_CCBUF[1] = TC_COUNT16_CCBUF_CCBUF(pulse);
		LOG_DBG("m_tc_set_pulse invoked 16: 0x%x", TC_COUNT16_CCBUF_CCBUF(pulse));
		break;

	case BIT_MODE_32:
		PWM_MODE32(pwm_reg)->TC_CCBUF[1] = TC_COUNT32_CCBUF_CCBUF(pulse);
		LOG_DBG("m_tc_set_pulse invoked 32 : 0x%x", TC_COUNT32_CCBUF_CCBUF(pulse));
		break;

	default:
		LOG_ERR("%s : Unsupported PWM mode %d", __func__, max_bit_width);
		return -ENOTSUP;
	}

	return MCHP_PWM_SUCCESS;
}

/*
 * This function sets the period value for the TC based on the max_bit_width.
 * It writes the period value to the appropriate register (TC_PER or TC_CC[0]).
 * Logs the period value and max_bit_width for debugging purposes.
 * After setting the period, it waits for synchronization to complete.
 */
static int32_t tc_set_period(const void *pwm_reg, const uint32_t max_bit_width,
			     const uint32_t period)
{
	bool slave_mode = check_slave_status(pwm_reg);

	if (slave_mode == true) {
		LOG_ERR("tc is in slave mode");
		return -EBUSY;
	}

	switch (max_bit_width) {
	case BIT_MODE_8:
		PWM_MODE8(pwm_reg)->TC_PER = TC_COUNT8_PER_PER(period);
		break;

	case BIT_MODE_16:
		PWM_MODE16(pwm_reg)->TC_CC[0u] = TC_COUNT16_CC_CC(period);
		break;

	case BIT_MODE_32:
		PWM_MODE32(pwm_reg)->TC_CC[0] = TC_COUNT32_CC_CC(period);
		break;

	default:
		LOG_ERR("%s : Unsupported PWM mode %d", __func__, max_bit_width);
		return -ENOTSUP;
	}
	tc_sync_wait(pwm_reg, max_bit_width);

	return MCHP_PWM_SUCCESS;
}

/*
 * This function sets the period value for the TC based on the max_bit_width.
 * It writes the period value to the appropriate register (TC_PERBUF or TC_CCBUF[0]).
 * Logs the period value and max_bit_width for debugging purposes.
 * After setting the period, it waits for synchronization to complete.
 */
static int32_t tc_set_period_buf(const void *pwm_reg, const uint32_t max_bit_width,
				 const uint32_t period)
{
	bool slave_mode = check_slave_status(pwm_reg);

	if (slave_mode == true) {
		LOG_ERR("tc is in slave mode");
		return -EBUSY;
	}

	switch (max_bit_width) {
	case BIT_MODE_8:
		PWM_MODE8(pwm_reg)->TC_PERBUF = TC_COUNT8_CCBUF_CCBUF(period);
		break;

	case BIT_MODE_16:
		PWM_MODE16(pwm_reg)->TC_CCBUF[0u] = TC_COUNT16_CCBUF_CCBUF(period);
		break;

	case BIT_MODE_32:
		PWM_MODE32(pwm_reg)->TC_CCBUF[0u] = TC_COUNT32_CCBUF_CCBUF(period);
		break;

	default:
		LOG_ERR("%s : Unsupported PWM mode %d", __func__, max_bit_width);
		return -ENOTSUP;
	}
	LOG_DBG("period %d bit:  set to %x", max_bit_width, period);
	tc_sync_wait(pwm_reg, max_bit_width);

	return MCHP_PWM_SUCCESS;
}

/*
 * This function sets the invert mode for the specified channel based on the
 * max_bit_width. It first disables the TC, waits for synchronization, and then
 * sets the invert mask in the TC_DRVCTRL register. After setting the invert
 * mask, it re-enables the TC and waits for synchronization again.
 */
static int32_t tc_set_invert(const void *pwm_reg, const uint32_t max_bit_width, uint32_t channel)
{
	uint8_t reg_val;
	bool slave_mode = check_slave_status(pwm_reg);

	if (slave_mode == true) {
		LOG_ERR("tc is in slave mode");
		return -EBUSY;
	}
	uint32_t invert_mask = BIT(channel + TC_DRVCTRL_INVEN0_Pos);

	tc_enable(pwm_reg, max_bit_width, false);
	tc_sync_wait(pwm_reg, max_bit_width);

	switch (max_bit_width) {
	case BIT_MODE_8:
		reg_val = PWM_MODE8(pwm_reg)->TC_DRVCTRL;
		reg_val &= (~TC_DRVCTRL_INVEN_Msk);
		reg_val |= invert_mask;
		PWM_MODE8(pwm_reg)->TC_DRVCTRL = reg_val;
		LOG_DBG("tc set invert 0x%x invoked", invert_mask);
		break;

	case BIT_MODE_16:
		reg_val = PWM_MODE16(pwm_reg)->TC_DRVCTRL;
		reg_val &= (~TC_DRVCTRL_INVEN_Msk);
		reg_val |= invert_mask;
		PWM_MODE16(pwm_reg)->TC_DRVCTRL = reg_val;
		break;

	case BIT_MODE_32:
		reg_val = PWM_MODE32(pwm_reg)->TC_DRVCTRL;
		reg_val &= (~TC_DRVCTRL_INVEN_Msk);
		reg_val |= invert_mask;
		PWM_MODE32(pwm_reg)->TC_DRVCTRL = reg_val;
		break;

	default:
		tc_enable(pwm_reg, max_bit_width, true);
		tc_sync_wait(pwm_reg, max_bit_width);
		LOG_ERR("%s : Unsupported PWM mode %d", __func__, max_bit_width);
		return -ENOTSUP;
	}
	tc_enable(pwm_reg, max_bit_width, true);
	tc_sync_wait(pwm_reg, max_bit_width);

	return MCHP_PWM_SUCCESS;
}

/*
 * This function retrieves the invert status for the specified channel based on
 * the max_bit_width. It reads the invert status from the TC_DRVCTRL register and
 * checks if the invert mask is set. Returns true if the invert status is not
 * set, otherwise returns false.
 */
static bool tc_get_invert_status(const void *pwm_reg, const uint32_t max_bit_width,
				 uint32_t channel)
{
	uint32_t invert_status = 0;
	uint32_t invert_mask = 1 << (channel + TC_DRVCTRL_INVEN0_Pos);

	LOG_DBG("mchp_pwm_get_invert_status 0x%x invoked", invert_mask);
	switch (max_bit_width) {
	case BIT_MODE_8:
		invert_status = PWM_MODE8(pwm_reg)->TC_DRVCTRL & invert_mask;
		break;

	case BIT_MODE_16:
		invert_status = PWM_MODE16(pwm_reg)->TC_DRVCTRL & invert_mask;
		break;

	case BIT_MODE_32:
		invert_status = PWM_MODE32(pwm_reg)->TC_DRVCTRL & invert_mask;
		break;

	default:
		LOG_ERR("%s : Unsupported PWM mode %d", __func__, max_bit_width);
		break;
	}

	return (invert_status == 0) ? true : false;
}

/*
 * This function sets the prescaler value for the TC based on the max_bit_width.
 * It writes the prescaler value to the TC_CTRLA register and also sets the configuration
 * for reloading/resetting the counter on next prescaler clock Position.
 * After setting the prescaler, it waits for synchronization to complete.
 */
static int32_t tc_set_prescaler(const void *pwm_reg, const uint32_t max_bit_width,
				uint32_t prescaler)
{
	uint32_t reg_val = 0;
	bool slave_mode = check_slave_status(pwm_reg);

	if (slave_mode == true) {
		LOG_ERR("tc is in slave mode");
		return -EBUSY;
	}
	prescaler = tc_get_prescale_val(prescaler);
	switch (max_bit_width) {
	case BIT_MODE_8:
		reg_val = PWM_MODE8(pwm_reg)->TC_CTRLA;
		reg_val &= ~(TC_CTRLA_PRESCSYNC_Msk | TC_CTRLA_PRESCALER_Msk);
		reg_val |= (prescaler | TC_CTRLA_PRESCSYNC_PRESC);
		PWM_MODE8(pwm_reg)->TC_CTRLA = reg_val;
		break;

	case BIT_MODE_16:
		reg_val = PWM_MODE16(pwm_reg)->TC_CTRLA;
		reg_val &= ~(TC_CTRLA_PRESCSYNC_Msk | TC_CTRLA_PRESCALER_Msk);
		reg_val |= (prescaler | TC_CTRLA_PRESCSYNC_PRESC);
		PWM_MODE16(pwm_reg)->TC_CTRLA = reg_val;
		break;

	case BIT_MODE_32:
		reg_val = PWM_MODE32(pwm_reg)->TC_CTRLA;
		reg_val &= ~(TC_CTRLA_PRESCSYNC_Msk | TC_CTRLA_PRESCALER_Msk);
		reg_val |= (prescaler | TC_CTRLA_PRESCSYNC_PRESC);
		PWM_MODE32(pwm_reg)->TC_CTRLA = reg_val;
		break;

	default:
		LOG_ERR("%s : Unsupported PWM mode %d", __func__, max_bit_width);
		return -ENOTSUP;
	}
	tc_sync_wait(pwm_reg, max_bit_width);

	return MCHP_PWM_SUCCESS;
}

/*
 * This function sets the wave generation type for the TC based on the
 * max_bit_width. It writes the appropriate wave generation value to the TC_WAVE
 * register. After setting the wave type, it waits for synchronization to
 * complete.
 *
 * In 16-bit/32-bit mode, the PWM wave type is set as MPWM mode as default. This is because the MAX
 * value of the counter can be controlled in that mode only for setting proper period.
 */
static int32_t tc_set_wave_type(const void *pwm_reg, const uint32_t max_bit_width,
				uint32_t wave_type)
{
	bool slave_mode = check_slave_status(pwm_reg);

	if (slave_mode == true) {
		LOG_ERR("tc is in slave mode");
		return -EBUSY;
	}

	switch (max_bit_width) {
	case BIT_MODE_8:
		PWM_MODE8(pwm_reg)->TC_WAVE = TC_WAVE_WAVEGEN(wave_type);
		break;

	case BIT_MODE_16:
		PWM_MODE16(pwm_reg)->TC_WAVE = TC_WAVE_WAVEGEN(TC_WAVE_WAVEGEN_MPWM);
		break;

	case BIT_MODE_32:
		PWM_MODE32(pwm_reg)->TC_WAVE = TC_WAVE_WAVEGEN(TC_WAVE_WAVEGEN_MPWM);
		break;

	default:
		LOG_ERR("%s : Unsupported PWM mode %d", __func__, max_bit_width);
		return -ENOTSUP;
	}
	tc_sync_wait(pwm_reg, max_bit_width);
	LOG_DBG("%s invoked", __func__);

	return MCHP_PWM_SUCCESS;
}

/*
 * Initializes the TC for PWM by performing the following steps:
 * 1. Resets the TC registers.
 * 2. Sets the TC mode.
 * 3. Sets the prescaler value.
 * 4. Sets the wave generation type to NPWM.
 * 5. Sets the period to 0.
 * 6. Enables the TC.
 */
static int tc_init(const struct pwm_mchp_config *const mchp_pwm_cfg)
{
	const void *pwm_reg = mchp_pwm_cfg->regs;
	const uint32_t max_bit_width = mchp_pwm_cfg->max_bit_width;
	int ret;

	ret = tc_reset_regs(pwm_reg, max_bit_width);
	if (ret != MCHP_PWM_SUCCESS) {
		return ret;
	}

	ret = tc_set_mode(pwm_reg, max_bit_width);
	if (ret != MCHP_PWM_SUCCESS) {
		return ret;
	}

	ret = tc_set_prescaler(pwm_reg, max_bit_width, mchp_pwm_cfg->prescaler);
	if (ret != MCHP_PWM_SUCCESS) {
		return ret;
	}

	ret = tc_set_wave_type(pwm_reg, max_bit_width, TC_WAVE_WAVEGEN_NPWM);
	if (ret != MCHP_PWM_SUCCESS) {
		return ret;
	}

	ret = tc_set_period(pwm_reg, max_bit_width, 0);
	if (ret != MCHP_PWM_SUCCESS) {
		return ret;
	}

	ret = tc_enable(pwm_reg, max_bit_width, true);
	if (ret != MCHP_PWM_SUCCESS) {
		return ret;
	}

	return MCHP_PWM_SUCCESS;
}

static int pwm_mchp_set_cycles(const struct device *pwm_dev, uint32_t channel, uint32_t period,
			       uint32_t pulse, pwm_flags_t flags)
{

	const struct pwm_mchp_config *const mchp_pwm_cfg = pwm_dev->config;

	struct pwm_mchp_data *mchp_pwm_data = pwm_dev->data;
	const void *pwm_reg = mchp_pwm_cfg->regs;
	const uint32_t max_bit_width = mchp_pwm_cfg->max_bit_width;
	uint64_t top = BIT64(max_bit_width) - 1;
	int ret_val;

	k_mutex_lock(&mchp_pwm_data->lock, MCHP_PWM_LOCK_TIMEOUT);
	bool invert_flag_set = ((flags & PWM_POLARITY_INVERTED) != 0);
	bool not_inverted = tc_get_invert_status(pwm_reg, max_bit_width, channel);

	if ((invert_flag_set == true) && (not_inverted == true)) {
		ret_val = tc_set_invert(pwm_reg, max_bit_width, channel);
		if (ret_val < 0) {
			LOG_ERR("PWM peripheral busy");
			return -EBUSY;
		}
	}

	if (channel >= mchp_pwm_cfg->channels) {
		LOG_ERR("channel %d is invalid", channel);
		return -EINVAL;
	}

	if ((period > top) || (pulse > top)) {
		LOG_ERR("period or pulse is out of range");
		return -EINVAL;
	}

	ret_val = tc_set_pulse_buf(pwm_reg, max_bit_width, channel, pulse);
	if (ret_val < 0) {
		LOG_ERR("PWM peripheral busy");
		return -EBUSY;
	}
	ret_val = tc_set_period_buf(pwm_reg, max_bit_width, period);
	if (ret_val < 0) {
		LOG_ERR("PWM peripheral busy");
		return -EBUSY;
	}
	k_mutex_unlock(&mchp_pwm_data->lock);

	return ret_val;
}

static int pwm_mchp_get_cycles_per_sec(const struct device *pwm_dev, uint32_t channel,
				       uint64_t *cycles)
{
	const struct pwm_mchp_config *const mchp_pwm_cfg = pwm_dev->config;
	struct pwm_mchp_data *mchp_pwm_data = pwm_dev->data;
	uint32_t periph_clk_freq = 0;
	int ret_val;

	if (channel >= (mchp_pwm_cfg->channels)) {
		LOG_ERR("channel %d is invalid", channel);
		return -EINVAL;
	}
	k_mutex_lock(&mchp_pwm_data->lock, MCHP_PWM_LOCK_TIMEOUT);

	ret_val = clock_control_get_rate(mchp_pwm_cfg->pwm_clock.clock_dev,
					 mchp_pwm_cfg->pwm_clock.host_gclk, &periph_clk_freq);
	if (ret_val < 0) {
		LOG_ERR("clock get rate failed");
		return ret_val;
	}

	*cycles = periph_clk_freq / mchp_pwm_cfg->prescaler;

	k_mutex_unlock(&mchp_pwm_data->lock);

	return MCHP_PWM_SUCCESS;
}

static int pwm_mchp_init(const struct device *pwm_dev)
{
	int ret_val;
	const struct pwm_mchp_config *const mchp_pwm_cfg = pwm_dev->config;
	struct pwm_mchp_data *mchp_pwm_data = pwm_dev->data;

	k_mutex_init(&mchp_pwm_data->lock);

	ret_val = clock_control_on(mchp_pwm_cfg->pwm_clock.clock_dev,
				   mchp_pwm_cfg->pwm_clock.host_gclk);
	if ((ret_val < 0) && (ret_val != -EALREADY)) {
		LOG_ERR("Failed to enable the host_gclk for PWM: %d", ret_val);
		return ret_val;
	}
	ret_val = clock_control_on(mchp_pwm_cfg->pwm_clock.clock_dev,
				   mchp_pwm_cfg->pwm_clock.host_mclk);
	if ((ret_val < 0) && (ret_val != -EALREADY)) {
		LOG_ERR("Failed to enable the host_mclk for PWM: %d", ret_val);
		return ret_val;
	}
	/* If the mode is 32 bit the turn on the clock of the client peripheral as well.
	 * If the client clock is not provided in the device tree that means it is not
	 * supported for that particular instance. The MCLK of the client peripheral is to
	 * be turned on in case 32 bit mode is to be enabled
	 */
	if (mchp_pwm_cfg->max_bit_width == BIT_MODE_32) {
		if (mchp_pwm_cfg->pwm_clock.client_mclk != NULL) {

			ret_val = clock_control_on(mchp_pwm_cfg->pwm_clock.clock_dev,
						   (mchp_pwm_cfg->pwm_clock.client_mclk));
			if ((ret_val < 0) && (ret_val != -EALREADY)) {
				LOG_ERR("Failed to enable the client_mclk: %d", ret_val);
				return ret_val;
			}
		} else {
			LOG_ERR("Peripheral does not support 32 bit mode");
			return -ENOTSUP;
		}
	}

	ret_val = pinctrl_apply_state(mchp_pwm_cfg->pinctrl_config, PINCTRL_STATE_DEFAULT);
	if (ret_val < 0) {
		LOG_ERR("pincontrol apply state failed: %d", ret_val);
		return ret_val;
	}
	ret_val = tc_init(mchp_pwm_cfg);
	ret_val = (ret_val == -EALREADY) ? 0 : ret_val;

	return ret_val;
}

static DEVICE_API(pwm, pwm_mchp_api) = {
	.set_cycles = pwm_mchp_set_cycles,
	.get_cycles_per_sec = pwm_mchp_get_cycles_per_sec,
};

#define PWM_MCHP_DATA_DEFN(n) static struct pwm_mchp_data pwm_mchp_data_##n

/* clang-format off */
#define GET_THE_CLIENT_MCLOCK_IF_AVAILABLE(n)						\
	COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(n, client_mclk),				\
	((void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, client_mclk, subsystem))),		\
	NULL)

#define PWM_MCHP_CLOCK_ASSIGN(n)							\
	.pwm_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),			\
	.pwm_clock.host_mclk = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),\
	.pwm_clock.host_gclk = (void *)DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem),	\
	.pwm_clock.client_mclk = GET_THE_CLIENT_MCLOCK_IF_AVAILABLE(n)

#define PWM_MCHP_CONFIG_DEFN(n)						\
	static const struct pwm_mchp_config pwm_mchp_config_##n = {     \
		.prescaler = DT_INST_PROP(n, prescaler),                \
		.pinctrl_config = PINCTRL_DT_INST_DEV_CONFIG_GET(n),    \
		.channels = DT_INST_PROP(n, channels),                  \
		.regs = (void *)DT_INST_REG_ADDR(n),                    \
		.max_bit_width = DT_INST_PROP(n, max_bit_width),        \
		PWM_MCHP_CLOCK_ASSIGN(n)}


#define PWM_MCHP_DEVICE_DT_DEFN(n)								\
	DEVICE_DT_INST_DEFINE(n, pwm_mchp_init, NULL, &pwm_mchp_data_##n, &pwm_mchp_config_##n,	\
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, &pwm_mchp_api)

#define PWM_MCHP_DEVICE_INIT(n)		\
	PINCTRL_DT_INST_DEFINE(n);	\
	PWM_MCHP_DATA_DEFN(n);          \
	PWM_MCHP_CONFIG_DEFN(n);        \
	PWM_MCHP_DEVICE_DT_DEFN(n);

/* clang-format on */
DT_INST_FOREACH_STATUS_OKAY(PWM_MCHP_DEVICE_INIT)
