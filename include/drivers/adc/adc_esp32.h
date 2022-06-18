/* SPDX-License-Identifier: Apache-2.0 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_ADC_ESP32_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_ADC_ESP32_H_

enum adc_esp32_atten_e {
	ADC_ESP32_ATTEN_0 = 0,
	ADC_ESP32_ATTEN_1,
	ADC_ESP32_ATTEN_2,
	ADC_ESP32_ATTEN_3,
	ADC_ESP32_ATTEN_COUNT,
};

/* This method sets attenuation */
/*
 * | Atten. Ref. | Gain | Min volt. | Max. volt. |
 * |:-----------:+-----:+----------:+-----------:|
 * |      0      |  0,0 |       100 |        950 |
 * |      1      |  2,5 |       100 |       1250 |
 * |      2      |  6,0 |       150 |       1750 |
 * |      3      | 11,0 |       150 |       2450 |
 */
/* For now, assume linear progression between limits and do a simple
 * linear interpolation */
int adc_esp32_set_atten(const struct device           *dev,
			const uint8_t                  channel_id,
			const enum adc_esp32_atten_e   atten);

/* At some point, a "characterisation" of the ADC may be needed. In the example
 * it is called with esp_adc_cal_characterize. */
/* After this, all that is needed is calling adc1_get_raw or adc2_get_raw. */
/* To convert a raw value to a physical quantity, use
 * esp_adc_cal_raw_to_voltage. */
/* In the adc_hal.h, the functions adc_ll_set_atten and
 * adc_ll_rtc_enable_channel are used, which suggests that these may the HAL
 * ways of setting up a channel. */
int adc_esp32_raw_to_millivolts(const struct device  *dev,
				const uint8_t         channel_id,
				const uint8_t         resolution,
				const int32_t         adc_ref_voltage,
			        int32_t              *valp);

#endif /* ZEPHYR_INCLUDE_DRIVERS_ADC_ADC_ESP32_H_ */
