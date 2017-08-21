/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Temperature sensor driver for STM32F401x chips.
 *
 * @warning Temperature readings should be use for RELATIVE
 *          TEMPERATURE CHANGES ONLY.
 *
 *          Inter-chip temperature sensor readings may vary by as much
 *          as 45 degrees C.
 */

#define SYS_LOG_DOMAIN "TEMPSTM32F401"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_SENSOR_LEVEL

#include <drivers/clock_control/stm32_clock_control.h>
#include <errno.h>
#include <logging/sys_log.h>
#include <misc/__assert.h>
#include <sensor.h>

#include "temp_stm32.h"

#define DEV_CFG(dev)							\
	((const struct temp_stm32_config*)(dev)->config->config_info)
#define DEV_DATA(dev) ((struct temp_stm32_data *)(dev)->driver_data)

/*
 * HACK: absent a better way to learn the analog voltage reference on
 * the board we are running on, hard-code V_REF+ to 3.3V, and assume
 * V_REF- = 0V.
 */
#define VREF_MILLIVOLTS 3300
#define VREF_VOLTS ((float)VREF_MILLIVOLTS / 1000.0f)

/*
 * See STM32F401xD, STM32F401xE datasheet 6.3.21 and chip
 * reference manual and ST RM0368 chapter 11.
 */
#define STM32F401_V25		0.76f	/* volts */
#define STM32F401_AVG_SLOPE	0.0025f /* volts / (degree C) */

#define ADC_RESOLUTION_12BIT 0x00
#define ADC_TO_VOLTS(adc_val) ((float)(adc_val) * VREF_VOLTS / 4095.0f)
#define ADC_PRESCALER_PCLK_DIV_8 0x03
#define ADC_SAMPLE_480_CYCLES 0x07
#define ADC_ONE_CONVERSION 0x00

/* This is valid for STM32F401x. */
#define ADC_TEMP_CHANNEL 18
#define ADC_TEMP_SMPR(adc) (adc)->SMPR1
#define ADC_TEMP_SMPR_SMP ADC_SMPR1_SMP18
#define ADC_TEMP_SMPR_POS ADC_SMPR1_SMP18_Pos

static int temp_stm32f401x_sample_fetch(struct device *dev,
					enum sensor_channel chan)
{
	const struct temp_stm32_config *cfg = DEV_CFG(dev);
	ADC_TypeDef *adc = cfg->adc;
	u32_t tmp;

	__ASSERT(cfg->adc_channel == ADC_TEMP_CHANNEL,
		 "Expected temperature sensor channel %d", ADC_TEMP_CHANNEL);

	/*
	 * Configure ADC for polled conversion of temperature sensor.
	 *
	 * TODO: this configuration could be improved (timing, etc.).
	 */

	/*
	 * CR1:
	 *
	 * - 12 bit resolution (the maximum).
	 *
	 * - Software initiated, polled conversion only (no
	 *   interrupts, injected groups, scan mode, etc.).
	 */

	tmp = adc->CR1;
	tmp &= ~(ADC_CR1_RES | ADC_CR1_JDISCEN | ADC_CR1_DISCEN |
		 ADC_CR1_JAUTO | ADC_CR1_SCAN | ADC_CR1_JEOCIE |
		 ADC_CR1_AWDIE | ADC_CR1_EOCIE);
	tmp |= (ADC_RESOLUTION_12BIT << ADC_CR1_RES_Pos);
	adc->CR1 = tmp;

	/*
	 * CR2:
	 *
	 * - Software initiated, polled conversion of a single channel
	 *   only (no external triggers, DMA, continuous conversion).
	 *
	 * - Right-aligned data in DR.
	 *
	 * - EOC bit in SR should be set at end of conversion.
	 */

	adc->CR2 &= ~(ADC_CR2_EXTEN | ADC_CR2_JEXTEN | ADC_CR2_ALIGN |
		      ADC_CR2_EOCS | ADC_CR2_DDS | ADC_CR2_DMA |
		      ADC_CR2_CONT);

	/*
	 * SMPR:
	 *
	 * Temperature sensor sample time is 480 ADC cycles (the
	 * maximum).
	 */

	tmp = ADC_TEMP_SMPR(adc);
	tmp &= ~ADC_TEMP_SMPR_SMP;
	tmp |= (ADC_SAMPLE_480_CYCLES << ADC_TEMP_SMPR_POS);
	ADC_TEMP_SMPR(adc) = tmp;

	/*
	 * SQRx: Convert channel. One channel is being converted.
	 */

