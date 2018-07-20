/*
 * Copyright (c) 2018 Kokoon Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <board.h>
#include <adc.h>
#include <device.h>
#include <kernel.h>
#include <init.h>

#define SYS_LOG_DOMAIN "SMT32F4_ADC"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_ADC_LEVEL
#include <logging/sys_log.h>

#include <clock_control/stm32_clock_control.h>

#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_adc.h>
#include <stm32f4xx_hal_adc_ex.h>
#include <stm32f4xx_hal_cortex.h>

#include "adc_stm32f4.h"

typedef struct adc_stm32f4_control {
	int initFlag;
	int isrConnectedFlag;
	struct k_sem adcReadSem;
	struct adc_drvData * actDrv;
	struct k_msgq adcVals;
	uint16_t adcValBuff[adcChannel_max];
} adc_stm32f4_control_t;


static adc_stm32f4_control_t ADCcontrol = {
	.initFlag = 0,
	.isrConnectedFlag = 0,
	.actDrv = NULL
};

ISR_DIRECT_DECLARE(ADC_IRQHandler)
{
	HAL_ADC_IRQHandler(&ADCcontrol.actDrv->hadc);
	ISR_DIRECT_PM(); /* PM done after servicing interrupt for best latency */
	return 1; /* We should check if scheduling decision should be made */
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
	uint16_t v;
	v = (uint16_t) HAL_ADC_GetValue(hadc);
	k_msgq_put(&ADCcontrol.adcVals, &v, K_NO_WAIT);
}

#define ADC_STM32F4_SETBITMASK(bit) (1 << (bit))

static void adc_stm32f4_enable(struct device *dev)
{
	const struct adc_config *config = dev->config->config_info;

	SYS_LOG_DBG("adc%u enable", (unsigned int)config->adcDevNum);
}

static void adc_stm32f4_disable(struct device *dev)
{
	const struct adc_config *config = dev->config->config_info;
	struct adc_drvData *drvData = dev->driver_data;

	SYS_LOG_DBG("adc%u disable", (unsigned int)config->adcDevNum);
	HAL_ADC_Stop_IT(&drvData->hadc);
}

static int adc_stm32f4_read(struct device *dev, struct adc_seq_table *seq_tbl)
{
	struct adc_drvData *drvData = dev->driver_data;
	const struct adc_config *config = dev->config->config_info;

	adc_channel_index_t i;
	uint32_t activeChannels;
	struct adc_seq_entry * pE;
	uint16_t val;
	int rc;

	rc = stm32adcError_None;
	SYS_LOG_DBG("adc%u conversion", (unsigned int)config->adcDevNum);
	k_sem_take(&ADCcontrol.adcReadSem, K_FOREVER);
	ADCcontrol.actDrv = drvData;
	// ok let's read for once the values
	HAL_ADC_Start_IT(&drvData->hadc);
	pE = seq_tbl->entries;
	activeChannels = config->activeChannels;
	i = adcChannel_0;
	while(activeChannels) {
		if(activeChannels & 0x1) {
			if (!k_msgq_get(&ADCcontrol.adcVals, &val,ADC_STM32_ADC_TIMEOUT_US)) {
				*((uint16_t *) pE->buffer) = val;
				pE++;
			} else {
				rc = stm32adcError_ADCtimeout;
				*((uint16_t *) pE->buffer) = 0;
				goto error;
			}
		}
		activeChannels >>= 1;
		i++;
	}

	HAL_ADC_Stop_IT(&drvData->hadc);
	k_sem_give(&ADCcontrol.adcReadSem);
	return rc;
error:
	HAL_ADC_Stop_IT(&drvData->hadc);
	k_msgq_purge(&ADCcontrol.adcVals);
	k_sem_give(&ADCcontrol.adcReadSem);
	return rc;
}

static struct adc_driver_api api_funcs = {
	.enable  = adc_stm32f4_enable,
	.disable = adc_stm32f4_disable,
	.read    = adc_stm32f4_read,
};


