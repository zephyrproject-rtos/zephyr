/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_EDAC_NXP_H_
#define ZEPHYR_DRIVERS_EDAC_NXP_H_
/* Callback data provided to function passed to notify_cb_set */
struct edac_nxp_callback_data {
	/* Number of corrected errors */
	uint8_t corr_err_count;
	/* Syndrome ECC bits for last single bit ECC event */
	uint8_t err_syndrome;
	/* Address of last ECC event */
	uint32_t err_addr;
	/* Type of last ECC event */
	uint32_t err_status;
};

#endif /* ZEPHYR_DRIVERS_EDAC_NXP_H_ */
