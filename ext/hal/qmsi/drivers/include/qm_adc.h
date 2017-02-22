/*
 * Copyright (c) 2017, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QM_ADC_H__
#define __QM_ADC_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

#if (QUARK_D2000)

/**
 * Analog to Digital Converter (ADC).
 *
 * @defgroup groupADC Quark D2000 ADC
 * @{
 */

/**
 * ADC sample size type.
 */
typedef uint16_t qm_adc_sample_t;

/**
 * ADC calibration type.
 */
typedef uint8_t qm_adc_calibration_t;

typedef enum {
	QM_ADC_IDLE,     /**< ADC idle. */
	QM_ADC_COMPLETE, /**< ADC transfer complete. */
	QM_ADC_OVERFLOW, /**< ADC FIFO overflow error. */
} qm_adc_status_t;

/**
 * ADC resolution type.
 */
typedef enum {
	QM_ADC_RES_6_BITS,  /**< 6-bit mode. */
	QM_ADC_RES_8_BITS,  /**< 8-bit mode. */
	QM_ADC_RES_10_BITS, /**< 10-bit mode. */
	QM_ADC_RES_12_BITS  /**< 12-bit mode. */
} qm_adc_resolution_t;

/**
 * ADC operating mode type.
 */
typedef enum {
	QM_ADC_MODE_DEEP_PWR_DOWN, /**< Deep power down mode. */
	QM_ADC_MODE_PWR_DOWN,      /**< Power down mode. */
	QM_ADC_MODE_STDBY,	 /**< Standby mode. */
	QM_ADC_MODE_NORM_CAL,      /**< Normal mode, with calibration. */
	QM_ADC_MODE_NORM_NO_CAL    /**< Normal mode, no calibration. */
} qm_adc_mode_t;

/**
 * ADC channels type.
 */
typedef enum {
	QM_ADC_CH_0,  /**< ADC Channel 0. */
	QM_ADC_CH_1,  /**< ADC Channel 1. */
	QM_ADC_CH_2,  /**< ADC Channel 2. */
	QM_ADC_CH_3,  /**< ADC Channel 3. */
	QM_ADC_CH_4,  /**< ADC Channel 4. */
	QM_ADC_CH_5,  /**< ADC Channel 5. */
	QM_ADC_CH_6,  /**< ADC Channel 6. */
	QM_ADC_CH_7,  /**< ADC Channel 7. */
	QM_ADC_CH_8,  /**< ADC Channel 8. */
	QM_ADC_CH_9,  /**< ADC Channel 9. */
	QM_ADC_CH_10, /**< ADC Channel 10. */
	QM_ADC_CH_11, /**< ADC Channel 11. */
	QM_ADC_CH_12, /**< ADC Channel 12. */
	QM_ADC_CH_13, /**< ADC Channel 13. */
	QM_ADC_CH_14, /**< ADC Channel 14. */
	QM_ADC_CH_15, /**< ADC Channel 15. */
	QM_ADC_CH_16, /**< ADC Channel 16. */
	QM_ADC_CH_17, /**< ADC Channel 17. */
	QM_ADC_CH_18  /**< ADC Channel 18. */
} qm_adc_channel_t;

/**
 * ADC interrupt callback source.
 */
typedef enum {
	QM_ADC_TRANSFER,     /**< Transfer complete or error callback. */
	QM_ADC_MODE_CHANGED, /**< Mode change complete callback. */
	QM_ADC_CAL_COMPLETE, /**< Calibration complete callback. */
} qm_adc_cb_source_t;

/**
 * ADC configuration type.
 */
typedef struct {
	/**
	 * Sample interval in ADC clock cycles, defines the period to wait
	 * between the start of each sample and can be in the range
	 * [(resolution+2) - 255].
	 */
	uint8_t window;
	qm_adc_resolution_t resolution; /**< 12, 10, 8, 6-bit resolution. */
} qm_adc_config_t;

/**
 * ADC transfer type.
 */
