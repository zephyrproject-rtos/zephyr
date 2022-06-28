/*
 * Copyright (c) 2022 Wolter HV <wolterhv@gmx.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_ADC_ESP32_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_ADC_ESP32_H_

/*!
 * @brief Attenuation enum to decouple Zephyr API from ESP-IDF API.
 */
enum adc_esp32_atten_e {
	ADC_ESP32_ATTEN_0 = 0,
	ADC_ESP32_ATTEN_1,
	ADC_ESP32_ATTEN_2,
	ADC_ESP32_ATTEN_3,
	ADC_ESP32_ATTEN_COUNT,
};

/*!
 * @brief Gets the attenuation on an ADC channel.
 *
 * @param dev		the ADC device handle
 * @param channel_id	the channel
 * @param atten		the attenuation
 *
 * @retval 0		on success
 * @retval -EINVAL	if channel_id is invalid
 */
int adc_esp32_get_atten(const struct device     *dev,
			const uint8_t            channel_id,
			enum adc_esp32_atten_e  *atten);

/*!
 * @brief Sets the attenuation on an ADC channel.
 *
 * @param dev		the ADC device handle
 * @param channel_id	the channel
 * @param atten		the attenuation
 *
 * @retval 0		on success
 * @retval -EINVAL	if channel_id is invalid
 * @retval -EINVAL	if attenuation is invalid
 */
int adc_esp32_set_atten(const struct device           *dev,
			const uint8_t                  channel_id,
			const enum adc_esp32_atten_e   atten);


/*!
 * @brief Updates the internal reference voltage measurement.
 */
int adc_esp32_update_meas_ref_internal(const struct device *dev);

/*!
 * @brief Sets the parameter to the value of the last reference voltage
 * measurement.
 */
int adc_esp32_get_meas_ref_internal(const struct device  *dev,
				    uint16_t             *meas_ref_internal);

/*!
 * @brief Characterizes the ADC channel for accurate conversions from raw values
 * to millivolts.
 */
int adc_esp32_characterize_by_channel(const struct device  *dev,
			              const uint8_t         resolution,
			              const uint8_t         channel_id);

int adc_esp32_characterize_by_atten(const struct device           *dev,
			            const uint8_t                  resolution,
			            const enum adc_esp32_atten_e   atten);
/*!
 * @brief Converts an ADC raw measurement into a value in millivolts.
 *
 * The converted value is stored at the location where the raw value was read
 * from.
 *
 * Conversions are performed considering the following look-up table:
 *
 * | adc_esp32_atten_e | Gain / dB | Range start / mV | Range end / mV |
 * |:-----------------:+----------:+-----------------:+---------------:|
 * | ADC_ESP32_ATTEN_0 |       0,0 |              100 |            950 |
 * | ADC_ESP32_ATTEN_1 |       2,5 |              100 |           1250 |
 * | ADC_ESP32_ATTEN_2 |       6,0 |              150 |           1750 |
 * | ADC_ESP32_ATTEN_3 |      11,0 |              150 |           2450 |
 *
 * Source: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html?highlight=adc#adc-attenuation
 *
 * Note: A linear correlation is assumed between the real voltage and the raw
 * value, though this is not exactly true.
 *
 * @param dev			the ADC device handle
 * @param channel_id		the channel
 * @param resolution		the resolution / width
 * @param adc_ref_voltage	the reference voltage
 * @param valp			pointer to the raw value
 *
 * @retval 0		on success
 * @retval -EINVAL	if channel_id is invalid
 * @retval -EINVAL	if resolution is invalid
 * @retval -EINVAL	if the reference voltage is negative
 * @retval -EINVAL	if the raw value is out of range according to the
 *			resolution
 */
/* At some point, a "characterisation" of the ADC may be needed. In the example
 * it is called with esp_adc_cal_characterize. */
/* After this, all that is needed is calling adc1_get_raw or adc2_get_raw. */
/* To convert a raw value to a physical quantity, use
 * esp_adc_cal_raw_to_voltage. */
int adc_esp32_raw_to_millivolts(const struct device  *dev,
			        int32_t              *value);

#endif /* ZEPHYR_INCLUDE_DRIVERS_ADC_ADC_ESP32_H_ */
