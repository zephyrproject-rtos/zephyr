/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_COMMON_NORDIC_NRF_USBHS_WRAPPER_H_
#define ZEPHYR_DRIVERS_USB_COMMON_NORDIC_NRF_USBHS_WRAPPER_H_

#include <zephyr/device.h>

#define NRF_USBHS_VREG_READY	BIT(0)
#define NRF_USBHS_PHY_READY	BIT(1)

/*
 * Nordic USBHS wrapper is a set of registers used by both USB BC12 driver and
 * vendor specific part in UDC driver.  The wrapper can be considered to be MFD
 * device. Both drivers depends on the VBUS regulator driver and require some
 * synchronisation when accessing wrapper register.
 *
 * The device argument in all the functions below is the wrapper device, what
 * is always a parent of the BC12 and UDC drivers.
 */

/*
 * Set the event handler callback for the BC12 device driver. The callback will
 * be called in the regulator driver ISR context, and must not be blocking.
 */
void nrf_usbhs_wrapper_set_bc12_cb(const struct device *dev,
				   regulator_callback_t cb,
				   const void *const user_data);

/*
 * Set the event handler callback for the UDC device driver. The callback will
 * be called in the regulator driver ISR context, and must not be blocking.
 */
void nrf_usbhs_wrapper_set_udc_cb(const struct device *dev,
				  regulator_callback_t cb,
				  const void *const user_data);

/*
 * Enable VBUS regulator. Can be called from both drivers. Regulator has own
 * reference counting.
 */
int nrf_usbhs_wrapper_vreg_enable(const struct device *dev);

/*
 * Disable VBUS regulator. Can be called from both drivers. Regulator has own
 * reference counting.
 */
int nrf_usbhs_wrapper_vreg_disable(const struct device *dev);

/*
 * Post the event that the BC12 driver is finished, and PHY may be used by the
 * UDC driver. Set the PHY to the default state, and if necessary, enable the
 * controller after the BC12 detection process is finished.
 */
void nrf_usbhs_wrapper_bc12_finished(const struct device *dev);

/*
 * Disable the PHY and controller. This function can be called from UDC and
 * BC12 drivers and has own reference counter.
 */
void nrf_usbhs_wrapper_disable(const struct device *dev);

/*
 * Enable PHY and controller. This function should be called from the UDC
 * driver. It increments internal referece counter.
 */
void nrf_usbhs_wrapper_enable_udc(const struct device *dev);

/*
 * Enable PHY. This function should be called from the BC12 driver. It
 * increments internal referece counter.
 */
void nrf_usbhs_wrapper_enable_phy(const struct device *dev, const bool suspended);

/* Start USB peripheral */
void nrf_usbhs_wrapper_stop(const struct device *dev);

/* Stop USB peripheral */
void nrf_usbhs_wrapper_start(const struct device *dev);

/*
 * Get the pointer to the event struct where NRF_USBHS_VREG_READY and
 * NRF_USBHS_PHY_READY are posted. NRF_USBHS_PHY_READY signals that PHY can be
 * used by the UDC driver.
 */
struct k_event *nrf_usbhs_wrapper_get_events_ptr(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_USB_COMMON_NORDIC_NRF_USBHS_WRAPPER_H_ */
