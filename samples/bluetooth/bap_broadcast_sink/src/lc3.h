/**
 * @file
 * @brief Bluetooth BAP Broadcast Sink LC3 header
 *
 * This files handles all the LC3 related functionality for the sample
 *
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SAMPLE_BAP_BROADCAST_SINK_LC3_H
#define SAMPLE_BAP_BROADCAST_SINK_LC3_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/iso.h>
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

#include "stream_rx.h"

#define LC3_MAX_SAMPLE_RATE_HZ     48000U
#define LC3_MAX_FRAME_DURATION_US  10000U
#define LC3_MAX_NUM_SAMPLES_MONO                                                                   \
	((LC3_MAX_FRAME_DURATION_US * LC3_MAX_SAMPLE_RATE_HZ) / USEC_PER_SEC)
#define LC3_MAX_NUM_SAMPLES_STEREO (LC3_MAX_NUM_SAMPLES_MONO * 2U)

/**
 * @brief Enables LC3 for a stream
 *
 * This will initialize the LC3 decoder given the @p stream codec configuration
 *
 * @param stream The stream to enable LC3 for

 * @retval 0 Success
 * @retval -EINVAL The stream is not LC3 codec configured or the codec configuration is invalid
 */
int lc3_enable(struct stream_rx *stream);

/**
 * @brief Disabled LC3 for a stream
 *
 * @param stream The stream to disable LC3 for

 * @retval 0 Success
 * @retval -EINVAL The stream is LC3 initialized
 */
int lc3_disable(struct stream_rx *stream);

/**
 * @brief Enqueue an SDU for decoding
 *
 * @param stream The stream that received the SDU
 * @param info Information about the SDU
 * @param buf The buffer of the SDU
 */
void lc3_enqueue_for_decoding(struct stream_rx *stream, const struct bt_iso_recv_info *info,
			      struct net_buf *buf);

/**
 * @brief Initialize the LC3 module
 *
 * This will start the thread if not already initialized
 *
 * @retval 0 Success
 * @retval -EALREADY Already initialized
 */
int lc3_init(void);
#endif /* SAMPLE_BAP_BROADCAST_SINK_LC3_H */
