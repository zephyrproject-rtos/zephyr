/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ADC_NPCX_THRESHOLD_H_
#define _ADC_NPCX_THRESHOLD_H_

#include <zephyr/device.h>

enum adc_npcx_threshold_param_l_h {
	ADC_NPCX_THRESHOLD_PARAM_L_H_HIGHER,
	ADC_NPCX_THRESHOLD_PARAM_L_H_LOWER,
};

enum adc_npcx_threshold_param_type {
	/* Selects ADC channel to be used for measurement */
	ADC_NPCX_THRESHOLD_PARAM_CHNSEL,
	/* Sets relation between measured value and assetion threshold value.*/
	ADC_NPCX_THRESHOLD_PARAM_L_H,
	/* Sets the threshold value to which measured data is compared. */
	ADC_NPCX_THRESHOLD_PARAM_THVAL,
	/* Sets worker queue thread to be notified */
	ADC_NPCX_THRESHOLD_PARAM_WORK,

	ADC_NPCX_THRESHOLD_PARAM_MAX,
};

struct adc_npcx_threshold_param {
	/* Threshold ocntrol parameter */
	enum adc_npcx_threshold_param_type type;
	/* Parameter value */
	uint32_t val;
};

/**
 * @brief Convert input value in millivolts to corresponding threshold register
 * value.
 *
 * @note This function is available only if @kconfig{CONFIG_ADC_CMP_NPCX}
 * is selected.
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param val_mv    Input value in millivolts to be converted.
 * @param thrval    Pointer of variable to hold the result of conversion.
 *
 * @returns 0 on success, negative result if input cannot be converted due to
 *          overflow.
 */
int adc_npcx_threshold_mv_to_thrval(const struct device *dev, uint32_t val_mv,
								uint32_t *thrval);

/**
 * @brief Set ADC threshold parameter.
 *
 * @note This function is available only if @kconfig{CONFIG_ADC_CMP_NPCX}
 * is selected.
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param th_sel    Threshold selected.
 * @param param     Pointer of parameter structure.
 *                  See struct adc_npcx_threshold_param for supported
 *                  parameters.
 *
 * @returns 0 on success, negative error code otherwise.
 */
int adc_npcx_threshold_ctrl_set_param(const struct device *dev,
				      const uint8_t th_sel,
				      const struct adc_npcx_threshold_param
				      *param);

/**
 * @brief Enables/Disables ADC threshold interruption.
 *
 * @note This function is available only if @kconfig{CONFIG_ADC_CMP_NPCX}
 * is selected.
 *
 * @param dev       Pointer to the device structure for the driver instance.
 * @param th_sel    Threshold selected.
 * @param enable    Enable or disables threshold interruption.
 *
 * @returns 0 on success, negative error code otherwise.
 *            all parameters must be configure prior enabling threshold
 *            interruption, otherwise error will be returned.
 */
int adc_npcx_threshold_ctrl_enable(const struct device *dev, uint8_t th_sel,
				   const bool enable);

#endif /*_ADC_NPCX_THRESHOLD_H_ */
