/*
 * Copyright (c) 2023 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock subsystem identifiers for Ambiq devices.
 * @ingroup clock_control_ambiq
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_AMBIQ_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_AMBIQ_H_

/**
 * @defgroup clock_control_ambiq Ambiq
 * @ingroup clock_control_interface_ext
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Clocks handled by the CLOCK peripheral.
 *
 * Used as the @c sys argument to the clock_control API.
 */
enum clock_control_ambiq_type {
	CLOCK_CONTROL_AMBIQ_TYPE_HFXTAL_BLE,         /**< HFXTAL for the BLE controller. */
	CLOCK_CONTROL_AMBIQ_TYPE_HFXTAL_USB,         /**< HFXTAL for USB. */
	CLOCK_CONTROL_AMBIQ_TYPE_HFXTAL_ADC,         /**< HFXTAL for the ADC. */
	CLOCK_CONTROL_AMBIQ_TYPE_HFXTAL_AUADC,       /**< HFXTAL for the audio ADC. */
	CLOCK_CONTROL_AMBIQ_TYPE_HCXTAL_DBGCTRL,     /**< HCXTAL for debug control. */
	CLOCK_CONTROL_AMBIQ_TYPE_HCXTAL_CLKGEN_MISC, /**< HCXTAL for clock generator misc. */
	CLOCK_CONTROL_AMBIQ_TYPE_HCXTAL_CLKGEN_CLKOUT, /**< HCXTAL for the clock output. */
	CLOCK_CONTROL_AMBIQ_TYPE_HCXTAL_PDM,         /**< HCXTAL for PDM. */
	CLOCK_CONTROL_AMBIQ_TYPE_HCXTAL_IIS,         /**< HCXTAL for I2S. */
	CLOCK_CONTROL_AMBIQ_TYPE_HCXTAL_IOM,         /**< HCXTAL for the I/O master. */
	CLOCK_CONTROL_AMBIQ_TYPE_LFXTAL,             /**< Low-frequency crystal oscillator. */
	CLOCK_CONTROL_AMBIQ_TYPE_MAX                 /**< Number of clock subsystem types. */
};

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_CLOCK_CONTROL_AMBIQ_H_ */
