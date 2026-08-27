/*
 * SPDX-FileCopyrightText: Copyright Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USB_COMMON_NORDIC_NRF_USBHS_WRAPPER_H_
#define ZEPHYR_DRIVERS_USB_COMMON_NORDIC_NRF_USBHS_WRAPPER_H_

#include <zephyr/device.h>

#define NRF_USBHS_VBUS_READY	BIT(0)

/*
 * Nordic USBHS wrapper is a set of registers used by both USB BC12 driver and
 * vendor specific part in UDC driver.  The wrapper can be considered to be MFD
 * device. Both drivers depends on the VBUS regulator driver and require some
 * synchronisation when accessing wrapper register.
 *
 * The wrapper does not modify the wrapper registers on its own. Changes to the
 * wrapper registers are always performed by the DWC2 driver quirks or BC12
 * driver by calling functions declared in this header.
 *
 * The device argument in all the functions below is the wrapper device, what
 * is always a parent of the BC12 and UDC drivers.
 */


/* This function should be called from BC12 driver when VBUS is detected. */
void nrf_usbhs_wrapper_bc12_vbus_detected(const struct device *dev);

/* This function should be called from BC12 driver when detection is finished. */
void nrf_usbhs_wrapper_bc12_finished(const struct device *dev, const bool dcp);

/* This function should be called from BC12 driver when VBUS is removed. */
void nrf_usbhs_wrapper_bc12_vbus_removed(const struct device *dev);

/*
 * This function should be called from the BC12 driver when the role of the
 * device changes from disconnected to portable device and vice versa.
 */
void nrf_usbhs_wrapper_bc12_pd_enabled(const struct device *dev, const bool enabled);

/* This function should be called from the UDC driver pre enable quirk.  */
int nrf_usbhs_wrapper_udc_pre_enable(const struct device *dev);

/*
 * Allow D+ pull-up to become active. This function should be called from the
 * UDC driver post enable quirk.
 */
void nrf_usbhs_wrapper_udc_post_enable(const struct device *dev);

/* This function should be called from UDC driver disable quirk. */
void nrf_usbhs_wrapper_udc_disable(const struct device *dev);

/* Start USB peripheral */
void nrf_usbhs_wrapper_stop(const struct device *dev);

/* Stop USB peripheral */
void nrf_usbhs_wrapper_start(const struct device *dev);

/*
 * Get the pointer to the event struct where NRF_USBHS_PHY_READY are posted.
 * NRF_USBHS_PHY_READY signals that PHY can be used by the UDC driver.
 */
struct k_event *nrf_usbhs_wrapper_get_events_ptr(const struct device *dev);

/* Enable VBUS regulator. Can be called from both drivers. */
int nrf_usbhs_wrapper_vreg_enable(const struct device *dev);

/* Disable VBUS regulator. Can be called from both drivers. */
int nrf_usbhs_wrapper_vreg_disable(const struct device *dev);

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

#endif /* ZEPHYR_DRIVERS_USB_COMMON_NORDIC_NRF_USBHS_WRAPPER_H_ */
