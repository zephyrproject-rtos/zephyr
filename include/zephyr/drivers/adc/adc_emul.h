/**
 * @file
 *
 * @brief Backend API for emulated ADC
 */

/*
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_ADC_ADC_EMUL_H_
#define ZEPHYR_INCLUDE_DRIVERS_ADC_ADC_EMUL_H_

#include <zephyr/types.h>
#include <zephyr/drivers/adc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Emulated ADC backend API
 * @defgroup adc_emul Emulated ADC
 * @ingroup adc_interface
 * @{
 *
 * Behaviour of emulated ADC is application-defined. As-such, each
 * application may
 *
 * - define a Device Tree overlay file to indicate the number of ADC
 *   controllers as well as the number of channels for each controller
 * - set default reference voltages in Device Tree or using
 *   @ref adc_emul_ref_voltage_set
 * - asynchronously call @ref adc_emul_const_value_set in order to set
 *   constant mV value on emulated ADC input
 * - asynchronously call @ref adc_emul_value_func_set in order to assign
 *   function which will be used to obtain voltage on emulated ADC input
 *
 * An example of an appropriate Device Tree overlay file is in
 * tests/drivers/adc/adc_api/boards/native_posix.overlay
 *
 * An example of using emulated ADC backend API is in the file
 * tests/drivers/adc/adc_emul/src/main.c
 */

/**
 * @brief Type definition of the function which is used to obtain ADC
 *        mV input values
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param chan ADC channel to sample
 * @param data User data which was passed on @ref adc_emul_value_func_set
 * @param result The result value which will be set as input for ADC @p chan
 *
 * @return 0 on success
 * @return other as error code which ends ADC context
 */
typedef int (*adc_emul_value_func)(const struct device *dev, unsigned int chan,
				   void *data, uint32_t *result);

/**
 * @brief Set constant mV value input for emulated ADC @p chan
 *
 * @param dev The emulated ADC device
 * @param chan The channel of ADC which input is assigned
 * @param value New voltage in mV to assign to @p chan input
 *
 * @return 0 on success
 * @return -EINVAL if an invalid argument is provided
 */
int adc_emul_const_value_set(const struct device *dev, unsigned int chan,
			     uint32_t value);

/**
 * @brief Set function used to obtain voltage for input of emulated
 *        ADC @p chan
 *
 * @param dev The emulated ADC device
 * @param chan The channel of ADC to which @p func is assigned
 * @param func New function to assign to @p chan
 * @param data Pointer to data passed to @p func on call
 *
 * @return 0 on success
 * @return -EINVAL if an invalid argument is provided
 */
int adc_emul_value_func_set(const struct device *dev, unsigned int chan,
			    adc_emul_value_func func, void *data);

/**
 * @brief Set reference voltage
 *
 * @param dev The emulated ADC device
 * @param ref Reference config which is changed
 * @param value New reference voltage in mV
 *
 * @return 0 on success
 * @return -EINVAL if an invalid argument is provided
 */
int adc_emul_ref_voltage_set(const struct device *dev, enum adc_reference ref,
			     uint16_t value);
/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_ADC_ADC_EMUL_H_ */
