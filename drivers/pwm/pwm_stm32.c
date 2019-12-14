/*
 * Copyright (c) 2016 Linaro Limited.
 * Copyright (c) 2019 Max van Kessel.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <soc.h>
#include <device.h>
#include <kernel.h>
#include <init.h>
#include <drivers/pwm.h>
#include <mfd/mfd_timer_stm32.h>

/* convenience defines */
#define DEV_CFG(dev)							\
	((const struct pwm_stm32_config * const)(dev)->config->config_info)
#define DEV_DATA(dev)							\
	((struct pwm_stm32_data * const)(dev)->driver_data)
#define DEV_BASE(data)							\
	((struct mfd_timer_stm32_data * const)(data)->tim->driver_data)

/** Configuration data */
struct pwm_stm32_config {
	char * parent;
	u8_t mode;
	bool preload;
	u8_t polarity;
};

/** Runtime driver data */
struct pwm_stm32_data {
	struct device *tim;		/**< Parent timer device */
};

#define CHAN(n)	LL_TIM_CHANNEL_CH##n
static const u32_t timer_channels[] = {
	CHAN(1),
	CHAN(2),
	CHAN(3),
	CHAN(4),
};

static int init(struct device *dev)
{
	int err = -EIO;
	const struct pwm_stm32_config 	*cfg = DEV_CFG(dev);
	struct pwm_stm32_data 		*data = DEV_DATA(dev);

	data->tim = device_get_binding(cfg->parent);
	if (data->tim != NULL) {
		err = 0;
	}

	return err;
}

/*
 * Set the period and pulse width for a PWM pin.
 *
 * Parameters
 * dev: Pointer to PWM device structure
 * pwm: PWM channel to set
 * period_cycles: Period (in timer count)
 * pulse_cycles: Pulse width (in timer count).
 *
 * return 0, or negative errno code
 */
static int pwm_stm32_pin_set(struct device *dev, u32_t pwm,
			     u32_t period_cycles, u32_t pulse_cycles,
			     pwm_flags_t flags)
{
	const struct pwm_stm32_config 	*cfg = DEV_CFG(dev);
	struct pwm_stm32_data 		*data = DEV_DATA(dev);
	const struct mfd_timer_stm32_data *parent = DEV_BASE(data);

	TIM_TypeDef *tim = (TIM_TypeDef *)parent->tim;
	bool counter_32b = false;

	if (period_cycles == 0U || pulse_cycles > period_cycles) {
		return -EINVAL;
	}

	if (flags) {
		/* PWM polarity not supported (yet?) */
		return -ENOTSUP;
	}

#ifdef CONFIG_SOC_SERIES_STM32F1X
	/* FIXME: IS_TIM_32B_COUNTER_INSTANCE not available on
	 * SMT32F1 Cube HAL since all timer counters are 16 bits
	 */
	counter_32b = 0;
#else
	counter_32b = IS_TIM_32B_COUNTER_INSTANCE(tim);
#endif

	/*
	 * The timer counts from 0 up to the value in the ARR register (16-bit).
	 * Thus period_cycles cannot be greater than UINT16_MAX + 1.
	 */
	if (!counter_32b && (period_cycles > 0x10000U)) {
		/* 16 bits counter does not support requested period
		 * You might want to adapt PWM output clock to adjust
		 * cycle durations to fit requested period into 16 bits
		 * counter
		 */
		return -ENOTSUP;
	}

	LL_TIM_SetAutoReload(tim, period_cycles - 1);

	u32_t config = cfg->polarity << TIM_CCER_CC1P_Pos;

	LL_TIM_OC_ConfigOutput(tim, timer_channels[pwm - 1], config);

	config = cfg->mode << TIM_CCMR1_OC1M_Pos;

	LL_TIM_OC_SetMode(tim, timer_channels[pwm - 1], config);

	if (pwm == 1U) {
		LL_TIM_OC_SetCompareCH1(tim, pulse_cycles);
	} else if (pwm == 2U) {
		LL_TIM_OC_SetCompareCH2(tim, pulse_cycles);
	} else if (pwm == 3U) {
		LL_TIM_OC_SetCompareCH3(tim, pulse_cycles);
	} else {
		LL_TIM_OC_SetCompareCH4(tim, pulse_cycles);
	}

	LL_TIM_CC_EnableChannel(tim, timer_channels[pwm - 1]);

	if (IS_TIM_BREAK_INSTANCE(tim)) {
		LL_TIM_EnableAllOutputs(tim);
	}

	mfd_timer_stm32_enable(data->tim);

	return 0;
}

static int get_cycles_per_sec(struct device *dev, u32_t pwm, u64_t *cycles)
{
	ARG_UNUSED(pwm);
	struct pwm_stm32_data *data = DEV_DATA(dev);

	return mfd_timer_stm32_get_cycles_per_sec(data->tim, cycles);
}

static const struct pwm_driver_api api = {
	.pin_set = pin_set,
	.get_cycles_per_sec = get_cycles_per_sec,
};

#define PWM_DEVICE_INIT_STM32(n)			  		\
static struct pwm_stm32_data pwm_stm32_dev_data_ ## n; 	  	  	\
static const struct pwm_stm32_config pwm_stm32_dev_cfg_ ## n = { 	\
	.parent = DT_INST_## n ##_ST_STM32_PWM_BUS_NAME,		\
	.mode = DT_INST_## n ##_ST_STM32_PWM_ST_COMPARE_MODE,		\
	.preload = DT_INST_## n ##_ST_STM32_PWM_ST_PRELOAD_ENABLE,	\
	.polarity = DT_INST_## n ##_ST_STM32_PWM_ST_POLARITY_ENUM,	\
};								  	\
								  	\
DEVICE_AND_API_INIT(pwm_stm32_ ## n,					\
		    DT_INST_## n ##_ST_STM32_PWM_LABEL,	 		\
		    &init,				  		\
		    &pwm_stm32_dev_data_ ## n,			  	\
		    &pwm_stm32_dev_cfg_ ## n,			  	\
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
		    &api)

#if DT_INST_0_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(0);
#endif

#if DT_INST_1_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(1);
#endif

#if DT_INST_2_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(2);
#endif

#if DT_INST_3_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(3);
#endif

#if DT_INST_4_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(4);
#endif

#if DT_INST_5_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(5);
#endif

#if DT_INST_6_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(6);
#endif

#if DT_INST_7_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(7);
#endif

#if DT_INST_8_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(8);
#endif

#if DT_INST_9_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(9);
#endif

#if DT_INST_10_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(10);
#endif

#if DT_INST_11_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(11);
#endif

#if DT_INST_12_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(12);
#endif

#if DT_INST_13_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(13);
#endif

#if DT_INST_14_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(14);
#endif

#if DT_INST_15_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(15);
#endif

#if DT_INST_16_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(16);
#endif

#if DT_INST_17_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(17);
#endif

#if DT_INST_18_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(18);
#endif

#if DT_INST_19_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(19);
#endif

#if DT_INST_20_ST_STM32_PWM
PWM_DEVICE_INIT_STM32(20);
#endif


