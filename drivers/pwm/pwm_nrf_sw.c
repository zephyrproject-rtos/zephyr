/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/pwm.h>
#include <nrf_timer.h>
#include <nrf_gpio.h>
#include <nrf_gpiote.h>
#if defined(PPI_PRESENT)
#include <nrfx_ppi.h>
#else
#include <nrfx_dppi.h>
#endif
#include <logging/log.h>

LOG_MODULE_REGISTER(pwm_nrf_sw, CONFIG_PWM_LOG_LEVEL);

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
	u32_t pin;
	u32_t pulse_cycles;
};

struct pwm_data {
	struct chan_map map[PWM_MAP_SIZE];
	u32_t period_cycles;
	u8_t  timer_prescaler;
};

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

static int pwm_nrf_sw_pin_set(struct device *dev, u32_t pin,
			      u32_t period_cycles, u32_t pulse_cycles)
{
	struct pwm_data *data = dev->driver_data;
	u8_t i;
	u8_t channel;
	u8_t gpiote_index;
	bool other_channel_active;
	bool reconfigure_period = false;

	/* Find a channel for driving the requested pin. If there is one already
	 * associated with the pin, use it. If not, use the first one that is
	 * free. Always look through the whole map to see if any other channel
	 * is active.
	 */
	other_channel_active = false;
	channel = PWM_MAP_SIZE;
	for (i = 0; i <  PWM_MAP_SIZE; ++i) {
		if (data->map[i].pin == pin) {
			channel = i;
		} else if (data->map[i].pulse_cycles != 0) {
			other_channel_active = true;
		} else if (channel == PWM_MAP_SIZE) {
			channel = i;
		}
	}
	if (channel == PWM_MAP_SIZE) {
		LOG_ERR("No more channels available.");
		return -ENOMEM;
	}

	gpiote_index = DT_INST_0_NORDIC_NRF_SW_PWM_GPIOTE_BASE + channel;

	if (pulse_cycles == 0 || pulse_cycles >= period_cycles) {
		if (pulse_cycles == 0) {
			/* 0% duty cycle, keep pin low. */
			nrf_gpio_pin_clear(pin);

			LOG_DBG("pin %u, 0%%.", pin);
		} else {
			/* 100% duty cycle, keep pin high. */
			nrf_gpio_pin_set(pin);

			LOG_DBG("pin %u, 100%%.", pin);
		}
		nrf_gpio_cfg_output(pin);
		/* Let GPIO take over the control of the pin. */
		nrf_gpiote_te_default(gpiote_index);

		data->map[channel].pulse_cycles = 0;

		if (!other_channel_active) {
			nrf_timer_task_trigger(TIMER_REGS, NRF_TIMER_TASK_STOP);
			data->period_cycles = 0;
		}

		return 0;
	}

	if (period_cycles != data->period_cycles) {
		u8_t prescaler;

		if (other_channel_active) {
			LOG_ERR("Incompatible period.");
			return -EINVAL;
		}

		prescaler = pwm_find_prescaler(period_cycles);
		if (prescaler > MAX_TIMER_PRESCALER) {
			LOG_ERR("Prescaler for period_cycles %u not found.",
				period_cycles);
			return -EINVAL;
		}

		data->period_cycles   = period_cycles;
		data->timer_prescaler = prescaler;
		reconfigure_period = true;
	}

	data->map[channel].pin = pin;
	data->map[channel].pulse_cycles = pulse_cycles;

	LOG_DBG("pin %u, pulse %u, period %u, prescaler: %u.",
		data->map[channel].pin, data->map[channel].pulse_cycles,
		data->period_cycles, data->timer_prescaler);

	/* The TIMER must be stopped during its reconfiguration, otherwise we
	 * may end up with an inverted PWM when the period compare event occurs
	 * before the pulse compare event, since the GPIO is toggled on both
	 * these events, and not set on the period one and reset on the other.
	 */
	nrf_timer_task_trigger(TIMER_REGS, NRF_TIMER_TASK_STOP);

	if (reconfigure_period) {
		nrf_timer_frequency_set(TIMER_REGS,
			(nrf_timer_frequency_t)data->timer_prescaler);
		nrf_timer_cc_write(TIMER_REGS, PWM_PERIOD_TIMER_CHANNEL,
			data->period_cycles >> data->timer_prescaler);
	}

	nrf_timer_cc_write(TIMER_REGS, channel,
		pulse_cycles >> data->timer_prescaler);

	/* Configure the GPIOTE task that will toggle the pin on compare events
	 * from the TIMER. Initially set the pin high.
	 * TODO - replace with a proper function from the GPIOTE HAL when such
	 * 	  function becomes available.
	 */
	NRF_GPIOTE->CONFIG[gpiote_index] =
		((pin << GPIOTE_CONFIG_PSEL_Pos) & GPIOTE_CONFIG_PSEL_Msk) |
		(GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
		(GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
		(GPIOTE_CONFIG_OUTINIT_High << GPIOTE_CONFIG_OUTINIT_Pos);

	nrf_timer_task_trigger(TIMER_REGS, NRF_TIMER_TASK_CLEAR);
	nrf_timer_task_trigger(TIMER_REGS, NRF_TIMER_TASK_START);

	return 0;
}

static int pwm_nrf_sw_get_cycles_per_sec(struct device *dev, u32_t pwm,
					 u64_t *cycles)
{
	/* Since this function might be removed (see issue #6958), the maximum
	 * supported frequency (16 MHz) is always returned here, and dynamically
	 * adjusted timer prescalers are used in the pin set function.
	 */
	*cycles = 16ul * 1000ul * 1000ul;

	return 0;
}

static const struct pwm_driver_api pwm_nrf_sw_drv_api_funcs = {
	.pin_set = pwm_nrf_sw_pin_set,
	.get_cycles_per_sec = pwm_nrf_sw_get_cycles_per_sec,
};

static int alloc_ppi_channels(u8_t gpiote_index,
			      nrf_timer_event_t period_event,
			      nrf_timer_event_t channel_event)
{
	nrfx_err_t ret;

#if defined(PPI_PRESENT)
	nrf_ppi_channel_t first_ppi_channel, second_ppi_channel;
	u32_t gpiote_task_address =
		/* TODO - replace with a proper function from the GPIOTE HAL
		 * 	  when such function becomes available.
		 */
		(u32_t)&(NRF_GPIOTE->TASKS_OUT[gpiote_index]);

	ret = nrfx_ppi_channel_alloc(&first_ppi_channel);
	if (ret != NRFX_SUCCESS) {
		return -ENOMEM;
	}
	nrf_ppi_channel_endpoint_setup(
		first_ppi_channel,
		(u32_t)nrf_timer_event_address_get(TIMER_REGS, channel_event),
		gpiote_task_address);

	ret = nrfx_ppi_channel_alloc(&second_ppi_channel);
	if (ret != NRFX_SUCCESS) {
		return -ENOMEM;
	}
	nrf_ppi_channel_endpoint_setup(
		second_ppi_channel,
		(u32_t)nrf_timer_event_address_get(TIMER_REGS, period_event),
		gpiote_task_address);

	nrf_ppi_channels_enable(BIT(first_ppi_channel) |
				BIT(second_ppi_channel));
#else
	u8_t dppi_channel;

	ret = nrfx_dppi_channel_alloc(&dppi_channel);
	if (ret != NRFX_SUCCESS) {
		return -ENOMEM;
	}
	nrf_timer_publish_set(TIMER_REGS, channel_event, dppi_channel);
	nrf_timer_publish_set(TIMER_REGS, period_event, dppi_channel);
	nrf_gpiote_subscribe_set(
		/* TODO - replace with a proper function from the GPIOTE HAL
		 * 	  when such function becomes available.
		 */
		offsetof(NRF_GPIOTE_Type, TASKS_OUT[gpiote_index]),
		dppi_channel);
	nrf_dppi_channels_enable(NRF_DPPIC, BIT(dppi_channel));
#endif /* defined(PPI_PRESENT) */

	return 0;
}

static int pwm_nrf_sw_init(struct device *dev)
{
	u8_t channel;
	u8_t gpiote_index = DT_INST_0_NORDIC_NRF_SW_PWM_GPIOTE_BASE;
	nrf_timer_event_t period_event =
		nrf_timer_compare_event_get(PWM_PERIOD_TIMER_CHANNEL);

	/* Setup the timer used for generating signal phase switching events. */
	nrf_timer_mode_set(TIMER_REGS, NRF_TIMER_MODE_TIMER);
	nrf_timer_bit_width_set(TIMER_REGS,
		GET_TIMER_CAPABILITY(MAX_SIZE) == 32 ? NRF_TIMER_BIT_WIDTH_32
						     : NRF_TIMER_BIT_WIDTH_16);
	/* The last compare channel is used for setting the PWM period.
	 * Enable the shortcut that will clear the timer on the compare event
	 * on this channel.
	 */
	nrf_timer_shorts_enable(TIMER_REGS, PWM_PERIOD_TIMER_SHORT);

	for (channel = 0; channel < PWM_MAP_SIZE; ++channel) {
		nrf_timer_event_t channel_event =
			nrf_timer_compare_event_get(channel);
		int error = alloc_ppi_channels(gpiote_index,
					       period_event,
					       channel_event);
		if (error) {
			LOG_ERR("Failed to allocate PPI channels.");
			return error;
		}

		++gpiote_index;
	}

	return 0;
}

static struct pwm_data pwm_nrf_sw_0_data;

DEVICE_AND_API_INIT(pwm_nrf_sw_0,
		    CONFIG_PWM_NRF_SW_0_DEV_NAME,
		    pwm_nrf_sw_init,
		    &pwm_nrf_sw_0_data,
		    NULL,
		    POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &pwm_nrf_sw_drv_api_funcs);
