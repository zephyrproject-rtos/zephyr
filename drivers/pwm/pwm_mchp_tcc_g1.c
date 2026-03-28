/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file pwm_mchp_tcc_g1.c
 * @brief PWM driver for Microchip tcc g1 peripheral.
 *
 * This file provides the implementation of pwm functions
 * for Microchip tcc g1 Peripheral.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>

/******************************************************************************
 * @brief Devicetree definitions
 *****************************************************************************/
#define DT_DRV_COMPAT microchip_tcc_g1_pwm

/*******************************************
 * Const and Macro Defines
 *******************************************/

LOG_MODULE_REGISTER(pwm_mchp_tcc_g1, CONFIG_PWM_LOG_LEVEL);

#define PWM_REG(addr) ((tcc_registers_t *)addr)

#define MCHP_PWM_SUCCESS 0

/**
 * @brief Timeout duration for acquiring the PWM lock.
 *
 * This macro defines the timeout duration for acquiring the PWM lock.
 * The timeout is specified in milliseconds.
 */
#define MCHP_PWM_LOCK_TIMEOUT K_MSEC(10)

/**
 * @brief Initialize the PWM lock.
 *
 * This macro initializes the PWM lock.
 *
 * @param p_lock Pointer to the lock to be initialized.
 */
#define MCHP_PWM_DATA_LOCK_INIT(p_lock) k_mutex_init(p_lock)

/**
 * @brief Acquire the PWM lock.
 *
 * This macro acquires the PWM lock. If the lock is not available, the
 * function will wait for the specified timeout duration.
 *
 * @param p_lock Pointer to the lock to be acquired.
 * @return 0 if the lock was successfully acquired, or a negative error code.
 */
#define MCHP_PWM_DATA_LOCK(p_lock) k_mutex_lock(p_lock, MCHP_PWM_LOCK_TIMEOUT)

/**
 * @brief Release the PWM lock.
 *
 * This macro releases the PWM lock.
 *
 * @param p_lock Pointer to the lock to be released.
 * @return 0 if the lock was successfully released, or a negative error code.
 */
#define MCHP_PWM_DATA_UNLOCK(p_lock) k_mutex_unlock(p_lock)

/* Timeout values for WAIT_FOR macro */
#define TIMEOUT_VALUE_US 1000
#define DELAY_US         2

/***********************************
 * Typedefs and Enum Declarations
 **********************************/
typedef enum {
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
} pwm_prescale_modes_t;

/**
 * @brief Structure for managing flags for the pwm.
 */
typedef enum {
	PWM_MCHP_FLAGS_CAPTURE_TYPE_PERIOD,
	PWM_MCHP_FLAGS_CAPTURE_TYPE_PULSE,
	PWM_MCHP_FLAGS_CAPTURE_TYPE_BOTH,
	PWM_MCHP_FLAGS_CAPTURE_MODE_SINGLE,
	PWM_MCHP_FLAGS_CAPTURE_MODE_CONTINUOUS,
} pwm_mchp_flags_t;

/**
 * @brief Structure to hold PWM data specific to Microchip hardware.
 */
typedef struct {
	struct k_mutex lock; /* Lock type for PWM configuration */
} pwm_mchp_data_t;

typedef struct mchp_counter_clock {
	/* Clock driver */
	const struct device *clock_dev;

	/* Main clock subsystem. */
	clock_control_subsys_t host_core_sync_clk;
	/* Generic clock subsystem. */
	clock_control_subsys_t periph_async_clk;
} pwm_mchp_clock_t;

/**
 * @brief Structure to hold the configuration for Microchip PWM.
 */
typedef struct {
	pwm_mchp_clock_t pwm_clock;                      /* PWM clock configuration */
	const struct pinctrl_dev_config *pinctrl_config; /* Pin control configuration */
	void *regs;                                      /* Pointer to PWM peripheral registers */
	uint32_t max_bit_width; /* Used for finding out the resolution of the pwm peripheral */
	uint16_t prescaler;     /* Prescaler value for PWM */
	uint8_t channels;       /* Number of PWM channels */
	uint32_t freq;          /* Frequency of the PWM signal */
} pwm_mchp_config_t;

/***********************************
 * Internal functions
 ***********************************/

/**
 *Get the prescale value based on the given prescaler.
 */
