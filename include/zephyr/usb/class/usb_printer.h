/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Printer Class Device API
 *
 * Header follows the USB Printer Class Specification v1.1
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_PRINTER_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_PRINTER_H_

#include <zephyr/usb/usb_device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB Printer Class codes
 */
#define USB_PRINTER_CLASS            0x07  /* Printer Device Class */
#define USB_PRINTER_SUBCLASS        0x01  /* Printer Device Subclass */
#define USB_PRINTER_PROTOCOL_UNI    0x01  /* Unidirectional Interface */
#define USB_PRINTER_PROTOCOL_BI     0x02  /* Bidirectional Interface */
#define USB_PRINTER_PROTOCOL_1284   0x03  /* IEEE 1284.4 Compatible Interface */

/**
 * @brief USB Printer Class specific requests
 */
#define USB_PRINTER_GET_DEVICE_ID   0x00  /* Get IEEE 1284 Device ID string */
#define USB_PRINTER_GET_PORT_STATUS 0x01  /* Get current printer status */
#define USB_PRINTER_SOFT_RESET      0x02  /* Resets printer without changing configs */

/**
 * @brief USB Printer Port Status bits
 */
#define USB_PRINTER_STATUS_ERROR    BIT(3) /* Paper empty, offline, etc */
#define USB_PRINTER_STATUS_SELECTED BIT(4) /* Printer selected */
#define USB_PRINTER_STATUS_PAPER    BIT(5) /* Paper empty */

/**
 * @brief USB Printer data received callback
 *
 * @param data Pointer to received data
 * @param len Length of received data
 */
typedef void (*printer_data_received_cb)(const uint8_t *data, size_t len);

/**
 * @brief USB Printer configuration options
 */
struct usb_printer_config {
    /** USB device status callback */
    void (*status_cb)(bool configured);
    /** Callback for printer class requests */
    int (*class_handler)(struct usb_setup_packet *setup, int32_t *len, uint8_t **data);
    /** Device ID string (IEEE 1284) */
    const char *device_id;
    /** Data received callback */
    printer_data_received_cb data_received;
};

/**
 * @brief Initialize USB printer device
 *
 * @param[in] config Pointer to USB printer config structure
 *
 * @return 0 if successful, negative errno code on failure
 */
int usb_printer_init(const struct usb_printer_config *config);

/**
 * @brief Update printer status flags
 *
 * @param[in] status New printer status flags
 *
 * @return 0 if successful, negative errno code on failure
 */
int usb_printer_update_status(uint8_t status);

/**
 * @brief Send data back to the host
 *
 * @param[in] data Pointer to data buffer
 * @param[in] len Length of data to send
 *
 * @return Number of bytes sent if successful, negative errno code on failure
 */
int usb_printer_send_data(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_PRINTER_H_ */