int adc_stm32f4_cfgChannel (ADC_HandleTypeDef * phadc, uint32_t c, uint32_t r) {
	ADC_ChannelConfTypeDef cfg;
	GPIO_InitTypeDef gpioCfg;
	GPIO_TypeDef * GPIOport;

	SYS_LOG_DBG("try to config adc channel %u (rank=%u) ...", c, r);

	cfg.Rank = r;
	cfg.SamplingTime = ADC_SAMPLETIME_480CYCLES;
	cfg.Offset = 0;
	
	gpioCfg.Mode = GPIO_MODE_ANALOG;
	gpioCfg.Pull = GPIO_NOPULL;

	// set the pins and special function registers
	GPIOport =  NULL;

	switch (c) {
		case adcChannel_0:
			GPIOport = GPIOA;
			gpioCfg.Pin = GPIO_PIN_0;
			cfg.Channel = ADC_CHANNEL_0;
			break;
		case adcChannel_1:
			GPIOport = GPIOA;
			gpioCfg.Pin = GPIO_PIN_1;
			cfg.Channel = ADC_CHANNEL_1;
			break;
		case adcChannel_2:
			GPIOport = GPIOA;
			gpioCfg.Pin = GPIO_PIN_2;
			cfg.Channel = ADC_CHANNEL_2;
			break;
		case adcChannel_3:
			GPIOport = GPIOA;
			gpioCfg.Pin = GPIO_PIN_3;
			cfg.Channel = ADC_CHANNEL_3;
			break;
		case adcChannel_4:
			GPIOport = GPIOA;
			gpioCfg.Pin = GPIO_PIN_4;
			cfg.Channel = ADC_CHANNEL_4;
			break;
		case adcChannel_5:
			GPIOport = GPIOA;
			gpioCfg.Pin = GPIO_PIN_5;
			cfg.Channel = ADC_CHANNEL_5;
			break;
		case adcChannel_6:
			GPIOport = GPIOA;
			gpioCfg.Pin = GPIO_PIN_6;
			cfg.Channel = ADC_CHANNEL_6;
			break;
		case adcChannel_7:
			GPIOport = GPIOA;
			gpioCfg.Pin = GPIO_PIN_7;
			cfg.Channel = ADC_CHANNEL_7;
			break;
		case adcChannel_8:
			GPIOport = GPIOB;
			gpioCfg.Pin = GPIO_PIN_0;
			cfg.Channel = ADC_CHANNEL_8;
			break;
		case adcChannel_9:
			GPIOport = GPIOB;
			gpioCfg.Pin = GPIO_PIN_1;
			cfg.Channel = ADC_CHANNEL_9;
			break;
		case adcChannel_10:
			GPIOport = GPIOC;
			gpioCfg.Pin = GPIO_PIN_0;
			cfg.Channel = ADC_CHANNEL_10;
			break;
		case adcChannel_11:
			GPIOport = GPIOC;
			gpioCfg.Pin = GPIO_PIN_1;
			cfg.Channel = ADC_CHANNEL_11;
			break;
		case adcChannel_12:
			GPIOport = GPIOC;
			gpioCfg.Pin = GPIO_PIN_2;
			cfg.Channel = ADC_CHANNEL_12;
			break;
		case adcChannel_13:
			GPIOport = GPIOC;
			gpioCfg.Pin = GPIO_PIN_3;
			cfg.Channel = ADC_CHANNEL_13;
			break;
		case adcChannel_14:
			GPIOport = GPIOC;
			gpioCfg.Pin = GPIO_PIN_4;
			cfg.Channel = ADC_CHANNEL_14;
			break;
		case adcChannel_15:
			GPIOport = GPIOC;
			gpioCfg.Pin = GPIO_PIN_5;
			cfg.Channel = ADC_CHANNEL_15;
			break;

		case adcChannel_vref:
			cfg.Channel = ADC_CHANNEL_VREFINT;
			ADC->CCR |= ADC_CCR_TSVREFE;
			break;

		case adcChannel_temp:
			cfg.Channel = ADC_CHANNEL_TEMPSENSOR;
			break;

		case adcChannel_vbat:
			cfg.Channel = ADC_CHANNEL_VBAT;
			ADC->CCR |= ADC_CCR_VBATE;
			break;

		default:
			// none
			break;
	}
	if(GPIOport) HAL_GPIO_Init(GPIOport, &gpioCfg);

	if (HAL_ADC_ConfigChannel(phadc, &cfg) != HAL_OK) {
		SYS_LOG_ERR("config adc channel failed");
		return stm32adcError_adcHALconfigChannel;
	}
	SYS_LOG_DBG("internal adc channel %u has been configured",cfg.Channel);

	return stm32adcError_None;
}