static uint32_t tcc_get_prescale_val(uint32_t prescaler)
{
	uint32_t prescaler_val = 0;

	switch (prescaler) {
	case PWM_PRESCALE_1:
		prescaler_val = TCC_CTRLA_PRESCALER_DIV1;
		break;
	case PWM_PRESCALE_2:
		prescaler_val = TCC_CTRLA_PRESCALER_DIV2;
		break;
	case PWM_PRESCALE_4:
		prescaler_val = TCC_CTRLA_PRESCALER_DIV4;
		break;
	case PWM_PRESCALE_8:
		prescaler_val = TCC_CTRLA_PRESCALER_DIV8;
		break;
	case PWM_PRESCALE_16:
		prescaler_val = TCC_CTRLA_PRESCALER_DIV16;
		break;
	case PWM_PRESCALE_64:
		prescaler_val = TCC_CTRLA_PRESCALER_DIV64;
		break;
	case PWM_PRESCALE_256:
		prescaler_val = TCC_CTRLA_PRESCALER_DIV256;
		break;
	case PWM_PRESCALE_1024:
		prescaler_val = TCC_CTRLA_PRESCALER_DIV1024;
		break;
	default:
		prescaler_val = TCC_CTRLA_PRESCALER_DIV1; /* Default fallback */
		LOG_ERR("Unsupported prescaler specified in dts. Initialising with default "
			"prescaler of DIV1");

		break;
	}

	return prescaler_val;
}

/**
 *Enable or disable the PWM instance.
 */
static inline void tcc_enable(void *pwm_reg, bool enable)
{

	if (enable != 0) {
		PWM_REG(pwm_reg)->TCC_CTRLA |= TCC_CTRLA_ENABLE(1);
	} else {
		PWM_REG(pwm_reg)->TCC_CTRLA &= ~TCC_CTRLA_ENABLE(1);
	}
	LOG_DBG("%s %d invoked", __func__, enable);
}

/**
 *Wait for the PWM instance to complete synchronization.
 */
static inline void tcc_sync_wait(void *pwm_reg)
{

	if (!WAIT_FOR(((PWM_REG(pwm_reg)->TCC_SYNCBUSY) != 0), TIMEOUT_VALUE_US,
		      k_busy_wait(DELAY_US))) {
		LOG_ERR("TCC_SYNCBUSY wait timed out");
	}
	LOG_DBG("%s invoked", __func__);
}

/**
 *Set the output inversion for a specific PWM channel.
 */
static int32_t tcc_set_invert(void *pwm_reg, uint32_t channel)
{
	uint32_t invert_mask = 1 << (channel + TCC_DRVCTRL_INVEN0_Pos);

	tcc_enable(pwm_reg, false);
	tcc_sync_wait(pwm_reg);
	PWM_REG(pwm_reg)->TCC_DRVCTRL |= invert_mask;
	tcc_enable(pwm_reg, true);
	tcc_sync_wait(pwm_reg);
	LOG_DBG("tcc set invert 0x%x invoked", invert_mask);

	return MCHP_PWM_SUCCESS;
}

/**
 *Initialize the PWM instance with the specified prescaler.
 */
void tcc_init(void *pwm_reg, uint32_t prescaler)
{
	prescaler = tcc_get_prescale_val(prescaler);
	PWM_REG(pwm_reg)->TCC_CTRLA = TCC_CTRLA_SWRST(1);
	tcc_sync_wait(pwm_reg);
	PWM_REG(pwm_reg)->TCC_CTRLA |= prescaler;
	PWM_REG(pwm_reg)->TCC_WAVE = TCC_WAVE_WAVEGEN_NPWM;
	PWM_REG(pwm_reg)->TCC_PER = TCC_PER_PER(0);
	tcc_enable(pwm_reg, true);
}

/**
 *Get the output inversion status for a specific PWM channel.
 */
static inline bool tcc_get_invert_status(void *pwm_reg, uint32_t channel)
{
	uint32_t invert_status = 0;
	uint32_t invert_mask = 1 << (channel + TCC_DRVCTRL_INVEN0_Pos);

	LOG_DBG("tcc get invert status 0x%x invoked", invert_mask);
	invert_status = PWM_REG(pwm_reg)->TCC_DRVCTRL & invert_mask;

	return (invert_status == 0) ? true : false;
}

/***********************************
 * Zephyr APIs
 **********************************/
/**
 * @brief Set the PWM cycles for a specific channel.
 *
 * This function sets the PWM period and pulse width for a specified channel. It also handles the
 * polarity inversion if required.
 *
 * @param pwm_dev Pointer to the PWM device structure.
 * @param channel PWM channel number.
 * @param period PWM period in cycles.
 * @param pulse PWM pulse width in cycles.
 * @param flags PWM flags (e.g., polarity inversion).
 *
 * @return 0 on success, -EINVAL if the channel is invalid or the period/pulse is out of range.
 */
