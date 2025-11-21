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

#include <stddef.h>
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

#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)
	/**
	 * @brief Callback type for incoming raw MIDI 1.0 messages from host
	 * @note Only available when :kconfig:option:`CONFIG_USBD_MIDI2_ALTSETTING_MIDI1` is
	 * enabled.
	 * @param[in] dev        The MIDI device receiving the packet
	 * @param[in] cable_num  Virtual cable/group number
	 * @param[in] bytes      Pointer to the MIDI status byte followed by data bytes
	 * @param[in] len        Number of valid bytes pointed to by @p bytes (2 or 3)
	 *
	 * Called only when the host selected the MIDI 1.0 alternate setting. When the
	 * MIDI 2.0 alternate is active, @ref rx_packet_cb is used instead.
	 */
	void (*rx_midi1_cb)(const struct device *dev, uint8_t cable_num, const uint8_t *bytes,
			    uint8_t len);
#endif

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
 * @brief Configure which USB-MIDI alternate settings are available
 *
 * Allows applications to expose only the MIDI 1.0, only the MIDI 2.0, or both
 * alternate settings during enumeration. The caller must disable the USB stack
 * before invoking this API so the descriptors can be safely updated.
 *
 * @param dev           MIDI device instance
 * @param enable_midi1  True to expose the MIDI 1.0 alternate setting
 * @param enable_midi2  True to expose the MIDI 2.0 alternate setting
 *
 * @return 0 on success, -EBUSY if the USB device is enabled, or -EINVAL if
 *         both alternates are disabled.
 */
int usbd_midi_set_mode(const struct device *dev, bool enable_midi1, bool enable_midi2);

#if IS_ENABLED(CONFIG_USBD_MIDI2_ALTSETTING_MIDI1)
/**
 * @brief Send a raw MIDI 1.0 message to the host
 *
 * Converts the supplied MIDI 1.0 status/data bytes into a USB-MIDI 1.0 event
 * when the host selected the legacy alternate setting, or into an equivalent
 * Universal MIDI Packet when the host selected the MIDI 2.0 alternate setting.
 * This helper is only available when
 * :kconfig:option:`CONFIG_USBD_MIDI2_ALTSETTING_MIDI1` is enabled.
 *
 * @param dev           MIDI device instance
 * @param cable_number  Virtual cable (group) to send the event on (0-15)
 * @param midi_bytes    Pointer to the MIDI status byte followed by payload bytes
 * @param len           Number of valid bytes in @p midi_bytes (2 or 3 bytes)
 *
 * @return 0 on success, -EIO if the interface is not ready, -ENOBUFS when the
 *         transmit ring is full, or a negative errno code on invalid input.
 *
 * @note Only MIDI 1.0 channel voice messages are supported.
 */
int usbd_midi_send_midi1(const struct device *dev, uint8_t cable_number, const uint8_t *midi_bytes,
			 size_t len);
#endif

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