int adc_stm32f4_init(struct device *dev)
{
	const struct adc_config *config = dev->config->config_info;
	struct adc_drvData *drvData = dev->driver_data;
	adc_channel_index_t i;
	uint32_t r;
	uint32_t activeChannels;

	SYS_LOG_INF("init adc%u", (unsigned int)config->adcDevNum);

	// init adc control structure
	if (!ADCcontrol.initFlag) {
		k_sem_init(&ADCcontrol.adcReadSem, 0, 1);
		k_sem_give(&ADCcontrol.adcReadSem);
		k_msgq_init(&ADCcontrol.adcVals, (char *)ADCcontrol.adcValBuff, sizeof(uint16_t), sizeof(ADCcontrol.adcValBuff)/sizeof(uint16_t));
		ADCcontrol.actDrv = NULL;
		ADCcontrol.isrConnectedFlag = 0;
		ADCcontrol.initFlag = 1;
	}

	if(!ADCcontrol.isrConnectedFlag) {
		IRQ_DIRECT_CONNECT(ADC_IRQn, 0, ADC_IRQHandler, 0);
		irq_enable(ADC_IRQn);
		ADCcontrol.isrConnectedFlag = 1;
	}

	switch(config->adcDevNum) {
		#ifdef CONFIG_ADC_0
		case 0:
			__HAL_RCC_ADC1_CLK_ENABLE();
			drvData->hadc.Instance = ADC1;
			break;
		#endif

		#ifdef CONFIG_ADC_1
		case 1:
			__HAL_RCC_ADC2_CLK_ENABLE();
			drvData->hadc.Instance = ADC2;
			break;
		#endif
		#ifdef CONFIG_ADC_2
		case 2:
			__HAL_RCC_ADC3_CLK_ENABLE();
			drvData->hadc.Instance = ADC3;
			break;
		#endif
		default:
			SYS_LOG_ERR("unknown ADC unit");
			return stm32adcError_UnknownADCunit;
	}

	drvData->hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
	drvData->hadc.Init.Resolution = ADC_RESOLUTION_12B;
	drvData->hadc.Init.ScanConvMode = ENABLE;
	drvData->hadc.Init.ContinuousConvMode = DISABLE;
	drvData->hadc.Init.DiscontinuousConvMode = DISABLE;
	drvData->hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	drvData->hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	drvData->hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	drvData->hadc.Init.DMAContinuousRequests = DISABLE;
	drvData->hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	drvData->hadc.Init.NbrOfConversion = 0;
	activeChannels = config->activeChannels;
	while(activeChannels) {
		if(activeChannels & 1) drvData->hadc.Init.NbrOfConversion++;
		activeChannels >>= 1;
	}
	SYS_LOG_INF("use %u multiplexed channels", (unsigned int) drvData->hadc.Init.NbrOfConversion);

	// start the adc
	if (HAL_ADC_Init(&drvData->hadc) != HAL_OK) {
		SYS_LOG_ERR("HAL ADC init failed");
		return stm32adcError_adcHALinit;
	}

	// setup the sequencer for the channels
	i = 0;
	r = 1;
	activeChannels = config->activeChannels;
	while(activeChannels) {
		if(activeChannels & 1) {
			if(adc_stm32f4_cfgChannel(&drvData->hadc, (uint32_t) i, r)) {
				SYS_LOG_ERR("activate adc channel %u failed", i);
				return stm32adcError_configChannel;
			} else {
				SYS_LOG_INF("activate adc channel %u", i);
			}
			r++;
		}
		activeChannels >>= 1;
		i++;
	}


	return stm32adcError_None;
}


#ifdef CONFIG_ADC_0

struct adc_drvData adc_drvData_dev0 = {
	};