static int pwm_mchp_set_cycles(const struct device *pwm_dev, uint32_t channel, uint32_t period,
			       uint32_t pulse, pwm_flags_t flags)
{
	const pwm_mchp_config_t *const mchp_pwm_cfg = pwm_dev->config;
	pwm_mchp_data_t *mchp_pwm_data = pwm_dev->data;
	int ret_val = -EINVAL;
	uint32_t top = (BIT(mchp_pwm_cfg->max_bit_width) - 1);

	MCHP_PWM_DATA_LOCK(&mchp_pwm_data->lock);

	if (channel >= mchp_pwm_cfg->channels) {
		LOG_ERR("channel %d is invalid", channel);
	} else if ((period > top) || (pulse > top)) {
		LOG_ERR("period or pulse is out of range");
	} else {

		bool invert_flag_set = ((flags & PWM_POLARITY_INVERTED) != 0);
		bool not_inverted = tcc_get_invert_status(mchp_pwm_cfg->regs, channel);

		if ((invert_flag_set == true) && (not_inverted == true)) {
			tcc_set_invert(mchp_pwm_cfg->regs, channel);
		}

		PWM_REG(mchp_pwm_cfg->regs)->TCC_CCBUF[channel] = TCC_CCBUF_CCBUF(pulse);
		PWM_REG(mchp_pwm_cfg->regs)->TCC_PER = TCC_PER_PER(period);
		ret_val = MCHP_PWM_SUCCESS;
	}

	MCHP_PWM_DATA_UNLOCK(&mchp_pwm_data->lock);

	return ret_val;
}

/**
 * @brief Get the number of PWM cycles per second for a specific channel.
 *
 * This function retrieves the frequency of the PWM signal in cycles per second for a specified
 * channel.
 *
 * @param pwm_dev Pointer to the PWM device structure.
 * @param channel PWM channel number.
 * @param cycles Pointer to store the number of cycles per second.
 *
 * @return 0 on success, -EINVAL if the channel is invalid.
 */
static int pwm_mchp_get_cycles_per_sec(const struct device *pwm_dev, uint32_t channel,
				       uint64_t *cycles)
{
	const pwm_mchp_config_t *const mchp_pwm_cfg = pwm_dev->config;
	pwm_mchp_data_t *mchp_pwm_data = pwm_dev->data;
	uint32_t periph_clk_freq = 0;
	int ret_val = -EINVAL;

	MCHP_PWM_DATA_LOCK(&mchp_pwm_data->lock);

	if (channel < (mchp_pwm_cfg->channels)) {
		/* clang-format off */
		clock_control_get_rate(
			 mchp_pwm_cfg->pwm_clock.clock_dev,
			mchp_pwm_cfg->pwm_clock.periph_async_clk,
			&periph_clk_freq);
		/* clang-format on */
		*cycles = periph_clk_freq / mchp_pwm_cfg->prescaler;
		ret_val = MCHP_PWM_SUCCESS;
	} else {
		LOG_ERR("channel %d is invalid", channel);
	}

	MCHP_PWM_DATA_UNLOCK(&mchp_pwm_data->lock);

	return ret_val;
}

/******************************************************************************
 * @brief Zephyr driver instance creation
 *****************************************************************************/

/**
 * @brief PWM driver API structure for the Microchip PWM device.
 *
 * This structure defines the API functions for the Microchip PWM driver, including setting PWM
 * cycles, getting the number of cycles per second, and optionally configuring, enabling, and
 * disabling PWM capture.
 */
static DEVICE_API(pwm, pwm_mchp_api) = {
	.set_cycles = pwm_mchp_set_cycles,
	.get_cycles_per_sec = pwm_mchp_get_cycles_per_sec,
};

/**
 * @brief Initialize the Microchip PWM device.
 *
 * This function initializes the Microchip PWM device by applying the pin control configuration and
 * initializing the PWM hardware with the specified prescaler.
 *
 * @param pwm_dev Pointer to the PWM device structure.
 *
 * @return 0 on success, negative error code on failure.
 */
