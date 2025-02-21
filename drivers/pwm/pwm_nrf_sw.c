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
#include <helpers/nrfx_gppi.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_rtc.h>
#include <hal/nrf_timer.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pwm_nrf_sw, CONFIG_PWM_LOG_LEVEL);

#define GENERATOR_NODE	DT_INST_PHANDLE(0, generator)
#define GENERATOR_CC_NUM	DT_PROP(GENERATOR_NODE, cc_num)

#if DT_NODE_HAS_COMPAT(GENERATOR_NODE, nordic_nrf_rtc)
#define USE_RTC		1
#define GENERATOR_ADDR	((NRF_RTC_Type *) DT_REG_ADDR(GENERATOR_NODE))
#define GENERATOR_BITS	24
BUILD_ASSERT(DT_INST_PROP(0, clock_prescaler) == 0,
	     "Only clock-prescaler = <0> is supported when used with RTC");
#else
#define USE_RTC		0
#define GENERATOR_ADDR	((NRF_TIMER_Type *) DT_REG_ADDR(GENERATOR_NODE))
#define GENERATOR_BITS	DT_PROP(GENERATOR_NODE, max_bit_width)
#endif

#define PWM_0_MAP_SIZE DT_INST_PROP_LEN(0, channel_gpios)

/* One compare channel is needed to set the PWM period, hence +1. */
#if ((PWM_0_MAP_SIZE + 1) > GENERATOR_CC_NUM)
#error "Invalid number of PWM channels configured."
#endif

#if defined(PPI_FEATURE_FORKS_PRESENT) || defined(DPPI_PRESENT)
#define PPI_FORK_AVAILABLE 1
#else
#define PPI_FORK_AVAILABLE 0
#endif

/* When RTC is used, one more PPI task endpoint is required for clearing
 * the counter, so when FORK feature is not available, one more PPI channel
 * needs to be used.
 */
#if USE_RTC && !PPI_FORK_AVAILABLE
#define PPI_PER_CH 3
#else
#define PPI_PER_CH 2
#endif

struct pwm_config {
	union {
		NRF_RTC_Type *rtc;
		NRF_TIMER_Type *timer;
	};
	nrfx_gpiote_t gpiote[PWM_0_MAP_SIZE];
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

static int pwm_nrf_sw_set_cycles(const struct device *dev, uint32_t channel,
				 uint32_t period_cycles, uint32_t pulse_cycles,
				 pwm_flags_t flags)
{
	const struct pwm_config *config = dev->config;
	NRF_TIMER_Type *timer = pwm_config_timer(config);
	NRF_RTC_Type *rtc = pwm_config_rtc(config);
	NRF_GPIOTE_Type *gpiote;
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
		if (GENERATOR_BITS < 32 &&
		    period_cycles > BIT_MASK(GENERATOR_BITS)) {
			LOG_ERR("Too long period (%u), adjust PWM prescaler!",
				period_cycles);
			return -EINVAL;
		}
	}

	gpiote = config->gpiote[channel].p_reg;
	psel_ch = config->psel_ch[channel];
	gpiote_ch = data->gpiote_ch[channel];
	ppi_chs = data->ppi_ch[channel];

	LOG_DBG("channel %u, period %u, pulse %u",
		channel, period_cycles, pulse_cycles);

	/* clear PPI used */
	ppi_mask = BIT(ppi_chs[0]) | BIT(ppi_chs[1]) |
		   (PPI_PER_CH > 2 ? BIT(ppi_chs[2]) : 0);
	nrfx_gppi_channels_disable(ppi_mask);

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

