/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended EDAC API of NXP Error Reporting Module (ERM)
 * @ingroup edac_interface_ext
 */

#ifndef ZEPHYR_DRIVERS_EDAC_NXP_ERM_H_
#define ZEPHYR_DRIVERS_EDAC_NXP_ERM_H_

/**
 * @brief NXP ERM EDAC driver
 * @defgroup edac_nxp_erm_interface NXP ERM EDAC
 * @ingroup edac_interface_ext
 * @{
 */

/** Callback data provided to function passed to @ref edac_notify_callback_set */
struct edac_nxp_callback_data {
	/** Number of corrected errors */
	uint8_t corr_err_count;
	/** Syndrome ECC bits for last single bit ECC event */
	uint8_t err_syndrome;
	/** Address of last ECC event */
	uint32_t err_addr;
	/** Type of last ECC event */
	uint32_t err_status;
};

/** @} */

#endif /* ZEPHYR_DRIVERS_EDAC_NXP_ERM_H_ */
