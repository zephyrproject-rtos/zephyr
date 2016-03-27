/*
 * Copyright (c) 2015, Intel Corporation
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
* Analog to Digital Converter (ADC) driver for Quark Microcontrollers.
*
* @defgroup groupADC ADC
* @{
*/

/**
 * ADC Resolution type.
 */
typedef enum {
	QM_ADC_RES_6_BITS,
	QM_ADC_RES_8_BITS,
	QM_ADC_RES_10_BITS,
	QM_ADC_RES_12_BITS
} qm_adc_resolution_t;

/**
 * ADC operating mode type.
 */
typedef enum {
	QM_ADC_MODE_DEEP_PWR_DOWN,
	QM_ADC_MODE_PWR_DOWN,
	QM_ADC_MODE_STDBY,
	QM_ADC_MODE_NORM_CAL,
	QM_ADC_MODE_NORM_NO_CAL
} qm_adc_mode_t;

/**
 * ADC channels type.
*/
typedef enum {
	QM_ADC_CH_0,
	QM_ADC_CH_1,
	QM_ADC_CH_2,
	QM_ADC_CH_3,
	QM_ADC_CH_4,
	QM_ADC_CH_5,
	QM_ADC_CH_6,
	QM_ADC_CH_7,
	QM_ADC_CH_8,
	QM_ADC_CH_9,
	QM_ADC_CH_10,
	QM_ADC_CH_11,
	QM_ADC_CH_12,
	QM_ADC_CH_13,
	QM_ADC_CH_14,
	QM_ADC_CH_15,
	QM_ADC_CH_16,
	QM_ADC_CH_17,
	QM_ADC_CH_18
} qm_adc_channel_t;

/**
 * ADC configuration type.
 */
typedef struct
{
	/* Sample interval in ADC clock cycles, defines the period to wait
	 * between the start of each sample and can be in the range
	 * [(resolution+2)-255]. */
	uint8_t window;
	qm_adc_resolution_t resolution; /* 12, 10, 8, 6-bit resolution */
} qm_adc_config_t;

/**
 * ADC xfer type.
 */
typedef struct
{
	qm_adc_channel_t *ch; /* Channel sequence array (1-32 channels) */
	uint32_t ch_len;      /* Number of channels in the above array */
	uint32_t *samples;    /* Array to store samples */
	uint32_t samples_len; /* Length of sample array */
	void (*complete_callback)(void); /* User callback for interrupt mode */
	void (*error_callback)(void);    /* User callback for error condition */
} qm_adc_xfer_t;

/**
 * ADC Interrupt Service Routine
 */
void qm_adc_0_isr(void);

/**
 * Switch operating mode of ADC.
 *
 * @param [in] adc Which ADC to enable.
 * @param [in] mode ADC operating mode.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_adc_set_mode(const qm_adc_t adc, const qm_adc_mode_t mode);

/**
 * Calibrate the ADC. It is necessary to calibrate if it is intended to use
 * Normal Mode With Calibration. The calibration must be performed if the ADC
 * is used for the first time or has been in in deep power down mode. This
 * call is blocking.
 *
 * @param [in] adc Which ADC to calibrate.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_adc_calibrate(const qm_adc_t adc);

/**
 * Set ADC configuration. This sets the sample window and resolution.
 *
 * @brief Set ADC configuration.
 * @param [in] adc Which ADC to configure.
 * @param [in] cfg ADC configuration.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_adc_set_config(const qm_adc_t adc, const qm_adc_config_t *const cfg);

/**
 * Retrieve ADC configuration. This gets the sample window and resolution.
 *
 * @param [in] adc Which ADC to read the configuration of.
 * @param [out] cfg ADC configuration.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_adc_get_config(const qm_adc_t adc, qm_adc_config_t *const cfg);

/**
 * Convert values from the ADC. This blocking call can read 1-32 ADC values
 * into the array provided.
 *
 * @brief Poll based ADC convert
 * @param [in] adc Which ADC to read.
 * @param [in] xfer Channel and sample info.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_adc_convert(const qm_adc_t adc, qm_adc_xfer_t *xfer);

/**
 * Read current value from ADC channel. Convert values from the ADC, this is a
 * non-blocking call and will call the user provided callback after the
 * requested number of samples have been converted.
 *
 * @brief IRQ based ADC convert
 * @param [in] adc Which ADC to read.
 * @param [in] xfer Channel, sample and callback info.
 * @return qm_rc_t QM_RC_OK on success, error code otherwise.
 */
qm_rc_t qm_adc_irq_convert(const qm_adc_t adc, qm_adc_xfer_t *xfer);

/**
 * @}
 */
#endif /* QUARK_D2000 */

#endif /* __QM_ADC_H__ */
