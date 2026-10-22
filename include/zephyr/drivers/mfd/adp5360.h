/*
 * Copyright (c) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_ADP5360_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_ADP5360_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mfd_interface_adp5360 MFD ADP5360 Interface
 * @ingroup mfd_interfaces
 * @since 4.5
 * @version 0.1.0
 * @{
 */

#include <zephyr/device.h>

/**
 * @brief Child device of adp5360
 */
enum child_dev {
	ADP5360_DEV_REG = 0, /**< Regulator */
	ADP5360_DEV_CHG,     /**< Charger */
	ADP5360_DEV_FG,      /**< Fuelgauge */
	ADP5360_DEV_MAX,     /**< Maximum number of child devices */
};

/**
 * @brief ADP5360 interrupt types
 */
enum adp5360_interrupt_type {
	ADP5360_INTERRUPT_VBUS_VOLTAGE_THRESHOLD = 0, /**< VBUS voltage threshold interrupt */
	ADP5360_INTERRUPT_CHARGER_MODE_CHANGE,        /**< Charger mode change interrupt */
	ADP5360_INTERRUPT_BAT_VOLTAGE_THRESHOLD,      /**< Battery voltage threshold interrupt */
	ADP5360_INTERRUPT_TEMP_THRESHOLD,             /**< Temperature threshold interrupt */
	ADP5360_INTERRUPT_BAT_PROTECTION,             /**< Battery protection interrupt */
	ADP5360_INTERRUPT_ADPICHG,                    /**< Adaptive charge interrupt */
	ADP5360_INTERRUPT_SOC_ACM,                    /**< State of charge ACM interrupt */
	ADP5360_INTERRUPT_SOC_LOW,                    /**< Low state of charge interrupt */
	ADP5360_INTERRUPT_BUCKBSTPGOOD,               /**< Buck boost power good interrupt */
	ADP5360_INTERRUPT_BUCKPGOOD,                  /**< Buck power good interrupt */
	ADP5360_INTERRUPT_WATCHDOG_TIMEOUT,           /**< Watchdog timeout interrupt */
	ADP5360_INTERRUPT_MANUAL_RESET,               /**< Manual reset interrupt */
};

/**
 * @brief Power good pin selection
 */
enum pgood_pin {
	ADP5360_PGOOD1 = 1, /**< Power good pin 1 */
	ADP5360_PGOOD2,     /**< Power good pin 2 */
};

/**
 * @brief Power good status types
 */
enum adp5360_pgood_status_type {
	ADP5360_PGOOD_STATUS_CHARGE_COMPLETE = 0, /**< Charge complete status */
	ADP5360_PGOOD_STATUS_VBUS_OK,             /**< VBUS OK status */
	ADP5360_PGOOD_STATUS_BATTERY_OK,          /**< Battery OK status */
	ADP5360_PGOOD_STATUS_VOUT2_OK,            /**< VOUT2 OK status */
	ADP5360_PGOOD_STATUS_VOUT1_OK,            /**< VOUT1 OK status */
};

/**
 * @brief Interrupt callback function type for ADP5360 MFD interrupts
 *
 * @param dev Pointer to the device structure for the MFD instance
 * @param user_data User data provided when registering the callback
 */
typedef void (*mfd_adp5360_interrupt_callback)(const struct device *dev, void *user_data);

/**
 * @brief Child device interrupt callback structure
 */
struct child_interrupt_callback {
	mfd_adp5360_interrupt_callback handler; /**< Interrupt handler callback */
	void *user_data;                        /**< User data for the callback */
};

/**
 * @brief Perform software reset of the ADP5360 device
 *
 * @param dev Pointer to the device structure for the MFD instance
 *
 * @retval 0 on success
 * @retval -errno negative errno code on failure
 */
int mfd_adp5360_software_reset(const struct device *dev);

/**
 * @brief Perform hardware reset of the ADP5360 device
 *
 * @param dev Pointer to the device structure for the MFD instance
 *
 * @retval 0 on success
 * @retval -errno negative errno code on failure
 */
int mfd_adp5360_hardware_reset(const struct device *dev);

/**
 * @brief Register a callback for reset interrupt events
 *
 * @param dev Pointer to the device structure for the MFD instance
 * @param handler Callback function to be invoked on reset interrupt
 * @param user_data User data to be passed to the callback
 *
 * @retval 0 on success
 * @retval -errno negative errno code on failure
 */
int mfd_adp5360_reset_trigger_set(const struct device *dev, mfd_adp5360_interrupt_callback handler,
				  void *user_data);

/**
 * @brief Register a callback for power good (PGOOD) pin status changes
 *
 * @param dev Pointer to the device structure for the MFD instance
 * @param pin PGOOD pin to monitor (PGOOD1 or PGOOD2)
 * @param type Type of PGOOD status change to monitor
 * @param handler Callback function to be invoked on PGOOD status change
 * @param user_data User data to be passed to the callback
 *
 * @retval 0 on success
 * @retval -errno negative errno code on failure
 */
int mfd_adp5360_pgood_trigger_set(const struct device *dev, enum pgood_pin pin,
				  enum adp5360_pgood_status_type type,
				  mfd_adp5360_interrupt_callback handler, void *user_data);

/**
 * @brief Register a callback for a specific interrupt type
 *
 * @param dev Pointer to the device structure for the MFD instance
 * @param type Type of interrupt to monitor
 * @param handler Callback function to be invoked on interrupt
 * @param user_data User data to be passed to the callback
 *
 * @retval 0 on success
 * @retval -errno negative errno code on failure
 */
int mfd_adp5360_interrupt_trigger_set(const struct device *dev, enum adp5360_interrupt_type type,
			    mfd_adp5360_interrupt_callback handler, void *user_data);

/**
 * @brief Enable shipment mode on the ADP5360 device
 *
 * In shipment mode, the device enters a low-power state to preserve
 * battery charge during storage and shipping.
 *
 * @param dev Pointer to the device structure for the MFD instance
 *
 * @retval 0 on success
 * @retval -errno negative errno code on failure
 */
int mfd_adp5360_shipment_mode_enable(const struct device *dev);

/**
 * @brief Disable shipment mode on the ADP5360 device
 *
 * @param dev Pointer to the device structure for the MFD instance
 *
 * @retval 0 on success
 * @retval -errno negative errno code on failure
 */
int mfd_adp5360_shipment_mode_disable(const struct device *dev);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_ADP5360_H_ */
