/*
 * Copyright (c) 2024 Titouan Christophe
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBD_MIDI_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBD_MIDI_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief USB-MIDI 2.0 class device API
 * @defgroup usb_midi USB MIDI 2.0 Class device API
 * @ingroup usb
 * @since 4.1
 * @version 0.1.0
 * @see midi20: "Universal Serial Bus Device Class Definition for MIDI Devices"
 *              Document Release 2.0 (May 5, 2020)
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/audio/midi.h>

/** @brief Runtime status of a USB-MIDI2 interface */
enum usbd_midi_status {
	/** The interface is not enabled by the host */
	USBD_MIDI_DISABLED = 0,
	/** The interface is enabled in USB-MIDI2.0 mode by the host */
	USBD_MIDI_2_ENABLED,
};

/**
 * @brief      USB-MIDI application event handlers
 */
struct usbd_midi_ops {
	/**
	 * @brief Callback type for incoming Universal MIDI Packets from host
	 * @param[in]  dev   The USB-MIDI interface receiving the packet
	 * @param[in]  ump   The received packet in Universal MIDI Packet format
	 */
	void (*rx_packet_cb)(const struct device *dev, const struct midi_ump ump);

	/**
	 * @brief Callback type for USB-MIDI interface runtime status change
	 * @param[in]  dev   The USB-MIDI interface
	 * @param[in]  st    The current status of the interface
	 */
	void (*status_change_cb)(const struct device *dev, enum usbd_midi_status st);
};

/**
 * @brief      Send a Universal MIDI Packet to the host
 * @param[in]  dev   The USB-MIDI interface
 * @param[in]  ump   The packet to send, in Universal MIDI Packet format
 * @return     0 on success
 *             -EIO if MIDI2.0 is not enabled by the host
 *             -EAGAIN if there isn't room in the transmission buffer
 */
int usbd_midi_send(const struct device *dev, const struct midi_ump ump);

/**
 * @brief Set the application event handlers on a USB-MIDI interface
 * @param[in]  dev   The USB-MIDI interface
 * @param[in]  ops   The event handlers. Pass NULL to reset all callbacks
 */
void usbd_midi_set_ops(const struct device *dev, const struct usbd_midi_ops *ops);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
