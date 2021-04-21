/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_sw_pwm

#include <soc.h>
#include <hal/nrf_gpio.h>

#include <drivers/pwm.h>
#include <nrf_peripherals.h>

#define LOG_LEVEL CONFIG_PWM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(pwm_nrf5_sw);

BUILD_ASSERT(DT_INST_NODE_HAS_PROP(0, timer_instance) !=
	     DT_INST_NODE_HAS_PROP(0, generator),
	     "Please define either the timer-instance or generator property, but not both");

#if DT_INST_NODE_HAS_PROP(0, timer_instance)

#define USE_RTC		0
#define GENERATOR_ADDR	_CONCAT(NRF_TIMER, DT_INST_PROP(0, timer_instance))
#define GENERATOR_CC_NUM \
	_CONCAT(_CONCAT(TIMER, DT_INST_PROP(0, timer_instance)), _CC_NUM)

#else /* DT_INST_NODE_HAS_PROP(0, timer_instance) */

#define GENERATOR_NODE	DT_PHANDLE(DT_DRV_INST(0), generator)
#define GENERATOR_CC_NUM	DT_PROP(GENERATOR_NODE, cc_num)

#if DT_NODE_HAS_COMPAT(GENERATOR_NODE, nordic_nrf_rtc)
#define USE_RTC		1
#define GENERATOR_ADDR	((NRF_RTC_Type *) DT_REG_ADDR(GENERATOR_NODE))
BUILD_ASSERT(DT_INST_PROP(0, clock_prescaler) == 0,
	     "Only clock-prescaler = <0> is supported when used with RTC");
#else
#define USE_RTC		0
#define GENERATOR_ADDR	((NRF_TIMER_Type *) DT_REG_ADDR(GENERATOR_NODE))
#endif

#endif /* DT_INST_NODE_HAS_PROP(0, timer_instance) */

/* One compare channel is needed to set the PWM period, hence +1. */
#if ((DT_INST_PROP(0, channel_count) + 1) > GENERATOR_CC_NUM)
#error "Invalid number of PWM channels configured."
#endif
#define PWM_0_MAP_SIZE DT_INST_PROP(0, channel_count)

struct pwm_config {
	union {
		NRF_RTC_Type *rtc;
		NRF_TIMER_Type *timer;
	};
	uint8_t gpiote_base;
	uint8_t ppi_base;
	uint8_t map_size;
	uint8_t prescaler;
};

struct chan_map {
	uint32_t pwm;
	uint32_t pulse_cycles;
};

struct pwm_data {
	uint32_t period_cycles;
	struct chan_map map[PWM_0_MAP_SIZE];
};

static inline NRF_RTC_Type *pwm_config_rtc(const struct pwm_config *config)
{
#if USE_RTC
	return config->rtc;
#else
	return NULL;
#endif
}

static inline NRF_TIMER_Type *pwm_config_timer(const struct pwm_config *config)
{
#if !USE_RTC
	return config->timer;
#else
	return NULL;
#endif
}

