/*
 * Copyright (c) 2026 Tolt Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB CDC ECM and CDC NCM device class public header
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBD_CDC_ETH_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBD_CDC_ETH_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/net/ethernet.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB CDC ECM and CDC NCM device class API
 * @defgroup usbd_cdc_eth USB CDC ECM and CDC NCM device class API
 * @ingroup usb
 * @since 4.6
 * @version 0.1.0
 * @{
 */

/**
 * @brief Set the remote MAC address of a CDC ECM instance
 *
 * The remote MAC address is the address of the host side of the link, reported
 * to the host in the iMACAddress string descriptor. It replaces the
 * remote-mac-address devicetree property and must be set before usbd_init().
 *
 * @param[in] dev CDC ECM Ethernet device
 * @param[in] mac Remote MAC address
 *
 * @retval 0 on success
 * @retval -EINVAL the address is multicast or all zeros
 * @retval -EBUSY the USB device support is initialized
 */
int usbd_cdc_ecm_set_remote_mac(const struct device *dev, const uint8_t mac[NET_ETH_ADDR_LEN]);

/**
 * @brief Set the remote MAC address of a CDC NCM instance
 *
 * The remote MAC address is the address of the host side of the link, reported
 * to the host in the iMACAddress string descriptor. It replaces the
 * remote-mac-address devicetree property and must be set before usbd_init().
 *
 * @param[in] dev CDC NCM Ethernet device
 * @param[in] mac Remote MAC address
 *
 * @retval 0 on success
 * @retval -EINVAL the address is multicast or all zeros
 * @retval -EBUSY the USB device support is initialized
 */
int usbd_cdc_ncm_set_remote_mac(const struct device *dev, const uint8_t mac[NET_ETH_ADDR_LEN]);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USBD_CDC_ETH_H_ */
