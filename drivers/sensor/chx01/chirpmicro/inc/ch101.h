/*
 * Copyright 2023 Chirp Microsystems. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file ch101.h
 *
 * @brief Internal definitions for the Chirp CH101 ultrasonic sensor.
 *
 * This file contains various hardware-defined values for the CH101 sensor.
 *
 * You should not need to edit this file or call the driver functions directly.  Doing so
 * will reduce your ability to benefit from future enhancements and releases from Chirp.
 *
 */

#ifndef CH101_H_
#define CH101_H_

#define CH101_DATA_MEM_SIZE 0x800
#define CH101_DATA_MEM_ADDR 0x0200
#define CH101_PROG_MEM_SIZE 0x800
#define CH101_PROG_MEM_ADDR 0xF800
#define CH101_FW_SIZE	    CH101_PROG_MEM_SIZE

/** Index of first sample to use for calculating bandwidth. */
#define CH101_BANDWIDTH_INDEX_1 6
/** Index of second sample to use for calculating bandwidth. */
#define CH101_BANDWIDTH_INDEX_2 (CH101_BANDWIDTH_INDEX_1 + 1)
/** Index for calculating scale factor. */
#define CH101_SCALEFACTOR_INDEX 4

#define CH101_MAX_TICK_INTERVAL 256

#endif /* CH101_H_ */
