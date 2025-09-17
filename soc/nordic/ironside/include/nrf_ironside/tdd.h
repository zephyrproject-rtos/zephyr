/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_TDD_H_
#define ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_TDD_H_

#include <nrf_ironside/call.h>

#include <nrfx.h>

#define IRONSIDE_SE_TDD_SERVICE_ERROR_INVALID_CONFIG (1)

#define IRONSIDE_SE_CALL_ID_TDD_V0 4

#define IRONSIDE_SE_TDD_SERVICE_REQ_CONFIG_IDX  0
#define IRONSIDE_SE_TDD_SERVICE_RSP_RETCODE_IDX 0

enum ironside_se_tdd_config {
	RESERVED0 = 0, /* Reserved */
	/** Turn off the TDD */
	IRONSIDE_SE_TDD_CONFIG_OFF = 1,
	/** Turn on the TDD with default configuration */
	IRONSIDE_SE_TDD_CONFIG_ON_DEFAULT = 2,
};

/**
 * @brief Control the Trace and Debug Domain (TDD).
 *
 * @param config The configuration to be applied.
 *
 * @retval 0 on success.
 * @retval -IRONSIDE_SE_TDD_SERVICE_ERROR_INVALID_CONFIG if the configuration is invalid.
 * @retval Positive error status if reported by IronSide call (see error codes in @ref call.h).
 */
int ironside_se_tdd_configure(const enum ironside_se_tdd_config config);

#endif /* ZEPHYR_SOC_NORDIC_IRONSIDE_INCLUDE_NRF_IRONSIDE_TDD_H_ */
