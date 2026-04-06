/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public extended API for the Infineon AutAnalog SAR ADC driver.
 *
 * Provides vendor-specific helpers for reading FIR and FIFO.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_INFINEON_AUTANALOG_SAR_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_INFINEON_AUTANALOG_SAR_H_

#include <zephyr/device.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Infineon AutAnalog SAR ADC extended API
 * @defgroup infineon_autanalog_sar_interface Infineon AutAnalog SAR ADC
 * @ingroup adc_interface_ext
 * @since 4.5
 * @version 1.0.0
 *
 * @note These vendor functions are not serialized against adc_read() and must
 *       not be called concurrently with an ADC conversion on the same device.
 *       They access the single shared SAR, and a call that overlaps an
 *       in-flight read (including an asynchronous adc_read_async()) can
 *       corrupt the conversion or the FIR configuration.  Callers are
 *       responsible for ensuring no conversion is in progress, e.g. by
 *       invoking them from the same thread that performs the reads.
 *
 * @{
 */

/**
 * @brief Read the FIR filter output result.
 *
 * @param dev  ADC device (infineon,autanalog-sar-adc instance).
 * @param fir_idx  FIR filter index (0 or 1).
 * @param result   Pointer to store the 32-bit signed FIR output value.
 *
 * @retval 0 on success.
 * @retval -EINVAL if parameters are invalid.
 * @retval -EBUSY if no FIR result is available yet.
 */
int adc_ifx_autanalog_sar_fir_read_result(const struct device *dev, uint8_t fir_idx,
					  int32_t *result);

/**
 * @brief Get the FIR range-detection (limit) status bitmap.
 *
 * Returns a bitmask indicating which FIR filters have triggered their
 * range-detection condition.  Bit 0 corresponds to FIR 0, bit 1 to FIR 1.
 *
 * @param dev     ADC device (infineon,autanalog-sar-adc instance).
 * @param fir_idx FIR filter index (0 or 1).
 * @param status  Pointer to the returned status bitmask.
 *
 * @retval 0 on success.
 * @retval -EINVAL if parameters are invalid.
 */
int adc_ifx_autanalog_sar_fir_get_limit_status(const struct device *dev, uint8_t fir_idx,
					       uint8_t *status);

/**
 * @brief Clear FIR range-detection (limit) status bits.
 *
 * @param dev       ADC device (infineon,autanalog-sar-adc instance).
 * @param fir_idx   FIR filter index (0 or 1).
 *
 * @retval 0 on success.
 * @retval -EINVAL if fir_idx is invalid.
 */
int adc_ifx_autanalog_sar_fir_clear_limit_status(const struct device *dev, uint8_t fir_idx);

/**
 * @brief Load FIR filter coefficients at runtime.
 *
 * Replaces the coefficient set for the specified FIR filter.  The FIR
 * filter must have been configured during initialization via the
 * devicetree binding.
 *
 * @warning This rewrites the FIR hardware configuration that the read path
 *          shares.  Do not call it while an ADC conversion is in progress on
 *          this device, or the conversion's FIR configuration may be torn.
 *
 * @param dev              ADC device (infineon,autanalog-sar-adc instance).
 * @param fir_idx          FIR filter index (0 or 1).
 * @param coefficients     Pointer to the coefficient array (Q1.15 format).
 * @param num_coefficients Number of coefficients (1 to 64).
 *
 * @retval 0 on success.
 * @retval -EINVAL if parameters are invalid.
 * @retval -EIO if the PDL operation failed.
 */
int adc_ifx_autanalog_sar_fir_load_coefficients(const struct device *dev, uint8_t fir_idx,
						const int16_t *coefficients,
						uint8_t num_coefficients);

/**
 * @brief Read data from a FIFO buffer.
 *
 * Reads up to @a max_words conversion results from the specified FIFO buffer.
 *
 * @param dev        ADC device (infineon,autanalog-sar-adc instance).
 * @param buf_idx    FIFO buffer index (0 to 7, depending on split config).
 * @param data_out   Pointer to the output buffer for 32-bit signed results.
 * @param max_words  Maximum number of words to read.
 *
 * @retval >=0 Number of words actually read.
 * @retval -EINVAL if parameters are invalid.
 */
int adc_ifx_autanalog_sar_fifo_read(const struct device *dev, uint8_t buf_idx, int32_t *data_out,
				    uint16_t max_words);

/**
 * @brief Read all data from a FIFO buffer.
 *
 * Drains the entire contents of the specified FIFO buffer into the output
 * array.  The caller must ensure that @a data_out is large enough to hold
 * the maximum buffer size (determined by the FIFO split configuration).
 *
 * @param dev        ADC device (infineon,autanalog-sar-adc instance).
 * @param buf_idx    FIFO buffer index (0 to 7, depending on split config).
 * @param data_out   Pointer to the output buffer for 32-bit signed results.
 *
 * @retval >=0 Number of words actually read.
 * @retval -EINVAL if parameters are invalid.
 */
int adc_ifx_autanalog_sar_fifo_read_all(const struct device *dev, uint8_t buf_idx,
					int32_t *data_out);

/**
 * @brief Read data and channel IDs from a FIFO buffer.
 *
 * Reads all data from the specified FIFO buffer together with the associated
 * channel identifiers.  The fifo-chan-id property must be enabled on the
 * parent SAR ADC node for channel IDs to be present in the FIFO data.
 *
 * @param dev        ADC device (infineon,autanalog-sar-adc instance).
 * @param buf_idx    FIFO buffer index (0 to 7, depending on split config).
 * @param input      ADC input type (0 = GPIO, 1 = MUX) for channel ID decoding.
 * @param data_out   Pointer to the output buffer for 32-bit signed results.
 * @param chan_id_out Pointer to the output buffer for channel ID values.
 *
 * @retval >=0 Number of words actually read.
 * @retval -EINVAL if parameters are invalid.
 */
int adc_ifx_autanalog_sar_fifo_read_chan_id(const struct device *dev, uint8_t buf_idx,
					    uint8_t input, int32_t *data_out, uint8_t *chan_id_out);

/**
 * @brief Get the current fill level of a FIFO buffer.
 *
 * Returns the number of data words currently stored in the specified
 * FIFO buffer.
 *
 * @param dev        ADC device (infineon,autanalog-sar-adc instance).
 * @param buf_idx    FIFO buffer index (0 to 7, depending on split config).
 * @param size       Pointer to store the number of words in the buffer.
 *
 * @retval 0 on success.
 * @retval -EINVAL if parameters are invalid.
 */
int adc_ifx_autanalog_sar_fifo_get_size(const struct device *dev, uint8_t buf_idx, uint16_t *size);

/**
 * @brief FIFO watermark interrupt callback type.
 *
 * Called from ISR context when a FIFO watermark interrupt fires.
 *
 * @param dev          ADC device.
 * @param fifo_status  FIFO interrupt status bitmask (CY_AUTANALOG_INT_FIFO_*).
 * @param user_data    User data passed to adc_ifx_autanalog_sar_fifo_set_callback().
 */
typedef void (*adc_ifx_autanalog_sar_fifo_callback_t)(const struct device *dev,
						      uint32_t fifo_status, void *user_data);

/**
 * @brief Register a callback for FIFO watermark interrupts.
 *
 * When a FIFO buffer fill level reaches its configured threshold, the
 * registered callback is invoked from ISR context with the FIFO interrupt
 * status bitmask.
 *
 * @param dev        ADC device (infineon,autanalog-sar-adc instance).
 * @param callback   Callback function, or NULL to unregister.
 * @param user_data  Opaque pointer passed to the callback.
 *
 * @retval 0 on success.
 */
int adc_ifx_autanalog_sar_fifo_set_callback(const struct device *dev,
					    adc_ifx_autanalog_sar_fifo_callback_t callback,
					    void *user_data);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ADC_INFINEON_AUTANALOG_SAR_H_ */
