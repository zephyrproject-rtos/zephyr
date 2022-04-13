/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_sw_pwm

#include <soc.h>
#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>
#include <hal/nrf_gpio.h>

#include <drivers/pwm.h>
#include <nrf_peripherals.h>

#define LOG_LEVEL CONFIG_PWM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(pwm_nrf5_sw);

#define GENERATOR_NODE	DT_INST_PHANDLE(0, generator)
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

#define PWM_0_MAP_SIZE DT_INST_PROP_LEN(0, channel_gpios)

/* One compare channel is needed to set the PWM period, hence +1. */
#if ((PWM_0_MAP_SIZE + 1) > GENERATOR_CC_NUM)
#error "Invalid number of PWM channels configured."
#endif

/* When RTC is used, one more PPI task endpoint is required for clearing
 * the counter, so when FORK feature is not available, one more PPI channel
 * needs to be used.
 */
#if USE_RTC && !defined(PPI_FEATURE_FORKS_PRESENT)
#define PPI_PER_CH 3
#else
#define PPI_PER_CH 2
#endif

struct pwm_config {
	union {
		NRF_RTC_Type *rtc;
		NRF_TIMER_Type *timer;
	};
	uint8_t psel_ch[PWM_0_MAP_SIZE];
	uint8_t map_size;
	uint8_t prescaler;
};

struct pwm_data {
	uint32_t period_cycles;
	uint32_t pulse_cycles[PWM_0_MAP_SIZE];
	uint8_t ppi_ch[PWM_0_MAP_SIZE][PPI_PER_CH];
	uint8_t gpiote_ch[PWM_0_MAP_SIZE];
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
				 uint32_t channel, uint32_t period_cycles,
				 uint32_t pulse_cycles)
{
	uint8_t i;

	/* allow 0% and 100% duty cycle, as it does not use PWM. */
	if ((pulse_cycles == 0U) || (pulse_cycles == period_cycles)) {
		return 0;
	}

	/* fail if requested period does not match already running period */
	for (i = 0U; i < map_size; i++) {
		if ((i != channel) &&
		    (data->pulse_cycles[i] != 0U) &&
		    (period_cycles != data->period_cycles)) {
			return -EINVAL;
		}
	}

	return 0;
}

static int pwm_nrf5_sw_pin_set(const struct device *dev, uint32_t pwm,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	const struct pwm_config *config = dev->config;
	NRF_TIMER_Type *timer = pwm_config_timer(config);
	NRF_RTC_Type *rtc = pwm_config_rtc(config);
	struct pwm_data *data = dev->data;
	uint32_t ppi_mask;
	uint8_t channel = pwm;
	uint8_t psel_ch;
	uint8_t gpiote_ch;
	const uint8_t *ppi_chs;
	uint32_t ret;

	if (flags) {
		/* PWM polarity not supported (yet?) */
		return -ENOTSUP;
	}

	if (channel >= config->map_size) {
		LOG_ERR("Invalid channel: %u.", channel);
		return -EINVAL;
	}

	/* check if requested period is allowed while other channels are
	 * active.
	 */
	ret = pwm_period_check(data, config->map_size, channel, period_cycles,
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
			LOG_ERR("Too long period (%u), adjust PWM prescaler!",
				period_cycles);
			return -EINVAL;
		}
	}

	psel_ch = config->psel_ch[channel];
	gpiote_ch = data->gpiote_ch[channel];
	ppi_chs = data->ppi_ch[channel];

	LOG_DBG("channel %u, period %u, pulse %u",
		channel, period_cycles, pulse_cycles);

	/* clear GPIOTE config */
	NRF_GPIOTE->CONFIG[gpiote_ch] = 0;

	/* clear PPI used */
	ppi_mask = BIT(ppi_chs[0]) | BIT(ppi_chs[1]) |
		   (PPI_PER_CH > 2 ? BIT(ppi_chs[2]) : 0);
	NRF_PPI->CHENCLR = ppi_mask;

	/* configure GPIO pin as output */
	nrf_gpio_cfg_output(psel_ch);
	if (pulse_cycles == 0U) {
		/* 0% duty cycle, keep pin low */
		nrf_gpio_pin_clear(psel_ch);

		goto pin_set_pwm_off;
	} else if (pulse_cycles == period_cycles) {
		/* 100% duty cycle, keep pin high */
		nrf_gpio_pin_set(psel_ch);

		goto pin_set_pwm_off;
	} else {
		/* x% duty cycle, start PWM with pin low */
		nrf_gpio_pin_clear(psel_ch);
	}

	/* configure RTC / TIMER */
	if (USE_RTC) {
		rtc->EVENTS_COMPARE[1 + channel] = 0;
		rtc->EVENTS_COMPARE[0] = 0;

		/*
		 * '- 1' adjusts pulse and period cycles to the fact that CLEAR
		 * task event is generated always one LFCLK cycle after period
		 * COMPARE value is reached.
		 */
		rtc->CC[1 + channel] = pulse_cycles - 1;
		rtc->CC[0] = period_cycles - 1;
		rtc->TASKS_CLEAR = 1;
	} else {
		timer->EVENTS_COMPARE[1 + channel] = 0;
		timer->EVENTS_COMPARE[0] = 0;

		timer->CC[1 + channel] = pulse_cycles;
		timer->CC[0] = period_cycles;
		timer->TASKS_CLEAR = 1;
	}

	/* configure GPIOTE, toggle with initialise output high */
	NRF_GPIOTE->CONFIG[gpiote_ch] = 0x00130003 | (psel_ch << 8);

	/* setup PPI */
	if (USE_RTC) {
		NRF_PPI->CH[ppi_chs[0]].EEP =
			(uint32_t) &(rtc->EVENTS_COMPARE[1 + channel]);
		NRF_PPI->CH[ppi_chs[0]].TEP =
			(uint32_t) &(NRF_GPIOTE->TASKS_OUT[gpiote_ch]);
		NRF_PPI->CH[ppi_chs[1]].EEP =
			(uint32_t) &(rtc->EVENTS_COMPARE[0]);
		NRF_PPI->CH[ppi_chs[1]].TEP =
			(uint32_t) &(NRF_GPIOTE->TASKS_OUT[gpiote_ch]);
#if defined(PPI_FEATURE_FORKS_PRESENT)
		NRF_PPI->FORK[ppi_chs[1]].TEP =
			(uint32_t) &(rtc->TASKS_CLEAR);
#else
		NRF_PPI->CH[ppi_chs[2]].EEP =
			(uint32_t) &(rtc->EVENTS_COMPARE[0]);
		NRF_PPI->CH[ppi_chs[2]].TEP =
			(uint32_t) &(rtc->TASKS_CLEAR);
#endif
	} else {
		NRF_PPI->CH[ppi_chs[0]].EEP =
			(uint32_t) &(timer->EVENTS_COMPARE[1 + channel]);
		NRF_PPI->CH[ppi_chs[0]].TEP =
			(uint32_t) &(NRF_GPIOTE->TASKS_OUT[gpiote_ch]);
		NRF_PPI->CH[ppi_chs[1]].EEP =
			(uint32_t) &(timer->EVENTS_COMPARE[0]);
		NRF_PPI->CH[ppi_chs[1]].TEP =
			(uint32_t) &(NRF_GPIOTE->TASKS_OUT[gpiote_ch]);
	}
	NRF_PPI->CHENSET = ppi_mask;

	/* start timer, hence PWM */
	if (USE_RTC) {
		rtc->TASKS_START = 1;
	} else {
		timer->TASKS_START = 1;
	}

	/* store the period and pulse cycles */
	data->period_cycles = period_cycles;
	data->pulse_cycles[channel] = pulse_cycles;

	return 0;

