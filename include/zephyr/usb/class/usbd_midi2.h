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
 * @brief USB MIDI 2.0 class device API
 * @defgroup usbd_midi2 USB MIDI 2.0 Class device API
 * @ingroup usb
 * @since 4.1
 * @version 0.1.0
 * @see midi20: "Universal Serial Bus Device Class Definition for MIDI Devices"
 *              Document Release 2.0 (May 5, 2020)
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/audio/midi.h>

/**
 * @brief      MIDI2 application event handlers
 */
struct usbd_midi_ops {
	/**
	 * @brief Callback type for incoming Universal MIDI Packets from host
	 * @param[in]  dev   The MIDI2 device receiving the packet
	 * @param[in]  ump   The received packet in Universal MIDI Packet format
	 */
	void (*rx_packet_cb)(const struct device *dev, const struct midi_ump ump);

	/**
	 * @brief Callback type for MIDI2 interface runtime status change
	 * @param[in]  dev    The MIDI2 device
	 * @param[in]  ready  True if the interface is enabled by the host
	 */
	void (*ready_cb)(const struct device *dev, const bool ready);
};

/**
 * @brief      Send a Universal MIDI Packet to the host
 * @param[in]  dev   The MIDI2 device
 * @param[in]  ump   The packet to send, in Universal MIDI Packet format
 * @return     0 on success, all other values should be treated as error
 *             -EIO if USB MIDI 2.0 is not enabled by the host
 *             -ENOBUFS if there is no space in the TX buffer
 */
int usbd_midi_send(const struct device *dev, const struct midi_ump ump);

/**
 * @brief Set the application event handlers on a USB MIDI device
 * @param[in]  dev   The MIDI2 device
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