		/* clear GPIOTE config */
		nrf_gpiote_te_default(gpiote, gpiote_ch);

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
			nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_STOP);
		} else {
			nrf_timer_task_trigger(timer, NRF_TIMER_TASK_STOP);
		}

		return 0;
	}

	/* configure RTC / TIMER */
	if (USE_RTC) {
		nrf_rtc_event_clear(rtc,
			nrf_rtc_compare_event_get(1 + channel));
		nrf_rtc_event_clear(rtc,
			nrf_rtc_compare_event_get(0));

		/*
		 * '- 1' adjusts pulse and period cycles to the fact that CLEAR
		 * task event is generated always one LFCLK cycle after period
		 * COMPARE value is reached.
		 */
		nrf_rtc_cc_set(rtc, 1 + channel, pulse_cycles - 1);
		nrf_rtc_cc_set(rtc, 0, period_cycles - 1);
		nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_CLEAR);
	} else {
		nrf_timer_event_clear(timer,
			nrf_timer_compare_event_get(1 + channel));
		nrf_timer_event_clear(timer,
			nrf_timer_compare_event_get(0));

		nrf_timer_cc_set(timer, 1 + channel, pulse_cycles);
		nrf_timer_cc_set(timer, 0, period_cycles);
		nrf_timer_task_trigger(timer, NRF_TIMER_TASK_CLEAR);
	}

	/* Configure GPIOTE - toggle task with proper initial output value. */
	gpiote->CONFIG[gpiote_ch] =
		(GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) |
		((uint32_t)psel_ch << 8) |
		(GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos) |
		((uint32_t)active_level << GPIOTE_CONFIG_OUTINIT_Pos);

	/* setup PPI */
	uint32_t pulse_end_event_address, period_end_event_address;
	nrf_gpiote_task_t pulse_end_task, period_end_task;
#if defined(GPIOTE_FEATURE_SET_PRESENT) && defined(GPIOTE_FEATURE_CLR_PRESENT)
	if (active_level == 0) {
		pulse_end_task  = nrf_gpiote_set_task_get(gpiote_ch);
		period_end_task = nrf_gpiote_clr_task_get(gpiote_ch);
	} else {
		pulse_end_task  = nrf_gpiote_clr_task_get(gpiote_ch);
		period_end_task = nrf_gpiote_set_task_get(gpiote_ch);
	}
#else
	pulse_end_task = period_end_task = nrf_gpiote_out_task_get(gpiote_ch);
#endif
	uint32_t pulse_end_task_address =
		nrf_gpiote_task_address_get(gpiote, pulse_end_task);
	uint32_t period_end_task_address =
		nrf_gpiote_task_address_get(gpiote, period_end_task);

	if (USE_RTC) {
		uint32_t clear_task_address =
			nrf_rtc_event_address_get(rtc, NRF_RTC_TASK_CLEAR);

		pulse_end_event_address =
			nrf_rtc_event_address_get(rtc,
				nrf_rtc_compare_event_get(1 + channel));
		period_end_event_address =
			nrf_rtc_event_address_get(rtc,
				nrf_rtc_compare_event_get(0));

#if PPI_FORK_AVAILABLE
		nrfx_gppi_fork_endpoint_setup(ppi_chs[1],
					      clear_task_address);
#else
		nrfx_gppi_channel_endpoints_setup(ppi_chs[2],
						  period_end_event_address,
						  clear_task_address);
#endif
	} else {
		pulse_end_event_address =
			nrf_timer_event_address_get(timer,
				nrf_timer_compare_event_get(1 + channel));
		period_end_event_address =
			nrf_timer_event_address_get(timer,
				nrf_timer_compare_event_get(0));
	}

	nrfx_gppi_channel_endpoints_setup(ppi_chs[0],
					  pulse_end_event_address,
					  pulse_end_task_address);
	nrfx_gppi_channel_endpoints_setup(ppi_chs[1],
					  period_end_event_address,
					  period_end_task_address);
	nrfx_gppi_channels_enable(ppi_mask);

	/* start timer, hence PWM */
	if (USE_RTC) {
		nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_START);
	} else {
		nrf_timer_task_trigger(timer, NRF_TIMER_TASK_START);
	}

	/* store the period and pulse cycles */
	data->period_cycles = period_cycles;
	data->pulse_cycles[channel] = pulse_cycles;

	return 0;
}

