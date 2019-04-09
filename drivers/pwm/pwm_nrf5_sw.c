/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/pwm.h>
#include <nrf_timer.h>
#include <nrf_gpio.h>
#include <nrf_gpiote.h>
#include <nrf_ppi.h>

#define LOG_LEVEL CONFIG_PWM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(pwm_nrf5_sw);

#define TIMER_INSTANCE \
	_CONCAT(TIMER, DT_INST_0_NORDIC_NRF_SW_PWM_TIMER_INSTANCE)
#define TIMER_REGS  _CONCAT(NRF_, TIMER_INSTANCE)
#define GET_TIMER_CAPABILITY(capability) \
	(_CONCAT(_CONCAT(TIMER_INSTANCE, _), capability))

/* One compare channel is needed to set the PWM period, hence +1. */
#if ((DT_INST_0_NORDIC_NRF_SW_PWM_CHANNEL_COUNT + 1) > \
     GET_TIMER_CAPABILITY(CC_NUM))
#error "Invalid number of PWM channels configured."
#endif
#define PWM_MAP_SIZE  DT_INST_0_NORDIC_NRF_SW_PWM_CHANNEL_COUNT

/* Nordic TIMER peripherals allow prescalers 0-9. */
#define MAX_TIMER_PRESCALER  9
/* Nordic TIMERs can be 16- or 32-bit wide. */
#define MAX_TIMER_VALUE  (GET_TIMER_CAPABILITY(MAX_SIZE) == 32 ? UINT32_MAX \
							       : UINT16_MAX)

#if (GET_TIMER_CAPABILITY(CC_NUM) == 6)
#define PWM_PERIOD_TIMER_CHANNEL  5
#else
#define PWM_PERIOD_TIMER_CHANNEL  3
#endif
#define PWM_PERIOD_TIMER_SHORT  _CONCAT(_CONCAT(NRF_TIMER_SHORT_COMPARE, \
						PWM_PERIOD_TIMER_CHANNEL), \
					_CLEAR_MASK)

struct chan_map {
	u32_t pwm;
	u32_t pulse_cycles;
};

struct pwm_data {
	u32_t period_cycles;
	struct chan_map map[PWM_MAP_SIZE];
};

static u32_t pwm_period_check(struct pwm_data *data, u32_t pwm,
			      u32_t period_cycles, u32_t pulse_cycles)
{
	u8_t i;

	/* allow 0% and 100% duty cycle, as it does not use PWM. */
	if ((pulse_cycles == 0U) || (pulse_cycles == period_cycles)) {
		return 0;
	}

	/* fail if requested period does not match already running period */
	for (i = 0U; i < PWM_MAP_SIZE; i++) {
		if ((data->map[i].pwm != pwm) &&
		    (data->map[i].pulse_cycles != 0U) &&
		    (period_cycles != data->period_cycles)) {
			return -EINVAL;
		}
	}

	return 0;
}

static u8_t pwm_channel_map(struct pwm_data *data, u32_t pwm)
{
	u8_t i;

	/* find pin, if already present */
	for (i = 0U; i < PWM_MAP_SIZE; i++) {
		if (pwm == data->map[i].pwm) {
			return i;
		}
	}

	/* find a free entry */
	i = PWM_MAP_SIZE;
	while (i--) {
		if (data->map[i].pulse_cycles == 0U) {
			break;
		}
	}

	return i;
}

static u8_t pwm_find_prescaler(u32_t period_cycles)
{
	u8_t prescaler;

	for (prescaler = 0; prescaler <= MAX_TIMER_PRESCALER; ++prescaler) {
		if (period_cycles <= MAX_TIMER_VALUE) {
			break;
		}

		period_cycles >>= 1;
	}

	return prescaler;
}

