/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_sw_pwm

#include <soc.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <nrfx_gpiote.h>
#include <nrfx_ppi.h>
#include <hal/nrf_gpio.h>
#include <nrf_peripherals.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_nrf5_sw, CONFIG_PWM_LOG_LEVEL);

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
	uint8_t initially_inverted;
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

static int pwm_nrf5_sw_set_cycles(const struct device *dev, uint32_t channel,
				  uint32_t period_cycles, uint32_t pulse_cycles,
				  pwm_flags_t flags)
{
	const struct pwm_config *config = dev->config;
	NRF_TIMER_Type *timer = pwm_config_timer(config);
	NRF_RTC_Type *rtc = pwm_config_rtc(config);
	struct pwm_data *data = dev->data;
	uint32_t ppi_mask;
	uint8_t active_level;
	uint8_t psel_ch;
	uint8_t gpiote_ch;
	const uint8_t *ppi_chs;
	int ret;

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

	active_level = (flags & PWM_POLARITY_INVERTED) ? 0 : 1;

	/*
	 * If the duty cycle is 0% or 100%, there is no need to generate
	 * the PWM signal, just	keep the output pin in inactive or active
	 * state, respectively.
	 */
	if (pulse_cycles == 0 || pulse_cycles == period_cycles) {
		nrf_gpio_pin_write(psel_ch,
			pulse_cycles == 0 ? !active_level
					  :  active_level);

		/* No PWM generation for this channel. */
		data->pulse_cycles[channel] = 0U;

		/* Check if PWM signal is generated on any channel. */
		for (uint8_t i = 0; i < config->map_size; i++) {
			if (data->pulse_cycles[i]) {
				return 0;
			}
		}

		/* No PWM generation needed, stop the timer. */
		if (USE_RTC) {
			rtc->TASKS_STOP = 1;
		} else {
			timer->TASKS_STOP = 1;
		}

		return 0;
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

	/* Configure GPIOTE - toggle task with proper initial output value. */
	NRF_GPIOTE->CONFIG[gpiote_ch] =
		(GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
		((uint32_t)psel_ch << 8) |
		(GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
		((uint32_t)active_level << GPIOTE_CONFIG_OUTINIT_Pos);

	/* setup PPI */
	if (USE_RTC) {
		NRF_PPI->CH[ppi_chs[0]].EEP =
			(uint32_t) &rtc->EVENTS_COMPARE[1 + channel];
		NRF_PPI->CH[ppi_chs[0]].TEP =
			(uint32_t) &NRF_GPIOTE->TASKS_OUT[gpiote_ch];
		NRF_PPI->CH[ppi_chs[1]].EEP =
			(uint32_t) &rtc->EVENTS_COMPARE[0];
		NRF_PPI->CH[ppi_chs[1]].TEP =
			(uint32_t) &NRF_GPIOTE->TASKS_OUT[gpiote_ch];
#if defined(PPI_FEATURE_FORKS_PRESENT)
		NRF_PPI->FORK[ppi_chs[1]].TEP =
			(uint32_t) &rtc->TASKS_CLEAR;
#else
		NRF_PPI->CH[ppi_chs[2]].EEP =
			(uint32_t) &rtc->EVENTS_COMPARE[0];
		NRF_PPI->CH[ppi_chs[2]].TEP =
			(uint32_t) &rtc->TASKS_CLEAR;
#endif
	} else {
		NRF_PPI->CH[ppi_chs[0]].EEP =
			(uint32_t) &timer->EVENTS_COMPARE[1 + channel];
		NRF_PPI->CH[ppi_chs[0]].TEP =
			(uint32_t) &NRF_GPIOTE->TASKS_OUT[gpiote_ch];
		NRF_PPI->CH[ppi_chs[1]].EEP =
			(uint32_t) &timer->EVENTS_COMPARE[0];
		NRF_PPI->CH[ppi_chs[1]].TEP =
			(uint32_t) &NRF_GPIOTE->TASKS_OUT[gpiote_ch];
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
}

static int pwm_nrf5_sw_get_cycles_per_sec(const struct device *dev,
					  uint32_t channel, uint64_t *cycles)
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
	.set_cycles = pwm_nrf5_sw_set_cycles,
	.get_cycles_per_sec = pwm_nrf5_sw_get_cycles_per_sec,
};

static int pwm_nrf5_sw_init(const struct device *dev)
{
	const struct pwm_config *config = dev->config;
	struct pwm_data *data = dev->data;
	NRF_TIMER_Type *timer = pwm_config_timer(config);
	NRF_RTC_Type *rtc = pwm_config_rtc(config);

	for (uint32_t i = 0; i < config->map_size; i++) {
		nrfx_err_t err;

		/* Allocate resources. */
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

		/* Set initial state of the output pins. */
		nrf_gpio_pin_write(config->psel_ch[i],
			(config->initially_inverted & BIT(i)) ? 1 : 0);
		nrf_gpio_cfg_output(config->psel_ch[i]);
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

#define ACTIVE_LOW_BITS(_node_id, _prop, _idx) \
	((DT_GPIO_FLAGS_BY_IDX(_node_id, _prop, _idx) & GPIO_ACTIVE_LOW) \
	 ? BIT(_idx) : 0) |

static const struct pwm_config pwm_nrf5_sw_0_config = {
	COND_CODE_1(USE_RTC, (.rtc), (.timer)) = GENERATOR_ADDR,
	.psel_ch = {
		DT_INST_FOREACH_PROP_ELEM(0, channel_gpios, PSEL_AND_COMMA)
	},
	.initially_inverted =
		DT_INST_FOREACH_PROP_ELEM(0, channel_gpios, ACTIVE_LOW_BITS) 0,
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
