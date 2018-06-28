//
// STM32F4xx HAL ADC driver
//
// author: Stefan Jaritz
// 	2018 Kokoon Technology Limited
#ifndef STM32F4_ADC_H_
#define STM32F4_ADC_H_

#include <zephyr/types.h>
#include <adc.h>

#ifdef __cplusplus
extern "C" {
#endif


//! type and enum for channel ids
typedef enum adcChannels {
	adcChannel_0 = ADC_CHANNEL_0,
	adcChannel_1 = ADC_CHANNEL_1,
	adcChannel_2 = ADC_CHANNEL_2,
	adcChannel_3 = ADC_CHANNEL_3,
	adcChannel_4 = ADC_CHANNEL_4,
	adcChannel_5 = ADC_CHANNEL_5,
	adcChannel_6 = ADC_CHANNEL_6,
	adcChannel_7 = ADC_CHANNEL_7,
	adcChannel_8 = ADC_CHANNEL_8,
	adcChannel_9 = ADC_CHANNEL_9,
	adcChannel_10 = ADC_CHANNEL_10,
	adcChannel_11 = ADC_CHANNEL_11,
	adcChannel_12 = ADC_CHANNEL_12,
	adcChannel_13 = ADC_CHANNEL_13,
	adcChannel_14 = ADC_CHANNEL_14,
	adcChannel_15 = ADC_CHANNEL_15,
	adcChannel_temp = ADC_CHANNEL_TEMPSENSOR, // 16
	adcChannel_vref = ADC_CHANNEL_VREFINT, // 17
	adcChannel_vbat = ADC_CHANNEL_VBAT, // 18
	adcChannel_max,
	adcChannel_unused,
} adc_channel_index_t;

enum stm32adcErrors {
	stm32adcError_None = 0,
	stm32adcError_HALerror = 10,
	stm32adcError_adcHALinit,
	stm32adcError_adcHALconfigChannel,
	stm32adcError_adcHALerror,
	stm32adcError_drvError = 100,
	stm32adcError_configChannel,
	stm32adcError_UnknownADCunit,
};

//! definition of the config structure
typedef struct adc_config {
	uint32_t adcDevNum; // number of the device
	uint32_t activeChannels; // bit mask defining the channels
} adc_config_t;

//! defiuntion of the driver data
typedef struct adc_drvData {
	// handle to adc defintion
	ADC_HandleTypeDef hadc;
} adc_drvData_t;


//
// @brief ADC Initialization function.
//
// Inits device model for the ADC IP from Dataware.
//
// @param dev Pointer to the device structure descriptor that
// will be initialized.
//
// @return Integer: 0 for success, error otherwise.
//
int adc_stm32f4_init(struct device *dev);

#ifdef __cplusplus
}
#endif

#endif  /*  STM32F4_ADC_H_ */
