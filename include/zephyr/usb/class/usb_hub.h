/*
 * Copyright (c) 2022 Emerson Electric Co.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Hub Class device API header
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_HUB_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_HUB_H_

/**
 * @name USB Hub Class Feature Selectors defined in specification, Table 11-17.
 * @{
 */
#define USB_HCFS_C_HUB_LOCAL_POWER	0x00    /**< Local Power Source */
#define USB_HCFS_C_HUB_OVER_CURRENT	0x01    /**< Over-current */
#define USB_HCFS_PORT_CONNECTION	0x00    /**< Current Connect Status */
#define USB_HCFS_PORT_ENABLE		0x01    /**< Port Enabled/Disabled */
#define USB_HCFS_PORT_SUSPEND		0x02    /**< Suspended */
#define USB_HCFS_PORT_OVER_CURRENT	0x03    /**< Over-current */
#define USB_HCFS_PORT_RESET		0x04    /**< Reset */
#define USB_HCFS_PORT_POWER		0x08    /**< Port Power */
#define USB_HCFS_PORT_LOW_SPEED		0x09    /**< Low-speed device attached */
#define USB_HCFS_C_PORT_CONNECTION	0x10    /**< Connect Status Change */
#define USB_HCFS_C_PORT_ENABLE		0x11    /**< Port Enable/Disable Change */
#define USB_HCFS_C_PORT_SUSPEND		0x12    /**< Suspended Change */
#define USB_HCFS_C_PORT_OVER_CURRENT	0x13    /**< Over-current Indicator Change */
#define USB_HCFS_C_PORT_RESET		0x14    /**< Reset Change */
#define USB_HCFS_PORT_TEST		0x15    /**< Port Test Mode */
#define USB_HCFS_PORT_INDICATOR		0x16    /**< Port Indicator Control */
/** @} */

/**
 * @name USB Hub Class Request Codes defined in specification, Table 11-16.
 * @{
 */
#define USB_HCREQ_GET_STATUS		0x00    /**< Get Hub Status */
#define USB_HCREQ_CLEAR_FEATURE		0x01    /**< Clear Hub */
#define USB_HCREQ_SET_FEATURE		0x03    /**< Set Hub Feature */
#define USB_HCREQ_GET_DESCRIPTOR	0x06    /**< Get Hub Descriptor */
#define USB_HCREQ_SET_DESCRIPTOR	0x07    /**< Set Hub Descriptor */
#define USB_HCREQ_CLEAR_TT_BUFFER	0x08    /**< Clear Transaction Translator (TT) Buffer */
#define USB_HCREQ_RESET_TT		0x09    /**< Reset TT */
#define USB_HCREQ_GET_TT_STATE		0x0A    /**< Get TT State */
#define USB_HCREQ_STOP_TT		0x0B    /**< Stop TT */
/** @} */

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_HUB_H_ */