static uint32_t pwm_period_check(struct pwm_data *data, uint8_t map_size,
				 uint32_t pwm, uint32_t period_cycles,
				 uint32_t pulse_cycles)
{
	uint8_t i;

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

static uint8_t pwm_channel_map(struct pwm_data *data, uint8_t map_size,
			       uint32_t pwm)
{
	uint8_t i;

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

static int pwm_nrf5_sw_pin_set(const struct device *dev, uint32_t pwm,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	const struct pwm_config *config = dev->config;
	NRF_TIMER_Type *timer = pwm_config_timer(config);
	NRF_RTC_Type *rtc = pwm_config_rtc(config);
	struct pwm_data *data = dev->data;
	uint8_t ppi_index;
	uint32_t ppi_mask;
	uint8_t channel;
	uint32_t ret;

	if (flags) {
		/* PWM polarity not supported (yet?) */
		return -ENOTSUP;
	}

	/* check if requested period is allowed while other channels are
	 * active.
	 */
	ret = pwm_period_check(data, config->map_size, pwm, period_cycles,
			       pulse_cycles);
	if (ret) {
		LOG_ERR("Incompatible period");
		return ret;
	}

	if (USE_RTC) {
		/* pulse_cycles - 1 is written to 24-bit CC */
		if (period_cycles > BIT_MASK(24) + 1) {
			LOG_ERR("Too long period (%u)!", period_cycles);
			return -EINVAL;
		}
	} else {
		/* TODO: if the assigned NRF_TIMER supports higher bit
		 * resolution, use that info in config struct.
		 */
		if (period_cycles > UINT16_MAX) {
			LOG_ERR("Too long period (%u), adjust pwm prescaler!",
				period_cycles);
			return -EINVAL;
		}
	}

	/* map pwm pin to GPIOTE config/channel */
	channel = pwm_channel_map(data, config->map_size, pwm);
	if (channel >= config->map_size) {
		LOG_ERR("No more channels available");
		return -ENOMEM;
	}

	LOG_DBG("PWM %d, period %u, pulse %u", pwm,
			period_cycles, pulse_cycles);

	/* clear GPIOTE config */
	NRF_GPIOTE->CONFIG[config->gpiote_base + channel] = 0;

	/* clear PPI used */
	if (USE_RTC) {
		ppi_index = config->ppi_base + (channel * 3);
		ppi_mask = BIT(ppi_index) | BIT(ppi_index + 1) |
			BIT(ppi_index + 2);
	} else {
		ppi_index = config->ppi_base + (channel * 2);
		ppi_mask = BIT(ppi_index) | BIT(ppi_index + 1);
	}
	NRF_PPI->CHENCLR = ppi_mask;

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

	/* configure RTC / TIMER */
	if (USE_RTC) {
		rtc->EVENTS_COMPARE[channel] = 0;
		rtc->EVENTS_COMPARE[config->map_size] = 0;

		/*
		 * '- 1' adjusts pulse and period cycles to the fact that CLEAR
		 * task event is generated always one LFCLK cycle after period
		 * COMPARE value is reached.
		 */
		rtc->CC[channel] = pulse_cycles - 1;
		rtc->CC[config->map_size] = period_cycles - 1;
		rtc->TASKS_CLEAR = 1;
	} else {
		timer->EVENTS_COMPARE[channel] = 0;
		timer->EVENTS_COMPARE[config->map_size] = 0;

		timer->CC[channel] = pulse_cycles;
		timer->CC[config->map_size] = period_cycles;
		timer->TASKS_CLEAR = 1;
	}

	/* configure GPIOTE, toggle with initialise output high */
	NRF_GPIOTE->CONFIG[config->gpiote_base + channel] = 0x00130003 |
							    (pwm << 8);

	/* setup PPI */
	if (USE_RTC) {
		NRF_PPI->CH[ppi_index].EEP =
			(uint32_t) &(rtc->EVENTS_COMPARE[channel]);
		NRF_PPI->CH[ppi_index].TEP =
			(uint32_t) &(NRF_GPIOTE->TASKS_OUT[channel]);
		NRF_PPI->CH[ppi_index + 1].EEP =
			(uint32_t) &(rtc->EVENTS_COMPARE[config->map_size]);
		NRF_PPI->CH[ppi_index + 1].TEP =
			(uint32_t) &(NRF_GPIOTE->TASKS_OUT[channel]);
		NRF_PPI->CH[ppi_index + 2].EEP =
			(uint32_t) &(rtc->EVENTS_COMPARE[config->map_size]);
		NRF_PPI->CH[ppi_index + 2].TEP =
			(uint32_t) &(rtc->TASKS_CLEAR);
	} else {
		NRF_PPI->CH[ppi_index].EEP =
			(uint32_t) &(timer->EVENTS_COMPARE[channel]);
		NRF_PPI->CH[ppi_index].TEP =
			(uint32_t) &(NRF_GPIOTE->TASKS_OUT[channel]);
		NRF_PPI->CH[ppi_index + 1].EEP =
			(uint32_t) &(timer->EVENTS_COMPARE[config->map_size]);
		NRF_PPI->CH[ppi_index + 1].TEP =
			(uint32_t) &(NRF_GPIOTE->TASKS_OUT[channel]);
	}
	NRF_PPI->CHENSET = ppi_mask;

	/* start timer, hence PWM */
	if (USE_RTC) {
		rtc->TASKS_START = 1;
	} else {
		timer->TASKS_START = 1;
	}

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
		if (USE_RTC) {
			rtc->TASKS_STOP = 1;
		} else {
			timer->TASKS_STOP = 1;
		}
	}

	return 0;
}

static int pwm_nrf5_sw_get_cycles_per_sec(const struct device *dev,
					  uint32_t pwm,
					  uint64_t *cycles)
{
	const struct pwm_config *config = dev->config;

	if (USE_RTC) {
		/*
		 * RTC frequency is derived from 32768Hz source without any
		 * prescaler
		 */
		*cycles = 32768UL;
	} else {
		/*
		 * HF timer frequency is derived from 16MHz source with a
		 * prescaler
		 */
		*cycles = 16000000UL / BIT(config->prescaler);
	}

	return 0;
}

static const struct pwm_driver_api pwm_nrf5_sw_drv_api_funcs = {
	.pin_set = pwm_nrf5_sw_pin_set,
	.get_cycles_per_sec = pwm_nrf5_sw_get_cycles_per_sec,
};

static int pwm_nrf5_sw_init(const struct device *dev)
{
	const struct pwm_config *config = dev->config;
	NRF_TIMER_Type *timer = pwm_config_timer(config);
	NRF_RTC_Type *rtc = pwm_config_rtc(config);

	if (USE_RTC) {
		/* setup RTC */
		rtc->PRESCALER = 0;

		/*
		 * TODO: set EVTEN to map_size if not 3, i.e. if RTC supports
		 * less than 4 compares, then less channels can be supported.
		 */
		rtc->EVTENSET = (RTC_EVTENSET_COMPARE0_Msk |
				 RTC_EVTENSET_COMPARE1_Msk |
				 RTC_EVTENSET_COMPARE2_Msk |
				 RTC_EVTENSET_COMPARE3_Msk);
	} else {
		/* setup HF timer */
		timer->MODE = TIMER_MODE_MODE_Timer;
		timer->PRESCALER = config->prescaler;
		timer->BITMODE = TIMER_BITMODE_BITMODE_16Bit;

		/*
		 * TODO: set shorts according to map_size if not 3, i.e. if
		 * NRF_TIMER supports more than 4 compares, then more channels
		 * can be supported.
		 */
		timer->SHORTS = TIMER_SHORTS_COMPARE3_CLEAR_Msk;
	}

	return 0;
}

static const struct pwm_config pwm_nrf5_sw_0_config = {
	COND_CODE_1(USE_RTC, (.rtc), (.timer)) = GENERATOR_ADDR,
	.ppi_base = DT_INST_PROP(0, ppi_base),
	.gpiote_base = DT_INST_PROP(0, gpiote_base),
	.map_size = PWM_0_MAP_SIZE,
	.prescaler = DT_INST_PROP(0, clock_prescaler),
};

static struct pwm_data pwm_nrf5_sw_0_data;

DEVICE_DT_INST_DEFINE(0,
		    pwm_nrf5_sw_init,
		    device_pm_control_nop,
		    &pwm_nrf5_sw_0_data,
		    &pwm_nrf5_sw_0_config,
		    POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &pwm_nrf5_sw_drv_api_funcs);
