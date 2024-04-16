/*
 * Copyright (c) 2023 SLB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_xmc4xxx_ccu8_pwm

#include <soc.h>
#include <zephyr/dt-bindings/pwm/pwm.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>

#include <xmc_ccu8.h>
#include <xmc_scu.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_xmc4xxx_ccu8, CONFIG_PWM_LOG_LEVEL);

#define NUM_SLICES 4
#define NUM_CHANNELS (NUM_SLICES * 2)
#define MAX_DEAD_TIME_VALUE 255
#define MAX_SLICE_PRESCALER 15
#define MAX_DEADTIME_PRESCALER 3
#define SLICE_ADDR_FROM_MODULE(module_ptr, idx) ((uint32_t)(module_ptr) + ((idx) + 1) * 0x100)

struct pwm_xmc4xxx_ccu8_config {
	XMC_CCU8_MODULE_t *ccu8;
	const struct pinctrl_dev_config *pcfg;
	const uint8_t slice_prescaler[NUM_SLICES];
	const uint8_t slice_deadtime_prescaler[NUM_SLICES];
	const uint32_t deadtime_high_ns[NUM_CHANNELS];
	const uint32_t deadtime_low_ns[NUM_CHANNELS];
};

static int pwm_xmc4xxx_ccu8_init(const struct device *dev)
{
	const struct pwm_xmc4xxx_ccu8_config *config = dev->config;

	/* enables the CCU8 clock and ungates clock to CCU8x */
	XMC_CCU8_EnableModule(config->ccu8);
	XMC_CCU8_StartPrescaler(config->ccu8);

	for (int i = 0; i < NUM_SLICES; i++) {
		XMC_CCU8_SLICE_t *slice;
		XMC_CCU8_SLICE_DEAD_TIME_CONFIG_t deadtime_conf = {0};
		XMC_CCU8_SLICE_COMPARE_CONFIG_t slice_conf = {
			.prescaler_initval = config->slice_prescaler[i],
			.invert_out1 = 1,
			.invert_out3 = 1,
		};

		if (config->slice_prescaler[i] > MAX_SLICE_PRESCALER) {
			LOG_ERR("Invalid slice_prescaler value %d. Range [0, 15]",
				config->slice_prescaler[i]);
			return -EINVAL;
		}

		if (config->slice_deadtime_prescaler[i] > MAX_DEADTIME_PRESCALER) {
			LOG_ERR("Invalid dead time prescaler value %d. Range [0, 3]",
				config->slice_deadtime_prescaler[i]);
			return -EINVAL;
		}

		slice = (XMC_CCU8_SLICE_t *)SLICE_ADDR_FROM_MODULE(config->ccu8, i);
		XMC_CCU8_SLICE_CompareInit(slice, &slice_conf);

		deadtime_conf.div = config->slice_deadtime_prescaler[i];
		if (config->deadtime_high_ns[2*i] > 0 || config->deadtime_low_ns[2*i] > 0) {
			deadtime_conf.enable_dead_time_channel1 = 1;
		}
		deadtime_conf.channel1_st_path = config->deadtime_high_ns[2*i] > 0;
		deadtime_conf.channel1_inv_st_path = config->deadtime_low_ns[2*i] > 0;

		if (config->deadtime_high_ns[2*i + 1] > 0 || config->deadtime_low_ns[2*i + 1] > 0) {
			deadtime_conf.enable_dead_time_channel2 = 1;
		}
		deadtime_conf.channel2_st_path = config->deadtime_high_ns[2*i + 1] > 0;
		deadtime_conf.channel2_inv_st_path = config->deadtime_low_ns[2*i + 1] > 0;

		XMC_CCU8_SLICE_DeadTimeInit(slice, &deadtime_conf);
	}

	return pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
}

