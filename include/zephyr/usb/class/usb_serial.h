/*
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_SERIAL_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_SERIAL_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>


#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/uart/uart_internal.h>

#define CURRENT_BUFFER	0
#define NEXT_BUFFER	1

/**
 * Device-specific quirks table
 * Maps VID/PID to interface numbers for AT command ports
 */
struct usb_serial_quirk {
	uint16_t vid;
	uint16_t pid;
	uint8_t at_iface;      /* Interface number for AT commands */
	const char *desc;
};

/**
 * @brief USB Serial Port structure
 *
 * Thread-safe structure for managing USB serial port state
 */
struct usb_serial_port {
	/* usb device information */
	struct usb_device *udev;
	uint8_t iface_num;
	uint8_t bulk_in_ep;
	uint8_t bulk_out_ep;
	uint16_t bulk_in_mps;
	uint16_t bulk_out_mps;

	bool in_use;
	struct k_mutex lock;
#if defined(CONFIG_UART_ASYNC_API)
	const struct device *uart_dev;
	uart_callback_t uart_cb;
	void *uart_cb_user_data;
	uint8_t *cur_buf;
	size_t cur_len;
	size_t cur_off;

	const uint8_t *tx_buf;
	size_t tx_len;

	uint8_t *next_buf;
	size_t next_len;
#endif
};

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_SERIAL_H_ */
