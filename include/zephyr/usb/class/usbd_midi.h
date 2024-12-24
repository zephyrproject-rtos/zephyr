/*
 * Copyright (c) 2024 Titouan Christophe
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USB_CLASS_USBD_MIDI_H_
#define ZEPHYR_INCLUDE_USB_CLASS_USBD_MIDI_H_

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

/**
 * @brief      Send a Universal MIDI Packet to the host
 * @param[in]  dev   The USB-MIDI interface
 * @param[in]  ump   The packet to send, in Universal MIDI Packet format
 * @return     0 on success
 *             -EIO if MIDI2.0 is not enabled by the host
 *             -EAGAIN if there isn't room in the transmission buffer
 */
int usbd_midi_send(const struct device *dev, const uint32_t ump[static 4]);

/**
 * @brief Callback type for incoming Universal MIDI Packets from host
 * @param[in]  dev   The USB-MIDI interface receiving the packet
 * @param[in]  ump   The received packet in Universal MIDI Packet format
 */
typedef void (*usbd_midi_callback)(const struct device *dev, const uint32_t ump[static 4]);

/**
 * @brief Set a user callback to invoke when receiving a packet from host
 * @param[in]  dev   The USB-MIDI interface
 * @param[in]  cb    The function to call
 */
void usbd_midi_set_callback(const struct device *dev, usbd_midi_callback cb);

/**
 * @}
 */

#endif
