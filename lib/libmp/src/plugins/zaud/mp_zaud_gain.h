/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio gain element header file.
 */

#ifndef __MP_ZAUD_GAIN_H__
#define __MP_ZAUD_GAIN_H__

#include <src/core/mp_transform.h>

/** @brief Cast object pointer to MpZaudGain pointer */
#define MP_ZAUD_GAIN(self) ((MpZaudGain *)self)

/**
 * @struct MpZaudGain
 * @brief Audio gain element structure
 *
 * This structure represents a gain transform element.
 * It provides audio volume control with percentage-based gain settings,
 * fixed-point arithmetic for efficient processing, and mute
 * functionality.
 */
typedef struct {
	/** Base transform element structure */
	MpTransform transform;
	/** Gain level as percentage (0-1000, where 100 = unity gain) */
	int gain_percent;
	/** Gain value in Q16.16 fixed-point format for efficient computation */
	int32_t gain_fixed;
	/** Mute flag - when true, optimizes processing by bypassing gain calculations */
	bool mute;
} MpZaudGain;

/**
 * @brief Initialize an audio gain element.
 *
 * This function initializes the audio gain processing element with default
 * values and sets up the function pointers.
 *
 * @param self Pointer to the MpElement structure to be initialized as a
 *             gain element.
 */
void mp_zaud_gain_init(MpElement *self);

#endif /* __MP_ZAUD_GAIN_H__ */
