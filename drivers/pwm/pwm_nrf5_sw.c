/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>

#include <drivers/pwm.h>
#include <nrf_peripherals.h>

#define LOG_LEVEL CONFIG_PWM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(pwm_nrf5_sw);

#define TIMER_INSTANCE \
	_CONCAT(TIMER, DT_INST_0_NORDIC_NRF_SW_PWM_TIMER_INSTANCE)
#define GET_TIMER_CAPABILITY(capability) \
	(_CONCAT(_CONCAT(TIMER_INSTANCE, _), capability))

/* One compare channel is needed to set the PWM period, hence +1. */
#if ((DT_INST_0_NORDIC_NRF_SW_PWM_CHANNEL_COUNT + 1) > \
     GET_TIMER_CAPABILITY(CC_NUM))
#error "Invalid number of PWM channels configured."
#endif
#define PWM_0_MAP_SIZE DT_INST_0_NORDIC_NRF_SW_PWM_CHANNEL_COUNT

/* Nordic TIMER peripherals allow prescalers 0-9. */
#define MAX_TIMER_PRESCALER  9
/* Nordic TIMERs can be 16- or 32-bit wide. */
#define MAX_TIMER_VALUE  (GET_TIMER_CAPABILITY(MAX_SIZE) == 32 ? UINT32_MAX \
							       : UINT16_MAX)

struct pwm_config {
	NRF_TIMER_Type *timer;
	u8_t gpiote_base;
	u8_t ppi_base;
	u8_t map_size;
};

struct chan_map {
	u32_t pwm;
	u32_t pulse_cycles;
};

struct pwm_data {
	u32_t period_cycles;
	struct chan_map map[PWM_0_MAP_SIZE];
};

static u32_t pwm_period_check(struct pwm_data *data, u8_t map_size,
				 u32_t pwm, u32_t period_cycles,
				 u32_t pulse_cycles)
{
	u8_t i;

	/* allow 0% and 100% duty cycle, as it does not use PWM. */
	if ((pulse_cycles == 0U) || (pulse_cycles == period_cycles)) {
		return 0;
	}

	/* fail if requested period does not match already running period */
	for (i = 0U; i < map_size; i++) {
		if ((data->map[i].pwm != pwm) &&
		    (data->map[i].pulse_cycles != 0U) &&
		    (period_cycles != data->period_cycles)) {
			return -EINVAL;
		}
	}

	return 0;
}