static struct adc_config adc_config_dev0 = {
		.adcDevNum = 0,
		.activeChannels = 0
		#ifdef CONFIG_ADC0_CHAN0
			+ ADC_STM32F4_SETBITMASK(adcChannel_0)
		#endif
		#ifdef CONFIG_ADC0_CHAN1
			+ ADC_STM32F4_SETBITMASK(adcChannel_1)
		#endif
		#ifdef CONFIG_ADC0_CHAN2
			+ ADC_STM32F4_SETBITMASK(adcChannel_2)
		#endif
		#ifdef CONFIG_ADC0_CHAN3
			+ ADC_STM32F4_SETBITMASK(adcChannel_3)
		#endif
		#ifdef CONFIG_ADC0_CHAN4
			+ ADC_STM32F4_SETBITMASK(adcChannel_4)
		#endif
		#ifdef CONFIG_ADC0_CHAN5
			+ ADC_STM32F4_SETBITMASK(adcChannel_5)
		#endif
		#ifdef CONFIG_ADC0_CHAN6
			+ ADC_STM32F4_SETBITMASK(adcChannel_6)
		#endif
		#ifdef CONFIG_ADC0_CHAN7
			+ ADC_STM32F4_SETBITMASK(adcChannel_7)
		#endif
		#ifdef CONFIG_ADC0_CHAN8
			+ ADC_STM32F4_SETBITMASK(adcChannel_8)
		#endif
		#ifdef CONFIG_ADC0_CHAN9
			+ ADC_STM32F4_SETBITMASK(adcChannel_9)
		#endif
		#ifdef CONFIG_ADC0_CHAN10
			+ ADC_STM32F4_SETBITMASK(adcChannel_10)
		#endif
		#ifdef CONFIG_ADC0_CHAN11
			+ ADC_STM32F4_SETBITMASK(adcChannel_11)
		#endif
		#ifdef CONFIG_ADC0_CHAN12
			+ ADC_STM32F4_SETBITMASK(adcChannel_12)
		#endif
		#ifdef CONFIG_ADC0_CHAN13
			+ ADC_STM32F4_SETBITMASK(adcChannel_13)
		#endif
		#ifdef CONFIG_ADC0_CHAN14
			+ ADC_STM32F4_SETBITMASK(adcChannel_14)
		#endif
		#ifdef CONFIG_ADC0_CHAN15
			+ ADC_STM32F4_SETBITMASK(adcChannel_15)
		#endif
			#ifdef CONFIG_ADC0_CHAN_TEMP
			+ ADC_STM32F4_SETBITMASK(adcChannel_temp)
		#endif
		#ifdef CONFIG_ADC0_CHAN_VREFINT
			+ ADC_STM32F4_SETBITMASK(adcChannel_vref)
		#endif
		#ifdef CONFIG_ADC0_CHAN_VABT
			+ ADC_STM32F4_SETBITMASK(adcChannel_vbat)
		#endif
		,
};


DEVICE_AND_API_INIT(adc_stm32f4, CONFIG_ADC_0_NAME, &adc_stm32f4_init,
		    &adc_drvData_dev0, &adc_config_dev0,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &api_funcs);
#endif // CONFIG_ADC_0

#ifdef CONFIG_ADC_1

struct adc_drvData adc_drvData_dev1 = {
	};

