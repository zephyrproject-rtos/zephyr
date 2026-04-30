/*
 * Copyright (c) 2026 Clovis Corde
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file delta_bsdiff.h
 * @brief BSDiff+Heatshrink backend configuration.
 */
#ifndef ZEPHYR_INCLUDE_MGMT_DELTA_BSDIFF_H_
#define ZEPHYR_INCLUDE_MGMT_DELTA_BSDIFF_H_

#include <zephyr/mgmt/delta/delta_algorithm.h>

/** BSDiff+Heatshrink patch magic string. */
#define DELTA_BSDIFF_MAGIC                "BSDIFFHS"
/** Length of the magic string in bytes. */
#define DELTA_BSDIFF_MAGIC_SIZE           8
/** Length of the target firmware size field in bytes. */
#define DELTA_BSDIFF_TARGET_SIZE_LEN      8
/** Length of the heatshrink window_sz2 field in bytes. */
#define DELTA_BSDIFF_WINDOW_SZ2_LEN      1
/** Length of the heatshrink lookahead_sz2 field in bytes. */
#define DELTA_BSDIFF_LOOKAHEAD_SZ2_LEN   1
/** Total header size: magic + target size + window_sz2 + lookahead_sz2. */
#define DELTA_BSDIFF_HEADER_SIZE                                                                   \
	(DELTA_BSDIFF_MAGIC_SIZE + DELTA_BSDIFF_TARGET_SIZE_LEN +                                 \
	 DELTA_BSDIFF_WINDOW_SZ2_LEN + DELTA_BSDIFF_LOOKAHEAD_SZ2_LEN)

/** @brief BSDiff+Heatshrink backend configuration. */
struct delta_bsdiff_cfg {
	/** Heatshrink decompression window size (log2) */
	uint8_t heatshrink_window_sz2;
	/** Heatshrink decompression lookahead size (log2) */
	uint8_t heatshrink_lookahead_sz2;
};

/** @brief BSDiff+Heatshrink backend API instance. */
extern const struct delta_backend_api delta_backend_bsdiff_api;

#endif /* ZEPHYR_INCLUDE_MGMT_DELTA_BSDIFF_H_ */
