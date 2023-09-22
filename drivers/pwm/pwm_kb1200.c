/*
 * Copyright (c) 2023 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb1200_pwm

#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <soc.h>

#include <zephyr/logging/log.h>

#include <zephyr/drivers/gpio.h>
LOG_MODULE_REGISTER(pwm_kb1200, LOG_LEVEL_ERR);

/* Device config */
struct pwm_kb1200_config {
	/* pwm controller base address */
	uintptr_t *base_addr;
	uint32_t pwm_channel;
};

/* Driver data */
struct pwm_kb1200_data {
	/* PWM cycles per second */
	uint32_t cycles_per_sec;
};

/* Driver convenience defines */
#define DRV_CONFIG(dev)     ((const struct pwm_kb1200_config *)(dev)->config)
#define DRV_DATA(dev)       ((struct pwm_kb1200_data *)(dev)->data)
#define HAL_INSTANCE(dev)   (PWM_T *)(DRV_CONFIG(dev)->base_addr)

static const uint32_t pwm_kb1200_pin_cfg[11][2] = {
	{ PWM0_GPIO_Num, PINMUX_FUNC_B },   // GPIO3A
	{ PWM1_GPIO_Num, PINMUX_FUNC_C },   // GPIO38
	{ PWM2_GPIO_Num, PINMUX_FUNC_B },   // GPIO3B
	{ PWM3_GPIO_Num, PINMUX_FUNC_B },   // GPIO26
	{ PWM4_GPIO_Num, PINMUX_FUNC_B },   // GPIO31
	{ PWM5_GPIO_Num, PINMUX_FUNC_B },   // GPIO30
	{ PWM6_GPIO_Num, PINMUX_FUNC_B },   // GPIO37
	{ PWM7_GPIO_Num, PINMUX_FUNC_B },   // GPIO23
	{ PWM8_GPIO_Num, PINMUX_FUNC_C },   // GPIO00
	{ PWM9_GPIO_Num, PINMUX_FUNC_C }    // GPIO22
};

#define PWM_INPUT_FREQ_HI       32000000u
#define PWM_MAX_PRESCALER       (1UL << (6))
#define PWM_MAX_CYCLES          (1UL << (14))

/* PWM local functions */
//static void pwm_kb1200_configure(const struct device *dev, int clk_bus)
//{
	//const struct pwm_kb1200_config *config = dev->config;
	//struct pwm_reg *inst = config->base;
//}

/* PWM api functions */
static int pwm_kb1200_set_cycles(const struct device *dev, uint32_t channel,
			       uint32_t period_cycles, uint32_t pulse_cycles,
			       pwm_flags_t flags)
{
	/* Single channel for each pwm device */
	ARG_UNUSED(channel);
	const struct pwm_kb1200_config *config = DRV_CONFIG(dev);
	PWM_T *pwm = (PWM_T *)(config->base_addr);
	uint32_t port_num = config->pwm_channel;
	int prescaler;
	uint32_t HighLen;
	uint32_t CycleLen;

	if(((pwm->PWMCFG)&0x01) == 0x00)
	{
		PINMUX_DEV_T pinmux = gpio_pinmux(pwm_kb1200_pin_cfg[port_num][0]);
		gpio_pinmux_set(pinmux.port, pinmux.pin, pwm_kb1200_pin_cfg[port_num][1]);
	}

	/*
	 * Calculate PWM prescaler that let period_cycles map to
	 * maximum pwm period cycles and won't exceed it.
	 * Then prescaler = ceil (period_cycles / pwm_max_period_cycles)
	 */
	prescaler = DIV_ROUND_UP(period_cycles, PWM_MAX_CYCLES);
	if (prescaler > PWM_MAX_PRESCALER) {
		return -EINVAL;
	}

	HighLen  = (pulse_cycles / prescaler);
	CycleLen = (period_cycles / prescaler);

	/* Select PWM inverted polarity (ie. active-low pulse). */
	if (flags & PWM_POLARITY_INVERTED) {
		HighLen = CycleLen - HighLen;
	}

	/* Set PWM prescaler. */
	pwm->PWMCFG = (pwm->PWMCFG&0xC0FF)|((prescaler-1)<<8);
	
    /* 
     * period_cycles: PWM Cycle Length
     * pulse_cycles : PWM High Length    
     */
	*(uint16_t*)&pwm->PWMHIGH = HighLen;
	*(uint16_t*)&pwm->PWMCYC  = CycleLen;

    /* Start pwm */
    pwm->PWMCFG |= 0x01;
    
	return 0;
}

static int pwm_kb1200_get_cycles_per_sec(const struct device *dev,
				       uint32_t channel, uint64_t *cycles)
{
	/* Single channel for each pwm device */
	ARG_UNUSED(channel);
	ARG_UNUSED(dev);
	
	if (cycles) {
		/* User does not have to know about lowest clock,
		 * the driver will select the most relevant one.
		 */
		*cycles = PWM_INPUT_FREQ_HI;    //32Mhz
	}
	return 0;
}

/* PWM driver registration */
static const struct pwm_driver_api pwm_kb1200_driver_api = {
	.set_cycles = pwm_kb1200_set_cycles,
	.get_cycles_per_sec = pwm_kb1200_get_cycles_per_sec
};

static int pwm_kb1200_init(const struct device *dev)
{
	PWM_T *pwm = (PWM_T *)(DRV_CONFIG(dev)->base_addr);
	pwm->PWMCFG = PWM_SOURCE_CLK_32M|PWM_RULE1|PWM_PUSHPULL;

	return 0;
}

#define KB1200_PWM_INIT(inst)                                                 \
	static const struct pwm_kb1200_config pwm_kb1200_cfg_##inst = {           \
		.base_addr = (uintptr_t *)DT_INST_REG_ADDR(inst),                     \
		.pwm_channel = (uint32_t)DT_INST_PROP(inst, pwm_channel)                    \
	};                                                                        \
	static struct pwm_kb1200_data pwm_kb1200_data_##inst;                     \
	DEVICE_DT_INST_DEFINE(inst,					       \
			    &pwm_kb1200_init, NULL,			       \
			    &pwm_kb1200_data_##inst, &pwm_kb1200_cfg_##inst,       \
			    PRE_KERNEL_1, CONFIG_PWM_INIT_PRIORITY,	            \
			    &pwm_kb1200_driver_api);

DT_INST_FOREACH_STATUS_OKAY(KB1200_PWM_INIT)