typedef struct {
	qm_adc_channel_t *ch; /**< Channel sequence array (1-32 channels). */
	uint8_t ch_len;       /**< Number of channels in the above array. */
	qm_adc_sample_t *samples; /**< Array to store samples. */
	uint32_t samples_len;     /**< Length of sample array. */

	/**
	 * Transfer callback.
	 *
	 * Called when a conversion is performed or an error is detected.
	 *
	 * @param[in] data The callback user data.
	 * @param[in] error 0 on success.
	 *                  Negative @ref errno for possible error codes.
	 * @param[in] status ADC status.
	 * @param[in] source Interrupt callback source.
	 */
	void (*callback)(void *data, int error, qm_adc_status_t status,
			 qm_adc_cb_source_t source);
	void *callback_data; /**< Callback user data. */
} qm_adc_xfer_t;

/**
 * Switch operating mode of ADC.
 *
 * This call is blocking.
 *
 * @param[in] adc Which ADC to enable.
 * @param[in] mode ADC operating mode.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_adc_set_mode(const qm_adc_t adc, const qm_adc_mode_t mode);

/**
 * Switch operating mode of ADC.
 *
 * This call is non-blocking and will call the user callback on completion.
 *
 * @param[in] adc Which ADC to enable.
 * @param[in] mode ADC operating mode.
 * @param[in] callback Callback called on completion.
 * @param[in] callback_data The callback user data.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_adc_irq_set_mode(const qm_adc_t adc, const qm_adc_mode_t mode,
			void (*callback)(void *data, int error,
					 qm_adc_status_t status,
					 qm_adc_cb_source_t source),
			void *callback_data);

/**
 * Calibrate the ADC.
 *
 * It is necessary to calibrate if it is intended to use Normal Mode With
 * Calibration. The calibration must be performed if the ADC is used for the
 * first time or has been in deep power down mode. This call is blocking.
 *
 * @param[in] adc Which ADC to calibrate.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_adc_calibrate(const qm_adc_t adc);

/**
 * Calibrate the ADC.
 *
 * It is necessary to calibrate if it is intended to use Normal Mode With
 * Calibration. The calibration must be performed if the ADC is used for the
 * first time or has been in deep power down mode. This call is non-blocking
 * and will call the user callback on completion.
 *
 * @param[in] adc Which ADC to calibrate.
 * @param[in] callback Callback called on completion.
 * @param[in] callback_data The callback user data.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_adc_irq_calibrate(const qm_adc_t adc,
			 void (*callback)(void *data, int error,
					  qm_adc_status_t status,
					  qm_adc_cb_source_t source),
			 void *callback_data);

/**
 * Set ADC calibration data.
 *
 * @param[in] adc Which ADC to set calibration for.
 * @param[in] cal Calibration data.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_adc_set_calibration(const qm_adc_t adc, const qm_adc_calibration_t cal);

/**
 * Get the current calibration data for an ADC.
 *
 * @param[in] adc Which ADC to get calibration for.
 * @param[out] cal Calibration data. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_adc_get_calibration(const qm_adc_t adc, qm_adc_calibration_t *const cal);

/**
 * Set ADC configuration.
 *
 * This sets the sample window and resolution.
 *
 * @param[in] adc Which ADC to configure.
 * @param[in] cfg ADC configuration. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_adc_set_config(const qm_adc_t adc, const qm_adc_config_t *const cfg);

/**
 * Synchronously read values from the ADC.
 *
 * This blocking call can read 1-32 ADC values into the array provided.
 *
 * @param[in] adc Which ADC to read.
 * @param[in,out] xfer Channel and sample info. This must not be NULL.
 * @param[out] status Get status of the ADC device.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_adc_convert(const qm_adc_t adc, qm_adc_xfer_t *const xfer,
		   qm_adc_status_t *const status);

/**
 * Asynchronously read values from the ADC.
 *
 * This is a non-blocking call and will call the user provided callback after
 * the requested number of samples have been converted.
 *
 * @param[in] adc Which ADC to read.
 * @param[in,out] xfer Channel sample and callback info. This must not be NULL.
 *
 * @return Standard errno return type for QMSI.
 * @retval 0 on success.
 * @retval Negative @ref errno for possible error codes.
 */
int qm_adc_irq_convert(const qm_adc_t adc, qm_adc_xfer_t *const xfer);

/**
 * @}
 */
#endif /* QUARK_D2000 */

#endif /* __QM_ADC_H__ */