static int pwm_xmc4xxx_ccu8_set_cycles(const struct device *dev, uint32_t channel,
			      uint32_t period_cycles, uint32_t pulse_cycles,
			      pwm_flags_t flags)
{
	const struct pwm_xmc4xxx_ccu8_config *config = dev->config;
	XMC_CCU8_SLICE_t *slice;
	uint32_t high_deadtime_value = 0, low_deadtime_value = 0;
	uint64_t cycles;
	int slice_idx = channel / 2;

	if (channel >= NUM_CHANNELS) {
		return -EINVAL;
	}

	if (period_cycles == 0 || period_cycles > UINT16_MAX + 1 || pulse_cycles > UINT16_MAX) {
		return -EINVAL;
	}

	slice = (XMC_CCU8_SLICE_t *)SLICE_ADDR_FROM_MODULE(config->ccu8, slice_idx);
	slice->PRS = period_cycles - 1;

	if (channel & 0x1) {
		slice->CR2S = period_cycles - pulse_cycles;
	} else {
		slice->CR1S = period_cycles - pulse_cycles;
	}
	slice->PSL = flags & PWM_POLARITY_INVERTED;

	/* set channel dead time */
	cycles = XMC_SCU_CLOCK_GetCcuClockFrequency() >> config->slice_prescaler[slice_idx];
	cycles >>= config->slice_deadtime_prescaler[slice_idx];
	high_deadtime_value = config->deadtime_high_ns[channel] * cycles / NSEC_PER_SEC;
	low_deadtime_value = config->deadtime_low_ns[channel] * cycles / NSEC_PER_SEC;

	if (high_deadtime_value > MAX_DEAD_TIME_VALUE || low_deadtime_value > MAX_DEAD_TIME_VALUE) {
		return -EINVAL;
	}

	XMC_CCU8_SLICE_SetDeadTimeValue(slice, channel & 0x1, high_deadtime_value,
					low_deadtime_value);

	XMC_CCU8_EnableShadowTransfer(config->ccu8, BIT(slice_idx * 4));

	/* start if not already running */
	XMC_CCU8_EnableClock(config->ccu8, slice_idx);
	XMC_CCU8_SLICE_StartTimer(slice);

	return 0;
}

static int pwm_xmc4xxx_ccu8_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					       uint64_t *cycles)
{
	const struct pwm_xmc4xxx_ccu8_config *config = dev->config;

	if (channel >= NUM_CHANNELS) {
		return -EINVAL;
	}

	*cycles = XMC_SCU_CLOCK_GetCcuClockFrequency() >> config->slice_prescaler[channel / 2];

	return 0;
}

static const struct pwm_driver_api pwm_xmc4xxx_ccu8_driver_api = {
	.set_cycles = pwm_xmc4xxx_ccu8_set_cycles,
	.get_cycles_per_sec = pwm_xmc4xxx_ccu8_get_cycles_per_sec,
};

#define PWM_XMC4XXX_CCU8_INIT(n)                                                           \
PINCTRL_DT_INST_DEFINE(n);                                                                 \
											   \
static const struct pwm_xmc4xxx_ccu8_config config##n = {                                  \
	.ccu8 = (CCU8_GLOBAL_TypeDef *)DT_INST_REG_ADDR(n),                                \
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
	.slice_prescaler = DT_INST_PROP(n, slice_prescaler),                               \
	.slice_deadtime_prescaler = DT_INST_PROP(n, slice_deadtime_prescaler),             \
	.deadtime_high_ns = DT_INST_PROP(n, channel_deadtime_high),                        \
	.deadtime_low_ns = DT_INST_PROP(n, channel_deadtime_low),                          \
};                                                                                         \
											   \
DEVICE_DT_INST_DEFINE(n, pwm_xmc4xxx_ccu8_init, NULL, NULL, &config##n, POST_KERNEL,       \
		      CONFIG_PWM_INIT_PRIORITY, &pwm_xmc4xxx_ccu8_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_XMC4XXX_CCU8_INIT)