pin_set_pwm_off:
	data->pulse_cycles[channel] = 0U;
	bool pwm_active = false;

	/* stop timer if all channels are inactive */
	for (channel = 0U; channel < config->map_size; channel++) {
		if (data->pulse_cycles[channel]) {
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
	struct pwm_data *data = dev->data;
	NRF_TIMER_Type *timer = pwm_config_timer(config);
	NRF_RTC_Type *rtc = pwm_config_rtc(config);

	/* Allocate resources. */
	for (uint32_t i = 0; i < config->map_size; i++) {
		nrfx_err_t err;

		for (uint32_t j = 0; j < PPI_PER_CH; j++) {
			err = nrfx_ppi_channel_alloc(&data->ppi_ch[i][j]);
			if (err != NRFX_SUCCESS) {
				/* Do not free allocated resource. It is a fatal condition,
				 * system requires reconfiguration.
				 */
				LOG_ERR("Failed to allocate PPI channel");
				return -ENOMEM;
			}
		}

		err = nrfx_gpiote_channel_alloc(&data->gpiote_ch[i]);
		if (err != NRFX_SUCCESS) {
			/* Do not free allocated resource. It is a fatal condition,
			 * system requires reconfiguration.
			 */
			LOG_ERR("Failed to allocate GPIOTE channel");
			return -ENOMEM;
		}
	}

	if (USE_RTC) {
		/* setup RTC */
		rtc->PRESCALER = 0;

		rtc->EVTENSET = (RTC_EVTENSET_COMPARE0_Msk |
				 RTC_EVTENSET_COMPARE1_Msk |
				 RTC_EVTENSET_COMPARE2_Msk |
				 RTC_EVTENSET_COMPARE3_Msk);
	} else {
		/* setup HF timer */
		timer->MODE = TIMER_MODE_MODE_Timer;
		timer->PRESCALER = config->prescaler;
		timer->BITMODE = TIMER_BITMODE_BITMODE_16Bit;

		timer->SHORTS = TIMER_SHORTS_COMPARE0_CLEAR_Msk;
	}

	return 0;
}

#define PSEL_AND_COMMA(_node_id, _prop, _idx) \
	NRF_DT_GPIOS_TO_PSEL_BY_IDX(_node_id, _prop, _idx),

static const struct pwm_config pwm_nrf5_sw_0_config = {
	COND_CODE_1(USE_RTC, (.rtc), (.timer)) = GENERATOR_ADDR,
	.psel_ch = {
		DT_INST_FOREACH_PROP_ELEM(0, channel_gpios, PSEL_AND_COMMA)
	},
	.map_size = PWM_0_MAP_SIZE,
	.prescaler = DT_INST_PROP(0, clock_prescaler),
};

static struct pwm_data pwm_nrf5_sw_0_data;

DEVICE_DT_INST_DEFINE(0,
		    pwm_nrf5_sw_init,
		    NULL,
		    &pwm_nrf5_sw_0_data,
		    &pwm_nrf5_sw_0_config,
		    POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &pwm_nrf5_sw_drv_api_funcs);
