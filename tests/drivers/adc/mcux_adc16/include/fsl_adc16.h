/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_DRIVERS_ADC_MCUX_ADC16_FSL_ADC16_H_
#define TESTS_DRIVERS_ADC_MCUX_ADC16_FSL_ADC16_H_

#include <stdbool.h>
#include <stdint.h>

/* Only the register fields and SDK interfaces used by the non-DMA driver. */
typedef struct {
	volatile uint32_t SC1[2];
	volatile uint32_t CFG1;
	volatile uint32_t CFG2;
	volatile uint32_t R[2];
	volatile uint32_t SC2;
} ADC_Type;

#define ADC_SC1_ADCH_MASK    0x1FU
#define ADC_SC1_ADCH_SHIFT   0U
#define ADC_SC1_DIFF_MASK    0x20U
#define ADC_SC1_AIEN_MASK    0x40U
#define ADC_SC1_COCO_MASK    0x80U
#define ADC_CFG1_MODE_MASK   0xCU
#define ADC_CFG1_MODE(x)     (((uint32_t)(x) << 2U) & ADC_CFG1_MODE_MASK)
#define ADC_CFG2_ADLSTS_MASK 0x3U
#define ADC_CFG2_ADLSTS(x)   ((uint32_t)(x) & ADC_CFG2_ADLSTS_MASK)
#define ADC_SC2_REFSEL_MASK  0x3U
#define ADC_SC2_REFSEL(x)    ((uint32_t)(x) & ADC_SC2_REFSEL_MASK)

typedef enum {
	kADC16_Resolution8or9Bit = 0U,
	kADC16_Resolution12or13Bit = 1U,
	kADC16_Resolution10or11Bit = 2U,
} adc16_resolution_t;

typedef enum {
	kADC16_HardwareAverageCount4 = 0U,
	kADC16_HardwareAverageCount8 = 1U,
	kADC16_HardwareAverageCount16 = 2U,
	kADC16_HardwareAverageCount32 = 3U,
	kADC16_HardwareAverageDisabled = 4U,
} adc16_hardware_average_mode_t;

typedef enum {
	kADC16_ReferenceVoltageSourceVref = 0U,
	kADC16_ReferenceVoltageSourceValt = 1U,
} adc16_reference_voltage_source_t;

typedef enum {
	kADC16_ClockDivider1 = 0U,
	kADC16_ClockDivider2 = 1U,
	kADC16_ClockDivider4 = 2U,
	kADC16_ClockDivider8 = 3U,
} adc16_clock_divider_t;

typedef enum {
	kADC16_ChannelMuxB = 1U,
} adc16_channel_mux_mode_t;

typedef struct {
	uint32_t channelNumber;
	bool enableInterruptOnConversionCompleted;
	bool enableDifferentialConversion;
} adc16_channel_config_t;

typedef struct {
	adc16_reference_voltage_source_t referenceVoltageSource;
	adc16_clock_divider_t clockDivider;
} adc16_config_t;

void ADC16_Init(ADC_Type *base, const adc16_config_t *config);
void ADC16_GetDefaultConfig(adc16_config_t *config);
void ADC16_SetHardwareAverage(ADC_Type *base, adc16_hardware_average_mode_t mode);
void ADC16_SetChannelMuxMode(ADC_Type *base, adc16_channel_mux_mode_t mode);
void ADC16_EnableHardwareTrigger(ADC_Type *base, bool enable);
void ADC16_SetChannelConfig(ADC_Type *base, uint32_t group, const adc16_channel_config_t *config);
uint32_t ADC16_GetChannelConversionValue(ADC_Type *base, uint32_t group);

#endif /* TESTS_DRIVERS_ADC_MCUX_ADC16_FSL_ADC16_H_ */