static int pwm_nrf5_sw_pin_set(struct device *dev, u32_t pwm,
			       u32_t period_cycles, u32_t pulse_cycles)
{
	struct pwm_data *data = dev->driver_data;
	u8_t gpiote_index;
	u8_t ppi_index;
	u8_t channel;
	u8_t prescaler;
	u32_t ret;

	/* check if requested period is allowed while other channels are
	 * active.
	 */
	ret = pwm_period_check(data, pwm, period_cycles, pulse_cycles);
	if (ret) {
		LOG_ERR("Incompatible period");
		return ret;
	}

	/* map pwm pin to GPIOTE config/channel */
	channel = pwm_channel_map(data, pwm);
	if (channel >= PWM_MAP_SIZE) {
		LOG_ERR("No more channels available");
		return -ENOMEM;
	}

	prescaler = pwm_find_prescaler(period_cycles);
	if (prescaler > MAX_TIMER_PRESCALER) {
		LOG_ERR("Prescaler for period_cycles %u not found.",
			period_cycles);
		return -EINVAL;
	}

	LOG_DBG("PWM %d, period %u, pulse %u", pwm,
			period_cycles, pulse_cycles);

	/* clear GPIOTE config */
	gpiote_index = DT_INST_0_NORDIC_NRF_SW_PWM_GPIOTE_BASE + channel;
	nrf_gpiote_te_default(gpiote_index);

	/* clear PPI used */
	ppi_index = DT_INST_0_NORDIC_NRF_SW_PWM_PPI_BASE + (channel << 1);
	nrf_ppi_channels_disable(BIT(ppi_index) | BIT(ppi_index + 1));

	/* configure GPIO pin as output */
	nrf_gpio_cfg_output(pwm);
	if (pulse_cycles == 0U) {
		/* 0% duty cycle, keep pin low */
		nrf_gpio_pin_clear(pwm);

		goto pin_set_pwm_off;
	} else if (pulse_cycles == period_cycles) {
		/* 100% duty cycle, keep pin high */
		nrf_gpio_pin_set(pwm);

		goto pin_set_pwm_off;
	} else {
		/* x% duty cycle, start PWM with pin low */
		nrf_gpio_pin_clear(pwm);
	}


	/* The TIMER must be stopped during its reconfiguration, otherwise we
	 * may end up with an inversed PWM when the period compare event occurs
	 * before the pulse compare event, since the GPIO is toggled on both
	 * these events, and not set on the period one and reset on the other.
	 */
	nrf_timer_task_trigger(TIMER_REGS, NRF_TIMER_TASK_STOP)

	nrf_timer_frequency_set(TIMER_REGS, (nrf_timer_frequency_t)prescaler);
	nrf_timer_cc_write(TIMER_REGS, channel, pulse_cycles >> prescaler);
	nrf_timer_cc_write(TIMER_REGS, PWM_MAP_SIZE,
		period_cycles >> prescaler);
	nrf_timer_task_trigger(TIMER_REGS, NRF_TIMER_TASK_CLEAR);

	nrf_gpiote_task_configure(gpiote_index, pwm,
		NRF_GPIOTE_POLARITY_TOGGLE, NRF_GPIOTE_INITIAL_VALUE_HIGH);
	nrf_gpiote_task_enable(gpiote_index);

	/* setup PPI */
	{
		nrf_timer_event_t channel_event =
			nrf_timer_compare_event_get(channel);
		nrf_timer_event_t period_event =
			nrf_timer_compare_event_get(PWM_PERIOD_TIMER_CHANNEL);
		u32_t gpiote_task_address =
			(u32_t)&(NRF_GPIOTE->TASKS_OUT[gpiote_index]);
		nrf_ppi_channel_endpoint_setup(
			ppi_index,
			(u32_t)nrf_timer_event_address_get(TIMER_REGS,
							   channel_event),
			gpiote_task_address);
		nrf_ppi_channel_endpoint_setup(
			ppi_index + 1,
			(u32_t)nrf_timer_event_address_get(TIMER_REGS,
							   period_event),
			gpiote_task_address);
		nrf_ppi_channels_enable(BIT(ppi_index) | BIT(ppi_index + 1));
	}

	/* start timer, hence PWM */
	nrf_timer_task_trigger(TIMER_REGS, NRF_TIMER_TASK_START);

	/* store the pwm/pin and its param */
	data->period_cycles = period_cycles;
	data->map[channel].pwm = pwm;
	data->map[channel].pulse_cycles = pulse_cycles;

	return 0;

pin_set_pwm_off:
	data->map[channel].pulse_cycles = 0U;
	bool pwm_active = false;

	/* stop timer if all channels are inactive */
	for (channel = 0U; channel < PWM_MAP_SIZE; channel++) {
		if (data->map[channel].pulse_cycles) {
			pwm_active = true;
			break;
		}
	}

	if (!pwm_active) {
		/* No active PWM, stop timer */
		nrf_timer_task_trigger(TIMER_REGS, NRF_TIMER_TASK_STOP);
	}

	return 0;
}

static int pwm_nrf5_sw_get_cycles_per_sec(struct device *dev, u32_t pwm,
					  u64_t *cycles)
{
	/* Since this function might be removed (see issue #6958), the maximum
	 * supported frequency (16 MHz) is always returned here, and dynamically
	 * adjusted timer prescalers are used in the pin set function.
	 */
	*cycles = 16ul * 1000ul * 1000ul;

	return 0;
}

static const struct pwm_driver_api pwm_nrf5_sw_drv_api_funcs = {
	.pin_set = pwm_nrf5_sw_pin_set,
	.get_cycles_per_sec = pwm_nrf5_sw_get_cycles_per_sec,
};

static int pwm_nrf5_sw_init(struct device *dev)
{
	/* setup HF timer */
	nrf_timer_mode_set(TIMER_REGS, NRF_TIMER_MODE_TIMER);
	nrf_timer_bit_width_set(TIMER_REGS,
		GET_TIMER_CAPABILITY(MAX_SIZE) == 32 ? NRF_TIMER_BIT_WIDTH_32
						     : NRF_TIMER_BIT_WIDTH_16);

	/* The last compare channel is used for setting the PWM period.
	 * Enable the shortcut that will clear the timer on the compare event
	 * on this channel.
	 */
	nrf_timer_shorts_enable(TIMER_REGS, PWM_PERIOD_TIMER_SHORT);

	return 0;
}

static struct pwm_data pwm_nrf5_sw_0_data;

DEVICE_AND_API_INIT(pwm_nrf5_sw_0,
		    CONFIG_PWM_NRF5_SW_0_DEV_NAME,
		    pwm_nrf5_sw_init,
		    &pwm_nrf5_sw_0_data,
		    NULL,
		    POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &pwm_nrf5_sw_drv_api_funcs);