	tmp = adc->SQR3;
	tmp &= ~ADC_SQR3_SQ1;
	tmp |= (ADC_TEMP_CHANNEL << ADC_SQR3_SQ1_Pos);
	adc->SQR3 = tmp;

	tmp = adc->SQR1;
	tmp &= ~ADC_SQR1_L;
	tmp |= (ADC_ONE_CONVERSION << ADC_SQR1_L_Pos);
	adc->SQR1 = tmp;

	/*
	 * Start the ADC conversion, and wait for it to complete.
	 */

	adc->SR = ~(ADC_SR_EOC | ADC_SR_STRT);
	adc->CR2 |= ADC_CR2_SWSTART;

	do {
	} while (!(adc->SR & ADC_SR_EOC));
	adc->SR = ~(ADC_SR_EOC | ADC_SR_STRT);

	return 0;
}

static inline float stm32f401x_temp_c(float v_sense)
{
	/*
	 * The voltage read by the temperature sensor, v_sense, is a
	 * linear function of the temperature. The point-slope form of
	 * the line is:
	 *
	 * (Temperature - 25.0°C) * Avg_Slope = v_sense - V_25
	 *
	 * Where V_25 is the measured voltage at 25°C, and Avg_Slope
	 * is the slope of the line in V/°C.
	 *
	 * Use this formula to convert the voltage to a
	 * temperature. See ST RM0368 11.9 and the chip datasheet for
	 * more details.
	 */
	return ((v_sense - STM32F401_V25) / STM32F401_AVG_SLOPE + 25.0f);
}

static int temp_stm32f401x_channel_get(struct device *dev,
				       enum sensor_channel chan,
				       struct sensor_value *val)
{
	const struct temp_stm32_config *cfg = DEV_CFG(dev);
	ADC_TypeDef *adc = cfg->adc;
	u32_t adc_dr = adc->DR;
	float v_sense = ADC_TO_VOLTS(adc_dr);
	float deg_c = stm32f401x_temp_c(v_sense);

	/* TODO fractional part */
	val->val1 = (int32_t)deg_c;
	val->val2 = 0;
	return 0;
}

static const struct sensor_driver_api temp_stm32f401x_driver_api = {
	.sample_fetch = temp_stm32f401x_sample_fetch,
	.channel_get = temp_stm32f401x_channel_get,
};

static inline void __temp_stm32f401x_get_clock(struct device *dev)
{
	struct temp_stm32_data *data = DEV_DATA(dev);
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	__ASSERT_NO_MSG(clk);
	data->clock = clk;
}

static int temp_stm32f401x_init(struct device *dev)
{
	const struct temp_stm32_config *cfg = DEV_CFG(dev);
	struct temp_stm32_data *data = DEV_DATA(dev);
	ADC_TypeDef *adc = cfg->adc;
	ADC_Common_TypeDef *adc_common = cfg->adc_common;
	u32_t tmp;

	/* Turn on digital clock. */
        __temp_stm32f401x_get_clock(dev);
	clock_control_on(data->clock,
			 (clock_control_subsys_t*)&cfg->pclken);

	/* Turn on ADC. */
	adc->CR2 |= ADC_CR2_ADON;

	/* ADC configuration for temperature sensor.
	 *
	 * - ADC clock prescaler to slowest possible (HACK) to avoid
	 *   reading data sheets to figure out fastest possible.
	 *
	 * - Select temperature sensor, deselect VBAT (they are
	 *   mutually exclusive, and VBAT has precedence).
	 */
	tmp = adc_common->CCR;
	tmp &= ~(ADC_CCR_VBATE | ADC_CCR_ADCPRE);
	tmp |= ((ADC_PRESCALER_PCLK_DIV_8 << ADC_CCR_ADCPRE_Pos) |
		ADC_CCR_TSVREFE);
	adc_common->CCR = tmp;

	return 0;
}

static struct temp_stm32_config temp_stm32f401x_config = {
	.adc = ADC1,
	.adc_common = ADC,
	.adc_channel = ADC_TEMP_CHANNEL,
	.pclken = { .bus = STM32_CLOCK_BUS_APB2,
		    .enr = LL_APB2_GRP1_PERIPH_ADC1 },
};

static struct temp_stm32_data temp_stm32f401x_data;

DEVICE_AND_API_INIT(temp_stm32f401x, CONFIG_TEMP_STM32F401X_NAME,
		    temp_stm32f401x_init, &temp_stm32f401x_data,
		    &temp_stm32f401x_config, POST_KERNEL,
		    CONFIG_SENSOR_INIT_PRIORITY, &temp_stm32f401x_driver_api);
