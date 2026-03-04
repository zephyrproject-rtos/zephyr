/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Vendor-specific API for Infineon AutAnalog MFD driver.
 *
 * The AutAnalog subsystem contains multiple analog peripherals (SAR ADC, DAC,
 * comparators, programmable threshold comparator) coordinated by an Autonomous
 * Controller (AC). This header defines the API used by child peripheral drivers
 * to interact with the shared MFD parent. It can also be used by the application
 * to directly control the Autonomous Controller if needed.
 */

#ifndef SOC_INFINEON_EDGE_COMMON_INFINEON_AUTANALOG_H_
#define SOC_INFINEON_EDGE_COMMON_INFINEON_AUTANALOG_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Peripheral types within the AutAnalog subsystem
 *
 * Used to identify child peripherals when registering ISR handlers.
 */
enum ifx_autanalog_periph {
	IFX_AUTANALOG_PERIPH_SAR_ADC, /**< SAR ADC peripheral */
	IFX_AUTANALOG_PERIPH_DAC0,    /**< DAC0 peripheral */
	IFX_AUTANALOG_PERIPH_DAC1,    /**< DAC1 peripheral */
	IFX_AUTANALOG_PERIPH_CTB0,    /**< CTB0 peripheral */
	IFX_AUTANALOG_PERIPH_CTB1,    /**< CTB1 peripheral */
	IFX_AUTANALOG_PERIPH_PTCOMP0, /**< Programmable Threshold Comparator 0 */
	IFX_AUTANALOG_PERIPH_AC,      /**< Autonomous Controller */
	IFX_AUTANALOG_PERIPH_COUNT,   /**< Number of AutAnalog peripheral types */
};

/** ISR handler type for AutAnalog child devices */
typedef void (*ifx_autanalog_child_isr_t)(const struct device *dev);

/**
 * @brief Register a child peripheral ISR handler with the AutAnalog MFD
 *
 * Registers an interrupt handler for a specific sub-peripheral.  When the shared
 * AutAnalog interrupt fires and the source matches the specified peripheral, the
 * registered handler will be called.  The interrupt mask for the peripheral is
 * automatically enabled when a handler is registered.
 *
 * @param dev AutAnalog MFD device
 * @param child_dev Child device registering the handler
 * @param periph Peripheral type identifier
 * @param handler ISR callback function
 */
void ifx_autanalog_set_irq_handler(const struct device *dev, const struct device *child_dev,
				   enum ifx_autanalog_periph periph,
				   ifx_autanalog_child_isr_t handler);

/**
 * @brief Start the AutAnalog Autonomous Controller
 *
 * Starts the Autonomous Controller state machine.  The AC coordinates the
 * operation of all sub-peripherals according to the configured State Transition
 * Table (STT).
 *
 * @param dev AutAnalog MFD device
 * @return 0 on success, negative error code on failure
 */
int ifx_autanalog_start_autonomous_control(const struct device *dev);

/**
 * @brief Pause the AutAnalog Autonomous Controller
 *
 * Pauses the Autonomous Controller state machine.  This should be called before
 * reconfiguring sub-peripheral sequencer tables or other AC-dependent settings.
 *
 * @param dev AutAnalog MFD device
 * @return 0 on success, negative error code on failure
 */
int ifx_autanalog_pause_autonomous_control(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* SOC_INFINEON_EDGE_COMMON_INFINEON_AUTANALOG_H_ */
