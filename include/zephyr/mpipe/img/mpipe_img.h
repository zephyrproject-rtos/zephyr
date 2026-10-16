/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Image codec plugin.
 * @ingroup mpipe_img
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_IMG_MPIPE_IMG_H_
#define ZEPHYR_INCLUDE_MPIPE_IMG_MPIPE_IMG_H_

/**
 * @defgroup mpipe_img Image Codecs
 * @ingroup mpipe_plugins
 * @brief Elements that turn a coded image stream into frames and decode them.
 *
 * The image plugin covers the two steps between a file or a network socket and
 * something displayable. A parser finds the frame boundaries in a stream that
 * arrives as undifferentiated bytes and emits whole images; a decoder turns one
 * of those into raw pixels.
 *
 * They are separate elements because the split is real: the parser is what
 * first knows the format, and it can be followed either by the software decoder
 * here or by a hardware decoder from the video plugin, without either side
 * knowing which.
 */

#endif /* ZEPHYR_INCLUDE_MPIPE_IMG_MPIPE_IMG_H_ */
