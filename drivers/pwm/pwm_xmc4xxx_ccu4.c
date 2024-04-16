/*
 * Copyright (c) 2023 SLB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_xmc4xxx_ccu4_pwm

#include <soc.h>
#include <zephyr/dt-bindings/pwm/pwm.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/pinctrl.h>

#include <xmc_ccu4.h>
#include <xmc_scu.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pwm_xmc4xxx_ccu4, CONFIG_PWM_LOG_LEVEL);

#define NUM_SLICES 4
#define NUM_CHANNELS NUM_SLICES
#define SLICE_ADDR_FROM_MODULE(module_ptr, idx) ((uint32_t)(module_ptr) + ((idx) + 1) * 0x100)

struct pwm_xmc4xxx_ccu4_config {
	XMC_CCU4_MODULE_t *ccu4;
	const struct pinctrl_dev_config *pcfg;
	const uint8_t slice_prescaler[NUM_SLICES];
};

static int pwm_xmc4xxx_ccu4_init(const struct device *dev)
{
	const struct pwm_xmc4xxx_ccu4_config *config = dev->config;

	/* enables the CCU4 clock and ungates clock to CCU4x */
	XMC_CCU4_EnableModule(config->ccu4);
	XMC_CCU4_StartPrescaler(config->ccu4);

	for (int i = 0; i < NUM_SLICES; i++) {
		XMC_CCU4_SLICE_t *slice;
		XMC_CCU4_SLICE_COMPARE_CONFIG_t slice_conf = {
			.prescaler_initval = config->slice_prescaler[i]
		};

		slice = (XMC_CCU4_SLICE_t *)SLICE_ADDR_FROM_MODULE(config->ccu4, i);
		XMC_CCU4_SLICE_CompareInit(slice, &slice_conf);
	}

	return pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
}

static int pwm_xmc4xxx_ccu4_set_cycles(const struct device *dev, uint32_t channel,
			      uint32_t period_cycles, uint32_t pulse_cycles,
			      pwm_flags_t flags)
{
	const struct pwm_xmc4xxx_ccu4_config *config = dev->config;
	XMC_CCU4_SLICE_t *slice;
	int slice_idx = channel;

	if (channel >= NUM_CHANNELS) {
		return -EINVAL;
	}

	if (period_cycles == 0 || period_cycles > UINT16_MAX + 1 || pulse_cycles > UINT16_MAX) {
		return -EINVAL;
	}

	slice = (XMC_CCU4_SLICE_t *)SLICE_ADDR_FROM_MODULE(config->ccu4, slice_idx);
	slice->PRS = period_cycles - 1;
	slice->CRS = period_cycles - pulse_cycles;
	slice->PSL = flags & PWM_POLARITY_INVERTED;

	XMC_CCU4_EnableShadowTransfer(config->ccu4, BIT(slice_idx * 4));

	/* start if not already running */
	XMC_CCU4_EnableClock(config->ccu4, slice_idx);
	XMC_CCU4_SLICE_StartTimer(slice);

	return 0;
}

static int pwm_xmc4xxx_ccu4_get_cycles_per_sec(const struct device *dev, uint32_t channel,
					       uint64_t *cycles)
{
	const struct pwm_xmc4xxx_ccu4_config *config = dev->config;
	int slice_idx = channel;

	if (channel >= NUM_SLICES) {
		return -EINVAL;
	}

	*cycles = XMC_SCU_CLOCK_GetCcuClockFrequency() >> config->slice_prescaler[slice_idx];

	return 0;
}

static const struct pwm_driver_api pwm_xmc4xxx_ccu4_driver_api = {
	.set_cycles = pwm_xmc4xxx_ccu4_set_cycles,
	.get_cycles_per_sec = pwm_xmc4xxx_ccu4_get_cycles_per_sec,
};

#define PWM_XMC4XXX_CCU4_INIT(n)                                                           \
PINCTRL_DT_INST_DEFINE(n);                                                                 \
											   \
static const struct pwm_xmc4xxx_ccu4_config config##n = {                                  \
	.ccu4 = (CCU4_GLOBAL_TypeDef *)DT_INST_REG_ADDR(n),                                \
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
	.slice_prescaler = DT_INST_PROP(n, slice_prescaler)};                              \
											   \
DEVICE_DT_INST_DEFINE(n, pwm_xmc4xxx_ccu4_init, NULL, NULL, &config##n, POST_KERNEL,       \
		      CONFIG_PWM_INIT_PRIORITY, &pwm_xmc4xxx_ccu4_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PWM_XMC4XXX_CCU4_INIT)