static int pwm_mchp_init(const struct device *pwm_dev)
{
	int ret_val;
	const pwm_mchp_config_t *const mchp_pwm_cfg = pwm_dev->config;
	pwm_mchp_data_t *mchp_pwm_data = pwm_dev->data;

	MCHP_PWM_DATA_LOCK_INIT(&mchp_pwm_data->lock);
	do {

		ret_val = clock_control_on(mchp_pwm_cfg->pwm_clock.clock_dev,
					   mchp_pwm_cfg->pwm_clock.periph_async_clk);
		if ((ret_val < 0) && (ret_val != -EALREADY)) {
			LOG_ERR("Failed to enable the periph_async_clk for PWM: %d", ret_val);
			break;
		}
		ret_val = clock_control_on(mchp_pwm_cfg->pwm_clock.clock_dev,
					   mchp_pwm_cfg->pwm_clock.host_core_sync_clk);
		if ((ret_val < 0) && (ret_val != -EALREADY)) {
			LOG_ERR("Failed to enable the host_core_sync_clk for PWM: %d", ret_val);
			break;
		}
		ret_val = pinctrl_apply_state(mchp_pwm_cfg->pinctrl_config, PINCTRL_STATE_DEFAULT);
		if (ret_val < 0) {
			LOG_ERR("Pincontrol apply state failed %d", ret_val);
			break;
		}
		tcc_init(mchp_pwm_cfg->regs, mchp_pwm_cfg->prescaler);
	} while (0);
	ret_val = (ret_val == -EALREADY) ? 0 : ret_val;

	return ret_val;
}

/**
 * @brief Macro to define the PWM data structure for a specific instance.
 *
 * This macro defines the PWM data structure for a specific instance of the Microchip PWM device.
 *
 * @param n Instance number.
 * @param ip IP identifier.
 */
#define PWM_MCHP_DATA_DEFN(n) static pwm_mchp_data_t pwm_mchp_data_##n

/**
 * @brief Macro to assign clock configurations for the Microchip PWM device.
 *
 * This macro assigns the clock configurations for the PWM device, including the
 * host core synchronous clock, and
 * peripheral asynchronous clock (conditionally).
 *
 * @param n Device tree node number.
 *
 * @note This macro conditionally includes  peripheral asynchronous clock configurations based on
 * the presence of relevant device tree properties.
 */
/* clang-format off */
#define PWM_MCHP_CLOCK_ASSIGN(n)                                         \
	.pwm_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),           \
	.pwm_clock.host_core_sync_clk = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),\
	COND_CODE_1(DT_NODE_EXISTS(DT_INST_CLOCKS_CTLR_BY_NAME(n, rtcclk)),			\
	(.pwm_clock.periph_async_clk =								\
	 (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, rtcclk, subsystem)),),				\
	()) COND_CODE_1(DT_NODE_EXISTS(DT_INST_CLOCKS_CTLR_BY_NAME(n, gclk)),               \
	(.pwm_clock.periph_async_clk = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, gclk, subsystem)),),\
	())
/* clang-format on */

/**
 * @brief Macro to define the PWM configuration structure for a specific instance.
 *
 * This macro defines the PWM configuration structure for a specific instance of the Microchip PWM
 * device.
 *
 * @param n Instance number.
 */
#define PWM_MCHP_CONFIG_DEFN(n)                                                                    \
	static const pwm_mchp_config_t pwm_mchp_config_##n = {                                     \
		.prescaler = DT_INST_PROP(n, prescaler),                                           \
		.pinctrl_config = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                               \
		.channels = DT_INST_PROP(n, channels),                                             \
		.regs = (void *)DT_INST_REG_ADDR(n),                                               \
		.max_bit_width = DT_INST_PROP(n, max_bit_width),                                   \
		PWM_MCHP_CLOCK_ASSIGN(n)}

/**
 * @brief Macro to define the device structure for a specific instance of the PWM device.
 *
 * This macro defines the device structure for a specific instance of the Microchip PWM device.
 * It uses the DEVICE_DT_INST_DEFINE macro to create the device instance with the specified
 * initialization function, data structure, configuration structure, and driver API.
 *
 * @param n Instance number.
 */
#define PWM_MCHP_DEVICE_DT_DEFN(n)                                                                 \
	DEVICE_DT_INST_DEFINE(n, pwm_mchp_init, NULL, &pwm_mchp_data_##n, &pwm_mchp_config_##n,    \
			      POST_KERNEL, CONFIG_PWM_INIT_PRIORITY, &pwm_mchp_api)

/**
 *Initialize the PWM device with pin control, data, and configuration definitions.
 */
#define PWM_MCHP_DEVICE_INIT(n)                                                                    \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	PWM_MCHP_DATA_DEFN(n);                                                                     \
	PWM_MCHP_CONFIG_DEFN(n);                                                                   \
	PWM_MCHP_DEVICE_DT_DEFN(n);

/* Run init macro for each pwm-generic node */
DT_INST_FOREACH_STATUS_OKAY(PWM_MCHP_DEVICE_INIT)
