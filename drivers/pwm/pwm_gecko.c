/*
 * Copyright (c) 2021 Sun Amar.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT silabs_gecko_pwm

#include <zephyr/drivers/pwm.h>
#include <zephyr/dt-bindings/pwm/pwm.h>
#include <em_cmu.h>
#include <em_timer.h>

/** PWM configuration. */
struct pwm_gecko_config {
	TIMER_TypeDef *timer;
	CMU_Clock_TypeDef clock;
	uint16_t prescaler;
	TIMER_Prescale_TypeDef prescale_enum;
	uint8_t channel;
	uint8_t location;
	uint8_t port;
	uint8_t pin;
};

static int pwm_gecko_set_cycles(const struct device *dev, uint32_t channel,
				uint32_t period_cycles, uint32_t pulse_cycles,
				pwm_flags_t flags)
{
	TIMER_InitCC_TypeDef compare_config = TIMER_INITCC_DEFAULT;
	const struct pwm_gecko_config *cfg = dev->config;

	if (BUS_RegMaskedRead(&cfg->timer->CC[channel].CTRL,
		_TIMER_CC_CTRL_MODE_MASK) != timerCCModePWM) {

#ifdef _TIMER_ROUTE_MASK
		BUS_RegMaskedWrite(&cfg->timer->ROUTE,
			_TIMER_ROUTE_LOCATION_MASK,
			cfg->location << _TIMER_ROUTE_LOCATION_SHIFT);
		BUS_RegMaskedSet(&cfg->timer->ROUTE, 1 << channel);
#elif defined(_TIMER_ROUTELOC0_MASK)
		BUS_RegMaskedWrite(&cfg->timer->ROUTELOC0,
			_TIMER_ROUTELOC0_CC0LOC_MASK <<
			(channel * _TIMER_ROUTELOC0_CC1LOC_SHIFT),
			cfg->location << (channel * _TIMER_ROUTELOC0_CC1LOC_SHIFT));
		BUS_RegMaskedSet(&cfg->timer->ROUTEPEN, 1 << channel);
#else
#error Unsupported device
#endif

		compare_config.mode = timerCCModePWM;
		TIMER_InitCC(cfg->timer, channel, &compare_config);
	}

	cfg->timer->CC[channel].CTRL |= (flags & PWM_POLARITY_INVERTED) ?
		TIMER_CC_CTRL_OUTINV : 0;

	TIMER_TopSet(cfg->timer, period_cycles);

	TIMER_CompareBufSet(cfg->timer, channel, pulse_cycles);

	return 0;
}

static int pwm_gecko_get_cycles_per_sec(const struct device *dev,
					uint32_t channel, uint64_t *cycles)
{
	const struct pwm_gecko_config *cfg = dev->config;

	*cycles = CMU_ClockFreqGet(cfg->clock) / cfg->prescaler;

	return 0;
}

static const struct pwm_driver_api pwm_gecko_driver_api = {
	.set_cycles = pwm_gecko_set_cycles,
	.get_cycles_per_sec = pwm_gecko_get_cycles_per_sec,
};

static int pwm_gecko_init(const struct device *dev)
{
	TIMER_Init_TypeDef timer = TIMER_INIT_DEFAULT;
	const struct pwm_gecko_config *cfg = dev->config;

	CMU_ClockEnable(cfg->clock, true);

	CMU_ClockEnable(cmuClock_GPIO, true);
	GPIO_PinModeSet(cfg->port, cfg->pin, gpioModePushPull, 0);

	timer.prescale = cfg->prescale_enum;
	TIMER_Init(cfg->timer, &timer);

	return 0;
}

#define CLOCK_TIMER(id) _CONCAT(cmuClock_TIMER, id)
#define PRESCALING_FACTOR(factor) \
	((_CONCAT(timerPrescale, factor)))

#define PWM_GECKO_INIT(index)							\
	static const struct pwm_gecko_config pwm_gecko_config_##index = {	\
		.timer = (TIMER_TypeDef *)DT_REG_ADDR(DT_INST_PARENT(index)),	\
		.clock = CLOCK_TIMER(index),					\
		.prescaler = DT_INST_PROP(index, prescaler),			\
		.prescale_enum = (TIMER_Prescale_TypeDef)			\
			PRESCALING_FACTOR(DT_INST_PROP(index, prescaler)),	\
		.location = DT_INST_PROP_BY_IDX(index, pin_location, 0),	\
		.port = (GPIO_Port_TypeDef)					\
			DT_INST_PROP_BY_IDX(index, pin_location, 1),		\
		.pin = DT_INST_PROP_BY_IDX(index, pin_location, 2),		\
	};									\
										\
	DEVICE_DT_INST_DEFINE(index, &pwm_gecko_init, NULL, NULL,		\
				&pwm_gecko_config_##index, POST_KERNEL,		\
				CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
				&pwm_gecko_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_GECKO_INIT)
