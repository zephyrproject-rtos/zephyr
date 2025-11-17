/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USB_SERIAL_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USB_SERIAL_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#define RX_RINGBUF_SIZE		CONFIG_USBH_SERIAL_RX_BUF_SIZE

struct usb_serial_api {
	int (*write)(const struct device *dev, const uint8_t *buf, size_t len);
	int (*read)(const struct device *dev, uint8_t *buf, size_t max_len);
	int (*set_rx_cb)(const struct device *dev, void (*cb)(void *user_data), void *user_data);
	int (*set_tx_cb)(const struct device *dev, void (*cb)(void *user_data), void *user_data);
};

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

	/* RX data buffering */
	struct ring_buf ringbuf;
	uint8_t rx_buf[RX_RINGBUF_SIZE];

	struct k_mutex port_mutex;
	struct k_sem sync_sem;
	bool in_use;

	void (*rx_notify_cb)(void *user_data);
	void *rx_notify_user_data;
	void (*tx_notify_cb)(void *user_data);
	void *tx_notify_user_data;
};

static inline int usb_serial_write(const struct device *dev,
				   const uint8_t *buf, size_t len)
{
	const struct usb_serial_api *api = dev->api;
	return api->write(dev, buf, len);
}

static inline int usb_serial_read(const struct device *dev, uint8_t *buf,
				  size_t max_len)
{
	const struct usb_serial_api *api = dev->api;
	return api->read(dev, buf, max_len);
}

static inline int usb_serial_set_rx_cb(const struct device *dev, void (*cb)(void *user_data),
				       void *user_data)
{	const struct usb_serial_api *api = dev->api;

	return api->set_rx_cb(dev, cb, user_data);
}

static inline int usb_serial_set_tx_cb(const struct device *dev, void (*cb)(void *user_data),
				       void *user_data)
{	const struct usb_serial_api *api = dev->api;

	return api->set_tx_cb(dev, cb, user_data);
}

#endif /* ZEPHYR_INCLUDE_USB_CLASS_USB_SERIAL_H_ */
