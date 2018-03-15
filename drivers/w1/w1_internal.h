/*
 * Copyright (c) 2018 Roman Tataurov <diytronic@yandex.ru>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Driver for software driven 1-Wire using GPIO lines *
 */
#ifndef __DRIVERS_W1_INTERNAL_H
#define __DRIVERS_W1_INTERNAL_H

#include <w1.h>

/*
 * Indexes into delay table for each part of 1-Wire timing waveform.
 * Based on MAXIM 1-Wire Master Timing table
 */
#define T_A   0
#define T_B   1
#define T_C   2
#define T_D   3
#define T_E   4
#define T_F   5
#define T_G   6
#define T_H   7
#define T_I   8
#define T_J   9

/**
 * The standard timing. The approximate 60 μs length of a time slot
 * gives a speed of 1 000 000/60 = 16 666 or 16 kbps.
 */
static const u32_t delays_standard[10] = {
	[T_A] = 6,
	[T_B] = 64,
	[T_C] = 60,
	[T_D] = 10,
	[T_E] = 9,
	[T_F] = 55,
	[T_G] = 0,
	[T_H] = 490,
	[T_I] = 70,
	[T_J] = 410,
};

/**
 * Fast timing (called Overdrive) for a speedier bus with 142 kbps.
 *
 * TODO: overdrive mode not ready yet - here is just a delays for it
 */
static const u32_t delays_overdrive[10] = {
	[T_A] = 1,
	[T_B] = 8,
	[T_C] = 8,
	[T_D] = 3,
	[T_E] = 1,
	[T_F] = 7,
	[T_G] = 3,
	[T_H] = 70,
	[T_I] = 9,
	[T_J] = 40,
};


#endif /* __DRIVERS_W1_INTERNAL_H */