static int pwm_nrf_sw_get_cycles_per_sec(const struct device *dev,
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

static DEVICE_API(pwm, pwm_nrf_sw_drv_api_funcs) = {
	.set_cycles = pwm_nrf_sw_set_cycles,
	.get_cycles_per_sec = pwm_nrf_sw_get_cycles_per_sec,
};

static int pwm_nrf_sw_init(const struct device *dev)
{
	const struct pwm_config *config = dev->config;
	struct pwm_data *data = dev->data;
	NRF_TIMER_Type *timer = pwm_config_timer(config);
	NRF_RTC_Type *rtc = pwm_config_rtc(config);

	for (uint32_t i = 0; i < config->map_size; i++) {
		nrfx_err_t err;

		/* Allocate resources. */
		for (uint32_t j = 0; j < PPI_PER_CH; j++) {
			err = nrfx_gppi_channel_alloc(&data->ppi_ch[i][j]);
			if (err != NRFX_SUCCESS) {
				/* Do not free allocated resource. It is a fatal condition,
				 * system requires reconfiguration.
				 */
				LOG_ERR("Failed to allocate PPI channel");
				return -ENOMEM;
			}
		}

		err = nrfx_gpiote_channel_alloc(&config->gpiote[i],
						&data->gpiote_ch[i]);
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
		nrf_rtc_prescaler_set(rtc, 0);
		nrf_rtc_event_enable(rtc, NRF_RTC_INT_COMPARE0_MASK |
					  NRF_RTC_INT_COMPARE1_MASK |
					  NRF_RTC_INT_COMPARE2_MASK |
					  NRF_RTC_INT_COMPARE3_MASK);
	} else {
		/* setup HF timer */
		nrf_timer_mode_set(timer, NRF_TIMER_MODE_TIMER);
		nrf_timer_prescaler_set(timer, config->prescaler);
		nrf_timer_bit_width_set(timer,
			GENERATOR_BITS == 32 ? NRF_TIMER_BIT_WIDTH_32
					     : NRF_TIMER_BIT_WIDTH_16);
		nrf_timer_shorts_enable(timer,
			NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK);
	}

	return 0;
}

#define PSEL_AND_COMMA(_node_id, _prop, _idx) \
	NRF_DT_GPIOS_TO_PSEL_BY_IDX(_node_id, _prop, _idx),

#define ACTIVE_LOW_BITS(_node_id, _prop, _idx) \
	((DT_GPIO_FLAGS_BY_IDX(_node_id, _prop, _idx) & GPIO_ACTIVE_LOW) \
	 ? BIT(_idx) : 0) |

#define GPIOTE_AND_COMMA(_node_id, _prop, _idx) \
	NRFX_GPIOTE_INSTANCE(NRF_DT_GPIOTE_INST_BY_IDX(_node_id, _prop, _idx)),

static const struct pwm_config pwm_nrf_sw_0_config = {
	COND_CODE_1(USE_RTC, (.rtc), (.timer)) = GENERATOR_ADDR,
	.gpiote = {
		DT_INST_FOREACH_PROP_ELEM(0, channel_gpios, GPIOTE_AND_COMMA)
	},
	.psel_ch = {
		DT_INST_FOREACH_PROP_ELEM(0, channel_gpios, PSEL_AND_COMMA)
	},
	.initially_inverted =
		DT_INST_FOREACH_PROP_ELEM(0, channel_gpios, ACTIVE_LOW_BITS) 0,
	.map_size = PWM_0_MAP_SIZE,
	.prescaler = DT_INST_PROP(0, clock_prescaler),
};

static struct pwm_data pwm_nrf_sw_0_data;

DEVICE_DT_INST_DEFINE(0,
		    pwm_nrf_sw_init,
		    NULL,
		    &pwm_nrf_sw_0_data,
		    &pwm_nrf_sw_0_config,
		    POST_KERNEL,
		    CONFIG_PWM_INIT_PRIORITY,
		    &pwm_nrf_sw_drv_api_funcs);
