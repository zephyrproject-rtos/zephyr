/**
 * @file
 * @brief Bluetooth BAP Broadcast Sink sample USB header
 *
 * This files handles all the USB related functionality for the sample
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SAMPLE_BAP_BROADCAST_SINK_USB_H
#define SAMPLE_BAP_BROADCAST_SINK_USB_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys_clock.h>
#include <zephyr/toolchain.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_audio.h>

#define USB_SAMPLE_RATE_HZ 48000U

/**
 * @brief Add decoded frame to the USB buffer
 *
 * @param chan_allocation The channel of the frame (@ref BT_AUDIO_LOCATION_FRONT_LEFT, @ref
 *                        BT_AUDIO_LOCATION_FRONT_RIGHT or @ref BT_AUDIO_LOCATION_MONO_AUDIO)
 * @param frame The frame
 * @param frame_size The size of @p in octets
 * @param ts The timestamp of the frame
 *
 * @retval 0 Success
 * @retval -EINVAL Invalid channel, frame of framesize
 * @retval -ENOEXEC Old timestamp, discarded
 * @retval -ENOMEM No memory to enqueue
 */
int usb_add_frame_to_usb(enum bt_audio_location chan_allocation, const int16_t *frame,
			 size_t frame_size, uint32_t ts);

/**
 * @brief Clear last sent SDU
 *
 * If only part of the SDU could be decoded, this should be called
 */
void usb_clear_frames_to_usb(void);

/**
 * @brief Initialize the USB module
 *
 * This will start the USB thread if not already initialized
 *
 * @retval 0 Success
 * @retval -EALREADY Already initialized
 */
int usb_init(void);

#endif /* SAMPLE_BAP_BROADCAST_SINK_USB_H */