static struct adc_config adc_config_dev1 = {
		.adcDevNum = 0,
		.activeChannels = 0
		#ifdef CONFIG_ADC1_CHAN0
			+ ADC_STM32F4_SETBITMASK(adcChannel_0)
		#endif
		#ifdef CONFIG_ADC1_CHAN1
			+ ADC_STM32F4_SETBITMASK(adcChannel_1)
		#endif
		#ifdef CONFIG_ADC1_CHAN2
			+ ADC_STM32F4_SETBITMASK(adcChannel_2)
		#endif
		#ifdef CONFIG_ADC1_CHAN3
			+ ADC_STM32F4_SETBITMASK(adcChannel_3)
		#endif
		#ifdef CONFIG_ADC1_CHAN4
			+ ADC_STM32F4_SETBITMASK(adcChannel_4)
		#endif
		#ifdef CONFIG_ADC1_CHAN5
			+ ADC_STM32F4_SETBITMASK(adcChannel_5)
		#endif
		#ifdef CONFIG_ADC1_CHAN6
			+ ADC_STM32F4_SETBITMASK(adcChannel_6)
		#endif
		#ifdef CONFIG_ADC1_CHAN7
			+ ADC_STM32F4_SETBITMASK(adcChannel_7)
		#endif
		#ifdef CONFIG_ADC1_CHAN8
			+ ADC_STM32F4_SETBITMASK(adcChannel_8)
		#endif
		#ifdef CONFIG_ADC1_CHAN9
			+ ADC_STM32F4_SETBITMASK(adcChannel_9)
		#endif
		#ifdef CONFIG_ADC1_CHAN10
			+ ADC_STM32F4_SETBITMASK(adcChannel_10)
		#endif
		#ifdef CONFIG_ADC1_CHAN11
			+ ADC_STM32F4_SETBITMASK(adcChannel_11)
		#endif
		#ifdef CONFIG_ADC1_CHAN12
			+ ADC_STM32F4_SETBITMASK(adcChannel_12)
		#endif
		#ifdef CONFIG_ADC1_CHAN13
			+ ADC_STM32F4_SETBITMASK(adcChannel_13)
		#endif
		#ifdef CONFIG_ADC1_CHAN14
			+ ADC_STM32F4_SETBITMASK(adcChannel_14)
		#endif
		#ifdef CONFIG_ADC1_CHAN15
			+ ADC_STM32F4_SETBITMASK(adcChannel_15)
		#endif
			#ifdef CONFIG_ADC1_CHAN_TEMP
			+ ADC_STM32F4_SETBITMASK(adcChannel_temp)
		#endif
		#ifdef CONFIG_ADC1_CHAN_VREFINT
			+ ADC_STM32F4_SETBITMASK(adcChannel_vref)
		#endif
		#ifdef CONFIG_ADC1_CHAN_VABT
			+ ADC_STM32F4_SETBITMASK(adcChannel_vbat)
		#endif
		,
};


DEVICE_AND_API_INIT(adc_stm32f4, CONFIG_ADC_1_NAME, &adc_stm32f4_init,
		    &adc_drvData_dev1, &adc_config_dev1,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &api_funcs);
#endif // CONFIG_ADC_1

#ifdef CONFIG_ADC_2

struct adc_drvData adc_drvData_dev2 = {
	};

static struct adc_config adc_config_dev2 = {
		.adcDevNum = 0,
		.activeChannels = 0
		#ifdef CONFIG_ADC2_CHAN0
			+ ADC_STM32F4_SETBITMASK(adcChannel_0)
		#endif
		#ifdef CONFIG_ADC2_CHAN1
			+ ADC_STM32F4_SETBITMASK(adcChannel_1)
		#endif
		#ifdef CONFIG_ADC2_CHAN2
			+ ADC_STM32F4_SETBITMASK(adcChannel_2)
		#endif
		#ifdef CONFIG_ADC2_CHAN3
			+ ADC_STM32F4_SETBITMASK(adcChannel_3)
		#endif
		#ifdef CONFIG_ADC2_CHAN10
			+ ADC_STM32F4_SETBITMASK(adcChannel_10)
		#endif
		#ifdef CONFIG_ADC2_CHAN11
			+ ADC_STM32F4_SETBITMASK(adcChannel_11)
		#endif
		#ifdef CONFIG_ADC2_CHAN12
			+ ADC_STM32F4_SETBITMASK(adcChannel_12)
		#endif
		#ifdef CONFIG_ADC2_CHAN13
			+ ADC_STM32F4_SETBITMASK(adcChannel_13)
		#endif
			#ifdef CONFIG_ADC2_CHAN_TEMP
			+ ADC_STM32F4_SETBITMASK(adcChannel_temp)
		#endif
		#ifdef CONFIG_ADC2_CHAN_VREFINT
			+ ADC_STM32F4_SETBITMASK(adcChannel_vref)
		#endif
		#ifdef CONFIG_ADC2_CHAN_VABT
			+ ADC_STM32F4_SETBITMASK(adcChannel_vbat)
		#endif
		,
};


DEVICE_AND_API_INIT(adc_stm32f4, CONFIG_ADC_2_NAME, &adc_stm32f4_init,
		    &adc_drvData_dev2, &adc_config_dev2,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &api_funcs);
#endif // CONFIG_ADC_2