static u8_t pwm_channel_map(struct pwm_data *data, u8_t map_size,
			       u32_t pwm)
{
	u8_t i;

	/* find pin, if already present */
	for (i = 0U; i < map_size; i++) {
		if (pwm == data->map[i].pwm) {
			return i;
		}
	}

	/* find a free entry */
	i = map_size;
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
	struct pwm_config *config;
	NRF_TIMER_Type *timer;
	struct pwm_data *data;
	u8_t ppi_index;
	u8_t channel;
	u8_t prescaler;
	u32_t ret;

	config = (struct pwm_config *)dev->config->config_info;
	timer = config->timer;
	data = dev->driver_data;

	/* check if requested period is allowed while other channels are
	 * active.
	 */
	ret = pwm_period_check(data, config->map_size, pwm, period_cycles,
			       pulse_cycles);
	if (ret) {
		LOG_ERR("Incompatible period");
		return ret;
	}

	/* map pwm pin to GPIOTE config/channel */
	channel = pwm_channel_map(data, config->map_size, pwm);
	if (channel >= config->map_size) {
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
	NRF_GPIOTE->CONFIG[config->gpiote_base + channel] = 0;

	/* clear PPI used */
	ppi_index = config->ppi_base + (channel << 1);
	NRF_PPI->CHENCLR = BIT(ppi_index) | BIT(ppi_index + 1);

	/* configure GPIO pin as output */
	NRF_GPIO->DIRSET = BIT(pwm);
	if (pulse_cycles == 0U) {
		/* 0% duty cycle, keep pin low */
		NRF_GPIO->OUTCLR = BIT(pwm);

		goto pin_set_pwm_off;
	} else if (pulse_cycles == period_cycles) {
		/* 100% duty cycle, keep pin high */
		NRF_GPIO->OUTSET = BIT(pwm);

		goto pin_set_pwm_off;
	} else {
		/* x% duty cycle, start PWM with pin low */
		NRF_GPIO->OUTCLR = BIT(pwm);
	}

	timer->EVENTS_COMPARE[channel] = 0;
	timer->EVENTS_COMPARE[config->map_size] = 0;

	timer->PRESCALER = prescaler;
	timer->CC[channel] = pulse_cycles >> prescaler;
	timer->CC[config->map_size] = period_cycles >> prescaler;
	timer->TASKS_CLEAR = 1;

	/* configure GPIOTE, toggle with initialise output high */
	NRF_GPIOTE->CONFIG[config->gpiote_base + channel] = 0x00130003 |
							    (pwm << 8);

	/* setup PPI */
	NRF_PPI->CH[ppi_index].EEP = (u32_t)
				     &(timer->EVENTS_COMPARE[channel]);
	NRF_PPI->CH[ppi_index].TEP = (u32_t)
				     &(NRF_GPIOTE->TASKS_OUT[channel]);
	NRF_PPI->CH[ppi_index + 1].EEP = (u32_t)
					 &(timer->EVENTS_COMPARE[
							 config->map_size]);
	NRF_PPI->CH[ppi_index + 1].TEP = (u32_t)
					 &(NRF_GPIOTE->TASKS_OUT[channel]);
	NRF_PPI->CHENSET = BIT(ppi_index) | BIT(ppi_index + 1);

	/* start timer, hence PWM */
	timer->TASKS_START = 1;

	/* store the pwm/pin and its param */
	data->period_cycles = period_cycles;
	data->map[channel].pwm = pwm;
	data->map[channel].pulse_cycles = pulse_cycles;

	return 0;

pin_set_pwm_off:
	data->map[channel].pulse_cycles = 0U;
	bool pwm_active = false;

	/* stop timer if all channels are inactive */
	for (channel = 0U; channel < config->map_size; channel++) {
		if (data->map[channel].pulse_cycles) {
			pwm_active = true;
			break;
		}
	}

	if (!pwm_active) {
		/* No active PWM, stop timer */
		timer->TASKS_STOP = 1;
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
	struct pwm_config *config;
	NRF_TIMER_Type *timer;

	config = (struct pwm_config *)dev->config->config_info;
	timer = config->timer;

	/* setup HF timer */
	timer->MODE = TIMER_MODE_MODE_Timer;
	timer->BITMODE = GET_TIMER_CAPABILITY(MAX_SIZE) == 32
			 ? TIMER_BITMODE_BITMODE_32Bit
			 : TIMER_BITMODE_BITMODE_16Bit;

	/* TODO: set shorts according to map_size if not 3, i.e. if NRF_TIMER
	 * supports more than 4 compares, then more channels can be supported.
	 */
	timer->SHORTS = TIMER_SHORTS_COMPARE3_CLEAR_Msk;

	return 0;
}

static const struct pwm_config pwm_nrf5_sw_0_config = {
	.timer = _CONCAT(NRF_TIMER, DT_INST_0_NORDIC_NRF_SW_PWM_TIMER_INSTANCE),
	.ppi_base = DT_INST_0_NORDIC_NRF_SW_PWM_PPI_BASE,
	.gpiote_base = DT_INST_0_NORDIC_NRF_SW_PWM_GPIOTE_BASE,
	.map_size = PWM_0_MAP_SIZE,
};

static struct pwm_data pwm_nrf5_sw_0_data;

DEVICE_AND_API_INIT(pwm_nrf5_sw_0,
		    CONFIG_PWM_NRF5_SW_0_DEV_NAME,
		    pwm_nrf5_sw_init,
		    &pwm_nrf5_sw_0_data,
		    &pwm_nrf5_sw_0_config,
		    POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &pwm_nrf5_sw_drv_api_funcs);
