/*
 * Copyright (c) 2025 Calian Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended EDAC API of Synopsys DDR controller
 * @ingroup edac_interface_ext
 */

#ifndef ZEPHYR_DRIVERS_EDAC_EDAC_SYNOPSYS_H_
#define ZEPHYR_DRIVERS_EDAC_EDAC_SYNOPSYS_H_

/**
 * @brief Synopsys DDR controller EDAC driver
 * @defgroup edac_synopsys_interface Synopsys DDR controller EDAC
 * @ingroup edac_interface_ext
 * @{
 */

#include <stdint.h>

/** Callback data provided to function passed to @ref edac_notify_callback_set */
struct edac_synopsys_callback_data {
	/** Number of corrected errors since last callback */
	uint16_t corr_err_count;
	/** Rank number of last corrected ECC error */
	uint8_t corr_err_rank;
	/** Bank group number of last corrected ECC error */
	uint8_t corr_err_bg;
	/** Bank number of last corrected ECC error */
	uint8_t corr_err_bank;
	/** Row number of last corrected ECC error */
	uint32_t corr_err_row;
	/** Column number of last corrected ECC error */
	uint8_t corr_err_col;
	/** Syndrome (data pattern) of last corrected ECC error */
	uint64_t corr_err_syndrome;
	/** Syndrome ECC bits for last corrected ECC error */
	uint8_t corr_err_syndrome_ecc;
	/** Bitmask of corrected error bits in data word */
	uint64_t corr_err_bitmask;
	/** Bitmask of corrected error bits in ECC word */
	uint8_t corr_err_bitmask_ecc;

	/** Number of uncorrected errors since last callback */
	uint16_t uncorr_err_count;
	/** Rank number of last uncorrected ECC error */
	uint8_t uncorr_err_rank;
	/** Bank group number of last uncorrected ECC error */
	uint8_t uncorr_err_bg;
	/** Bank number of last uncorrected ECC error */
	uint8_t uncorr_err_bank;
	/** Row number of last uncorrected ECC error */
	uint32_t uncorr_err_row;
	/** Column number of last uncorrected ECC error */
	uint8_t uncorr_err_col;
	/** Syndrome (data pattern) of last uncorrected ECC error */
	uint64_t uncorr_err_syndrome;
	/** Syndrome ECC bits of last uncorrected ECC error */
	uint8_t uncorr_err_syndrome_ecc;
};

/** @} */

#endif /* ZEPHYR_DRIVERS_EDAC_EDAC_SYNOPSYS_H_ */
