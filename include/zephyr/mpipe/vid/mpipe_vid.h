/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Video plugin.
 * @ingroup mpipe_vid
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_H_
#define ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_H_

/**
 * @defgroup mpipe_vid Video
 * @ingroup mpipe_plugins
 * @brief Elements backed by Zephyr video devices, and the software fallbacks.
 *
 * The video plugin sits on Zephyr's video API. It covers capture from a camera,
 * memory-to-memory transforms performed by hardware, and a software converter
 * for the pixel-format changes no hardware on the board can do.
 *
 * Video is where zero-copy matters most, so the pools here hand out the
 * driver's own buffers rather than copies of them, and the shared video object
 * translates between what a driver reports and what a capability says. The two
 * spellings do not match exactly - the video API states a single supported size
 * as a degenerate range, mpipe as a fixed value - and that translation lives in
 * one place so the round trip stays exact.
 */

#endif /* ZEPHYR_INCLUDE_MPIPE_VID_MPIPE_VID_H_ */
